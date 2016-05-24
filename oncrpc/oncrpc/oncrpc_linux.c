/*===*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
                          O N C R P C _ O S _ L I N U X . C
 *===*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*!
  @file
    oncrpc_os_linux.c

  @brief
    This module contains a Linux specific implementation of generic OS
    functionality.

  @detail
    These functions should be implemented using the native OS functions
    provided by the OS being used.

 Copyright (c) 2008 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.

*/
/*===*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_linux.c#4 $

when       who     what, where, why
--------   ---     -------------------------------------------------------
07/17/08    rr     Add Android support
03/31/08    rr     Change timer create to only create when set.
02/15/08    rr     Add Timer and timer callback support
02/13/08    ih     Fixes return value of oncrpc_event_wait()
02/21/08    rr     Fix bug in oncrpc_event_wait, bug had no effect since we 
                   only use one signal.
10/31/07   ptm     Moved task name APIs to common code.
10/29/07    hn     Added missing argument to oncrpc_thread_exit.
10/16/07   ptm     Merge tls-header-type with tls-type.
10/29/07    hn     Added missing argument to oncrpc_thread_exit.
10/17/07    hn     tls_get and tls_delete now take a thread's key, added
                   tls_get_self and tls_delete_self as the no args version
10/12/07   ptm     Added oncrpc-rpc-is-allowed.
10/08/07   ptm     Clean up oncrpc thread handle references.
08/22/07   ptm     Unified access to thread local storage.
05/08/07   RJS     Initial version Derived from oncrpc_os_r e x.c

===========================================================================*/

/*===========================================================================
                              INCLUDE FILES
 *=========================================================================*/
#include <unistd.h>
#include <pthread.h> 
#include <stdio.h>
#include <sys/types.h>

#include <time.h>
#ifdef  FEATURE_ANDROID
#include "signal.h"
#else
#include <sys/signal.h>
#endif

#include "oncrpc.h"
#include "oncrpci.h"
#include "oncrpc_os.h"

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/
/* Thread Local Storage (tls) Type */
typedef struct {
  oncrpc_tls_type        tls;              /* common fields, must be first */
  /* start of OS specific fields */
  pthread_cond_t         cv;               /* Condition variable       */
  pthread_mutex_t        mutex;            /* Mutex                    */
  uint32                 sigs;             /* Signal mask cf. R E X    */
} oncrpc_tls_linux_type;

/* TLS table size - Size for max num threads */
#define ONCRPC_TLS_TABLE_SIZE 200

static oncrpc_tls_linux_type oncrpc_tls_buf[ONCRPC_TLS_TABLE_SIZE];

/* Mutex to control access to the tls table */
static pthread_mutex_t  oncrpc_tls_mutex = PTHREAD_MUTEX_INITIALIZER;

static oncrpc_tls_table_type oncrpc_tls_table;

/* Key for the thread-specific buffer */
static pthread_key_t oncrpc_tls_key;

/* Once-only initialisation of the key */
static pthread_once_t oncrpc_tls_key_once = PTHREAD_ONCE_INIT;

/* Timer releated action */
static struct sigaction timer_sig_act;

typedef enum
{
  TIMER_ACTIVE,
  TIMER_INACTIVE
}timer_state_type;

typedef struct {
  q_link_type                link;
  timer_state_type           timer_state;
  boolean                    timer_created;
  struct itimerspec          timeout;
  timer_t                    timer_id;
  oncrpc_event_t             event;
  struct sigevent            evp;
  oncrpc_thread_handle       thread_handle;
  oncrpc_timer_cb_func_type  cb_func;
  oncrpc_timer_cb_param_type cb_param;
}linux_timer_type;

static q_type            linux_timer_active_q;
static pthread_mutex_t   linux_timer_q_mutex;       /* Mutex                    */
static pthread_mutex_t   linux_timer_state_mutex;   /* Mutex                    */
/*===========================================================================
                          GLOBAL DATA DECLARATIONS
===========================================================================*/


