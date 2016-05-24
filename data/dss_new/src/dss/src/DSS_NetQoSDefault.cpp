/*======================================================

FILE:  DSS_NetQoSDefault.cpp

SERVICES:

GENERAL DESCRIPTION:
Implementation of DSSNetQoSDefault class

=====================================================

Copyright (c) 2008 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_NetQoSDefault.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

//===================================================================
//   Includes and Public Data Declarations
//===================================================================

//-------------------------------------------------------------------
// Include Files
//-------------------------------------------------------------------
#include "DSS_Common.h"
#include "dserrno.h"

#include "DSS_NetQoSDefault.h"
#include "DSS_Globals.h"
#include "DSS_CritScope.h"

using namespace ds::Net;
//-------------------------------------------------------------------
// Constant / Define Declarations
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Type Declarations (typedef, struct, enum, etc.)
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Global Constant Data Declarations
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Global Data Declarations
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Forward Declarations
//-------------------------------------------------------------------

//===================================================================
//              Macro Definitions
//===================================================================

//===================================================================
//            DSSNetQoSDefault Functions Definitions
//===================================================================

// TODO: documentation
DSSNetQoSDefault::DSSNetQoSDefault(IQoSDefault* pNetQoSDefault):
   mpNetQoSDefault(pNetQoSDefault)
{
   if (NULL != pNetQoSDefault) {
      (void)mpNetQoSDefault->AddRef();
   }
}



//===================================================================
//  FUNCTION:   DSSNetQoSDefault::~DSSNetQoSDefault
//
//  DESCRIPTION:
//  Destructor of the DSSNetQoSDefault class.
//===================================================================
/*lint -e{1551} */
DSSNetQoSDefault::~DSSNetQoSDefault() 
{
   // release the Default QoS Session
   DSSCommon::ReleaseIf((IQI**)&mpNetQoSDefault);
}
/*lint –restore */

//===================================================================


void * DSSNetQoSDefault::operator new
(
   unsigned int numBytes
)  throw()
{
   return ps_mem_get_buf( PS_MEM_DSAL_NET_QOS_DEFAULT_TYPE);
} /* DSSNetQoSDefault::operator new() */


void DSSNetQoSDefault::operator delete
(
   void *  bufPtr
)
{
   PS_MEM_FREE(bufPtr);
   return;
} /* DSSNetQoSDefault::operator delete() */


