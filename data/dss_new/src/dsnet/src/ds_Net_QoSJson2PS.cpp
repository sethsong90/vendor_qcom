/*===========================================================================
FILE: ds_Net_QoSJSon2PS.cpp

OVERVIEW: This file provides implementation of the QoSJSon2PS class.

DEPENDENCIES: None

Copyright (c) 2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEstd.h"
#include "AEEStdErr.h"
#include "ds_Net_QoSJson2PS.h"
#include "ds_Net_QoSSecondary.h"
#include "ds_Utils_CreateInstance.h"
#include "ds_Net_Privileges_Def.h"
#include "AEEIPrivSet.h"
#include "AEEStdinet.h"
#include "AEENetAddrTypes.h"

#include "AEECEnv.h"

#include "AEECJSONTree.h"

using namespace ds::Error;
using namespace ds::Net;


const QoSJSon2PS::Str2CodeTable TrafficClassValues[] = 
{ 
  {"conversational", QoSJSon2PS::CONVERSATIONAL}, 
  {"streaming", QoSJSon2PS::STREAMING}, 
  {"interactive", QoSJSon2PS::INTERACTIVE}, 
  {"background", QoSJSon2PS::BACKGROUND},

  {NULL, -1}
};

const QoSJSon2PS::Str2CodeTable DataRateFormatValues[] = 
{ 
  {"min_max", QoSJSon2PS::MIN_MAX}, 
  {"token_bucket", QoSJSon2PS::TOKEN_BUCKET}, 

  {NULL, -1}
};

const QoSJSon2PS::Str2CodeTable WlanUserPriorityValues[] = 
{ 
  {"best_effort", QoSJSon2PS::BEST_EFFORT}, 
  {"background", QoSJSon2PS::BACKGROUND_P}, 
  {"reserved", QoSJSon2PS::RESERVED}, 
  {"excellent_effort", QoSJSon2PS::EXCELLENT_EFFORT},
  {"controlled_load", QoSJSon2PS::CONTROLLED_LOAD},
  {"video", QoSJSon2PS::VIDEO},
  {"voice", QoSJSon2PS::VOICE},
  {"network_control", QoSJSon2PS::NETWORK_CONTROL},

  {NULL, -1}
};

QoSJSon2PS::QoSJSon2PS()
: refCnt (1),
weakRefCnt (1),
mpIEnv(NULL),
mpiTree(NULL),
mPSQoSSpecWrapper(NULL)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p", this, 0, 0);  
}

void QoSJSon2PS::Destructor
(
 void
 )
 throw()
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

} /* QoSJSon2PS::Destructor() */

QoSJSon2PS::~QoSJSon2PS()
throw()
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  DS_UTILS_RELEASEIF(mpIEnv); /*lint !e1550 !e1551 */
  DS_UTILS_RELEASEIF(mPSQoSSpecWrapper); /*lint !e1550 !e1551 */
  DS_UTILS_RELEASEIF(mpiTree); /*lint !e1550 !e1551 */

} /* QoSJSon2PS::~QoSJSon2PS() */


/*---------------------------------------------------------------------------
Inherited functions from IQoSJSon2PS.
---------------------------------------------------------------------------*/
::AEEResult QoSJSon2PS::ConvertJSon2PSQoSSpec
(
 const char* sQoSSpec,
 NetPlatform::PSQoSSpecType** pPSQoSFlowSpecArray,
 int* piSpecArrayLen
)
{
  int nErr, nErrRx, nErrTx, n;
  IQI *piQI=0;
  JSValue *pjvRoot=0;
  JSValue *pjvArray, *pjvSpecObj;
  JSPair pair;
  JSONType type;
  (*piSpecArrayLen) = 0; //make sure the piSpecArrayLen is initialized to 0

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
  Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == sQoSSpec || NULL == pPSQoSFlowSpecArray || NULL == piSpecArrayLen)
  {
    LOG_MSG_ERROR ("NULL received in one of the arguments ", 0, 0, 0);
    return AEE_EFAILED;
  }

  if (NULL == mpIEnv)
  {
    nErr = env_CreateInstance(AEECLSID_CEnv, (void **) &mpIEnv);
    if (AEE_SUCCESS != nErr)
    {
      LOG_MSG_ERROR("Cannot recover a CS environment pointer, CS ret=%d",
        nErr, 0, 0);
      goto bail;
    }
  }

  if(NULL == mpiTree)
  {
    nErr = mpIEnv->CreateInstance(AEECLSID_CJSONTree, (void **)&mpiTree);
    if(AEE_SUCCESS != nErr)
    {
      goto bail;
    }
  }

  // Pars the JSon string into JSon Value
  nErr = mpiTree->ParseJSON(sQoSSpec, 0, &pjvRoot);
  if(AEE_SUCCESS != nErr)
  {
    goto bail;
  }

  // Go over all the QoSSpecs in the request
  // Get RxFlows
  nErr = mpiTree->ObjectGet(pjvRoot, "QoSSpecs", &pair);
  if (nErr != AEE_SUCCESS) 
  {
     return AEE_EFAILED;
  }
  pjvArray = pair.pjv;

  // verify that we got an array. This array should contain QoSSpecs.
  nErr = mpiTree->GetType(pjvArray, &type);
  if (type != JSONArray)
  {
    return AEE_EFAILED;
  }

  // Get the number of elements in the array
  nErr = mpiTree->GetNumEntries(pjvArray, &n);
  if (nErr != AEE_SUCCESS)
  {
    return AEE_EFAILED;
  }

  // Allocate an array of pointers to PSQoSSpecType.
  *pPSQoSFlowSpecArray = new NetPlatform::PSQoSSpecType[n];
  if (NULL == *pPSQoSFlowSpecArray)
  {
     return AEE_ENOMEMORY;
  }

  // Nullify the pPSQoSFlowSpecArray array
  memset (*pPSQoSFlowSpecArray, 0, sizeof(NetPlatform::PSQoSSpecType)*n);

  (*piSpecArrayLen) = n;
  // Extract each of the QoSSpecs from the array and convert them.
  for(int i=0 ; i<n ; i++)
  {
    (*pPSQoSFlowSpecArray)[i].field_mask = 0;

    nErr = mpiTree->ArrayGet(pjvArray, i, &pjvSpecObj);
    if (type != JSONArray)
    {
      return AEE_EFAILED;
    }

    // The QoSSpec should have at least one direction flow (Rx or Tx).

    // Check if there are RxFlows and convert them 
    nErrRx = ConvertJSonObj2PS(pjvSpecObj, "RxFlows", RX_FLOW, &((*pPSQoSFlowSpecArray)[i]));


    // Check if there are TxFlows and convert them 
    nErrTx = ConvertJSonObj2PS(pjvSpecObj, "TxFlows", TX_FLOW, &((*pPSQoSFlowSpecArray)[i]));
    
    // If there are no Rx and no Tx flows, this QoS spec is not valid
    if((AEE_SUCCESS != nErrRx) && (AEE_SUCCESS != nErrTx))
    {
       goto bail;
    }

    // Convert RxFilters
    nErrRx = ConvertJSonObj2PS(pjvSpecObj, "RxFilters", RX_FILTER, &((*pPSQoSFlowSpecArray)[i]));

    // Convert TxFilters
    nErrTx = ConvertJSonObj2PS(pjvSpecObj, "TxFilters", TX_FILTER, &((*pPSQoSFlowSpecArray)[i]));
  }

  return AEE_SUCCESS;

