
/*===================================================

FILE:  DSS_QoSRequestExScope.cpp

SERVICES:
A utility class to automatically release various variables 
when out of scope

=====================================================

Copyright (c) 2011 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
EDIT HISTORY FOR MODULE

Please notice that the changes are listed in reverse chronological order.

$Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_QoSRequestExScope.cpp#1 $
$DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

when       who what, where, why
---------- --- ------------------------------------------------------------
2011-02-23 aa  History added.

===========================================================================*/

#include "customer.h"
#include "DSS_MemoryManagement.h"
#include "DSS_QoSRequestExScope.h"

using namespace ds::Net;

// You should define instance of this class with pointer to some (initialized) object 
// and type of this object , when you wish memory to be automatically freed on scope exit


DSSQoSRequestExScope::DSSQoSSpecScope::DSSQoSSpecScope():
   mrxFilter(NULL), mtxFilter(NULL), mrxFlows(NULL), mtxFlows(NULL),
   mnRxFilterLen(0), mnTxFilterLen(0), mnRxFlowsLen(0), mnTxFlowsLen(0){}

void 
DSSQoSRequestExScope::DSSQoSSpecScope::Init
(
 IIPFilterPriv** rxFilter,
 int nRxFilterLen,
 IIPFilterPriv** txFilter,
 int nTxFilterLen,
 IQoSFlowPriv** rxFlows,
 int nRxFlowsLen,
 IQoSFlowPriv** txFlows,
 int nTxFlowsLen
)
{
   mrxFilter = rxFilter;
   mnRxFilterLen = nRxFilterLen;
   
   mtxFilter = txFilter;
   mnTxFilterLen = nTxFilterLen;

   mrxFlows = rxFlows;
   mnRxFlowsLen = nRxFlowsLen;

   mtxFlows = txFlows;
   mnTxFlowsLen = nTxFlowsLen;
}

DSSQoSRequestExScope::DSSQoSSpecScope::~DSSQoSSpecScope()
{
   int i;

   for(i = 0; i < mnRxFilterLen; i++)
   {
      DSSCommon::ReleaseIf((IQI**)&mrxFilter[i]);
   }

   PS_SYSTEM_HEAP_MEM_FREE(mrxFilter);

   for(i = 0; i < mnTxFilterLen; i++)
   {
      DSSCommon::ReleaseIf((IQI**)&mtxFilter[i]);
   }

   PS_SYSTEM_HEAP_MEM_FREE(mtxFilter);

   for(i = 0; i < mnRxFlowsLen; i++)
   {
      DSSCommon::ReleaseIf((IQI**)&mrxFlows[i]);
   }

   PS_SYSTEM_HEAP_MEM_FREE(mrxFlows);

   for(i = 0; i < mnTxFlowsLen; i++)
   {
      DSSCommon::ReleaseIf((IQI**)&mtxFlows[i]);
   }

   PS_SYSTEM_HEAP_MEM_FREE(mtxFlows);

}

void* DSSQoSRequestExScope::DSSQoSSpecScope::operator new[] 
(
 unsigned int size
) throw ()
{
   return ps_system_heap_mem_alloc(size);
}

void DSSQoSRequestExScope::DSSQoSSpecScope::operator delete[] 
(
 void* ptr
)
{
   PS_SYSTEM_HEAP_MEM_FREE(ptr);
}

DSSQoSRequestExScope::DSSQoSRequestExScope
(
 int nQoSSpecScopeLen
)
{
   maQoSSpecScope = new DSSQoSSpecScope[nQoSSpecScopeLen];

   if (NULL != maQoSSpecScope)
   {
      mnQoSSpecScopeLen = nQoSSpecScopeLen;
   }
}

DSSQoSRequestExScope::~DSSQoSRequestExScope()
{
   delete[] maQoSSpecScope;
}

AEEResult DSSQoSRequestExScope::SetNthQoSSpec
(
 int n,
 IIPFilterPriv** rxFilter,
 int nRxFilterLen,
 IIPFilterPriv** txFilter,
 int nTxFilterLen,
 IQoSFlowPriv** rxFlows,
 int nRxFlowsLen,
 IQoSFlowPriv** txFlows,
 int nTxFlowsLen
)
{
   if (n < 0 || n >= mnQoSSpecScopeLen)
   {
      return AEE_EBADPARM;
   }

   maQoSSpecScope[n].Init(rxFilter,
                          nRxFilterLen,
                          txFilter,
                          nTxFilterLen,
                          rxFlows,
                          nRxFlowsLen,
                          txFlows,
                          nTxFlowsLen);

   return AEE_SUCCESS;

}



