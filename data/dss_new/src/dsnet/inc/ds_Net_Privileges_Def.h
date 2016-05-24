#ifndef DS_NET_PRIVILEGES_DEF_H
#define DS_NET_PRIVILEGES_DEF_H

/*============================================================================
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#define ds_Net_AEEPRIVID_PGoNull 0x109d4a7
#define ds_Net_AEEPRIVID_PGoDormant 0x109d4da
#define ds_Net_AEEPRIVID_PMCastClient 0x1099fd4
#define ds_Net_AEEPRIVID_PNetMonitored 0x10a2aa8
#define ds_Net_AEEPRIVID_PMTPD 0x10a2aa9
#define ds_Net_AEEPRIVID_PNetOTA 0x10a2aaa
#define ds_Net_AEEPRIVID_PNetPolicy 0x10a2aab
#define ds_Net_AEEPRIVID_PNetSystem 0x10a2aac
#define ds_Net_AEEPRIVID_PNet 0x10a2aad
#define ds_Net_AEEPRIVID_PBCMCSUpdateDB 0x10a2aae
#define ds_Net_AEEPRIVID_PPrivateAddr 0x10a2aaf
#define ds_Net_AEEPRIVID_PQoS 0x10a2ab0
#define ds_Net_AEEPRIVID_PTechUMTS 0x10a2ab1
#define ds_Net_AEEPRIVID_PTech1x 0x10a2ab2
#define ds_Net_AEEPRIVID_NetHAT 0x10708aa
#define ds_Net_AEEPRIVID_PrimaryQoSSession 0x104549a
#define ds_Net_AEEPRIVID_RLPConfig 0x10aaac0
#else /* C++ */
#include "AEEStdDef.h"
namespace ds
{
   namespace Net
   {
      const ::AEEPRIVID AEEPRIVID_PGoNull = 0x109d4a7;
      const ::AEEPRIVID AEEPRIVID_PGoDormant = 0x109d4da;
      const ::AEEPRIVID AEEPRIVID_PMCastClient = 0x1099fd4;
      const ::AEEPRIVID AEEPRIVID_PNetMonitored = 0x10a2aa8;
      const ::AEEPRIVID AEEPRIVID_PMTPD = 0x10a2aa9;
      const ::AEEPRIVID AEEPRIVID_PNetOTA = 0x10a2aaa;
      const ::AEEPRIVID AEEPRIVID_PNetPolicy = 0x10a2aab;
      const ::AEEPRIVID AEEPRIVID_PNetSystem = 0x10a2aac;
      const ::AEEPRIVID AEEPRIVID_PNet = 0x10a2aad;
      const ::AEEPRIVID AEEPRIVID_PBCMCSUpdateDB = 0x10a2aae;
      const ::AEEPRIVID AEEPRIVID_PPrivateAddr = 0x10a2aaf;
      const ::AEEPRIVID AEEPRIVID_PQoS = 0x10a2ab0;
      const ::AEEPRIVID AEEPRIVID_PTechUMTS = 0x10a2ab1;
      const ::AEEPRIVID AEEPRIVID_PTech1x = 0x10a2ab2;
      const ::AEEPRIVID AEEPRIVID_NetHAT = 0x10708aa;
      const ::AEEPRIVID AEEPRIVID_PrimaryQoSSession = 0x104549a;
      const ::AEEPRIVID AEEPRIVID_RLPConfig = 0x10aaac0;
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_PRIVILEGES_DEF_H
