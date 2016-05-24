#ifndef __DSS_GENSCOPE_H__
#define __DSS_GENSCOPE_H__

/*===================================================

FILE:  DSS_GenScope.h

SERVICES:
A utility class to automatically release various variables 
when out of scope

=====================================================

Copyright (c) 2008 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_GenScope.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

// You should define instance of this class with pointer to some (initialized) object 
// and type of this object , when you wish memory to be automatically freed on scope exit

#include "DSS_Common.h"

class DSSGenScope {
public:
   typedef enum {
      NONE,
      IDSIQI_TYPE,
      DS_Network_DomainName_ARRAY_PTR,
      IDSNetQoSSecondary_ARRAY_PTR,
      IDSNetMCastSession_ARRAY_PTR,
      IDSNetIPFilter_ARRAY_PTR,
      IDSNetQoSFlow_ARRAY_PTR,
      IDSNetFirewallRule_ARRAY_PTR,
      GEN_SCRATCHPAD_ARRAY,
      Scope_ARRAY
   } ScopeVariableType ;


   DSSGenScope(void* var = NULL, ScopeVariableType vType = NONE, int nLen = 0):mpVar(var), mnLen(nLen), mVarType(vType) {};
   void SetParams(void* var = NULL, ScopeVariableType vType = NONE, int nLen = 0);
   ~DSSGenScope();
   void Release();

private:
   void* mpVar;
   int mnLen;
   ScopeVariableType mVarType;
};

inline void DSSGenScope::SetParams(void* val, ScopeVariableType vType, int nLen)
{
   mpVar = val;
   mVarType = vType;
   mnLen = nLen;
}

#endif // __DSS_GENSCOPE_H__
