/******************************************************************************

                      D S S _ T E S T _ N E T C . C

******************************************************************************/

/******************************************************************************

  @file    dss_test_netc.c
  @brief   DSS Test Network Controller App

  DESCRIPTION
  App to control data network interfaces for native linux sockets testing.

  ---------------------------------------------------------------------------
  Copyright (c) 2008-2012 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/dss_test_netc.c#2 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/10/08   vk         Updates for CDMA test support
01/31/08   vk         Initial version

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include "dss_test.h"

#define NETC_MAX_DNS_ADDRS 2
#define NETC_MAX_COMMAND_STR_LEN  200
#define NETC_DNS_INFO_FILE_PATH  "/etc/resolv.conf"

const char Test_id[]   = "dss_test_netc";
const char Test_name[] = "DSS Test Net Ctrl App";
const char Test_desc[] = "DSS Test Net Ctrl App";

typedef enum {
    NETC_CMD_INVALID = 0,
    NETC_CMD_EXIT,
    NETC_CMD_NETUP,
    NETC_CMD_NETDOWN
} netc_cmd_t;

typedef enum {
    NETC_STATE_IDLE = 0,
    NETC_STATE_NETUP
} netc_state_t;

typedef struct {
    int signo;
} netc_signal_msg_t;

static netc_state_t State = NETC_STATE_IDLE;

static sint15 Dss_nh;

struct {
    int             pipefd[2];
} netc_cmd_ctrl;

static __inline__ void 
netc_state_set (netc_state_t new_state) {
    State = new_state;
}

static __inline__ netc_state_t 
netc_state_get (void) {
    return State;
}

static __inline__ void 
netc_dss_nh_set (sint15 dss_nh) {
    Dss_nh = dss_nh;
}

static __inline__ sint15 
netc_dss_nh_get (void) {
    return Dss_nh;
}

static void
netc_signal_handler (int signo)
{
    netc_signal_msg_t signal_msg;
    int nwrite;

    signal_msg.signo = signo;

    nwrite = dss_test_peer_write_buf_complete
             (
                netc_cmd_ctrl.pipefd[1], 
                (const char *)&signal_msg, 
                sizeof(signal_msg)
             );

    assert(nwrite == sizeof(signal_msg));
    return;
}

static void 
netc_sig_init (void)
{
    struct sigaction signal_action;

    assert(pipe(netc_cmd_ctrl.pipefd) == 0);

    signal_action.sa_handler = netc_signal_handler;
    assert(sigemptyset(&signal_action.sa_mask) == 0);

    /* signal_action.sa_flags = SA_RESTART; */
    signal_action.sa_flags = 0;

    assert(sigaction(SIGUSR1, &signal_action, NULL) == 0);
    assert(sigaction(SIGUSR2, &signal_action, NULL) == 0);
    assert(sigaction(SIGTERM, &signal_action, NULL) == 0);

    return;
}

netc_cmd_t 
netc_signal_msg_to_cmd (netc_signal_msg_t * signal_msg)
{
    netc_cmd_t cmd;

    switch (signal_msg->signo) {
    case SIGUSR1:
        cmd = NETC_CMD_NETUP;
        break;
    case SIGUSR2:
        cmd = NETC_CMD_NETDOWN;
        break;
    case SIGTERM:
        cmd = NETC_CMD_EXIT;
        break;
    default:
        cmd = NETC_CMD_INVALID;
        break;
    }
    return cmd;
}

netc_cmd_t
netc_read_cmd (void)
{
    netc_cmd_t cmd;
    int nread;
    netc_signal_msg_t signal_msg;

    memset(&cmd, 0, sizeof(cmd));

    for (;;) {
        nread = dss_test_peer_read_buf_complete
                (
                    netc_cmd_ctrl.pipefd[0], 
                    (char *)&signal_msg, 
                    sizeof(signal_msg)
                );

        if (nread == sizeof(signal_msg)) {
            cmd = netc_signal_msg_to_cmd(&signal_msg);
            break;
        }
    }

    return cmd;
}

