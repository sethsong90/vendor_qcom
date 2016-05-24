#ifndef DS_NET_QOS_DEFPRIV_H
#define DS_NET_QOS_DEFPRIV_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Net_IQoSFlowPriv.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_IIPFilterPriv.h"

/**
  * ds Net QoS Def Private module.
  * This module groups all the Private QoS types and constants.
  */
struct ds_Net__SeqIQoSFlowPrivType__seq_IQoSFlowPriv_Net_ds {
   struct ds_Net_IQoSFlowPriv** data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net__SeqIQoSFlowPrivType__seq_IQoSFlowPriv_Net_ds ds_Net__SeqIQoSFlowPrivType__seq_IQoSFlowPriv_Net_ds;
typedef ds_Net__SeqIQoSFlowPrivType__seq_IQoSFlowPriv_Net_ds ds_Net_SeqIQoSFlowPrivType;
struct ds_Net_QoSSpecType {
   struct ds_Net_IQoSFlowPriv** rxFlows;
   int rxFlowsLen;
   int rxFlowsLenReq;
   boolean rxMinFlow;
   AEEINTERFACE_PADMEMBERS(txFlows, 3)
   struct ds_Net_IQoSFlowPriv** txFlows;
   int txFlowsLen;
   int txFlowsLenReq;
   boolean txMinFlow;
   AEEINTERFACE_PADMEMBERS(rxFilter, 3)
   struct ds_Net_IIPFilterPriv** rxFilter;
   int rxFilterLen;
   int rxFilterLenReq;
   struct ds_Net_IIPFilterPriv** txFilter;
   int txFilterLen;
   int txFilterLenReq;
};
typedef struct ds_Net_QoSSpecType ds_Net_QoSSpecType;
struct ds_Net_QoSSpecPrimaryType {
   struct ds_Net_IQoSFlowPriv** rxFlows;
   int rxFlowsLen;
   int rxFlowsLenReq;
   boolean rxMinFlow;
   AEEINTERFACE_PADMEMBERS(txFlows, 3)
   struct ds_Net_IQoSFlowPriv** txFlows;
   int txFlowsLen;
   int txFlowsLenReq;
   boolean txMinFlow;
   AEEINTERFACE_PADMEMBERS(__pad, 3)
};
typedef struct ds_Net_QoSSpecPrimaryType ds_Net_QoSSpecPrimaryType;
struct ds_Net__SeqQoSSpecType__seq_QoSSpecType_Net_ds {
   ds_Net_QoSSpecType* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net__SeqQoSSpecType__seq_QoSSpecType_Net_ds ds_Net__SeqQoSSpecType__seq_QoSSpecType_Net_ds;
typedef ds_Net__SeqQoSSpecType__seq_QoSSpecType_Net_ds ds_Net_SeqQoSSpecType;
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Net_IQoSFlowPriv.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_IIPFilterPriv.h"

/**
  * ds Net QoS Def Private module.
  * This module groups all the Private QoS types and constants.
  */
namespace ds
{
   namespace Net
   {
      struct _SeqIQoSFlowPrivType__seq_IQoSFlowPriv_Net_ds {
         IQoSFlowPriv** data;
         int dataLen;
         int dataLenReq;
      };
      typedef _SeqIQoSFlowPrivType__seq_IQoSFlowPriv_Net_ds SeqIQoSFlowPrivType;
      struct QoSSpecType {
         IQoSFlowPriv** rxFlows;
         int rxFlowsLen;
         int rxFlowsLenReq;
         boolean rxMinFlow;
         AEEINTERFACE_PADMEMBERS(txFlows, 3)
         IQoSFlowPriv** txFlows;
         int txFlowsLen;
         int txFlowsLenReq;
         boolean txMinFlow;
         AEEINTERFACE_PADMEMBERS(rxFilter, 3)
         IIPFilterPriv** rxFilter;
         int rxFilterLen;
         int rxFilterLenReq;
         IIPFilterPriv** txFilter;
         int txFilterLen;
         int txFilterLenReq;
      };
      struct QoSSpecPrimaryType {
         IQoSFlowPriv** rxFlows;
         int rxFlowsLen;
         int rxFlowsLenReq;
         boolean rxMinFlow;
         AEEINTERFACE_PADMEMBERS(txFlows, 3)
         IQoSFlowPriv** txFlows;
         int txFlowsLen;
         int txFlowsLenReq;
         boolean txMinFlow;
         AEEINTERFACE_PADMEMBERS(__pad, 3)
      };
      struct _SeqQoSSpecType__seq_QoSSpecType_Net_ds {
         QoSSpecType* data;
         int dataLen;
         int dataLenReq;
      };
      typedef _SeqQoSSpecType__seq_QoSSpecType_Net_ds SeqQoSSpecType;
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_QOS_DEFPRIV_H
