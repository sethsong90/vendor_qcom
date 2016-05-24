/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                O N C R P C _ P L U G G E R _ S E R V E R . C 

GENERAL DESCRIPTION
  The plugger keeps track of the svc_register registeration database.
  This file implements the database registeration and de-registration
  as well as the find/dispatch functionality. 

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

  ONCRPC calls these functions.  See ONCRPC.

 Copyright (c) 2005-2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_plugger_server.c#5 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
09/25/08    rr     Changes to support backwards compatible API
08/27/08    rr     Added svc_register_auto_apiversions function for handling
                   backwards compatible apis.
06/02/08    hn     Added svc_register_auto function.
04/29/08    ih     Added WM7 support
04/09/08    hn     Moved XDR_CONTROL to correct svc_register function.
07/10/07    ptm    Clean up featrization and remove SMD transport.
06/01/07    hn     Don't svc_lock() if registration fails in svc_register
06/01/07    hn     svc_register() registers w/ plugger module & lower layers,
                   svc_register_with_plugger() only w/ plugger module.
06/01/07    hn     Deprecated _with_proxy registration function.
05/21/07    hn     Fixed conditional by adding required parenthesis.
05/11/07    hn     Added backwards compatibility support for router protocol.
05/11/07    hn     svc_register_with_proxy handles repeat registration.
11/22/06    ptm    Remove STA proxy task - no longer needed.
10/03/06    hn     Added support for locking/unlocking RPC services.
08/17/06    ptm    Fix reported min and max server versions.
08/01/06    ptm    Add support for STA proxy task.
12/23/05    ptm    Featurized oncrpc_plugger_export_to_mask routine.
09/13/05    clp    Featurized the include of oncrpc_proxy.h
05/27/05    clp    Added get_port functionality.
05/20/05    clp    Added this header comment.
===========================================================================*/

#include "comdef.h"
#include "oncrpc.h"
#include "oncrpci.h"
#include "oncrpc_plugger.h"
#include "oncrpc_plugger_init.h"
#include "err.h"
#include "assert.h"
#include "oncrpc_proxy.h"



#define ONCRPC_PLUGGER_ALL_TRANS (0xFFFFFFFF)

/*======================================================================
 * Defines to support the STA services
 *======================================================================*/
#define ONCRPC_STA_MIN_PROG_NUM 0x20000000
#define ONCRPC_STA_MAX_PROG_NUM 0x2000FFFF

typedef struct oncrpc_plugger_reg_element_struct {
  oncrpc_plugger_list_struct                  public;
  struct oncrpc_plugger_reg_element_struct ** next; /* Point to next in public */
  __dispatch_fn_t                             dispatch_func;
  uint8                                     * dynamic_addr;
  xdr_s_type                                * xdr_list; /* to allow free */
  bool_t                                      locked;
} oncrpc_plugger_reg_element;

  
static oncrpc_plugger_reg_element * oncrpc_plugger_list_head = NULL;



static oncrpc_plugger_reg_element *
oncrpc_plugger_get_list_item
(
  bool_t allocate_if_not_found,
  u_int  prog,
  u_int  vers
)
{
  oncrpc_plugger_reg_element * curr = oncrpc_plugger_list_head;

  while ( NULL != curr )
  {
    if ( prog == curr->public.prog && vers == curr->public.vers )
    {
      return curr;
    }
    curr = *curr->next;
  }
  if ( TRUE == allocate_if_not_found )
  {
    curr = oncrpc_mem_alloc( sizeof( oncrpc_plugger_reg_element ) );
    ASSERT( NULL != curr );
    memset( curr, 0, sizeof( oncrpc_plugger_reg_element ) );
    curr->public.prog = prog;
    curr->public.vers = vers;
    curr->public.xport_mask = 0;
    curr->next = (oncrpc_plugger_reg_element **)&curr->public.next;
    *curr->next = oncrpc_plugger_list_head;
    /* LOCK? */
    oncrpc_plugger_list_head = curr;
    /* UNLOCK? */
    curr->locked = FALSE; /* Services are unlocked by default until all changes
                           * are put in the code to unlock them manually.
                           */
  }
  return curr;
} /* oncrpc_plugger_get_list_item */