bail:
  mpiTree->DeleteValue(pjvRoot);

  return AEE_EFAILED;

} /* ConvertJSon2PSQoSSpec() */


::AEEResult QoSJSon2PS::ConvertJSonObj2PS
(
  JSValue* pJSonValueRoot,
  char* sObjectName,
  JSonQoSObjectType objType,
  NetPlatform::PSQoSSpecType* pPSQoSSpec
)
{
  int nErr, n;
  JSValue *pjvArray, *pjvSpecObj;
  JSPair pair;
  JSONType type;
  boolean bMinFlow = FALSE;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
  Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == pJSonValueRoot || NULL == sObjectName || NULL == pPSQoSSpec)
  {
    LOG_MSG_ERROR ("NULL argument received", 0, 0, 0);
    return AEE_EFAILED;
  }


  if (NULL == mPSQoSSpecWrapper)
  {
    mPSQoSSpecWrapper = new PSQoSSpecWrapper;
  }

  // Get the requested JSon Object
  nErr = mpiTree->ObjectGet(pJSonValueRoot, sObjectName, &pair);
  if (AEE_SUCCESS != nErr)
  {
      return AEE_EFAILED;
  }
  pjvArray = pair.pjv;

  // verify that we got an array.
  nErr = mpiTree->GetType(pjvArray, &type);
  if (type != JSONArray || AEE_SUCCESS != nErr)
  {
    return AEE_EFAILED;
  }

  // Get the number of elements in the array
  nErr = mpiTree->GetNumEntries(pjvArray, &n);
  if (nErr != AEE_SUCCESS)
  {
    return AEE_EFAILED;
  }

  // Convert all the JSon objects to PS objecs
  for(int i=0 ; i<n ; i++)
  {
    nErr = mpiTree->ArrayGet(pjvArray, i, &pjvSpecObj);
    if (type != JSONArray)
    {
      return AEE_EFAILED;
    }

    switch(objType)
    {
      case RX_FLOW:
      {
         // allocate the Auxiliary Rx flows
         if(bMinFlow && n>2)
         {
            pPSQoSSpec->field_mask |= QOS_MASK_RX_AUXILIARY_FLOWS;
            // We have more than two flows and one of them is minimum flow. 
            // So we need to allocate auxiliary flows. 
            pPSQoSSpec->rx.flow_template.aux_flow_list_ptr = new NetPlatform::PSFlowSpecType[n-2];
            pPSQoSSpec->rx.flow_template.num_aux_flows = n-2;
         }

         // update the auxiliary flows number if there are no auxiliary flows.
         if(1 == n)
         {
            pPSQoSSpec->rx.flow_template.num_aux_flows = 0;
         }

         if(FALSE == bMinFlow)
         {
            pPSQoSSpec->rx.flow_template.aux_flow_list_ptr = new NetPlatform::PSFlowSpecType[n-1];
            pPSQoSSpec->rx.flow_template.num_aux_flows = n-1;
         }

         pPSQoSSpec->field_mask |= QOS_MASK_RX_FLOW;

         // Get the RX min flow flag
         nErr = mpiTree->ObjectGet(pJSonValueRoot, "RxMinFlow", &pair);
         if (AEE_SUCCESS != nErr)
         {
            bMinFlow = FALSE;
         }
         else
         {
            nErr = mpiTree->GetBool(pair.pjv, &bMinFlow);
            if (AEE_SUCCESS != nErr)
            {
               return AEE_EFAILED;
            }

            if (TRUE == bMinFlow)
            {
               pPSQoSSpec->field_mask |= QOS_MASK_RX_MIN_FLOW;
            }
         }

        // Call the function that converts the JSon Flow to PS Flow.

        if (i == 0)
        {
           //This is the first flow that we convert. So we store it in the req flow.
           nErr = ConvertJSonFlow2PS(pjvSpecObj, &(pPSQoSSpec->rx.flow_template.req_flow));
        }

        if(i>0 && i<n-1)
        {
           //This is one of the auxiliary flows. 
           nErr = ConvertJSonFlow2PS(pjvSpecObj, &(pPSQoSSpec->rx.flow_template.aux_flow_list_ptr[i-1]));
        }

        if(i==n-1 && TRUE == bMinFlow)
        {
           //This is the last flow in the list and it is a minimum flow
           nErr = ConvertJSonFlow2PS(pjvSpecObj, &(pPSQoSSpec->rx.flow_template.min_req_flow));
        }
        break;
      }

      case TX_FLOW:
      {
         // allocate the Auxiliary Tx flows
         if(bMinFlow && n>2)
         {
            pPSQoSSpec->field_mask |= QOS_MASK_TX_AUXILIARY_FLOWS;
            // We have more than two flows and one of them is minimum flow. 
            // So we need to allocate auxiliary flows. 
            pPSQoSSpec->tx.flow_template.aux_flow_list_ptr = new NetPlatform::PSFlowSpecType[n-2];
            pPSQoSSpec->tx.flow_template.num_aux_flows = n-2;
         }

         // update the auxiliary flows number if there are no auxiliary flows.
         if(1 == n)
         {
            pPSQoSSpec->tx.flow_template.num_aux_flows = 0;
         }

         if(FALSE == bMinFlow)
         {
            pPSQoSSpec->tx.flow_template.aux_flow_list_ptr = new NetPlatform::PSFlowSpecType[n-1];
            pPSQoSSpec->tx.flow_template.num_aux_flows = n-1;
         }

         pPSQoSSpec->field_mask |= QOS_MASK_TX_FLOW;

         // Get the TX min flow flag
         nErr = mpiTree->ObjectGet(pJSonValueRoot, "TxMinFlow", &pair);
         if (AEE_SUCCESS != nErr)
         {
            bMinFlow = FALSE;
         }
         else
         {
            nErr = mpiTree->GetBool(pair.pjv, &bMinFlow);
            if (AEE_SUCCESS != nErr)
            {
               return AEE_EFAILED;
            }

            if (TRUE == bMinFlow)
            {
               pPSQoSSpec->field_mask |= QOS_MASK_TX_MIN_FLOW;
            }
         }

         // Call the function that converts the JSon Flow to PS Flow.

         if (i == 0)
         {
            //This is the first flow that we convert. So we store it in the req flow.
            nErr = ConvertJSonFlow2PS(pjvSpecObj, &(pPSQoSSpec->tx.flow_template.req_flow));
         }

         if(i>0 && i<n-1)
         {
            //This is one of the auxiliary flows. 
            nErr = ConvertJSonFlow2PS(pjvSpecObj, &(pPSQoSSpec->tx.flow_template.aux_flow_list_ptr[i-1]));
         }

         if(i==n-1 && TRUE == bMinFlow)
         {
            //This is the last flow in the list and it is a minimum flow
            nErr = ConvertJSonFlow2PS(pjvSpecObj, &(pPSQoSSpec->tx.flow_template.min_req_flow));
         }
         break;
      }

      case RX_FILTER:
      {
         // allocate the Rx filters list
         pPSQoSSpec->rx.fltr_template.list_ptr = new NetPlatform::PSFilterSpecType[n];
         pPSQoSSpec->rx.fltr_template.num_filters = n;

        // Call the function that converts the JSon Filter to PS Filter.
        nErr = QoSJSon2PS::ConvertJSonFilter2PS(pjvSpecObj, &(pPSQoSSpec->rx.fltr_template.list_ptr[i]));
        break;
      }

      case TX_FILTER:
      {
         // allocate the Tx filters list
         pPSQoSSpec->tx.fltr_template.list_ptr = new NetPlatform::PSFilterSpecType[n];
         pPSQoSSpec->tx.fltr_template.num_filters = n;

        // Call the function that converts the JSon Filter to PS Filter.
        nErr = QoSJSon2PS::ConvertJSonFilter2PS(pjvSpecObj, &(pPSQoSSpec->tx.fltr_template.list_ptr[i]));
        break;
      }

      default:
        return AEE_EFAILED;
    }
  }

  return AEE_SUCCESS;
} /* ConvertJSonFlow2PS() */

