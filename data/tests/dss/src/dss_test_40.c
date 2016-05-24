/******************************************************************************

                        D S S _ T E S T _ 4 0 . C

******************************************************************************/

/******************************************************************************

  @file    dss_test_40.c
  @brief   DSS API Test 40

  DESCRIPTION
  Tests EVENT REG/DEREG for IFACE state change events. 

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/dss_test_40.c#1 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/10/08   vk         Updates for CDMA test support
09/28/07   vk         Initial version

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dss_test.h"

const char Test_id[]   = "dss_test_40";
const char Test_name[] = "test event reg/dereg for IFACE state change events";
const char Test_desc[] = "test event reg/dereg for IFACE state change events";

static dss_iface_ioctl_state_type Iface_state = IFACE_DOWN;
static dss_iface_id_type          Iface_id    = DSS_IFACE_INVALID_ID;

static void
dss_test_ev_cb_func
(
    dss_iface_ioctl_event_enum_type          event,
    dss_iface_ioctl_event_info_union_type    event_info,
    void                                     *user_data,
    sint15                                   dss_nethandle,
    dss_iface_id_type                        iface_id
)
{
    (void)event_info;

    dss_test_log("In dss_test_ev_cb_func: nethandle = %d, iface_id = %ld, user_data = %d\n",
           dss_nethandle, iface_id, (int) user_data);

    if (user_data != DSS_TEST_USER_DATA_PTR_DEF_VAL) {
        Iface_state = IFACE_STATE_INVALID;
        return;
    }

    switch (Iface_state) {
    case IFACE_DOWN:
        if (event == DSS_IFACE_IOCTL_COMING_UP_EV) {
            Iface_state = IFACE_COMING_UP;
        }
        break;
    case IFACE_COMING_UP:
        if (event == DSS_IFACE_IOCTL_UP_EV) {
            Iface_state = IFACE_UP;
        }
        break;
    case IFACE_UP:
        if (event == DSS_IFACE_IOCTL_GOING_DOWN_EV) {
            Iface_state = IFACE_GOING_DOWN;
        }
        break;
    case IFACE_GOING_DOWN:
        if (event == DSS_IFACE_IOCTL_DOWN_EV) {
            Iface_state = IFACE_DOWN;
        }
        break;
    default:
        Iface_state = IFACE_STATE_INVALID;
    }

    return;
}

void
dss_test_40 (void)
{
    dss_net_policy_info_type net_policy;
    int status;
    sint15 dss_nh;
    dss_iface_id_type iface;
    dss_iface_ioctl_ev_cb_type reg_cb, dereg_cb;

    if ((status = dss_test_dss_init_net_policy_info(&net_policy)) < 0) {
        dss_test_fail();
        goto error;
    }

    net_policy.iface.kind = DSS_IFACE_NAME;
    net_policy.iface.info.name = DSS_IFACE_ANY;

    if ((status = dss_test_dss_get_iface_id_by_policy(&net_policy, &iface, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error;
    }

    net_policy.iface.kind = DSS_IFACE_ID;
    net_policy.iface.info.id = iface;

    if ((status = dss_test_dsnet_get_handle(&net_policy, &dss_nh)) < 0) {
        dss_test_fail();
        goto error;
    }

    Iface_id = iface;

	#ifndef FEATURE_DS_NO_DCM

    reg_cb.event_cb = &dss_test_ev_cb_func;
    reg_cb.user_data_ptr = DSS_TEST_USER_DATA_PTR_DEF_VAL;
    reg_cb.app_id = dss_nh;

    dereg_cb.event_cb = &dss_test_ev_cb_func;
    dereg_cb.app_id = dss_nh;

    reg_cb.event = DSS_IFACE_IOCTL_COMING_UP_EV;

    if ((status = dss_test_dss_iface_ioctl_reg_event_cb(iface, &reg_cb, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_cleanup;
    }

    reg_cb.event = DSS_IFACE_IOCTL_UP_EV;

    if ((status = dss_test_dss_iface_ioctl_reg_event_cb(iface, &reg_cb, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_cleanup;
    }

    reg_cb.event = DSS_IFACE_IOCTL_GOING_DOWN_EV;

    if ((status = dss_test_dss_iface_ioctl_reg_event_cb(iface, &reg_cb, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_cleanup;
    }

    reg_cb.event = DSS_IFACE_IOCTL_DOWN_EV;

    if ((status = dss_test_dss_iface_ioctl_reg_event_cb(iface, &reg_cb, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_cleanup;
    }

    if ((status = dss_test_dsnet_start_sync(dss_nh, DSS_SUCCESS, 0)) < 0) {
        dss_test_fail();
        goto error_cleanup;
    }

    sleep(5);

    if (Iface_state != IFACE_UP) {
        dss_test_log("Iface_state (%d) != IFACE_UP!\n", Iface_state);
        dss_test_fail();
        goto error_net_cleanup;
    }

    if ((status = dss_test_dsnet_stop_sync(dss_nh, DSS_SUCCESS, 0)) < 0) {
        dss_test_abort();
        goto error_cleanup;
    }

    sleep(5);

    if (Iface_state != IFACE_DOWN) {
        dss_test_log("Iface_state (%d) != IFACE_DOWN!\n", Iface_state);
        dss_test_fail();
        goto error_cleanup;
    }

    if ((status = dss_test_dsnet_start_sync(dss_nh, DSS_SUCCESS, 0)) < 0) {
        dss_test_fail();
        goto error_cleanup;
    }

    sleep(5);

    if (Iface_state != IFACE_UP) {
        dss_test_log("Iface_state (%d) != IFACE_UP!\n", Iface_state);
        dss_test_fail();
        goto error_net_cleanup;
    }

    dereg_cb.event = DSS_IFACE_IOCTL_DOWN_EV;

    if ((status = dss_test_dss_iface_ioctl_dereg_event_cb(iface, &dereg_cb, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_net_cleanup;
    }

    if ((status = dss_test_dsnet_stop_sync(dss_nh, DSS_SUCCESS, 0)) < 0) {
        dss_test_abort();
        goto error_cleanup;
    }

    sleep(5);

    if (Iface_state != IFACE_GOING_DOWN) {
        dss_test_log("Iface_state (%d) != IFACE_GOING_DOWN!\n", Iface_state);
        dss_test_fail();
        goto error_cleanup;
    }

    dereg_cb.event = DSS_IFACE_IOCTL_GOING_DOWN_EV;

    if ((status = dss_test_dss_iface_ioctl_dereg_event_cb(iface, &dereg_cb, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_cleanup;
    }

    dereg_cb.event = DSS_IFACE_IOCTL_UP_EV;

    if ((status = dss_test_dss_iface_ioctl_dereg_event_cb(iface, &dereg_cb, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_cleanup;
    }

    dereg_cb.event = DSS_IFACE_IOCTL_COMING_UP_EV;

    if ((status = dss_test_dss_iface_ioctl_dereg_event_cb(iface, &dereg_cb, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_cleanup;
    }

	#endif /* !FEATURE_DS_NO_DCM */

    if ((status = dss_test_dsnet_release_handle(dss_nh)) < 0) {
        dss_test_abort();
        goto error;
    }

    dss_test_pass();
    goto error;

error_net_cleanup:
    if ((status = dss_test_dsnet_stop_sync(dss_nh, DSS_SUCCESS, 0)) < 0) {
        dss_test_abort();
        goto error_cleanup;
    }

error_cleanup:
    if ((status = dss_test_dsnet_release_handle(dss_nh)) < 0) {
        dss_test_abort();
    }

error:
    return;
}

int 
main (int argc, char * argv[])
{
	dss_test_set_def(Test_id, Test_name, Test_desc, DSS_TEST_TECH_ANY);
    dss_test_init(argc, argv);
    dss_test_enter();

    dss_test_40();

    return dss_test_exit();
}