#ifdef FEATURE_ONCRPC_PLUGGER
static oncrpc_plugger_reg_element *
oncrpc_plugger_remove_item_by_elf
(
  void * elf
)
{
  oncrpc_plugger_reg_element ** curr = &oncrpc_plugger_list_head;
  oncrpc_plugger_reg_element *  ret = NULL;

  while ( NULL != *curr )
  {
    if ( elf == (*curr)->dynamic_addr )
    {
      MSG_MED( "oncrpc_plugger_remove_item_by_elf removed %x %d", 
               (*curr)->public.prog, (*curr)->public.vers, 0 );
      /* Fun with double pointers */
      ret = *curr;
      /* LOCK? */
      *curr= *(*curr)->next;
      /* UNLOCK? */
      break;
    }
    curr = (*curr)->next;
  }
  return ret;
} /* oncrpc_plugger_remove_item_by_elf */

static void
oncrpc_plugger_reg_module
( 
 oncrpc_plugger_reg_element * element,
 u_int                        xport_mask
)
{
  XDR * transp;
  rpcprog_t prog = element->public.prog;
  rpcvers_t vers = element->public.vers;
  __dispatch_fn_t dispatch_func = element->dispatch_func;


  /* Modify xport mask to include only those that are actually off */
  xport_mask &= ~element->public.xport_mask;
  element->public.xport_mask |= xport_mask;

#ifdef FEATURE_ONCRPC_DIAG
  if ( ONCRPC_PLUGGER_DIAG_TRANS == (ONCRPC_PLUGGER_DIAG_TRANS & xport_mask) )
  {
    transp = svcdiag_create(0,0);
    if ( transp == NULL ) {
      ERR_FATAL( "cannot create diag service for 0x%x %d.", prog, vers, 0);
    }
    if ( !svc_register(transp, prog, vers, dispatch_func, 
                       ONCRPC_DIAG_PROTOCOL) )
    {
      ERR_FATAL( "unable to register (0x%x, %d, diag).", prog, vers, 0);
    }
  }
#endif /* FEATURE_ONCRPC_DIAG */

  if ( ONCRPC_PLUGGER_RTR_TRANS == (ONCRPC_PLUGGER_RTR_TRANS & xport_mask) )
  {
    transp = svcrtr_create(0,0);
    if ( transp == NULL ) {
      ERR_FATAL( "cannot create sm service for 0x%x %d.", prog, vers, 0);
    }
    if ( !svc_register(transp, prog, vers, dispatch_func,
                       ONCRPC_RTR_PROTOCOL) )
    {
      ERR_FATAL( "unable to register (0x%x, %d, sm).", prog, vers, 0);
    }
  }
} /* oncrpc_plugger_reg_module */

static oncrpc_plugger_reg_element *
oncrpc_plugger_init_module
(
  u_int xport_mask,
  u_int prog,
  u_int vers,
  __dispatch_fn_t dispatch_func,
  void (*boot_func)(void)
)
{
  oncrpc_plugger_reg_element * element;

  ASSERT( NULL != dispatch_func );
  element = oncrpc_plugger_get_list_item( TRUE, prog, vers );
  ASSERT( NULL != element );
  element->dispatch_func = dispatch_func;

  if ( NULL != boot_func )
  {
    boot_func();
  }

  oncrpc_plugger_reg_module( element, xport_mask );
  return element;
} /* oncrpc_plugger_init_module */
#endif /* FEATURE_ONCRPC_PLUGGER */

