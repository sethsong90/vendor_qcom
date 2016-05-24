/******************************************************************************
-----------------------------------------------------------------------------
 Copyright (c) 2011 Qualcomm Technologies, Inc.
 All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
-----------------------------------------------------------------------------
******************************************************************************/
#include "kgsl_helper.h"
int kgsl_nominal()
{
	int rv = 0;

	kgsl_test_log(KGSL_LOG_LEVEL_INFO, "\nStarting Nominal\n\n");
	if (strncmp(dev_name, KGSL_3D_DEV, 13) == 0) {
		TEST_STEP(0, rv, kgsl_test_openclose(), "|---OPEN/CLOSE---|\n");
		TEST_STEP(0, rv, kgsl_test_simple_getprops(),
			  "|---GET PROPS---|\n");
		TEST_STEP(0, rv, kgsl_test_create_destroy(),
			  "|---CREATE/DESTROY---|\n");
		TEST_STEP(0, rv, kgsl_test_simple(1), "|---SIMPLE TEST---|\n");
	} else if ((strncmp(dev_name, KGSL_2D0_DEV, 13) == 0) ||
		   (strncmp(dev_name, KGSL_2D1_DEV, 13) == 0)) {
		TEST_STEP(0, rv, kgsl_test_openclose(), "|---OPEN/CLOSE---|\n");
		TEST_STEP(0, rv, kgsl_test_create_destroy(),
			  "|---CREATE/DESTROY---|\n");
	}
	return rv;
}
