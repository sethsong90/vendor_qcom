/******************************************************************************

                        D S C _ D C M . C

******************************************************************************/

/******************************************************************************

  @file    dsc_dcm.c
  @brief   DSC's DCM (Connection Manager) Module

  DESCRIPTION
  Implementation of DCM (Data Connection Manager) module.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id$

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/08/09   SM         Added Support for physlink Evnt Indications
03/24/09   SM         Added Call End Reason Code Support
05/30/08   vk         Added support for GET_DEVICE_NAME IOCTL
04/14/08   ac         Increase the maximum number of iface by 1. It is for CDMA
03/09/08   vk         Incorporated code review comments
01/12/08   vk         Correcting iface selection to prefer UP ifaces
12/18/07   vk         Support for automatic client deregistration
11/29/07   vk         Cleaned up lint warnings
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* Needed for recursive pthread mutexes */
#endif

#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "dsci.h"
#include "dsc_util.h"
#include "dsc_dcm.h"
#include "dsc_dcmi.h"
#include "dsc_qmi_nasi.h"
#include "dserrno.h"

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Constant representing max number of IFACEs supported
---------------------------------------------------------------------------*/
#define DSC_MAX_IFACE   4

/*--------------------------------------------------------------------------- 
   Constant representing max number of network handles supported
---------------------------------------------------------------------------*/
#define DSC_MAX_NH      128

/*--------------------------------------------------------------------------- 
   Constant representing max number of clients supported
---------------------------------------------------------------------------*/
#define DSC_MAX_CLNT    16

/*--------------------------------------------------------------------------- 
   Constant representing the offset to be added to the local IFACE index to 
   obtain the IFACE ID
---------------------------------------------------------------------------*/
#define DSC_DCM_IFACE_ID_OFFSET (DCM_IFACE_ID_INVALID+1)

/*--------------------------------------------------------------------------- 
   Forward structure declaration needed for subsequent definitions
---------------------------------------------------------------------------*/
struct dsc_dcm_iface_s;

/*--------------------------------------------------------------------------- 
   Type representing collection of data structures used for event reporting
   maintained for each event per network handle
---------------------------------------------------------------------------*/
typedef struct dsc_dcm_nh_ev_info_s {
    ds_dll_el_t * l_head;
    ds_dll_el_t * l_tail;
} dsc_dcm_nh_ev_info_t;

typedef enum {
    DSC_DCM_NH_STATE_ACTIVE   = 0, /* Client owning net handle is active */
    DSC_DCM_NH_STATE_DETACHED = 1  /* Client owning net handle has exited */
} dsc_dcm_nh_state_t;

/*--------------------------------------------------------------------------- 
   Type representing collection of control info maintained for each network
   handle
---------------------------------------------------------------------------*/
typedef struct dsc_dcm_nh_info_s {
    int                             nh_id;            /* network handle id */
    int                             clnt_hdl;          /* client handle id */
    dcm_net_cb_fcn                  net_cb;            /* network callback */
    int                             call_end_reason;       /*LastNetDownReasonCode*/
    void                          * net_cb_user_data;     /* user data for */
                                                       /* network callback */
    dcm_iface_ioctl_event_cb_fcn    ev_cb;               /* event callback */
    void                          * ev_cb_user_data;      /* user data for */
                                                         /* event callback */
    dcm_net_policy_info_t           net_policy;          /* network policy */
    struct dsc_dcm_iface_s        * iface_p;              /* iface pointer */
    dsc_dcm_nh_ev_info_t            ev_info[DSS_IFACE_IOCTL_REG_EVENT_MAX];
                                                             /* event info */
    dsc_dcm_nh_state_t              state;         /* network handle state */
} dsc_dcm_nh_info_t;

/*--------------------------------------------------------------------------- 
   Type representing collection of data structures used for tracking 
   network handles bound to an IFACE
---------------------------------------------------------------------------*/
typedef struct dsc_dcm_if_nh_info_s {
    ds_dll_el_t * l_head; /* head of list of network handles */
    ds_dll_el_t * l_tail; /* tail of list of network handles */
    int           n_elem; /* number of elements in the list */
} dsc_dcm_if_nh_info_t;

/*--------------------------------------------------------------------------- 
   Type representing collection of data structures used for tracking 
   all events registered for an IFACE
---------------------------------------------------------------------------*/
typedef struct dsc_dcm_if_ev_info_s {
    ds_dll_el_t * l_head; /* head of list of events registered */
    ds_dll_el_t * l_tail; /* tail of list of events registered */
} dsc_dcm_if_ev_info_t;

/*--------------------------------------------------------------------------- 
   Type representing collection of generic configuration information for 
   an IFACE
---------------------------------------------------------------------------*/
typedef struct dsc_dcm_if_cfg_s {
    ip_addr_type ipv4_addr;      /* address assigned to iface */
    ip_addr_type ipv4_prim_dns_addr;
    ip_addr_type ipv4_seco_dns_addr;
    ip_addr_type ipv4_gateway_addr;
} dsc_dcm_if_cfg_t;

/*--------------------------------------------------------------------------- 
   Type representing collection of data structures and state variables 
   constituting an IFACE
---------------------------------------------------------------------------*/
typedef struct dsc_dcm_iface_s {
    dcm_iface_id_t          if_id; /* iface id */
    dcm_iface_name_t        if_name; /* iface name */
    dcm_iface_name_t        if_group; /* iface group */
    dcm_iface_state_t       if_state; /* iface state */
    dcm_phys_link_state_t   if_phlnk_state; /* phys link state */
    dsc_dcm_if_cfg_t        if_cfg; /* iface configuration */
    dsc_dcm_if_nh_info_t    if_nh_info; /* network handle info */
    dsc_dcm_if_ev_info_t    if_ev_info[DSS_IFACE_IOCTL_REG_EVENT_MAX];
                                /* event info */
    dsc_dcm_if_op_tbl_t     if_op_tbl; /* iface operations table */
    void                  * call_hdl; /* lower layer opaque identifier */
} dsc_dcm_iface_t;

/*--------------------------------------------------------------------------- 
   Type representing collection of control info maintained for each client
   handle
---------------------------------------------------------------------------*/
typedef struct dsc_dcm_clnt_info_s {
    int           clnt_id; /* client handle id */
    ds_dll_el_t * l_head;  /* head of list of network handles */
    ds_dll_el_t * l_tail;  /* tail of list of network handles */
} dsc_dcm_clnt_info_t;

/*--------------------------------------------------------------------------- 
   Collection of DCM module's global control info
---------------------------------------------------------------------------*/
static struct {
    pthread_mutex_t mutx; /* global mutex */
} dsc_dcm_ctrl;

/*--------------------------------------------------------------------------- 
   Array of pointers to IFACEs
---------------------------------------------------------------------------*/
static dsc_dcm_iface_t * dsc_dcm_if_parr[DSC_MAX_IFACE];

/*--------------------------------------------------------------------------- 
   Array of pointers to network handles
---------------------------------------------------------------------------*/
static dsc_dcm_nh_info_t * dsc_dcm_nh_info_parr[DSC_MAX_NH];

/*--------------------------------------------------------------------------- 
   List of pointers to client handles
---------------------------------------------------------------------------*/
static dsc_dcm_clnt_info_t * dsc_dcm_clnt_info_parr[DSC_MAX_CLNT];

/*--------------------------------------------------------------------------- 
   Forward declaration of helper function to delete specified event from the 
   list of events registered for the interface
---------------------------------------------------------------------------*/
static void
dsc_dcm_if_ev_info_del 
(
    dcm_iface_id_t if_id,
    dss_iface_ioctl_event_enum_type event,
    int dcm_nethandle
);

/*--------------------------------------------------------------------------- 
   Forward declaration of helper function to add specified event to the 
   list of events registered for the interface
---------------------------------------------------------------------------*/
static void
dsc_dcm_if_ev_info_add 
(
    dcm_iface_id_t if_id,
    dss_iface_ioctl_event_enum_type event,
    int dcm_nethandle
);

