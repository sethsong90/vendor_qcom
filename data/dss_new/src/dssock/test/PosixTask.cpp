/*===========================================================================
  FILE: PosixTask.cpp

  OVERVIEW: This file is a helper wrapping a Posix Task thread. It contain
  functions to listen to messages from event call backs or test cases and
  call the corresponding indications. It also has helper synchronization
  methods.
  
  DEPENDENCIES: None

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/main/latest/
  test/PosixTask.cpp $
  $DateTime: 2009/09/28 14:45:31 $ $Author: dmudgal $

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "PosixTask.h"

using namespace PS::QTF;

/*===========================================================================

                      STATIC DECLARATIONS

===========================================================================*/
sem_t PosixTask::callBackSemaphore [ PS::QTF::Event::SOCKET_EVENT_MAX ];


/*===========================================================================

                      PUBLIC MEMBER FUNCTIONS

===========================================================================*/
pthread_t * PosixTask::CreatePosixThread
(
  void            * ( * start_routine )(void *),
  void                * arg
)
{
  int threadErr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  threadErr = pthread_create( &threadId, NULL, start_routine, arg );

  if ( threadErr )
  {
    MSG_ERROR( "Error creating thread", 0, 0, 0 );
    return 0;
  }
  return &threadId;
}

int PosixTask::Init
(
  void
)
{
  int idx = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*--------------------------------------------------------------
     Intialize the Semaphores
--------------------------------------------------------------*/

  for( ; idx < PS::QTF::Event::SOCKET_EVENT_MAX ; idx++)
  {
    sem_init( &( callBackSemaphore[ idx ] ), 0, 0 );
  }

  if( CreatePosixThread( &PosixTask::PosixTaskStart, NULL ) == 0 )
  {
    MSG_ERROR( "Error Creating PosixThread", 0, 0, 0 );
    return -1;
  }

  return 0;
}

void * PosixTask::WaitForThreadCompletion
(
  void
)
{
  void * retVal = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  pthread_join(threadId, &retVal);
  return retVal;
}

void * PosixTask::PosixTaskStart
(
  void * arg
)
{
  uint32                      bytesRcvd;
  PosixTaskMsg                msg;
  int                         status = 0;
  int                         idx = 0;
  ps_ip_addr_type             ipaddr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  while ( 1 )
  {

    qtf_tmc_msgr_receive( (uint8*) &msg, sizeof( msg ), &bytesRcvd );
    MSG_HIGH( "Message of type %d received", msg.msgType, 0, 0 );
    
    switch ( msg.msgType )
    {

    case PS::QTF::Event::IFACE_UP:

      ps_iface_up_ind( (ps_iface_type *)( msg.arg ) );
      PosixTask::IfaceUpCallBack( NULL );

      break;
    
    case PS::QTF::Event::IFACE_DOWN:

      ps_iface_down_ind( (ps_iface_type *)( msg.arg ) );
      PosixTask::CallBack( PS::QTF::Event::IFACE_DOWN );

      break;

    case PS::QTF::Event::IFACE_CONFIGURE:
      
      ps_iface_configuring_ind( ( ps_iface_type * )( msg.arg ) );
      PosixTask::IfaceConfigureCallBack( NULL );

      break;

    case PS::QTF::Event::FLOW_ACTIVATE:

      ps_flow_activate_ind( ( ps_flow_type * )( msg.arg ), 
                            PS_EIC_NOT_SPECIFIED );

      PosixTask::FlowActivateCallBack( NULL );

      break;
    
     case PS::QTF::Event::FLOW_SUSPEND:

      ps_flow_suspend_ind( ( ps_flow_type * )( msg.arg ), 
                            PS_EIC_NOT_SPECIFIED );

      PosixTask::CallBack( PS::QTF::Event::FLOW_SUSPEND );
      break;

    case PS::QTF::Event::PHYS_LINK_UP:

      ps_phys_link_up_ind( ( ps_phys_link_type * ) ( msg.arg ) );
      PosixTask::CallBack( PS::QTF::Event::PHYS_LINK_UP );
      break;
    
    case PS::QTF::Event::PHYS_LINK_DOWN:

      ps_phys_link_down_ind( ( ps_phys_link_type * ) ( msg.arg ) );
      PosixTask::CallBack( PS::QTF::Event::PHYS_LINK_DOWN );
      break;
    
    case PS::QTF::Event::PHYS_LINK_GONE:

      ps_phys_link_gone_ind( ( ps_phys_link_type * ) ( msg.arg ) );
      PosixTask::CallBack( PS::QTF::Event::PHYS_LINK_GONE );
      break;

    case PS::QTF::Event::IFACE_ROUTABLE_IND:

      ps_iface_routeable_ind( (ps_iface_type *)( msg.arg ) );
      PosixTask::CallBack( PS::QTF::Event::IFACE_ROUTABLE_IND );
      break;

    case PS::QTF::Event::CHANGE_IP:

      memset( &ipaddr, 0, sizeof(ipaddr) );
      ipaddr.type = IPV4_ADDR;
      ipaddr.addr.v4.ps_s_addr = 10 << 24 | 45 << 16 | 155 << 8 | 232;
      ps_iface_set_v4_addr( (ps_iface_type *)( msg.arg ), &ipaddr );
      PosixTask::CallBack( PS::QTF::Event::CHANGE_IP );
      break;
      
    case PS::QTF::Event::CHANGE_FLTR:

      ps_iface_ipfltr_updated_ind( (ps_iface_type *)( msg.arg ),
                                   IP_FLTR_CLIENT_QOS_OUTPUT,
                                   0,
                                   1 );

      PosixTask::CallBack( PS::QTF::Event::CHANGE_FLTR );

      break;
      
    case PS::QTF::Event::FLOW_DISABLED:

      ps_iface_disable_flow( ( ps_iface_type * )( msg.arg ), DS_FLOW_ALL_MASK );
      PosixTask::CallBack( PS::QTF::Event::FLOW_DISABLED );

      break;
    
    case PS::QTF::Event::FLOW_ENABLED:

      ps_iface_enable_flow( ( ps_iface_type * )( msg.arg ), DS_FLOW_ALL_MASK );
      PosixTask::CallBack( PS::QTF::Event::FLOW_ENABLED );

      break;

    case PS::QTF::Event::FLOW_TX_DISABLED:

      ps_flow_disable_tx( ( ps_flow_type * )( msg.arg ), DS_FLOW_PS_FLOW_MASK );
      PosixTask::CallBack( PS::QTF::Event::FLOW_TX_DISABLED );

      break;

    case PS::QTF::Event::FLOW_TX_ENABLED:

      ps_flow_enable_tx( ( ps_flow_type * )( msg.arg ), DS_FLOW_PS_FLOW_MASK );
      PosixTask::CallBack( PS::QTF::Event::FLOW_TX_ENABLED );

      break;

    case PS::QTF::Event::PHYSLINK_FLOW_ENABLED:

      ps_phys_link_enable_flow( ( ps_phys_link_type * ) ( msg.arg ),
                                  DS_FLOW_PHYS_LINK_MASK);

      PosixTask::CallBack( PS::QTF::Event::PHYSLINK_FLOW_ENABLED );
      break;

    case PS::QTF::Event::PHYSLINK_FLOW_DISABLED:
      
      ps_phys_link_disable_flow( ( ps_phys_link_type * ) ( msg.arg ),
                                  DS_FLOW_PHYS_LINK_MASK);

      PosixTask::CallBack( PS::QTF::Event::PHYSLINK_FLOW_DISABLED );
      break;

    case PS::QTF::Event::END_POSIX_TASK:

      status = PS::QTF::Event::END_POSIX_TASK;
      break;

    default:

      MSG_HIGH("Message type %d did not match any of the "
               "defined message types", msg.msgType,0 ,0);
      status = PS::QTF::Event::END_POSIX_TASK ;
      break;
      
    }

    if( status == PS::QTF::Event::END_POSIX_TASK )
    {
      break;
    }

  } /* while( 1 ) */
  
  MSG_HIGH("Exiting PosixTask", 0, 0, 0);

  for( ; idx < PS::QTF::Event::SOCKET_EVENT_MAX ; idx ++ )
  {
    sem_destroy( &( callBackSemaphore[ idx ] ) );
  }

  return NULL;
}

