
/*===========================================================================
  FILE: PSFlowSuspendTest.cpp

  OVERVIEW: This file tests the behavior of a Socket receiving a
  FLOW_SUSPEND event.

  DEPENDENCIES: None

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/main/latest/test/
  FlowSuspend/PSFlowSuspendTest.cpp#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

  when       who what, where, why
  ---------- --------- ------------------------------------------------------------
  2008-05-14 gnamasiv Created module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "SocketTestUtils.h"
#include "ps_iface_defs.h"
#include "DSErrors.h"
#undef htonl
#undef htons
#undef ntohl
#undef ntohs

#define FLOWSUSPEND_PHYSLINK_NUMBER 2
#define FLOWSUSPEND_FLOW_NUMBER 1

#include "RexUtils.h"

using namespace PS::QTF;
using namespace DS::Sock;
using namespace DS::Net;

TF_DEFINE_TEST_CASE(COMPONENT, PSFlowSuspendTest);

/*===========================================================================

                      PUBLIC MEMBER FUNCTIONS

===========================================================================*/
static RexUtils rexUtils;
static SocketTestUtils sockTestUtils;
static char* objective = "a.	The Sec Qos object created for the socket is "
                         "issued a suspend flow command.\n" 
                          "b.	Verify that flow shifts to the default flow.\n"
                          "c.	Write succeeds.\n";
/**
   @brief Do all the required setup to set the socket to the 
    			proper statebefore begining of test.
    
     Initialize and start ps_task
     Create and initialize the Iface, PhysLink, Iface and Socket
         
   @param None 
    
   @retval None 
*/ 
void PSFlowSuspendTest::Setup
(
  void
)
{
  int16            ps_errno = 0;
  PSIfaceHelper  * ifaceHelper = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  PS_QTF_VERSION( "$Id: //source/qcom/qct/modem/datacommon/dssock"
                  "/main/latest/test/"
                   "FlowSuspend/PSFlowSuspendTest.cpp#1 $" );

  PS_QTF_CLASSIFICATION( QTF_FUNCTIONAL );
  PS_QTF_OWNER( "SanDiego" );
  PS_QTF_GROUP( "DataCommon" );
  PS_QTF_CDPS_REGRESSION_TEST( TRUE );
  PS_QTF_CDPS_SMOKE_TEST( TRUE );
 
  PS_QTF_OBJECTIVE( objective );

	rexUtils.SpawnPSTask("PSFlowSuspendTest");


  sockTestUtils.Init(
                      Family::INET, 
                      Type::DGRAM, 
                      Protocol::UDP,
                      FLOWSUSPEND_PHYSLINK_NUMBER,
                      FLOWSUSPEND_FLOW_NUMBER
                      );

}

/**
   @brief Verify the behavior of Socket when it receives a Suspend event 
    
   a.	The Sec Qos object created for the socket is issued a
    suspend flow command.
   b.	Verify that flow shifts to the default flow.
   c.	Write succeeds.
    
   @param None 
    
   @retval None 
*/
void PSFlowSuspendTest::Test
(
  void
)
{
  DS::ErrorType        errno;
  PSIfaceHelper      * ifaceHelper = NULL;
  IDSNetQoSSecondary * secQos;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  ifaceHelper =  sockTestUtils.GetPSIfaceHelper();

  /*--------------------------------------------------------------
  To test if the flowforwarding is enabled
  --------------------------------------------------------------*/
  ifaceHelper->setTxFunction(
    PSIfaceHelper::STAPSIfaceTxCmdCbFlowForwardingEnabled );

  secQos = sockTestUtils.GetSecQos();
  secQos->Suspend();
  
  PosixTask::WaitForCallBack( PS::QTF::Event::FLOW_SUSPEND );

	errno = sockTestUtils.WriteData();

	if(errno != DS::Error::SUCCESS)
	{
	  TF_MSG("Testcase PSFlowSuspendTest failed");
		TF_ASSERT(0);
	}

}

/**
   @brief Cleanup the socket and Qtf variables 
    
   Cleanup the socket and Qtf variables  
       
   @param None 
    
   @retval None 
 */
void PSFlowSuspendTest::Teardown
(
  void
)
{
	sockTestUtils.CleanUp();
}


