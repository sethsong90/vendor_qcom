/******************************************************************************

                        D S C _ T E S T . C

******************************************************************************/

/******************************************************************************

  @file    dsc_test.c
  @brief   Test code for DSC

  DESCRIPTION
  Implementation of some test code for DSC.

  ---------------------------------------------------------------------------
  Copyright (c) 2007 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/src/dsc_test.c#2 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
12/18/07   vk         Support for automatic client deregistration
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef FEATURE_DS_DEBUG_MTRACE
#include <mcheck.h>
#endif
#include "dsci.h"
#include "dsc_util.h"
#include "dsc_cmd.h"
#include "dsc_call.h"
#include "dsc_qmi_wds.h"
#include "dsc_kif.h"
#include "dsc_dcmi.h"
#if 0
#include "dssocket.h"
#endif

dsc_cmd_t dsc_test_cmd_hello[5];

void dsc_test_hello_execute(dsc_cmd_t * cmd, void * data)
{
    fprintf(stderr, "hello! %d\n", (int)data);
}

void dsc_test_hello_free(dsc_cmd_t * cmd, void * data)
{
    fprintf(stderr, "bye! %d\n", (int)data);
}

void dsc_test_hello_create(int i) {
    dsc_test_cmd_hello[i].execute_f = dsc_test_hello_execute;
    dsc_test_cmd_hello[i].free_f = dsc_test_hello_free;
    dsc_test_cmd_hello[i].data = (void *)i;
}

void dsc_test_hello_post(int i) {
    dsc_cmdq_enq(&dsc_test_cmd_hello[i]);
}

void dsc_test_hello (void) {
    int i;

    for (i = 0; i < 5; ++i) {
        dsc_test_hello_create(i);
        dsc_test_hello_post(i);
    }
    return;
}

#if 0
void dsc_test (void) {
    char buf[128];
    int  num;

    while (1) {
        scanf("%s", buf);
        if (strncmp(buf, "hello", 5) == 0) {
            dsc_test_hello();
        } else if (strncmp(buf, "exit", 4) == 0) {
            break;
        }
    }
    return;
}
#endif

typedef int (* dsc_test_cmd_f) (int arg1, int arg2);

void dsc_pricall_wds_start_interface_cnf (int link, dsc_op_status_t status);
void dsc_pricall_wds_stop_interface_cnf (int link, dsc_op_status_t status);
void dsc_pricall_wds_stop_interface_ind (int link);

void dsc_pricall_kif_opened (int link, dsc_op_status_t status, void * clnt_hdl);
void dsc_pricall_kif_closed (int link, dsc_op_status_t status, void * clnt_hdl);

#if 0
int dsc_callmgr_alloc_pricall_test (int arg1, int arg2) {
	int callid;
	callid = dsc_callmgr_alloc_pricall();
	return callid;
}

int dsc_callmgr_free_pricall_test (int arg1, int arg2) {
	dsc_callmgr_free_pricall((dsc_callid_t)arg1);
	return 0;
}
#endif

int dsc_pricall_connect_req_test (int arg1, int arg2) {
    return dsc_pricall_connect_req((dsc_callid_t)arg1);
}

int dsc_pricall_disconnect_req_test (int arg1, int arg2) {
    return dsc_pricall_disconnect_req((dsc_callid_t)arg1);
}

int dsc_pricall_wds_start_interface_cnf_test (int arg1, int arg2) {
    dsc_pricall_wds_start_interface_cnf (arg1, (dsc_op_status_t)arg2);
    return 0;
}

int dsc_pricall_wds_stop_interface_cnf_test (int arg1, int arg2) {
    dsc_pricall_wds_stop_interface_cnf (arg1, (dsc_op_status_t)arg2);
    return 0;
}

int dsc_pricall_wds_stop_interface_ind_test (int arg1, int arg2) {
    dsc_pricall_wds_stop_interface_ind (arg1);
    return 0;
}

int dsc_pricall_kif_opened_test (int arg1, int arg2) {
    dsc_pricall_kif_opened (arg1, (dsc_op_status_t)arg2, 0);
    return 0;
}

int dsc_pricall_kif_closed_test (int arg1, int arg2) {
    dsc_pricall_kif_closed (arg1, (dsc_op_status_t)arg2, 0);
    return 0;
}

struct dsc_test_cmd_s {
    dsc_test_cmd_f cmd_f;
    char * desc;
};

#if 0
struct dsc_test_cmd_s dsc_test_cmd_arr[] = {
	/*dsc_callmgr_alloc_pricall_test, "dsc_callmgr_alloc_pricall_test <dummy> <dummy>",*/
	/*dsc_callmgr_free_pricall_test, "dsc_callmgr_free_pricall_test <callid> <dummy>", */
    dsc_pricall_connect_req_test, "dsc_pricall_connect_req_test <callid> <dummy>",
    dsc_pricall_disconnect_req_test, "dsc_pricall_disconnect_req_test <callid> <dummy>",
    dsc_pricall_wds_start_interface_cnf_test, "dsc_pricall_wds_start_interface_cnf_test <link> <status>",
    dsc_pricall_wds_stop_interface_cnf_test, "dsc_pricall_wds_stop_interface_cnf_test <link> <status>",
    dsc_pricall_wds_stop_interface_ind_test, "dsc_pricall_wds_stop_interface_ind_test <link> <status>",
    dsc_pricall_kif_opened_test, "dsc_pricall_kif_opened_test <link> <status>",
    dsc_pricall_kif_closed_test, "dsc_pricall_kif_closed_test <link> <status>"
};
#endif