/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/
static void oncrpc_timer_os_init(void);
void timer_sig_handler(int signo);
boolean oncrpc_timer_new_common
(
  oncrpc_timer_ptr   *ptimer,       /*!< Pointer to a timer structure        */
  oncrpc_event_t      event,         /*!< Event mask to associate with timer  */
  oncrpc_timer_cb_func_type  cb_func,
  oncrpc_timer_cb_param_type cb_param
);

/*===========================================================================
  FUNCTION  oncrpc_linxu_thread_exit
===========================================================================*/
/*!
@brief
  Bridge function from pthread cleanup API to internal thread exit API.

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static void oncrpc_linux_thread_exit( void * dummy_alloc )
{
  free(dummy_alloc);
  oncrpc_thread_exit( (void *) pthread_self() );
} /* oncrpc_linux_thread_exit */

/*===========================================================================
  FUNCTION  oncrpc_tls_key_alloc
===========================================================================*/
/*!
@brief
  Create tls key with associated thread exit function. This registers the
  thread exit funtion so that it is called when the thread exits.

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static void oncrpc_tls_key_alloc( void )
{
  pthread_key_create(&oncrpc_tls_key, oncrpc_linux_thread_exit);
}

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  oncrpc_os_init
===========================================================================*/
/*!
@brief
  Initialises the TLS table structure

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void oncrpc_os_init(void)
{
  oncrpc_tls_table.table       = oncrpc_tls_buf;
  oncrpc_tls_table.entry_size  = sizeof( oncrpc_tls_linux_type );
  oncrpc_tls_table.num_entries = ONCRPC_TLS_TABLE_SIZE;
  oncrpc_tls_table.crit_sec    = (oncrpc_crit_sect_ptr *) &oncrpc_tls_mutex;
  oncrpc_timer_os_init();
} /* oncrpc_os_init */

/*===========================================================================
  FUNCTION  oncrpc_thread_handle_get
===========================================================================*/
/*!
@brief
  Returns the thread handle. It's meaning is OS dependent and should only
  be used as handle in non-OS specific files.

@return
  Thread handle.

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
oncrpc_thread_handle oncrpc_thread_handle_get(void)
{
  return
    (oncrpc_thread_handle *)oncrpc_tls_get_common( (void *) &oncrpc_tls_table,
                                                   (void *) pthread_self() );
} /* oncrpc_thread_handle_get */

/*===========================================================================
  FUNCTION  oncrpc_tls_init
===========================================================================*/
/*!
@brief
  Initialize the OS specific files of a tls struct.

  Also, since there is tls memory associated with this thread, when the
  thread exits we must call oncrpc_thread_exit. So, do what ever is required
  for that to happen for this OS.

@return
  None

@note

  - Dependencies
    - Assumes the call set the entire tls struct to 0.

  - Side Effects
    - None
*/
/*=========================================================================*/
void oncrpc_tls_init(oncrpc_tls_type *ptr)
{
  oncrpc_tls_linux_type *tls = (oncrpc_tls_linux_type *) ptr;

  /* Setup default name */
  /* @todo Where can we get the thread name on Linux? */
  tls->tls.name[0] = '\0';

  /* Initialize OS specific fields */
    pthread_cond_init(&tls->cv, NULL);
    pthread_mutex_init(&tls->mutex, NULL);

  /* Setup thread exit callback */
  pthread_once(&oncrpc_tls_key_once, oncrpc_tls_key_alloc);
  pthread_setspecific(oncrpc_tls_key, malloc(1));
  pthread_getspecific(oncrpc_tls_key);
} /* oncrpc_tls_init */