static void 
netc_net_init (void)
{
    dss_net_policy_info_type net_policy;
    int status;
    sint15 dss_nh;
    const dss_test_prog_args_t * arg_ptr;

    if ((status = dss_test_dss_init_net_policy_info(&net_policy)) < 0) {
        exit(1);
    }

    /* First set test specific defaults */
    net_policy.iface.kind = DSS_IFACE_NAME;
    net_policy.iface.info.name = DSS_IFACE_ANY;
    net_policy.family = DSS_AF_INET;
    net_policy.umts.pdp_profile_num = DSS_TEST_DEFAULT_PDP_PROF;

    /* check if default IP family should be overridden */
    if ((arg_ptr = dss_test_find_prog_args('v',
                                           "--family",
                                           &Prog_args[0],
                                           Prog_args_size)) != NULL) {

        switch (dss_test_prog_args_get_val_int(arg_ptr))
        {
          case DSS_TEST_IPV4_TYPE:
            {
              net_policy.family = DSS_AF_INET;
            }
            break;

          case DSS_TEST_IPV6_TYPE:
            {
              net_policy.family = DSS_AF_INET6;
            }
            break;

          default:
            {
              dss_test_log("Unknown IP family requested, restoring to "
                            "DSS_AF_INET");
            }
            break;
        }

        dss_test_log("Setting IP family in policy to:%d", net_policy.family);
    }

    if (dss_test_tech_get() == DSS_TEST_TECH_UMTS) {

        net_policy.iface.info.name = DSS_IFACE_UMTS;
        /* check if default pdp profile should be overriden */
        if ((arg_ptr = dss_test_find_prog_args('k',
                                               "--profile",
                                               &Prog_args[0],
                                               Prog_args_size)) != NULL) {

          dss_test_log("Setting profile number in policy to:%d",
                         dss_test_prog_args_get_val_int(arg_ptr));

          net_policy.umts.pdp_profile_num =
            dss_test_prog_args_get_val_int(arg_ptr);
        }
    } else if (dss_test_tech_get() == DSS_TEST_TECH_CDMA) {
        net_policy.iface.info.name = DSS_IFACE_CDMA_SN;

        /* check if default pdp profile should be overriden used in EHRPD */
        if ((arg_ptr = dss_test_find_prog_args('k',
                                               "--profile",
                                               &Prog_args[0],
                                               Prog_args_size)) != NULL) {

          dss_test_log("Setting profile number in policy to:%d",
                         dss_test_prog_args_get_val_int(arg_ptr));

          net_policy.cdma.data_session_profile_id =
            dss_test_prog_args_get_val_int(arg_ptr);
        }

    } else {
        dss_test_log("Unsupported tech type provided, "
                       "restoring to DSS_IFACE_ANY iface type");
    }

    if ((status = dss_test_dsnet_get_handle(&net_policy, &dss_nh)) < 0) {
        exit(1);
    }
    
    netc_dss_nh_set(dss_nh);
    return;
}



