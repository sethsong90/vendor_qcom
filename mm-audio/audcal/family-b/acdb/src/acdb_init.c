/*===========================================================================
    FILE:           acdb_init.c

    OVERVIEW:       This file determines the logic to initialize the ACDB.

    DEPENDENCIES:   None

                    Copyright (c) 2010-2013 Qualcomm Technologies, Inc.
                    All Rights Reserved.
                    Qualcomm Technologies Proprietary and Confidential.
========================================================================== */

/*===========================================================================
    EDIT HISTORY FOR MODULE

    This section contains comments describing changes made to the module.
    Notice that changes are listed in reverse chronological order. Please
    use ISO format for dates.

    $Header: //source/qcom/qct/multimedia2/Audio/audcal4/acdb_sw/main/latest/acdb/src/acdb_init.c#2 $

    when        who     what, where, why
    ----------  ---     -----------------------------------------------------
    2013-06-07  mh      Corrected checkpatch errors
    2010-09-21  ernanl  Enable ADIE calibration support.
    2010-06-26  vmn     Initial revision.

========================================================================== */

/* ---------------------------------------------------------------------------
 * Include Files
 *--------------------------------------------------------------------------- */

#include "acdb_init.h"
#include "acdb_init_utility.h"
#include "acdb_parser.h"

/* ---------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Type Declarations
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Global Data Definitions
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Static Variable Definitions
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Static Function Declarations and Definitions
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Externalized Function Definitions
 *--------------------------------------------------------------------------- */
int32_t acdb_init (const char* pFilename,uint8_t **pBfr,uint32_t *pFileSize,uint32_t *pVerMaj,uint32_t *pVerMin, uint32_t *pRevVer)
{
	// acdb_init module should be able to handle both the zipped and unzipped format of acdb file
   int32_t result = ACDB_INIT_SUCCESS;

   result = AcdbGetFileData (pFilename,pBfr,pFileSize);
   if(result != ACDB_PARSE_SUCCESS || *pBfr == NULL)
	   return result;
   if(ACDB_PARSE_SUCCESS != IsAcdbFileValid(*pBfr,*pFileSize))
	   result = ACDB_INIT_FAILURE;
   else
   {
	   //Check if its a zipped acdb file, if so process it
	   if(ACDB_PARSE_COMPRESSED == IsAcdbFileZipped(*pBfr,*pFileSize))
	   {
		   // This is a zipped file, we have to unzip this using zlib
		   // and check the acdb file validity once again.
	   }
   }
    if(AcdbFileGetSWVersion(*pBfr,*pFileSize,pVerMaj,pVerMin,pRevVer)!=ACDB_PARSE_SUCCESS)
	   return ACDB_INIT_FAILURE;

   return result;
}