void dsc_kif_opened_test (int link, dsc_op_status_t status, void * clnt_hdl)
{
    printf("received open_cb, link %d, status %d\n", link, status);
}

void dsc_kif_closed_test (int link, dsc_op_status_t status, void * clnt_hdl)
{
    printf("received close_cb, link %d, status %d\n", link, status);
}

dsc_kif_clntcb_t dsc_test_ki_cb = {
    .opened_cb = &dsc_kif_opened_test,
    .closed_cb = &dsc_kif_closed_test
};

int dsc_kif_open_test (int arg1, int arg2) {
    dsc_kif_open (arg1, &dsc_test_ki_cb, 0);
    return 0;
}

int dsc_kif_close_test (int arg1, int arg2) {
    dsc_kif_close (arg1);
    return 0;
}

#if 0
struct dsc_test_cmd_s dsc_test_cmd_arr[] = {
	dsc_kif_open_test, "dsc_kif_open_test <link> <dummy>",
	dsc_kif_close_test, "dsc_kif_close_test <link> <dummy>", 
};
#endif

dcm_net_policy_info_t dsc_dcm_net_policy_test = {
    .iface.kind = DSS_IFACE_NAME,
    .iface.info.name = UMTS_IFACE,
    .umts.pdp_profile_num = 1
};

void dsc_dcm_net_cb_test 
(
    int               dcm_nethandle,                               /* Application id */
    dcm_iface_id_t    iface_id,                            /* Interfcae id structure */
    int               dss_errno,     /* type of network error, ENETISCONN, ENETNONET.*/
    void            * net_cb_user_data                       /* Call back User data  */
)
{
    printf("In dsc_dcm_net_cb_test: nh = %d, if_id = %d, errno = %d, cb_data = %d\n",
           dcm_nethandle, (int)iface_id, dss_errno, (int) net_cb_user_data);
}

void dsc_dcm_iface_ioctl_event_cb_test
(
  int                              dcm_nethandle,
  dcm_iface_id_t                   iface_id,
  dcm_iface_ioctl_event_t        * event,
  void                           * user_data
)
{
    printf("In dsc_dcm_iface_ioctl_event_cb_test: nh = %d, if_id = %d, event = %d, cb_data = %d\n",
           dcm_nethandle, (int)iface_id, event->name, (int) user_data);
}

