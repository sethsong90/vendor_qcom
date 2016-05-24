/*===========================================================================
  FILE: LinuxSocketPlatformEventManager.cpp

  OVERVIEW: This file implements EventManager module for socket platform. This
  module registers events with PSStack and translates the callbacks in to
  C++ interfaces.

  DEPENDENCIES: None

  Copyright (c) 2008,2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header:$
  $DateTime:$

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  05/05/2010 ar  Adapted for Linux platform.
  05/02/2008 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include <pthread.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#include "comdef.h"
#include "customer.h"
#include "amssassert.h"
#include "msg.h"
#ifdef FEATURE_DSS_LINUX
#include "ds_sl_list.h"
#else
#include "list.h"
#endif
#include "ds_cmdq.h"
#include "ds_Utils_DebugMsg.h"
#include "LinuxPlatformSocket.h"
#include "LinuxPlatformSocketEventManager.h"

using namespace ds::Utils;
using namespace PS::Sock;
using namespace PS::Sock::Platform;
using namespace PS::Sock::Platform::EventManager;

typedef struct LinuxSocketEvS
{
  EventManager::LinuxSocketOpEnum operation;
  LinuxSocket         *socket;
} LinuxSocketEventStruct;

typedef struct LinuxSocketMapS
{
  list_link_type  node;
  LinuxSocket    *socket;
} LinuxEventToSocketMapStruct;


/*===========================================================================

                        LOCAL DATA DEFINITIONS

===========================================================================*/
/* Socket object monitoring info */
static struct LinuxEventMonitorInfoS {
  list_type        socketList;
  pthread_t        eventThread;
  pthread_mutex_t  mutx;   /* Mutex for protecting the list operations */
  fd_set           readSet;
  fd_set           writeSet;
  SOCKET           maxfd;
  boolean          kill;
  int              controlpipe[2];
} LinuxEventMonitorInfo;

#define LINUX_NULL_FD     (-1)
#define LINUX_NULL_THREAD (-1)
#define IS_THREAD_ACTIVE() (LinuxEventMonitorInfo.eventThread != LINUX_NULL_THREAD)

/* Command message FIFO and processing thread */
static ds_cmdq_info_t  LinuxMsgQ;
#define LINUX_MSG_Q_MAX            (100)        /* Warning level */

/*===========================================================================

                        PRIVATE FUNCTION DEFINITIONS

===========================================================================*/

#if 0
static SOCKET LinuxSocketFindMaxSocket( void )
{
  LinuxEventToSocketMapStruct  *item = NULL;
  SOCKET  max_sock = LINUX_NULL_FD;
  
  /* Check for empty list */
  if( 0 == list_size( &LinuxEventMonitorInfo.socketList ) )
    return LINUX_NULL_FD;

  /* Loop over socket list to find largest fd value */
  item = (LinuxEventToSocketMapStruct*)
         list_peek_front( &LinuxEventMonitorInfo.socketList );
  while( NULL != item )
  {
    if( max_sock < item->socket->GetLinuxSock() )
      max_sock = item->socket->GetLinuxSock();

    /* Get next item */
    item = (LinuxEventToSocketMapStruct*)
           list_peek_front( &LinuxEventMonitorInfo.socketList );
  }

  return max_sock;
} /*  LinuxSocketFindMaxSocket() */
#endif

static int LinuxSocketCompare
(
  void* item_ptr,
  void* compare_val
)
{
  LinuxEventToSocketMapStruct *item = (LinuxEventToSocketMapStruct*)item_ptr;

  ASSERT( item );
  
  /* Compare socket values, terminate search on match */
  if( item->socket == (LinuxSocket*)compare_val )
    return 1;
  
  return 0;
} /*  LinuxSocketCompare() */


