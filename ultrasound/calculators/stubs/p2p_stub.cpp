/*===========================================================================
                           p2p_stub.cpp

DESCRIPTION: Provide stub to P2P lib.


INITIALIZATION AND SEQUENCING REQUIREMENTS: None


Copyright (c) 2011-2012 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/
#define LOG_TAG "P2P_stub"

/*----------------------------------------------------------------------------
Include files
----------------------------------------------------------------------------*/
#include "usf_log.h"
#include "P2PExports.h"

/*----------------------------------------------------------------------------
  Defines
----------------------------------------------------------------------------*/
#define STUB_MSG_INTERVAL 5000

/*------------------------------------------------------------------------------
  Function definitions
------------------------------------------------------------------------------*/

/*==============================================================================
  FUNCTION:  QcUsP2PLibGetSizes
==============================================================================*/
/**
  This function returns 0 in the sizeInBytes.
*/
void QcUsP2PLibGetSizes (int *sizeInBytes)
{
  LOGW("%s: Stub.",
       __FUNCTION__);

  *sizeInBytes = 0;
}

/*==============================================================================
  FUNCTION:  QcUsP2PLibInit
==============================================================================*/
/**
  This function do nothing and returns success.
*/
int QcUsP2PLibInit(signed char *piWork,
                   int iDeviceId,
                   int iPatternType,
                   int mics_num,
                   float *pfMicX,
                   float *pfMicY,
                   float *pfMicZ,
                   int spkrs_num,
                   float *pfSpeakerX,
                   float *pfSpeakerY,
                   float *pfSpeakerZ,
                   int iMicFrameSize,
                   int iSpeakerFrameSize)
{
  LOGW("%s: Stub.",
       __FUNCTION__);

  return QC_US_P2P_LIB_STATUS_SUCCESS;
}

/*==============================================================================
  FUNCTION:  QcUsP2PLibEngine
==============================================================================*/
/**
  This function prints stub msg every STUB_MSG_INTERVAL
  times the function is called.
*/
void QcUsP2PLibEngine(signed short *piMicSignal,
                      signed short *piSpeakerSignal,
                      float *pfAzimuthAngle,
                      float *pfInclinationAngle,
                      float *pfDistance,
                      char *pcOutputValid,
                      int iSequenceNum,
                      int *pbPatternUpdate)
{
  static int print_stub_msg_counter = STUB_MSG_INTERVAL;
  if (STUB_MSG_INTERVAL == print_stub_msg_counter)
  {
    LOGW("%s: Stub.",
         __FUNCTION__);
    print_stub_msg_counter = 0;
  }
  else
  {
    print_stub_msg_counter++;
  }
}

/*==============================================================================
  FUNCTION:  QcUsP2PLibTerminate
==============================================================================*/
/**
  This function do nothing.
*/
void QcUsP2PLibTerminate(signed char *piWork)
{
  LOGW("%s: Stub.",
       __FUNCTION__);
}

/*==============================================================================
  FUNCTION:  QcUsP2PLibGetVersion
==============================================================================*/
/**
  This function gets buffer of 256 bytes, pVersion,
  and returns string of stub version inside pVersion.
  It returns the size of the stub version string in the len.
*/
void QcUsP2PLibGetVersion (char *pVersion,
                           int *len)
{
  LOGW("%s: Stub.",
       __FUNCTION__);
  strlcpy(pVersion,
          STUB_VERSION,
          *len);
  *len = sizeof(STUB_VERSION);
}