/*--------------------------------------------------------------------------- 
   Inline mutator for setting client handle for a given net handle
---------------------------------------------------------------------------*/
static __inline__ void
dsc_dcm_nh_set_clnt_hdl (int nh, int clnt_hdl)
{
    dsc_dcm_nh_info_parr[nh]->clnt_hdl = clnt_hdl;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting client handle for a given net handle
---------------------------------------------------------------------------*/
static __inline__ int
dsc_dcm_nh_get_clnt_hdl (int nh)
{
    return dsc_dcm_nh_info_parr[nh]->clnt_hdl;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting network callback function for a given handle
---------------------------------------------------------------------------*/
static __inline__ void
dsc_dcm_nh_set_net_cb (int nh, dcm_net_cb_fcn net_cb, void * net_cb_user_data)
{
    dsc_dcm_nh_info_parr[nh]->net_cb = net_cb;
    dsc_dcm_nh_info_parr[nh]->net_cb_user_data = net_cb_user_data;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting event callback function for a given handle
---------------------------------------------------------------------------*/
static __inline__ void
dsc_dcm_nh_set_ev_cb 
(
    int nh, 
    dcm_iface_ioctl_event_cb_fcn ev_cb, 
    void * ev_cb_user_data
)
{
    dsc_dcm_nh_info_parr[nh]->ev_cb = ev_cb;
    dsc_dcm_nh_info_parr[nh]->ev_cb_user_data = ev_cb_user_data;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting network policy for a given handle
---------------------------------------------------------------------------*/
static __inline__ void
dsc_dcm_nh_set_net_policy (int nh, dcm_net_policy_info_t * net_policy)
{
    dsc_dcm_nh_info_parr[nh]->net_policy = *net_policy;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting IFACE pointer for a given handle
---------------------------------------------------------------------------*/
static __inline__ void
dsc_dcm_nh_set_iface (int nh, dsc_dcm_iface_t * iface)
{
    dsc_dcm_nh_info_parr[nh]->iface_p = iface;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting IFACE pointer for a given handle
---------------------------------------------------------------------------*/
static __inline__ dsc_dcm_iface_t * 
dsc_dcm_nh_get_iface (int nh)
{
    return dsc_dcm_nh_info_parr[nh]->iface_p;
}

/*--------------------------------------------------------------------------- 
   Inline function for determining if handle is detached
---------------------------------------------------------------------------*/
static __inline__ int 
dsc_dcm_nh_is_detached (int nh)
{
    return (dsc_dcm_nh_info_parr[nh]->state == DSC_DCM_NH_STATE_DETACHED)
            ? 1 : 0;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting handle state to detached
---------------------------------------------------------------------------*/
static __inline__ void 
dsc_dcm_nh_set_detached (int nh)
{
    dsc_dcm_nh_info_parr[nh]->state = DSC_DCM_NH_STATE_DETACHED;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting IFACE pointer for a given IFACE ID
---------------------------------------------------------------------------*/
static __inline__ dsc_dcm_iface_t *
dsc_dcm_if_get_iface (dcm_iface_id_t if_id)
{
    dsc_log_high("dsc_dcm_if_get_name: ENTRY ");
    dsc_log_high( "input arg if_id [%d], ret [%x]", 
                  (int)if_id, (unsigned int)dsc_dcm_if_parr[if_id - DSC_DCM_IFACE_ID_OFFSET] );
    dsc_log_high("dsc_dcm_if_get_name: EXIT ");
    return dsc_dcm_if_parr[if_id - DSC_DCM_IFACE_ID_OFFSET];
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting the inet socket address structure to the 
   specified IPv4 address for a given IFACE
---------------------------------------------------------------------------*/
static __inline__ void
dsc_dcm_if_cfg_set_ipv4_addr 
(
    dsc_dcm_iface_t * iface, 
    ip_addr_type    * ipv4_addr
)
{
    //iface->if_cfg.ipv4_addr = *ipv4_addr;
    memcpy(&iface->if_cfg.ipv4_addr,ipv4_addr,sizeof(ip_addr_type));
}

/*---------------------------------------------------------------------------
   Inline mutator for setting the inet socket address structure to the
   specified gateway address for a given IFACE
---------------------------------------------------------------------------*/
static __inline__ void
dsc_dcm_if_cfg_set_ipv4_gateway_addr
(
    dsc_dcm_iface_t * iface,
    ip_addr_type    * gtwy_addr
)
{
    memcpy(&iface->if_cfg.ipv4_gateway_addr,gtwy_addr,sizeof(ip_addr_type));
}

/*---------------------------------------------------------------------------
   Inline mutator for setting the inet socket address structure to the
   specified primary dns address for a given IFACE
---------------------------------------------------------------------------*/
static __inline__ void
dsc_dcm_if_cfg_set_ipv4_prim_dns_addr
(
    dsc_dcm_iface_t * iface,
    ip_addr_type    * prim_dns_addr
)
{
    memcpy(&iface->if_cfg.ipv4_prim_dns_addr,prim_dns_addr,sizeof(ip_addr_type));
}

/*---------------------------------------------------------------------------
   Inline mutator for setting the inet socket address structure to the
   specified secondary dns address for a given IFACE
---------------------------------------------------------------------------*/
static __inline__ void
dsc_dcm_if_cfg_set_ipv4_seco_dns_addr
(
    dsc_dcm_iface_t * iface,
    ip_addr_type    * seco_dns_addr
)
{
    memcpy(&iface->if_cfg.ipv4_seco_dns_addr,seco_dns_addr,sizeof(ip_addr_type));
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting the IPv4 address of a given IFACE
---------------------------------------------------------------------------*/
static __inline__ ip_addr_type 
dsc_dcm_if_cfg_get_ipv4_addr (dsc_dcm_iface_t * iface)
{
    return iface->if_cfg.ipv4_addr;
}
/*--------------------------------------------------------------------------- 
    Radio Technology related Declarations
---------------------------------------------------------------------------*/
#define DSC_MAX_RADIO_IFS       6

static  dsc_nas_tech_type       dsc_dcm_radio_if[DSC_MAX_RADIO_IFS];  


/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_dcm_comp_int_func
===========================================================================*/
/*!
@brief
  General integer comparison function used for list operations.   

@return
  long - difference between the two arguments 

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static long int 
dsc_dcm_comp_int_func (const void * first, const void * second)
{
    long int f, s;

    f = (long int)first;
    s = (long int)second;

    return f - s;
}

/*===========================================================================
  FUNCTION  dsc_dcm_verify_if_op_tbl
===========================================================================*/
/*!
@brief
  Helper function used to verify that the iface operations table pointer 
  passed in is valid and all mandatory function callback pointers are set.

@return
  int - 0 if iface operations table is valid, -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static int 
dsc_dcm_verify_if_op_tbl (const dsc_dcm_if_op_tbl_t * if_op_tbl_p)
{
    if ((if_op_tbl_p == NULL) ||
        (if_op_tbl_p->if_match_f == NULL) ||
        (if_op_tbl_p->if_set_config_f == NULL) ||
        (if_op_tbl_p->if_up_cmd == NULL) ||
        (if_op_tbl_p->if_down_cmd == NULL))
    {
        return -1;
    }

    return 0;
}

/*===========================================================================
  FUNCTION  dsc_dcm_verify_if_event
===========================================================================*/
/*!
@brief
  Helper function used to verify that the specified event is a valid one.   

@return
  int - 0 if event is valid, -1 otherwise 

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_dcm_verify_if_event (dss_iface_ioctl_event_enum_type event)
{
    /* Check if specified event is in the valid range */
    if (((long)event < (long)DSS_IFACE_IOCTL_MIN_EV) ||
        ((long)event > (long)DSS_IFACE_IOCTL_EVENT_MAX))
    {
        return -1;
    }
    return 0;
}

/*===========================================================================
  FUNCTION  dsc_dcm_verify_nh
===========================================================================*/
/*!
@brief
  Helper function used to verify that the specified network handle is a 
  valid, currently allocated handle.   

@return
  int - 0 if handle is valid, -1 otherwise 

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static int 
dsc_dcm_verify_nh (int nh)
{
    int rval = -1;

    /* Range check */
    if ((nh < 0) || (nh >= DSC_MAX_NH)) {
        goto error;
    }

    /* Verify network handle has been allocated */
    if (dsc_dcm_nh_info_parr[nh] == NULL) {
        goto error;
    }

    /* Double check net handle id */
    ds_assert(dsc_dcm_nh_info_parr[nh]->nh_id == nh);
    rval = 0;

error:
    return rval;
}

/*===========================================================================
  FUNCTION  dsc_dcm_nh_ev_info_init
===========================================================================*/
/*!
@brief
  Initializes the lists of events registered for notification for a given
  client.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_nh_ev_info_init (dsc_dcm_nh_info_t * nh_info)
{
    int i;
    ds_dll_el_t * head;
    dsc_dcm_nh_ev_info_t * ev_info;

    /* Iterate over all events and initialize list for each one */
    for (i = 0; i < (int)DSS_IFACE_IOCTL_REG_EVENT_MAX; ++i)
    {
        ev_info = &nh_info->ev_info[i];

        if ((head = ds_dll_init(NULL)) == NULL) {
            dsc_log_err("ds_dll_init failed in ds_dcm_nh_ev_info_init");
            dsc_abort();
            return;
        }

        /* Initialize list head and tail ptrs */
        ev_info->l_head = head;
        ev_info->l_tail = head;
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_nh_ev_info_free
===========================================================================*/
/*!
@brief
  Deregisters all events currently registered for by a given client and 
  deallocates/frees all data structures used in event handling for that
  client.

@return
  void

@note

  - Dependencies
    - Requires that data structures had been initialized using the 
      dsc_dcm_nh_ev_info_init() routine.  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_nh_ev_info_free (dsc_dcm_nh_info_t * nh_info)
{
    int i;
    ds_dll_el_t * node;
    dsc_dcm_nh_ev_info_t * ev_info;
    const void * data;
    dcm_iface_id_t if_id;

    /* Iterate over all events and deallocate memory for each list */
    for (i = 0; i < (int)DSS_IFACE_IOCTL_REG_EVENT_MAX; ++i)
    {
        ev_info = &nh_info->ev_info[i];

        ds_assert(ev_info->l_head);
        ds_assert(ev_info->l_tail);

        /* Iterate over the list, deallocating each element */
        while ((node = ds_dll_deq(ev_info->l_head, &ev_info->l_tail, &data)))
        {
            if_id = (dcm_iface_id_t)data;

            /* Delete event from the per-iface event data structures also */
            dsc_dcm_if_ev_info_del
            (
                if_id,
                (dss_iface_ioctl_event_enum_type)i, 
                nh_info->nh_id
            );

            /* Free memory for the node */
            ds_dll_free(node);
        }

        /* Destroy the list once each element has been freed */
        ds_dll_destroy(ev_info->l_head);
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_nh_ev_add
===========================================================================*/
/*!
@brief
  Adds an event to the list of registered events for a given network handle 
  and IFACE. 

@return
  int - 0 if succesful, -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_dcm_nh_ev_add 
(
    int nh, 
    dcm_iface_id_t if_id, 
    dcm_iface_ioctl_event_name_t event
)
{
    ds_dll_el_t * node;
    dsc_dcm_nh_ev_info_t * ev_info;
    dsc_dcm_nh_info_t * nh_info;

    if(nh < 0 || nh >= DSC_MAX_NH)
    {
      dsc_log_err("Invalid net Handle %d",nh);
      return -1;
    }
    nh_info = dsc_dcm_nh_info_parr[nh];
    ds_assert(nh_info);
    ds_assert(nh == nh_info->nh_id);
    
    if((int)event >= DSS_IFACE_IOCTL_REG_EVENT_MAX)
    {
        dsc_log_err("dsc_dcm_nh_ev_add: event %d not valid", event);
        return -1;
    }
    ev_info = &nh_info->ev_info[(int)event];

    /* Search if event is already registered for on this iface */
    node = ds_dll_search
    (
        ev_info->l_head, 
        (void *)if_id, 
        dsc_dcm_comp_int_func
    ); 

    if (node) {
        /* Event is already registered, so no need to do anything else */
        dsc_log_err("dsc_dcm_nh_ev_add: event %d already registered for nh %d, if %d",
                    event, nh, (int)if_id);
        return 0;
    }

    /* Event is not registered, so add event to the list */
    node = ds_dll_enq(ev_info->l_tail, NULL, (void *)if_id);
    ds_assert(node);
    ev_info->l_tail = node;

    /* Also add event to the per-iface list of registered events */
    dsc_dcm_if_ev_info_add(if_id, event, nh);

    return 0;
}

/*===========================================================================
  FUNCTION  dsc_dcm_nh_ev_del
===========================================================================*/
/*!
@brief
  Deletes an event from the list of registered events for a given network 
  handle and IFACE. 

@return
  int - 0 if successful, -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_dcm_nh_ev_del 
(
    int nh, 
    dcm_iface_id_t if_id, 
    dcm_iface_ioctl_event_name_t event
)
{
    ds_dll_el_t * node;
    dsc_dcm_nh_ev_info_t * ev_info;
    dsc_dcm_nh_info_t * nh_info;

    if(nh < 0 || nh >= DSC_MAX_NH)
    {
      dsc_log_err("Invalid net Handle %d",nh);
      return -1;
    }
    nh_info = dsc_dcm_nh_info_parr[nh];
    ds_assert(nh_info);
    ds_assert(nh == nh_info->nh_id);

    if((int)event >= DSS_IFACE_IOCTL_REG_EVENT_MAX)
    {
        dsc_log_err("dsc_dcm_nh_ev_add: event %d not valid", event);
        return -1;
    }
    ev_info = &nh_info->ev_info[(int)event];

    /* Delete iface entry from list of ifaces that this event was registered 
    ** on. 
    */
    node = ds_dll_delete
    (
        ev_info->l_head,
        &ev_info->l_tail,
        (void *)if_id,
        dsc_dcm_comp_int_func
    );

    if (node) {
        ds_dll_free(node);
    } else {
        /* App tried event deregistration without registering. Return error.. */
        dsc_log_err("dsc_dcm_nh_ev_del: event %d not registered for nh %d, if %d",
                    event, nh, (int)if_id);
        return -1;
    }

    /* Delete event from the per-iface list of events registered for */
    dsc_dcm_if_ev_info_del(if_id, event, nh);

    return 0;
}

/*===========================================================================
  FUNCTION  dsc_dcm_alloc_nh
===========================================================================*/
/*!
@brief
  Allocates a network handle and initializes all associated data structures. 

@return
  int - handle value if allocation is successful, -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_dcm_alloc_nh (int clnt_hdl)
{
    int i;
    dsc_dcm_nh_info_t * nh_info;
    (void)clnt_hdl;
    
    /* Iterate over the array of network handle pointers, and find an unused
    ** entry.
    */
    for (i = 0; i < DSC_MAX_NH; ++i) {
        if (dsc_dcm_nh_info_parr[i] == NULL) {
            break;
        }
    }

    if (i >= DSC_MAX_NH) {
        /* None available, return error */
        dsc_log_err("Cannot alloc nethandle - limit reached");
        return -1;
    }

    /* Found unused entry. Allocate memory for network handle */
    if ((nh_info = dsc_malloc(sizeof(dsc_dcm_nh_info_t))) == NULL) {
        dsc_log_err("Cannot alloc memory for nethandle");
        dsc_abort();
    }

    /* Initialize memory */
    memset(nh_info, 0, sizeof(dsc_dcm_nh_info_t));

    /* Store handle info ptr */
    dsc_dcm_nh_info_parr[i] = nh_info;

    /* For debug purposes, save handle id */
    nh_info->nh_id = i;

    /* Initialize handle state to active */
    nh_info->state = DSC_DCM_NH_STATE_ACTIVE;

    /* Initialize event management data structures for network handle */
    dsc_dcm_nh_ev_info_init(nh_info);

    return i;
}

/*===========================================================================
  FUNCTION  dsc_dcm_release_nh
===========================================================================*/
/*!
@brief
  Frees a network handle and deallocates all associated data structures. 

@return
  void

@note

  - Dependencies
    - Handle must have been allocated previously. 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_release_nh (int nh)
{
    dsc_dcm_nh_info_t * nh_info;

    dsc_log_low("dsc_dcm_release_nh called for nh %d", nh);

    if(nh < 0 || nh >= DSC_MAX_NH)
    {
      dsc_log_err("Invalid net Handle %d",nh);
      return;
    }
    nh_info = dsc_dcm_nh_info_parr[nh];
    ds_assert(nh_info);
    ds_assert(nh == nh_info->nh_id);

    /* Free event management data structures for network handle */
    dsc_dcm_nh_ev_info_free(nh_info);

    /* Free memory allocated for network handle info */
    dsc_free(nh_info);

    /* Reset entry in handle pointer array */
    dsc_dcm_nh_info_parr[nh] = NULL;

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_nh_call_net_cb
===========================================================================*/
/*!
@brief
  Calls the registered network callback of the specified client. 

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void 
dsc_dcm_nh_call_net_cb (int nh, dcm_iface_id_t if_id, int net_errno)
{
    dsc_dcm_nh_info_t * nh_info;

    dsc_log_func_entry();

    /* Don't call network callback if client has exited, as otherwise 
    ** the RPC call will fail.
    */
    if (dsc_dcm_nh_is_detached(nh)) {
        goto error;
    }

    nh_info = dsc_dcm_nh_info_parr[nh];

    /* Call client's network callback */
    (* nh_info->net_cb)
    (
        nh, 
        (dcm_iface_id_t) (if_id), 
        net_errno, 
        nh_info->net_cb_user_data
    );

error:
    dsc_log_func_exit();
    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_nh_call_ev_cb
===========================================================================*/
/*!
@brief
  Calls the registered event callback of the specified client. 

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_nh_call_ev_cb
(
    int                       nh, 
    dcm_iface_id_t            if_id,
    dcm_iface_ioctl_event_t * event
)
{
    dsc_dcm_nh_info_t * nh_info;
    dsc_dcm_nh_ev_info_t * ev_info;
    ds_dll_el_t * node;

    dsc_log_func_entry();

    ds_assert(dsc_dcm_verify_nh(nh) == 0);

    /* Don't call event callback if client has exited, as otherwise 
    ** the RPC call will fail.
    */
    if (dsc_dcm_nh_is_detached(nh)) {
        goto error;
    }

    nh_info = dsc_dcm_nh_info_parr[nh];
    ev_info = &nh_info->ev_info[(int)(event->name)];

    /* For debug purposes, check that the iface is registered for for this 
    ** event.
    */
    node = ds_dll_search
           (
               ev_info->l_head,
               (void *)if_id,
               dsc_dcm_comp_int_func
           );

    ds_assert(node);

    /* Call client's event callback */
    (* nh_info->ev_cb)
	(
		nh, 
		(dcm_iface_id_t) (if_id), 
		event, 
		nh_info->ev_cb_user_data
	);

error:
    dsc_log_func_exit();
    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_verify_if_id
===========================================================================*/
/*!
@brief
  Helper function for verifying the validity of the specified IFACE ID. 

@return
  int - 0 if IFACE ID is valid, -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static int 
dsc_dcm_verify_if_id (dcm_iface_id_t if_id)
{
    int rval = -1;
    unsigned long if_idx;

    /* Verify iface id is not invalid */
    if (if_id == DCM_IFACE_ID_INVALID) {
        goto error;
    }

    /* Constant offset subtraction to get the iface ptr array index */
    if_idx = if_id - DSC_DCM_IFACE_ID_OFFSET;

    /* Range check */
    if (if_idx >= DSC_MAX_IFACE) {
        goto error;
    }
    
    /* Ensure that the iface ptr is not null */
    if (dsc_dcm_if_parr[if_idx] == NULL) {
        goto error;
    }

    /* Double check that the iface id matches */
    ds_assert(dsc_dcm_if_parr[if_idx]->if_id == if_id);

    rval = 0;

error:
    return rval;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_nh_info_init
===========================================================================*/
/*!
@brief
  Initializes the list for managing network handles bound to a given IFACE. 

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_if_nh_info_init (dsc_dcm_if_nh_info_t * if_nh_info)
{
    ds_dll_el_t * head; 

    /* Initialize list */
    if ((head = ds_dll_init(NULL)) == NULL) {
        dsc_log_err("ds_dll_init failed in ds_dcm_if_nh_info_init");
        dsc_abort();
        return;
    }

    /* Initialize list head and tail ptrs */
    if_nh_info->l_head = head;
    if_nh_info->l_tail = head;

    /* Initialize number of elements in list */
    if_nh_info->n_elem = 0;

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_add_nh
===========================================================================*/
/*!
@brief
  Adds the specified network handle to the list of handles bound to an IFACE. 

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_if_add_nh (dsc_dcm_iface_t * iface, int nh)
{
    ds_dll_el_t * node;
    dsc_dcm_if_nh_info_t * if_nh_info;

    ds_assert(iface);
    if_nh_info = &iface->if_nh_info;

    /* Add net handle to list of net handles for iface */
    node = ds_dll_enq(if_nh_info->l_tail, NULL, (void *)nh);
    ds_assert(node);

    if_nh_info->l_tail = node;

    /* Increment number of elements in list */
    ++if_nh_info->n_elem;

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_del_nh
===========================================================================*/
/*!
@brief
  Deletes the specified network handle from the list of handles bound to an
  IFACE. 

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_if_del_nh (dsc_dcm_iface_t * iface, int nh)
{
    ds_dll_el_t * node;
    dsc_dcm_if_nh_info_t * if_nh_info;

    ds_assert(iface);
    if_nh_info = &iface->if_nh_info;

    /* Delete handle from list of net handles for iface */
    node = ds_dll_delete
    (
        if_nh_info->l_head,
        &if_nh_info->l_tail,
        (void *)nh,
        dsc_dcm_comp_int_func
    );

    ds_assert(node);
    ds_dll_free(node);

    /* Decrement the number of elements in list */
    --if_nh_info->n_elem;

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_call_net_cbs
===========================================================================*/
/*!
@brief
  Calls the network callbacks of all network handles bound to the specified
  IFACE. 

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_if_call_net_cbs (dsc_dcm_iface_t * iface, int net_errno)
{
    ds_dll_el_t * node;
    dsc_dcm_if_nh_info_t * if_nh_info;
    dcm_iface_id_t if_id;
    const void * nh;

    ds_assert(iface);
    if_nh_info = &iface->if_nh_info;
    if_id = iface->if_id;

    /* Get head of list of net handles bound to iface */
    node = if_nh_info->l_head;

    /* Iterate over the list, calling net callback for each net handle */
    while ((node = ds_dll_next(node, &nh))) {
        dsc_dcm_nh_call_net_cb((int)nh, if_id, net_errno);
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_del_all_nh
===========================================================================*/
/*!
@brief
  Removes all network handles from the list of handles bound to the given 
  IFACE. 

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_if_del_all_nh (dsc_dcm_iface_t * iface)
{
    ds_dll_el_t * node = NULL;
    dsc_dcm_if_nh_info_t * if_nh_info;
    const void * nh;

    ds_assert(iface);
    if_nh_info = &iface->if_nh_info;

    /* Iterate over the list of net handles, removing each one */
    while ((node = ds_dll_deq(if_nh_info->l_head, &if_nh_info->l_tail, &nh)))
    {
        /* Set iface ptr for net handle to null */
        dsc_dcm_nh_set_iface((int)nh, NULL);

        /* Deallocate memory for the node */
        ds_dll_free(node);
    }

    /* Set number of elements in list to zero */
    if_nh_info->n_elem = 0;

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_ev_info_init
===========================================================================*/
/*!
@brief
  Initializes the lists of events registered for a given IFACE. 

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_if_ev_info_init (dsc_dcm_iface_t * iface)
{
    ds_dll_el_t * head; 
    int i;
    dsc_dcm_if_ev_info_t * if_ev_info;

    /* For each event, initialize the list of network handles that have 
    ** registered for event notification. 
    */
    for (i = 0; i < (int)DSS_IFACE_IOCTL_REG_EVENT_MAX; ++i)
    {
        if_ev_info = &iface->if_ev_info[i];

        /* Initialize the list of network handles for this event */
        if ((head = ds_dll_init(NULL)) == NULL) {
            dsc_log_err("ds_dll_init failed in ds_dcm_if_ev_info_init");
            dsc_abort();
            return;
        }

        /* Initialize list head and tail pointers */
        if_ev_info->l_head = head;
        if_ev_info->l_tail = head;
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_ev_info_add
===========================================================================*/
/*!
@brief
  Adds an event to the list of events registered for notification by a 
  given client for the specified IFACE. 

@return
  void

@note

  - Dependencies
    - if_id, event and nethandle must be valid

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_if_ev_info_add 
(
    dcm_iface_id_t if_id,
    dss_iface_ioctl_event_enum_type event,
    int dcm_nethandle
)
{
    dsc_dcm_iface_t * iface;
    ds_dll_el_t * node;
    dsc_dcm_if_ev_info_t * if_ev_info;

    iface = dsc_dcm_if_get_iface(if_id);
    if_ev_info = &iface->if_ev_info[(int)event];

    /* Search if event is already registered for on this iface */
    node = ds_dll_search
    (
        if_ev_info->l_head, 
        (void *)dcm_nethandle, 
        dsc_dcm_comp_int_func
    ); 

    if (node) {
        /* Event is already registered. Print debug message and return */
        dsc_log_err("dsc_dcm_if_ev_info_add: event %d already registered for nh %d",
                    event, dcm_nethandle);
        return;
    }

    /* Event not previously registered for. Add net handle to the list of 
    ** net handles that have registered for this event on this iface.
    */
    node = ds_dll_enq(if_ev_info->l_tail, NULL, (void *)dcm_nethandle);
    ds_assert(node);
    if_ev_info->l_tail = node;

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_ev_info_del
===========================================================================*/
/*!
@brief
  Deletes an event from the list of events registered for notification by a 
  given client for the specified IFACE. 

@return
  void

@note

  - Dependencies
    - if_id, event and nethandle must be valid

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_if_ev_info_del 
(
    dcm_iface_id_t if_id,
    dss_iface_ioctl_event_enum_type event,
    int dcm_nethandle
)
{
    dsc_dcm_iface_t * iface;
    ds_dll_el_t * node;
    dsc_dcm_if_ev_info_t * if_ev_info;

    iface = dsc_dcm_if_get_iface(if_id);
    if_ev_info = &iface->if_ev_info[(int)event];

    /* Delete net handle from the list of handles registered for this event
    ** on this iface. 
    */
    node = ds_dll_delete
    (
        if_ev_info->l_head,
        &if_ev_info->l_tail,
        (void *)dcm_nethandle,
        dsc_dcm_comp_int_func
    );

    if (node) {
        /* Deallocate memory for the list node */
        ds_dll_free(node);
    } else {
        dsc_log_err("dsc_dcm_if_ev_info_del: event %d not registered for nh %d",
                    event, dcm_nethandle);
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_clnt_nh_add
===========================================================================*/
/*!
@brief
  Adds net handle to the list of net handles for the client handle.

@return
  void

@note

  - Dependencies
    - Client handle and net handle must be valid

  - Side Effects
    - None
*/
/*=========================================================================*/
void
dsc_dcm_clnt_nh_add (int clnt_hdl, int nh)
{
    ds_dll_el_t * node;
    dsc_dcm_clnt_info_t * clnt_info;

    /* Get pointer to client handle info blob */
    clnt_info = dsc_dcm_clnt_info_parr[clnt_hdl];
    ds_assert(clnt_info);

    /* Add net handle to the list of net handles for this client */
    node = ds_dll_enq(clnt_info->l_tail, NULL, (void *)nh);
    ds_assert(node);
    clnt_info->l_tail = node;

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_clnt_nh_del
===========================================================================*/
/*!
@brief
  Deletes net handle from the list of net handles for the client handle.

@return
  void

@note

  - Dependencies
    - Client handle and net handle must be valid

  - Side Effects
    - None
*/
/*=========================================================================*/
void
dsc_dcm_clnt_nh_del (int clnt_hdl, int nh)
{
    ds_dll_el_t * node;
    dsc_dcm_clnt_info_t * clnt_info;

    clnt_info = dsc_dcm_clnt_info_parr[clnt_hdl];
    ds_assert(clnt_info);

    /* Delete net handle from the list of net handles for this client */
    node = ds_dll_delete
    (
        clnt_info->l_head,
        &clnt_info->l_tail,
        (void *)nh,
        dsc_dcm_comp_int_func
    );

    ds_assert(node);

    /* Deallocate memory for the node */
    ds_dll_free(node);

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_verify_clnt_hdl
===========================================================================*/
/*!
@brief
  Verifies that client handle is valid and exists.

@return
  int - 0, if handle is valid, -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static int 
dsc_dcm_verify_clnt_hdl (int clnt_hdl)
{
    int rval = -1;

    /* Range check */
    if ((clnt_hdl < 0) || (clnt_hdl >= DSC_MAX_CLNT)) {
        goto error;
    }

    /* Verify that the client handle pointer is valid */
    if (dsc_dcm_clnt_info_parr[clnt_hdl] == NULL) {
        goto error;
    }

    /* Double check that client handle id matches */
    ds_assert(dsc_dcm_clnt_info_parr[clnt_hdl]->clnt_id == clnt_hdl);
    rval = 0;

error:
    return rval;
}

/*===========================================================================
  FUNCTION  dsc_dcm_alloc_clnt_hdl
===========================================================================*/
/*!
@brief
  Allocates a client handle and initializes all associated data structures. 

@return
  int - handle value if allocation is successful, -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_dcm_alloc_clnt_hdl (void)
{
    int i;
    dsc_dcm_clnt_info_t * clnt_info;
    ds_dll_el_t * head;

    /* Find an unused entry in the client info ptr array */
    for (i = 0; i < DSC_MAX_CLNT; ++i) {
        if (dsc_dcm_clnt_info_parr[i] == NULL) {
            break;
        }
    }

    if (i >= DSC_MAX_CLNT) {
        /* No unused entry exists. Return error */
        dsc_log_err("Cannot alloc client handle - limit reached");
        return -1;
    }

    /* Allocate memory for the client info blob */
    if ((clnt_info = dsc_malloc(sizeof(dsc_dcm_clnt_info_t))) == NULL) {
        dsc_log_err("Cannot alloc memory for client handle");
        dsc_abort();
    }

    /* Initialize memory */
    memset(clnt_info, 0, sizeof(dsc_dcm_clnt_info_t));

    /* Set pointer to client info blob in array */
    dsc_dcm_clnt_info_parr[i] = clnt_info;

    /* For debug purposes, record client id in the client info blob */
    clnt_info->clnt_id = i;
    
    /* Initialize the list of network handles for this client */
    if ((head = ds_dll_init(NULL)) == NULL) {
        dsc_log_err("ds_dll_init failed in dsc_dcm_alloc_clnt_hdl");
        dsc_abort();
    }

    /* Initialize the list head and tail pointers */
    clnt_info->l_head = head;
    clnt_info->l_tail = head;

    return i;
}

/*===========================================================================
  FUNCTION  dsc_dcm_match_iface_name
===========================================================================*/
/*!
@brief
  Helper function for determining if the specified IFACE is of a matching 
  kind given the specified name or group names. 

@return
  int - 0 if IFACE matches the specified kind, -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_dcm_match_iface_name 
(
    const dsc_dcm_iface_t * iface,
    dcm_iface_id_name_t if_name
)
{
    dcm_iface_id_name_t   temp_if_name = 0;

    if (dsc_dcm_radio_if[0] == DSC_NAS_UMTS_TECH || dsc_dcm_radio_if[0] == DSC_NAS_GSM_TECH) {
      temp_if_name = UMTS_IFACE;
    } else if (dsc_dcm_radio_if[0] == DSC_NAS_CDMA200_TECH ||
                      dsc_dcm_radio_if[0] == DSC_NAS_CDMA_HRPD_TECH) {
      temp_if_name = CDMA_SN_IFACE;
    }

    /* Iface name passed in can be either a name or a group. Name must 
    ** exactly match the passed in iface's name, for it to be a match. If 
    ** group, need to match this against all groups that the iface belongs
    ** to. 
    */
    if ((((long)if_name & (long)IFACE_MASK) && 
         ((long)iface->if_name == (long)if_name)) ||
        ((!((long)if_name & (long)IFACE_MASK)) && 
		 ((long)iface->if_group & (long)if_name)))
    {
        if ((!((long)if_name & (long)IFACE_MASK)) && 
                  ((long)iface->if_name != (long)temp_if_name)) {
          dsc_log_high("dsc_dcm_match_iface_name:Iface %x Not Selected",iface->if_name);
          return -1;
        }
        dsc_log_high("dsc_dcm_match_iface_name:Iface %x selected ",iface->if_name);
        return 0;
    }

    return -1;
}

/*===========================================================================
  FUNCTION  dsc_dcm_get_iface_for_policy
===========================================================================*/
/*!
@brief
  Returns pointer to an IFACE compatible with the specified network policy.  

@return
  dsc_dcm_iface_t * - pointer to IFACE if a matching IFACE exists, 
                      NULL otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static dsc_dcm_iface_t * 
dsc_dcm_get_iface_for_policy (dcm_net_policy_info_t * policy)
{
    dsc_dcm_iface_t * iface = NULL;
    dsc_dcm_iface_t * iface_unpref = NULL;
    int i;
    dsc_op_status_t result;
    dsc_nas_radio_tech_info     radio_info;
    /* Iterate over all ifaces and find the one that best matches the 
    ** specified policy.
    */
    if(dsc_nas_query_technology(&radio_info) < 0)
    {
      dsc_log_err("Nas Query returned Error \n");
      return NULL;
    } 
    
    for(i = 0; i < radio_info.num_radio_ifaces; i++)
      dsc_dcm_radio_if[i] = radio_info.radio_if[i];

    /* No Service: Return NULL IFACE 
       This is a temporary Solution.
       TODO: Find the Preffered technology and select
       preffered technology iface.*/

    if (dsc_dcm_radio_if[0] == DSC_NAS_NO_SRVC_TECH)
    {
      dsc_log_err("Nas Query retuned with NO SRVC technology type. \n");
      return NULL;
    }
    
    dsc_log_err(" =======> Tech type = %x \n",dsc_dcm_radio_if[0]);

    for (i = 0; i < DSC_MAX_IFACE; ++i) {
        dsc_log_high( "finding IFACE" );
        /* If iface pointer is null then ignore this entry */
        if ((iface = dsc_dcm_if_parr[i]) == NULL) {
            continue;
        }

        /* Ignore iface if specified iface id or name/group does not match */
        if (policy->iface.kind == DSS_IFACE_ID) {
            if (iface->if_id != policy->iface.info.id) {
                continue;
            }
        } else {
            if (dsc_dcm_match_iface_name(iface, policy->iface.info.name) != 0) {
                continue;
            }
        }

        /* If policy is UP_ONLY, then ignore ifaces that are not up */
        if ((iface->if_state != IFACE_UP) && 
            (policy->policy_flag == DSS_IFACE_POLICY_UP_ONLY))
        {
            continue;
        }

        /* Call lower layer function to compare iface configuration with 
        ** specified configuration in policy. Not needed if iface is down.
        */
        if (iface->if_state == IFACE_DOWN) {
            result = DSC_OP_SUCCESS;
        } else {
            result = (* (iface->if_op_tbl.if_match_f))
                        (iface->if_id, iface->call_hdl, policy);
        }

        /* Process result based on policy flag. Immediately break out of 
        ** loop if best matching iface is found. Continue to next iteration 
        ** of loop if iface does not match. Otherwise proceed. 
        */
        if (policy->policy_flag == DSS_IFACE_POLICY_UP_ONLY) 
        {
            /* If UP_ONLY, select iface if configuration matches. If it doesn't,
            ** iface is an unpreferred iface (as iface cannot be down).
            */
            if (result == DSC_OP_SUCCESS) {
                break;
            }
        } else if (policy->policy_flag == DSS_IFACE_POLICY_UP_PREFERRED) 
        {
            /* If UP_PREF, select iface if iface is up and configuration 
            ** matches. Reject iface if iface is down and configuration 
            ** doesn't match. Otherwise, iface is an unpreferred iface, as 
            ** either iface is down or configuration doesn't match. 
            */
            if ((result == DSC_OP_SUCCESS) && (iface->if_state == IFACE_UP)) {
                break;
            } else if ((result == DSC_OP_FAIL) && 
                       (iface->if_state == IFACE_DOWN))
            {
                continue;
            }
        } else 
        {
            /* If ANY, select iface if configuration matches and iface is up. 
            ** If configuration doesn't match, reject iface. If configuration 
            ** matches but iface is down, iface is unpreferred. 
            */
            if (result == DSC_OP_FAIL) {
                continue;
            } else if (iface->if_state == IFACE_UP) {
                break;
            }
        }

        /* If we reach here, the current iface is an unpreferred iface. Store
        ** unpreferred iface pointer, in case we can't find a better match. 
        ** Note the special case for UP_PREF, as we need to give preference 
        ** to an iface that is up over a down iface (with matching config).
        */
        if ((!iface_unpref) || 
            ((policy->policy_flag == DSS_IFACE_POLICY_UP_PREFERRED) && 
             (iface_unpref->if_state != IFACE_UP) && 
             (iface->if_state == IFACE_UP)))
        {
            iface_unpref = iface;
        }
    }

    /* Iface selection complete. If the index into the iface array is overflown,
    ** we could not find a best match. Select the unpreferred iface in this 
    ** case. 
    */
    if (i >= DSC_MAX_IFACE) {
        if (iface_unpref) {
            iface = iface_unpref;
        } else {
            iface = NULL;
        }
    }   
    dsc_log_high( "dsc_dcm_get_iface_for_policy:IFACE [%x]", (unsigned int)iface );
    if (iface) {
        /* Found a matching iface. If this iface is down, set configuration 
        ** of iface.
        */
        if (iface->if_state == IFACE_DOWN) {
            dsc_log_high( "IFACE [%x] is down, calling if_set_config_f at [%x]", 
                          (unsigned int)iface, (unsigned int)iface->if_op_tbl.if_set_config_f );
            result = (* (iface->if_op_tbl.if_set_config_f))
                        (iface->if_id, iface->call_hdl, policy);
            if (result == DSC_OP_FAIL) {
                dsc_log_err("set_config failed for iface %ld", 
                            iface->if_id);
                iface = NULL;
            }
        }
    }
    /* Return selected iface (or NULL if none selected) */
    dsc_log_func_exit();
    return iface;
}

/*===========================================================================
  FUNCTION  dss_iface_ioctl_reg_dereg_event_cb
===========================================================================*/
/*!
@brief
  Helper function for registration/deregistration of an event for 
  client notification in response to an IOCTL call.  

@return
  int - 0 if successful, -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dss_iface_ioctl_reg_dereg_event_cb
(
    dcm_iface_ioctl_name_t          ioctl_name,
    dcm_iface_id_t                  if_id,
    dcm_iface_ioctl_event_name_t    event_name,
    int                             nh
)
{
    int rval = -1;

    /* Add or delete event for network handle depending on the ioctl type */
    if (ioctl_name == DSS_IFACE_IOCTL_REG_EVENT_CB) {
        rval = dsc_dcm_nh_ev_add(nh, if_id, event_name);
    } else if (ioctl_name == DSS_IFACE_IOCTL_DEREG_EVENT_CB) {
        rval = dsc_dcm_nh_ev_del(nh, if_id, event_name);
    } else {
        dsc_abort();
    }

    return rval;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_call_ev_cbs
===========================================================================*/
/*!
@brief
  Calls the client event notification callbacks for notifying clients of 
  an event on the specified IFACE. 

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_if_call_ev_cbs
(
    dsc_dcm_iface_t         * iface,
    dcm_iface_ioctl_event_t * event
)
{
    ds_dll_el_t * node;
    const void * data;
    int nh;
    dsc_dcm_if_ev_info_t * if_ev_info;

    /* Get iface event info blob ptr */
    if_ev_info = &iface->if_ev_info[(int)(event->name)];

    /* Get head of list of network handles that have registered for this 
    ** event.
    */
    node = if_ev_info->l_head;

    /* Iterate over the list, calling event callback for each net handle */
    while ((node = ds_dll_next(node, &data))) {
        nh = (int)data;
        dsc_dcm_nh_call_ev_cb(nh, iface->if_id, event);
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_call_if_state_ev_cbs
===========================================================================*/
/*!
@brief
  Calls the client event notification callbacks for notifying clients of 
  an IFACE state change event for the specified IFACE. 

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_if_call_if_state_ev_cbs
(
    dsc_dcm_iface_t * iface, 
    dcm_iface_state_t curr_if_state, 
    dcm_iface_state_t prev_if_state
)
{
    dcm_iface_ioctl_event_t event;

    /* Don't call event callback if there is no state change */
    if (curr_if_state == prev_if_state) {
        return;
    }

    /* Set event name based on current iface state */
    switch (curr_if_state) {
    case IFACE_UP:
        event.name = DSS_IFACE_IOCTL_UP_EV;
        break;
    case IFACE_DOWN:
        event.name = DSS_IFACE_IOCTL_DOWN_EV;
        break;
    case IFACE_COMING_UP:
        event.name = DSS_IFACE_IOCTL_COMING_UP_EV;
        break;
    case IFACE_GOING_DOWN:
        event.name = DSS_IFACE_IOCTL_GOING_DOWN_EV;
        break;
    default:
        return;
    }

    event.info.iface_state_info = curr_if_state;

    /* Call all event callbacks registered for this iface */
    dsc_dcm_if_call_ev_cbs(iface, &event);

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_call_physlink_state_ev_cbs
===========================================================================*/
/*!
@brief
  Calls the client event notification callbacks for notifying clients of 
  an IFACE state change event for the specified IFACE and the specified event. 

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_if_call_physlink_state_ev_cbs
(
    dsc_dcm_iface_t * iface, 
    dcm_iface_state_t curr_if_state, 
    int               dorm_status
)
{
  dcm_iface_ioctl_event_t event;

  dsc_log_func_entry();

  /* Set event name based on dormancy status */
    dsc_log_high("dsc_dcm_if_call_physlink_state_ev_cbs:dorm_status is%d\n",dorm_status);
    switch (dorm_status) {
      case DSC_DCM_PHYSLINK_DORMANT:
        event.name = DSS_IFACE_IOCTL_PHYS_LINK_DOWN_EV;
        break;
      case DSC_DCM_PHYSLINK_ACTIVE:
        event.name = DSS_IFACE_IOCTL_PHYS_LINK_UP_EV;
        break;
      default:
        {
          dsc_log_high(" Invalid Dorm Status Received %d\n",dorm_status);
          goto error;
        }
    }

    event.info.iface_state_info = curr_if_state;

    /* Call all event callbacks registered for this iface */
    dsc_dcm_if_call_ev_cbs(iface, &event);
    dsc_log_func_exit();
error:
  return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_call_ll_ioctl
===========================================================================*/
/*!
@brief
  Calls the virtual function registered by lower layers for processing 
  IFACE IOCTLs.

@return
  dsc_op_status_t - DSC_OP_SUCCESS if IOCTL was handled successfully, 
                    DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static dsc_op_status_t
dsc_dcm_if_call_ll_ioctl 
(
    dsc_dcm_iface_t * iface, 
    dsc_dcm_iface_ioctl_t * ioctl
)
{
    dsc_op_status_t status;
    dcm_iface_id_t if_id;
    void * call_hdl;

    if_id = iface->if_id;

    if (iface->if_op_tbl.if_ioctl_f == NULL) {
        dsc_log_err("Iface id %d has null ioctl handler", (int)if_id);
        return DSC_OP_FAIL;
    }

    call_hdl = iface->call_hdl;

    /* Call registered lower layer ioctl handler */
    status = (* iface->if_op_tbl.if_ioctl_f)(if_id, call_hdl, ioctl);
    return status;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_get_ipv4_addr
===========================================================================*/
/*!
@brief
  Helper function that calls the lower layer IOCTL to get the IPv4 address
  assigned to the specified IFACE and sets it in the IFACE's control blob. 

@return
  void

@note

  - Dependencies
    - IFACE must be in the UP state and must have an IPv4 address assigned.

  - Side Effects
    - None
*/
/*=========================================================================*/
static void 
dsc_dcm_if_get_ipv4_addr (dsc_dcm_iface_t * iface)
{
    dsc_dcm_iface_ioctl_t ioctl;

    ioctl.name = DSS_IFACE_IOCTL_GET_IPV4_ADDR;

    /* Call lower layer ioctl to get ipv4 address */
    if (dsc_dcm_if_call_ll_ioctl(iface, &ioctl) == DSC_OP_FAIL) {
        dsc_log_err("dsc_dcm_if_get_ipv4_addr: ll_ioctl failed");
        return;
    }

    /* Store ipv4 address in the configuration blob */
    dsc_dcm_if_cfg_set_ipv4_addr(iface, &ioctl.info.ipv4_addr);
    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_get_gateway_addr
===========================================================================*/
/*!
@brief
  Helper function that calls the lower layer IOCTL to get the gateway address
  assigned to the specified IFACE and sets it in the IFACE's control blob.

@return
  void

@note

 - Dependencies
   - IFACE must be in the UP state and must have an gateway address assigned.

 - Side Effects
   - None
*/
/*=========================================================================*/
static void
dsc_dcm_if_get_ipv4_gateway_addr (dsc_dcm_iface_t * iface)
{
    dsc_dcm_iface_ioctl_t ioctl;

    ioctl.name = DSS_IFACE_IOCTL_GET_IPV4_GATEWAY_ADDR;

    /* Call lower layer ioctl to get ipv4 address */
    if (dsc_dcm_if_call_ll_ioctl(iface, &ioctl) == DSC_OP_FAIL) {
        dsc_log_err("dsc_dcm_if_get_ipv4_gateway_addr: ll_ioctl failed");
        return;
    }

    /* Store ipv4 address in the configuration blob */
    dsc_dcm_if_cfg_set_ipv4_gateway_addr(iface, &ioctl.info.gateway);
    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_get_prim_dns_addr
===========================================================================*/
/*!
@brief
  Helper function that calls the lower layer IOCTL to get the primary dns
  address assigned to the specified IFACE and sets it in the IFACE's control
  blob.

@return
  void

@note

  - Dependencies
    - IFACE must be in the UP state and must have an primary dns address
      assigned.

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_if_get_ipv4_prim_dns_addr (dsc_dcm_iface_t * iface)
{
    dsc_dcm_iface_ioctl_t ioctl;

    ioctl.name = DSS_IFACE_IOCTL_GET_IPV4_PRIM_DNS_ADDR;

    /* Call lower layer ioctl to get ipv4 address */
    if (dsc_dcm_if_call_ll_ioctl(iface, &ioctl) == DSC_OP_FAIL) {
        dsc_log_err("dsc_dcm_if_get_ipv4_prim_dns_addr: ll_ioctl failed");
        return;
    }

    /* Store ipv4 address in the configuration blob */
    dsc_dcm_if_cfg_set_ipv4_prim_dns_addr(iface, &ioctl.info.dns_addr.dns_primary);
    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_get_ipv4_seco_dns_addr
===========================================================================*/
/*!
@brief
  Helper function that calls the lower layer IOCTL to get the secondary dns
  address assigned to the specified IFACE and sets it in the IFACE's control
  blob.

@return
  void

@note

 - Dependencies
   - IFACE must be in the UP state and must have an secondary dns address
     assigned.

 - Side Effects
   - None
*/
/*=========================================================================*/
static void
dsc_dcm_if_get_ipv4_seco_dns_addr (dsc_dcm_iface_t * iface)
{
    dsc_dcm_iface_ioctl_t ioctl;

    ioctl.name = DSS_IFACE_IOCTL_GET_IPV4_SECO_DNS_ADDR;

    /* Call lower layer ioctl to get ipv4 address */
    if (dsc_dcm_if_call_ll_ioctl(iface, &ioctl) == DSC_OP_FAIL) {
        dsc_log_err("dsc_dcm_if_get_ipv4_seco_dns_addr: ll_ioctl failed");
        return;
    }

    /* Store ipv4 address in the configuration blob */
    dsc_dcm_if_cfg_set_ipv4_seco_dns_addr(iface, &ioctl.info.dns_addr.dns_secondary);
    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_get_cfg_at_if_up
===========================================================================*/
/*!
@brief
  Function that fetches all necessary configuration for an IFACE from the 
  lower layers when the IFACE moves to the UP state. 

@return
  void

@note

  - Dependencies
    - IFACE must be in the UP state.

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_dcm_if_get_cfg_at_if_up (dsc_dcm_iface_t * iface)
{
    /* Fetch and store ipv4 address in the configuration blob */
    dsc_dcm_if_get_ipv4_addr(iface);
    dsc_dcm_if_get_ipv4_gateway_addr(iface);
    dsc_dcm_if_get_ipv4_prim_dns_addr(iface);
    dsc_dcm_if_get_ipv4_seco_dns_addr(iface);
    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_iface_ioctl_bind_sock_to_iface
===========================================================================*/
/*!
@brief
  Helper function to process the IOCTL to bind socket to specified IFACE. 

@return
  int - 0 on success, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_dcm_iface_ioctl_bind_sock_to_iface
(
    dsc_dcm_iface_t     * iface, 
    dcm_iface_ioctl_t   * ioctl
)
{
    dsc_dcm_iface_ioctl_t dsc_dcm_ioctl;

    dsc_dcm_ioctl.name = ioctl->name;
    dsc_dcm_ioctl.info.bind_info = ioctl->info.bind_sock_to_iface_info;

    /* Call lower layer ioctl handler to bind socket to iface */
    if (dsc_dcm_if_call_ll_ioctl(iface, &dsc_dcm_ioctl) == DSC_OP_FAIL) {
        dsc_log_err("dsc_dcm_iface_ioctl_bind_sock_to_iface: ll_ioctl failed");
        dsc_abort();
    }

    return 0;
}

/*===========================================================================
  FUNCTION  dsc_dcm_iface_ioctl_get_device_name
===========================================================================*/
/*!
@brief
  Helper function to process the IOCTL to get device name.

@return
  int - 0 on success, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_dcm_iface_ioctl_get_device_name
(
    dsc_dcm_iface_t     * iface, 
    dcm_iface_ioctl_t   * ioctl
)
{
    dsc_dcm_iface_ioctl_t dsc_dcm_ioctl;

    dsc_dcm_ioctl.name = ioctl->name;

    /* Call lower layer ioctl handler to bind socket to iface */
    if (dsc_dcm_if_call_ll_ioctl(iface, &dsc_dcm_ioctl) == DSC_OP_FAIL) {
        dsc_log_err("dsc_dcm_iface_ioctl_get_device_name: ll_ioctl failed");
        return -1;
    }

    memcpy 
    (
        ioctl->info.device_name_info.device_name,
        dsc_dcm_ioctl.info.device_name_info.device_name,
        DSS_MAX_DEVICE_NAME_LEN
    );

    return 0;
}

/*===========================================================================
  FUNCTION  dsc_dcm_iface_ioctl_dormancy
===========================================================================*/
/*!
@brief
  Helper function to process the IOCTL related to physlink dormancy.

@return
  int - 0 on success, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_dcm_iface_ioctl_dormancy
(
    dsc_dcm_iface_t     * iface, 
    dcm_iface_ioctl_t   * ioctl
)
{
   dsc_dcm_iface_ioctl_t dsc_dcm_ioctl;

   ds_assert(iface);
   ds_assert(ioctl);

   dsc_dcm_ioctl.name = ioctl->name;

   /* Call lower layer ioctl handler to make the physlink go dormant/active*/
   if (dsc_dcm_if_call_ll_ioctl(iface, &dsc_dcm_ioctl) == DSC_OP_FAIL) {
       dsc_log_err("dsc_dcm_iface_ioctl_dormancy: ll_ioctl failed");
       return -1;
   }
   return 0;
}


/*===========================================================================
  FUNCTION  dsc_dcm_iface_ioctl_get_data_bearer
===========================================================================*/
/*!
@brief
  Helper function to process the IOCTL related to get the data bearer.

@return
  int - 0 on success, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_dcm_iface_ioctl_get_data_bearer
(
    dsc_dcm_iface_t     * iface, 
    dcm_iface_ioctl_t   * ioctl
)
{
   dsc_dcm_iface_ioctl_t dsc_dcm_ioctl;

   ds_assert(iface);
   ds_assert(ioctl);

   dsc_dcm_ioctl.name = ioctl->name;

   /* Call lower layer ioctl handler to make the physlink go dormant/active*/
   if (dsc_dcm_if_call_ll_ioctl(iface, &dsc_dcm_ioctl) == DSC_OP_FAIL) {
       dsc_log_err("dsc_dcm_iface_ioctl_dormancy: ll_ioctl failed");
       return -1;
   }

   ioctl->info.data_bearer_tech = dsc_dcm_ioctl.info.data_bearer_tech;
   return 0;
}

/*===========================================================================
  FUNCTION  dsc_dcm_lock
===========================================================================*/
/*!
@brief
  Wrapper function to acquire the global lock of the DCM module. 

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static __inline__ void
dsc_dcm_lock (void)
{
    /* Acquire global mutex */
    ds_assert(pthread_mutex_lock(&dsc_dcm_ctrl.mutx) == 0);
}

/*===========================================================================
  FUNCTION  dsc_dcm_unlock
===========================================================================*/
/*!
@brief
  Wrapper function to release the global lock of the DCM module. 

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static __inline__ void 
dsc_dcm_unlock (void)
{
    /* Release global mutex */
    ds_assert(pthread_mutex_unlock(&dsc_dcm_ctrl.mutx) == 0);
}

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_dcm_init
===========================================================================*/
/*!
@brief
  Initialization routine for the DCM module. 

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void
dsc_dcm_init (void)
{
    pthread_mutexattr_t attr;

    /* Initialize mutex attributes */
    (void)pthread_mutexattr_init(&attr);

    /* Set mutex attributes to support recursive locking */
    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) < 0) {
        dsc_log_err("Cannot set mutex attribute to RECURSIVE");
        dsc_abort();
    }

    /* Initialize global mutex */
    (void)pthread_mutex_init(&dsc_dcm_ctrl.mutx, &attr);

    /* Destroy mutex attributes after use */
    (void)pthread_mutexattr_destroy(&attr);

    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_get_name
===========================================================================*/
/*!
@brief
  Returns name of the iface pointed to be the specified iface id.

@return
  dcm_iface_name_t - Name of this iface

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dcm_iface_name_t
dsc_dcm_if_get_name (dcm_iface_id_t if_id) 
{
    dsc_dcm_iface_t * iface;
    ds_assert(dsc_dcm_verify_if_id(if_id) == 0);
    /*
    ds_assert((iface = dsc_dcm_if_get_iface(if_id)));
    */
    iface = dsc_dcm_if_get_iface(if_id);
    ds_assert( iface );
    return iface->if_name;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_create
===========================================================================*/
/*!
@brief
  Creates an IFACE of the specified type in the system and registers the 
  lower-layer specified handlers. 

@return
  dcm_iface_id_t - IFACE ID if successful, DCM_IFACE_ID_INVALID otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dcm_iface_id_t
dsc_dcm_if_create
(
    dcm_iface_name_t        if_name,
    dcm_iface_name_t        if_group, 
    void *                  call_hdl, 
    dsc_dcm_if_op_tbl_t   * if_op_tbl_p
)
{
    int i;
    dsc_dcm_iface_t * iface;
    dcm_iface_id_t if_id = DCM_IFACE_ID_INVALID;

    dsc_log_func_entry();

    /* Acquire global lock */
    dsc_dcm_lock();

    /* Make sure that the iface operations table is valid */
    if (dsc_dcm_verify_if_op_tbl(if_op_tbl_p) < 0) {
        dsc_log_err("Verification of if op tbl failed");
        dsc_abort();
        goto error;
    }

    /* Find an unused entry in the iface ptr array */
    for (i = 0; i < DSC_MAX_IFACE; ++i) {
        if (dsc_dcm_if_parr[i] == NULL) {
            break;
        }
    }

    if (i >= DSC_MAX_IFACE) {
        /* Cannot find unused entry, so return error */
        dsc_log_err("Cannot create IFACE - out of entries");
        goto error;
    }

    /* Allocate memory for iface */
    if ((iface = dsc_malloc(sizeof(dsc_dcm_iface_t))) == NULL) {
        dsc_log_err("Cannot alloc memory for IFACE");
        dsc_abort();
        goto error;
    }

    /* Initialize memory */
    memset(iface, 0, sizeof(dsc_dcm_iface_t));

    /* Iface id is a constant offset from the index in the iface ptr array */
    if_id = (dcm_iface_id_t)(i) + DSC_DCM_IFACE_ID_OFFSET;

    /* Set iface id, name, group, etc. */
    iface->if_id = if_id;
    iface->if_name = if_name;
    iface->if_group = if_group;
    iface->if_op_tbl = *if_op_tbl_p;
    iface->call_hdl = call_hdl;

    /* Initialize list of net handles bound to iface */
    dsc_dcm_if_nh_info_init(&iface->if_nh_info);

    /* Initialize event management data structures for iface */
    dsc_dcm_if_ev_info_init(iface);

    /* Initialize iface state and phys link state to down */
    iface->if_state = IFACE_DOWN;
    iface->if_phlnk_state = PHYS_LINK_DOWN;

    /* Store iface pointer in array */
    dsc_dcm_if_parr[i] = iface;

error:

    /* Release global lock */
    dsc_dcm_unlock();

    dsc_log_func_exit();

    /* Return iface id, or invalid id if creation fails */
    return if_id;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_up_ind
===========================================================================*/
/*!
@brief
  Function used by the lower layers to indicate that the IFACE should be 
  moved to the UP state.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - Associated network callbacks of clients are called.
*/
/*=========================================================================*/
void
dsc_dcm_if_up_ind
(
    dcm_iface_id_t if_id
)
{
    dsc_dcm_iface_t * iface;
    dcm_iface_state_t prev_if_state;

    dsc_log_func_entry();

    /* Acquire global mutex */
    dsc_dcm_lock();

    /* Verify that iface id correponds to a valid iface */
    if (dsc_dcm_verify_if_id(if_id) < 0) {
        dsc_log_err("Bogus if_id %d passed in dsc_dcm_if_up_ind", (int)if_id);
        dsc_abort();
        goto error;
    }

    /* Get iface ptr from iface id */
    iface = dsc_dcm_if_get_iface(if_id);

    /* Save current iface state */
    prev_if_state = iface->if_state;

    /* Process event based on current iface state */
    switch (iface->if_state) {
    case IFACE_COMING_UP:
        /* Iface is in coming up state. Transition to up state and call the 
        ** network callbacks. 
        */
        iface->if_state = IFACE_UP;
        dsc_dcm_if_call_net_cbs(iface, DS_ENETISCONN);

        /* Also, store iface configuration for later use */
        dsc_dcm_if_get_cfg_at_if_up(iface);
        break;
    case IFACE_UP:
        /* Iface is already up. This should not happen */
        dsc_log_err("IFACE %d already up", (int)if_id);
        dsc_abort();
        goto error;
    case IFACE_GOING_DOWN:
        /* It is possible in a race condition that lower layer indicates 
        ** iface is up while application tries to bring the call down, and 
        ** the latter is received just before the former. Ignore up indication 
        ** in this case. 
        */
        dsc_log_high("IFACE UP IND received when iface %d is going down",
                     (int)if_id);
        goto error;
    default:
        /* All other cases. This should not happen */
        dsc_log_err("IFACE UP IND received when iface %d in %d state", 
                    (int)if_id, iface->if_state);
        dsc_abort();
        goto error;
    }

    /* Notify clients of iface state change, if state change occured */
    dsc_dcm_if_call_if_state_ev_cbs(iface, iface->if_state, prev_if_state);

error:
    /* Release global mutex */
    dsc_dcm_unlock();

    dsc_log_func_exit();
    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_reconfigured_ind
===========================================================================*/
/*!
@brief
  Function used by the lower layers to indicate that the IFACE is 
  reconfigured.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - Associated network callbacks of clients are called.
*/
/*=========================================================================*/
void
dsc_dcm_if_reconfigured_ind
(
    dcm_iface_id_t if_id
)
{
    dsc_dcm_iface_t * iface;

    dsc_log_func_entry();

    /* Acquire global mutex */
    dsc_dcm_lock();

    /* Verify that iface id correponds to a valid iface */
    if (dsc_dcm_verify_if_id(if_id) < 0) {
        dsc_log_err("Bogus if_id %d passed in dsc_dcm_if_reconfigured_ind", (int)if_id);
        goto error;
    }

    /* Get iface ptr from iface id */
    iface = dsc_dcm_if_get_iface(if_id);

    /* Process event based on current iface state */
    switch (iface->if_state) {
    case IFACE_UP:
        dsc_dcm_if_call_net_cbs(iface, DS_ENETRECONFIGURED);
        /* Also, store iface configuration for later use */
        dsc_dcm_if_get_cfg_at_if_up(iface);
        break;
    default:
        /* All other cases. This should not happen */
        dsc_log_err("IFACE RECONFIGURED IND received when iface %d in %d state", 
                    (int)if_id, iface->if_state);
        goto error;
    }

error:
    /* Release global mutex */
    dsc_dcm_unlock();

    dsc_log_func_exit();
    return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_get_reason_code
===========================================================================*/
/*!
@brief
    Returns the last netdown reason code from the nethandle info structure.
@return
    Returns DSC_DCM_NO_ERR on successful operation reason code containing the 
    valid reason code. DSC_DCM_ERR_VAL if unsuccessful.

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/

int     
dsc_dcm_get_reason_code
(       
  int net_hdl,
  int *reason_code
)   
{ 
  dsc_dcm_nh_info_t * nh_info;

  if(net_hdl < 0 || net_hdl >= DSC_MAX_NH)
  {
    dsc_log_err("Invalid net Handle %d",net_hdl);
    return DSC_DCM_ERR_VAL;
  }
  ds_assert(reason_code != NULL);
  nh_info = dsc_dcm_nh_info_parr[net_hdl];

  if (nh_info == NULL || net_hdl != nh_info->nh_id)
  {
    dsc_log_err("dsc_dcm_get_reason_code: Cannot Handle Bad Input. %d",net_hdl);
    return DSC_DCM_ERR_VAL;
  }

  *reason_code = nh_info->call_end_reason;
    
  return DSC_DCM_NO_ERR;
}   

/*===========================================================================
  FUNCTION  dsc_dcm_store_last_netdown_reason_code
===========================================================================*/
/*!
@brief
    Stores the last net down reason code,when the iface goes down due to call
    termination in the net handle info structure.
@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/

static void
dcm_set_last_netdown_reason_code
(
dsc_dcm_iface_t    *iface,
int                 reason_code
)
{
  ds_dll_el_t * node = NULL;
  dsc_dcm_nh_info_t * nh_info = NULL;
  const void * data = NULL;
  int nh;
  ds_assert(iface != NULL);
  /* Get head of list of network handles that have registered for this
   *iface.
   */
  node = iface->if_nh_info.l_head;

  /*Iterate over the list, and set the call end reason for each nethdl*/
  while ((node = ds_dll_next(node, &data)))
  {
    nh = (int)data;
    nh_info = dsc_dcm_nh_info_parr[nh];
    ds_assert(nh_info != NULL);
    if (nh_info)
    {
      nh_info->call_end_reason = reason_code;
    }
  }
  return;
}

/*===========================================================================
  FUNCTION  dsc_dcm_if_down_ind
===========================================================================*/
/*!
@brief
  Function used by the lower layers to indicate that the IFACE should be 
  moved to the DOWN state.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - Associated network callbacks of clients are called.
*/
/*=========================================================================*/
void
dsc_dcm_if_down_ind
(
    int            call_end_reason,
    dcm_iface_id_t if_id
)
{
    dsc_dcm_iface_t * iface;
    dcm_iface_state_t prev_if_state;

    dsc_log_func_entry();

    /* Acquire global mutex */
    dsc_dcm_lock();

    /* Verify that iface id correponds to a valid iface */
    if (dsc_dcm_verify_if_id(if_id) < 0) {
        dsc_log_err("Bogus if_id %d passed in dsc_dcm_if_down_ind", (int)if_id);
        dsc_abort();
        goto error;
    }

    /* Get iface ptr from iface id */
    iface = dsc_dcm_if_get_iface(if_id);

    /* Save current iface state */
    prev_if_state = iface->if_state;

    /* Process event based on current iface state */
    switch (iface->if_state) {
    case IFACE_GOING_DOWN:
    case IFACE_UP:
    case IFACE_COMING_UP:
        /* If iface is in going down state, this is a confirmation of call
        ** termination. If iface is in up state, this is a network initiated
        ** teardown. If iface is in coming up state, this is a rejection from
        ** the network. In all cases, change state to down and notify clients
        ** using the registered network callbacks. 
        */
        iface->if_state = IFACE_DOWN;
        dsc_dcm_if_call_net_cbs(iface, DS_ENETNONET);
        /* Store the last net down reason code
        * in the nhandles pertaining to the iface 
        */
        dsc_log_high("-------->The call end reason code in dcm_down_ind is %d",call_end_reason);
        dcm_set_last_netdown_reason_code(iface,call_end_reason);
        /* Unbind all net handles currently bound to this iface */
        dsc_dcm_if_del_all_nh(iface);
        break;
    case IFACE_DOWN:
        /* Iface is already down. This is unexpected in current impl. */
        dsc_log_err("IFACE %d already down", (int)if_id);
        dsc_abort();
        goto error;
    default:
        /* All other cases. This should not happen */
        dsc_log_err("IFACE DOWN IND received when iface %d in %d state", 
                    (int)if_id, iface->if_state);
        dsc_abort();
    }
    /* Notify clients of iface state change, if state change occured */
    dsc_dcm_if_call_if_state_ev_cbs(iface, iface->if_state, prev_if_state);

error:
    /* Release global mutex */
    dsc_dcm_unlock();

    dsc_log_func_exit();
    return;
}


/*===========================================================================
  FUNCTION  dsc_dcm_physlink_state_change_ind
===========================================================================*/
/*!
@brief
  Function used by the lower layers to indicate physlink state change.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - Associated network callbacks of clients are called.
*/
/*=========================================================================*/
void
dsc_dcm_physlink_state_change_ind
(
    int            dorm_status,
    dcm_iface_id_t if_id
)
{
  dsc_dcm_iface_t * iface;
  dsc_log_func_entry();
 
  /* Acquire global mutex */
  dsc_dcm_lock();

  /* Verify that iface id correponds to a valid iface */
  if (dsc_dcm_verify_if_id(if_id) < 0) {
      dsc_log_err("Bogus if_id %d passed in dsc_dcm_if_down_ind", (int)if_id);
      dsc_abort();
      goto error;
  }

  /* Get iface ptr from iface id */
  iface = dsc_dcm_if_get_iface(if_id);

  /* Process event based on current iface state */
  switch (iface->if_state) {
    case IFACE_UP:
    /* Notify clients of iface state change, if state change occured */
    dsc_dcm_if_call_physlink_state_ev_cbs(iface, iface->if_state, dorm_status);
    break;
    default:
      /* All other cases. This should not happen */
      dsc_log_err("Ignoring physlink state change indication received when iface %d in %d state", 
                  (int)if_id, iface->if_state);
  }

error:
 /* Release global mutex */
 dsc_dcm_unlock();
 return;   
}
/*===========================================================================
  FUNCTION  dcm_get_net_handle
===========================================================================*/
/*!
@brief
  Allocates a network handle to the client. 

@return
  int - network handle if allocation is successful, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
dcm_get_net_handle
(
    int                             dcm_clnt_hdl,
    dcm_net_cb_fcn                  net_cb,
    void                          * net_cb_user_data,
    dcm_iface_ioctl_event_cb_fcn    ev_cb,
    void                          * ev_cb_user_data,
    int                           * dss_errno
)
{
    int nh = -1;

    dsc_log_func_entry();

    /* Acquire global mutex */
    dsc_dcm_lock();

    /* Verify that client handle corresponds to a valid client */
    if (dsc_dcm_verify_clnt_hdl(dcm_clnt_hdl) < 0) {
        /* Unrecognized/invalid client. Return error */
        dsc_log_err("Invalid client handle %d in dcm_get_net_handle",
                    dcm_clnt_hdl);
        *dss_errno = DS_EBADAPP;
        goto error;
    }

    /* Double check that net_cb is not null. This should never happen normally
    ** as this is a callback registered by the dss library instance. 
    */
    if (!net_cb) {
        dsc_log_err("Null net_cb in dcm_get_net_handle");
        *dss_errno = DS_EINVAL;
        goto error;
    }

    /* Allocate net handle */
    if ((nh = dsc_dcm_alloc_nh(dcm_clnt_hdl)) < 0) {
        /* Handle allocation failed. Return "max app" error */
        dsc_log_err("Cannot alloc net handle for client %d", dcm_clnt_hdl);
        *dss_errno = DS_EMAPP;
        nh = -1;
        goto error;
    }

    dsc_log_low("dcm_get_net_handle: alloc'ing nh %d", nh);

    /* Set network and event callbacks for net handle */
    dsc_dcm_nh_set_net_cb(nh, net_cb, net_cb_user_data);
    dsc_dcm_nh_set_ev_cb(nh, ev_cb, ev_cb_user_data);

    /* Set client handle for net handle */
    dsc_dcm_nh_set_clnt_hdl(nh, dcm_clnt_hdl);

    /* Add net handle to list of net handles for this client */
    dsc_dcm_clnt_nh_add(dcm_clnt_hdl, nh);

error:
    /* Release global mutex */
    dsc_dcm_unlock();

    dsc_log_func_exit();
    return nh;
}

/*===========================================================================
  FUNCTION  dcm_release_net_handle
===========================================================================*/
/*!
@brief
  Deallocates a previously assigned network handle. 

@return
  dsc_op_status_t - DSC_OP_SUCCESS if network handle is released, 
                    DSC_OP_FAIL otherwise.

@note

  - Dependencies
    - Client must have closed network by calling dcm_net_close() and the 
      network handle must be unbound (i.e. not associated with any IFACE),
      otherwise the call will fail.

  - Side Effects
    - None
*/
/*=========================================================================*/
dsc_op_status_t
dcm_release_net_handle
(
    int         dcm_nethandle,
    int       * dss_errno
)
{
    dsc_op_status_t status = DSC_OP_FAIL;
    dsc_dcm_iface_t * iface;
    int clnt_hdl;

    dsc_log_func_entry();

    /* Acquire global mutex */
    dsc_dcm_lock();

    dsc_log_low("dcm_release_net_handle: nh %d", dcm_nethandle);

    /* Verify that net handle is valid */
    if (dsc_dcm_verify_nh(dcm_nethandle) < 0) {
        /* Invalid handle. Return "bad app" error */
        dsc_log_err("Bogus nethandle passed in dcm_release_net_handle");
        *dss_errno = DS_EBADAPP;
        goto error;
    }

    /* Make sure that the handle is not bound to an iface. If yes, that means
    ** that the client has an up network and must first tear it down.
    */
    iface = dsc_dcm_nh_get_iface(dcm_nethandle);

    if (iface != NULL) {
        /* Net handle is bound to an iface. Return "net exists" error */
        dsc_log_err("IFACE not null - failing dcm_release_net_handle");
        *dss_errno = DS_ENETEXIST;
        goto error;
    }

    /* All clear to release net handle if we get here */

    /* Get client handle corresponding to this net handle */
    clnt_hdl = dsc_dcm_nh_get_clnt_hdl(dcm_nethandle);

    /* Delete net handle from the list of net handles for this client */
    dsc_dcm_clnt_nh_del(clnt_hdl, dcm_nethandle);

    /* Deallocate net handle */
    dsc_dcm_release_nh(dcm_nethandle);
    status = DSC_OP_SUCCESS;

error:
    /* Release global mutex */
    dsc_dcm_unlock();

    dsc_log_func_exit();
    return status;
}

/*===========================================================================
  FUNCTION  dcm_net_open
===========================================================================*/
/*!
@brief
  Bring up network using the specified network policy. Note that if the 
  operation fails, the applicable error code is returned in integer pointed
  to by dss_errno. 

@return
  dsc_op_status_t - DSC_OP_SUCCESS if network is successfully brought up, 
                    DSC_OP_FAIL otherwise 
@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dsc_op_status_t
dcm_net_open
(
    int                     dcm_nethandle,
    dcm_net_policy_info_t * net_policy,
    int                   * dss_errno
)
{
    dsc_dcm_iface_t * iface;
    dcm_iface_state_t if_state, prev_if_state;
    dsc_op_status_t   status = DSC_OP_FAIL;
    int               net_errno;

    dsc_log_func_entry();

    /* Acquire global mutex */
    dsc_dcm_lock();

    /* Verify that net handle is valid */
    if (dsc_dcm_verify_nh(dcm_nethandle) < 0) {
        /* Handle validation failed. Return "bad app" error */
        dsc_log_err("Bogus nethandle passed in dcm_net_open");
        *dss_errno = DS_EBADAPP;
        goto error;
    }

    /* Make sure that valid net_policy is specified. Note that dss lib always
    ** passes a net policy, which may be default if app doesn't specify any.
    */
    if (!net_policy) {
        dsc_log_err("Null net_policy in dcm_net_open");
        *dss_errno = DS_EINVAL;
        goto error;
    }

    /* Make sure that client hasn't already opened network using this net
    ** handle.
    */
    iface = dsc_dcm_nh_get_iface(dcm_nethandle);

    if (iface != NULL) {
        /* Handle is already associated with an iface. We process request
        ** without doing iface selection again.
        */
        dsc_log_high("IFACE already selected for app in dcm_net_open");
    } else {
        /* Do iface selection based on specified net policy */
        if ((iface = dsc_dcm_get_iface_for_policy(net_policy)) == NULL) {
            /* Could not find any iface that fits the bill. Return "no net"
            ** error. 
            */
            dsc_log_err("No matching iface found, failing dcm_net_open");
            *dss_errno = DS_ENETNONET;
            goto error;
        }
        /* A matching iface was found. Bind net handle to this iface */
        dsc_dcm_nh_set_iface(dcm_nethandle, iface);

        /* Save net policy for net handle */
        dsc_dcm_nh_set_net_policy(dcm_nethandle, net_policy);

        /* Add net handle to the list of handles bound to this iface */
        dsc_dcm_if_add_nh(iface, dcm_nethandle);
    }

    /* Save current iface state */
    prev_if_state = if_state = iface->if_state;

    /* Make sure that iface is not going down */
    if (if_state == IFACE_GOING_DOWN) {
        /* Iface is going down. Return "net close in progress" error */
        *dss_errno = DS_ENETCLOSEINPROGRESS;
        goto error;
    }

    /* Bring up iface if iface is currently down */
    if (if_state == IFACE_DOWN) {
        /* Double check for debugging purposes that the callback function 
        ** ptr is valid, i.e. not null. This should not normally happen as 
        ** we validate this at the time of iface creation. 
        */
        ds_assert(iface->if_op_tbl.if_up_cmd != NULL);

        /* Call lower layer function to bring up iface */
        status = 
        (*(iface->if_op_tbl.if_up_cmd))(iface->if_id, iface->call_hdl);
        if (status == DSC_OP_FAIL) {
            /* Iface up cmd failed for some reason. Return error. This 
            ** normally should not occur. 
            */
            dsc_log_err("iface up cmd failed for iface %d", (int)iface->if_id);
            *dss_errno = DS_EINVAL;
            goto error;
        }

        /* Change state to coming up */
        if_state = iface->if_state = IFACE_COMING_UP;

        /* Reset status to failure for now, as we reused this var above */
        status = DSC_OP_FAIL;
    }

    /* Depending on iface state, return appropriate error code to client */
    switch (if_state) {
    case IFACE_UP:
        /* Iface is already up. Return "net is connected" error */
        net_errno = DS_ENETISCONN;
        status = DSC_OP_SUCCESS;
        break;
    case IFACE_COMING_UP:
        /* Iface is coming up. Return "net in progress" error */
        net_errno = DS_ENETINPROGRESS;
        break;
    default:
        /* In any other state, return error. This should never be reached */
        dsc_log_err("invalid if state %d", if_state);
        *dss_errno = DS_EINVAL;
        goto error;
    }

    /* Call network callback with the error code determined above */
    dsc_dcm_nh_call_net_cb(dcm_nethandle, iface->if_id, net_errno);

    /* Notify registered clients of potential iface state change */
    dsc_dcm_if_call_if_state_ev_cbs(iface, iface->if_state, prev_if_state);

    /* Return "would block" error. Note that if iface is already up, the 
    ** network callback called above is also used to notify the app of network 
    ** 'coming up' along with this function returning success.
    */
    if (status != DSC_OP_SUCCESS)
        *dss_errno = DS_EWOULDBLOCK;

error:
    /* Release global mutex */
    dsc_dcm_unlock();

    dsc_log_func_exit();
    return status;
}

/*===========================================================================
  FUNCTION  dcm_net_close
===========================================================================*/
/*!
@brief
  Bring down the network/IFACE that the specified network handle is bound to. 
  Note that if the operation fails, the applicable error code is returned in
  the integer pointed to by dss_errno. 

@return
  dsc_op_status_t - DSC_OP_SUCCESS if network is successfully brought down, 
                    DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dsc_op_status_t
dcm_net_close
(
    int                     dcm_nethandle,
    int                   * dss_errno
)
{
    dsc_op_status_t status = DSC_OP_FAIL; 
    dsc_dcm_iface_t * iface;
    dcm_iface_state_t prev_if_state;
    dsc_dcm_if_nh_info_t * if_nh_info;
    int    net_errno;

    dsc_log_func_entry();

    /* Acquire global mutex */
    dsc_dcm_lock();

    /* Verify that net handle is valid */
    if (dsc_dcm_verify_nh(dcm_nethandle) < 0) {
        /* Handle validation failed. Return "bad app" error */
        dsc_log_err("Bogus nethandle passed in dcm_net_close");
        *dss_errno = DS_EBADAPP;
        goto error;
    }

    /* Get iface ptr for net handle */
    iface = dsc_dcm_nh_get_iface(dcm_nethandle);

    /* Make sure iface ptr is non null */
    if (!iface) {
        /* Net handle is not bound to any iface, meaning there is nothing to
        ** do. Return success.  
        */
        dsc_log_high("dcm_net_close: net already down");
        status = DSC_OP_SUCCESS;
        goto error;
    }

    /* Save current iface state */
    prev_if_state = iface->if_state;

    /* Get net handle info blob for iface */
    if_nh_info = &iface->if_nh_info;

    /* Check if this is the only net handle bound to iface */
    if (if_nh_info->n_elem > 1) {
        /* Other clients are using this iface. No need to bring the iface down.
        ** Unbind net handle from iface and return success to client. 
        */
        dsc_dcm_if_del_nh(iface, dcm_nethandle);
        dsc_dcm_nh_set_iface(dcm_nethandle, NULL);
        status = DSC_OP_SUCCESS;
        goto error;
    }

    /* We can only get here if this handle is the sole user of this iface. 
    ** Double check. 
    */
    ds_assert(if_nh_info->n_elem > 0);

    /* Check if handle is detached, i.e. client has exited. If yes, we unbind
    ** net handle from iface so as to avoid dispatching network and event 
    ** callbacks to this client as a result of trying to bring the iface down.
    */
    if (dsc_dcm_nh_is_detached(dcm_nethandle)) {
        dsc_dcm_if_del_nh(iface, dcm_nethandle);
        dsc_dcm_nh_set_iface(dcm_nethandle, NULL);
    }

    /* Process based on iface's current state */
    switch (iface->if_state) {
    case IFACE_UP:
        /* Intentional fall through */
    case IFACE_COMING_UP:
        /* Bring down iface if iface is in up or coming up state */
        iface->if_state = IFACE_GOING_DOWN;

        /* Double check for debugging purposes that the handler func is valid */
        ds_assert(iface->if_op_tbl.if_down_cmd != NULL);

        /* Call lower layer function to bring down iface */
        status = 
        (*(iface->if_op_tbl.if_down_cmd))(iface->if_id, iface->call_hdl);
        if (status == DSC_OP_FAIL) {
            /* Unexpected failure in lower layer handle. Return error */ 
            dsc_log_err("iface down cmd failed for iface %d", (int)iface->if_id);
            *dss_errno = DS_EINVAL;
            goto error;
        }

        /* Return "net close in progress" error to client */
        net_errno = DS_ENETCLOSEINPROGRESS;

        /* Reset status to fail as we reused the var above */
        status = DSC_OP_FAIL;
        break;
    case IFACE_GOING_DOWN:
        /* Iface is in going down state. Return "net close in progress" error */
        net_errno = DS_ENETCLOSEINPROGRESS;
        break;
    case IFACE_DOWN:
        /* Iface is already down so return success. Note that we should not 
        ** get here as otherwise iface ptr should have been null, so print 
        ** debug message also. 
        */
        status = DSC_OP_SUCCESS;
        dsc_log_err("iface state is down for iface bound to net handle %d",
                    dcm_nethandle);
        goto error;
    default:
        /* In all other states, return error. We should not get here */
        dsc_log_err("invalid if state %d", iface->if_state);
        *dss_errno = DS_EINVAL;
        goto error;
    }

    /* Call network callback with the error code determined above */
    dsc_dcm_nh_call_net_cb(dcm_nethandle, iface->if_id, net_errno);

    /* Notify registered clients of potential iface state change */
    dsc_dcm_if_call_if_state_ev_cbs(iface, iface->if_state, prev_if_state);

    /* Return "would block" error. Note that if iface was brought down, the
    ** network callback called above is used to notify the app of network 
    ** 'going down'. 
    */
    if (status != DSC_OP_SUCCESS)
        *dss_errno = DS_EWOULDBLOCK;

error:
    /* Release global mutex */
    dsc_dcm_unlock();

    dsc_log_func_exit();
    return status;
}

/*===========================================================================
  FUNCTION  dcm_get_net_status
===========================================================================*/
/*!
@brief
  Queries the network state and returns the corresponding error code in the 
  variable pointed to by dss_errno. 

@return
  dsc_op_status_t - DSC_OP_SUCCESS if operation is successful, 
                    DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dsc_op_status_t
dcm_get_net_status
(
    int   dcm_nethandle,
    int * dss_errno
)
{
    dsc_dcm_iface_t * iface;

    dsc_log_func_entry();

    /* Acquire global mutex */
    dsc_dcm_lock();

    /* Verify that net handle is valid */
    if (dsc_dcm_verify_nh(dcm_nethandle) < 0) {
        /* Handle validation failed. Return "bad app" error */
        dsc_log_err("Bogus nethandle passed in dcm_get_net_status");
        *dss_errno = DS_EBADAPP;
        goto error;
    }

    /* Get iface ptr for net handle */
    iface = dsc_dcm_nh_get_iface(dcm_nethandle);

    /* Check if handle is bound to a network */
    if (iface == NULL) {
        /* Handle is not bound to any iface. Return "no net" error */
        *dss_errno = DS_ENETNONET;
        goto error;
    }

    /* Process based on iface's current state */
    switch (iface->if_state) {
    case IFACE_DOWN:
        /* Iface is down, return "no net" error. Note that we should not get
        ** here, as iface ptr associated with net handle should be null 
        ** in this case. So abort for debug purposes.
        */
        *dss_errno = DS_ENETNONET;
        dsc_log_err("iface state is down for a bound iface for nh %d", 
                    dcm_nethandle);
        dsc_abort();
        break;
    case IFACE_UP:
        /* Iface is up, return "net is connected" error */
        *dss_errno = DS_ENETISCONN;
        break;
    case IFACE_COMING_UP:
        /* Iface is coming up, return "net in progress" error */
        *dss_errno = DS_ENETINPROGRESS;
        break;
    case IFACE_GOING_DOWN:
        /* Iface is going down, return "net close in progress" error */
        *dss_errno = DS_ENETCLOSEINPROGRESS;
        break;
    default:
        /* In all other states, abort for debug purposes. We should not get 
        ** here. 
        */
        dsc_log_err("Invalid state %d for iface %d", 
                    iface->if_state, (int)iface->if_id);
        dsc_abort();
        *dss_errno = DS_EINVAL;
    }

error:
    /* Release global mutex */
    dsc_dcm_unlock();

    dsc_log_func_exit();

    /* This function always returns failure with the appropriate error code 
    ** set.
    */
    return DSC_OP_FAIL;
}

/*===========================================================================
  FUNCTION  dcm_get_iface_id
===========================================================================*/
/*!
@brief
  Returns the IFACE ID that the specified network handle is bound to. 

@return
  dcm_iface_id_t - IFACE ID if the network handle is bound to an IFACE,
                   DCM_IFACE_ID_INVALID otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dcm_iface_id_t
dcm_get_iface_id
(
    int dcm_nethandle
)
{
    dsc_dcm_iface_t * iface = NULL;
    dcm_iface_id_t if_id = DCM_IFACE_ID_INVALID;

    dsc_log_func_entry();

    /* Acquire global mutex */
    dsc_dcm_lock();

    /* Verify that net handle is valid */
    if (dsc_dcm_verify_nh(dcm_nethandle) < 0) {
        /* Handle validation failed. Return invalid iface id */
        dsc_log_err("Bogus nethandle [%d] passed in dcm_get_net_status",
                    dcm_nethandle);
        goto error;
    }

    /* Get iface ptr for net handle */
    iface = dsc_dcm_nh_get_iface(dcm_nethandle);

    /* Make sure iface ptr is not null, otherwise return invalid iface id */
    if (iface == NULL) {
        goto error;
    }

    if_id = iface->if_id;

error:
    /* Release global mutex */
    dsc_dcm_unlock();

    dsc_log_func_exit();
    return if_id;
}

