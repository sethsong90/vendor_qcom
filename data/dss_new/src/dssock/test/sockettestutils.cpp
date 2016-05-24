/*===========================================================================
  FILE: SocketTestUtils.cpp

  OVERVIEW: This file is a helper wrapping the Socket object and providing
   functions for creating, destroying and writing to the socket.
  
  DEPENDENCIES: None

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/main/latest/
  test/SocketTestUtils.h $
  $DateTime: 2009/09/28 14:45:31 $ $Author: dmudgal $

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "SocketTestUtils.h"

extern "C"
{
#include "dssdns.h"
}

using namespace PS::QTF;

/*===========================================================================

                      STATIC DEFENITIONS

===========================================================================*/

#if 0
PSFlowHelper                * SocketTestUtils::flowHelperArr[];
int                           SocketTestUtils::currentFlowCount = 0;
int                           SocketTestUtils::physLinkIndex;
PSIfaceHelper               * SocketTestUtils::ifaceHelper;
PhysLinkHelper             ** SocketTestUtils::physLinkHelperArr;

ps_iface_tx_cmd_f_ptr_type    SocketTestUtils::ifaceTxCBFnPtr;
ps_iface_cmd_f_ptr_type       SocketTestUtils::bringUpIfaceCmdFnPtr;
ps_iface_cmd_f_ptr_type       SocketTestUtils::tearDownIfaceCmdFnPtr;
ps_iface_ioctl_f_ptr_type     SocketTestUtils::ifaceIoctlFnPtr;
ps_flow_ioctl_f_ptr_type      SocketTestUtils::psFlowIoctlFnPtr;

ps_phys_link_cmd_f_ptr_type   SocketTestUtils::physLinkUpFnPtr;
ps_phys_link_cmd_f_ptr_type   SocketTestUtils::physLinkDownFnPtr;
ps_phys_link_cmd_f_ptr_type   SocketTestUtils::physLinkNullFnPtr;

ps_flow_cmd_f_ptr_type        SocketTestUtils::activateCmdFuncPtr;
ps_flow_cmd_f_ptr_type        SocketTestUtils::configureCmdFuncPtr;
ps_flow_cmd_f_ptr_type        SocketTestUtils::suspendCmdFuncPtr;
ps_flow_cmd_f_ptr_type        SocketTestUtils::goNullCmdFuncPtr;
ps_flow_ioctl_f_ptr_type      SocketTestUtils::ioctlCmdFuncPtr;
#endif
int PS::QTF::SocketTestUtils::currentFlowCount = 0;
PS::QTF::PSIfaceHelper * PS::QTF::SocketTestUtils::ifaceHelper = NULL;
/*===========================================================================

                      PUBLIC MEMBER FUNCTIONS

===========================================================================*/

#if 0
ISocket * SocketTestUtils::CreateSocket
(
  FamilyType      family,
  SocketType      socketType,
  ProtocolType    protocol
)
{
  DS::ErrorType    returnVal;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  returnVal =
    DSCreateInstance(0,
                     AEECLSID_DSNetPolicy,
                     0,
                     reinterpret_cast <void **> (&netPolicyPtr));

 returnVal =
    DSCreateInstance(0,
                     AEECLSID_DSSockSocketFactory,
                     0,
                     reinterpret_cast <void **> (&socketFactoryPtr));

  netPolicyPtr->SetIfaceName(DS::Net::IfaceName::STA);
  netPolicyPtr->SetPolicyFlag(DS::Policy::ANY);
  netPolicyPtr->SetAddressFamily(DSS_AF_INET);

  returnVal = socketFactoryPtr->CreateISocketByPolicy(family,
                                                      socketType, 
                                                      protocol, 
                                                      netPolicyPtr, 
                                                      &sockPtr);
  return sockPtr;
}
#endif

