/*===========================================================================
                          M M   W r a p p e r
                    f o r   M M   D e b u g   M s g

*//** @file MMDebugMsg.c
  This file defines a methods that can be used to output debug messages

Copyright (c) 2012 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*========================================================================*/


/*===========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/platform/OSAbstraction/WM/rel/2.0/src/MMDebugMsg.c#1 $

when       who         what, where, why
--------   ---         -------------------------------------------------------
07/19/12   gkapalli    Created file.


============================================================================*/



/*===========================================================================
 Include Files
============================================================================*/
#include <MMDebugMsg.h>
#include "MMMalloc.h"
#include "MMThread.h"
#include <string.h>
#include <stdlib.h>
#include "MMCriticalSection.h"
#include "AEEstd.h"
#include "MMFile.h"
#include <fcntl.h>


/* =======================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains definitions for constants, macros, types, variables
and other items needed by this module.

========================================================================== */


#define MAP_SIZE (MSG_SSID_APPS_LAST - MSG_SSID_APPS +1)
#define MMOSAL_CONFIG_BUFFER_SIZE  2048
#define MMOSAL_LOGMASK_CONFIG_FILE "/data/mmosal_logmask.cfg"

static int  nMMOSALDebugRefCnt = 0;
static MM_HANDLE hMMOSALDebugLock = NULL;
static int ssidMaskMap[MAP_SIZE];

/* =======================================================================
**                        Class & Function Definations
** ======================================================================= */
/**
 * Initializes the Diag Interface
 *
 * @return 0 value on success else failure
 */
int MM_Debug_Initialize()
{
  int index;
  int res=1;

  /*We can have a potential race condition here when two threads enter same time.
  The likelihood and impact is small(some logs will not be printed).*/
  if (nMMOSALDebugRefCnt == 0)
  {
    char MMOSALDebugConfig[MMOSAL_CONFIG_BUFFER_SIZE];
    MM_HANDLE nMMOSALDebugConfigFile = NULL;
    int pnBytesRead;

    (void)MM_CriticalSection_Create(&hMMOSALDebugLock);
    MM_CriticalSection_Enter(hMMOSALDebugLock);

    nMMOSALDebugRefCnt++;

    /*  Default bit mask. Error and Fatal are enabled.
    0 0 1 1 1 1 1 1 <- bit mask
        D F E H M L (Debug, Fatal, Error, High, Medium, Low)
    0 0 0 1 1 1 0 0 default priority*/

    for(index=0;index<MAP_SIZE;index++)
    {
      ssidMaskMap[index]=28;
    }

    MMOSALDebugConfig[0] = '\0';
    res = MM_File_Create(MMOSAL_LOGMASK_CONFIG_FILE, MM_FILE_CREATE_R, &nMMOSALDebugConfigFile);

    if (res == 0)
    {
      MM_File_Read(nMMOSALDebugConfigFile, MMOSALDebugConfig, (size_t)MMOSAL_CONFIG_BUFFER_SIZE, &pnBytesRead);
      if (pnBytesRead > 0)
      {
        MMOSALDebugConfig[pnBytesRead] = '\0';
      }
      MM_File_Release(nMMOSALDebugConfigFile);
    }

    if (std_strlen(MMOSALDebugConfig) > 0)
    {
      char sPattern[32];
      unsigned int nLogMask = 0;
      char* pLogMask = NULL;

      (void)std_strlprintf(sPattern, sizeof(sPattern), "LOGMASK = %d", MSG_SSID_APPS_ALL);
      pLogMask = std_strstr(MMOSALDebugConfig, sPattern);
      if (pLogMask)
      {
        int index;
        nLogMask = atoi(pLogMask + std_strlen(sPattern)+1);
        for(index=0;index<MAP_SIZE;index++)
        {
          ssidMaskMap[index]=nLogMask;
        }
      }

      (void)std_strlprintf(sPattern, sizeof(sPattern), "LOGMASK = ");
      pLogMask = std_strstr(MMOSALDebugConfig, sPattern);

      if (pLogMask)
      {
        int nSysID=0;
        do
        {
          pLogMask = pLogMask + std_strlen(sPattern);
          nSysID=atoi(pLogMask)-MSG_SSID_APPS;
          nLogMask = atoi(pLogMask+5);
          if (nSysID < MAP_SIZE)
          {
           ssidMaskMap[nSysID]=nLogMask;
           pLogMask = std_strstr(pLogMask, sPattern);
          }
        } while (pLogMask!='\0');
      }
    }
    MM_CriticalSection_Leave(hMMOSALDebugLock);
  }
  else
  {
    MM_CriticalSection_Enter(hMMOSALDebugLock);
    nMMOSALDebugRefCnt++;
    MM_CriticalSection_Leave(hMMOSALDebugLock);
  }

  return 0;
}

/**
 * De-Initializes the Diag Interface
 *
 * @return 0 value on success else failure
 */
int MM_Debug_Deinitialize()
{
  MM_CriticalSection_Enter(hMMOSALDebugLock);
  if (--nMMOSALDebugRefCnt == 0)
  {
    MM_CriticalSection_Leave(hMMOSALDebugLock);
    MM_CriticalSection_Release(hMMOSALDebugLock);
    hMMOSALDebugLock = NULL;
  }
  else
  {
    MM_CriticalSection_Leave(hMMOSALDebugLock);
  }
  return 0;
}

/**
 * Get the log mask for the specified system ID from the config file
 *
 * @return log mask
 */
unsigned int GetLogMask(const unsigned int nSysID)
{
  unsigned int nLogMask = 0;

  //MM_CriticalSection_Enter(hMMOSALDebugLock);
  nLogMask = (nMMOSALDebugRefCnt > 0) ? ssidMaskMap[nSysID-6000] : 28;
  //MM_CriticalSection_Leave(hMMOSALDebugLock);

  return nLogMask;
}

