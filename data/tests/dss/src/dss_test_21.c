/******************************************************************************

                        D S S _ T E S T _ 2 1 . C

******************************************************************************/

/******************************************************************************

  @file    dss_test_21.c
  @brief   DSS API Test 21

  DESCRIPTION
  Tests that various apis verify net policy before proceeding. 

  ---------------------------------------------------------------------------
  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/dss_test_21.c#1 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/10/08   vk         Updates for CDMA test support
04/05/08   vk         Initial version

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dss_test.h"

const char Test_id[]   = "dss_test_21";
const char Test_name[] = "net policy verification by dss";
const char Test_desc[] = "Tests that dss apis verify net policy";

void
dss_test_21 (void)
{
    dss_net_policy_info_type net_policy;
    int status;
    sint15 dss_nh;
    dss_iface_id_type if_id;

    /* Start with the 'good' cases */

    /* Verify that null net policy is accepted by dsnet_get_handle */
    if ((status = dss_test_dsnet_get_handle(NULL, &dss_nh)) < 0) {
        dss_test_fail();
        goto error;
    }

    if ((status = dss_test_dsnet_release_handle(dss_nh)) < 0) {
        dss_test_fail();
        goto error;
    }

    /* Verify that properly inited net policy is accepted by dsnet_get_handle */
    if ((status = dss_test_dss_init_net_policy_info(&net_policy)) < 0) {
        dss_test_fail();
        goto error;
    }

    net_policy.iface.kind = DSS_IFACE_NAME;
    net_policy.iface.info.name = DSS_IFACE_ANY_DEFAULT;

    if ((status = dss_test_dsnet_get_handle(&net_policy, &dss_nh)) < 0) {
        dss_test_fail();
        goto error;
    }

    /* Verify that properly inited net policy is accepted by dsnet_set_policy */
    if ((status = dss_test_dsnet_set_policy
                  (
                    dss_nh, 
                    &net_policy, 
                    DSS_SUCCESS, 
                    0
                  )
        ) < 0)
    {
        dss_test_fail();
        goto error_cleanup;
    }

    if ((status = dss_test_dsnet_release_handle(dss_nh)) < 0) {
        dss_test_fail();
        goto error;
    }

    /* Verify that properly inited net policy is accepted by 
    ** dss_get_iface_id_by_policy.
    */
    if ((status = dss_test_dss_get_iface_id_by_policy
                  (
                    &net_policy, 
                    &if_id, 
                    DSS_SUCCESS, 
                    0
                  )
        ) < 0)
    {
        dss_test_fail();
        goto error;
    }

    /* Now the bad cases */

    memset(&net_policy, 0, sizeof(dss_net_policy_info_type));

    /* Verify that bad net policy is rejected by dsnet_get_handle */
    if ((status = dss_test_ex_dsnet_get_handle
                  (
                    &net_policy, 
                    &dss_nh,
                    DSS_ERROR,
                    DS_EFAULT
                  )
        ) < 0) 
    {
        dss_test_log("Bad net policy not rejected by dsnet_get_handle\n");
        dss_test_fail();
        goto error;
    }

    if ((status = dss_test_dsnet_get_handle(NULL, &dss_nh)) < 0) {
        dss_test_fail();
        goto error;
    }

    /* Verify that null net policy is rejected by dsnet_set_policy */
    if ((status = dss_test_dsnet_set_policy
                  (
                    dss_nh, 
                    NULL,
                    DSS_ERROR, 
                    DS_EFAULT
                  )
        ) < 0)
    {
        dss_test_log("Null net policy not rejected by dsnet_set_policy\n");
        dss_test_fail();
        goto error_cleanup;
    }

    /* Verify that bad net policy is rejected by dsnet_set_policy */
    if ((status = dss_test_dsnet_set_policy
                  (
                    dss_nh, 
                    &net_policy, 
                    DSS_ERROR, 
                    DS_EFAULT
                  )
        ) < 0)
    {
        dss_test_log("Bad net policy not rejected by dsnet_set_policy\n");
        dss_test_fail();
        goto error_cleanup;
    }

    /* Verify that bad net policy is rejected by dss_get_iface_id_by_policy */
    if ((status = dss_test_dss_get_iface_id_by_policy
                  (
                    &net_policy, 
                    &if_id, 
                    DSS_ERROR, 
                    DS_EFAULT
                  )
        ) < 0)
    {
        dss_test_log("Bad net policy not rejected by dss_get_iface_id_by_policy\n");
        dss_test_fail();
        goto error_cleanup;
    }

    dss_test_pass();    

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

    dss_test_21();

    return dss_test_exit();
}