int SocketTestUtils::RegisterCallBack
(
  int      event,
  void ( * callBackFn )( void * arg ),
  void   * userData
)
{ 
  ISignalCBFactory  * signalCBFactoryPtr = NULL;
  DS::ErrorType       dsErrno;
  ISignal           * networkStateChangedSignalPtr = 0;
  ISignalCtl        * networkStateChangedSignalCtlPtr = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  dsErrno =
      DSCreateInstance( 0,
                        AEECLSID_DSUtilsSignalCBFactory,
                        0,
                        reinterpret_cast <void **> ( &signalCBFactoryPtr) );

  if (DS::Error::SUCCESS != dsErrno)
  {

    MSG_ERROR("Error on creating Signal Callback factory", 0, 0, 0);
    return -1;
  }

  dsErrno =
  signalCBFactoryPtr->CreateSignal(callBackFn,
                                  userData,
                                  &networkStateChangedSignalPtr,
                                  &networkStateChangedSignalCtlPtr);    

  if (DS::Error::SUCCESS != dsErrno)
  {

    MSG_ERROR("Error on creating Signal", 0, 0, 0);
    signalCBFactoryPtr->Release();
    return -1;
  }

  dsErrno =  sockPtr->RegEvent(networkStateChangedSignalPtr, event);

  if (DS::Error::SUCCESS != dsErrno)
  {

    MSG_ERROR("Error in registering signal", 0, 0, 0);
    signalCBFactoryPtr->Release();
    networkStateChangedSignalPtr->Release();
    return -1;
  }

  signalCBFactoryPtr->Release();
  return 0;
}

#if 0
void SocketTestUtils::SetIfaceCallBacks
(
  ps_iface_tx_cmd_f_ptr_type    pIfaceTxCBFnPtr,
  ps_iface_cmd_f_ptr_type       pBringUpIfaceCmdFnPtr,
  ps_iface_cmd_f_ptr_type       pTearDownIfaceCmdFnPtr,
  ps_iface_ioctl_f_ptr_type     pIfaceIoctlFnPtr,
  ps_flow_ioctl_f_ptr_type      pPsFlowIoctlFnPtr
)
{
  ifaceTxCBFnPtr =        pIfaceTxCBFnPtr;
  bringUpIfaceCmdFnPtr =  pBringUpIfaceCmdFnPtr;
  tearDownIfaceCmdFnPtr = pTearDownIfaceCmdFnPtr;
  ifaceIoctlFnPtr =       pIfaceIoctlFnPtr;
  psFlowIoctlFnPtr =      pPsFlowIoctlFnPtr;

}

void SocketTestUtils::SetPhysLinkCallBacks
(
  ps_phys_link_cmd_f_ptr_type   pPhysLinkUpFnPtr,
  ps_phys_link_cmd_f_ptr_type   pPhysLinkDownFnPtr,
  ps_phys_link_cmd_f_ptr_type   pPhysLinkNullFnPtr
)
{
  physLinkUpFnPtr =   pPhysLinkUpFnPtr;
  physLinkDownFnPtr = pPhysLinkDownFnPtr;
  physLinkNullFnPtr = pPhysLinkNullFnPtr;

}

void SocketTestUtils::SetFlowCallBacks
(
  ps_flow_cmd_f_ptr_type        pActivateCmdFuncPtr,
  ps_flow_cmd_f_ptr_type        pConfigureCmdFuncPtr,
  ps_flow_cmd_f_ptr_type        pSuspendCmdFuncPtr,
  ps_flow_cmd_f_ptr_type        pGoNullCmdFuncPtr,
  ps_flow_ioctl_f_ptr_type      pIoctlCmdFuncPtr  
)
{
  activateCmdFuncPtr =  pActivateCmdFuncPtr;
  configureCmdFuncPtr = pConfigureCmdFuncPtr;
  suspendCmdFuncPtr =   pSuspendCmdFuncPtr;
  goNullCmdFuncPtr =    pGoNullCmdFuncPtr;
  ioctlCmdFuncPtr =     pIoctlCmdFuncPtr;
}
#endif

