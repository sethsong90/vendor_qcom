/******************************************************************************

                        D S S _ T E S T _ 2 2 . C

******************************************************************************/

/******************************************************************************

  @file    dss_test_22.c
  @brief   DSS API Test 22

  DESCRIPTION
  Tests APN override in a variety of cases.

  ---------------------------------------------------------------------------
  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/dss_test_22.c#1 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
05/28/08   vk         Initial version

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "dss_test.h"

const char Test_id[]   = "dss_test_22";
const char Test_name[] = "tests APN override";
const char Test_desc[] = "tests APN override";

const char Override_APN_1[] = "OverrideAPN1";
const char Override_APN_2[] = "OverrideAPN2";

void
dss_test_22 (void)
{
    dss_net_policy_info_type net_policy_1, net_policy_2;
    int status;
    sint15 dss_nh_1, dss_nh_2;
    dss_iface_id_type if_1, if_2;

    if ((status = dss_test_dss_init_net_policy_info(&net_policy_1)) < 0) {
        dss_test_fail();
        goto error;
    }

    net_policy_1.iface.kind = DSS_IFACE_NAME;
    net_policy_1.iface.info.name = DSS_IFACE_UMTS;
    net_policy_1.umts.pdp_profile_num = DSS_TEST_PDP_PROF_1;
    net_policy_1.umts.apn.length = strlen(Override_APN_1);
    memcpy
    (
        net_policy_1.umts.apn.name,
        Override_APN_1,
        net_policy_1.umts.apn.length
    );

    if ((status = dss_test_dss_init_net_policy_info(&net_policy_2)) < 0) {
        dss_test_fail();
        goto error;
    }

    if ((status = dss_test_dsnet_get_handle(&net_policy_1, &dss_nh_1)) < 0) {
        dss_test_fail();
        goto error;
    }

	#ifndef FEATURE_DS_NO_DCM

    if ((status = dss_test_dsnet_start_sync(dss_nh_1, DSS_SUCCESS, 0)) < 0) {
        dss_test_fail();
        goto error_cleanup;
    }

    if ((status = dss_test_dss_get_iface_id(dss_nh_1, &if_1, DSS_SUCCESS)) < 0) {
        dss_test_fail();
        goto error_net_cleanup;
    }

    net_policy_2.iface.kind = DSS_IFACE_NAME;
    net_policy_2.iface.info.name = DSS_IFACE_UMTS;
    net_policy_2.umts.pdp_profile_num = DSS_TEST_PDP_PROF_1;

    if ((status = dss_test_dss_get_iface_id_by_policy(&net_policy_2, &if_2, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_net_cleanup;
    }

    if (if_1 == if_2) {
        dss_test_fail();
        goto error_net_cleanup;
    }

    dss_test_log("APN o'ride and non-o'ride for same prof ID test good..\n");

    net_policy_2.umts.apn.length = strlen(Override_APN_2);
    memcpy
    (
        net_policy_2.umts.apn.name,
        Override_APN_2,
        net_policy_2.umts.apn.length
    );

    if ((status = dss_test_dss_get_iface_id_by_policy(&net_policy_2, &if_2, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_net_cleanup;
    }

    if (if_1 == if_2) {
        dss_test_fail();
        goto error_net_cleanup;
    }

    dss_test_log("APN o'ride with same prof IDs test good..\n");

    net_policy_2.umts.pdp_profile_num = DSS_TEST_PDP_PROF_2;
    net_policy_2.umts.apn.length = strlen(Override_APN_1);
    memcpy
    (
        net_policy_2.umts.apn.name,
        Override_APN_1,
        net_policy_2.umts.apn.length
    );

    if ((status = dss_test_dss_get_iface_id_by_policy(&net_policy_2, &if_2, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_net_cleanup;
    }

    if (if_1 != if_2) {
        dss_test_fail();
        goto error_net_cleanup;
    }

    dss_test_log("APN o'ride with diff prof IDs test good..\n");

    net_policy_2.umts.pdp_profile_num = DSS_TEST_PDP_PROF_1_MATCH;
    net_policy_2.umts.apn.length = 0;

    if ((status = dss_test_dsnet_get_handle(&net_policy_2, &dss_nh_2)) < 0) {
        dss_test_fail();
        goto error_net_cleanup;
    }

    if ((status = dss_test_dsnet_start_sync(dss_nh_2, DSS_SUCCESS, 0)) < 0) {
        dss_test_fail();
        goto error_cleanup_2;
    }

    if ((status = dss_test_match_nh_ifaces(dss_nh_1, dss_nh_2, DSS_ERROR)) < 0) {
        dss_test_fail();
        goto error_net_cleanup_2;
    }

    dss_test_log("Verified that with o'ride call really uses a diff apn..\n");

    if ((status = dss_test_dsnet_stop_sync(dss_nh_2, DSS_SUCCESS, 0)) < 0) {
        dss_test_abort();
        goto error_cleanup_2;
    }

    sleep(5);

    if ((status = dss_test_dsnet_stop_sync(dss_nh_1, DSS_SUCCESS, 0)) < 0) {
        dss_test_abort();
        goto error_cleanup;
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

error_net_cleanup_2:
    if ((status = dss_test_dsnet_stop_sync(dss_nh_2, DSS_SUCCESS, 0)) < 0) {
        dss_test_abort();
        goto error_net_cleanup;
    }

error_cleanup_2:
    if ((status = dss_test_dsnet_release_handle(dss_nh_2)) < 0) {
        dss_test_abort();
        goto error_net_cleanup;
    }

error_net_cleanup:
    if ((status = dss_test_dsnet_stop_sync(dss_nh_1, DSS_SUCCESS, 0)) < 0) {
        dss_test_abort();
        goto error_cleanup;
    }

error_cleanup:
    if ((status = dss_test_dsnet_release_handle(dss_nh_1)) < 0) {
        dss_test_abort();
    }

error:
    return;
}

int 
main (int argc, char * argv[])
{
	dss_test_set_def(Test_id, Test_name, Test_desc, DSS_TEST_TECH_UMTS);
    dss_test_init(argc, argv);
    dss_test_enter();

    dss_test_22();

    return dss_test_exit();
}