static xdr_s_type *
oncrpc_plugger_unregister_xdr
(
  oncrpc_plugger_reg_element * element,
  rpcprot_t                    prot
)
{
  xdr_s_type ** curr_xdr;
  xdr_s_type  * xdr = NULL;

  ASSERT( NULL != element );
  /* Find the xdr, if any, in the list */
  curr_xdr = &element->xdr_list;
  while ( NULL != *curr_xdr )
  {
    if ( (*curr_xdr)->protocol == prot )
    {
      /* Found the xdr */
      xdr = *curr_xdr;
      /* LOCK? */
      *curr_xdr = (*curr_xdr)->xp_next;
      /* UNLOCK? */
      break;
    }
    curr_xdr = &(*curr_xdr)->xp_next;
  }
  return xdr;
} /* oncrpc_plugger_unregister_xdr */

static void
oncrpc_plugger_unreg_module
( 
 oncrpc_plugger_reg_element * element,
 u_int                        xport_mask
)
{
  xdr_s_type * xdr;

  xdr = NULL;
  /* Modify xport mask to include only those that are actually on */
  xport_mask &= element->public.xport_mask;

#ifdef FEATURE_ONCRPC_DIAG
  if ( ONCRPC_PLUGGER_DIAG_TRANS == (ONCRPC_PLUGGER_DIAG_TRANS & xport_mask) )
  {
    xdr = oncrpc_plugger_unregister_xdr( element, ONCRPC_DIAG_PROTOCOL );
    if ( NULL != xdr ) 
    {
      XDR_DESTROY( xdr );
    }
  }
#endif /* FEATURE_ONCRPC_DIAG */

  if ( ONCRPC_PLUGGER_RTR_TRANS == (ONCRPC_PLUGGER_RTR_TRANS & xport_mask) )
  {
    xdr = oncrpc_plugger_unregister_xdr( element, ONCRPC_RTR_PROTOCOL );
    if ( NULL != xdr ) 
    {
      XDR_DESTROY( xdr );
    }
  }

#ifdef FEATURE_ONCRPC_LO
  if ( ONCRPC_PLUGGER_LO_TRANS == (ONCRPC_PLUGGER_LO_TRANS & xport_mask) )
  {
    xdr = oncrpc_plugger_unregister_xdr( element, ONCRPC_LO_PROTOCOL );
    if ( NULL != xdr ) 
    {
      XDR_DESTROY( xdr );
    }
  }
#endif /* FEATURE_ONCRPC_LO */


  /* Clear the bits for the services we have turned off.  */
  element->public.xport_mask &= ~xport_mask;
} /* oncrpc_plugger_unreg_module */


static uint32 
oncrpc_plugger_xport_to_mask
(
  rpcprot_t protocol
)
{
  switch ( protocol )
  {
#ifdef FEATURE_ONCRPC_DIAG
  case ONCRPC_DIAG_PROTOCOL:
    return ONCRPC_PLUGGER_DIAG_TRANS;
#endif /* FEATURE_ONCRPC_DIAG */

  case ONCRPC_RTR_PROTOCOL:
    return ONCRPC_PLUGGER_RTR_TRANS;

#ifdef FEATURE_ONCRPC_LO
  case ONCRPC_LO_PROTOCOL:
    return ONCRPC_PLUGGER_LO_TRANS;
#endif /* FEATURE_ONCRPC_LO */
  }

  ERR_FATAL( "Unknown protocol %d", protocol, 0, 0 );
  return 0;
} /* oncrpc_plugger_xport_to_mask */

/* The ONCRPC exported API */