int SocketTestUtils::Init
(
  FamilyType                    family,
  SocketType                    socketType,
  ProtocolType                  protocol,
  int                           numPhysLinks,
  int                           numSecFlows
)
{
  int                index;
  IDSNetQoSManager * qosManager;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  do
  {

    /*--------------------------------------------------------------
    1. Create Iface
    --------------------------------------------------------------*/
    ifaceHelper = CreateIfaceAndDefaultFlow
                  (
                  numPhysLinks
                  );

    if( ifaceHelper == NULL )
    {
      break;
    }

    /*--------------------------------------------------------------
    2. Start PosixTask
    --------------------------------------------------------------*/
    posixTask = StartPosixTask();

    if( posixTask == NULL )
    {
      break;
    }
    
    /*--------------------------------------------------------------
    3. SetPhysLinkCallBacks
    --------------------------------------------------------------*/
    SetPhysLinkCallBacks( ifaceHelper );

    /*--------------------------------------------------------------
    4. Create Net Policy
    --------------------------------------------------------------*/
    netPolicyPtr =  CreateNetPolicy();
    
    if( netPolicyPtr == NULL )
    {
      break;
    }

    /*--------------------------------------------------------------
    5. Create Network
    --------------------------------------------------------------*/
    network = CreateNetwork( netPolicyPtr );

    if( network == NULL )
    {
      break;
    }
    
    /*--------------------------------------------------------------
    6. Bringup physLinks
    --------------------------------------------------------------*/
    BringUpPhysLinks( ifaceHelper );
    /*--------------------------------------------------------------
    7. Create Qos Manager
    --------------------------------------------------------------*/
    if ( numSecFlows > 0 )
    {
      qosManager = CreateQosManager( network );
  
      if( qosManager == NULL )
      {
        break;
      }
      
      /*--------------------------------------------------------------
      To skip default flows, index starts at 1. There is no SecQos 
      object corresponding to default flow. So index for SecQoS starts 
      at 0.
      --------------------------------------------------------------*/
      for ( index = 1; index <= numSecFlows; index++ )
      {
        /*--------------------------------------------------------------
        8. Set flow creation state to iface. 
        --------------------------------------------------------------*/
        if ( index < numPhysLinks )
        {
          ConfigureSecFlow( ifaceHelper, index );
        }

        /*--------------------------------------------------------------
        9. Create Sec Qos
        --------------------------------------------------------------*/
        secQosArr [ index - 1 ] = CreateFlow( qosManager );
  
        /*--------------------------------------------------------------
        10. Activate Flows
        --------------------------------------------------------------*/
        ActivateSecFlows( ifaceHelper, index );
      }
      
      currentFlowCount = numSecFlows;
      
      qosManager->Release();
    }
   
    /*--------------------------------------------------------------
    11. Create Socket
    --------------------------------------------------------------*/
    sockPtr = CreateSocket
              (
              family,
              socketType,
              protocol,
              network
              );
    /*--------------------------------------------------------------
    12. Wait for events. Depending upon physlinks, flows and ifaces 
    command callbacks, wait 
    --------------------------------------------------------------*/
    WaitForEvents( ifaceHelper, numPhysLinks, numSecFlows );

    /*--------------------------------------------------------------
    13. InitRouteMetaInfo (Write once)
    --------------------------------------------------------------*/
    WriteData( sockPtr );

    return 0;
  } while( 0 );
/*--------------------------------------------------------------
14. CleanUp in case of errors
--------------------------------------------------------------*/
  CleanUp();
  return -1;
  
}

void SocketTestUtils::WaitForEvents
( 
  PSIfaceHelper * ifaceHelper,
  int             numPhysLinks,
  int             numSecFlows
)
{
  int index;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  for ( index = 0; index < numPhysLinks; index++ )
  {
    PosixTask::WaitForCallBack( PS::QTF::Event::PHYS_LINK_UP );
  }

  /*--------------------------------------------------------------
  Wait for number of Sec flows + 1 to account for the default 
  flow as well.
  --------------------------------------------------------------*/

  for ( index = 0; index < numSecFlows + 1; index++ )
  {
    PosixTask::WaitForCallBack( PS::QTF::Event::FLOW_ACTIVATE );
  }

  PosixTask::WaitForCallBack( PS::QTF::Event::IFACE_UP );
}

