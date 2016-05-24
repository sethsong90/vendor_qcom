#ifndef ONCRPC_OS_H
#define ONCRPC_OS_H
/*===*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
                          O N C R P C _ O S . H
 *===*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*!
  @file
    oncrpc_os.h

  @brief
    Generic OS function prototypes and typedefs.

  @detail
    These functions should be implemented using the native OS functions
    provided by the OS being used.

 Copyright (c) 2007 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
 */
/*===*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_os.h#3 $

when       who     what, where, why
--------   ---     -------------------------------------------------------
03/25/08    ih     Add oncrpc_tls_find and oncrpc_tls_find_common
02/15/08    rr     Add oncrpc_timer_new_cb
11/30/07   ptm     Add oncrpc_strncpy.
10/31/07   ptm     Add support for task names in tls.
10/17/07   ptm     Merge tls-header-type with tls-type.
10/17/07    hn     tls_get and tls_delete now take a thread's key, added
                   tls_get_self and tls_delete_self as the no args version
10/12/07   ptm     Added oncrpc-rpc-is-allowed and oncrpc-get-task-name.
10/08/07   ptm     Clean up oncrpc thread handle references.
08/22/07   ptm     Unified access to thread local storage.
07/10/07   ptm     Change d word to uint 32.
04/13/07   RJS     First revision.

===========================================================================*/
/*===========================================================================
                      TYPE DEFINITIONS AND TYPES
===========================================================================*/

#include "oncrpc.h"

/* We want this to be at least 13 so that there is room for 12 bytes plus
   NULL because we can log up to 12 bytes.*/
#define ONCRPC_THREAD_NAME_SIZE        13

typedef void*  oncrpc_thread_handle;  /*!< Generic OS thread/task handle   */
typedef uint32 oncrpc_event_t;        /*!< Generic OS event type           */
typedef void*  oncrpc_crit_sect_ptr;  /*!< Generic OS critical section ptr */
typedef void*  oncrpc_timer_ptr;      /*!< Generic OS timer ptr            */
typedef uint32 oncrpc_time_ms_t;      /*!< Generic OS timer ms time type   */
typedef uint32 oncrpc_task_pri_t;     /*!< Generic OS task priority type   */

typedef enum {
  TASK_STATE_UNDEF,
  TASK_STATE_RUNNING,
  TASK_STATE_STOPPED
} oncrpc_thread_state_type;

typedef struct {
  q_link_type               link;   /* Must be first */
  boolean  valid;
  void    *key;
  char                      name[ONCRPC_THREAD_NAME_SIZE];
  oncrpc_thread_handle      thread;
  oncrpc_thread_state_type  state;
  xdr_s_type               *clnt;
  xdr_s_type               *xdr;
  void                     *cb_data;
  void                     *parent; /* Thread ID of parent */
  uint32                    priority;
  rpcprot_t                 protocol;
} oncrpc_tls_type;

typedef struct {
  void                 *table;
  uint32                entry_size;
  uint32                num_entries;
  oncrpc_crit_sect_ptr *crit_sec;
} oncrpc_tls_table_type;

extern xdr_s_type *oncrpc_router_read_xdr;


typedef void (*oncrpc_timer_cb_func_type)( unsigned long data );
typedef unsigned long  oncrpc_timer_cb_param_type;

/*===========================================================================
                      EXTERNAL FUNCTION PROTOTYPES
===========================================================================*/

/*= = = = = = = = = = = = =
 * Initialisation
 *= = = = = = = = = = = = =*/
void                  oncrpc_os_init(void);
void                  oncrpc_tls_init(oncrpc_tls_type *tls);


/*= = = = = = = = = = = = =
 * Tasks/Threads
 *= = = = = = = = = = = = =*/
oncrpc_thread_handle   oncrpc_thread_handle_get(void);


/*= = = = = = = = = = = = =
 * Thread Local Storage
 *= = = = = = = = = = = = =*/
oncrpc_tls_type *oncrpc_tls_get( void * key );
void             oncrpc_tls_delete( void * key );
oncrpc_tls_type *oncrpc_tls_find( void * key );
oncrpc_tls_type *oncrpc_tls_get_self( void );
void             oncrpc_tls_delete_self( void );


/*= = = = = = = = = = = = =
 * Events
 *= = = = = = = = = = = = =*/
void                  oncrpc_event_set(oncrpc_thread_handle pthread_handle,
                                       oncrpc_event_t event);
void                  oncrpc_event_clr(oncrpc_thread_handle pthread_handle,
                                       oncrpc_event_t event);
oncrpc_event_t        oncrpc_event_wait(oncrpc_thread_handle pthread_handle,
                                        oncrpc_event_t event);
oncrpc_event_t        oncrpc_event_get(oncrpc_thread_handle pthread_handle,
                                        oncrpc_event_t event);
void                  oncrpc_task_pri_set(oncrpc_thread_handle pthread_handle,
                                          oncrpc_task_pri_t priority);

/*= = = = = = = = = = = = =
 * Critical sections
 *= = = = = = = = = = = = =*/
void                  oncrpc_crit_sect_init (oncrpc_crit_sect_ptr *cs);
void                  oncrpc_crit_sect_enter(oncrpc_crit_sect_ptr  cs);
void                  oncrpc_crit_sect_leave(oncrpc_crit_sect_ptr  cs);


/*= = = = = = = = = = = = =
 * Timers
 *= = = = = = = = = = = = =*/
boolean               oncrpc_timer_new (oncrpc_timer_ptr *timer,
                                        oncrpc_event_t event);
void                  oncrpc_timer_free(oncrpc_timer_ptr  timer);
void                  oncrpc_timer_set (oncrpc_timer_ptr  timer,
                                        oncrpc_time_ms_t msecs);
void                  oncrpc_timer_clr (oncrpc_timer_ptr  timer);

boolean oncrpc_timer_new_cb
(
  oncrpc_timer_ptr           *ptimer,       /*!< Pointer to a timer structure        */  
  oncrpc_timer_cb_func_type  cb_func,
  oncrpc_timer_cb_param_type cb_param
);
/*= = = = = = = = = = = = =
 * Common routines
 *= = = = = = = = = = = = =*/
/* @todo perhaps these should be in oncrpc_osi.h instead */
oncrpc_tls_type *oncrpc_tls_get_common( oncrpc_tls_table_type *tbl,
                                        void *key );
void oncrpc_tls_delete_common( oncrpc_tls_table_type *tbl, void *key );
oncrpc_tls_type *oncrpc_tls_find_common( oncrpc_tls_table_type *tbl, 
                                         void *key );

void oncrpc_rpc_is_allowed( void );

void oncrpc_set_task_name( char *name );
void oncrpc_get_task_name( char *name, uint32 len );
char *oncrpc_strncpy(char *dst, char *src, uint32 len);


/*= = = = = = = = = = = = =
 * De-initialisation
 *= = = = = = = = = = = = =*/
void oncrpc_tls_deinit(void);

/*===========================================================================
 * End of module
 *=========================================================================*/
#endif /* ONCRPC_OS_H */