static void LinuxSocketProcessEvent
(
  EventType  event,
  LinuxSocket * platformSockPtr
)
{
  IEventListener *  eventListenerPtr;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if( NULL == platformSockPtr )
  {
    MSG_ERROR( "LinuxSocketProcessEvent: NULL arg", 0, 0, 0 );
    ASSERT(0);
    return;
  }

  eventListenerPtr = platformSockPtr->GetIEventListener();
  if (0 == eventListenerPtr)
  {
    MSG_ERROR( "LinuxSocketProcessEvent: NULL evListener, sock 0x%x",
               platformSockPtr, 0, 0 );
    ASSERT( 0);
    return;
  }
  
  /* Post event to the event listener interface */
  switch( event ) {
    case Event::READ:
      MSG_LOW( "LinuxSocketProcessEvent: posting READ to sock 0x%x",
               platformSockPtr,0,0);
      /* Check for pending simple read event */
      if( SOCK_EVENT_READ & platformSockPtr->GetPendingEvents() ) {
        platformSockPtr->ClearPendingEvent( SOCK_EVENT_READ );
        eventListenerPtr->SetEvent( PS::Sock::Platform::Event::READ );
      }
      /* Check for pending listen event */
      if( SOCK_EVENT_LISTEN & platformSockPtr->GetPendingEvents() ) {
        platformSockPtr->ClearPendingEvent( SOCK_EVENT_LISTEN );
        platformSockPtr->SetState( SOCK_STATE_CONNECTED );
        eventListenerPtr->SetEvent( PS::Sock::Platform::Event::READ );
      }
      /* Check for pending close event */
      if( SOCK_EVENT_CLOSE & platformSockPtr->GetPendingEvents() ) {
        char buff[1];
        int len;
        platformSockPtr->ClearPendingEvent( SOCK_EVENT_CLOSE );

        /* Peek at receive buffer, should be empty when socket closed */
        len = recv( platformSockPtr->GetLinuxSock(), buff, sizeof(buff), MSG_PEEK );
        if( 0 < len ) {
          MSG_ERROR( "Socket not empty on pending close event %d ",
                     platformSockPtr->GetLinuxSock(),0,0 );
          eventListenerPtr->SetEvent( PS::Sock::Platform::Event::READ );
        } else {
	  // DSS expects no event
	  // eventListenerPtr->ProcessEvent( PS::Sock::Platform::Event::CLOSE );
        }
      }
      /* Check for pending accept event */
      if( SOCK_EVENT_ACCEPT & platformSockPtr->GetPendingEvents() ) {
        platformSockPtr->ClearPendingEvent( SOCK_EVENT_ACCEPT );
        platformSockPtr->SetState( SOCK_STATE_CONNECTED );
        eventListenerPtr->SetEvent( PS::Sock::Platform::Event::ACCEPT );
      }
      break;
      
    case Event::WRITE:
      MSG_LOW( "LinuxSocketProcessEvent: posting WRITE to sock 0x%x",
               platformSockPtr,0,0);
      /* Check for pending simple write event */
      if( SOCK_EVENT_WRITE & platformSockPtr->GetPendingEvents() ) {
        platformSockPtr->ClearPendingEvent( SOCK_EVENT_WRITE );
      }
      /* Check for pending connect event */
      if( SOCK_EVENT_CONNECT & platformSockPtr->GetPendingEvents() )
      {
        platformSockPtr->ClearPendingEvent( SOCK_EVENT_CONNECT );
        platformSockPtr->SetState( SOCK_STATE_CONNECTED );
      }
      eventListenerPtr->SetEvent( PS::Sock::Platform::Event::WRITE );
      break;
      
    default:
      MSG_ERROR( "LinuxSocketProcessEvent: unsupport event 0x%x", event, 0, 0);
  }
  
  return;
} /* LinuxSocketProcessEvent() */


