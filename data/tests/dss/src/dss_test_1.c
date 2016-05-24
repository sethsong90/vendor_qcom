/******************************************************************************

                        D S S _ T E S T _ 1 . C

******************************************************************************/

/******************************************************************************

  @file    dss_test_1.c
  @brief   DSS API Test 1

  DESCRIPTION
  Tests DSS Net Open and Close Success Case.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/dss_test_1.c#1 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/10/08   vk         Updates for CDMA test support
09/28/07   vk         Initial version

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dss_test.h"

const char Test_id[]   = "dss_test_1";
const char Test_name[] = "test net open and close";
const char Test_desc[] = "Opens network then closes network, no data transfer";

void
dss_test_1 (void)
{
    dss_net_policy_info_type net_policy;
    int status;
    sint15 dss_nh;
    const dss_test_prog_args_t * arg_ptr;
    dss_iface_id_type if_id;
    int if_match_pass = TRUE;
    dss_iface_name_enum_type if_name;
    if ((status = dss_test_dss_init_net_policy_info(&net_policy)) < 0) {
        dss_test_fail();
        goto error;
    }

    net_policy.iface.kind = DSS_IFACE_NAME;
    net_policy.data_call_origin =  DSS_DATA_CALL_ORIGIN_EMBEDDED;

    if (dss_test_tech_get() == DSS_TEST_TECH_UMTS) {
        net_policy.iface.info.name = DSS_IFACE_UMTS;
        net_policy.umts.pdp_profile_num = DSS_TEST_DEFAULT_PDP_PROF;

        /* check if default pdp profile should be overriden */
        if ((arg_ptr = dss_test_find_prog_args('-', "--profile", &Prog_args[0], Prog_args_size)) != NULL) {
    	    net_policy.umts.pdp_profile_num = 
			    dss_test_prog_args_get_val_int(arg_ptr);
        }
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

    /* Verify that selected IFACE's tech matches requested tech */

    if ((status = dss_test_dss_get_iface_id(dss_nh, &if_id, DSS_SUCCESS)) < 0) {
        dss_test_fail();
        if_match_pass = FALSE;
        dss_test_log("Failed to get iface id\n");
    }

    if_name = dss_test_get_if_name_for_tech(dss_test_tech_get());

    if ((status = dss_test_dss_iface_ioctl_get_iface_name
             (
                if_id, 
                if_name,
                DSS_SUCCESS,
                0
             )
         ) < 0)
    {
        dss_test_fail();
        if_match_pass = FALSE;
        dss_test_log("Iface name different from desired type\n");
    }

    sleep(5);

    if ((status = dss_test_dsnet_stop_sync(dss_nh, DSS_SUCCESS, 0)) < 0) {
        dss_test_abort();
        goto error_cleanup;
    }

	#endif /* !FEATURE_DS_NO_DCM */

    if ((status = dss_test_dsnet_release_handle(dss_nh)) < 0) {
        dss_test_fail();
        goto error;
    }

    if (if_match_pass) { 
        dss_test_pass();    
    }

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

    dss_test_1();

    return dss_test_exit();
}