int PosixTask::PostMessage
(
  int        messageType, 
  void     * userData
)
{

  PosixTaskMsg msg;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/  
  TF_MSG( "Sending Command Message of type %d", messageType, 0, 0 );

  msg.msgType = messageType;
  msg.arg = userData;

  qtf_tmc_msgr_send( &msg.hdr, sizeof ( msg ) );
  
  return 0;
}

void PosixTask::WriteCallBack
(
  void* arg
)
{

  TF_MSG("Calling write callback",0, 0, 0);
  sem_post( &( callBackSemaphore [ PS::QTF::Event::WRITE ] ) );
}

void PosixTask::EventCallBack
(
  ps_iface_type             * this_iface_ptr,
  ps_iface_event_enum_type    event,
  ps_iface_event_info_u_type  event_info,
  void                      * user_data_ptr
)
{

  TF_MSG("Calling write callback",0, 0, 0);
  sem_post( &( callBackSemaphore[ *((int*) user_data_ptr) ]));
}


void PosixTask::IfaceConfigureCallBack
(
  void* arg
)
{

  TF_MSG("Calling Iface Configure callback",0, 0, 0);
  sem_post( &( callBackSemaphore[ PS::QTF::Event::IFACE_CONFIGURE ]) );
}

void PosixTask::FlowActivateCallBack
(
  void* arg
)
{

  TF_MSG("Calling Flow Activate callback", 0, 0, 0);
  sem_post( &( callBackSemaphore [ PS::QTF::Event::FLOW_ACTIVATE ] ) );
}

void PosixTask::IfaceDownCallBack
(
  void* arg
)
{

  TF_MSG("Calling Iface Down callback", 0, 0, 0);
  sem_post( &( callBackSemaphore [ PS::QTF::Event::IFACE_DOWN ] ) );
}

void PosixTask::CallBack
(
  int index
)
{

  TF_MSG("Calling %d callback", index, 0, 0);
  sem_post( &( callBackSemaphore [ index ] ) );
}

void PosixTask::IfaceUpCallBack
(
  void* arg
)
{

  TF_MSG("Calling Iface Up callback",0, 0, 0);
  sem_post( &( callBackSemaphore [ PS::QTF::Event::IFACE_UP ] ) );
}

void PosixTask::WaitForCallBack
(
  PS::QTF::EventType event
)
{

  TF_MSG("Calling Wait for callback on %d", event, 0,0);
  sem_wait( &( callBackSemaphore[event ] ) );
}