::AEEResult QoSJSon2PS::ConvertJSonFlow2PS
(
  JSValue* pJSonValueFlow,
  ip_flow_type* pFlow
)
{
  int nErr, nLen, code, n1, n2, n3, nDataRateFormat;
  JSPair pair;
  JSValue *pjvFlowObj;
  JSONType type;
  const char* pString;
  unsigned char flowMask = 0;
  boolean b;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
  Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == pJSonValueFlow || NULL == pFlow)
  {
    LOG_MSG_ERROR ("NULL argument received", 0, 0, 0);
    return AEE_EFAILED;
  }

  /*-------------------------------------------------------------------------
  Get Traffic Class
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "TrfClass", &pair);
  if (AEE_SUCCESS == nErr)
  {
     pjvFlowObj = pair.pjv;

     nErr = mpiTree->GetType(pjvFlowObj, &type);
     if (type != JSONString) 
     {
        return AEE_EFAILED;
     }

     nErr = mpiTree->GetString(pjvFlowObj, &pString, &nLen);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     // convert the string to code
     Str2Code(TrafficClassValues, pString, &code);

     mPSQoSSpecWrapper->SetIPVsn(IP_V6);

     switch (code)
     {
     case CONVERSATIONAL:
        // TODO: update mask!!!
        mPSQoSSpecWrapper->SetTrafficClass(IP_TRF_CLASS_CONVERSATIONAL, flowMask);
        break;
     case STREAMING:
        // TODO: update mask!!!
        mPSQoSSpecWrapper->SetTrafficClass(IP_TRF_CLASS_STREAMING, flowMask);
        break;
     case INTERACTIVE:
        // TODO: update mask!!!
        mPSQoSSpecWrapper->SetTrafficClass(IP_TRF_CLASS_INTERACTIVE, flowMask);
        break;
     case BACKGROUND:
        // TODO: update mask!!!
        mPSQoSSpecWrapper->SetTrafficClass(IP_TRF_CLASS_BACKGROUND, flowMask);
        break;
     default:
        return AEE_EFAILED;
     }
  }

  /*-------------------------------------------------------------------------
  Get Data Rate Format
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "DataRateFormat", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFlowObj = pair.pjv;

     // Get the data rate format string
     nErr = mpiTree->GetString(pjvFlowObj, &pString, &nLen);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     // convert the string to code
     Str2Code(DataRateFormatValues, pString, &nDataRateFormat);

     /*-------------------------------------------------------------------------
     Get Data Rate min max
     -------------------------------------------------------------------------*/
     if (MIN_MAX == nDataRateFormat)
     {
        nErr = mpiTree->ObjectGet(pJSonValueFlow, "DataRateMinMax", &pair);
        if (nErr == AEE_SUCCESS) 
        {
           pjvFlowObj = pair.pjv;

           // Get Max Rate
           nErr = mpiTree->ObjectGet(pjvFlowObj, "MaxRate", &pair);
           nErr = mpiTree->GetInt(pair.pjv, &n1);
           if (nErr != AEE_SUCCESS) 
           {
              return AEE_EFAILED;
           }

           // Get Guaranteed Rate
           nErr = mpiTree->ObjectGet(pjvFlowObj, "GuaranteedRate", &pair);
           nErr = mpiTree->GetInt(pair.pjv, &n2);
           if (nErr != AEE_SUCCESS) 
           {
              return AEE_EFAILED;
           }

           mPSQoSSpecWrapper->SetDataRateMinMax(n1, n2);
        }  
     }

     /*-------------------------------------------------------------------------
     Get Data Rate Token Bucket
     -------------------------------------------------------------------------*/
     if (TOKEN_BUCKET == nDataRateFormat)
     {
        nErr = mpiTree->ObjectGet(pJSonValueFlow, "DataRateTokenBucket", &pair);
        if (nErr == AEE_SUCCESS) 
        {
           pjvFlowObj = pair.pjv;

           // Get Peak Rate
           nErr = mpiTree->ObjectGet(pjvFlowObj, "PeakRate", &pair);
           nErr = mpiTree->GetInt(pair.pjv, &n1);
           if (nErr != AEE_SUCCESS) 
           {
              return AEE_EFAILED;
           }

           // Get Token Rate
           nErr = mpiTree->ObjectGet(pjvFlowObj, "TokenRate", &pair);
           nErr = mpiTree->GetInt(pair.pjv, &n2);
           if (nErr != AEE_SUCCESS) 
           {
              return AEE_EFAILED;
           }

           // Get Size
           nErr = mpiTree->ObjectGet(pjvFlowObj, "Size", &pair);
           nErr = mpiTree->GetInt(pair.pjv, &n3);
           if (nErr != AEE_SUCCESS) 
           {
              return AEE_EFAILED;
           }

           mPSQoSSpecWrapper->SetDataRateTokenBucket(n1, n2, n3);
        }
     }
  } 



  /*-------------------------------------------------------------------------
  Get Latency
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "Latency", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFlowObj = pair.pjv;

     // Get val
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetLatency(n1);
  }
  
  /*-------------------------------------------------------------------------
  Get Latency Variance
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "LatencyVariance", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFlowObj = pair.pjv;

     // Get val
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetLatencyVar(n1);
  }

  /*-------------------------------------------------------------------------
  Get Pkt Err Rate
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "PktErrRate", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFlowObj = pair.pjv;

     // Get Multiplier
     nErr = mpiTree->ObjectGet(pjvFlowObj, "Multiplier", &pair);
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     // Get Exponent
     nErr = mpiTree->ObjectGet(pjvFlowObj, "Exponent", &pair);
     nErr = mpiTree->GetInt(pair.pjv, &n2);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetPktErrRate(n1, n2);
  }
  
  /*-------------------------------------------------------------------------
  Get Min Policed Pkt Size
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "MinPolicedPktSize", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFlowObj = pair.pjv;

     // Get val
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetMinPolicedPktSize(n1);
  }

  /*-------------------------------------------------------------------------
  Get Max Allowed Pkt Size
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "MaxAllowedPktSize", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFlowObj = pair.pjv;

     // Get val
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetMaxAllowedPktSize(n1);
  }

  /*-------------------------------------------------------------------------
  Get Pkt Err Rate
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "NominalSDUSize", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFlowObj = pair.pjv;

     // Get Fixed
     nErr = mpiTree->ObjectGet(pjvFlowObj, "Fixed", &pair);
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     // Get Size
     nErr = mpiTree->ObjectGet(pjvFlowObj, "Size", &pair);
     nErr = mpiTree->GetInt(pair.pjv, &n2);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetNominalSDUSize(n1, n2);
  }

  /*-------------------------------------------------------------------------
  Get Umts Residual Ber
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "UmtsResidualBer", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFlowObj = pair.pjv;

     // Get val
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetUmtsResBer(n1);
  }


  /*-------------------------------------------------------------------------
  Get Umts Trf Pri
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "UmtsTrfPri", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFlowObj = pair.pjv;

     // Get val
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetUmtsTrfPri(n1);
  }

  /*-------------------------------------------------------------------------
  Get Umts ImCn Flag
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "UmtsImCnFlag", &pair);
  if (AEE_SUCCESS == nErr)
  {
     pjvFlowObj = pair.pjv;

     // Get val
     nErr = mpiTree->GetBool(pair.pjv, &b);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetUmtsImCnFlag(b);
  }

  /*-------------------------------------------------------------------------
  Get Umts Sig Ind
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "UmtsSigInd", &pair);
  if (AEE_SUCCESS == nErr)
  {
     pjvFlowObj = pair.pjv;

     // Get val
     nErr = mpiTree->GetBool(pair.pjv, &b);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetUmtsSigInd(b);
  }

  /*-------------------------------------------------------------------------
  Get Cdma Profile ID
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "CdmaProfileID", &pair);
  if (AEE_SUCCESS == nErr)
  {
     pjvFlowObj = pair.pjv;

     // Get val
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetCdmaProfileID(n1);
  }

  /*-------------------------------------------------------------------------
  Get Cdma Flow Priority
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "CdmaFlowPriority", &pair);
  if (AEE_SUCCESS == nErr)
  {
     pjvFlowObj = pair.pjv;

     // Get val
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetCdmaFlowPriority(n1);
  }

  /*-------------------------------------------------------------------------
  Get Wlan User Priority
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "WlanUserPriority", &pair);
  if (AEE_SUCCESS == nErr)
  {
     pjvFlowObj = pair.pjv;

     nErr = mpiTree->GetType(pjvFlowObj, &type);
     if (type != JSONString) 
     {
        return AEE_EFAILED;
     }

     nErr = mpiTree->GetString(pjvFlowObj, &pString, &nLen);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     // convert the string to code
     Str2Code(TrafficClassValues, pString, &code);

     switch (code)
     {
     case BEST_EFFORT:
        mPSQoSSpecWrapper->SetWlanUserPriority(WLAN_USER_PRI_BEST_EFFORT);
        break;
     case BACKGROUND_P:
        mPSQoSSpecWrapper->SetWlanUserPriority(WLAN_USER_PRI_BACKGROUND);
        break;
     case RESERVED:
        mPSQoSSpecWrapper->SetWlanUserPriority(WLAN_USER_PRI_RESERVED);
        break;
     case EXCELLENT_EFFORT:
        mPSQoSSpecWrapper->SetWlanUserPriority(WLAN_USER_PRI_EXCELLENT_EFFORT);
        break;
     case CONTROLLED_LOAD:
        mPSQoSSpecWrapper->SetWlanUserPriority(WLAN_USER_PRI_CONTROLLED_LOAD);
        break;
     case VIDEO:
        mPSQoSSpecWrapper->SetWlanUserPriority(WLAN_USER_PRI_VIDEO);
        break;
     case VOICE:
        mPSQoSSpecWrapper->SetWlanUserPriority(WLAN_USER_PRI_VOICE);
        break;
     case NETWORK_CONTROL:
        mPSQoSSpecWrapper->SetWlanUserPriority(WLAN_USER_PRI_NETWORK_CONTROL);
        break;
     default:
        return AEE_EFAILED;
     }
  }

  /*-------------------------------------------------------------------------
  Get Wlan Min Service Interval
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "WlanMinServiceInterval", &pair);
  if (AEE_SUCCESS == nErr)
  {
     pjvFlowObj = pair.pjv;

     // Get val
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetWlanMinServiceInterval(n1);
  }

  /*-------------------------------------------------------------------------
  Get Wlan Max Service Interval
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "WlanMaxServiceInterval", &pair);
  if (AEE_SUCCESS == nErr)
  {
     pjvFlowObj = pair.pjv;

     // Get val
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetWlanMaxServiceInterval(n1);
  }

  /*-------------------------------------------------------------------------
  Get Wlan Inactivity Interval
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFlow, "WlanInactivityInterval", &pair);
  if (AEE_SUCCESS == nErr)
  {
     pjvFlowObj = pair.pjv;

     // Get Peak Rate
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetWlanInactivityInterval(n1);
  }

  /*-------------------------------------------------------------------------
  Copy the internal PSQoSSpecWrapper ps flow to the user ps flow
  -------------------------------------------------------------------------*/
  mPSQoSSpecWrapper->GetPSQoSFlowSpec(pFlow);

  return AEE_SUCCESS;
}

