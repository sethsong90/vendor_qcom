/*===========================================================================
    FILE:           acdb_init_utility.c

    OVERVIEW:       This file contains the acdb init utility functions
                    implemented specifically in the win32 environment.

    DEPENDENCIES:   None

                    Copyright (c) 2012 Qualcomm Technologies, Inc.
                    All Rights Reserved.
                    Qualcomm Technologies Proprietary and Confidential.
========================================================================== */

/*===========================================================================
    EDIT HISTORY FOR MODULE

    This section contains comments describing changes made to the module.
    Notice that changes are listed in reverse chronological order. Please
    use ISO format for dates.

    $Header: //source/qcom/qct/multimedia2/Audio/audcal3/acdb_hlos/rel/2.5/src/acdb_init_utility.c#1 $

    when        who     what, where, why
    ----------  ---     -----------------------------------------------------
    2010-07-08  vmn     Initial revision.

========================================================================== */

/* ---------------------------------------------------------------------------
 * Include Files
 *--------------------------------------------------------------------------- */

#include "acdb_init_utility.h"
#include "acdb_init.h"
//modified for QNX build

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

int32_t AcdbGetFileData (const char* pFilename, uint8_t **pBfr, uint32_t *pFileSize)
{
   int32_t result = ACDB_UTILITY_INIT_SUCCESS;
   FILE *fp = NULL;
   uint32_t nBytesRead = 0;

   if (pFilename == NULL || pFileSize == NULL || pBfr == NULL)
   {
      return ACDB_UTILITY_INIT_FAILURE;
   }
   else
   {
	  fp = fopen(pFilename, "rb");
      if (fp == NULL)
      {
         return ACDB_UTILITY_INIT_FAILURE;
      }

      fseek (fp, 0, SEEK_END);
      *pFileSize = ftell (fp);
      fseek (fp, 0, SEEK_SET);

	  *pBfr = (uint8_t *)malloc(*pFileSize);
	  if(*pBfr != NULL)
	  {
		  nBytesRead = (uint32_t) fread (*pBfr,sizeof (uint8_t),(size_t)*pFileSize,fp);
	  }
	  else
	  {
		  result = ACDB_UTILITY_INIT_FAILURE;
	  }
	  if(nBytesRead != *pFileSize)
	  {
		  result = ACDB_UTILITY_INIT_FAILURE;
		  if(*pBfr != NULL)
		  {
			 free(*pBfr);
		  }
	  }
	  fclose (fp);
   }
   return result;
}

void  AcdbFreeFileData (void *pBfr)
{
    if(pBfr != NULL)
	{
		free(pBfr);
		pBfr = NULL;
	}
}
