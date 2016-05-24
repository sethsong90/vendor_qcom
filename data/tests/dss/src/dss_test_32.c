/******************************************************************************

                        D S S _ T E S T _ 3 2 . C

******************************************************************************/

/******************************************************************************

  @file    dss_test_32.c
  @brief   DSS API Test 32

  DESCRIPTION
  Tests GET_DEVICE_NAME IOCTL returns correct device name.

  ---------------------------------------------------------------------------
  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/dss_test_32.c#1 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
05/30/08   vk         Initial version

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dss_test.h"

const char Test_id[]   = "dss_test_32";
const char Test_name[] = "test GET_DEVICE_NAME IOCTL returns correct name";
const char Test_desc[] = "test GET_DEVICE_NAME IOCTL returns correct name";

void
dss_test_32 (void)
{
    dss_net_policy_info_type net_policy;
    int status;
    sint15 dss_nh;
    dss_iface_id_type iface;
    sint15 dss_errno;
    dss_iface_ioctl_device_name_type dev_name_info;

    if ((status = dss_test_dss_init_net_policy_info(&net_policy)) < 0) {
        dss_test_fail();
        goto error;
    }

    net_policy.iface.kind = DSS_IFACE_NAME;
    net_policy.iface.info.name = DSS_IFACE_ANY;

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

    status = dss_iface_ioctl
             (
                iface, 
                DSS_IFACE_IOCTL_GET_DEVICE_NAME, 
                &dev_name_info, 
                &dss_errno
             );

    if (status < 0) {
        dss_test_log("Get Device Name ioctl failed when call up\n");
        dss_test_fail();
        goto error_net_cleanup;
    }

    dss_test_log("iface_name = %s\n", dev_name_info.device_name);

    sleep(5);

    if ((status = dss_test_dsnet_stop_sync(dss_nh, DSS_SUCCESS, 0)) < 0) {
        dss_test_abort();
        goto error_cleanup;
    }

    status = dss_iface_ioctl
             (
                iface, 
                DSS_IFACE_IOCTL_GET_DEVICE_NAME, 
                &dev_name_info, 
                &dss_errno
             );

    if (status == 0) {
        dss_test_log("Get Device Name ioctl succeeded when call down\n");
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

    dss_test_32();

    return dss_test_exit();
}