/*===========================================================================
FUNCTION SVC_REGISTER_AUTO_APIVERSIONS

DESCRIPTION
  This function automatically registers an RPC server based on just the
  program/version numbers information and the dispatch routine, and saves
  the "api_version" function which will return the set of minor api versions
  supported.

  It is meant to be used by RPC glue code generated for Qualcomm remoted APIs.

PARAMETERS
  program - program number
  version - max minor version supported, 
  dispatch - dispatch function for the service.
  api_versions - function to retrieve api versions.

RETURN VALUE
  Boolean, TRUE if registered ok, else FALSE.

DEPENDENCIES
  IMPORTANT: This routine depends on the requirement that any RPC program
  number assigned to a callback service have its 0x01000000 bit set, and that
  any RPC program that is not a callback service NOT have this bit set.

SIDE EFFECTS
  None
===========================================================================*/
bool_t
svc_register_auto_apiversions
(
  rpcprog_t    prog, 
  rpcvers_t    vers,
  void       (*dispatch) (struct svc_req *, SVCXPRT *),
  uint32     *(*api_versions) (uint32 *size_entries)
)
{
  SVCXPRT *svc;
  bool_t status = TRUE;
  bool_t (*register_routine)(SVCXPRT *, rpcprog_t, rpcvers_t,
                             void (*) (struct svc_req *, SVCXPRT *), rpcprot_t);

  /* If the program number is a callback program number, use
   * svc_register_with_plugger. Otherwise, use qc_svc_register
   */
  if ( prog & 0x01000000 )
  {
    register_routine = svc_register_with_plugger;
  }
  else
  {
    register_routine = qc_svc_register;
  }

  /* Register server on RPC Router transport */
  svc = svcrtr_create(0, 0);
  if ( svc == NULL )
  {
    ERR_FATAL( "Failed to create rtr server transport.", 0, 0, 0 );
    status = FALSE;
  }
  else if ( !register_routine(svc, prog, vers, dispatch, ONCRPC_RTR_PROTOCOL) )
  {
    ERR_FATAL( "Failed to register RPC program %d-%d on rtr transport.",
               prog, vers, 0 );
    status = FALSE;
  }

  return status;
}
/* notes on the svc_register and svc_unregister functions: 
 * The data structures (linked lists) are optimized for the register
 * case, which is guaranteed to happen. The unregister will likely be
 * an occational event (at most).
 * The passed in XDR's are stored in a linked list off of the
 * reg_element.  The only purpose of saving these is so that when
 * unregister is called, the xdr can be passed back allowing for it to
 * be freed.  The reg_element also contains a pointer to a dynamic
 * module, if reg_element is used for registering that, and in that
 * case will be freed when the dynamic module is freed.
 */

/*===========================================================================
FUNCTION SVC_REGISTER_AUTO

DESCRIPTION
  Wrapper to svc_register_auto_apiversions for backwards compatibiilty

PARAMETERS
  program - program number
  version - max minor version supported, 
  dispatch - dispatch function for the service.

RETURN VALUE
  Boolean, TRUE if registered ok, else FALSE.

DEPENDENCIES
  IMPORTANT: This routine depends on the requirement that any RPC program
  number assigned to a callback service have its 0x01000000 bit set, and that
  any RPC program that is not a callback service NOT have this bit set.

SIDE EFFECTS
  None
===========================================================================*/
bool_t
svc_register_auto
(
  rpcprog_t    prog, 
  rpcvers_t    vers,
  void       (*dispatch) (struct svc_req *, SVCXPRT *)
)
{
  return svc_register_auto_apiversions(prog,vers,dispatch,NULL);
} /* svc_register_auto */