::AEEResult QoSJSon2PS::ConvertJSonFilter2PS
(
 JSValue* pJSonValueFilter,
 ip_filter_type* pFilter
)
{
  int nErr, nLen, n1, n2;
  JSPair pair;
  JSValue *pjvFilterObj;
  JSONType type;
  const char* pString;
  unsigned char flowMask = 0;
  ds::INAddr6Type addr;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
  Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == pJSonValueFilter || NULL == pFilter)
  {
    LOG_MSG_ERROR ("NULL argument received", 0, 0, 0);
    return AEE_EFAILED;
  }

  /*-------------------------------------------------------------------------
  Get IP Vsn
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFilter, "IPVsn", &pair);
  if(AEE_SUCCESS != nErr)
  {
     LOG_MSG_ERROR ("IPVsn is not set.", 0, 0, 0);
     return AEE_EFAILED;
  }

  pjvFilterObj = pair.pjv;

  // Get val
  nErr = mpiTree->GetInt(pair.pjv, &n1);
  if (nErr != AEE_SUCCESS) 
  {
    return AEE_EFAILED;
  }

  mPSQoSSpecWrapper->SetIPVsn(n1);

  /*-------------------------------------------------------------------------
  Get Next Hdr Prot
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFilter, "NextHdrProt", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFilterObj = pair.pjv;

     // Get val
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetNextHdrProt(n1);
  }


  /*-------------------------------------------------------------------------
  Get Src Port
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFilter, "SrcPort", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFilterObj = pair.pjv;

     // Get Port
     nErr = mpiTree->ObjectGet(pjvFilterObj, "Port", &pair);
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     // Get Range
     nErr = mpiTree->ObjectGet(pjvFilterObj, "Range", &pair);
     nErr = mpiTree->GetInt(pair.pjv, &n2);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }
  }


  /*-------------------------------------------------------------------------
  Get Dst Port
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFilter, "DstPort", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFilterObj = pair.pjv;

     // Get Port
     nErr = mpiTree->ObjectGet(pjvFilterObj, "Port", &pair);
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     // Get Range
     nErr = mpiTree->ObjectGet(pjvFilterObj, "Range", &pair);
     nErr = mpiTree->GetInt(pair.pjv, &n2);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetDstPort(n1, n2);
  }

  /*-------------------------------------------------------------------------
  Get Src Addr
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFilter, "SrcAddr", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFilterObj = pair.pjv;


     // Get Addr
     nErr = mpiTree->ObjectGet(pjvFilterObj, "Addr", &pair);
     nErr = mpiTree->GetType(pair.pjv, &type);
     if (type != JSONString) 
     {
        return AEE_EFAILED;
     }

     nErr = mpiTree->GetString(pair.pjv, &pString, &nLen);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     //Convert string to address
     std_inet_pton(AEE_AF_INET6, pString, addr);

     mPSQoSSpecWrapper->SetSrcV6 (addr, 0);

     // Get SubnetMask
     //nErr = mpiTree->ObjectGet(pjvFilterObj, "SubnetMask", &pair);
     //nErr = mpiTree->GetType(pair.pjv, &type);
     //if (type != JSONString) 
     //{
     //  return AEE_EFAILED;
     //}

     //nErr = mpiTree->GetString(pair.pjv, &pString, &nLen);
     //if (nErr != AEE_SUCCESS) 
     //{
     //  return AEE_EFAILED;
     //}

     ////Convert string to address
     //std_inet_pton(AEE_AF_INET6, pString, addr.u6_addr8);

     //mPSQoSSpecWrapper->SetSrcV6 (addr, 0);
  }


  /*-------------------------------------------------------------------------
  Get Dst Addr
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFilter, "DstAddr", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFilterObj = pair.pjv;


     // Get Addr
     nErr = mpiTree->ObjectGet(pjvFilterObj, "Addr", &pair);
     nErr = mpiTree->GetType(pair.pjv, &type);
     if (type != JSONString) 
     {
        return AEE_EFAILED;
     }

     nErr = mpiTree->GetString(pair.pjv, &pString, &nLen);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     //Convert string to address
     std_inet_pton(AEE_AF_INET6, pString, addr);

     mPSQoSSpecWrapper->SetDstV6 (addr, 0);

     // Get SubnetMask
     //nErr = mpiTree->ObjectGet(pjvFilterObj, "SubnetMask", &pair);
     //nErr = mpiTree->GetType(pair.pjv, &type);
     //if (type != JSONString) 
     //{
     //  return AEE_EFAILED;
     //}

     //nErr = mpiTree->GetString(pair.pjv, &pString, &nLen);
     //if (nErr != AEE_SUCCESS) 
     //{
     //  return AEE_EFAILED;
     //}

     ////Convert string to address
     //std_inet_pton(AEE_AF_INET6, pString, addr.u6_addr8);

     //mPSQoSSpecWrapper->SetSrcV6 (addr, 0);
  }

  /*-------------------------------------------------------------------------
  Get ToS
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFilter, "ToS", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFilterObj = pair.pjv;

     // Get Val
     nErr = mpiTree->ObjectGet(pjvFilterObj, "Val", &pair);
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     // Get Mask
     nErr = mpiTree->ObjectGet(pjvFilterObj, "Mask", &pair);
     nErr = mpiTree->GetInt(pair.pjv, &n2);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetTos(n1, n2);
  }
 

  /*-------------------------------------------------------------------------
  Get Flow Label
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFilter, "FlowLabel", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFilterObj = pair.pjv;

     // Get Val
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetFlowLabel(n1);
  }


  /*-------------------------------------------------------------------------
  Get Traffic Class
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFilter, "TrafficClass", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFilterObj = pair.pjv;

     // Get Val
     nErr = mpiTree->ObjectGet(pjvFilterObj, "Val", &pair);
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     // Get Mask
     nErr = mpiTree->ObjectGet(pjvFilterObj, "Mask", &pair);
     nErr = mpiTree->GetInt(pair.pjv, &n2);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetTrafficClass(n1, n2);
  }

  /*-------------------------------------------------------------------------
  Get EspSpi
  -------------------------------------------------------------------------*/
  nErr = mpiTree->ObjectGet(pJSonValueFilter, "EspSpi", &pair);
  if (nErr == AEE_SUCCESS) 
  {
     pjvFilterObj = pair.pjv;

     // Get Val
     nErr = mpiTree->GetInt(pair.pjv, &n1);
     if (nErr != AEE_SUCCESS) 
     {
        return AEE_EFAILED;
     }

     mPSQoSSpecWrapper->SetEspSpi(n1);
  }

  /*-------------------------------------------------------------------------
  Copy the internal PSQoSSpecWrapper ps Filter to the user ps Filter
  -------------------------------------------------------------------------*/
  mPSQoSSpecWrapper->GetPSIPFilterSpec(pFilter);

  return AEE_SUCCESS;
}