static void* LinuxSocketEventThread
(
  void  *data
)
{
  int         cnt = 0;
  SOCKET      maxfd;
  LinuxEventToSocketMapStruct  *item = NULL;
  fd_set      readSet, writeSet;

  MSG_MED("Starting sockets monitoring thread",0,0,0 );
  (void)data;
  /* Infinite loop to monitor socket descriptors for events */
  for (;;)
  {
    /* Copy monitoring set to working set, which gets updated in select */
    pthread_mutex_lock( &LinuxEventMonitorInfo.mutx );
    readSet = LinuxEventMonitorInfo.readSet;
    writeSet = LinuxEventMonitorInfo.writeSet;
    maxfd = LinuxEventMonitorInfo.maxfd;
    pthread_mutex_unlock( &LinuxEventMonitorInfo.mutx );

    /* Indefinite wait for state change on socket descriptors */
    cnt = select( maxfd+1, &readSet, &writeSet, NULL, NULL );

    if( LINUX_ERROR == cnt )
    {
      /* Any other error is a problem */
      MSG_ERROR( "LinuxSocketEventThread: error during select: %s\n",
                 strerror(errno),0,0 );
      break;
    }
    else
    {

      pthread_mutex_lock( &LinuxEventMonitorInfo.mutx ); 

      /* Check for signal on control pipe */
      if( FD_ISSET( LinuxEventMonitorInfo.controlpipe[0],
                    &readSet ) ) {
        char buf[1];
        (void)read( LinuxEventMonitorInfo.controlpipe[0], &buf, sizeof(buf) );

        if( LinuxEventMonitorInfo.kill )
        {
          /* Exit thread */
          MSG_MED("Received command to terminate monitoring thread",0,0,0 );
          LinuxEventMonitorInfo.eventThread = LINUX_NULL_THREAD;
          pthread_mutex_unlock( &LinuxEventMonitorInfo.mutx );
          break;
        }
        else
        {
          /* Monitor fdset updated, continue loop */
          cnt--;
          MSG_LOW( "LinuxSocketEventThread: monitoring set updated", 0,0,0 );
        }
      }

      if( cnt ) {
        /* Loop over registered sockets */
        item = (LinuxEventToSocketMapStruct*)
          list_peek_front( &LinuxEventMonitorInfo.socketList );
        while( item )
        {
          /* Check for read event on socket descriptor */
          if( FD_ISSET( item->socket->GetLinuxSock(),
                        &readSet )) {
            MSG_LOW( "LinuxSocketEventThread: read event on sock 0x%x",
                     item->socket->GetLinuxSock(),0,0);
            FD_CLR( item->socket->GetLinuxSock(), &LinuxEventMonitorInfo.readSet );
            /* Notify socket EventListener */
            LinuxSocketProcessEvent( Event::READ, item->socket );
            cnt--;
          }

          /* Check for write event on socket descriptor */
          if( FD_ISSET( item->socket->GetLinuxSock(),
                        &writeSet )) {
            MSG_LOW( "LinuxSocketEventThread: write event on sock 0x%x",
                     item->socket->GetLinuxSock(),0,0);
            FD_CLR( item->socket->GetLinuxSock(), &LinuxEventMonitorInfo.writeSet );
            /* Notify socket EventListener */
            LinuxSocketProcessEvent( Event::WRITE, item->socket );
            cnt--;
          }

          /* Check for all fds processed */
          if( !cnt )
            break;

          /* Get next socket from list */
          item = (LinuxEventToSocketMapStruct*)
            list_peek_next( &LinuxEventMonitorInfo.socketList,
                            (list_link_type*)item );
        } /* while */

        /* Check for successful socket/event match(s) */
        if( cnt )
        {
          MSG_MED("No socket for select event, ignoring",0,0,0 );
        }
      }

      pthread_mutex_unlock( &LinuxEventMonitorInfo.mutx ); 
    }
  } /* forever loop */
  
  return 0;
} /* LinuxSocketEventThread() */