PSIfaceHelper * SocketTestUtils::CreateIfaceAndDefaultFlow
(
  int                           numPhysLinks,
  ps_iface_tx_cmd_f_ptr_type    pIfaceTxCBFnPtr,
  ps_iface_cmd_f_ptr_type       pBringUpIfaceCmdFnPtr,
  ps_iface_cmd_f_ptr_type       pTearDownIfaceCmdFnPtr,
  ps_iface_ioctl_f_ptr_type     pIfaceIoctlFnPtr,
  ps_flow_ioctl_f_ptr_type      pPsFlowIoctlFnPtr
)
{
  PSIfaceHelper * ifaceHelper = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ifaceHelper = PSIfaceHelper::CreateIfaceInstance
                (
                numPhysLinks,
                NULL,
                NULL,
                pIfaceTxCBFnPtr,
                pBringUpIfaceCmdFnPtr,
                pTearDownIfaceCmdFnPtr,
                pIfaceIoctlFnPtr
                );
  
  if( ifaceHelper == NULL )
  {
    MSG_ERROR("Error creating Iface", 0, 0, 0);
    return NULL;
  }

  ifaceHelper->GetPSFlowHelperPtr( 0 ) ->SetCallBacks(
    NULL,
    NULL,
    NULL,
    NULL,
    pPsFlowIoctlFnPtr );

  return ifaceHelper;
}

PosixTask * SocketTestUtils::StartPosixTask
(
  void
)
{
    DS::ErrorType       psErrno;
    PosixTask         * posixTask;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

    TF_MSG("Creating posix task");
    posixTask = new PosixTask();

    if( (psErrno = posixTask->Init() ) != 0 )
    {
      MSG_ERROR("Error initializing Posix Threads : %d", psErrno, 0, 0);
      return NULL;
    } 
    return posixTask;
}

IDSNetPolicy * SocketTestUtils::CreateNetPolicy
(
  void
)
{
  DS::ErrorType       psErrno;
  IDSNetPolicy      * netPolicyPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  TF_MSG("Creating Net Policy");

  psErrno =
    DSCreateInstance(0,
                     AEECLSID_DSNetPolicy,
                     0,
                     reinterpret_cast <void **> (&netPolicyPtr));

  if ( psErrno != 0 )
  {
    MSG_ERROR("Error in creating DSNet Policy : %d", psErrno, 0, 0);
    return NULL;
  }

  netPolicyPtr->SetIfaceName(DS::Net::IfaceName::STA);
  netPolicyPtr->SetPolicyFlag(DS::Policy::ANY);
  netPolicyPtr->SetAddressFamily(DSS_AF_INET);

  return netPolicyPtr;
}

void SocketTestUtils::SetPhysLinkCallBacks
( 
  PSIfaceHelper * ifaceHelper
)
{
}

#if 0
IDSNetworkFactory * SocketTestUtils::CreateNetworkFactory
(
  void
)
{
  DS::ErrorType       psErrno;
  IDSNetworkFactory * netFactory;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  TF_MSG("Creating network Factory");
  psErrno =
    DSCreateInstance(0,
                     AEECLSID_DSNetNetworkFactory,
                     0,
                     reinterpret_cast <void **> (&netFactory));

  if ( psErrno != 0 )
  {
    MSG_ERROR("Error in creating Network Factory : %d", psErrno, 0, 0);
    return NULL;
  }

  return netFactory;
}
#endif

void SocketTestUtils::BringUpPhysLinks
( 
  PSIfaceHelper * ifaceHelper 
)
{
  int              index;
  PhysLinkHelper * physLinkHelper;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  /*--------------------------------------------------------------
  Start after the default physLink
  --------------------------------------------------------------*/
  index = 1;

  while( 
         ( physLinkHelper = ifaceHelper->GetPhysLinkHelperPtr( index++ ) ) 
         != NULL 
       )
  {
    PosixTask::PostMessage(
      PS::QTF::Event::PHYS_LINK_UP,
      physLinkHelper->GetPhysLinkPtr()
      );
  }

}

