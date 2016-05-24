#ifndef __DSS_QOSREQUESTEXSCOPE_H__
#define __DSS_QOSREQUESTEXSCOPE_H__

/*===================================================

FILE:  DSS_QoSRequestExScope.h

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

$Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_QoSRequestExScope.h#1 $
$DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

when       who what, where, why
---------- --- ------------------------------------------------------------
2011-02-23 aa  History added.

===========================================================================*/

// You should define instance of this class with pointer to some (initialized) object 
// and type of this object , when you wish memory to be automatically freed on scope exit

#include "DSS_Common.h"

using namespace ds::Net;

class DSSQoSRequestExScope {
public:

   class DSSQoSSpecScope {
   public:
      DSSQoSSpecScope();
      ~DSSQoSSpecScope();

      /* Inits the members of DSSQoSSpecScope */
      void Init(IIPFilterPriv** rxFilter,
                int nRxFilterLen,
                IIPFilterPriv** txFilter,
                int nTxFilterLen,
                IQoSFlowPriv** rxFlows,
                int nRxFlowsLen,
                IQoSFlowPriv** txFlows,
                int nTxFlowsLen);

      /* DSSQoSRequestExScope will use these operators to create DSSQoSSpecScope array */
      void* operator new[](unsigned int size) throw ();
      void operator delete[](void* ptr);


   private:
      IIPFilterPriv** mrxFilter;
      IIPFilterPriv** mtxFilter;
      IQoSFlowPriv** mrxFlows;
      IQoSFlowPriv** mtxFlows;

      int mnRxFilterLen;
      int mnTxFilterLen;
      int mnRxFlowsLen;
      int mnTxFlowsLen;
   };

public:
   DSSQoSRequestExScope(int nQoSSpecScopeLen);
   ~DSSQoSRequestExScope();

   /* Inits the members of the n'th DSSQoSSpecScope in DSSQoSRequestExScope array */
   AEEResult SetNthQoSSpec(
      int n,
      IIPFilterPriv** rxFilter,
      int nRxFilterLen,
      IIPFilterPriv** txFilter,
      int nTxFilterLen,
      IQoSFlowPriv** rxFlows,
      int nRxFlowsLen,
      IQoSFlowPriv** txFlows,
      int nTxFlowsLen);


private:
   DSSQoSSpecScope* maQoSSpecScope;
   int mnQoSSpecScopeLen;

};

#endif // __DSS_QOSREQUESTEXSCOPE_H__