static void LinuxSocketEventExecute
(
  struct ds_cmd_s *cmd,
  void            *data
)
{
  LinuxSocketEventStruct *event_ptr = (LinuxSocketEventStruct*)data;
  LinuxEventToSocketMapStruct *item = NULL;
  list_link_type *node = NULL;
  
  ASSERT( event_ptr );
  (void) cmd;
  /* Process event info */
  switch( event_ptr->operation )
  {
    case LINUX_SOCKET_REG:
    {
      /* Add new socket to mapping table */
      item = (LinuxEventToSocketMapStruct*)
        ps_system_heap_mem_alloc( sizeof(LinuxEventToSocketMapStruct) );
      if( NULL == item ) {
	MSG_ERROR( "LinuxSocketEventExecute: failed on ps_system_heap_mem_alloc\n",0,0,0 );
	return;
      }
      
      item->socket = event_ptr->socket;

      pthread_mutex_lock( &LinuxEventMonitorInfo.mutx );
      
      list_push_back( &LinuxEventMonitorInfo.socketList, &item->node );
      LinuxEventMonitorInfo.maxfd = MAX( LinuxEventMonitorInfo.maxfd,
                                         event_ptr->socket->GetLinuxSock() );
      
      pthread_mutex_unlock( &LinuxEventMonitorInfo.mutx ); 
      
      MSG_MED( "LinuxSocketEventExecute: Register LinuxSock 0x%x sSocket %d\n",
               event_ptr->socket, event_ptr->socket->GetLinuxSock(), 0 );

    }
    break;
  
    case LINUX_SOCKET_DEREG:
    {
      /* Remove socket from mapping table */
      if( 0 < list_size( &LinuxEventMonitorInfo.socketList ) )
      {
        pthread_mutex_lock( &LinuxEventMonitorInfo.mutx );
  
        node = (list_link_type*)list_linear_search( &LinuxEventMonitorInfo.socketList,
                                                    LinuxSocketCompare,
                                                    (void*)event_ptr->socket );
        if( NULL != node )
        {
          /* Found node with matching socket */
          list_pop_item( &LinuxEventMonitorInfo.socketList, node );
          item = (LinuxEventToSocketMapStruct*)node;  /* Same starting address */
          MSG_MED( "LinuxSocketEventExecute: Deregister sock 0x%x sSocket %d\n",
                   event_ptr->socket, event_ptr->socket->GetLinuxSock(), 0);

          /* Remove from monitoring sets */
          FD_CLR( item->socket->GetLinuxSock(), &LinuxEventMonitorInfo.readSet );
          FD_CLR( item->socket->GetLinuxSock(), &LinuxEventMonitorInfo.writeSet );

          PS_SYSTEM_HEAP_MEM_FREE( item );

          /* Check for empty socket list */     
          if( 0 == list_size( &LinuxEventMonitorInfo.socketList ) )
          {
            /* Stop socket descriptor monitoring thread */
            LinuxEventMonitorInfo.kill = TRUE;
          }

          /* Signal monitoring thread to restart select() with updated
           * socket descriptor sets, or possibly exit */
          char buf[1] = {'1'};
          if( LINUX_ERROR == write( LinuxEventMonitorInfo.controlpipe[1], buf, sizeof(buf) ) ) {
            MSG_ERROR( "LinuxSocketEventExecute: failed on control pipe write\n", 0, 0, 0);
          }
        }
        else
        {
          MSG_ERROR( "LinuxSocketEventExecute: failed to locate socket 0x%x\n",
                     event_ptr->socket, 0, 0);
        }

        pthread_mutex_unlock( &LinuxEventMonitorInfo.mutx );
      
      }
    }
    break;

    default:
      MSG_ERROR( "LinuxSocketEventManager: ERROR invalid operation %d\n",
                 event_ptr->operation, 0, 0 );
      ASSERT(0);
      break;
  } /* end switch */

} /* LinuxSocketEventExecute() */


static void LinuxSocketEventRelease
(
  struct ds_cmd_s *cmd,
  void            *data
)
{
  LinuxSocketEventStruct *event = (LinuxSocketEventStruct*)data;
  (void) cmd;
  /* Release event buffer */
  PS_SYSTEM_HEAP_MEM_FREE( event );
} /* LinuxSocketEventRelease() */


/*===========================================================================

                       PUBLIC FUNCTION DEFINITIONS

===========================================================================*/

int EventManager::LinuxSocketEventManager
(
  LinuxSocketOpEnum     operation,
  LinuxSocket          *pSocket
)
{
  LinuxSocketEventStruct *event_ptr = NULL;
  ds_cmd_t               *msg_ptr = NULL;

  ASSERT( pSocket );

  /* Dynamically allocate event buffer */
  event_ptr = (LinuxSocketEventStruct*)ps_system_heap_mem_alloc( sizeof(LinuxSocketEventStruct) );
  if( 0 == event_ptr )
  {
    MSG_ERROR("LinuxSocketEventManager: failed to allocate event buffer",0,0,0);
    return LINUX_ERROR;
  }
  event_ptr->operation    = operation;
  event_ptr->socket       = pSocket;

  /* Dynamically allocate message buffer */
  msg_ptr = ( ds_cmd_t*)ps_system_heap_mem_alloc( sizeof(ds_cmd_t) );
  if( NULL == msg_ptr )
  {
    MSG_ERROR("LinuxSocketEventManager: failed to allocate message buffer",0,0,0);
    goto bail;
  }
  msg_ptr->execute_f = LinuxSocketEventExecute;
  msg_ptr->free_f    = LinuxSocketEventRelease;
  msg_ptr->data      = (void*)event_ptr;

  /* Post message to queue */
  if( LINUX_ERROR == ds_cmdq_enq( &LinuxMsgQ, msg_ptr ) )
  {
    switch(operation)
    {
      case PS::Sock::Platform::EventManager::LINUX_SOCKET_REG:
        MSG_ERROR("LinuxSocketEventManager: ERROR can't reg socket event.",0,0,0);
        break;

      case PS::Sock::Platform::EventManager::LINUX_SOCKET_DEREG:
        MSG_ERROR("LinuxSocketEventManager: ERROR can't unreg socket event.",0,0,0);
        break;

      default:
        ASSERT(0);
    }
    goto bail;
  }

  return LINUX_SUCCESS;

 bail:
  /* Cleanup dynamic memory */
  if( NULL != event_ptr ) {
    PS_SYSTEM_HEAP_MEM_FREE( event_ptr );
  }
  if( NULL != msg_ptr ) {
    PS_SYSTEM_HEAP_MEM_FREE( msg_ptr );
  }
  return LINUX_ERROR;
} /* LinuxSocketEventManager() */


