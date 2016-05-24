/******************************************************************************

                        D S S _ T E S T _ 2 0 . C

******************************************************************************/

/******************************************************************************

  @file    dss_test_20.c
  @brief   DSS API Test 20

  DESCRIPTION
  Tests dss_get_iface_id_by_policy returns right interface, in a variety of 
  cases.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/dss_test_20.c#1 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/10/08   vk         Updates for CDMA test support
09/28/07   vk         Initial version

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dss_test.h"

const char Test_id[]   = "dss_test_20";
const char Test_name[] = "tests dss_get_iface_id_by_policy returns the right IFACE";
const char Test_desc[] = "tests dss_get_iface_id_by_policy returns the right IFACE";

void
dss_test_20 (void)
{
    dss_net_policy_info_type net_policy_1, net_policy_2;
    int status;
    sint15 dss_nh;
    dss_iface_id_type if_1, if_2;

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

    if ((status = dss_test_dsnet_get_handle(&net_policy_1, &dss_nh)) < 0) {
        dss_test_fail();
        goto error;
    }

	#ifndef FEATURE_DS_NO_DCM

    if ((status = dss_test_dsnet_start_sync(dss_nh, DSS_SUCCESS, 0)) < 0) {
        dss_test_fail();
        goto error_cleanup;
    }

    if ((status = dss_test_dss_get_iface_id(dss_nh, &if_1, DSS_SUCCESS)) < 0) {
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

    if (if_1 != if_2) {
        dss_test_fail();
        goto error_net_cleanup;
    }

    dss_test_log("PDP_PROF_1 test good..\n");

    net_policy_2.umts.pdp_profile_num = DSS_TEST_PDP_PROF_1_MATCH;

    if ((status = dss_test_dss_get_iface_id_by_policy(&net_policy_2, &if_2, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_net_cleanup;
    }

    if (if_1 != if_2) {
        dss_test_fail();
        goto error_net_cleanup;
    }

    dss_test_log("PDP_PROF_1_MATCH test good..\n");

    net_policy_2.policy_flag = DSS_IFACE_POLICY_UP_ONLY;

    if ((status = dss_test_dss_get_iface_id_by_policy(&net_policy_2, &if_2, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_net_cleanup;
    }

    if (if_1 != if_2) {
        dss_test_fail();
        goto error_net_cleanup;
    }

    dss_test_log("POLICY_UP_ONLY test good..\n");

    net_policy_2.policy_flag = DSS_IFACE_POLICY_UP_PREFERRED;

    if ((status = dss_test_dss_get_iface_id_by_policy(&net_policy_2, &if_2, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_net_cleanup;
    }

    if (if_1 != if_2) {
        dss_test_fail();
        goto error_net_cleanup;
    }

    dss_test_log("POLICY_UP_PREF test good..\n");

    net_policy_2.umts.pdp_profile_num = DSS_TEST_PDP_PROF_2;
    net_policy_2.policy_flag = DSS_IFACE_POLICY_ANY;

    if ((status = dss_test_dss_get_iface_id_by_policy(&net_policy_2, &if_2, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_net_cleanup;
    }

    if (if_1 == if_2) {
        dss_test_fail();
        goto error_net_cleanup;
    }

    dss_test_log("PDP_PROF_2 test good..\n");

    net_policy_2.iface.info.name = DSS_IFACE_WWAN;
    net_policy_2.policy_flag = DSS_IFACE_POLICY_UP_ONLY;

    if ((status = dss_test_dss_get_iface_id_by_policy(&net_policy_2, &if_2, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_net_cleanup;
    }

    if (if_1 != if_2) {
        dss_test_fail();
        goto error_net_cleanup;
    }

    dss_test_log("IFACE_WWAN test good..\n");

    net_policy_2.iface.info.name = DSS_IFACE_ANY;

    if ((status = dss_test_dss_get_iface_id_by_policy(&net_policy_2, &if_2, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_net_cleanup;
    }

    if (if_1 != if_2) {
        dss_test_fail();
        goto error_net_cleanup;
    }

    dss_test_log("IFACE_ANY test good..\n");

    net_policy_2.iface.info.name = DSS_IFACE_ANY_DEFAULT;

    if ((status = dss_test_dss_get_iface_id_by_policy(&net_policy_2, &if_2, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_net_cleanup;
    }

    if (if_1 != if_2) {
        dss_test_fail();
        goto error_net_cleanup;
    }

    dss_test_log("IFACE_ANY_DEFAULT test good..\n");

    net_policy_2.iface.kind = DSS_IFACE_ID;
    net_policy_2.iface.info.id = if_1;

    if ((status = dss_test_dss_get_iface_id_by_policy(&net_policy_2, &if_2, DSS_SUCCESS, 0)) < 0)
    {
        dss_test_fail();
        goto error_net_cleanup;
    }

    if (if_1 != if_2) {
        dss_test_fail();
        goto error_net_cleanup;
    }

    dss_test_log("IFACE_ID test good..\n");

    sleep(5);

    if ((status = dss_test_dsnet_stop_sync(dss_nh, DSS_SUCCESS, 0)) < 0) {
        dss_test_abort();
        goto error_cleanup;
    }

    net_policy_2.iface.info.name = DSS_IFACE_UMTS;
    net_policy_2.policy_flag = DSS_IFACE_POLICY_UP_ONLY;

    if ((status = dss_test_dss_get_iface_id_by_policy(&net_policy_2, &if_2, DSS_ERROR, DS_ENOROUTE)) < 0)
    {
        dss_test_fail();
        goto error_cleanup;
    }

    dss_test_log("UP_ONLY failure test good..\n");

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
	dss_test_set_def(Test_id, Test_name, Test_desc, DSS_TEST_TECH_UMTS);
    dss_test_init(argc, argv);
    dss_test_enter();

    dss_test_20();

    return dss_test_exit();
}
