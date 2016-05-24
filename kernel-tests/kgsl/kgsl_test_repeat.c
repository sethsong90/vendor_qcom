/******************************************************************************
-----------------------------------------------------------------------------
 Copyright (c) 2011 Qualcomm Technologies, Inc.
 All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
-----------------------------------------------------------------------------
******************************************************************************/
#include "kgsl_helper.h"

int kgsl_repeat(int cnt)
{
	int i = 0, rv = 0;

	for (i = 0; i < cnt; i++) {
		kgsl_nominal();
		kgsl_adv();
		kgsl_stress();
	}

	return rv;
}