int dsc_dcm_get_net_handle_test (int arg1, int arg2) {
    int nh;
    int    dss_errno;

    nh = dcm_get_net_handle
         (
            0,
            &dsc_dcm_net_cb_test, 
            (void *)1, 
            &dsc_dcm_iface_ioctl_event_cb_test,
            (void *)2,
            &dss_errno
         );
    return nh;
}

int dsc_dcm_release_net_handle_test (int arg1, int arg2) {
    int    dss_errno; 
    dsc_op_status_t status;

    status = dcm_release_net_handle(arg1, &dss_errno);
    return status;
}

int dsc_dcm_net_open_test (int arg1, int arg2) {
    int    dss_errno;
    dsc_op_status_t status;

    status = dcm_net_open(arg1, &dsc_dcm_net_policy_test, &dss_errno);
    return status;
}

int dsc_dcm_net_close_test (int arg1, int arg2) {
    int    dss_errno;
    dsc_op_status_t status;

    status = dcm_net_close(arg1, &dss_errno);
    return status;
}

int dsc_dcm_reg_event_test (int nh, int if_id) {
    int dss_errno;
    dsc_op_status_t status;
    dcm_iface_ioctl_t ioctl;

    ioctl.name = DSS_IFACE_IOCTL_REG_EVENT_CB;
    ioctl.info.event_info.dcm_nethandle = nh;

    ioctl.info.event_info.name = DSS_IFACE_IOCTL_COMING_UP_EV;
    status = dcm_iface_ioctl(if_id, &ioctl, &dss_errno);
    if (status == DSC_OP_FAIL) {
        goto error;
    }

    ioctl.info.event_info.name = DSS_IFACE_IOCTL_GOING_DOWN_EV;
    status = dcm_iface_ioctl(if_id, &ioctl, &dss_errno);
    if (status == DSC_OP_FAIL) {
        goto error;
    }

    ioctl.info.event_info.name = DSS_IFACE_IOCTL_UP_EV;
    status = dcm_iface_ioctl(if_id, &ioctl, &dss_errno);
    if (status == DSC_OP_FAIL) {
        goto error;
    }

    ioctl.info.event_info.name = DSS_IFACE_IOCTL_DOWN_EV;
    status = dcm_iface_ioctl(if_id, &ioctl, &dss_errno);
    if (status == DSC_OP_FAIL) {
        goto error;
    }

error:
    return status;
}

int dsc_dcm_dereg_event_test (int nh, int if_id) {
    int dss_errno;
    dsc_op_status_t status;
    dcm_iface_ioctl_t ioctl;

    ioctl.name = DSS_IFACE_IOCTL_DEREG_EVENT_CB;
    ioctl.info.event_info.dcm_nethandle = nh;

    ioctl.info.event_info.name = DSS_IFACE_IOCTL_COMING_UP_EV;
    status = dcm_iface_ioctl(if_id, &ioctl, &dss_errno);
    if (status == DSC_OP_FAIL) {
        goto error;
    }

    ioctl.info.event_info.name = DSS_IFACE_IOCTL_GOING_DOWN_EV;
    status = dcm_iface_ioctl(if_id, &ioctl, &dss_errno);
    if (status == DSC_OP_FAIL) {
        goto error;
    }

    ioctl.info.event_info.name = DSS_IFACE_IOCTL_UP_EV;
    status = dcm_iface_ioctl(if_id, &ioctl, &dss_errno);
    if (status == DSC_OP_FAIL) {
        goto error;
    }

    ioctl.info.event_info.name = DSS_IFACE_IOCTL_DOWN_EV;
    status = dcm_iface_ioctl(if_id, &ioctl, &dss_errno);
    if (status == DSC_OP_FAIL) {
        goto error;
    }

error:
    return status;
}

int dsc_dcm_ioctl_get_if_name_test (int if_id, int dummy) {
    int dss_errno;

    dsc_op_status_t status;
    dcm_iface_ioctl_t ioctl;

    ioctl.name = DSS_IFACE_IOCTL_GET_IFACE_NAME;
    status = dcm_iface_ioctl(if_id, &ioctl, &dss_errno);
    if (status == DSC_OP_FAIL) {
        goto error;
    }

    printf("name = %x\n", ioctl.info.if_name);

error:
    return status;
}

