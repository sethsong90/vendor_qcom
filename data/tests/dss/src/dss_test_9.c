/******************************************************************************

                        D S S _ T E S T _ 9 . C

******************************************************************************/

/******************************************************************************

  @file    dss_test_9.c
  @brief   DSS API Test 9

  DESCRIPTION
  Tests DSS Net Open Success Case using policy UP_ONLY when a PDP is already 
  active.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/dss_test_9.c#1 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/10/08   vk         Updates for CDMA test support
09/28/07   vk         Initial version

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dss_test.h"

const char Test_id[]   = "dss_test_9";
const char Test_name[] = "test net open success using policy UP_ONLY with an IFACE already active";
const char Test_desc[] = "test net open success using policy UP_ONLY with an IFACE already active";

void
dss_test_9 (void)
{
    dss_net_policy_info_type net_policy_1, net_policy_2;
    int status;
    sint15 dss_nh_1, dss_nh_2;

    if ((status = dss_test_dss_init_net_policy_info(&net_policy_1)) < 0) {
        dss_test_fail();
        goto error;
    }

    net_policy_1.iface.kind = DSS_IFACE_NAME;

    if (dss_test_tech_get() == DSS_TEST_TECH_UMTS) {
        net_policy_1.iface.info.name = DSS_IFACE_UMTS;
        net_policy_1.umts.pdp_profile_num = DSS_TEST_DEFAULT_PDP_PROF;
    } else {
        net_policy_1.iface.info.name = DSS_IFACE_CDMA_SN;
    }

    if ((status = dss_test_dss_init_net_policy_info(&net_policy_2)) < 0) {
        dss_test_fail();
        goto error;
    }

    net_policy_2.iface.kind = DSS_IFACE_NAME;
    net_policy_2.policy_flag = DSS_IFACE_POLICY_UP_ONLY;

    if (dss_test_tech_get() == DSS_TEST_TECH_UMTS) {
        net_policy_2.iface.info.name = DSS_IFACE_UMTS;
        net_policy_2.umts.pdp_profile_num = DSS_TEST_DEFAULT_PDP_PROF;
    } else {
        net_policy_2.iface.info.name = DSS_IFACE_CDMA_SN;
    }

    if ((status = dss_test_dsnet_get_handle(&net_policy_1, &dss_nh_1)) < 0) {
        dss_test_fail();
        goto error;
    }

    if ((status = dss_test_dsnet_get_handle(&net_policy_2, &dss_nh_2)) < 0) {
        dss_test_fail();
        goto error_cleanup_handle_1;
    }

	#ifndef FEATURE_DS_NO_DCM

    if ((status = dss_test_dsnet_start_sync(dss_nh_1, DSS_SUCCESS, 0)) < 0) {
        dss_test_fail();
        goto error_cleanup_handle_2;
    }

    if ((status = dss_test_dsnet_start_sync(dss_nh_2, DSS_SUCCESS, 0)) < 0) {
        dss_test_fail();
        goto error_cleanup_net_1;
    }

    if ((status = dss_test_match_nh_ifaces(dss_nh_1, dss_nh_2, DSS_SUCCESS)) < 0) {
        dss_test_fail();
        goto error_cleanup_net_2;
    }

    sleep(5);

    if ((status = dss_test_dsnet_stop_sync(dss_nh_2, DSS_SUCCESS, 0)) < 0) {
        dss_test_fail();
        goto error_cleanup_net_1;
    }

    sleep(5);

    if ((status = dss_test_dsnet_stop_sync(dss_nh_1, DSS_SUCCESS, 0)) < 0) {
        dss_test_fail();
        goto error_cleanup_handle_2;
    }

	#endif /* !FEATURE_DS_NO_DCM */

    if ((status = dss_test_dsnet_release_handle(dss_nh_2)) < 0) {
        dss_test_abort();
        goto error;
    }

    if ((status = dss_test_dsnet_release_handle(dss_nh_1)) < 0) {
        dss_test_abort();
        goto error;
    }

    dss_test_pass();
    goto error;

error_cleanup_net_2:
    if ((status = dss_test_dsnet_stop_sync(dss_nh_2, DSS_SUCCESS, 0)) < 0) {
        dss_test_fail();
        goto error_cleanup_net_1;
    }

error_cleanup_net_1:
    if ((status = dss_test_dsnet_stop_sync(dss_nh_1, DSS_SUCCESS, 0)) < 0) {
        dss_test_fail();
        goto error_cleanup_handle_2;
    }

error_cleanup_handle_2:
    if ((status = dss_test_dsnet_release_handle(dss_nh_2)) < 0) {
        dss_test_abort();
        goto error;
    }

error_cleanup_handle_1:
    if ((status = dss_test_dsnet_release_handle(dss_nh_1)) < 0) {
        dss_test_abort();
        goto error;
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

    dss_test_9();

    return dss_test_exit();
}