IDSNetwork * SocketTestUtils::CreateNetwork
(
  IDSNetPolicy      * netPolicyPtr
)
{
  DS::ErrorType       psErrno;
  IDSNetwork        * network;
  IDSNetworkFactory * netFactory;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  TF_MSG("Creating network Factory");
  psErrno =
    DSCreateInstance(0,
                     AEECLSID_DSNetNetworkFactory,
                     0,
                     reinterpret_cast <void **> (&netFactory));

  if ( psErrno != 0 )
  {
    MSG_ERROR("Error in creating Network Factory : %d", psErrno, 0, 0);
    return NULL;
  }

  TF_MSG( "Creating Network Object" );
  psErrno = 
    netFactory->CreateIDSNetwork( DS::NetInterface::Mode::ACTIVE,
                                  netPolicyPtr,
                                  &network );

  /*-------------------------------------------------------------------------
    Not Checking returnVal here as EWOULDBLOCK is legal. Also, 
    CreateIDSNetwork sets network to NULL if it is unable to create it.
  -------------------------------------------------------------------------*/

  if ( network == 0 )
  {
    MSG_ERROR( "Error in creating Network Factory : %d", psErrno ,0,0 );
    return NULL;
  }
  
  netFactory->Release();

  /*--------------------------------------------------------------
  Wait for Iface to be brought up
  --------------------------------------------------------------*/
  
  //PosixTask::WaitForCallBack( PS::QTF::Event::IFACE_UP );
  return network;
}

IDSNetQoSManager * SocketTestUtils::CreateQosManager
(
  IDSNetwork * network
)
{
  DS::ErrorType       psErrno;
  IDSNetQoSManager  * qosManager;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  TF_MSG( "Creating QOS Manager" );
  psErrno = network->CreateIDSNetQoSManager( &qosManager );

  if ( psErrno != 0 )
  {
    MSG_ERROR( "Error in creating QOS Manager : %d", psErrno ,0 ,0 );
    return NULL;
  }
  
  return qosManager;
}

void SocketTestUtils::ConfigureSecFlow
(
  PSIfaceHelper * psIface,
  int             physLinkIndex
)
{
  psIface->SetFlowCreationState(
    psIface->GetPhysLinkHelperPtr( physLinkIndex )
    );
  
  return;
}

void SocketTestUtils::ActivateSecFlows
(
  PSIfaceHelper * psIface,
  int             flowIndex
)
{
    psIface->GetPSFlowHelperPtr( flowIndex )->Activate();
}


ISocket * SocketTestUtils::CreateSocket
(
  FamilyType                    family,
  SocketType                    socketType,
  ProtocolType                  protocol,
  IDSNetwork                  * network
)
{
  DS::ErrorType             psErrno;
  ISocket                 * sockPtr;
  DS::Sock::SocketFactory * socketFactoryPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
    
  TF_MSG( "Creating Socket Factory" );

  psErrno =
     DSCreateInstance( 0,
                       AEECLSID_DSSockSocketFactory,
                       0,
                       reinterpret_cast <void **> ( &socketFactoryPtr ) );

  if ( psErrno != 0 )
  {
    MSG_ERROR( "Error in creating Socket Factory : %d", psErrno, 0, 0 );
    return NULL;
  }

 
  psErrno = 
        socketFactoryPtr->CreateISocketByNetwork( family,
                                                socketType,
                                                protocol,
                                                network,
                                                &sockPtr );
    
  if ( psErrno != 0 )
  {
    MSG_ERROR( "Error in creating Socket : %d", psErrno ,0 ,0 );
    return NULL;
  }

  socketFactoryPtr->Release();

  return sockPtr;
}