int dsc_dcm_ioctl_get_if_state_test (int if_id, int dummy) {
    int dss_errno;
    dsc_op_status_t status;
    dcm_iface_ioctl_t ioctl;

    ioctl.name = DSS_IFACE_IOCTL_GET_STATE;
    status = dcm_iface_ioctl(if_id, &ioctl, &dss_errno);
    if (status == DSC_OP_FAIL) {
        goto error;
    }

    printf("state = %x\n", ioctl.info.if_state);

error:
    return status;
}

int dsc_dcm_ioctl_get_if_addr_test (int if_id, int dummy) {
    int dss_errno;
    dsc_op_status_t status;
    dcm_iface_ioctl_t ioctl;
	struct in_addr inaddr;

    ioctl.name = DSS_IFACE_IOCTL_GET_IPV4_ADDR;
    status = dcm_iface_ioctl(if_id, &ioctl, &dss_errno);
    if (status == DSC_OP_FAIL) {
        goto error;
    }

    printf("addr = %lx\n", ioctl.info.ipv4_addr.addr.v4);

	inaddr.s_addr = ioctl.info.ipv4_addr.addr.v4;
	printf("addr %s\n", inet_ntoa(inaddr));

error:
    return status;
}

#if 1
struct dsc_test_cmd_s dsc_test_cmd_arr[] = {
	{ dsc_dcm_get_net_handle_test, "dsc_dcm_get_net_handle_test <dummy> <dummy>" },
	{ dsc_dcm_release_net_handle_test, "dsc_dcm_release_net_handle_test <nh> <dummy>" },
	{ dsc_dcm_net_open_test, "dsc_dcm_net_open_test <nh> <dummy>" }, 
    { dsc_dcm_net_close_test, "dsc_dcm_net_close_test <nh> <dummy>" },
    { dsc_dcm_reg_event_test, "dsc_dcm_reg_event_test <nh> <if_id>" },
    { dsc_dcm_dereg_event_test, "dsc_dcm_dereg_event_test <nh> <if_id>" },
    { dsc_dcm_ioctl_get_if_name_test, "dsc_dcm_ioctl_get_if_name_test <if_id> <dummy>" },
    { dsc_dcm_ioctl_get_if_state_test, "dsc_dcm_ioctl_get_if_state_test <if_id> <dummy>" },
    { dsc_dcm_ioctl_get_if_addr_test, "dsc_dcm_ioctl_get_if_addr_test <if_id> <dummy>" },
    { dsc_pricall_wds_start_interface_cnf_test, "dsc_pricall_wds_start_interface_cnf_test <link> <status>" },
    { dsc_pricall_wds_stop_interface_cnf_test, "dsc_pricall_wds_stop_interface_cnf_test <link> <status>" },
    { dsc_pricall_wds_stop_interface_ind_test, "dsc_pricall_wds_stop_interface_ind_test <link> <status>" }
};
#endif

#if 0
void dss_test_net_cb_fcn
(
  sint15            dss_nethandle,                       /* Application id */
  dss_iface_id_type iface_id,                    /* Interfcae id structure */
  sint15            dss_errno, /* type of network error, ENETISCONN, ENETNONET.*/
  void            * net_cb_user_data               /* Call back User data  */
)
{
    printf("In dss_test_net_cb_fcn: nethandle = %d, iface_id = %ld, errno = %d, net_cb_user_data = %d\n",
           dss_nethandle, iface_id, dss_errno, (int) net_cb_user_data);
}

void dss_test_sock_cb_fcn
(
  sint15 dss_nethandle,                                  /* Application id */
  sint15 sockfd,                                      /* socket descriptor */
  uint32 event_mask,                                     /* Event occurred */
  void * sock_cb_user_data       /* User data specfied during registration */
)
{
    return;
}

int dss_test_net_cb_user_data = 5;
int dss_test_sock_cb_user_data = 7;