static int
netc_configure_routes(void)
{
    int status;
    sint15 dss_nh;
    sint15 dss_errno;
    int rval;
    dss_iface_name_enum_type if_name;
    dss_iface_ioctl_device_info_type dev_name;
    dss_iface_ioctl_ipv4_addr_type ipv4_addr;
    dss_iface_ioctl_gateway_ipv4_addr_type  gw_ip;
    char command[NETC_MAX_COMMAND_STR_LEN];
    struct in_addr addr;
    int i;
    ip_addr_type    dns_addrs[NETC_MAX_DNS_ADDRS];
    dss_iface_ioctl_get_all_dns_addrs_type   dns_info;
    dss_iface_id_type if_id;
    const dss_test_prog_args_t * arg_ptr;

    /* check if default IP family should be overridden */
    if ((arg_ptr = dss_test_find_prog_args('v',
                                           "--family",
                                           &Prog_args[0],
                                           Prog_args_size)) != NULL) {

        if (dss_test_prog_args_get_val_int(arg_ptr) == 
            DSS_TEST_IPV6_TYPE) {
            dss_test_log("Configuring rotues not supported for V6");
            return DSS_SUCCESS;
        }
    }

    dss_nh = netc_dss_nh_get();

    if ((status = dss_test_dss_get_iface_id(dss_nh, 
                                            &if_id, 
                                            DSS_SUCCESS)) < 0) {
        dss_test_log("Failed to get iface id\n");
        return DSS_ERROR;
    }

    memset(&dev_name, 0, sizeof(dev_name));

    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_GET_DEVICE_INFO, 
                           &dev_name, 
                           &dss_errno);

    if (rval == DSS_ERROR) {
        dss_test_log("Failed to get Interface/Device Name\n");
        return DSS_ERROR;
    }

    dss_test_log("device_name: %s\n", dev_name.device_name);

    memset(&ipv4_addr, 0, sizeof(ipv4_addr));
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_GET_IPV4_ADDR, 
                           &ipv4_addr, 
                           &dss_errno);

    if (rval == DSS_ERROR) {
        dss_test_log("Failed to get Interface V4 address\n");
        return DSS_ERROR;
    }

    dss_test_log("IPV4 address: %x\n", ipv4_addr.addr.v4);

    memset(&dns_info, 0, sizeof(dns_info));

    dns_info.num_dns_addrs = NETC_MAX_DNS_ADDRS;
    dns_info.dns_addrs_ptr = &dns_addrs[0];
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_GET_ALL_DNS_ADDRS, 
                           &dns_info, 
                           &dss_errno);

    if (rval == DSS_ERROR) {
        dss_test_log("Failed to get DNS INFO\n");
        return DSS_ERROR;
    }

    dss_test_log("Interface address: num_dns: %d, dns1:%x, dns2:%x\n", 
                 dns_info.num_dns_addrs, 
                 dns_addrs[0].addr.v4, 
                 dns_addrs[1].addr.v4);


    memset(&gw_ip, 0, sizeof(gw_ip));
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_GET_INTERFACE_GATEWAY_V4_ADDR, 
                           &gw_ip, 
                           &dss_errno);

    if (rval == DSS_ERROR) {
        dss_test_log("Failed to get Gateway IP\n");
        return DSS_ERROR;
    }

    /*Add default route for V4 only*/
    addr.s_addr = gw_ip.addr.v4;
    snprintf(command, 
             NETC_MAX_COMMAND_STR_LEN, 
             "route add default gw %s dev %s", 
             inet_ntoa(addr), dev_name.device_name);

    dss_test_log("%s", command);
    ds_system_call(command, strlen(command));

    /*Configure DNS*/
    memset(command, 0, NETC_MAX_COMMAND_STR_LEN);
    snprintf(command, 
             NETC_MAX_COMMAND_STR_LEN, 
             "echo -n > %s", 
             NETC_DNS_INFO_FILE_PATH);
    dss_test_log("%s", command);
    ds_system_call(command, strlen(command));

    for (i =0; i < dns_info.num_dns_addrs; i++) {
        addr.s_addr = dns_info.dns_addrs_ptr[i].addr.v4;
        memset(command, 0, NETC_MAX_COMMAND_STR_LEN);
        snprintf(command, 
                 NETC_MAX_COMMAND_STR_LEN, 
                 "echo nameserver %s >> %s", 
                 inet_ntoa(addr), NETC_DNS_INFO_FILE_PATH);
        dss_test_log("%s", command);
        ds_system_call(command, strlen(command));
    }
    return;
}

static void
netc_netup (void)
{
    int status;
    sint15 dss_nh;

    dss_nh = netc_dss_nh_get();

#ifndef FEATURE_DS_NO_DCM

    if ((status = dss_test_dsnet_start_sync(dss_nh, DSS_SUCCESS, 0)) < 0) {
        goto error;
    }

#endif /* !FEATURE_DS_NO_DCM */
    netc_configure_routes();
    netc_state_set(NETC_STATE_NETUP);

error:
    return;
}

static void 
netc_netdown (void)
{
    int status;
    sint15 dss_nh;

    dss_nh = netc_dss_nh_get();

#ifndef FEATURE_DS_NO_DCM

    if ((status = dss_test_dsnet_stop_sync(dss_nh, DSS_SUCCESS, 0)) < 0) {
        goto error;
    }

#endif /* !FEATURE_DS_NO_DCM */

    netc_state_set(NETC_STATE_IDLE);

error:
    return;
}

static void
netc_process_cmds (void)
{
    netc_cmd_t cmd; 

    while ((cmd = netc_read_cmd())) {
        dss_test_log("processing command: %d",cmd);
        switch (cmd) {
        case NETC_CMD_NETUP:
            if (netc_state_get() == NETC_STATE_IDLE) {
                netc_netup();
            }
            break;
        case NETC_CMD_NETDOWN:
            if (netc_state_get() == NETC_STATE_NETUP) {
                netc_netdown();
            }
            break;
        case NETC_CMD_EXIT:
#ifdef FEATURE_DATA_LINUX_LE
            exit(0);
            break;
#else
            goto error;
#endif
        default:
            /* ignore */
            break;
        }
    }

error:
    return;
}

static void 
netc_init (void)
{
    netc_net_init();
    netc_sig_init();
}

static void 
dss_test_netc (void)
{
    netc_init();
    netc_process_cmds();
    dss_test_pass();

    return;
}

int 
main (int argc, char * argv[])
{
    dss_test_set_def(Test_id, Test_name, Test_desc, DSS_TEST_TECH_ANY);
    dss_test_init(argc, argv);

    dss_test_enter();

    dss_test_netc();

    return dss_test_exit();
}