::AEEResult QoSJSon2PS::GenerateJSonErrFromPSSpec
(
 NetPlatform::PSQoSSpecType* pPSQoSFlowSpecArray,
 int piSpecArrayLen,
 char* sQoSSpecErr
)
{
  int nErr, numOfAuxFlows, numOfFilters, nLen = 0, nLenReq;
  JSValue* pjvRoot = 0, *pjvArray = 0, *pjvInt = 0;
  uint32 errMask;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
  Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == pPSQoSFlowSpecArray)
  {
     LOG_MSG_ERROR ("pPSQoSFlowSpecArray == NULL", 0, 0, 0);
     return AEE_EFAILED;
  }

  if (NULL == mpIEnv)
  {
    nErr = env_CreateInstance(AEECLSID_CEnv, (void **) &mpIEnv);
    if (AEE_SUCCESS != nErr)
    {
      LOG_MSG_ERROR("Cannot recover a CS environment pointer, CS ret=%d",
        nErr, 0, 0);
      goto bail;
    }
  }

  if(NULL == mpiTree)
  {
    nErr = mpIEnv->CreateInstance(AEECLSID_CJSONTree, (void **)&mpiTree);
    if(AEE_SUCCESS != nErr) 
    {
      goto bail;
    }
  }

  // create an empty JSon tree. This tree will contain all the errors
  mpiTree->ParseJSON( "{}", std_strlen("{}"), &pjvRoot);

  // create and empty array. This array shall contain all the errors. 
  mpiTree->CreateArray(&pjvArray);

  ///*-------------------------------------------------------------------------
  //Go over the QoS Specs and convert all the error masks to JSon string
  //-------------------------------------------------------------------------*/

  for (int qosSpecIndex=0; qosSpecIndex<piSpecArrayLen; qosSpecIndex++)
  {

    /*-------------------------------------------------------------------------
    Rx Flows conversion
    -------------------------------------------------------------------------*/

    // Get the req flow errors
    errMask = pPSQoSFlowSpecArray[qosSpecIndex].rx.flow_template.req_flow.err_mask;

    // Convert the requested tx flow errors to JSon 
    ConvertPSErrors2JSon(pjvArray, errMask, qosSpecIndex, "RxFlow", 0, 0, RX_FLOW);

    numOfAuxFlows = pPSQoSFlowSpecArray[qosSpecIndex].rx.flow_template.num_aux_flows;

    for(int flowIndex=1 ; flowIndex <= numOfAuxFlows ; flowIndex++)
    {
      errMask = pPSQoSFlowSpecArray[qosSpecIndex].rx.flow_template.aux_flow_list_ptr[flowIndex].err_mask;

      // Convert all the aux tx flow errors to JSon 
      ConvertPSErrors2JSon(pjvArray, errMask, qosSpecIndex, "RxFlow", flowIndex, 0, RX_FLOW);
    }

    /*-------------------------------------------------------------------------
    Tx Flows conversion
    -------------------------------------------------------------------------*/

    // Get the req flow errors
    errMask = pPSQoSFlowSpecArray[qosSpecIndex].tx.flow_template.req_flow.err_mask;

    // Convert the requested tx flow errors to JSon 
    ConvertPSErrors2JSon(pjvArray, errMask, qosSpecIndex, "TxFlow", 0, 0, TX_FLOW);

    numOfAuxFlows = pPSQoSFlowSpecArray[qosSpecIndex].tx.flow_template.num_aux_flows;

    for(int flowIndex=1 ; flowIndex <= numOfAuxFlows ; flowIndex++)
    {
      errMask = pPSQoSFlowSpecArray[qosSpecIndex].tx.flow_template.aux_flow_list_ptr[flowIndex].err_mask;

      // Convert all the aux tx flow errors to JSon 
      ConvertPSErrors2JSon(pjvArray, errMask, qosSpecIndex, "TxFlow", flowIndex, 0, TX_FLOW);
    }

    /*-------------------------------------------------------------------------
    Rx Filters conversion
    -------------------------------------------------------------------------*/
    numOfFilters = pPSQoSFlowSpecArray[qosSpecIndex].rx.fltr_template.num_filters;

    for(int filterIndex=1 ; filterIndex <= numOfFilters ; filterIndex++)
    {
      errMask = pPSQoSFlowSpecArray[qosSpecIndex].rx.fltr_template.list_ptr[filterIndex].ip_hdr.v6.err_mask;

      // Convert all the aux tx flow errors to JSon 
      ConvertPSErrors2JSon(pjvArray, errMask, qosSpecIndex, "RxFilter", filterIndex, 0, RX_FILTER);
    }

    /*-------------------------------------------------------------------------
    Tx Filters conversion
    -------------------------------------------------------------------------*/
    numOfFilters = pPSQoSFlowSpecArray[qosSpecIndex].tx.fltr_template.num_filters;

    for(int filterIndex=1 ; filterIndex <= numOfFilters ; filterIndex++)
    {
      errMask = pPSQoSFlowSpecArray[qosSpecIndex].tx.fltr_template.list_ptr[filterIndex].ip_hdr.v6.err_mask;

      // Convert all the aux tx flow errors to JSon 
      ConvertPSErrors2JSon(pjvArray, errMask, qosSpecIndex, "TxFilter", filterIndex, 0, RX_FILTER);
    }
  }

  // Add the error array to the JSon Error object
  mpiTree->ObjectSet(pjvRoot, "QoSSpecs", pjvArray);

  // Turn the JSon value to JSon string.
  mpiTree->Serialize(pjvRoot, sQoSSpecErr, nLen, &nLenReq);