/*===========================================================================
FUNCTION SVC_REGISTER_WITH_PLUGGER (registers only with plugger module)

DESCRIPTION
  This function allocates a slot in the rpc_svc_list and registers the
  parameters in it.

PARAMETERS
  xprt, program, version, protocol and dispatch function for the service.

RETURN VALUE
  Boolean, TRUE if registered ok, else FALSE.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
bool_t
svc_register_with_plugger
(
  xdr_s_type * xdr, 
  rpcprog_t    prog, 
  rpcvers_t    vers,
  void       (*dispatch) (struct svc_req *, SVCXPRT *),
  rpcprot_t    protocol
)
{
  oncrpc_plugger_reg_element * element;
  uint32                       xport_mask;
  bool_t                       ret = FALSE;

  if ( ((rpcprot_t)-3) == protocol )
  {
    protocol = ONCRPC_RTR_PROTOCOL;
  }
  xport_mask = oncrpc_plugger_xport_to_mask( protocol );
  if ( 0 != xport_mask )
  {
    element = oncrpc_plugger_get_list_item( TRUE, prog, vers );
    ASSERT( NULL != element );
    if ( 0 == element->public.xport_mask )
    {
      element->dispatch_func = dispatch;
    } 
    if ( ! ( element->public.xport_mask & xport_mask ) )
    {
      xdr->xp_next = element->xdr_list;
      /* LOCK? */
      element->xdr_list = xdr;
      /* UNLOCK? */
      element->public.xport_mask |= xport_mask;
    }
    ret = TRUE;
  }

#ifdef FEATURE_ONCRPC_PM
  pmap_set( prog, vers, protocol, XPORT_GET_PORT(xdr->xport) );
#endif

  return ret;
} /* svc_register_with_plugger */


/*===========================================================================
FUNCTION SVC_REGISTER (registers with plugger module and lower layers)

DESCRIPTION
  Calls svc_register_with_plugger and then makes the lower layer in ONCRPC
  know about the registration.

PARAMETERS
  xprt, program, version, protocol and dispatch function for the service.

RETURN VALUE
  Boolean, TRUE if registered ok, else FALSE.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
bool_t
qc_svc_register
(
  SVCXPRT * xprt, 
  rpcprog_t prog, 
  rpcvers_t vers,
  void (*dispatch) (struct svc_req *, SVCXPRT *),
  rpcprot_t protocol
)
{
  bool_t ret;
  oncrpc_control_register_server_type control_cmd;

  ret = svc_register_with_plugger( xprt, prog, vers, dispatch, protocol);

  if( TRUE == ret )
  {
    if ( ONCRPC_STA_MIN_PROG_NUM <= prog && ONCRPC_STA_MAX_PROG_NUM >= prog )
    {
      /* STA services are enabled by default */
      svc_lock(prog, vers, FALSE);
    }

    /* Register server with lower layers */
    control_cmd.prog_ver.prog = prog;
    control_cmd.prog_ver.ver = vers;
        
    if(! XDR_CONTROL(xprt,ONCRPC_CONTROL_REGISTER_SERVER,&control_cmd))
    {
      MSG_ERROR("Failed to register server", prog, vers, 0);
      ret = FALSE;
    }
  }  

  return ret;
} /* svc_register */

bool_t
svc_register_with_proxy
(
  xdr_s_type * xdr, 
  rpcprog_t    prog, 
  rpcvers_t    vers,
  void       (*dispatch) (struct svc_req *, SVCXPRT *),
  rpcprot_t    protocol,
  oncrpc_proxy_task_s_type *proxy
)
{
  return svc_register( xdr, prog, vers, dispatch, protocol);
} /* svc_register_with_proxy */

/*===========================================================================
FUNCTION SVC_LOCK

DESCRIPTION
  This function can be used by tasks to lock/unlock their registered services.

PARAMETERS
  program and version to lock/unlock.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void
svc_lock
(
  rpcprog_t prog,
  rpcvers_t vers,
  bool_t    lock
)
{
  oncrpc_plugger_reg_element * curr = oncrpc_plugger_list_head;

  while ( NULL != curr )
  {
    if ( curr->public.prog == prog && curr->public.vers == vers )
    {
      curr->locked = lock;
      break;
    }
    curr = *curr->next;
  }
} /* svc_lock */

