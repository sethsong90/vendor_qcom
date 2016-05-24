/******************************************************************************

                        D S S _ T E S T _ 7 . C

******************************************************************************/

/******************************************************************************

  @file    dss_test_7.c
  @brief   DSS API Test 7

  DESCRIPTION
  Tests DSS Net Open and Close Failure Case when application asks for an iface
  with policy UP ONLY, when no iface is up.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/dss_test_7.c#1 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/10/08   vk         Updates for CDMA test support
09/28/07   vk         Initial version

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dss_test.h"

const char Test_id[]   = "dss_test_7";
const char Test_name[] = "test net open failure for net policy UP_ONLY when no IFACE is up";
const char Test_desc[] = "Opens network with failure expected when net policy is UP_ONLY when no IFACE is up";

void
dss_test_7 (void)
{
    dss_net_policy_info_type net_policy;
    int status;
    sint15 dss_nh;

    if ((status = dss_test_dss_init_net_policy_info(&net_policy)) < 0) {
        dss_test_fail();
        goto error;
    }

    net_policy.iface.kind = DSS_IFACE_NAME;
    net_policy.iface.info.name = DSS_IFACE_ANY;
    net_policy.policy_flag = DSS_IFACE_POLICY_UP_ONLY;

    if ((status = dss_test_dsnet_get_handle(&net_policy, &dss_nh)) < 0) {
        dss_test_fail();
        goto error;
    }

	#ifndef FEATURE_DS_NO_DCM

    if ((status = dss_test_dsnet_start_sync(dss_nh, DSS_ERROR, DS_ENETNONET)) < 0) {
        dss_test_fail();
        goto error_cleanup;
    }

	#endif /* !FEATURE_DS_NO_DCM */

    if ((status = dss_test_dsnet_release_handle(dss_nh)) < 0) {
        dss_test_fail();
        goto error;
    }

    dss_test_pass();
    goto error;

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

    dss_test_7();

    return dss_test_exit();
}