/*===========================================================================
  FUNCTION  oncrpc_tls_deinit
===========================================================================*/
/*!
@brief
  Delete the tls key. Gets executed as part of unloading the library

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void oncrpc_tls_deinit( void )
{
  pthread_key_delete(oncrpc_tls_key);
}

/*===========================================================================
  FUNCTION  oncrpc_tls_get
===========================================================================*/
/*!
@brief
  Returns a pointer to the common tls storage for the specified thread.

@return
  Pointer to tls storage.

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
oncrpc_tls_type *oncrpc_tls_get( void * key )
{
  oncrpc_tls_linux_type *ptr;

  ptr =
    (oncrpc_tls_linux_type *)oncrpc_tls_get_common( (void *) &oncrpc_tls_table,
                                                    key );

  return &ptr->tls;
} /* oncrpc_tls_get */

/*===========================================================================
  FUNCTION  oncrpc_tls_find
===========================================================================*/
/*!
@brief
  Returns a pointer to the common tls storage for the specified thread.

@return
  Pointer to tls storage.

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
oncrpc_tls_type *oncrpc_tls_find( void * key )
{
  oncrpc_tls_linux_type *ptr;

  ptr =
    (oncrpc_tls_linux_type *)oncrpc_tls_find_common( (void *) &oncrpc_tls_table,
                                                    key );
  if(ptr)
    return &ptr->tls;
  else
    return NULL;
} /* oncrpc_tls_find */

/*===========================================================================
  FUNCTION  oncrpc_tls_get_self
===========================================================================*/
/*!
@brief
  Returns a pointer to the common tls storage for this thread.

@return
  Pointer to tls storage.

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
oncrpc_tls_type *oncrpc_tls_get_self( void )
{
  return oncrpc_tls_get( (void *) pthread_self() );
} /* oncrpc_tls_get_self */

/*===========================================================================
  FUNCTION  oncrpc_tls_delete
===========================================================================*/
/*!
@brief
  Deletes all of the thread local storage allocated to the specified thread.

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void oncrpc_tls_delete( void * key )
{
  oncrpc_tls_delete_common( &oncrpc_tls_table, key );
} /* oncrpc_tls_delete */

/*===========================================================================
  FUNCTION  oncrpc_tls_delete_self
===========================================================================*/
/*!
@brief
  Deletes all of the thread local storage allocated to the current thread.

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void oncrpc_tls_delete_self( void )
{
  oncrpc_tls_delete( (void *) pthread_self() );
} /* oncrpc_tls_delete_self */

/*===========================================================================
FUNCTION ONCRPC_RPC_IS_ALLOWED

DESCRIPTION
  Calls ERR_FATAL if RPC is not allowed in the current context.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void oncrpc_rpc_is_allowed( void )
{
  /* @todo Is there any way to detect this and do we care on Linux? */
  /****************************
  if( rex_is_in_irq_mode() )
  {
    ERR_FATAL( "Interrupt level RPC Call", 0, 0, 0 );
  }
  ********************/
} /* oncrpc_rpc_is_allowed */

/*===========================================================================
  FUNCTION  oncrpc_event_set
===========================================================================*/
/*!
@brief
  Sets the requested signal(s) as defined by the event mask.

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void oncrpc_event_set
(
  oncrpc_thread_handle thread,          /*!< Thread handle                */
  oncrpc_event_t         event            /*!< Event mask of events to set  */
)
{
  oncrpc_tls_linux_type *ptr = (oncrpc_tls_linux_type *) thread;

  ASSERT( NULL != ptr );
  pthread_mutex_lock(&ptr->mutex);
  ptr->sigs |= event;
  pthread_cond_broadcast(&ptr->cv);
  pthread_mutex_unlock(&ptr->mutex);
} /* oncrpc_event_set */

/*===========================================================================
  FUNCTION  oncrpc_event_clr
===========================================================================*/
/*!
@brief
  Clears the requested signal(s) as defined by the event mask.

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void oncrpc_event_clr
(
  oncrpc_thread_handle thread,          /*!< Thread handle                 */
  oncrpc_event_t         event            /*!< Event mask of events to clear */
)
{
  oncrpc_tls_linux_type * ptr = (oncrpc_tls_linux_type *) thread;

  ASSERT( NULL != ptr );
  pthread_mutex_lock(&ptr->mutex);
  ptr->sigs &= ~event;
  pthread_mutex_unlock(&ptr->mutex);
} /* oncrpc_event_clr */


