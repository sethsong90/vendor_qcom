/*===========================================================================
  FILE: ConfiguringTest.cpp

  OVERVIEW: This file tests the behavior of a Socket receiving a CONFIGURE
  event.

  DEPENDENCIES: None

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/main/latest/test/
  Configuring/ConfiguringTest.cpp#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

  when       who what, where, why
  ---------- --------- ------------------------------------------------------
  2008-05-14 gnamasiv Created module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "CommonUtils.h"
#include "SocketTestUtils.h"
#include "ps_iface_defs.h"
#include "DSErrors.h"

#include "RexUtils.h"
#include "DS_Net_Platform.h"

using namespace PS::QTF;
using namespace DS::Sock;
using namespace DS::Net;

TF_DEFINE_TEST_CASE(COMPONENT, ConfiguringTest);

/*===========================================================================

                      PUBLIC MEMBER FUNCTIONS

===========================================================================*/

static RexUtils         rexUtils;
static SocketTestUtils  sockTestUtils;

static char*            objective = "1. The socket is taken to  "
                                    "IFACE_CONFIGURING state.\n"
                                    "2. If write is"
                                    " attempted on this socket, it would "
                                    "fail with EWOULDBLOCK";

/**
   @brief Do all the required setup to set the socket to the 
          proper statebefore begining of test.
    
   Initialize and start ps_task 
   Create and initialize the Iface, PhysLink and Socket 
    
   @param None 
    
   @retval None 
*/
void ConfiguringTest::Setup
(
  void
)
{
  PS_QTF_VERSION( "$Id: //source/qcom/qct/modem/datacommon/"
                  "dssock/main/latest/test/"
                   "Configuring/ConfiguringTest.cpp#1 $" );

  PS_QTF_CLASSIFICATION( QTF_FUNCTIONAL );
  PS_QTF_OWNER( "SanDiego" );
  PS_QTF_GROUP( "DataCommon" );
  PS_QTF_CDPS_REGRESSION_TEST( TRUE );
  PS_QTF_CDPS_SMOKE_TEST( TRUE );
 
  PS_QTF_OBJECTIVE( objective );

  rexUtils.SpawnPSTask("ConfiguringTest");

  if( sockTestUtils.Init(Family::INET, Type::DGRAM, Protocol::UDP) != 0 )
  {
    TF_MSG("Error in SocketUtils initialization");
    TF_ASSERT(0);
  }
}

/**
   @brief Test the behavior of Socket with Configuring event
    
    a.	The socket is taken to IFACE_CONFIGURING state.
    b.	If write is attempted on this socket, it would    
        fail with EWOULDBLOCK    
 
    @param None
    
    @retval None
*/
void ConfiguringTest::Test
(
  void
)
{
  DS::ErrorType   errno;
  PSIfaceHelper * ifaceHelper = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ifaceHelper = sockTestUtils.GetPSIfaceHelper();
  
  ifaceHelper->PostIfaceMessage( PS::QTF::Event::IFACE_CONFIGURE );
  /*
  PosixTask::PostMessage
    (
    PS::QTF::Event::IFACE_CONFIGURE, 
    sockTestUtils.GetPSIfacePtr()
    );
  */
  PosixTask::WaitForCallBack( PS::QTF::Event::IFACE_CONFIGURE );
  
  errno = sockTestUtils.WriteData();

  if ( errno != DS::Error::DSS_EWOULDBLOCK )
  {

    TF_MSG("Testcase IfaceConfiguringTest failed");
    TF_ASSERT(0);
  }

}

/**
   @brief Cleanup function 
    
   Frees sockets, iface, physLinks, end the posixthread 
   and destroy the semaphore 
    
   @param None 
    
   @retval None 
*/
void ConfiguringTest::Teardown()
{
  sockTestUtils.CleanUp();
}
