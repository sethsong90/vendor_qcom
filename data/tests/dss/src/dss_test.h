/******************************************************************************

                           D S S _ T E S T . H

******************************************************************************/

/******************************************************************************

  @file    dss_test.h
  @brief   DSS API Test Suite Common Framework Header File

  DESCRIPTION
  Header file for the framework for the DSS API test suite.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2012 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/dss_test.h#1 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/05/08   vk         Added testing of net policy verification.
01/31/08   vk         Added support for Network Controller test app.
01/23/08   vk         Added support for uplink TCP data transfer tests.
09/28/07   vk         Initial version

******************************************************************************/

#ifndef __DSS_TEST_H__
#define __DSS_TEST_H__

#include <stdio.h>
#include "ds_util.h"

#ifdef FEATURE_DATA_LINUX_LE
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "comdef.h"
#include "dssocket_defs.h"
#include "dss_netpolicy.h"
#include "dss_iface_ioctl.h"
#endif

#include "dssocket.h"

#define ARR_SIZ(x) (sizeof(x)/sizeof(x[0]))

#define DSS_TEST_DEFAULT_PDP_PROF   1
#define DSS_TEST_PDP_PROF_1         DSS_TEST_DEFAULT_PDP_PROF
#define DSS_TEST_PDP_PROF_2         (DSS_TEST_PDP_PROF_1 + 1)
#define DSS_TEST_PDP_PROF_3         (DSS_TEST_PDP_PROF_2 + 1)
#define DSS_TEST_PDP_PROF_1_MATCH   (DSS_TEST_PDP_PROF_3 + 1)
#define DSS_TEST_PDP_PROF_2_MATCH   (DSS_TEST_PDP_PROF_1_MATCH + 1)
#define DSS_TEST_PDP_PROF_3_MATCH   (DSS_TEST_PDP_PROF_2_MATCH + 1)

#define DSS_TEST_USER_DATA_PTR_DEF_VAL  ((void *)0x57891234)

#define DSS_TEST_TECH_ANY   0
#define DSS_TEST_TECH_UMTS  1
#define DSS_TEST_TECH_CDMA  2

#define DSS_TEST_IPV4_TYPE 4
#define DSS_TEST_IPV6_TYPE 6

typedef enum {
    DSS_TEST_PASS = 0,
    DSS_TEST_FAIL = 1,
    DSS_TEST_ABORT = 2,
    DSS_TEST_SKIP = 3
} dss_test_status_t;

typedef enum {
    DSS_TEST_ARG_STR,
    DSS_TEST_ARG_INT, 
    DSS_TEST_ARG_VOID
} dss_test_arg_t;

typedef struct dss_test_prog_args_s {
    char * name;
    char * str_val;
    char   short_name;
    dss_test_arg_t type;
    union {
        char * str_val;
        int int_val;
    } val;
} dss_test_prog_args_t;

typedef struct dss_test_prog_args_template_s {
    char          * name;
    char            short_name;
    dss_test_arg_t  type;
} dss_test_prog_args_template_t;

extern dss_test_status_t Test_status;
extern dss_test_prog_args_t Prog_args[];
extern int Prog_args_size;
extern dss_test_prog_args_template_t Prog_args_template[];

#ifdef FEATURE_DATA_LINUX_LE
#define dss_test_log(...) ds_log_high(__VA_ARGS__)
#else
#define dss_test_log(...) ds_log(__VA_ARGS__)
#endif

#define dss_test_fail()   { Test_status = DSS_TEST_FAIL; }
#define dss_test_pass()   { Test_status = DSS_TEST_PASS; }
#define dss_test_abort()  { Test_status = DSS_TEST_ABORT; }

#define dss_test_is_pass() ( Test_status == DSS_TEST_PASS )
#define dss_test_is_fail() ( Test_status == DSS_TEST_FAIL )
#define dss_test_is_abort() ( Test_status == DSS_TEST_ABORT )

typedef enum {
    DSS_TEST_PEER_MODE_SRVR = 0,
    DSS_TEST_PEER_MODE_CLNT
} dss_test_peer_mode_t;

FILE * 
dss_test_cfg_get_log_fp (void);

int 
dss_test_parse_prog_args 
(
    int argc, 
    char ** argv, 
    const dss_test_prog_args_template_t * arg_template_arr,
    int   arg_template_arr_sz,
    dss_test_prog_args_t * arg_arr,
    int   arg_arr_sz
);

const dss_test_prog_args_t * 
dss_test_find_prog_args 
(
    char short_name, 
    const char * argname, 
    const dss_test_prog_args_t * arg_arr, 
    int   arg_arr_sz
);

int 
dss_test_prog_args_get_val_int (const dss_test_prog_args_t * arg_ptr);