/*===========================================================================
  FUNCTION  oncrpc_event_get
===========================================================================*/
/*!
@brief
  Gets the set of events that matches the event mask.

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
oncrpc_event_t oncrpc_event_get
(
  oncrpc_thread_handle   thread,        /*!< Thread handle                 */
  oncrpc_event_t         event_mask     /*!< Event mask of events to return*/
)
{
  oncrpc_tls_linux_type * ptr = (oncrpc_tls_linux_type *) thread;

  oncrpc_event_t         event;        /*!< Events to return */
  ASSERT( NULL != ptr );
  pthread_mutex_lock(&ptr->mutex);
  event = ptr->sigs & event_mask;
  pthread_mutex_unlock(&ptr->mutex);
  return event;
} /* oncrpc_event_get */


/*===========================================================================
  FUNCTION  oncrpc_event_wait
===========================================================================*/
/*!
@brief
  Wait on the requested signal(s) as defined by the event mask.

@return
  Signals set

@note

  - Dependencies
    - None

  - Side Effects
    - The executing thread blocks waiting for any of the signals in the
      mask to be set.
*/
/*=========================================================================*/
oncrpc_event_t oncrpc_event_wait
(
  oncrpc_thread_handle thread,        /*!< Thread handle                 */
  oncrpc_event_t         event          /*!< Event mask of events to wait on  */
)
{
  oncrpc_tls_linux_type * ptr = (oncrpc_tls_linux_type *) thread;

  ASSERT( NULL != ptr );
  pthread_mutex_lock(&ptr->mutex);

  while( (ptr->sigs & event) == 0)
  {
    pthread_cond_wait(&ptr->cv, &ptr->mutex);
  }

  pthread_mutex_unlock(&ptr->mutex);

  return ptr->sigs;
} /* oncrpc_event_wait */

/*===========================================================================
  FUNCTION  oncrpc_task_pri_set
===========================================================================*/
/*!
@brief
  Sets the task's priority

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void oncrpc_task_pri_set
(
  oncrpc_thread_handle thread,         /*!< Thread handle                 */
  oncrpc_task_pri_t      priority        /*!< new thread priority           */
)
{
  /* @todo Implement task priority set for linux */
} /* oncrpc_task_pri_set */

/*===========================================================================
  FUNCTION  oncrpc_crit_sect_init
===========================================================================*/
/*!
@brief
  Allocates and initialises a critical section structure.

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - This routine "mallocs" the memory for a critical section structure.
*/
/*=========================================================================*/
void oncrpc_crit_sect_init
(
  oncrpc_crit_sect_ptr* pcs          /*!< Pointer to a critical section  */
)
{
  /* Allocate memory for the critical section */
  *pcs = (oncrpc_crit_sect_ptr) oncrpc_mem_alloc(sizeof(pthread_mutex_t));

  /* Initialise structure */
  if(*pcs != NULL)
  {
    pthread_mutex_init(*pcs, NULL);
  }
  else
  {
    ERR_FATAL( "Failed to allocate critical section memory", 0, 0, 0 );
  }
} /* oncrpc_crit_sect_init */

/*===========================================================================
  FUNCTION  oncrpc_crit_sect_enter
===========================================================================*/
/*!
@brief
  Enter a critical section.

@return
  None

@note

  - Dependencies
    - Critical section must first be initialised using oncrpc_crit_sect_init

  - Side Effects
    - None.
*/
/*=========================================================================*/
void oncrpc_crit_sect_enter
(
  oncrpc_crit_sect_ptr  pcs          /*!< Pointer to a critical section  */
)
{
  pthread_mutex_lock(pcs);
} /* oncrpc_crit_sect_enter */

