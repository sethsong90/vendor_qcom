#ifndef DS_SOCK_PRIVILEGES_DEF_H
#define DS_SOCK_PRIVILEGES_DEF_H

/*============================================================================
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#define ds_Sock_AEEPRIVID_PStreamSocket 0x1099fd1
#define ds_Sock_AEEPRIVID_PDatagramSocket 0x1099fd2
#define ds_Sock_AEEPRIVID_PServerSocket 0x1099fd3
#define ds_Sock_AEEPRIVID_PSocketMem 0x1099fd5
#define ds_Sock_AEEPRIVID_PNetworkTx 0x1099fd6
#define ds_Sock_AEEPRIVID_PPSProcessing 0x1099fd7
#define ds_Sock_AEEPRIVID_PReuseAddr 0x1099fd8
#define ds_Sock_AEEPRIVID_NetUrgent 0x10153f7
#else /* C++ */
#include "AEEStdDef.h"
namespace ds
{
   namespace Sock
   {
      const ::AEEPRIVID AEEPRIVID_PStreamSocket = 0x1099fd1;
      const ::AEEPRIVID AEEPRIVID_PDatagramSocket = 0x1099fd2;
      const ::AEEPRIVID AEEPRIVID_PServerSocket = 0x1099fd3;
      const ::AEEPRIVID AEEPRIVID_PSocketMem = 0x1099fd5;
      const ::AEEPRIVID AEEPRIVID_PNetworkTx = 0x1099fd6;
      const ::AEEPRIVID AEEPRIVID_PPSProcessing = 0x1099fd7;
      const ::AEEPRIVID AEEPRIVID_PReuseAddr = 0x1099fd8;
      const ::AEEPRIVID AEEPRIVID_NetUrgent = 0x10153f7;
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_SOCK_PRIVILEGES_DEF_H