void dss_test_id_set   (const char * test_id);
void dss_test_name_set (const char * test_name);
void dss_test_desc_set (const char * test_desc);
void dss_test_tech_set (int tech);

#define dss_test_set_def(a,b,c,d) \
            dss_test_id_set(a); \
            dss_test_name_set(b); \
            dss_test_desc_set(c); \
            dss_test_tech_set(d);

int dss_test_tech_get (void);

dss_iface_name_enum_type dss_test_get_if_name_for_tech (int tech);

void dss_test_init (int argc, char ** argv);
const char * dss_test_prog_args_get_val_str (const dss_test_prog_args_t * arg_ptr);
void dss_test_enter (void);
int dss_test_exit (void);
int dss_test_wait_for_net_event (sint15 dss_nh);

int dss_test_dss_init_net_policy_info (dss_net_policy_info_type * net_policy);
int 
dss_test_dsnet_set_policy 
(
    sint15 dss_nh, 
    dss_net_policy_info_type * net_policy,
    sint15 dss_status_val,
    sint15 dss_errno_val
);
int dss_test_dsnet_get_handle (dss_net_policy_info_type * net_policy, sint15 * dss_nh);
int
dss_test_ex_dsnet_get_handle 
(
    dss_net_policy_info_type * net_policy, 
    sint15 * dss_nh, 
    sint15 dss_status_val, 
    sint15 dss_errno_val
);
int dss_test_dsnet_release_handle (sint15 dss_nh);
int dss_test_dsnet_start (sint15 dss_nh);
int dss_test_dss_netstatus (sint15 dss_nh, sint15 dss_errno_val);
int dss_test_dss_get_iface_id (sint15 dss_nh, dss_iface_id_type * if_id, sint15 dss_status_val);
int dss_test_dss_get_iface_id_by_policy
(
    const dss_net_policy_info_type * net_policy, 
    dss_iface_id_type * if_id, 
    sint15 dss_status_val,
    sint15 dss_errno_val
);
int dss_test_dss_iface_ioctl_get_iface_name
(
    dss_iface_id_type        iface_id,
    dss_iface_name_enum_type iface_name,
    sint15                   dss_status_val,
    sint15                   dss_errno_val
);
int
dss_test_dss_iface_ioctl_get_state
(
    dss_iface_id_type           iface_id,
    dss_iface_ioctl_state_type  iface_state,
    sint15                      dss_status_val,
    sint15                      dss_errno_val
);
int dss_test_dss_iface_ioctl_reg_event_cb
(
    dss_iface_id_type            iface_id,
    dss_iface_ioctl_ev_cb_type * iface_ev_cb,
    sint15                       dss_status_val,
    sint15                       dss_errno_val
);
int dss_test_dss_iface_ioctl_dereg_event_cb
(
    dss_iface_id_type            iface_id,
    dss_iface_ioctl_ev_cb_type * iface_ev_cb,
    sint15                       dss_status_val,
    sint15                       dss_errno_val
);

int dss_test_dsnet_start_sync (sint15 dss_nh, int dss_status_val, sint15 dss_errno_val);
int dss_test_dsnet_stop_sync (sint15 dss_nh, int dss_status_val, sint15 dss_errno_val);

int 
dss_test_dss_socket 
(
    sint15 dss_nh,
    byte   family,
    byte   type,
    byte   protocol,
    int    dss_status_val,
    sint15 dss_errno_val,
    sint15 * dss_fd
);

int 
dss_test_dss_connect_sync 
(
    sint15 dss_nh, 
    sint15 dss_fd, 
    int dss_status_val, 
    sint15 dss_errno_val
);

int dss_test_udp_client_open (sint15 dss_nh, sint15 * dss_fd);
int dss_test_udp_client_close (sint15 dss_fd);
int dss_test_udp_client_send_file (sint15 dss_nh, sint15 dss_fd);

int dss_test_tcp_client_open (sint15 dss_nh, sint15 * dss_fd);
int dss_test_tcp_client_close (sint15 dss_fd);
int dss_test_tcp_client_send_file (sint15 dss_nh, sint15 dss_fd);

int dss_test_match_nh_ifaces(sint15 dss_nh_1, sint15 dss_nh_2, sint15 dss_status_val);
int dss_test_match_nh_ifaces_3
(
    sint15 dss_nh_1, 
    sint15 dss_nh_2, 
    sint15 dss_nh_3, 
    sint15 dss_status_val
);

int 
dss_test_peer_read_buf_complete (int fd, char * buf, size_t lmsg);

int 
dss_test_peer_write_buf_complete (int fd, const char * buf, size_t lmsg);

void 
dss_test_master_client (void);

#endif /* __DSS_TEST_H__ */