int dsnet_get_handle_test (int arg1, int arg2) {
    dss_net_policy_info_type net_policy;
    sint15 dss_errno;

    dss_init_net_policy_info(&net_policy);

    net_policy.iface.kind = DSS_IFACE_NAME;
    net_policy.iface.info.name = DSS_IFACE_UMTS;
    net_policy.umts.pdp_profile_num = 1;

    return dsnet_get_handle(&dss_test_net_cb_fcn, (void *)dss_test_net_cb_user_data,
                            &dss_test_sock_cb_fcn, (void *)dss_test_sock_cb_user_data,
                            &net_policy, &dss_errno);
}

int dsnet_release_handle_test (int arg1, int arg2) {
    sint15 dss_errno;

    return dsnet_release_handle(arg1, &dss_errno);
}

int dsnet_start_test (int arg1, int arg2) {
    sint15 dss_errno;

    return dsnet_start(arg1, &dss_errno);
}

int dsnet_stop_test (int arg1, int arg2) {
    sint15 dss_errno;

    return dsnet_stop(arg1, &dss_errno);
}

int dss_netstatus_test (int arg1, int arg2) {
    sint15 dss_errno;

    return dss_netstatus(arg1, &dss_errno);
}

int dss_get_iface_id_test (int arg1, int arg2) {
    return dss_get_iface_id(arg1);
}

void dss_ev_cb_test_func
(
  dss_iface_ioctl_event_enum_type          event,
  dss_iface_ioctl_event_info_union_type    event_info,
  void                                     *user_data,
  signed short int                         dss_nethandle,
  dss_iface_id_type                        iface_id
)
{
    printf("dss_ev_cb_test_func: event = %d, user_data = %d, nh = %d, if_id = %ld\n",
           event, (int)user_data, dss_nethandle, iface_id);
}

int dss_reg_ev_cb_test (int nh, int if_id) {
    sint15 dss_errno;
    dss_iface_ioctl_ev_cb_type ev_cb;
    int rval = -1;

    ev_cb.event_cb = &dss_ev_cb_test_func;
    ev_cb.user_data_ptr = (void *)5;
    ev_cb.dss_nethandle = nh;

    ev_cb.event = DSS_IFACE_IOCTL_DOWN_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_REG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    ev_cb.event = DSS_IFACE_IOCTL_UP_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_REG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    ev_cb.event = DSS_IFACE_IOCTL_GOING_DOWN_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_REG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    ev_cb.event = DSS_IFACE_IOCTL_COMING_UP_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_REG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    return rval;
}

int dss_dereg_ev_cb_test (int nh, int if_id) {
    sint15 dss_errno;
    dss_iface_ioctl_ev_cb_type ev_cb;
    int rval = -1;

    ev_cb.event_cb = &dss_ev_cb_test_func;
    ev_cb.user_data_ptr = (void *)5;
    ev_cb.dss_nethandle = nh;

    ev_cb.event = DSS_IFACE_IOCTL_DOWN_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_DEREG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    ev_cb.event = DSS_IFACE_IOCTL_UP_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_DEREG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    ev_cb.event = DSS_IFACE_IOCTL_GOING_DOWN_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_DEREG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    ev_cb.event = DSS_IFACE_IOCTL_COMING_UP_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_DEREG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    return rval;
}

int dss_ioctl_get_if_name_test (int if_id, int arg2) {
    sint15 dss_errno;
    int rval = -1;
    dss_iface_ioctl_iface_name_type name;

    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_GET_IFACE_NAME,
                           &name,
                           &dss_errno);

    if (rval == 0) {
        printf("name = %x\n", name);
    }
    return rval;
}

int dss_ioctl_get_if_state_test (int if_id, int arg2) {
    sint15 dss_errno;
    int rval = -1;
    dss_iface_ioctl_state_type state;

    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_GET_STATE,
                           &state,
                           &dss_errno);

    if (rval == 0) {
        printf("state = %x\n", state);
    }
    return rval;
}