int EventManager::LinuxSocketEventManager_Monitor
(
  LinuxSocket          *pSocket,
  EventType             event
)
{
  ASSERT( pSocket );

  pthread_mutex_lock( &LinuxEventMonitorInfo.mutx ); 

  /* Add socket FD to the monitoring set for requested event */
  switch( event )
  {
    case Event::READ:
      FD_SET( pSocket->GetLinuxSock(), &LinuxEventMonitorInfo.readSet );
      break;
    case Event::WRITE:
      FD_SET( pSocket->GetLinuxSock(), &LinuxEventMonitorInfo.writeSet );
      break;
    default:
      MSG_ERROR("LinuxSocketEventManager_Register, unsupported event %d", event,0,0);
      pthread_mutex_unlock( &LinuxEventMonitorInfo.mutx ); 
      return LINUX_ERROR;
  }

  /* Check status of event monitoring thread */
  if( IS_THREAD_ACTIVE() ) {
    /* Signal monitoring thread to restart select with updated monitoring set */
    char buf[1] = {'1'};
    if( LINUX_ERROR == write( LinuxEventMonitorInfo.controlpipe[1], buf, sizeof(buf) ) ) {
      MSG_ERROR( "LinuxSocketEventExecute: failed on control pipe write\n", 0, 0, 0);
    }

  } else {
    /* Start event monitoriing thread */
    LinuxEventMonitorInfo.kill = FALSE;
    
    /* Launch socket descriptor monitoring thread */
    if( 0 != pthread_create( &LinuxEventMonitorInfo.eventThread, 
                             NULL,
                             LinuxSocketEventThread,
                             NULL ) )
    {
      MSG_ERROR( "LinuxSocketEventManager_Monitor: Cannot start socket monitoring thread\n",
                 0,0,0 );
      pthread_mutex_unlock( &LinuxEventMonitorInfo.mutx ); 
      return LINUX_ERROR;
    }
  }

  pthread_mutex_unlock( &LinuxEventMonitorInfo.mutx ); 

  return LINUX_SUCCESS;
}


void EventManager::Init
(
  void
)
{

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  MSG_LOW( "Platform::EventManager::Init()", 0,0,0 );

  /*-------------------------------------------------------------------------
    Initialize message queue and listening thread.
  -------------------------------------------------------------------------*/
  if( 0 != ds_cmdq_init( &LinuxMsgQ, LINUX_MSG_Q_MAX ) ) {
    MSG_ERROR( "Event Manager default event read queue create failure\n", 0,0,0 );
    return;
  }
  
  /*-------------------------------------------------------------------------
    Initialize socket descriptor monitoring structure
  -------------------------------------------------------------------------*/
  list_init( &LinuxEventMonitorInfo.socketList );

  FD_ZERO( &LinuxEventMonitorInfo.readSet );
  FD_ZERO( &LinuxEventMonitorInfo.writeSet );
  
  LinuxEventMonitorInfo.eventThread = LINUX_NULL_THREAD;
  LinuxEventMonitorInfo.kill = FALSE;

  if( LINUX_ERROR == pthread_mutex_init(&LinuxEventMonitorInfo.mutx, NULL) ) {
    MSG_ERROR("Event Manager: pthread_mutex_init failed\n",0,0,0);
    return;
  }
  
  /* Create control pipe.  This is used to exit select() when
   * monitoring set changes */
  if( LINUX_ERROR == pipe( LinuxEventMonitorInfo.controlpipe ) ) {
    MSG_ERROR("Event Manager: pipe failed\n",0,0,0);
    return;
  }
  FD_SET( LinuxEventMonitorInfo.controlpipe[0],
          &LinuxEventMonitorInfo.readSet );
  LinuxEventMonitorInfo.maxfd = LinuxEventMonitorInfo.controlpipe[0];

  return;
} /* Platform::EventManager::Init() */