/*===========================================================================
  FUNCTION  dcm_get_iface_id_by_policy
===========================================================================*/
/*!
@brief
  Returns the IFACE ID of the IFACE compatible with (i.e. matching) the 
  specified network policy. 

@return
  dcm_iface_id_t - IFACE ID if an IFACE exists that matches the policy, 
                   DCM_IFACE_ID_INVALID otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dcm_iface_id_t
dcm_get_iface_id_by_policy
(
    dcm_net_policy_info_t * net_policy,
    int                   * dss_errno
)
{
    dsc_dcm_iface_t * iface;
    dcm_iface_id_t if_id = DCM_IFACE_ID_INVALID;

    dsc_log_func_entry();

    /* Acquire global mutex */
    dsc_dcm_lock();

    /* Do iface selection based on specified net policy */
    if ((iface = dsc_dcm_get_iface_for_policy(net_policy)) == NULL) {
        /* Failed to return any matching iface for specified policy. Return 
        ** "no route" error.
        */
        dsc_log_high("No matching iface found in dcm_get_iface_id_by_policy");
        *dss_errno = DS_ENOROUTE;
        goto error;
    }

    if_id = iface->if_id;

error:
    /* Release global mutex */
    dsc_dcm_unlock();

    dsc_log_func_exit();
    return if_id;
}