bail:
  //Release the JSon tree
  mpiTree->DeleteValue(pjvRoot);
  return AEE_SUCCESS;

} /* GenerateJSonErrFromPSSpec() */

::AEEResult QoSJSon2PS::ConvertPSErrors2JSon
(
  JSValue* pJSonValueErrorArray,
  uint32 errMask,
  int qoSSpecIndex,
  char* context,
  int index,
  int errType,
  JSonQoSObjectType objectType
)
{
  
  switch(objectType)
  {
    case RX_FLOW:
    case TX_FLOW:
    {
      if (IPFLOW_MASK_TRF_CLASS & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
                            context, index, "FlowOption", "TrfClass", errType);
      }

      if (IPFLOW_MASK_DATA_RATE & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "DataRateMinMax", errType);
      }

      if (IPFLOW_MASK_LATENCY & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "Latency", errType);
      }
      
      if (IPFLOW_MASK_LATENCY_VAR & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "LatencyVariance", errType);
      }

      if (IPFLOW_MASK_PKT_ERR_RATE & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "PktErrRate", errType);
      }

      if (IPFLOW_MASK_MIN_POLICED_PKT_SIZE & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "MinPolicedPktSize", errType);
      }

      if (IPFLOW_MASK_MAX_ALLOWED_PKT_SIZE & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "MaxAllowedPktSize", errType);
      }

      if (IPFLOW_MASK_UMTS_RES_BER & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "UmtsResidualBer", errType);
      }

      if (IPFLOW_MASK_UMTS_TRF_PRI & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "UmtsTrfPri", errType);
      }

      if (IPFLOW_MASK_CDMA_PROFILE_ID & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "CdmaProfileID", errType);
      } 

      if (IPFLOW_MASK_WLAN_USER_PRI & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "WlanUserPriority", errType);
      } 

      if (IPFLOW_MASK_WLAN_MIN_SERVICE_INTERVAL & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "WlanMinServiceInterval", errType);
      } 

      if (IPFLOW_MASK_WLAN_MAX_SERVICE_INTERVAL & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "WlanMaxServiceInterval", errType);
      } 

      if (IPFLOW_MASK_WLAN_INACTIVITY_INTERVAL & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "WlanInactivityInterval", errType);
      } 

      if (IPFLOW_MASK_NOMINAL_SDU_SIZE & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "NominalSDUSize", errType);
      } 

      if (IPFLOW_MASK_CDMA_FLOW_PRIORITY & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "CdmaFlowPriority", errType);
      } 

      if (IPFLOW_MASK_UMTS_IM_CN_FLAG & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "UmtsImCnFlag", errType);
      } 

      if (IPFLOW_MASK_UMTS_SIG_IND & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "UmtsSigInd", errType);
      } 

      if (IPFLOW_MASK_LTE_QCI & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FlowOption", "LTEQCI", errType);
      } 

      break;
    }

    case RX_FILTER:
    case TX_FILTER:
    {

      if (IPFLTR_MASK_IP6_SRC_ADDR & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FilterOption", "SrcAddr", errType);
      }

      if (IPFLTR_MASK_IP6_DST_ADDR & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FilterOption", "DstAddr", errType);
      }

      if (IPFLTR_MASK_IP6_NEXT_HDR_PROT & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FilterOption", "NextHdrProt", errType);
      }

      if (IPFLTR_MASK_IP6_TRAFFIC_CLASS & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FilterOption", "TrafficClass", errType);
      }

      if (IPFLTR_MASK_IP6_FLOW_LABEL & errMask)
      {
        ConvertPSError2JSon(pJSonValueErrorArray, qoSSpecIndex,
          context, index, "FilterOption", "FlowLabel", errType);
      }

      break;
    }

    default:
      return AEE_EFAILED;
  }

  return AEE_SUCCESS;
}