/*===========================================================================
FUNCTION SVC_UNREGISTER

DESCRIPTION
  This function finds the slots that contains the program/version and
  unregisters it, freeing the slots.  Note, one program may advertise
  on multiple transports.

PARAMETERS
  program and version to unregister.

RETURN VALUE
  Boolean, TRUE if registered ok, else FALSE.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void
svc_unregister 
(
  rpcprog_t prog, 
  rpcvers_t vers,
  rpcprot_t prot
)
{
  oncrpc_plugger_reg_element * element;
  uint32                       xport_mask;

  xport_mask = oncrpc_plugger_xport_to_mask( prot );
  element = oncrpc_plugger_get_list_item( FALSE, prog, vers );
  if ( NULL != element )
  {
    oncrpc_plugger_unreg_module( element, xport_mask );
  }
} /* svc_unregister */


/*===========================================================================
FUNCTION  SVC_FIND_DISPATCH

DESCRIPTION
  This function finds the slots that contains the program/version and
  dispatches the call for processing.

  Find supports backwards compatible API.  The program number must match.
  Version fields:
  Bit 31     : 1 = Hashkey version type, 0 = Backwards compatible version
  Bits 28-30 : Reserved
  Bits 16-27 : Major number (must match)
  Bits 15-0  : Minor version requested must be smaller or equal than 
               version supported by server.

PARAMETERS
  program and version to find.

RETURN VALUE
  ONCRPC_PLUGGER_FIND_DISPATCHED  (call was dispatched)
  ONCRPC_PLUGGER_FIND_NOT_FOUND  (call was not found)
  ONCRPC_PLUGGER_FIND_FOUND      (found not dispatched)
  ONCRPC_PLUGGER_FIND_LOCKED     (program locked)

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/

oncrpc_plugger_find_status
svc_find_dispatch
(
 xdr_s_type     * xdr,
 struct svc_req * call,
 rpcvers_t      * max,
 rpcvers_t      * min
)
{
  oncrpc_plugger_reg_element * curr = oncrpc_plugger_list_head;
  uint32 xport_mask = oncrpc_plugger_xport_to_mask( xdr->protocol );
  oncrpc_plugger_find_status ret = ONCRPC_PLUGGER_FIND_NOT_FOUND;
  
  if ( 0 == xport_mask )
  {
    ERR( "Unrecognized xport protocol %d", xdr->protocol, 0, 0 );
  }
  else
  {
    *max = 0;
    *min = 0xffffffff;
  
    while ( NULL != curr ) 
    {
      if ( curr->public.prog == call->rq_prog &&
           (curr->public.xport_mask & xport_mask) != 0 )
      {
        ret = ONCRPC_PLUGGER_FIND_FOUND;
        if( ( ( call->rq_vers & 0x80000000 ) && ( curr->public.vers == call->rq_vers ) ) ||
            ( !( call->rq_vers & 0x80000000 ) && 
               ( ( curr->public.vers & 0x0fff0000 ) == ( call->rq_vers & 0x0fff0000 ) ) 			  
             )
          )
        {
          /* Check if the service is currently locked */
          if ( curr->locked )
          {
            ret = ONCRPC_PLUGGER_FIND_LOCKED;
          }
          else
          {
            oncrpc_proxy_dispatch( call, xdr, curr->dispatch_func );
            ret = ONCRPC_PLUGGER_FIND_DISPATCHED;
          }
          break;
        }
        *min = MIN( curr->public.vers, *min );
        *max = MAX( curr->public.vers, *max );
      }
      curr = *curr->next;
    }
  }
  return ret;
} /* svc_find_dispatch */

#ifdef FEATURE_ONCRPC_PLUGGER
/* The XDR exported API */

static union oncrpc_plugger_result_union {
  char                * cptr_result;
  bool_t                bool_result;
  u_int                 uint_result;
  oncrpc_plugger_list   plugger_list;
} oncrpc_plugger_result;

void *
oncrpc_plugger_null_0_svc(void *argp, struct svc_req *rqstp)
{
  return (void *) &oncrpc_plugger_result.cptr_result;
}