/*===========================================================================
  FUNCTION  dcm_iface_ioctl
===========================================================================*/
/*!
@brief
  Generic IOCTL handler.  

@return
  dsc_op_status_t - DSC_OP_SUCCESS, if IOCTL is successfully processed, 
                    DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dsc_op_status_t
dcm_iface_ioctl
(
    dcm_iface_id_t           iface_id,
    dcm_iface_ioctl_t      * ioctl,
    int                    * dss_errno
)
{
    dsc_dcm_iface_t * iface;
    dsc_op_status_t status = DSC_OP_FAIL;

    dsc_log_func_entry();

    /* Acquire global mutex */
    dsc_dcm_lock();

    /* Verify that iface id is valid */
    if (dsc_dcm_verify_if_id(iface_id) < 0) {
        /* Iface id validation failed. Return "bad f" error */
        dsc_log_err("dcm_iface_ioctl: invalid iface_id %d received", (int)iface_id);
        *dss_errno = DS_EBADF;
        goto error;
    }

    /*assert on ioctl*/
    ds_assert(ioctl);

    /* Get iface ptr for iface id */
    iface = dsc_dcm_if_get_iface(iface_id);

    /* Process based on specified ioctl type */
    switch (ioctl->name) {
    case DSS_IFACE_IOCTL_GET_IPV4_ADDR:
        //ioctl->info.ipv4_addr = dsc_dcm_if_cfg_get_ipv4_addr(iface);
        memcpy(&ioctl->info.ipv4_addr,&iface->if_cfg.ipv4_addr,sizeof(ip_addr_type)) ;
        break;

    case DSS_IFACE_IOCTL_GET_IPV4_PRIM_DNS_ADDR:
        memcpy(&ioctl->info.ipv4_prim_dns_addr,&iface->if_cfg.ipv4_prim_dns_addr,sizeof(ip_addr_type));
        break;

    case DSS_IFACE_IOCTL_GET_IPV4_SECO_DNS_ADDR:
        memcpy(&ioctl->info.ipv4_seco_dns_addr,&iface->if_cfg.ipv4_seco_dns_addr,sizeof(ip_addr_type));
        break;

    case DSS_IFACE_IOCTL_GET_IPV4_GATEWAY_ADDR:
        memcpy(&ioctl->info.ipv4_gateway_addr,&iface->if_cfg.ipv4_gateway_addr,sizeof(ip_addr_type));
        break;

    case DSS_IFACE_IOCTL_GET_STATE:
        ioctl->info.if_state = iface->if_state;
        break;

    case DSS_IFACE_IOCTL_GET_PHYS_LINK_STATE:
        ioctl->info.phys_link_state = iface->if_phlnk_state;
        break;

    case DSS_IFACE_IOCTL_REG_EVENT_CB:
    case DSS_IFACE_IOCTL_DEREG_EVENT_CB:

        if (dsc_dcm_verify_if_event(ioctl->info.event_info.name) < 0) {
            dsc_log_err("dcm_iface_ioctl: invalid event %d received",
                        ioctl->info.event_info.name);
            *dss_errno = DS_EFAULT;
            goto error;
        }

        if (dsc_dcm_verify_nh(ioctl->info.event_info.dcm_nethandle) < 0) {
            dsc_log_err("dcm_iface_ioctl: invalid nethandle %d received",
                        ioctl->info.event_info.dcm_nethandle);
            *dss_errno = DS_EFAULT;
            goto error;
        }

        if (dss_iface_ioctl_reg_dereg_event_cb
        (
            ioctl->name,
            iface_id,
            ioctl->info.event_info.name, 
            ioctl->info.event_info.dcm_nethandle
        ) < 0)
        {
            *dss_errno = DS_EFAULT;
            goto error;
        }
        break;

    case DSS_IFACE_IOCTL_GET_IFACE_NAME:
        ioctl->info.if_name = iface->if_name;
        break;

    case DSS_IFACE_IOCTL_BIND_SOCK_TO_IFACE:
        if (dsc_dcm_iface_ioctl_bind_sock_to_iface(iface, ioctl) != 0) 
        {
            dsc_log_err("dcm_iface_ioctl: bind_sock_to_iface failed");
            *dss_errno = DS_EFAULT;
            goto error;
        }
        break;

    case DSS_IFACE_IOCTL_GET_DEVICE_NAME:
        if (dsc_dcm_iface_ioctl_get_device_name(iface, ioctl) != 0) 
        {
            dsc_log_err("dcm_iface_ioctl: get_device_name failed");
            *dss_errno = DS_EFAULT;
            goto error;
        }
        break; 

    case DSS_IFACE_IOCTL_GO_DORMANT:
    case DSS_IFACE_IOCTL_GO_ACTIVE:
    case DSS_IFACE_IOCTL_DORMANCY_INDICATIONS_ON:
    case DSS_IFACE_IOCTL_DORMANCY_INDICATIONS_OFF:
        if (dsc_dcm_iface_ioctl_dormancy(iface, ioctl) != 0) 
        {
            dsc_log_err("dcm_iface_ioctl: go dormant ioctl failed");
            *dss_errno = DS_EFAULT;
            goto error;
        }
        break;      
    case DSS_IFACE_IOCTL_GET_CURRENT_DATA_BEARER:
        if (dsc_dcm_iface_ioctl_get_data_bearer(iface, ioctl) != 0)
        {
          dsc_log_err("dcm_iface_ioctl: go data bearer ioctl failed");
          *dss_errno = DS_EFAULT;
          goto error;
        }
        break;
    default:
        dsc_log_err("dcm_iface_ioctl: invalid ioctl %d received",
                    ioctl->name);
        *dss_errno = DS_EINVAL;
        goto error;
    }

    status = DSC_OP_SUCCESS;