::AEEResult QoSJSon2PS::ConvertPSError2JSon
(
 JSValue* pJSonValueErrorArray,
 int qoSSpecIndex,
 char* context,
 int index,
 char* flowOrFilter,
 char* errorOption,
 int errType
)
{
  JSValue *pjvInt = 0, *pjvError = 0, *pjvString = 0;

  //create an empty Json string. This string shall contain
  // one specific error.
  mpiTree->ParseJSON( "{}", std_strlen("{}"), &pjvError);

  // add the QoS Spec index
  mpiTree->CreateInt(qoSSpecIndex, &pjvInt);
  mpiTree->ObjectSet(pjvError, "QoSSpecIndex", pjvInt);

  // add the context
  mpiTree->CreateString(context, std_strlen(context), &pjvString);
  mpiTree->ObjectSet(pjvError, "Context", pjvString);

  // add the Flow/Filter index
  mpiTree->CreateInt(index, &pjvInt);
  mpiTree->ObjectSet(pjvError, "Index", pjvInt);

  // add the error type
  mpiTree->CreateString("UNSPEC", std_strlen("UNSPEC"), &pjvString);
  mpiTree->ObjectSet(pjvError, "ErrorType", pjvString);


  // add the error option
  mpiTree->CreateString(errorOption, std_strlen(context), &pjvString);
  mpiTree->ObjectSet(pjvError, flowOrFilter, pjvString);
  
  // add this error to the JSon error array
  mpiTree->ArrayAppend(pJSonValueErrorArray, pjvError); 

  return AEE_SUCCESS;
}

AEEResult QoSJSon2PS::Str2Code
(
   const Str2CodeTable *list,
   const char *str,
   int *code
)
{
  if (NULL == str || NULL == *str || NULL == list) {
    return AEE_EFAILED;
  }

  for (; NULL != list->str; list++) 
  {
    if(0 == std_strcmp(list->str, str))
    {
      *code = list->code;
      return AEE_SUCCESS;
    }
  }

  return AEE_EFAILED;
}