/*===========================================================================
  FUNCTION  oncrpc_crit_sect_leave
===========================================================================*/
/*!
@brief
  Leave a critical section.

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void oncrpc_crit_sect_leave
(
  oncrpc_crit_sect_ptr  pcs          /*!< Pointer to a critical section  */
)
{
  pthread_mutex_unlock(pcs);
} /* oncrpc_crit_sect_leave */

/*===========================================================================
  FUNCTION  oncrpc_timer_os_init
===========================================================================*/
/*!
@brief
  Initialize the necessary infrastructure to support timers on linux.

@return

@note

  - Dependencies
    - None

  - Side Effects
*/
/*=========================================================================*/
static void oncrpc_timer_os_init()
{
  timer_sig_act.sa_handler = (void *)timer_sig_handler;
  timer_sig_act.sa_flags = 0;
  sigemptyset(&timer_sig_act.sa_mask);
  pthread_mutex_init(&linux_timer_q_mutex,NULL);
  pthread_mutex_init(&linux_timer_state_mutex,NULL);
  q_init ( &linux_timer_active_q );
}

/*===========================================================================
  FUNCTION  oncrpc_timer_new
===========================================================================*/
/*!
@brief
  Create a new timer using oncrpc timer interface and associates it 
  with the supplied event.

@return 
   TRUE  - Timer successfully created and initialised
   FALSE - Error creating timer

@note

  - Dependencies
    - None

  - Side Effects
*/
/*=========================================================================*/
boolean oncrpc_timer_new
(
  oncrpc_timer_ptr   *ptimer,       /*!< Pointer to a timer structure        */
  oncrpc_event_t     event         /*!< Event mask to associate with timer  */
)
{
  return oncrpc_timer_new_common
   (
     ptimer,       /*!< Pointer to a timer structure        */
     event,         /*!< Event mask to associate with timer  */
     (oncrpc_timer_cb_func_type) NULL,
     (oncrpc_timer_cb_param_type) NULL
  );
}

/*===========================================================================
  FUNCTION  oncrpc_timer_new_cb
===========================================================================*/
/*!
@brief
  Creates a new OS timer and specify a callback function for timeout 
  notification.

@return
  TRUE  - Timer successfully created and initialised
  FALSE - Error creating timer

@note

  - Dependencies
    - None

  - Side Effects
    - This routine "mallocs" the memory for a timer structure.
*/
/*=========================================================================*/
boolean oncrpc_timer_new_cb
(
  oncrpc_timer_ptr           *ptimer,   /*!< Pointer to a timer structure   */  
  oncrpc_timer_cb_func_type  cb_func,   /*!< Pointer to a callback function */  
  oncrpc_timer_cb_param_type cb_param   /*!< Callback data                  */  
)
{
  return oncrpc_timer_new_common
   (
     ptimer,   
     (oncrpc_event_t)NULL,
     cb_func,
     cb_param     
  );
} /* oncrpc_timer_new_cb */