oncrpc_plugger_list *
oncrpc_plugger_list_all_0_svc(void *argp, struct svc_req *rqstp)
{
  oncrpc_plugger_result.plugger_list = &oncrpc_plugger_list_head->public;
  return &oncrpc_plugger_result.plugger_list;
}

void *
oncrpc_plugger_register_all_0_svc(u_int *xport_mask, struct svc_req *rqstp)
{
  oncrpc_plugger_reg_element * element = oncrpc_plugger_list_head;

  while ( NULL != element )
  {
    oncrpc_plugger_reg_module( element, *xport_mask );
    element = *element->next;
  } 
  return (void *) &oncrpc_plugger_result.cptr_result;
}

void *
oncrpc_plugger_deregister_all_0_svc(u_int *xport_mask, struct svc_req *rqstp)
{
  oncrpc_plugger_reg_element * element = oncrpc_plugger_list_head;

  while ( NULL != element )
  {
    oncrpc_plugger_unreg_module( element, *xport_mask );
    element = *element->next;
  } 
  return (void *) &oncrpc_plugger_result.cptr_result;
}

void *
oncrpc_plugger_register_prog_0_svc(oncrpc_plugger_prog_args *argp, struct svc_req *rqstp)
{
  oncrpc_plugger_reg_element * element;

  element = oncrpc_plugger_get_list_item( FALSE, argp->prog, argp->vers );
  if ( NULL != element )
  {
    oncrpc_plugger_reg_module( element, argp->xport_mask );
  }
  return (void *) &oncrpc_plugger_result.cptr_result;
}

void *
oncrpc_plugger_deregister_prog_0_svc(oncrpc_plugger_prog_args *argp, struct svc_req *rqstp)
{
  oncrpc_plugger_reg_element * element;

  element = oncrpc_plugger_get_list_item( FALSE, argp->prog, argp->vers );
  if ( NULL != element )
  {
    oncrpc_plugger_unreg_module( element, argp->xport_mask );
  }
  return (void *) &oncrpc_plugger_result.cptr_result;
}

u_int *
oncrpc_plugger_add_prog_0_svc
(
  oncrpc_plugger_add_args *argp,
  struct svc_req *rqstp
)
{
  uint8 * elf;
  void  * dispatch_func;
  void  * boot_func;
  oncrpc_plugger_reg_element * element;

  elf = oncrpc_mem_alloc( argp->prog_code.prog_code_len );
  ASSERT(NULL != elf);
  memcpy( elf, argp->prog_code.prog_code_val, argp->prog_code.prog_code_len );
  dispatch_func = elf + argp->dispatch_offset;
  boot_func = NULL == argp->boot_offset ? NULL : elf + *argp->boot_offset;
  /* FIXME flush cache */
  element = oncrpc_plugger_init_module( 0, 
                                        argp->prog,
                                        argp->vers,
                                        (__dispatch_fn_t)dispatch_func,
                                        (void (*)(void))boot_func );

  element->dynamic_addr = elf;
  oncrpc_plugger_result.uint_result = (u_int)elf;
  return &oncrpc_plugger_result.uint_result;
}

bool_t *
oncrpc_plugger_remove_prog_0_svc(u_int *handle, struct svc_req *rqstp)
{
  oncrpc_plugger_reg_element * element;

  if ( NULL == handle )
  {
    oncrpc_plugger_result.bool_result = FALSE;
  }
  else
  {
    element = oncrpc_plugger_remove_item_by_elf( *(void **)handle );
    if ( NULL != element )
    {
      oncrpc_plugger_unreg_module( element, ONCRPC_PLUGGER_ALL_TRANS );
      oncrpc_mem_free( element );
      oncrpc_mem_free( handle );
      oncrpc_plugger_result.bool_result = TRUE;
    }
    else
    {
      oncrpc_plugger_result.bool_result = FALSE;
    }
  }
  return &oncrpc_plugger_result.bool_result;
}
#endif /* FEATURE_ONCRPC_PLUGGER */
