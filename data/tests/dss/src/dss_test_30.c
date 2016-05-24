/******************************************************************************

                        D S S _ T E S T _ 3 0 . C

******************************************************************************/

/******************************************************************************

  @file    dss_test_30.c
  @brief   DSS API Test 30

  DESCRIPTION
  Tests GET_IFACE_NAME IOCTL returns correct name for UMTS IFACE. 

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/dss_test_30.c#1 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/10/08   vk         Updates for CDMA test support
09/28/07   vk         Initial version

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dss_test.h"

const char Test_id[]   = "dss_test_30";
const char Test_name[] = "test GET_IFACE_NAME returns correct name for UMTS IFACE";
const char Test_desc[] = "test GET_IFACE_NAME returns correct name for UMTS IFACE";

void
dss_test_30 (void)
{
    dss_net_policy_info_type net_policy;
    int status;
    sint15 dss_nh;
    dss_iface_id_type iface;

    if ((status = dss_test_dss_init_net_policy_info(&net_policy)) < 0) {
        dss_test_fail();
        goto error;
    }

    net_policy.iface.kind = DSS_IFACE_NAME;

    if (dss_test_tech_get() == DSS_TEST_TECH_UMTS) {
        net_policy.iface.info.name = DSS_IFACE_UMTS;
        net_policy.umts.pdp_profile_num = DSS_TEST_DEFAULT_PDP_PROF;
    } else {
        net_policy.iface.info.name = DSS_IFACE_CDMA_SN;
    }

    if ((status = dss_test_dsnet_get_handle(&net_policy, &dss_nh)) < 0) {
        dss_test_fail();
        goto error;
    }

	#ifndef FEATURE_DS_NO_DCM

    if ((status = dss_test_dsnet_start_sync(dss_nh, DSS_SUCCESS, 0)) < 0) {
        dss_test_fail();
        goto error_cleanup;
    }

    if ((status = dss_test_dss_get_iface_id(dss_nh, &iface, DSS_SUCCESS)) < 0) {
        dss_test_fail();
        goto error_net_cleanup;
    }

    if (dss_test_tech_get() == DSS_TEST_TECH_UMTS) {
        if ((status = dss_test_dss_iface_ioctl_get_iface_name(iface, DSS_IFACE_UMTS, DSS_SUCCESS, 0)) < 0)
        {
            dss_test_fail();
            goto error_net_cleanup;
        }
    } else {
        if ((status = dss_test_dss_iface_ioctl_get_iface_name(iface, DSS_IFACE_CDMA_SN, DSS_SUCCESS, 0)) < 0)
        {
            dss_test_fail();
            goto error_net_cleanup;
        }
    }

    sleep(5);

    if ((status = dss_test_dsnet_stop_sync(dss_nh, DSS_SUCCESS, 0)) < 0) {
        dss_test_abort();
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

    dss_test_30();

    return dss_test_exit();
}