::AEEResult QoSJSon2PS::GenerateJSonFromQoSFlow 
(
  NetPlatform::PSFlowSpecType* pPSQoSFlowSpec,
  char* sQoSFlow,
  int nQoSFlowLen,
  int* nQoSFlowLenReq
)
{

  JSValue *pJSonValueFlow= 0,
          *pjvInt = 0,
          *pjvError = 0,
          *pjvString= 0,
          *pJSonValueTmp = 0;
  uint32 field_mask;
  int nErr;

  if (NULL == mpIEnv)
  {
     nErr = env_CreateInstance(AEECLSID_CEnv, (void **) &mpIEnv);
     if (AEE_SUCCESS != nErr)
     {
        LOG_MSG_ERROR("Cannot recover a CS environment pointer, CS ret=%d",
           nErr, 0, 0);
        return AEE_EFAILED;
     }
  }

  if(NULL == mpiTree)
  {
     nErr = mpIEnv->CreateInstance(AEECLSID_CJSONTree, (void **)&mpiTree);
     if(AEE_SUCCESS != nErr)
     {
        return AEE_EFAILED;
     }
  }

  //create an empty Json string. This string shall contain
  // one QoS Flow.
  mpiTree->ParseJSON( "{}", std_strlen("{}"), &pJSonValueFlow);

  field_mask = pPSQoSFlowSpec->field_mask;

  if (IPFLOW_MASK_TRF_CLASS & field_mask)
  {
     switch (pPSQoSFlowSpec->trf_class)
     {
     case IP_TRF_CLASS_CONVERSATIONAL:
        mpiTree->CreateString("conversational", std_strlen("conversational"), &pjvString);   
        break;
     case IP_TRF_CLASS_STREAMING:
        mpiTree->CreateString("streaming", std_strlen("streaming"), &pjvString);   
        break;
     case IP_TRF_CLASS_INTERACTIVE:
        mpiTree->CreateString("interactive", std_strlen("interactive"), &pjvString);   
        break;
     case IP_TRF_CLASS_BACKGROUND:
        mpiTree->CreateString("background", std_strlen("background"), &pjvString);   
        break;
     default:
        return AEE_EFAILED;
     }

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "TrfClass", pjvString);
  }

  if (IPFLOW_MASK_DATA_RATE & field_mask)
  {
     mpiTree->ParseJSON( "{}", std_strlen("{}"), &pJSonValueTmp);

     switch (pPSQoSFlowSpec->data_rate.format_type)
     {
     case DATA_RATE_FORMAT_MIN_MAX_TYPE:
        {
           mpiTree->CreateInt(pPSQoSFlowSpec->data_rate.format.min_max.max_rate, &pjvInt);
           mpiTree->ObjectSet(pJSonValueTmp, "MaxRate", pjvInt);

           mpiTree->CreateInt(pPSQoSFlowSpec->data_rate.format.min_max.guaranteed_rate, &pjvInt);
           mpiTree->ObjectSet(pJSonValueTmp, "GuaranteedRate", pjvInt);

           // add this setting to to the JSon QoS Flow
           mpiTree->ObjectSet(pJSonValueFlow, "DataRateMinMax", pJSonValueTmp);

           // also set the data rate format
           mpiTree->CreateString("min_max", std_strlen("min_max"), &pjvString);
           mpiTree->ObjectSet(pJSonValueFlow, "DataRateFormat", pjvString);
           break;
        }

     case DATA_RATE_FORMAT_TOKEN_BUCKET_TYPE:
        {
           mpiTree->CreateInt(pPSQoSFlowSpec->data_rate.format.token_bucket.peak_rate, &pjvInt);
           mpiTree->ObjectSet(pJSonValueTmp, "PeakRate", pjvInt);

           mpiTree->CreateInt(pPSQoSFlowSpec->data_rate.format.token_bucket.token_rate, &pjvInt);
           mpiTree->ObjectSet(pJSonValueTmp, "TokenRate", pjvInt);

           mpiTree->CreateInt(pPSQoSFlowSpec->data_rate.format.token_bucket.size, &pjvInt);
           mpiTree->ObjectSet(pJSonValueTmp, "Size", pjvInt);

           // add this setting to to the JSon QoS Flow
           mpiTree->ObjectSet(pJSonValueFlow, "DataRateTokenBucket", pJSonValueTmp);

           // also set the data rate format
           mpiTree->CreateString("token_bucket", std_strlen("token_bucket"), &pjvString);
           mpiTree->ObjectSet(pJSonValueFlow, "DataRateFormat", pjvString);
           break;
        }
     }
  }

  if (IPFLOW_MASK_LATENCY & field_mask)
  {
     mpiTree->CreateInt(pPSQoSFlowSpec->latency, &pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "Latency", pjvInt);
  }

  if (IPFLOW_MASK_LATENCY_VAR & field_mask)
  {
     mpiTree->CreateInt(pPSQoSFlowSpec->latency_var, &pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "LatencyVariance", pjvInt);
  }

  if (IPFLOW_MASK_PKT_ERR_RATE & field_mask)
  {
     mpiTree->ParseJSON( "{}", std_strlen("{}"), &pJSonValueTmp);

     mpiTree->CreateInt(pPSQoSFlowSpec->pkt_err_rate.exponent, &pjvInt);
     mpiTree->ObjectSet(pJSonValueTmp, "Exponent", pjvInt);

     mpiTree->CreateInt(pPSQoSFlowSpec->pkt_err_rate.multiplier, &pjvInt);
     mpiTree->ObjectSet(pJSonValueTmp, "Multiplier", pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "PktErrRate", pJSonValueTmp);

  }

  if (IPFLOW_MASK_MIN_POLICED_PKT_SIZE & field_mask)
  {
     mpiTree->CreateInt(pPSQoSFlowSpec->min_policed_pkt_size, &pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "MinPolicedPktSize", pjvInt);
  }

  if (IPFLOW_MASK_MAX_ALLOWED_PKT_SIZE & field_mask)
  {
     mpiTree->CreateInt(pPSQoSFlowSpec->max_allowed_pkt_size, &pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "MaxAllowedPktSize", pjvInt);
  }

  if (IPFLOW_MASK_UMTS_RES_BER & field_mask)
  {
     mpiTree->CreateInt(pPSQoSFlowSpec->umts_params.res_ber, &pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "UmtsResidualBer", pjvInt);
  }

  if (IPFLOW_MASK_UMTS_TRF_PRI & field_mask)
  {
     mpiTree->CreateInt(pPSQoSFlowSpec->umts_params.trf_pri, &pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "UmtsTrfPri", pjvInt);
  }

  if (IPFLOW_MASK_CDMA_PROFILE_ID & field_mask)
  {
     mpiTree->CreateInt(pPSQoSFlowSpec->cdma_params.profile_id, &pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "CdmaProfileID", pjvInt);
  } 

  if (IPFLOW_MASK_WLAN_USER_PRI & field_mask)
  {
     switch(pPSQoSFlowSpec->wlan_params.user_priority)
     {
     case BEST_EFFORT:
        mpiTree->CreateString("best_effort", std_strlen("best_effort"), &pjvString);
        break;
     case BACKGROUND_P:
        mpiTree->CreateString("background", std_strlen("background"), &pjvString);
        break;
     case RESERVED:
        mpiTree->CreateString("reserved", std_strlen("reserved"), &pjvString);
        break;
     case EXCELLENT_EFFORT:
        mpiTree->CreateString("excellent_effort", std_strlen("excellent_effort"), &pjvString);
        break;
     case CONTROLLED_LOAD:
        mpiTree->CreateString("controlled_load", std_strlen("controlled_load"), &pjvString);
        break;
     case VIDEO:
        mpiTree->CreateString("video", std_strlen("video"), &pjvString);
        break;
     case VOICE:
        mpiTree->CreateString("voice", std_strlen("voice"), &pjvString);
        break;
     case NETWORK_CONTROL:
        mpiTree->CreateString("network_control", std_strlen("network_control"), &pjvString);
        break;
     }

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "WlanUserPriority", pjvString);
  } 

  if (IPFLOW_MASK_WLAN_MIN_SERVICE_INTERVAL & field_mask)
  {
     mpiTree->CreateInt(pPSQoSFlowSpec->wlan_params.min_service_interval, &pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "WlanMinServiceInterval", pjvInt);
  } 

  if (IPFLOW_MASK_WLAN_MAX_SERVICE_INTERVAL & field_mask)
  {
     mpiTree->CreateInt(pPSQoSFlowSpec->wlan_params.max_service_interval, &pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "WlanMaxServiceInterval", pjvInt);
  } 

  if (IPFLOW_MASK_WLAN_INACTIVITY_INTERVAL & field_mask)
  {
     mpiTree->CreateInt(pPSQoSFlowSpec->wlan_params.inactivity_interval, &pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "WlanInactivityInterval", pjvInt);
  } 

  if (IPFLOW_MASK_NOMINAL_SDU_SIZE & field_mask)
  {
     mpiTree->ParseJSON( "{}", std_strlen("{}"), &pJSonValueTmp);

     mpiTree->CreateInt(pPSQoSFlowSpec->nominal_sdu_size.is_fixed, &pjvInt);
     mpiTree->ObjectSet(pJSonValueTmp, "Fixed", pjvInt);

     mpiTree->CreateInt(pPSQoSFlowSpec->nominal_sdu_size.size, &pjvInt);
     mpiTree->ObjectSet(pJSonValueTmp, "Size", pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "NominalSDUSize", pJSonValueTmp);
  } 

  if (IPFLOW_MASK_CDMA_FLOW_PRIORITY & field_mask)
  {
     mpiTree->CreateInt(pPSQoSFlowSpec->cdma_params.flow_priority, &pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "CdmaFlowPriority", pjvInt);
  } 

  if (IPFLOW_MASK_UMTS_IM_CN_FLAG & field_mask)
  {
     mpiTree->CreateInt(pPSQoSFlowSpec->umts_params.im_cn_flag, &pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "UmtsImCnFlag", pjvInt);
  } 

  if (IPFLOW_MASK_UMTS_SIG_IND & field_mask)
  {
     mpiTree->CreateInt(pPSQoSFlowSpec->umts_params.sig_ind, &pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "UmtsSigInd", pjvInt);
  } 

  if (IPFLOW_MASK_LTE_QCI & field_mask)
  {
     mpiTree->CreateInt(pPSQoSFlowSpec->lte_params.lte_qci, &pjvInt);

     // add this setting to to the JSon QoS Flow
     mpiTree->ObjectSet(pJSonValueFlow, "LTEQCI", pjvInt);
  } 


  // Turn the JSon value to JSon string.
  nErr = mpiTree->Serialize(pJSonValueFlow, sQoSFlow, nQoSFlowLen, nQoSFlowLenReq);
  if (nErr != AEE_SUCCESS) 
  {
     LOG_MSG_ERROR("JSon tree Serialize() failed. ret=%d",
        nErr, 0, 0);
     return nErr;
  }

  // Relesae the JSon tree
  mpiTree->DeleteValue(pJSonValueFlow);


  return AEE_SUCCESS;
}/*GenerateJSonFromQoSFlow()*/