void SocketTestUtils::CleanUp
(
  void
)
{
  int idx = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if(  posixTask != NULL )
  {
    PosixTask::PostMessage( PS::QTF::Event::END_POSIX_TASK, NULL );
    posixTask->WaitForThreadCompletion();
  }

  qtf_tmc_teardown();


  delete( ifaceHelper ); //How to delete the iface
  
  for ( ; idx < currentFlowCount; idx ++ )
  {
    secQosArr [ idx ] -> Release();
  }

  if ( netPolicyPtr != NULL )
  {
    netPolicyPtr->Release();
  }

#if 0
  if ( netFactory != NULL )
  {
    netFactory->Release();
  }
#endif

  if ( network != NULL )
  {
    network->Release();
  }

  if ( sockPtr != NULL )
  {
    sockPtr->Release();
  }

#if 0
  if ( socketFactoryPtr != NULL )
  {
    socketFactoryPtr->Release();
  }
#endif

  if ( posixTask != NULL )
  {
    delete(posixTask);
  }

}

SocketTestUtils::~SocketTestUtils
(
  void
)
{
  CleanUp();
}
DS::ErrorType SocketTestUtils::WriteData
(
  int       length
)
{
  return WriteData( sockPtr, length );
}

DS::ErrorType SocketTestUtils::WriteData
(
  ISocket * sockPtr,
  int       length
)
{
    int                 idx = 1;
    int                 numWritten;
    SockAddrStorageType remoteAddress;
    //To we need to check heap overflow here?
    byte              * testData = NULL;
    DS::ErrorType       err;
    
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

    testData = new byte [ length ];

    for ( ; idx < length ; idx++ )
    {
      testData [ idx ] = byte( idx % 255 );
    }

    TF_MSG("Writting dummy data");

    err = sockPtr->SendTo
                        (
                          testData,
                          length,
                          GetDummyAddress( &remoteAddress) ,
                          0,
                          &numWritten
                         );

    delete [] testData;
    return err;
}

SockAddrStorageType* SocketTestUtils::GetDummyAddress
(
 SockAddrStorageType * remoteAddr
)
{
  unsigned int    addr;
  SockAddrINType* remoteAddrPtr = reinterpret_cast <SockAddrINType*> ( remoteAddr);
  int16           dsErrno;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  dss_inet_pton( "10.45.155.231",
                 DSS_AF_INET,
                 &addr,
                 sizeof(struct ps_in_addr),
                 &dsErrno );

  //addr = htonl( 10 << 24 | 45 << 16 | 155 << 8 | 231 ); 
  memset( remoteAddrPtr, 0, sizeof( SockAddrINType ));
  remoteAddrPtr->family = DS::DSS_AF_INET;
  remoteAddrPtr->port = htons( 80 );
  remoteAddrPtr->addr = (unsigned int) htonl( addr );

  return remoteAddr;
}

#if 0
PhysLinkHelper * SocketTestUtils::GetPhysLinkHelper
(
  int index
)
{
  return physLinkHelperArr[index]; 
}
#endif
PSIfaceHelper * SocketTestUtils::GetPSIfaceHelper
(
  void
)
{
  return ifaceHelper;
}

#if 0
PSFlowHelper * SocketTestUtils::GetFlowHelper
(
  int index
)
{
  if( index == -1 )
  {
    return flowHelperArr[ currentFlowCount - 1 ];
  }
  else if( index < currentFlowCount )
  {
    return flowHelperArr[ index ];
  }
  else
  {
    MSG_ERROR( "Invalid flow index %d", index, 0, 0 );  
    return NULL;
  }
}
#endif


IDSNetQoSSecondary * SocketTestUtils::GetSecQos
(
  int index
)
{
  if( index == -1 )
  {
    return secQosArr [ currentFlowCount - 1 ];
  }
  else if( index < currentFlowCount )
  {
    return secQosArr[ index ];
  }
  else
  {
    MSG_ERROR( "Invalid flow index %d", index, 0, 0 );  
    return NULL;
  }
}

#if 0
void SocketTestUtils::SetFlowHelper
(
  PSFlowHelper * pPSFlowHelper,
  int            index
)
{
  if( index == -1 )
  {
    flowHelperArr[ currentFlowCount++ ] = pPSFlowHelper;
  }
  else if ( index < currentFlowCount )
  {
    flowHelperArr[ index ] = pPSFlowHelper;
  }
  else
  {
    MSG_ERROR( "Invalid flow index %d" , index, 0, 0);  
  }
}