/*===========================================================================
  FUNCTION  oncrpc_timer_new_common
===========================================================================*/
/*!
@brief
  Creates a new OS timer and associates it with the supplied event.
  If a callback function is not NULL, then the timer expiration will result
  in a callback.

  If a callback function is NULL, then the timer expiration will result
  in a signal being set to the current task which called the "new".

@return
  TRUE  - Timer successfully created and initialised
  FALSE - Error creating timer

@note

  - Dependencies
    - None

  - Side Effects
    - This routine "mallocs" the memory for a timer structure.
*/
/*=========================================================================*/
boolean oncrpc_timer_new_common
(
  oncrpc_timer_ptr   *ptimer,       /*!< Pointer to a timer structure        */
  oncrpc_event_t      event,         /*!< Event mask to associate with timer  */
  oncrpc_timer_cb_func_type  cb_func,
  oncrpc_timer_cb_param_type cb_param
)
{
  boolean rc = FALSE;
  linux_timer_type *ptr;
  linux_timer_type *p_timer_active_q_tip;
  ptr = *ptimer = (oncrpc_timer_ptr) oncrpc_mem_alloc(sizeof(linux_timer_type));    
  //printf ("%s LINE %d ptr= 0x%08x, ptimer=0x%08x\n",__FUNCTION__,__LINE__,(int)ptr,(int)*ptimer);  
  //printf ("TIMER CREATED pointer 0x%08x %s LINE %d\n",(int)ptimer,__FUNCTION__,(int)__LINE__);

  if (ptr != NULL) {
    //printf ("%s LINE %d\n",__FUNCTION__,__LINE__);
    rc = TRUE;
    ptr->event = event;       
    ptr->thread_handle = oncrpc_thread_handle_get();
    ptr->timer_id = 0;
    ptr->evp.sigev_signo = (int)SIGUSR2;
    ptr->timer_created = 0;
    ptr->timer_state = TIMER_INACTIVE;
    ptr->cb_func = cb_func;
    ptr->cb_param = cb_param;
    
    pthread_mutex_lock(&linux_timer_q_mutex);
    p_timer_active_q_tip = (linux_timer_type *) q_check (&linux_timer_active_q);
    q_put(&linux_timer_active_q,q_link( ptimer,&(ptr->link)));
    pthread_mutex_unlock(&linux_timer_q_mutex);
    sigaction(ptr->evp.sigev_signo, &timer_sig_act, NULL);
  }
  else
  {
    ERR_FATAL( "Failed to allocate timer memory", 0, 0, 0 );
  }  
  return (rc);
} /* oncrpc_timer_new */




/*===========================================================================
  FUNCTION  oncrpc_timer_free
===========================================================================*/
/*!
@brief
  Frees up any resources associated with a timer.

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void oncrpc_timer_free
(
  oncrpc_timer_ptr  ptimer         /*!< Pointer to a timer structure        */
)
{
   linux_timer_type *ptr = (oncrpc_timer_ptr )ptimer;
   if(! q_delete_ext(&linux_timer_active_q,&ptr->link))
   {
     ERR("Timer %d not found in queue",(int)ptr->timer_id,0,0);
   }
   if(ptr->timer_created != 0)
   {
     timer_delete(ptr->timer_id);
     ptr->timer_created = 0;
   }
   oncrpc_mem_free(ptr);
} /* oncrpc_timer_free */


/*===========================================================================
  FUNCTION  oncrpc_timer_set
===========================================================================*/
/*!
@brief
  Programs a timer to expire after the requested number of ms.

@return
  None

@note

  - Dependencies
    - Timer must first be initialised using oncrpc_timer_init

  - Side Effects
    - Timer will cause the requested event to occur when the time value is
      reached
*/
/*=========================================================================*/
void oncrpc_timer_set
(
  oncrpc_timer_ptr  ptimer,         /*!< Pointer to a timer structure      */
  oncrpc_time_ms_t  msecs           /*!< Expirary time in ms               */
)
{
  linux_timer_type *ptr = (oncrpc_timer_ptr)ptimer;   

  if(ptr !=NULL)
  { 
    pthread_mutex_lock(&linux_timer_state_mutex);   
    if(ptr->timer_created == 0)
    {
      int status;
      status = timer_create(CLOCK_REALTIME,  &(ptr->evp), &(ptr->timer_id));
      if(status == -1 )
      {
         printf("Timer Create failed function:%s line:%d \n",__FUNCTION__,__LINE__);         
         ptr->timer_state = TIMER_INACTIVE;
         pthread_mutex_unlock(&linux_timer_state_mutex);
         return;
      }  
      ptr->timer_created = 1;    
    }    
    ptr->timeout.it_value.tv_sec = msecs/1000;
    ptr->timeout.it_value.tv_nsec =  (long)(msecs%1000)*1000000;        
    timer_settime(ptr->timer_id, 0,&(ptr->timeout) ,NULL);
    ptr->timer_state = TIMER_ACTIVE;
    pthread_mutex_unlock(&linux_timer_state_mutex);
    }  
} /* oncrpc_timer_set */

