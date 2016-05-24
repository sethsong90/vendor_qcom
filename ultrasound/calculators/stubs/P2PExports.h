#ifndef __P2P_EXPORTS_H__
#define __P2P_EXPORTS_H__

/*============================================================================
                           P2PExports.h

DESCRIPTION:  Function definitions for the P2P lib (libqcp2p).

Copyright (c) 2011 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------
  Type Definitions
----------------------------------------------------------------------------*/
#define STUB_VERSION "0.0.0.0"

/**
  P2P library return status
*/
  typedef enum
  {
    QC_US_P2P_LIB_STATUS_SUCCESS     =  0,   /* Success */
    QC_US_P2P_LIB_STATUS_FAILURE     =  1,   /* General failure */
    QC_US_P2P_LIB_STATUS_BAD_PARAMS  =  2,   /* Bad parameters */
  } QcUsP2PLibStatusType;

/*----------------------------------------------------------------------------
  Function Declarations
----------------------------------------------------------------------------*/

/*============================================================================
  FUNCTION:  QcUsP2PLibGetSizes
============================================================================*/
/**
  Returns the size of the work space needed to the algorithm.
*/
  void QcUsP2PLibGetSizes(int *piSize);

/*============================================================================
  FUNCTION:  QcUsP2PLibInit
============================================================================*/
/**
  Init the P2P algorithm.
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
                     int iSpeakerFrameSize);

/*============================================================================
  FUNCTION:  QcUsP2PLibTerminate
============================================================================*/
/**
  Terminate the algorithm.
*/
  void QcUsP2PLibTerminate(signed char *piWork);

/*============================================================================
  FUNCTION:  QcUsP2PLibEngine
============================================================================*/
/**
  Returms P2P algorithm calculation.
*/
  void QcUsP2PLibEngine(signed short *piMicSignal,
                        signed short *piSpeakerSignal,
                        float *pfAzimuthAngle,
                        float *pfInclinationAngle,
                        float *pfDistance,
                        char *pcOutputValid,
                        int iSequenceNum,
                        int *pbPatternUpdate);

/*============================================================================
  FUNCTION:  QcUsP2PLibGetVersion
============================================================================*/
/**
  Returns the P2P lib version.
*/
  void QcUsP2PLibGetVersion(char *pcVersion,
                            int *piLen);

#ifdef __cplusplus
}
#endif

#endif // P2P_EXPORTS_H