ps_iface_type * SocketTestUtils::GetPSIfacePtr
(
  void
)
{
  return ifaceHelper->GetPSIfacePtr();
}

SocketFactory * SocketTestUtils::GetSocketFactoryPtr
(
  void
)
{
  return socketFactoryPtr;
}
#endif

Socket * SocketTestUtils::GetSocketPtr
(
  void
)
{
  return dynamic_cast<Socket*>(sockPtr);
}

#if 0
ps_phys_link_type * SocketTestUtils::GetPhysLinkPtr
(
  int idx
)
{
  return physLinkHelperArr[idx]->GetPhysLinkPtr(); 
}
#endif

int SocketTestUtils::SetSystemSocket
(
  void
)
{
  AEEResult               res = DS::Error::SUCCESS;
  DS::Sock::ISocketPriv * sockPrivate;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  res = sockPtr->QueryInterface(
                                DS::Sock::AEEIID_ISocketPriv, 
                                (void**)&sockPrivate);

  if ( DS::Error::SUCCESS != res ) 
  {
    MSG_ERROR("QueryInterface(DS::Sock::AEEIID_ISocketPriv) failed: %d", res, 0, 0);
    return -1;
  }

  sockPrivate->SetSystemOption(true);
  return 0;
}

#if 0
int SocketTestUtils::PSIfaceIoctlCallBack
(
  ps_iface_type        * ps_iface_ptr,
  ps_iface_ioctl_type    ioctl_name,
  void                 * argval,
  int16                * psErrno
)
{

  switch ( ioctl_name )
  {

    case PS_IFACE_IOCTL_QOS_REQUEST:
       SetFlowHelper(PSFlowHelper::CreateInstance(
                    ifaceHelper,
                    physLinkHelperArr [ physLinkIndex ],
                    ( ps_iface_ioctl_qos_request_type  * ) argval ,
                    activateCmdFuncPtr,
                    configureCmdFuncPtr,
                    suspendCmdFuncPtr,
                    goNullCmdFuncPtr,
                    ioctlCmdFuncPtr ) 
                    );

      break;

    default:
      MSG_ERROR("Operation not supported", 0, 0, 0);

      break;
  }

  return 0;
}
#endif

IDSNetQoSFlow ** SocketTestUtils::CreateTestQosFlowSpecs
(
  int flowCount
)
{
  IDSNetQoSFlow ** txFlowPtrPtr;
  int              returnVal = 0;
  int              idx = 0;
  int              cleanIdx = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  txFlowPtrPtr = new IDSNetQoSFlow* [ flowCount ];

  for( idx = 0 ; idx < flowCount; idx++ )
  {
    returnVal =
      DSCreateInstance(0,
                       AEECLSID_DSNetQoSFlowSpec,
                       0,
                       reinterpret_cast <void **> (&(txFlowPtrPtr [ idx ] )));

    if ( returnVal != 0 )
    {
      MSG_ERROR( "Error in creating Flow Spec : %d" , returnVal ,0,0);

      for( cleanIdx = 0; cleanIdx < idx; cleanIdx++ )
      {
        delete( txFlowPtrPtr [ cleanIdx ] );
      }

      delete( txFlowPtrPtr );
      txFlowPtrPtr = 0;
      break;
    }
    txFlowPtrPtr [ idx ]->SetLatency( 1000 );

  }

  return txFlowPtrPtr;
}

