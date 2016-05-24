#ifndef __ACDB_INIT_UTILITY_H__
#define __ACDB_INIT_UTILITY_H__
/*===========================================================================
    @file   acdb_init_utility.h

    The interface to the ACDBINIT utility functions.

    This file will provide API access to OS-specific init utility functions.

                    Copyright (c) 2012-2013 Qualcomm Technologies, Inc.
                    All Rights Reserved.
                    Qualcomm Technologies Proprietary and Confidential.
========================================================================== */
/* $Header: //source/qcom/qct/multimedia2/Audio/audcal4/acdb_sw/main/latest/acdb/inc/acdb_init_utility.h#1 $ */

/* ---------------------------------------------------------------------------
 * Include Files
 *--------------------------------------------------------------------------- */
#include "acdb_os_includes.h"

/* ---------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Type Declarations
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
* Class Definitions
*--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Function Declarations and Documentation
 *--------------------------------------------------------------------------- */

#define ACDB_UTILITY_INIT_SUCCESS 0
#define ACDB_UTILITY_INIT_FAILURE -1

int32_t AcdbGetFileData (const char* pFilename,uint8_t **pBfr,uint32_t *pFileSize);
void  AcdbFreeFileData (void *pBfr);

#endif /* __ACDB_INIT_UTILITY_H__ */