error:
    /* Release global mutex */
    dsc_dcm_unlock();

    dsc_log_func_exit();
    return status;
}

/*===========================================================================
  FUNCTION  dcm_get_clnt_handle
===========================================================================*/
/*!
@brief
  Allocates a client handle for the client.

@return
  int - client handle if allocation is successful, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
dcm_get_clnt_handle
(
    void
)
{
    int clnt_hdl = DCM_CLIENT_HDL_INVALID;

    dsc_log_func_entry();

    /* Acquire global mutex */
    dsc_dcm_lock();

    /* Allocate client handle */
    if ((clnt_hdl = dsc_dcm_alloc_clnt_hdl()) < 0) {
        /* Client handle allocation failed. Return invalid handle */
        dsc_log_err("client handle allocation failed");
        clnt_hdl = DCM_CLIENT_HDL_INVALID;
        goto error;
    }

    dsc_log_low("dcm_get_clnt_handle: alloc'ing clnt_hdl %d", clnt_hdl);

error:
    /* Release global mutex */
    dsc_dcm_unlock();

    dsc_log_func_exit();
    return clnt_hdl;
}

/*===========================================================================
  FUNCTION  dcm_release_clnt_handle
===========================================================================*/
/*!
@brief
  Deallocates a previously assigned client handle. 

@return
  dsc_op_status_t - DSC_OP_SUCCESS if client handle is released, DSC_OP_FAIL
                    otherwise.

@note

  - Dependencies
    - None

  - Side Effects
    - Releases all network handles opened by the client
*/
/*=========================================================================*/
dsc_op_status_t
dcm_release_clnt_handle
(
    int dcm_clnt_hdl
)
{
    int rval = DSC_OP_FAIL;
    dsc_dcm_clnt_info_t * clnt_info;
    ds_dll_el_t * node;
    const void * data;
    int nh;
    dsc_dcm_iface_t * iface;
    int dss_errno;

    dsc_log_func_entry();

    /* Acquire global mutex */
    dsc_dcm_lock();

    dsc_log_low("dcm_release_clnt_handle called for hdl %d", dcm_clnt_hdl);

    /* Verify that client handle is valid */
    if (dsc_dcm_verify_clnt_hdl(dcm_clnt_hdl) < 0) {
        /* Invalid client handle specified. Print debug message and return */
        dsc_log_err("dcm_release_clnt_handle: dsc_dcm_verify_clnt_hdl failed!");
        goto error;
    }

    /* Get client info ptr for client handle */
    clnt_info = dsc_dcm_clnt_info_parr[dcm_clnt_hdl];
    ds_assert(clnt_info);
    ds_assert(clnt_info->l_head);
    ds_assert(clnt_info->l_tail);

    /* Set all net handles to DETACHED */

    node = clnt_info->l_head;

    while ((node = ds_dll_next(node, &data))) 
    {
        nh = (int)data;

        if (dsc_dcm_verify_nh(nh) < 0) {
            dsc_log_err("dcm_release_clnt_handle: dsc_dcm_verify_nh failed!");
            dsc_abort();
        }
        
        dsc_log_low("dcm_release_clnt_handle: setting nh %d to detached", nh);
        dsc_dcm_nh_set_detached(nh);
    }

    /* Iterate over the list of net handles for this client handle, closing 
    ** network for each one and then releasing it.
    */
    while ((node = ds_dll_deq(clnt_info->l_head, &clnt_info->l_tail, &data)))
    {
        nh = (int)data;

        /* Close network if open */

        iface = dsc_dcm_nh_get_iface(nh);

        if (iface != NULL) {
            dsc_log_high("IFACE not null for nh %d - closing network..", nh);

            if (dcm_net_close(nh, &dss_errno) != DSC_OP_SUCCESS) {
                if (dss_errno != DS_EWOULDBLOCK) {
                    dsc_log_err("dcm_release_clnt_handle: net_close failed!");
                    dsc_abort();
                }
            }
        }

        /* Double check network is closed by checking that iface ptr is null */
        iface = dsc_dcm_nh_get_iface(nh);
        ds_assert(iface == NULL);

        /* Release net handle */
        dsc_dcm_release_nh(nh);

        /* Free memory for the node */
        ds_dll_free(node);
    }

    /* Destroy the list once each element has been freed */
    ds_dll_destroy(clnt_info->l_head);

    dsc_free(clnt_info);
    dsc_dcm_clnt_info_parr[dcm_clnt_hdl] = NULL;

    rval = DSC_OP_SUCCESS;

error:
    /* Release global mutex */
    dsc_dcm_unlock();

    dsc_log_func_exit();
    return rval;
}

/*===========================================================================
  FUNCTION  dcm_debug_print_iface_array
===========================================================================*/
/*!
@brief
  debugging 

@return
  
@note

  - Dependencies
    - None

  - Side Effects
    - 
*/
/*=========================================================================*/
void 
dcm_debug_print_iface_array(void)
{  
  int i;
  dsc_log_low( "--------------BEGIN printing dsc_dcm_if_parr[]----------------" );

  for( i = 0; i < DSC_MAX_IFACE; i++ )
  {
    dsc_log_high( "dsc_dcm_if_parr[%d] is pointing to [%x]", 
                  i, (unsigned int)dsc_dcm_if_parr[i] );
  }

  dsc_log_low( "--------------END printing dsc_dcm_if_parr[]----------------" );
}
