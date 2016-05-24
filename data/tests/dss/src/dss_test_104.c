/******************************************************************************

                        D S S _ T E S T _ 1 0 4 . C

******************************************************************************/

/******************************************************************************

  @file    dss_test_104.c
  @brief   DSS API Test 104

  DESCRIPTION
  Tests that an UP interface is preferred over a DOWN interface when doing 
  interface selection. 

  ---------------------------------------------------------------------------
  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/dss_test_104.c#1 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/10/08   vk         Updates for CDMA test support
01/12/08   vk         Initial version

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dss_test.h"

const char Test_id[]   = "dss_test_104";
const char Test_name[] = "test interface selection preferres UP iface";
const char Test_desc[] = "Tests that interface selection preferres an UP iface with matching policy over a DOWN iface";

void
dss_test_104 (void)
{
    dss_net_policy_info_type net_policy_1, net_policy_2;
    int status;
    sint15 dss_nh_1, dss_nh_2;
    dss_iface_id_type if_2, if_2b;

    if ((status = dss_test_dss_init_net_policy_info(&net_policy_1)) < 0) {
        dss_test_fail();
        goto error;
    }

    net_policy_1.iface.kind = DSS_IFACE_NAME;
    net_policy_1.iface.info.name = DSS_IFACE_UMTS;
    net_policy_1.umts.pdp_profile_num = DSS_TEST_PDP_PROF_1;

    if ((status = dss_test_dss_init_net_policy_info(&net_policy_2)) < 0) {
        dss_test_fail();
        goto error;
    }

    net_policy_2.iface.kind = DSS_IFACE_NAME;
    net_policy_2.iface.info.name = DSS_IFACE_UMTS;
    net_policy_2.umts.pdp_profile_num = DSS_TEST_PDP_PROF_2;

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

    if ((status = dss_test_match_nh_ifaces(dss_nh_1, dss_nh_2, DSS_ERROR)) < 0) {
        dss_test_fail();
        goto error_cleanup_net_2;
    }

    if ((status = dss_test_dss_get_iface_id(dss_nh_2, &if_2, DSS_SUCCESS)) < 0) {
        dss_test_fail();
        goto error_cleanup_net_2;
    }

    if ((status = dss_test_dsnet_stop_sync(dss_nh_1, DSS_SUCCESS, 0)) < 0) {
        dss_test_fail();
        goto error_cleanup_net_2;
    }

    if ((status = dss_test_dss_get_iface_id_by_policy(&net_policy_2, &if_2b, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_cleanup_net_2;
    }

    if (if_2b != if_2) {
        dss_test_log("if_2b (%ld) doesn't match if_2 (%ld)", if_2b, if_2);
        dss_test_fail();
        goto error_cleanup_net_2;
    }

    sleep(5);

    if ((status = dss_test_dsnet_stop_sync(dss_nh_2, DSS_SUCCESS, 0)) < 0) {
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
	dss_test_set_def(Test_id, Test_name, Test_desc, DSS_TEST_TECH_UMTS);
    dss_test_init(argc, argv);
    dss_test_enter();

    dss_test_104();

    return dss_test_exit();
}