IDSNetIPFilter ** SocketTestUtils::CreateTestFilterSpecs
(
  int filterCount
)
{

  IDSNetIPFilter               ** filterPtrPtr;
  int                             returnVal = 0;
  int                             idx = 0;
  int                             cleanIdx = 0;
  DS::NetIPFilter::IPv4AddrType * ipv4Addr;
  int16                           dssErrno;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  filterPtrPtr = new IDSNetIPFilter* [ filterCount ];

  for( idx = 0 ; idx < filterCount; idx++ )
  {
    returnVal =
      DSCreateInstance( 0,
                       AEECLSID_DSNetIPFilterSpec,
                       0,
                       reinterpret_cast <void **> (&( filterPtrPtr [ idx ] )) );

    if ( returnVal != 0 )
    {
      MSG_ERROR( "Error in creating Filter Spec : %d" , returnVal ,0,0 );

      for( cleanIdx = 0; cleanIdx < idx; cleanIdx++ )
      {
        delete( filterPtrPtr [ cleanIdx ] );
      }

      delete( filterPtrPtr );
      filterPtrPtr = 0;
      break;

    }

    filterPtrPtr [ idx ] -> SetIPVsn(DS::NetIPFilter::IPVersion::IPV4);
    filterPtrPtr [ idx ] -> SetNextHdrProt(DS::NetIPFilter::IPNextProtocol::UDP);

    ipv4Addr = new DS::NetIPFilter::IPv4AddrType();
        
    dss_inet_pton( "10.45.155.231",
                   DSS_AF_INET,
                   ipv4Addr->addr,
                   sizeof(struct ps_in_addr),
                   &dssErrno );

    dss_inet_pton( "255.255.255.255",
                   DSS_AF_INET,
                   ipv4Addr->addr,
                   sizeof(struct ps_in_addr),
                   &dssErrno );
/*
    ipv4Addr->addr[ 0 ] = 231;
    ipv4Addr->addr[ 1 ] = 155;
    ipv4Addr->addr[ 2 ] = 45;
    ipv4Addr->addr[ 3 ] = 10;

    ipv4Addr->subnetMask[ 0 ] = 255;
    ipv4Addr->subnetMask[ 1 ] = 255;
    ipv4Addr->subnetMask[ 2 ] = 255;
    ipv4Addr->subnetMask[ 3 ] = 255;
*/
    filterPtrPtr [ idx ] -> SetDstV4( ipv4Addr );
  }
  return filterPtrPtr;
}

IDSNetQoSSecondary * SocketTestUtils::CreateFlow
(
  IDSNetQoSManager    * dsQosManager,  
  ip_filter_spec_type * ipFilter,
  int                   flowCount,
  int                   filterCount
)
{

  int                      psErrno = 0;
  int                      returnVal = 0;
  int                      idx;
  DS::NetQoS::SpecType   * specType = NULL;
  IDSNetQoSSecondary     * secQos = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  specType = new DS::NetQoS::SpecType();

  do
  {
    /*---------------------------------------------------------------------
    Create the flow spec
    ---------------------------------------------------------------------*/
    specType->txFlowsLen = flowCount;
    specType->txFlows = CreateTestQosFlowSpecs ( flowCount );

    if( specType->txFlows == NULL )
    {
      break;
    }

    /*---------------------------------------------------------------------
    Create the filter spec
    ---------------------------------------------------------------------*/
    specType->txFilterLen = filterCount;
    specType->txFilter = CreateTestFilterSpecs ( filterCount );

    if( specType->txFilter == NULL )
    {
      break;
    }
    
    /*---------------------------------------------------------------------
    Request for secondary flow
    ---------------------------------------------------------------------*/
    if((psErrno = dsQosManager->RequestSecondary( 
      specType, 
      &secQos  
      ) ) != 0
      )

    {
      MSG_ERROR( "Error creating secondray QOS flow %d", 
                psErrno,
                0,
                0 );
      secQos = NULL;
      break;
    }
    return secQos;
  } while( 0 );
  
  /*-----------------------------------------------------------------------
  Clean up in case of errors
  -----------------------------------------------------------------------*/
  if( secQos != NULL )
  {
    delete secQos;
  }

  else if( specType != NULL )
  {
    delete specType;
  }
  
  else 
  {

    if( specType->txFlows != NULL )
    {

      for( idx = 0; idx < flowCount; idx ++ )
      {
        delete ( specType->txFlows [ idx ] );
      }

      delete [] (specType->txFlows);

    }
  
    if( specType->txFilter != NULL )
    {

      for( idx = 0; idx < filterCount; idx ++ )
      {
        delete ( specType->txFilter [ idx ] );
      }

      delete [] ( specType->txFilter );

    }
  }

  return NULL;
}