/*===========================================================================
  FUNCTION  oncrpc_timer_clr
===========================================================================*/
/*!
@brief
  Stops a currently running timer.

@return
  None

@note

  - Dependencies
    - Timer must first be initialised using oncrpc_timer_init

  - Side Effects
    - None
*/
/*=========================================================================*/
void oncrpc_timer_clr
(
  oncrpc_timer_ptr  ptimer         /*!< Pointer to a timer structure        */
)
{
  linux_timer_type *ptr = (oncrpc_timer_ptr)ptimer;   
  if(ptr !=NULL)
  {
    pthread_mutex_lock(&linux_timer_state_mutex);
    ptr->timer_state = TIMER_INACTIVE;
    pthread_mutex_unlock(&linux_timer_state_mutex);
    ptr->timeout.it_value.tv_sec = 0;
    ptr->timeout.it_value.tv_nsec =  0;
    timer_settime(ptr->timer_id, 0,&(ptr->timeout) ,NULL);
  }
} /* oncrpc_timer_clr */

/*===========================================================================
  FUNCTION  oncrpc_print_timer_info
===========================================================================*/
/*!
@brief
  Print timer info

@return
  None

@note

  - Dependencies
    - Timer must first be initialised using oncrpc_timer_init

  - Side Effects
    - None
*/
/*=========================================================================*/
void oncrpc_print_timer_info
(
  oncrpc_timer_ptr  ptimer         /*!< Pointer to a timer structure        */
)
{
   struct itimerspec gettime_value;
   linux_timer_type *timer_ptr = (linux_timer_type*)ptimer;
   timer_gettime(timer_ptr->timer_id,&gettime_value);
   printf("timer_gettime   Value sec:%u, ns:%lu \n",(int)(gettime_value.it_value.tv_sec), (long)(gettime_value.it_value.tv_nsec));
}


/*===========================================================================
  FUNCTION  timer_sig_handler
===========================================================================*/
/*!
@brief
  Process timer signal

@return
  None

@note

  - Dependencies
    - Timer must first be initialised using oncrpc_timer_init

  - Side Effects
    - None
*/
/*=========================================================================*/
void timer_sig_handler(int signo)
{
  linux_timer_type *p_timer;
  struct itimerspec gettime_value;
  int i = 0;
  switch(signo)
  {
    case SIGUSR2:      
      //printf(" TIMER SIGNAL RECEIVED:  \n");      
       /*------------------------------------------------------
        Got through active timer queue and check which
        timers are expired, send signal to those threads
        waiting on the timer signal 
        -----------------------------------------------------*/
      p_timer = (linux_timer_type *) q_check (&linux_timer_active_q);
      while( p_timer )
      {
         i++;
         if(p_timer->timer_state == TIMER_ACTIVE)           
         {
           timer_gettime(p_timer->timer_id,&gettime_value);
           if( (gettime_value.it_value.tv_sec == 0) && (gettime_value.it_value.tv_nsec == 0) )
           {
             pthread_mutex_lock(&linux_timer_state_mutex);
             p_timer->timer_state = TIMER_INACTIVE;
             pthread_mutex_unlock(&linux_timer_state_mutex);
             if(p_timer->cb_func != NULL)
             {
                 //printf("Timer %d is expired, calling callback \n",(int)p_timer->timer_id);
                 p_timer->cb_func(p_timer->cb_param);
             }
             else
             {
               //printf("Timer %d is expired, sending event \n",(int)p_timer->timer_id);
               oncrpc_event_set(p_timer->thread_handle, p_timer->event);
             }
           }
           else
           {
             //printf("Timer %d is active but not expired yet \n",(int)p_timer->timer_id);
           }
         }
         else
         {
           //printf("Timer %d is inactive \n",(int)p_timer->timer_id);
         }
         p_timer  = (linux_timer_type *)q_next(&linux_timer_active_q,&(p_timer->link));
      }  
      break;
  }
}

/*===========================================================================
 * End of module
 *=========================================================================*/