int dss_ioctl_get_if_addr_test (int if_id, int arg2) {
    sint15 dss_errno;
    int rval = -1;
    dss_iface_ioctl_ipv4_addr_type addr;
	struct in_addr inaddr;

    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_GET_IPV4_ADDR,
                           &addr,
                           &dss_errno);

    if (rval == 0) {
        printf("addr = %lx\n", addr.addr.v4);

		inaddr.s_addr = addr.addr.v4;
		printf("addr %s\n", inet_ntoa(inaddr));
    }
    return rval;
}

struct dsc_test_cmd_s dsc_test_cmd_arr[] = {
	{ dsnet_get_handle_test, "dsnet_get_handle_test <dummy> <dummy>" },
	{ dsnet_release_handle_test, "dsnet_release_handle_test <nh> <dummy>" },
	{ dsnet_start_test, "dsnet_start_test <nh> <dummy>" }, 
    { dsnet_stop_test, "dsnet_stop_test <nh> <dummy>" },
    { dss_netstatus_test, "dss_netstatus_test <nh> <dummy>" },
    { dss_get_iface_id_test, "dss_get_iface_id_test <nh> <dummy>" },
    { dss_reg_ev_cb_test, "dss_reg_ev_cb_test <nh> <if_id>" },
    { dss_dereg_ev_cb_test, "dss_dereg_ev_cb_test <nh> <if_id>" },
    { dss_ioctl_get_if_name_test, "dss_ioctl_get_if_name_test <if_id>" },
    { dss_ioctl_get_if_state_test, "dss_ioctl_get_if_state_test <if_id>" },
    { dss_ioctl_get_if_addr_test, "dss_ioctl_get_if_addr_test <if_id>" }, 
    { dsc_pricall_wds_start_interface_cnf_test, "dsc_pricall_wds_start_interface_cnf_test <link> <status>" },
    { dsc_pricall_wds_stop_interface_cnf_test, "dsc_pricall_wds_stop_interface_cnf_test <link> <status>" },
    { dsc_pricall_wds_stop_interface_ind_test, "dsc_pricall_wds_stop_interface_ind_test <link> <status>" }
};
#endif

void dsc_test_cmd_print (void)
{
    int i;

    for (i = 0; i < DSC_ARRSIZE(dsc_test_cmd_arr); ++i) {
        printf("%d %s\n", i+1, dsc_test_cmd_arr[i].desc);
    }
    return;
}


void dsc_test (void)
{
    char buf[128];
    int ret;
    int cmd;
    int arg1, arg2;

    while (1) {
        dsc_test_cmd_print();
        fgets(buf, 128, stdin);
        ret = sscanf(buf, "%d %d %d", &cmd, &arg1, &arg2);
        if (ret < 3) {
            if (cmd == 0) {
                break;
            } else {
                printf("invalid input. ignoring..\n");
            }
        } else {
            ret = (* dsc_test_cmd_arr[cmd-1].cmd_f)(arg1, arg2);
            printf("%s returned %d\n", dsc_test_cmd_arr[cmd-1].desc, ret);
        }
    }
    return;
}
 

dsc_socklthrd_hdl_t dsc_test_socklthrd_hdl;
dsc_socklthrd_fdmap_t dsc_test_socklthrd_fdmap[3];

void dsc_test_socklthrd_read(int fd)
{
    printf("dsc_test_socklthrd_read called for fd %d\n", fd);
}

#if 0
void dsc_test (void)
{
    int i;
    int maxfd = 3;

    if (dsc_socklthrd_init(&dsc_test_socklthrd_hdl, dsc_test_socklthrd_fdmap, maxfd) < 0) {
        printf("dsc_socklthrd_init failed\n");
        return;
    }
    for (i = 0; i < maxfd; ++i) {
        if (dsc_socklthrd_addfd(&dsc_test_socklthrd_hdl, i, dsc_test_socklthrd_read) < 0) {
            printf("dsc_socklthrd_addfd failed for fd %d\n", i);
            return;
        }
    }
    sleep(10);
    return;
}
#endif

void dsc_test_init (void)
{
#ifdef FEATURE_DS_DEBUG_MTRACE
	mtrace();
#endif
	return;
}
