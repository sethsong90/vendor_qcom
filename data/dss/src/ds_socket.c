/******************************************************************************

                        D S _ S O C K E T . C

******************************************************************************/

/******************************************************************************

  @file    ds_socket.c
  @brief   DSS API Implementation

  DESCRIPTION
  Implementation of DSS API for Linux.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2009,2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/dss/src/ds_socket.c#7 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
03/24/09   SM         Added Call End Reason Code Support
02/20/09   js         copy username/password/auth_pref from one net policy
                      to other
05/30/08   vk         Added support for GET_DEVICE_NAME IOCTL
05/22/08   vk         Added support for APN override in net policy
04/05/08   vk         Verification of net policy
04/04/08   vk         Added support for client control of logging in dss
12/18/07   vk         Support for automatic client deregistration
11/30/07   vk         Cleaned up lint warnings
11/26/07   vk         Added function headers and other comments
11/06/07   vk         Using safe string functions
09/28/07   vk         Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/select.h>
#include <pthread.h>
#include "stringl.h"
#include "ds_util.h"
#include "dssocket.h"
#include "ds_list.h"
#include "ds_fd_pass.h"
#include "dsc_dcm.h"
#include "dsc_qmi_wds.h"
#ifndef FEATURE_DS_LINUX_NO_RPC
#include "dsc_dcm_api_rpc.h"
#endif
/*===========================================================================
                     FORWARD DECLARATIONS
===========================================================================*/

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Constant representing the maximum number of clients (network handles) 
   supported.
---------------------------------------------------------------------------*/
#define DS_MAX_APP  10

/*--------------------------------------------------------------------------- 
   Constant representing the maximum number of sockets supported.
---------------------------------------------------------------------------*/
#define DS_MAX_SOCK 128

/*--------------------------------------------------------------------------- 
   Constant representing the offset added to the local DSS socket descriptor
   to obtain the DSS socket descriptor value passed to the client. 
---------------------------------------------------------------------------*/
#define DS_DSFD_OFFSET 101

/*--------------------------------------------------------------------------- 
   Constant representing the value set in the private cookie field in 
   net policy struct when initializing it. 
---------------------------------------------------------------------------*/
#define DSS_NETPOLICY_COOKIE (0x12343210UL)

/*--------------------------------------------------------------------------- 
   Type representing collection of info for a registered event callback
---------------------------------------------------------------------------*/
typedef struct ds_net_ev_cb_info_s {
    dss_iface_ioctl_event_cb    event_cb;
    void                      * user_data;
    dss_iface_id_type           iface_id;
} ds_net_ev_cb_info_t;

/*--------------------------------------------------------------------------- 
   Type representing data structure used for maintaining list of registered
   event callbacks
---------------------------------------------------------------------------*/
typedef struct ds_net_ev_cb_list_s {
    ds_dll_el_t * l_head;
    ds_dll_el_t * l_tail;
} ds_net_ev_cb_list_t;

/*--------------------------------------------------------------------------- 
   Type representing enumeration of socket operations
---------------------------------------------------------------------------*/
typedef enum {
    DS_SOCK_OP_SOCKET = 0,
    DS_SOCK_OP_CONNECT = 1,
    DS_SOCK_OP_READ = 2,
    DS_SOCK_OP_WRITE = 3,
    DS_SOCK_OP_ACCEPT = 4,
    DS_SOCK_OP_LISTEN = 5,
    DS_SOCK_OP_CLOSE = 6, 
    DS_SOCK_OP_BIND = 7, 
    DS_SOCK_OP_SHUTDOWN = 8, 
    DS_SOCK_OP_UNSPEC = 9
} ds_sock_op_t;

/*--------------------------------------------------------------------------- 
   Type representing enumeration of socket states
---------------------------------------------------------------------------*/
typedef enum {
    DS_SOCK_INVALID = 0,
    DS_SOCK_IDLE = 1,
    DS_SOCK_CONNECTING = 2, 
    DS_SOCK_LISTENING = 3
} ds_sock_state_t;

/*--------------------------------------------------------------------------- 
   Type representing an ACB (application control block); this captures all 
   state and other data related to a network handle
---------------------------------------------------------------------------*/
typedef struct {
    int                         inuse;
    int                         dcm_nethdl;
    dss_net_policy_info_type    net_policy;
    dss_net_cb_fcn              net_cb;
    void                      * net_cb_user_data;
    ds_net_ev_cb_list_t         net_ev_cb_list[DSS_IFACE_IOCTL_REG_EVENT_MAX];
    dss_sock_cb_fcn             sock_cb;
    void                      * sock_cb_user_data;
    ds_dll_el_t               * sk_lst_hd;
    ds_dll_el_t               * sk_lst_tl;
} ds_acb_t;

/*--------------------------------------------------------------------------- 
   Type representing collection of state and other data maintained for each 
   socket
---------------------------------------------------------------------------*/
typedef struct {
    int sysfd; /* real sockfd */
    int dsfd;
    ds_sock_state_t state;
    unsigned int pend_ev;
    unsigned int wait_ev;
    int nethdl;
} ds_sockinfo_t;

/*--------------------------------------------------------------------------- 
   Type representing collection of FD set info, for all currently open 
   sockets
---------------------------------------------------------------------------*/
typedef struct {
    fd_set rfds;
    fd_set wfds;
    fd_set xfds;
    int maxfd;
    int nfds;
} ds_fdset_info_t;

/*--------------------------------------------------------------------------- 
   Collection of control data structures that need to be statically 
   allocated in memory. 
---------------------------------------------------------------------------*/
static struct {
    ds_dll_el_t     ds_acb_lst_hd;
    ds_dll_el_t     ds_sk_lst_hd;
} ds_sock_ctrl_init;

/*--------------------------------------------------------------------------- 
   Collection of control information maintained by the library
---------------------------------------------------------------------------*/
struct {
    int             ds_dcm_hdl;
    ds_acb_t      * ds_acb_parr[DS_MAX_APP];
    ds_dll_el_t   * ds_acb_lst_hd;
    ds_dll_el_t   * ds_acb_lst_tl;
    pthread_mutex_t ds_acb_mutx;
    ds_sockinfo_t * ds_sockinfo_parr[DS_MAX_SOCK];
    ds_dll_el_t   * ds_sk_lst_hd;
    ds_dll_el_t   * ds_sk_lst_tl;
    ds_fdset_info_t ds_fdset_info;
    pthread_mutex_t ds_sock_mutx;
    pthread_mutex_t ds_net_mutx;
} ds_sock_ctrl = {
    .ds_dcm_hdl    = DCM_CLIENT_HDL_INVALID,
    .ds_acb_lst_hd = &ds_sock_ctrl_init.ds_acb_lst_hd,
    .ds_acb_lst_tl = &ds_sock_ctrl_init.ds_acb_lst_hd,
    .ds_sk_lst_hd  = &ds_sock_ctrl_init.ds_sk_lst_hd,
    .ds_sk_lst_tl  = &ds_sock_ctrl_init.ds_sk_lst_hd,
    .ds_acb_mutx   = PTHREAD_MUTEX_INITIALIZER,
    .ds_sock_mutx  = PTHREAD_MUTEX_INITIALIZER,
    .ds_net_mutx   = PTHREAD_MUTEX_INITIALIZER
};

/*--------------------------------------------------------------------------- 
   Boolean valued variable used to track library initialization
---------------------------------------------------------------------------*/
static int ds_init_done = FALSE;

/*--------------------------------------------------------------------------- 
   Collection of information used for invoking client callbacks in the 
   dedicated callback thread context
---------------------------------------------------------------------------*/
struct {
    pthread_mutex_t mutx;
    pthread_cond_t  cond;
    pthread_t       thrd;
    int             pipefd[2];
} ds_sock_cb_thrd_info;

/*--------------------------------------------------------------------------- 
   Type representing a message sent to the callback thread
---------------------------------------------------------------------------*/
typedef struct {
    int signo;
} ds_sock_cb_thrd_msg_t;

/*--------------------------------------------------------------------------- 
   Inline function to verify net policy
---------------------------------------------------------------------------*/
static __inline__ int 
ds_netpolicy_is_valid(const dss_net_policy_info_type * net_policy_p)
{
    return (((net_policy_p) && 
             (net_policy_p->dss_netpolicy_private.cookie != 
                DSS_NETPOLICY_COOKIE)) ? FALSE : TRUE);
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting handle assigned by DCM
---------------------------------------------------------------------------*/
static __inline__ int
ds_get_dcm_clnt_hdl (void)
{
    return ds_sock_ctrl.ds_dcm_hdl;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting handle assigned by DCM
---------------------------------------------------------------------------*/
static __inline__ void
ds_set_dcm_clnt_hdl (int dcm_clnt_hdl)
{
    ds_sock_ctrl.ds_dcm_hdl = dcm_clnt_hdl;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting dcm nethandle for a given network handle
---------------------------------------------------------------------------*/
static __inline__ void 
ds_nethdl_set_dcm_nethdl (int nethdl, int dcm_nethdl)
{
    ds_acb_t * ds_acb_p;
    ds_acb_p = ds_sock_ctrl.ds_acb_parr[nethdl];

    ds_acb_p->dcm_nethdl = dcm_nethdl;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting dcm nethandle for a given network handle
---------------------------------------------------------------------------*/
static __inline__ int
ds_nethdl_get_dcm_nethdl (int nethdl)
{
    ds_acb_t * ds_acb_p;
    ds_acb_p = ds_sock_ctrl.ds_acb_parr[nethdl];

    return ds_acb_p->dcm_nethdl;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting network policy for a given network handle
---------------------------------------------------------------------------*/
static __inline__ void
ds_nethdl_set_net_policy (int nethdl, dss_net_policy_info_type * net_policy)
{
    ds_acb_t * ds_acb_p;
    ds_acb_p = ds_sock_ctrl.ds_acb_parr[nethdl];

    ds_acb_p->net_policy = *net_policy;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting network policy for a given network handle
---------------------------------------------------------------------------*/
static __inline__ dss_net_policy_info_type *
ds_nethdl_get_net_policy (int nethdl)
{
    ds_acb_t * ds_acb_p;
    ds_acb_p = ds_sock_ctrl.ds_acb_parr[nethdl];

    return &ds_acb_p->net_policy;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting network callback for a given network handle
---------------------------------------------------------------------------*/
static __inline__ void
ds_nethdl_set_netcb
(
    int nethdl, 
    dss_net_cb_fcn net_cb,
    void * net_cb_user_data
)
{
    ds_acb_t * ds_acb_p;
    ds_acb_p = ds_sock_ctrl.ds_acb_parr[nethdl];

    ds_acb_p->net_cb = net_cb;
    ds_acb_p->net_cb_user_data = net_cb_user_data;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting network callback for a given network handle
---------------------------------------------------------------------------*/
static __inline__ dss_net_cb_fcn
ds_nethdl_get_netcb (int nethdl, void ** net_cb_user_data)
{
    ds_acb_t * ds_acb_p;
    ds_acb_p = ds_sock_ctrl.ds_acb_parr[nethdl];

    *net_cb_user_data = ds_acb_p->net_cb_user_data;
    return ds_acb_p->net_cb;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting socket callback for a given network handle
---------------------------------------------------------------------------*/
static __inline__ void
ds_nethdl_set_sockcb 
(
    int nethdl, 
    dss_sock_cb_fcn sock_cb, 
    void * sock_cb_user_data
)
{
    ds_acb_t * ds_acb_p;
    ds_acb_p = ds_sock_ctrl.ds_acb_parr[nethdl];

    ds_acb_p->sock_cb = sock_cb;
    ds_acb_p->sock_cb_user_data = sock_cb_user_data;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting socket callback for a given network handle
---------------------------------------------------------------------------*/
static __inline__ dss_sock_cb_fcn
ds_nethdl_get_sockcb (int nethdl, void ** sock_cb_user_data)
{
    ds_acb_t * ds_acb_p;
    ds_acb_p = ds_sock_ctrl.ds_acb_parr[nethdl];

    *sock_cb_user_data = ds_acb_p->sock_cb_user_data;
    return ds_acb_p->sock_cb;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting head pointer of the list of sockets for a 
   given network handle
---------------------------------------------------------------------------*/
static __inline__ ds_dll_el_t * 
ds_nethdl_get_sk_lst_hd (int nethdl)
{
    return ds_sock_ctrl.ds_acb_parr[nethdl]->sk_lst_hd;
}

/*--------------------------------------------------------------------------- 
   Inline function for getting index into the array of pointers to socket 
   control info for a given DSS socket descriptor
---------------------------------------------------------------------------*/
static __inline__ int
ds_sockinfo_get_index_from_dsfd (int dsfd)
{
    return dsfd - DS_DSFD_OFFSET;
}

/*--------------------------------------------------------------------------- 
   Inline function for getting the DSS socket descriptor for a given index 
   into the array of pointers to socket control info
---------------------------------------------------------------------------*/
static __inline__ int 
ds_sockinfo_get_dsfd_from_index (int indx)
{
    return indx + DS_DSFD_OFFSET;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting the state of a given DSS socket descriptor
---------------------------------------------------------------------------*/
static __inline__ ds_sock_state_t
ds_sockinfo_get_state (int dsfd)
{
    int indx;
    indx = ds_sockinfo_get_index_from_dsfd(dsfd);
    return ds_sock_ctrl.ds_sockinfo_parr[indx]->state;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting the system socket descriptor for a given DSS 
   socket descriptor
---------------------------------------------------------------------------*/
static __inline__ int 
ds_sockinfo_get_sysfd (int dsfd)
{
    int indx;
    indx = ds_sockinfo_get_index_from_dsfd(dsfd);
    return ds_sock_ctrl.ds_sockinfo_parr[indx]->sysfd;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting the network handle for a given DSS socket
   descriptor
---------------------------------------------------------------------------*/
static __inline__ int 
ds_sockinfo_get_nethdl (int dsfd)
{
    int indx;
    indx = ds_sockinfo_get_index_from_dsfd(dsfd);
    return ds_sock_ctrl.ds_sockinfo_parr[indx]->nethdl;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting the mask of events client is waiting on for a
   given DSS socket descriptor
---------------------------------------------------------------------------*/
static __inline__ unsigned int
ds_sockinfo_get_wait_ev (int dsfd)
{
    int indx;
    indx = ds_sockinfo_get_index_from_dsfd(dsfd);   
    return ds_sock_ctrl.ds_sockinfo_parr[indx]->wait_ev;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting the mask of events pending delivery to client
   for a given DSS socket descriptor
---------------------------------------------------------------------------*/
static __inline__ unsigned int
ds_sockinfo_get_pend_ev (int dsfd)
{
    int indx;
    indx = ds_sockinfo_get_index_from_dsfd(dsfd);   
    return ds_sock_ctrl.ds_sockinfo_parr[indx]->pend_ev;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting the system socket descriptor for a given DSS
   socket descriptor
---------------------------------------------------------------------------*/
static __inline__ void 
ds_sockinfo_set_sysfd (int dsfd, int sysfd)
{
    int indx;
    indx = ds_sockinfo_get_index_from_dsfd(dsfd);
    ds_sock_ctrl.ds_sockinfo_parr[indx]->sysfd = sysfd;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting the state of a given DSS socket descriptor
---------------------------------------------------------------------------*/
static __inline__ void
ds_sockinfo_set_state (int dsfd, ds_sock_state_t state)
{
    int indx;
    indx = ds_sockinfo_get_index_from_dsfd(dsfd);
    ds_sock_ctrl.ds_sockinfo_parr[indx]->state = state;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting the network handle for a given DSS socket 
   descriptor
---------------------------------------------------------------------------*/
static __inline__ void
ds_sockinfo_set_nethdl (int dsfd, int nethdl)
{
    int indx;
    indx = ds_sockinfo_get_index_from_dsfd(dsfd);
    ds_sock_ctrl.ds_sockinfo_parr[indx]->nethdl = nethdl;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for adding an event mask to the list of events that the 
   client is waiting on for a given DSS socket descriptor. 
---------------------------------------------------------------------------*/
static __inline__ void
ds_sockinfo_add_wait_ev (int dsfd, unsigned int wait_ev)
{
    int indx;
    indx = ds_sockinfo_get_index_from_dsfd(dsfd);
    ds_sock_ctrl.ds_sockinfo_parr[indx]->wait_ev |= wait_ev;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for removing an event mask from the list of events that  
   the client is waiting on for a given DSS socket descriptor. 
---------------------------------------------------------------------------*/
static __inline__ void
ds_sockinfo_clr_wait_ev (int dsfd, unsigned int wait_ev)
{
    int indx;
    indx = ds_sockinfo_get_index_from_dsfd(dsfd);
    ds_sock_ctrl.ds_sockinfo_parr[indx]->wait_ev &= ~wait_ev;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for adding an event mask to the list of events pending
   delivery to the client for a given DSS socket descriptor. 
---------------------------------------------------------------------------*/
static __inline__ void
ds_sockinfo_add_pend_ev (int dsfd, unsigned int pend_ev)
{
    int indx;
    indx = ds_sockinfo_get_index_from_dsfd(dsfd);
    ds_sock_ctrl.ds_sockinfo_parr[indx]->pend_ev |= pend_ev;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for clearing an event mask from the list of events pending
   delivery to the client for a given DSS socket descriptor. 
---------------------------------------------------------------------------*/
static __inline__ void
ds_sockinfo_clr_pend_ev (int dsfd, unsigned int pend_ev)
{
    int indx;
    indx = ds_sockinfo_get_index_from_dsfd(dsfd);
    ds_sock_ctrl.ds_sockinfo_parr[indx]->pend_ev &= ~pend_ev;
}

/*--------------------------------------------------------------------------- 
   Forward function declarations needed by subsequent definitions. 
---------------------------------------------------------------------------*/
static void ds_sock_cb_thrd_post_msg (const ds_sock_cb_thrd_msg_t * msg);
static void ds_sock_cb_thrd_init (void);

/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  ds_comp_void_func
===========================================================================*/
/*!
@brief
  Generic comparator used to compare two void pointers. Used in list 
  operations.

@return
  long int - 0 if pointers are equal, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static long int 
ds_comp_void_func (const void * first, const void * second)
{
    if (first == second) {
        return 0;
    }
    return -1;
}

/*===========================================================================
  FUNCTION  ds_comp_ev_cb_info_func
===========================================================================*/
/*!
@brief
  Comparator used to compare two event callback info structure pointers 
  for their iface IDs. Used in list operations.

@return
  long int - 0 if iface IDs are equal, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static long int 
ds_comp_ev_cb_info_func (const void * first, const void * second)
{
    const ds_net_ev_cb_info_t * f, * s;

    f = first;
    s = second;

    return (long int)(f->iface_id - s->iface_id);
}

/*===========================================================================
  FUNCTION  ds_fdset_add
===========================================================================*/
/*!
@brief
  Adds given socket descriptor to the fd set representing all socket 
  descriptors currently allocated.

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
ds_fdset_add (int sysfd)
{
    ds_fdset_info_t * fds_inf;
    fds_inf = &ds_sock_ctrl.ds_fdset_info;

    if (sysfd > fds_inf->maxfd) {
        fds_inf->maxfd = sysfd;
    }
    FD_SET(sysfd, &fds_inf->rfds);
    FD_SET(sysfd, &fds_inf->wfds);
    FD_SET(sysfd, &fds_inf->xfds);
    ++(fds_inf->nfds);
    return;
}

/*===========================================================================
  FUNCTION  ds_fdset_del
===========================================================================*/
/*!
@brief
  Deletes given socket descriptor from the fd set representing all socket 
  descriptors currently allocated.

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
ds_fdset_del (int sysfd)
{
    int j;
    int maxfd;
    ds_fdset_info_t * fds_inf;
    fds_inf = &ds_sock_ctrl.ds_fdset_info;

    FD_CLR(sysfd, &fds_inf->rfds);
    FD_CLR(sysfd, &fds_inf->wfds);
    FD_CLR(sysfd, &fds_inf->xfds);

    if (--(fds_inf->nfds) == 0) {
        fds_inf->maxfd = 0;
    } else if (sysfd >= fds_inf->maxfd) {
        j = sysfd - 1;
        maxfd = 0;
        do {
            if (FD_ISSET(j, &fds_inf->rfds)) {
                maxfd = j;
                break;
            }
        } while (--j >= 0);
        ds_assert(j >= 0);
        fds_inf->maxfd = maxfd;
    }
    return;
}

/*===========================================================================
  FUNCTION  ds_acb_lock
===========================================================================*/
/*!
@brief
  Wrapper function for acquiring the shared lock used for serializing access
  to ACBs.

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
ds_acb_lock (void)
{
    ds_assert(pthread_mutex_lock(&ds_sock_ctrl.ds_acb_mutx) == 0);
}

/*===========================================================================
  FUNCTION  ds_acb_unlock
===========================================================================*/
/*!
@brief
  Wrapper function for releasing the shared lock used for serializing access
  to ACBs.

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
ds_acb_unlock (void)
{
    ds_assert(pthread_mutex_unlock(&ds_sock_ctrl.ds_acb_mutx) == 0);
}

/*===========================================================================
  FUNCTION  ds_sock_lock
===========================================================================*/
/*!
@brief
  Wrapper function for acquiring the shared lock for serializing access
  to socket control info data structures.

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
ds_sock_lock (void)
{
    ds_assert(pthread_mutex_lock(&ds_sock_ctrl.ds_sock_mutx) == 0);
}

/*===========================================================================
  FUNCTION  ds_sock_unlock
===========================================================================*/
/*!
@brief
  Wrapper function for releasing the shared lock for serializing access
  to socket control info data structures.

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
ds_sock_unlock (void)
{
    ds_assert(pthread_mutex_unlock(&ds_sock_ctrl.ds_sock_mutx) == 0);
}

/*===========================================================================
  FUNCTION  ds_net_lock
===========================================================================*/
/*!
@brief
  Wrapper function for acquiring the shared lock used for serializing net 
  subsystem APIs. 

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
ds_net_lock (void)
{
    ds_assert(pthread_mutex_lock(&ds_sock_ctrl.ds_net_mutx) == 0);
}

/*===========================================================================
  FUNCTION  ds_net_unlock
===========================================================================*/
/*!
@brief
  Wrapper function for releasing the shared lock used for serializing net
  subsystem APIs.

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
ds_net_unlock (void)
{
    ds_assert(pthread_mutex_unlock(&ds_sock_ctrl.ds_net_mutx) == 0);
}

/*===========================================================================
  FUNCTION  ds_nethdl_ev_cb_init
===========================================================================*/
/*!
@brief
  Initializes data structures to manage event callback information for the 
  given network handle (ACB).

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
ds_nethdl_ev_cb_init (ds_acb_t * ds_acb_p)
{
    int i;
    ds_net_ev_cb_list_t * ev_cb_list;

    for (i = 0; i < (int)DSS_IFACE_IOCTL_REG_EVENT_MAX; ++i) 
    {
        ev_cb_list = &ds_acb_p->net_ev_cb_list[i];

        ev_cb_list->l_head = ds_dll_init(NULL);
        ds_assert(ev_cb_list->l_head);

        ev_cb_list->l_tail = ev_cb_list->l_head;
    }

    return;
}

/*===========================================================================
  FUNCTION  ds_nethdl_ev_cb_free
===========================================================================*/
/*!
@brief
  Destroys data structures and frees associated memory used for managing 
  event callback information for the specified network handle/ACB.

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
ds_nethdl_ev_cb_free (ds_acb_t * ds_acb_p)
{
    int i;
    ds_net_ev_cb_list_t * ev_cb_list;
    ds_net_ev_cb_info_t * ev_cb_info;
    ds_dll_el_t * node;

    for (i = 0; i < (int)DSS_IFACE_IOCTL_REG_EVENT_MAX; ++i)
    {
        ev_cb_list = &ds_acb_p->net_ev_cb_list[i];

        ds_assert(ev_cb_list->l_head);
        ds_assert(ev_cb_list->l_tail);

        while ((node = ds_dll_deq
                        (
                            ev_cb_list->l_head, 
                            &ev_cb_list->l_tail, 
                            (const void **)&ev_cb_info
                        )
               ))
        {
            free(ev_cb_info);
            ds_dll_free(node);
        }

        ds_dll_destroy(ev_cb_list->l_head);
    }

    return;
}

/*===========================================================================
  FUNCTION  ds_alloc_nethdl
===========================================================================*/
/*!
@brief
  Allocates a network handle and initializes data structures used in the 
  associated ACB.

@return
  int - network handle, if successfully allocated, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int 
ds_alloc_nethdl (void)
{  
    int i;
    ds_acb_t * ds_acb_p;
    ds_acb_t ** ds_acb_parr_p;
    ds_dll_el_t * newnode;

    ds_acb_lock();

    ds_acb_parr_p = ds_sock_ctrl.ds_acb_parr;

    for (i = 0; i < DS_MAX_APP; ++i) {
        if (ds_acb_parr_p[i] == NULL) {
            break;
        }
    }

    if (i == DS_MAX_APP) {
        i = -1;
        goto error;
    }

    ds_acb_p = malloc(sizeof(ds_acb_t));
    ds_assert(ds_acb_p);

    memset(ds_acb_p, 0, sizeof(ds_acb_t));
    ds_acb_p->inuse = TRUE;

    ds_acb_p->sk_lst_hd = ds_dll_init(NULL);
    ds_assert(ds_acb_p->sk_lst_hd);

    ds_acb_p->sk_lst_tl = ds_acb_p->sk_lst_hd;

    ds_acb_parr_p[i] = ds_acb_p;

    newnode = ds_dll_enq(ds_sock_ctrl.ds_acb_lst_tl, NULL, ds_acb_p);
    ds_assert(newnode);

    ds_sock_ctrl.ds_acb_lst_tl = newnode;

    ds_nethdl_ev_cb_init(ds_acb_p);

error:
    ds_acb_unlock();
    return i;
}

/*===========================================================================
  FUNCTION  ds_nethdl_call_netcb
===========================================================================*/
/*!
@brief
  Helper function to call the registered network callback for a given
  network handle.

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
ds_nethdl_call_netcb (int nethdl, dss_iface_id_type iface_id, sint15 dss_errno)
{
    dss_net_cb_fcn net_cb;
    void * user_data;

    net_cb = ds_nethdl_get_netcb(nethdl, &user_data);
    (*net_cb)((sint15)nethdl, iface_id, dss_errno, user_data);
    return;
}

/*===========================================================================
  FUNCTION  ds_nethdl_call_sockcb
===========================================================================*/
/*!
@brief
  Helper function to call the registered socket callback for a given network
  handle.

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
ds_nethdl_call_sockcb (int nethdl, int dsfd, unsigned int event)
{
    dss_sock_cb_fcn sock_cb;
    void * user_data;

    sock_cb = ds_nethdl_get_sockcb(nethdl, &user_data);
    sock_cb((sint15)nethdl, (sint15)dsfd, event, user_data);
    return;
}

/*===========================================================================
  FUNCTION  ds_nethdl_check_open_sock
===========================================================================*/
/*!
@brief
  Checks if there exists an open socket for a given network handle.

@return
  int - 0 if a socket exists, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int 
ds_nethdl_check_open_sock (int nethdl)
{
    ds_dll_el_t * node;
    const void * data;
    int rval = -1;

    ds_sock_lock();

    node = ds_nethdl_get_sk_lst_hd(nethdl);

    if (ds_dll_next(node, &data)) {
        rval = 0;
    }

    ds_sock_unlock();
    return rval;
}

/*===========================================================================
  FUNCTION  ds_nethdl_verify
===========================================================================*/
/*!
@brief
  Verifies if a given network handle is a valid one, i.e. currently allocated
  to a client.

@return
  int - 0 if the handle is valid, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int 
ds_nethdl_verify (int nethdl)
{
    if ((nethdl < 0) || (nethdl >= DS_MAX_APP) || 
        (ds_sock_ctrl.ds_acb_parr[nethdl] == NULL) ||
        (ds_sock_ctrl.ds_acb_parr[nethdl]->inuse == FALSE))
    {
        return -1;
    }
    return 0;
}

/*===========================================================================
  FUNCTION  ds_free_nethdl
===========================================================================*/
/*!
@brief
  Destructor for a network handle and the associated ACB.

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
ds_free_nethdl (int nethdl)
{
    ds_acb_t * ds_acb_p;
    ds_acb_t ** ds_acb_parr_p;
    ds_dll_el_t * node;

    ds_acb_lock();

    ds_assert(ds_nethdl_verify(nethdl) == 0);

    ds_acb_parr_p = ds_sock_ctrl.ds_acb_parr;
    ds_acb_p = ds_acb_parr_p[nethdl];

    node = ds_dll_delete
    (
        ds_sock_ctrl.ds_acb_lst_hd,
        &ds_sock_ctrl.ds_acb_lst_tl,
        ds_acb_p,
        ds_comp_void_func
    );

    ds_assert(node);
    ds_dll_free(node);

    ds_assert(ds_acb_p->sk_lst_hd == ds_acb_p->sk_lst_tl);
    ds_dll_destroy(ds_acb_p->sk_lst_hd);

    ds_nethdl_ev_cb_free(ds_acb_p);

    free(ds_acb_p);
    ds_acb_parr_p[nethdl] = NULL;

    ds_acb_unlock();
    return;
}

/*===========================================================================
  FUNCTION  ds_nethdl_add_sock_entry
===========================================================================*/
/*!
@brief
  Adds socket descriptor to the list of sockets open for a given network 
  handle.

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
ds_nethdl_add_sock_entry (int nethdl, int dsfd)
{
    ds_acb_t * ds_acb_p;
    ds_dll_el_t * node;

    ds_assert(ds_nethdl_verify(nethdl) == 0);

    ds_acb_p = ds_sock_ctrl.ds_acb_parr[nethdl];

    node = ds_dll_enq(ds_acb_p->sk_lst_tl, NULL, (void *)dsfd);
    ds_assert(node);

    ds_acb_p->sk_lst_tl = node;
    return;
}

/*===========================================================================
  FUNCTION  ds_nethdl_del_sock_entry
===========================================================================*/
/*!
@brief
  Deletes socket descriptor from the list of sockets open for a given network 
  handle.

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
ds_nethdl_del_sock_entry (int nethdl, int dsfd)
{
    ds_acb_t * ds_acb_p;
    ds_dll_el_t * node;

    ds_assert(ds_nethdl_verify(nethdl) == 0);

    ds_acb_p = ds_sock_ctrl.ds_acb_parr[nethdl];

    node = ds_dll_delete
    (
        ds_acb_p->sk_lst_hd,
        &ds_acb_p->sk_lst_tl,
        (void *)dsfd,
        ds_comp_void_func
    );

    ds_assert(node);

    ds_dll_free(node);
    return;
}

/*===========================================================================
  FUNCTION  ds_nethdl_reg_event_cb
===========================================================================*/
/*!
@brief
  Adds a network event to the list of events registered for by client for 
  a given network handle.

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
ds_nethdl_reg_event_cb 
(
    int nethdl, 
    dss_iface_id_type iface_id,
    dss_iface_ioctl_ev_cb_type * ev_cb
)
{
    ds_dll_el_t * node;
    ds_net_ev_cb_info_t ev_cb_info;
    ds_net_ev_cb_info_t * ev_cb_info_p = NULL;
    ds_net_ev_cb_list_t * ev_cb_list;
    ds_acb_t * ds_acb_p;
    int event;

    ds_acb_lock();

    event = (int)(ev_cb->event);
    ds_acb_p = ds_sock_ctrl.ds_acb_parr[nethdl];
    ev_cb_list = &ds_acb_p->net_ev_cb_list[event];

    ev_cb_info.iface_id = iface_id;

    node = ds_dll_delete
    (
        ev_cb_list->l_head,
        &ev_cb_list->l_tail,
        &ev_cb_info,
        ds_comp_ev_cb_info_func
    );

    if (node) {
        ds_log("ds_nethdl_reg_ev_cb: event %d already registered for nh %d, if %ld\n",
                event, nethdl, iface_id);
        ev_cb_info_p = (ds_net_ev_cb_info_t *) ds_dll_data(node);
        ds_dll_free(node);
    }

    if (!ev_cb_info_p) {
        ev_cb_info_p = malloc(sizeof(ds_net_ev_cb_info_t));
        ds_assert(ev_cb_info_p);
    }

    ev_cb_info_p->iface_id = iface_id;
    ev_cb_info_p->event_cb = ev_cb->event_cb;
    ev_cb_info_p->user_data = ev_cb->user_data_ptr;

    node = ds_dll_enq(ev_cb_list->l_tail, NULL, ev_cb_info_p);
    ds_assert(node);
    ev_cb_list->l_tail = node;

    ds_acb_unlock();
    return;
}

/*===========================================================================
  FUNCTION  ds_nethdl_dereg_event_cb
===========================================================================*/
/*!
@brief
  Deletes a network event from the list of events registered for by client 
  for a given network handle.

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
ds_nethdl_dereg_event_cb 
(
    int nethdl,
    dss_iface_id_type iface_id,
    dss_iface_ioctl_ev_cb_type * ev_cb
)
{
    ds_dll_el_t * node;
    ds_net_ev_cb_info_t ev_cb_info;
    ds_net_ev_cb_info_t * ev_cb_info_p = NULL;
    ds_net_ev_cb_list_t * ev_cb_list;
    ds_acb_t * ds_acb_p;
    int event;

    ds_acb_lock();

    event = (int)(ev_cb->event);
    ds_acb_p = ds_sock_ctrl.ds_acb_parr[nethdl];
    ev_cb_list = &ds_acb_p->net_ev_cb_list[event];

    ev_cb_info.iface_id = iface_id;

    node = ds_dll_delete
    (
        ev_cb_list->l_head,
        &ev_cb_list->l_tail,
        &ev_cb_info,
        ds_comp_ev_cb_info_func
    );

    if (!node) {
        ds_log("ds_nethdl_reg_ev_cb: event %d not registered for nh %d, if %ld\n",
                    event, nethdl, iface_id);
        return;
    }

    ev_cb_info_p = (ds_net_ev_cb_info_t *) ds_dll_data(node);
    ds_dll_free(node);
    free(ev_cb_info_p);

    ds_acb_unlock();
    return;
}

/*===========================================================================
  FUNCTION  ds_nethdl_call_ev_cb
===========================================================================*/
/*!
@brief
  Calls the event notification callback registered for a given event on the 
  specified network handle.

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
ds_nethdl_call_ev_cb 
(
    int nethdl, 
    dss_iface_id_type iface_id,
    dcm_iface_ioctl_event_t * event
)
{
    ds_acb_t * ds_acb_p;
    ds_dll_el_t * node;
    ds_net_ev_cb_list_t * ev_cb_list;
    ds_net_ev_cb_info_t ev_cb_info;
    ds_net_ev_cb_info_t * ev_cb_info_p;

    ds_acb_p = ds_sock_ctrl.ds_acb_parr[nethdl];
    ev_cb_list = &ds_acb_p->net_ev_cb_list[(int)(event->name)];

    ev_cb_info.iface_id = iface_id;
    node = ds_dll_search
            (
                ev_cb_list->l_head,
                &ev_cb_info,
                ds_comp_ev_cb_info_func
            );

    ds_assert(node);

    ev_cb_info_p = (ds_net_ev_cb_info_t *) ds_dll_data(node);
    ds_assert(ev_cb_info_p);
    ds_assert(ev_cb_info_p->event_cb);

    (* ev_cb_info_p->event_cb)
    (
        event->name, 
        event->info,
        ev_cb_info_p->user_data,
        nethdl, 
        iface_id
    );

    return;
}

/*===========================================================================
  FUNCTION  ds_alloc_sockfd
===========================================================================*/
/*!
@brief
  Allocates a DSS socket descriptor and allocates memory for associated 
  socket control block.

@return
  int - socket descriptor if allocation is successful, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int 
ds_alloc_sockfd (void)
{
    int i;
    ds_sockinfo_t * ds_sockinfo_p;
    ds_sockinfo_t ** ds_sockinfo_parr_p;
    ds_dll_el_t * newnode;

    ds_sockinfo_parr_p = ds_sock_ctrl.ds_sockinfo_parr;

    for (i = 0; i < DS_MAX_SOCK; ++i) {
        if (ds_sockinfo_parr_p[i] == NULL) {
            break;
        }
    }

    if (i == DS_MAX_SOCK) {
        return -1;
    }

    ds_sockinfo_p = malloc(sizeof(ds_sockinfo_t));
    ds_assert(ds_sockinfo_p);

    memset(ds_sockinfo_p, 0, sizeof(ds_sockinfo_t));
    ds_sockinfo_p->state = DS_SOCK_IDLE;
    ds_sockinfo_p->dsfd = ds_sockinfo_get_dsfd_from_index(i);

    ds_sockinfo_parr_p[i] = ds_sockinfo_p;

    newnode = ds_dll_enq(ds_sock_ctrl.ds_sk_lst_tl, NULL, ds_sockinfo_p);
    ds_assert(newnode);

    ds_sock_ctrl.ds_sk_lst_tl = newnode;

    return ds_sockinfo_p->dsfd;
}

/*===========================================================================
  FUNCTION  ds_sockinfo_verify_dsfd
===========================================================================*/
/*!
@brief
  Verifies that a given DSS socket descriptor is valid and is currently 
  allocated to a client.

@return
  int - 0 if socket descriptor is valid, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int 
ds_sockinfo_verify_dsfd (int dsfd)
{
    int indx;
    indx = ds_sockinfo_get_index_from_dsfd(dsfd);

    if ((indx < 0) || (indx >= DS_MAX_SOCK) || 
        (ds_sock_ctrl.ds_sockinfo_parr[indx] == NULL) || 
        (ds_sock_ctrl.ds_sockinfo_parr[indx]->state == DS_SOCK_INVALID))
    {
        return -1;
    }
    return 0;
}

/*===========================================================================
  FUNCTION  ds_free_sockfd
===========================================================================*/
/*!
@brief
  Deallocates a DSS socket descriptor and frees memory allocated for 
  associated socket control block.

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
ds_free_sockfd (int dsfd)
{
    int indx;
    ds_sockinfo_t * ds_sockinfo_p;
    ds_sockinfo_t ** ds_sockinfo_parr_p;
    ds_dll_el_t * node;

    indx = ds_sockinfo_get_index_from_dsfd(dsfd);
    ds_assert(ds_sockinfo_verify_dsfd(dsfd) == 0);

    ds_sockinfo_parr_p = ds_sock_ctrl.ds_sockinfo_parr;
    ds_sockinfo_p = ds_sockinfo_parr_p[indx];

    node = ds_dll_delete
    (
        ds_sock_ctrl.ds_sk_lst_hd,
        &ds_sock_ctrl.ds_sk_lst_tl,
        ds_sockinfo_p,
        ds_comp_void_func
    );

    ds_assert(node);
    ds_dll_free(node);

    free(ds_sockinfo_p);
    ds_sockinfo_parr_p[indx] = NULL;

    return;
}

#if 0 
/* The following function is not used currently but is in there for 
   future use. 
*/
/*===========================================================================
  FUNCTION  ds_sockinfo_get_dsfd
===========================================================================*/
/*!
@brief
  Returns the DSS socket descriptor corresponding to a given system socket
  descriptor.

@return
  int - DSS socket descriptor if found, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int 
ds_sockinfo_get_dsfd (int sysfd)
{
    ds_dll_el_t * node; 
    ds_sockinfo_t * ds_sockinfo_p = NULL;

    node = ds_sock_ctrl.ds_sk_lst_hd;
    while ((node = ds_dll_next(node, (const void **)&ds_sockinfo_p))) {
        if (ds_sockinfo_p->sysfd == sysfd) {
            break;
        }
    }
    if (!node) {
        return -1;
    }
    ds_assert(ds_sockinfo_p);
    return ds_sockinfo_p->dsfd;
}
#endif /* if 0 */

/*===========================================================================
  FUNCTION  ds_sigio_hdlr
===========================================================================*/
/*!
@brief
  Signal handler for SIGIO. Posts message to socket callback thread to  
  perform processing of the signal in the callback thread context. 

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
ds_sigio_hdlr (int signo)
{
    ds_sock_cb_thrd_msg_t sigio_msg;

    if (signo != SIGIO) {
        ds_log("ds_sigio_hdlr rcvd signal %d\n", signo);
    } else {
        ds_log("ds_sigio_hdlr rcvd signal SIGIO\n");
	}

    ds_assert(signo == SIGIO);
    sigio_msg.signo = SIGIO;

    ds_sock_cb_thrd_post_msg(&sigio_msg);
    return;
}

/*===========================================================================
  FUNCTION  ds_block_sigio
===========================================================================*/
/*!
@brief
  Blocks SIGIO signal dispatch to the process. 

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
ds_block_sigio (void)
{
    sigset_t mask;

    ds_assert(sigemptyset(&mask) == 0);
    ds_assert(sigaddset(&mask, SIGIO) == 0);

    ds_assert(pthread_sigmask(SIG_BLOCK, &mask, NULL) == 0);
    return;
}

/*===========================================================================
  FUNCTION  ds_unblock_sigio
===========================================================================*/
/*!
@brief
  Unblocks SIGIO signal dispatch to the process. 

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
ds_unblock_sigio (void)
{
    sigset_t mask;

    ds_assert(sigemptyset(&mask) == 0);
    ds_assert(sigaddset(&mask, SIGIO) == 0);

    ds_assert(pthread_sigmask(SIG_UNBLOCK, &mask, NULL) == 0);
    return;
}

/*===========================================================================
  FUNCTION  ds_sock_set_async_io
===========================================================================*/
/*!
@brief
  Enables asynchronous and non-blocking IO mode on the given socket 
  descriptor. 

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
ds_sock_set_async_io (int fd)
{
    int flags;

    ds_assert(fcntl(fd, F_SETOWN, getpid()) != -1);

    flags = fcntl(fd, F_GETFL, 0);
    ds_assert(flags != -1);

    flags |= (O_NONBLOCK | O_NONBLOCK);
    ds_assert(fcntl(fd, F_SETFL, flags) != -1);

    return;
}

/*===========================================================================
  FUNCTION  ds_read_msg
===========================================================================*/
/*!
@brief
  Reads a message of a specified size from a given socket. 

@return
  int - number of bytes read

@note

  - Dependencies
    - None

  - Side Effects
    - Blocks until the specified number of bytes have been read.
*/
/*=========================================================================*/
static int 
ds_read_msg (int fd, char * buf, size_t lmsg)
{
    int nread = 0;
    size_t rem;

    rem = lmsg;

    for (;;) {
        nread = read(fd, buf, rem);

        if (nread < 0) {
            perror("ds_read_msg: error in read: ");
            continue;
        }

        if (nread == 0) {
            break;
        }

        ds_assert((size_t)nread <= rem);
        rem -= (size_t)nread;
        buf += nread;

        if (rem == 0) {
            break;
        }
    }

    return (int)(lmsg - rem);
}

/*===========================================================================
  FUNCTION  ds_write_msg
===========================================================================*/
/*!
@brief
  Write a message of a specified size to a given socket. 

@return
  int - number of bytes written

@note

  - Dependencies
    - None

  - Side Effects
    - Blocks until the specified number of bytes have been written.
*/
/*=========================================================================*/
static int 
ds_write_msg (int fd, const char * buf, size_t lmsg)
{
    int nwrite = 0;
    size_t rem;

    rem = lmsg;

    for (;;) {
        nwrite = write(fd, buf, rem);

        if (nwrite < 0) {
            perror("ds_write_msg: error in write: ");
            break;
        }

        if (nwrite == 0) {
            continue;
        }

        ds_assert((size_t)nwrite <= rem);
        rem -= (size_t)nwrite;
        buf += nwrite;

        if (rem == 0) {
            break;
        }
    }

    return (int)(lmsg - rem);
}

/*===========================================================================
  FUNCTION  ds_sock_cb_thrd_post_msg
===========================================================================*/
/*!
@brief
  Posts given message to socket callback thread on the associated pipe. 

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
ds_sock_cb_thrd_post_msg (const ds_sock_cb_thrd_msg_t * msg)
{
    int nwrite;

    nwrite = ds_write_msg(ds_sock_cb_thrd_info.pipefd[1], (const char *)msg, sizeof(*msg));

    ds_assert(nwrite == (int)sizeof(*msg));
    return;
}

/*===========================================================================
  FUNCTION  ds_sock_cb_thrd_rcvd_sigio_msg
===========================================================================*/
/*!
@brief
  Performs processing of a SIGIO message. Dispatches socket callbacks for 
  IO events that have occured to interested clients. 

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
ds_sock_cb_thrd_rcvd_sigio_msg (const ds_sock_cb_thrd_msg_t * msg)
{
    fd_set rfds;
    fd_set wfds;
    fd_set xfds;
    int n;
    struct timeval timeout;
    int nethdl;
    ds_dll_el_t * node;
    ds_sockinfo_t * ds_sockinfo_p;
    int dsfd;
    int sysfd;
    int match;
    unsigned int pend_ev;
    (void)msg;
    
    ds_sock_lock();

    timeout.tv_sec = timeout.tv_usec = 0;
    rfds = ds_sock_ctrl.ds_fdset_info.rfds;
    wfds = ds_sock_ctrl.ds_fdset_info.wfds;
    xfds = ds_sock_ctrl.ds_fdset_info.xfds;
    n = ds_sock_ctrl.ds_fdset_info.maxfd + 1;

    if ((n = select(n, &rfds, &wfds, &xfds, &timeout)) < 0) {
        perror("select returned -1!");
        goto error;
    } else if (n == 0) {
        ds_log("select returned 0\n");
    }

    node = ds_sock_ctrl.ds_sk_lst_hd;
    while ((n > 0) && (node = ds_dll_next(node, (const void **)&ds_sockinfo_p))) {
        match = FALSE;
        pend_ev = 0;
        dsfd = ds_sockinfo_p->dsfd;
        sysfd = ds_sockinfo_get_sysfd(dsfd);

        ds_assert(ds_sockinfo_verify_dsfd(dsfd) == 0);

        if (FD_ISSET(sysfd, &rfds)) {
            match = TRUE;
            if (ds_sockinfo_get_state(dsfd) == DS_SOCK_LISTENING) {
                pend_ev |= DS_ACCEPT_EVENT;
            } else {
                pend_ev |= DS_READ_EVENT;
            }
        }
        if (FD_ISSET(sysfd, &wfds)) {
            match = TRUE;
            pend_ev |= DS_WRITE_EVENT;
        }
        if (FD_ISSET(sysfd, &xfds)) {
            ds_log("exception event set for fd %d\n", sysfd);
        }
        if (match == TRUE) {
            --n;
            ds_sockinfo_add_pend_ev(dsfd, pend_ev);
            if (pend_ev & ds_sockinfo_get_wait_ev(dsfd)) {
                nethdl = ds_sockinfo_get_nethdl(dsfd);
                ds_nethdl_call_sockcb(nethdl, dsfd, pend_ev);
            }
        }
    }

error:
    ds_sock_unlock();
    return;
}

/*===========================================================================
  FUNCTION  ds_sock_cb_thrd_main
===========================================================================*/
/*!
@brief
  Main function for the socket callback thread. Waits for signal messages on
  the associated pipe and processes them in order. 

@return
  void * - This function does not return

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static void * 
ds_sock_cb_thrd_main (void * dummy)
{
    int nread;
    ds_sock_cb_thrd_msg_t msg;
    (void)dummy;
    
    for (;;) {
        nread = ds_read_msg(ds_sock_cb_thrd_info.pipefd[0], (char *)&msg, sizeof(msg));

        ds_assert(nread == (int)sizeof(msg));
        ds_assert(msg.signo == SIGIO);

        ds_sock_cb_thrd_rcvd_sigio_msg(&msg);
    }
}

/*===========================================================================
  FUNCTION  ds_sock_cb_thrd_init
===========================================================================*/
/*!
@brief
  Spawns the socket callback thread.  

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
ds_sock_cb_thrd_init (void)
{
    ds_assert(pipe(ds_sock_cb_thrd_info.pipefd) == 0);

    ds_assert(pthread_mutex_init(&ds_sock_cb_thrd_info.mutx, NULL) == 0);
    ds_assert(pthread_cond_init(&ds_sock_cb_thrd_info.cond, NULL) == 0);

    ds_assert(pthread_create
        (
            &ds_sock_cb_thrd_info.thrd, 
            NULL, 
            ds_sock_cb_thrd_main, 
            NULL
        ) == 0); 

    return;
}

/*===========================================================================
  FUNCTION  ds_get_sys_shut_mode
===========================================================================*/
/*!
@brief
  Returns system socket shutdown mode corresponding to a given DSS socket 
  shutdown mode.  

@return
  int - system socket shutdown mode

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
ds_get_sys_shut_mode (uint16 how)
{
    int mode;

    switch (how) {
    case DSS_SHUT_RD:
        mode = SHUT_RD;
        break;
    case DSS_SHUT_WR:
        mode = SHUT_WR;
        break;
    case DSS_SHUT_RDWR:
        mode = SHUT_RDWR;
        break;
    default:
        abort();
    }
    return mode;
}

/*===========================================================================
  FUNCTION  dss_get_dss_errno_for_op
===========================================================================*/
/*!
@brief
  Given a system error number and the socket operation that resulted in the 
  error, returns the corresponding DSS error number. This function only
  handles error codes that map to different DSS error numbers depending on 
  the socket operation.

@return
  sint15 - DSS error number

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static sint15
dss_get_dss_errno_for_op (int syserrno, ds_sock_op_t op)
{
    sint15 dss_errno = DS_EINVAL;

    switch (syserrno) {
    case EAGAIN:
        switch (op) {
        case DS_SOCK_OP_CONNECT:
            dss_errno = DS_EINVAL;
            break;
        case DS_SOCK_OP_READ:
        case DS_SOCK_OP_WRITE:
            dss_errno = DS_EWOULDBLOCK;
            break;
		default:
			break;
        }
        break;
    case EINPROGRESS:
        switch (op) {
        case DS_SOCK_OP_CONNECT:
            dss_errno = DS_EWOULDBLOCK;
            break;
        default:
            dss_errno = DS_EINPROGRESS;
            break;
        }
        break;
    default:
        dss_errno = DS_EINVAL;
    }
    return dss_errno;
}

/*===========================================================================
  FUNCTION  dss_get_dss_errno
===========================================================================*/
/*!
@brief
  Given a system error number and the socket operation that resulted in the 
  error, returns the corresponding DSS error number. 

@return
  sint15 - DSS error number

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static sint15 
dss_get_dss_errno (int syserrno, ds_sock_op_t op)
{
    sint15 dss_errno;
    switch (syserrno) {
    case EACCES:
    case EPERM:
        dss_errno = DS_EACCES;
        break;
    case EADDRINUSE:
        dss_errno = DS_EADDRINUSE;
        break;
    case EAFNOSUPPORT:
        dss_errno = DS_EAFNOSUPPORT;
        break;
    case EBADF:
        dss_errno = DS_EBADF;
        break;
    case ECONNREFUSED:
        dss_errno = DS_ECONNREFUSED;
        break;
    case EINPROGRESS:
        dss_errno = dss_get_dss_errno_for_op(syserrno, op);
        break;
    case EFAULT:
        dss_errno = DS_EFAULT;
        break;
    case EISCONN:
        dss_errno = DS_EISCONN;
        break;
    case EMSGSIZE:
        dss_errno = DS_EMSGSIZE;
        break;
    case ENETUNREACH:
        dss_errno = DS_ENETUNREACH;
        break;
    case ENFILE:
    case EMFILE:
        dss_errno = DS_EMFILE;
        break;
    case ENOBUFS:
    case ENOMEM:
        dss_errno = DS_ENOMEM;
        break;
    case ENOTCONN:
        dss_errno = DS_ENOTCONN;
        break;
    case EOPNOTSUPP:
        dss_errno = DS_EOPNOTSUPP;
        break;
    case EPIPE:
        dss_errno = DS_EPIPE;
        break;
    case EPROTONOSUPPORT:
        dss_errno = DS_EPROTONOSUPPORT;
        break;
    case ETIMEDOUT:
        dss_errno = DS_ETIMEDOUT;
        break;
    case EAGAIN:
        dss_errno = dss_get_dss_errno_for_op(syserrno, op);
        break;
    case EALREADY:
    case EFBIG:
    case EINTR:
    case EINVAL:
    case EIO:
    case EISDIR:
    case ENOSPC:
    case ENOTSOCK:
    default:
        dss_errno = DS_EINVAL;
    }
    return dss_errno;
}

/*===========================================================================
  FUNCTION  ds_get_sys_optname
===========================================================================*/
/*!
@brief
  Given a DSS socket option returns the corresponding system socket option 
  code. 

@return
  int - system socket option code

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
ds_get_sys_optname (int optname, void * optval)
{
    int sysoptname;
    struct linger * lingeropt;

    switch (optname) {
    case DSS_SO_LINGER:
        sysoptname = SO_LINGER;
        lingeropt = (struct linger *)optval;
        lingeropt->l_linger /= 1000; /* convert from ms to seconds */
        break;
    default:
        return -1;
    }
    return sysoptname;
}

/*===========================================================================
  FUNCTION  ds_get_sys_socklevel
===========================================================================*/
/*!
@brief
  Given a DSS socket operation level returns the corresponding system socket 
  operation level. 

@return
  int - system socket operation level

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int 
ds_get_sys_socklevel (int level)
{
    int syslevel;

    switch (level) {
    case DSS_SOL_SOCKET:
        syslevel = SOL_SOCKET;
        break;
    default:
        return -1;
    }
    return syslevel;
}

/*===========================================================================
  FUNCTION  ds_dcm_net_cb_fcn
===========================================================================*/
/*!
@brief
  Callback function registered with DSC/DCM for receiving notification of 
  network events. 

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
ds_dcm_net_cb_fcn
(
  int               dcm_nethdl,                           /* Application id */
  dcm_iface_id_t    iface_id,                     /* Interfcae id structure */
  int               dss_errno, /* type of network error, ENETISCONN, ENETNONET.*/
  void            * net_cb_user_data                /* Call back User data  */
)
{
    int nethdl;
    dss_iface_id_type dss_iface_id;

    nethdl = (int)net_cb_user_data;
    dss_iface_id = iface_id;

    /* Debug */
    ds_log("in ds_dcm_net_cb_fcn, nethdl = %d, iface_id = %ld\n", 
            nethdl, iface_id);

    ds_acb_lock();

    if (ds_nethdl_verify(nethdl) < 0) {
        ds_log("ds_dcm_net_cb_fcn: invalid nethdl %d\n", nethdl);
        return;
    }

    ds_assert(dcm_nethdl == ds_nethdl_get_dcm_nethdl(nethdl));
    ds_nethdl_call_netcb(nethdl, dss_iface_id, (sint15)dss_errno);

    ds_acb_unlock();
    return;
}

/*===========================================================================
  FUNCTION  ds_dcm_iface_ioctl_event_cb_fcn
===========================================================================*/
/*!
@brief
  Callback function registered with DSC/DCM for receiving notification of 
  events that clients have explicitly registered for. 

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
ds_dcm_iface_ioctl_event_cb_fcn
(
  int                              dcm_nethandle,
  dcm_iface_id_t                   iface_id,
  dcm_iface_ioctl_event_t        * event,
  void                           * user_data
)
{
    int nethdl;

    ds_acb_lock();

    nethdl = (int)user_data;
    if (ds_nethdl_verify(nethdl) < 0) {
        ds_log("ds_dcm_iface_ioctl_event_cb_fcn: invalid nethdl %d\n", nethdl);
        return;
    }

    ds_assert(dcm_nethandle == ds_nethdl_get_dcm_nethdl(nethdl));
    ds_nethdl_call_ev_cb(nethdl, iface_id, event);

    ds_acb_unlock();
    return;
}

/*===========================================================================
  FUNCTION  ds_set_dcm_net_policy
===========================================================================*/
/*!
@brief
  Given a dss_net_policy populates the dcm_net_policy structure accordingly.

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
ds_set_dcm_net_policy 
(
    dcm_net_policy_info_t * dcm_net_policy, 
    dss_net_policy_info_type * dss_net_policy
)
{
    dss_net_policy_info_type default_dss_net_policy;

    ds_assert(dcm_net_policy);
    memset(dcm_net_policy, 0, sizeof(dcm_net_policy_info_t));

    if (!dss_net_policy) {
        dss_net_policy = &default_dss_net_policy;
        dss_init_net_policy_info(dss_net_policy);
    }

    dcm_net_policy->policy_flag = dss_net_policy->policy_flag;
    dcm_net_policy->iface.kind = dss_net_policy->iface.kind;
    dcm_net_policy->iface.info.id = dss_net_policy->iface.info.id;
    dcm_net_policy->iface.info.name = dss_net_policy->iface.info.name;
    dcm_net_policy->umts.pdp_profile_num = dss_net_policy->umts.pdp_profile_num;
    dcm_net_policy->cdma.data_session_profile_id = 
      dss_net_policy->cdma.data_session_profile_id;

    /* Copy APN for APN override support */
    if (DSS_UMTS_APN_MAX_LEN < dss_net_policy->umts.apn.length) {
        ds_log("ds_set_dcm_net_policy: APN length %d too long",
               dss_net_policy->umts.apn.length);
        goto error;
    }

    dcm_net_policy->umts.apn.length = dss_net_policy->umts.apn.length;
    memcpy
    (
        dcm_net_policy->umts.apn.name, 
        dss_net_policy->umts.apn.name,
        dss_net_policy->umts.apn.length
    );
    /* copy username */
    if (DSS_STRING_MAX_LEN < dss_net_policy->username.length) {
        ds_log("ds_set_dcm_net_policy: username length %d too long",
               dss_net_policy->username.length);
        goto error;
    }
    dcm_net_policy->username.length = dss_net_policy->username.length;
    memcpy
    (
        dcm_net_policy->username.value,
        dss_net_policy->username.value,
        dss_net_policy->username.length
    );
    /* copy password */
    if (DSS_STRING_MAX_LEN < dss_net_policy->password.length) {
        ds_log("ds_set_dcm_net_policy: password length %d too long",
               dss_net_policy->password.length);
        goto error;
    }
    dcm_net_policy->password.length = dss_net_policy->password.length;        
    memcpy
    (
        dcm_net_policy->password.value,
        dss_net_policy->password.value,
        dss_net_policy->password.length
    );
    /* copy auth_pref */
    if (dss_net_policy->auth_pref >= DSS_AUTH_PREF_MAX) {
        ds_log("ds_set_dcm_net_policy: invalid auth_pref %d\n", 
               dss_net_policy->auth_pref);
        goto error;
    }
    dcm_net_policy->auth_pref = dss_net_policy->auth_pref;
    dcm_net_policy->data_call_origin = dss_net_policy->data_call_origin;
        
error:
    return;
}

#ifndef FEATURE_DS_NO_DCM

/*===========================================================================
  FUNCTION  ds_sock_get_uds_srvr_path
===========================================================================*/
/*!
@brief
  Returns the local address to use for the unix domain server used to 
  pass socket descriptors to DSC/DCM.

@return
  const char * - pathname to use as local address

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static const char * 
ds_sock_get_uds_srvr_path (void)
{
    static char uds_srvr_path[DS_MAX_UDS_PATH_LEN];
    static char * uds_srvr_path_p = NULL;

    if (!uds_srvr_path_p) {
        (void)snprintf
        (
            uds_srvr_path, 
            DS_MAX_UDS_PATH_LEN,
            "/tmp/dss_uds_srvr.%d",
            getpid()
        );

        uds_srvr_path_p = uds_srvr_path;
    }

    return uds_srvr_path_p;
}

dss_iface_id_type
dss_get_iface_id_nolock
(
  sint15  dss_nethandle
)
{
    int dcm_nethdl;
    dss_iface_id_type iface_id = DSS_IFACE_INVALID_ID; 

    if (ds_nethdl_verify(dss_nethandle) < 0) {
        goto error;
    }

    dcm_nethdl = ds_nethdl_get_dcm_nethdl(dss_nethandle);
    iface_id = dcm_get_iface_id(dcm_nethdl);

error:
    return iface_id;
}

static int 
dss_iface_ioctl_nolock
(
  dss_iface_id_type        iface_id,
  dss_iface_ioctl_type     ioctl_name,
  void                   * argval_ptr,
  sint15                 * dss_errno
)
{
    int rval = -1;
    dcm_iface_ioctl_t dcm_ioctl;
    int dcm_nethdl;
    int dss_nethdl = -1;
    int errnum;
    size_t size;

    union {
        dss_iface_ioctl_ev_cb_type * ev_cb;
        dss_iface_ioctl_bind_sock_to_iface_type * bind_sock_to_iface_info;
        dss_iface_ioctl_device_name_type * device_name_info;
    } ioctl_info;

    dcm_ioctl.name = ioctl_name;

    switch (ioctl_name) {
    case DSS_IFACE_IOCTL_REG_EVENT_CB:
    case DSS_IFACE_IOCTL_DEREG_EVENT_CB:
        ioctl_info.ev_cb = argval_ptr;

        dss_nethdl = ioctl_info.ev_cb->dss_nethandle;
        if (ds_nethdl_verify(dss_nethdl) < 0) {
            *dss_errno = DS_EBADF;
            goto error;
        }

        dcm_nethdl = ds_nethdl_get_dcm_nethdl(dss_nethdl);
        dcm_ioctl.info.event_info.dcm_nethandle = dcm_nethdl;
        dcm_ioctl.info.event_info.name = ioctl_info.ev_cb->event; 
        break;
    case DSS_IFACE_IOCTL_BIND_SOCK_TO_IFACE:
        ioctl_info.bind_sock_to_iface_info = argval_ptr;

        dcm_ioctl.info.bind_sock_to_iface_info = 
            *(ioctl_info.bind_sock_to_iface_info);
        break;
    default:
        break;
    }

    if (DSC_OP_FAIL == dcm_iface_ioctl(iface_id, &dcm_ioctl, &errnum)) {
        *dss_errno = (sint15)errnum;
        goto error;
    }

    switch (ioctl_name) {
    case DSS_IFACE_IOCTL_REG_EVENT_CB:
        ds_nethdl_reg_event_cb(dss_nethdl, iface_id, ioctl_info.ev_cb);
        size = 0;
        break;
    case DSS_IFACE_IOCTL_DEREG_EVENT_CB:
        ds_nethdl_dereg_event_cb(dss_nethdl, iface_id, ioctl_info.ev_cb);
        size = 0;
        break;
    case DSS_IFACE_IOCTL_GET_IPV4_ADDR:
        size = sizeof(dss_iface_ioctl_ipv4_addr_type);
        break;
    case DSS_IFACE_IOCTL_GET_IPV4_PRIM_DNS_ADDR:
        size = sizeof(dss_iface_ioctl_ipv4_prim_dns_addr_type);
        break;
    case DSS_IFACE_IOCTL_GET_IPV4_SECO_DNS_ADDR:
        size = sizeof(dss_iface_ioctl_ipv4_seco_dns_addr_type);
        break;
    case DSS_IFACE_IOCTL_GET_IPV4_GATEWAY_ADDR:
        size = sizeof(dss_iface_ioctl_ipv4_gateway_addr_type);
        break;
    case DSS_IFACE_IOCTL_GET_STATE:
        size = sizeof(dss_iface_ioctl_state_type);
        break;
    case DSS_IFACE_IOCTL_GET_PHYS_LINK_STATE:
        size = sizeof(dss_iface_ioctl_phys_link_state_type);
        break;
    case DSS_IFACE_IOCTL_GET_IFACE_NAME:
        size = sizeof(dss_iface_ioctl_iface_name_type);
        break;
    case DSS_IFACE_IOCTL_BIND_SOCK_TO_IFACE:
        size = 0;
        break;
    case DSS_IFACE_IOCTL_GET_DEVICE_NAME:
        size = sizeof(dss_iface_ioctl_device_name_type);
        break;
    case DSS_IFACE_IOCTL_GO_DORMANT:
    case DSS_IFACE_IOCTL_GO_ACTIVE:
    case DSS_IFACE_IOCTL_DORMANCY_INDICATIONS_ON:
    case DSS_IFACE_IOCTL_DORMANCY_INDICATIONS_OFF:
        size = 0;
        break;
    case DSS_IFACE_IOCTL_GET_CURRENT_DATA_BEARER:
        size = sizeof(dss_iface_ioctl_data_bearer_tech_type);
        break;
    default:
        *dss_errno = DS_EINVAL;
        goto error;
    }

    memcpy(argval_ptr, &dcm_ioctl.info, size);
    rval = 0;

error:
    return rval;
}

/*===========================================================================
  FUNCTION  dss_bind_socket_to_iface
===========================================================================*/
/*!
@brief
  Communicates with DSC/DCM to bind given socket to specified IFACE.

@return
  sint15 - DSS_SUCCESS if successful, DSS_ERROR otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static sint15
dss_bind_socket_to_iface
(
    int sfd, 
    dss_iface_id_type if_id
)
{
    sint15 rval = DSS_ERROR;
    sint15 dss_errno;
    dss_iface_ioctl_bind_sock_to_iface_type bind_info;
    const char * uds_path;
    int ufd;
    struct sockaddr_un clntaddr;
    socklen_t clntaddr_len;

    if (if_id == DSS_IFACE_INVALID_ID) {
        goto error;
    }

    memset(&bind_info, 0, sizeof(bind_info));
    uds_path = ds_sock_get_uds_srvr_path();
    (void)strlcpy(bind_info.uds_path, uds_path, DS_MAX_UDS_PATH_LEN);

    if ((ufd = ds_open_uds_srvr(uds_path)) < 0) {
        ds_log("dss_bind_socket_to_iface: ds_open_uds_srvr failed!\n");
        goto error;
    }

    if (dss_iface_ioctl_nolock
        (
            if_id, 
            DSS_IFACE_IOCTL_BIND_SOCK_TO_IFACE, 
            &bind_info, 
            &dss_errno
        ) < 0)
    {
        ds_log("dss_bind_socket_to_iface: dss_iface_ioctl failed!\n");
        close(ufd);
        goto error;
    }

    clntaddr_len = sizeof(clntaddr);
    memset(&clntaddr, 0, clntaddr_len);

    if (ds_recv_initial_handshake_over_uds(ufd, &clntaddr, &clntaddr_len) < 0) {
        ds_log("dss_bind_socket_to_iface: recv_initial_handshake failed!\n");
        close(ufd);
        goto error;
    }

    if (ds_connect_to_uds_clnt(ufd, &clntaddr, clntaddr_len) < 0) {
        ds_log("dss_bind_socket_to_iface: ds_connect_to_uds_clnt failed!\n");
        close(ufd);
        goto error;
    }

    if (ds_send_fd_over_uds(ufd, sfd) < 0) {
        ds_log("dss_bind_socket_to_iface: send_fd failed!\n");
        close(ufd);
        goto error;
    }

    if (ds_recv_final_handshake_over_uds(ufd) < 0) {
        ds_log("dss_bind_socket_to_iface: recv_final_handshake failed\n");
        close(ufd);
        goto error;
    }

    close(ufd);
    rval = DSS_SUCCESS;

error:
    return rval;
}

/*===========================================================================
  FUNCTION  dss_bind_socket_to_net
===========================================================================*/
/*!
@brief
  Communicates with DSC/DCM to bind given socket to the corresponding IFACE
  that the specified network handle is associated with.

@return
  sint15 - DSS_SUCCESS if successful, DSS_ERROR otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static sint15
dss_bind_socket_to_net
(
    int     sfd,
    sint15  dss_nh
)
{
    sint15 rval = DSS_ERROR;
    dss_iface_id_type if_id;

    if_id = dss_get_iface_id_nolock(dss_nh);
    if (if_id == DSS_IFACE_INVALID_ID) {
        goto error;
    }

    rval = dss_bind_socket_to_iface(sfd, if_id);

error:
    return rval;
}

#endif /* !FEATURE_DS_NO_DCM */

/*===========================================================================
  FUNCTION  dss_getnextevent_for_sock
===========================================================================*/
/*!
@brief
  Returns a pending event on the socket. Only returns events that client is
  interested in receiving.

@return
  sint31 - socket event

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static unsigned int
dss_getnextevent_for_sock (int sockfd)
{
    unsigned int pend_ev, wait_ev; 
    unsigned int next_ev;

    pend_ev = ds_sockinfo_get_pend_ev(sockfd);
    wait_ev = ds_sockinfo_get_wait_ev(sockfd);

    if ((next_ev = wait_ev & pend_ev)) {
        if (next_ev & DS_READ_EVENT) {
            next_ev = DS_READ_EVENT;
        } else if (next_ev & DS_WRITE_EVENT) {
            next_ev = DS_WRITE_EVENT;
        } else if (next_ev & DS_ACCEPT_EVENT) {
            next_ev = DS_ACCEPT_EVENT;
        } else if (next_ev & DS_CLOSE_EVENT) {
            next_ev = DS_CLOSE_EVENT;
        } else {
            abort();
        }
        ds_sockinfo_clr_pend_ev(sockfd, next_ev);
        ds_sockinfo_clr_wait_ev(sockfd, next_ev);
    }
    return next_ev;
}

/*===========================================================================
  FUNCTION  dss_init
===========================================================================*/
/*!
@brief
  Initializes DSS library instance. Note that this function must be called 
  before using any DSS services. 
  
@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - Initializes ONCRPC and socket callback threads.
*/
/*=========================================================================*/
static void
dss_init (void)
{
    if (ds_init_done == TRUE) {
        return;
    }

    ds_net_lock();

    if (ds_init_done != TRUE) 
    {
        struct sigaction sigio_action;
        int dcm_clnt_hdl;

        /* Initialize logging to stderr, if needed */
        if (!ds_get_logfp()) {
            ds_log_init(stderr);
        }

        #ifndef FEATURE_DS_LINUX_NO_RPC

        oncrpc_init();
        dsc_dcm_apicb_app_init();
        oncrpc_task_start();

        #endif

        #ifndef FEATURE_DS_NO_DCM

        /* Register with DCM */
        dcm_clnt_hdl = dcm_get_clnt_handle();
        ds_set_dcm_clnt_hdl(dcm_clnt_hdl);

        #endif

        sigio_action.sa_handler = ds_sigio_hdlr;
		ds_assert(sigemptyset(&sigio_action.sa_mask) == 0);
        /* sigio_action.sa_flags = SA_RESTART; */
        sigio_action.sa_flags = 0;
        ds_assert(sigaction(SIGIO, &sigio_action, NULL) == 0);

        ds_sock_cb_thrd_init();

        ds_init_done = TRUE;
    }

    ds_net_unlock();
    return;
}

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dss_init_net_policy_info
===========================================================================*/
/*!
@brief
  Populates the policy info structure with default values.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - Initializes the fields in the policy info data structure.
*/
/*=========================================================================*/
void 
dss_init_net_policy_info
(
  dss_net_policy_info_type * policy_info_ptr      /* policy info structure */
)
{
    /* This function may be the first function call into DSS. Initialize
    ** if needed. 
    */
    dss_init();

    ds_log_func_entry();

    ds_assert(policy_info_ptr);
    memset(policy_info_ptr, 0, sizeof(dss_net_policy_info_type));

    policy_info_ptr->iface.kind = DSS_IFACE_NAME;
    policy_info_ptr->iface.info.name = DSS_IFACE_ANY_DEFAULT;
    policy_info_ptr->policy_flag = DSS_IFACE_POLICY_ANY;
    policy_info_ptr->auth_pref = DSS_AUTH_PREF_NOT_SPECIFIED;
    policy_info_ptr->data_call_origin  = DSS_DATA_CALL_ORIGIN_DEFAULT;

    /* policy_info_ptr->umts.pdp_profile_num = 0; */
    policy_info_ptr->umts.pdp_profile_num = 1;
    policy_info_ptr->umts.im_cn_flag = FALSE;
    policy_info_ptr->cdma.data_session_profile_id = DSS_CDMA_PROFILE_NOT_SPEC;

    /* Initialize the private cookie field. This is a coarse way to check
    ** if a net policy struct passed by client is properly inited. 
    */
    policy_info_ptr->dss_netpolicy_private.cookie = DSS_NETPOLICY_COOKIE;

    ds_log_func_exit();
    return;
}

/*===========================================================================
  FUNCTION  dss_get_app_net_policy
===========================================================================*/
/*!
@brief
  Fills in the policy info structure with the current net policy of the
  application.

@return
  sint15 - In case of successful completion, returns DSS_SUCCESS. Otherwise,
           returns DSS_ERROR and places the error number in dss_errno.

  Errno Values
  ------------
  DS_EBADAPP              Invalid application ID is specfied
  DS_EFAULT               Invalid policy_info_ptr is specified.

@note

  - Dependencies
    - None

  - Side Effects
    - Initializes the fields in the policy info data structure.
*/
/*=========================================================================*/
sint15 
dss_get_app_net_policy
(
  sint15 dss_nethandle,                                  /* Application id */
  dss_net_policy_info_type * policy_info_ptr,     /* policy info structure */
  sint15 * dss_errno                                       /* error number */
)
{
    sint15 status = DSS_ERROR;
    dss_net_policy_info_type * net_policy;

    ds_net_lock();

    if (ds_nethdl_verify(dss_nethandle) < 0) {
        *dss_errno = DS_EBADAPP;
        goto error;
    }

    if (policy_info_ptr == NULL) {
        *dss_errno = DS_EFAULT;
        goto error;
    }

    net_policy = ds_nethdl_get_net_policy(dss_nethandle);
    *policy_info_ptr = *net_policy;

    status = DSS_SUCCESS;

error:
    ds_net_unlock();
    return status;
}

/*===========================================================================
  FUNCTION  dss_set_app_net_policy
===========================================================================*/
/*!
@brief
  Sets the appliation netpolicy to the user specified value.

@return
  sint15 - In case of successful completion, returns DSS_SUCCESS. Otherwise,
           returns DSS_ERROR and places the error number in dss_errno.

  Errno Values
  ------------
  DS_EBADAPP              Invalid application ID is specfied
  DS_EFAULT               Invalid policy_info_ptr is specified.

@note

  - Dependencies
    - None

  - Side Effects
    - Sets the application netpolicy to the user specified value.
*/
/*=========================================================================*/
sint15 
dss_set_app_net_policy
(
  sint15 dss_nethandle,                                  /* Application id */
  dss_net_policy_info_type * policy_info_ptr,     /* policy info structure */
  sint15 * dss_errno                                       /* error number */
)
{
    sint15 status = DSS_ERROR;

    ds_net_lock();

    if (ds_nethdl_verify(dss_nethandle) < 0) {
        *dss_errno = DS_EBADAPP;
        goto error;
    }

    if ((policy_info_ptr == NULL) || (!ds_netpolicy_is_valid(policy_info_ptr)))
    {
        *dss_errno = DS_EFAULT;
        goto error;
    }

    ds_nethdl_set_net_policy(dss_nethandle, policy_info_ptr);
    status = DSS_SUCCESS;

error:
    ds_net_unlock();
    return status;
}

/*===========================================================================
  FUNCTION  dss_open_netlib2
===========================================================================*/
/*!
@brief
  Opens up the network library.  Assigns application ID and sets the
  application-defined callback functions to be called when library and
  socket calls would make progress. Stores the network policy info and
  uses it in further calls.

@return
  sint15 - Application ID (network handle), in case of successful completion.
           Otherwise, returns DSS_ERROR and places the error number in 
           dss_errno.

  Errno Values
  ------------
  DS_EBADAPP              Invalid application ID is specfied
  DS_EFAULT               Invalid policy_info_ptr is specified.

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
sint15 
dss_open_netlib2
(
  dss_net_cb_fcn net_cb,                      /* network callback function */
  void *  net_cb_user_data,             /* User data for network call back */
  dss_sock_cb_fcn sock_cb,                     /* socket callback function */
  void * sock_cb_user_data,              /* User data for socket call back */
  dss_net_policy_info_type * policy_info_ptr,       /* Network policy info */
  sint15 *dss_errno                               /* error condition value */
)
{
    int nethdl = DSS_ERROR;
    int dcm_nethdl;
    int dcm_clnt_hdl;
	int errnum;

    /* This function may be the first function call into DSS. Initialize
    ** if needed. (Note that the client is not required to pass a non-null 
    ** policy info ptr.)
    */
    dss_init();

    ds_log("in dss_open_netlib2\n");

    ds_net_lock();

    if (!ds_netpolicy_is_valid(policy_info_ptr)) {
        *dss_errno = DS_EFAULT;
        goto error;
    }

    if ((nethdl = ds_alloc_nethdl()) < 0) {
        *dss_errno = DS_EMAPP;
        nethdl = DSS_ERROR;
        goto error;
    }

    ds_assert(sock_cb != NULL);
    ds_nethdl_set_sockcb(nethdl, sock_cb, sock_cb_user_data);

    ds_assert(net_cb != NULL);
    ds_nethdl_set_netcb(nethdl, net_cb, net_cb_user_data);

    if (policy_info_ptr) {
        ds_nethdl_set_net_policy(nethdl, policy_info_ptr);
    }

    #ifndef FEATURE_DS_NO_DCM

    dcm_clnt_hdl = ds_get_dcm_clnt_hdl();

    dcm_nethdl = dcm_get_net_handle
        (
        dcm_clnt_hdl,
        ds_dcm_net_cb_fcn, 
        (void *)nethdl, 
		ds_dcm_iface_ioctl_event_cb_fcn,
		(void *)nethdl,
        &errnum
        );

    if (dcm_nethdl < 0) {
        ds_free_nethdl(nethdl);
        nethdl = DSS_ERROR;
		*dss_errno = (sint15)errnum;
        goto error;
    }

    ds_nethdl_set_dcm_nethdl(nethdl, dcm_nethdl);

    #endif /* !FEATURE_DS_NO_DCM */

error:
    ds_net_unlock();

    return (sint15)nethdl;
}

/*===========================================================================
  FUNCTION  dss_close_netlib
===========================================================================*/
/*!
@brief
  Closes the network library for the application.  All sockets must have
  been closed for the application, prior to closing.  If this is the last
  remaining application, the network subsytem (PPP/traffic channel) must
  have been brought down, prior to closing the network library.

@return
  sint15 - In case of successful completion, returns DSS_SUCCESS. Otherwise,
           returns DSS_ERROR and places the error number in dss_errno.

  dss_errno Values
  ----------------
  DS_EBADAPP        invalid application ID
  DS_ESOCKEXIST     there are existing sockets
  DS_ENETEXIST      the network subsystem exists

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
sint15 
dss_close_netlib
(
  sint15 dss_nethandle,                                  /* application ID */
  sint15 *dss_errno                               /* error condition value */
)
{
    int dcm_nethdl;
	int errnum;
    sint15 status = DSS_ERROR;

    ds_net_lock();

    if (ds_nethdl_verify(dss_nethandle) < 0) {
        *dss_errno = DS_EBADAPP;
        goto error;
    }

    if (ds_nethdl_check_open_sock(dss_nethandle) == 0) {
        *dss_errno = DS_SOCKEXIST;
        goto error;
    }

    #ifndef FEATURE_DS_NO_DCM

    dcm_nethdl = ds_nethdl_get_dcm_nethdl(dss_nethandle);

    if (DSC_OP_FAIL == dcm_release_net_handle(dcm_nethdl, &errnum)) {
		*dss_errno = (sint15)errnum;
        goto error;
    }

    #endif /* !FEATURE_DS_NO_DCM */

    ds_free_nethdl(dss_nethandle);
    status = DSS_SUCCESS;

error:
    ds_net_unlock();

    return status;
}

#ifndef FEATURE_DS_NO_DCM

/*===========================================================================
  FUNCTION  dss_pppopen
===========================================================================*/
/*!
@brief
  Starts up the network subsystem (CDMA data service and PPP) over the Um
  interface for all sockets.

@return
  sint15 - DSS_SUCCESS, if the network subsystem is already established.
           Otherwise, returns DSS_ERROR and places the error number in
           dss_errno.

  dss_errno Values
  ----------------
  DS_EBADAPP               invalid application ID specified
  DS_EWOULDBLOCK           the operation would block
  DS_ENETCLOSEINPROGRESS   network close in progress. The application
                           should only call dss_pppopen() after the
                           close/abort has completed.

@note

  - Dependencies
    -   dss_pppopen() cannot be called by the application if the network is 
        in the process of closing. The network layer cannot queue the open 
        request until the close is completely finished.  Therefore, the 
        application should wait for the net_callback_fn() to be called (after 
        dss_pppclose() has completed), before making a call to dss_pppopen().
        Note that a valid application ID must be specified as a parameter, 
        obtained by a successful return of dss_open_netlib().

  - Side Effects
    -   Initiates call origination and PPP negotiation.
*/
/*=========================================================================*/
sint15 
dss_pppopen
(
  sint15 dss_nethandle,                                  /* application id */
  sint15 *dss_errno                               /* error condition value */
)
{
    int dcm_nethdl;
	int errnum;
    dcm_net_policy_info_t dcm_net_policy;
    dss_net_policy_info_type * net_policy;
    sint15 status = DSS_ERROR;

    ds_log_func_entry();

    ds_net_lock();

    if (ds_nethdl_verify(dss_nethandle) < 0) {
        *dss_errno = DS_EBADAPP;
        goto error;
    }

    net_policy = ds_nethdl_get_net_policy(dss_nethandle);
    ds_set_dcm_net_policy(&dcm_net_policy, net_policy);

    dcm_nethdl = ds_nethdl_get_dcm_nethdl(dss_nethandle);

    /* Debug */
    ds_log("In dss_pppopen: calling dcm_net_open for nethdl %d\n",
            dss_nethandle);

    if (DSC_OP_FAIL == dcm_net_open(dcm_nethdl, &dcm_net_policy, &errnum)) {
		*dss_errno = (sint15)errnum;
        goto error;
    }

    status = DSS_SUCCESS;

error:
    ds_net_unlock();

    ds_log_func_exit();
    return status;
}

/*===========================================================================
  FUNCTION  dss_pppclose
===========================================================================*/
/*!
@brief
  Initiates termination to bring down PPP and the traffic channel.  Upon
  successful close of the network subsystem, invokes the network callback
  function.

@return
  sint15 - DSS_SUCCESS, if the network subsystem is already closed.
           Otherwise, returns DSS_ERROR and places the error number in
           dss_errno.

  dss_errno Values
  ----------------
  DS_EBADAPP               invalid application ID specified
  DS_EWOULDBLOCK           operation would block
  DS_ENETCLOSEINPROGRESS   network close in progress. A call to
                           dss_pppclose() has already been issued.

@note

  - Dependencies
    - None

  - Side Effects
    - Initiates termination of PPP.  Brings down PPP and traffic channel.
*/
/*=========================================================================*/
sint15 
dss_pppclose
(
  sint15 dss_nethandle,                                  /* application id */
  sint15 *dss_errno                               /* error condition value */
)
{
    int dcm_nethdl;
	int errnum;
    sint15 status = DSS_ERROR;

    ds_net_lock();

    if (ds_nethdl_verify(dss_nethandle) < 0) {
        *dss_errno = DS_EBADAPP;
        goto error;
    }

    dcm_nethdl = ds_nethdl_get_dcm_nethdl(dss_nethandle);

    if (DSC_OP_FAIL == dcm_net_close(dcm_nethdl, &errnum)) {
		*dss_errno = (sint15)errnum;
        goto error;
    }

    status = DSS_SUCCESS;

error:
    ds_net_unlock();
    return status;
}

/*===========================================================================
  FUNCTION  dss_netstatus
===========================================================================*/
/*!
@brief
  Provides status of network subsystem.  Called in response to DS_ENETDOWN
  errors.  Note that origination status is based on the last attempted
  origination.

@return
  sint15 - Returns DSS_ERROR and places the error number in dss_errno.

  dss_errno Values
  ----------------
  DS_EBADAPP               invalid application ID specified
  DS_ENETNONET             network subsystem unavailable for some unknown
                           reason
  DS_ENETISCONN            network subsystem is connected and available
  DS_ENETINPROGRESS        network subsystem establishment currently in
                           progress
  DS_ENETCLOSEINPROGRESS   network subsystem close in progress.

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
sint15 
dss_netstatus
(
  sint15 dss_nethandle,                                         /* application ID */
  sint15 *dss_errno                               /* error condition value */
)
{
    int dcm_nethdl;
	int errnum;

    ds_net_lock();

    ds_log("in dss_netstatus for nethdl %d\n", dss_nethandle);

    if (ds_nethdl_verify(dss_nethandle) < 0) {
        ds_log("dss_netstatus: nethdl %d invalid, returning EBADAPP\n",
                dss_nethandle);
        *dss_errno = DS_EBADAPP;
        goto error;
    }

    dcm_nethdl = ds_nethdl_get_dcm_nethdl(dss_nethandle);
    ds_assert(DSC_OP_FAIL == dcm_get_net_status(dcm_nethdl, &errnum));
	*dss_errno = (sint15)errnum;

error:
    ds_net_unlock();
    return DSS_ERROR;
}

/*===========================================================================
  FUNCTION  dss_get_last_netdown_reason
===========================================================================*/
/*!
@brief
  Returns the last net down reason code for a given network handle.
  
@return
  Returns DSS_ERROR if unsuccessful

@note
    
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
sint15
dss_last_netdownreason
(
  sint15                    dss_nethandle,
  dss_net_down_reason_type *reason,
  sint15                   *dss_errno
)
{
  int reason_code = 0, err_val = -1, rc = -1 , dcm_nethdl = 0; 

  if (NULL == dss_errno)
  {
    err_val = DSS_ERROR;
    goto error;
  }

  if (ds_nethdl_verify(dss_nethandle) < 0 || reason == NULL) {
    *dss_errno  = DS_EBADAPP;
    err_val = DSS_ERROR;
    goto error;
  }

  ds_net_lock();
  dcm_nethdl = ds_nethdl_get_dcm_nethdl(dss_nethandle);
  rc =  dsc_dcm_get_reason_code(dcm_nethdl,&reason_code);
  ds_net_unlock();
  if ( rc == 0)
    *reason = (dss_net_down_reason_type)reason_code;

  err_val = DSS_SUCCESS;
error:
  return err_val;
}

/*===========================================================================
  FUNCTION  dss_get_iface_id
===========================================================================*/
/*!
@brief
  Returns the iface ID associated with the given network handle.

@return
  dss_iface_id_type - iface ID if successful, DSS_IFACE_INVALID_ID otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dss_iface_id_type
dss_get_iface_id
(
  sint15  dss_nethandle
)
{
    dss_iface_id_type iface_id; 

    ds_net_lock();
    iface_id = dss_get_iface_id_nolock(dss_nethandle);
    ds_net_unlock();

    return iface_id;
}

/*===========================================================================
  FUNCTION  dss_get_iface_id_by_policy
===========================================================================*/
/*!
@brief
  Returns the interface id based on the specified network policy.

@return
  dss_iface_id_type - Iface ID, if an iface matching the specified policy 
                      exists. Otherwise returns DSS_ERROR and places the 
                      error number in dss_errno.

  dss_errno Values
  ----------------
  DS_EFAULT      Netpolicy structure is not initialized.
  DS_ENOROUTE    No interface can be determined from the network policy.

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dss_iface_id_type
dss_get_iface_id_by_policy
(
  dss_net_policy_info_type  net_policy_info,        /* Network policy info */
  sint15                  * dss_errno             /* error condition value */
)
{
	int errnum;
    dss_iface_id_type iface_id = DSS_IFACE_INVALID_ID; 
    dcm_net_policy_info_t dcm_net_policy;

    ds_log_func_entry();

    /* Note that it is not necessary to call dss_init() as the client 
    ** is required to pass a valid net policy which means that the client
    ** should have called dss_init_net_policy_info() a priori. 
    */

    if (!ds_netpolicy_is_valid(&net_policy_info)) {
        ds_log("invalid netpolicy specified\n");
        *dss_errno = DS_EFAULT;
        goto error;
    }
    
    ds_set_dcm_net_policy(&dcm_net_policy, &net_policy_info);
    iface_id = dcm_get_iface_id_by_policy(&dcm_net_policy, &errnum);

	if (iface_id == DSS_IFACE_INVALID_ID) {
		*dss_errno = (sint15)errnum;
	}

error:
    ds_log_func_exit();
    return iface_id;
}

/*===========================================================================
  FUNCTION  dss_iface_ioctl
===========================================================================*/
/*!
@brief
  Invokes specified IOCTL on the given IFACE.

@return
  int - 0 if IOCTL is successfully invoked, -1 otherwise

  dss_errno Values
  ----------------
  DS_EBADF      Returned by dss_iface_ioctl() if the specified id_ptr is 
                invalid (i.e. id_ptr does not map to a valid ps_iface_ptr).

  DS_EINVAL     Returned by dss_iface_ioctl() when the specified IOCTL 
                does not belong to the common set of IOCTLs and there is 
                no IOCTL mode handler registered for the specified interface.

  DS_EOPNOTSUPP Returned by the lower level IOCTL mode handler when specified
                IOCTL is not supported by the interface. For instance, this
                would be returned by interfaces that do not support a certain
                "iface specific common IOCTL" (i.e. these are common IOCTLs, 
                but the implementation is mode specific, for example,
                GO_DORMANT).

  DS_EFAULT     This error code is returned if the specified arguments for 
                the IOCTL are incorrect or if dss_iface_ioctl() or a mode 
                handler encounters an error while executing the IOCTL. For 
                instance, if the 1X interface cannot "GO_DORMANT" it would
                return this error.

  DS_NOMEMORY   This error code is returned if we run out of buffers during 
                execution.

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int 
dss_iface_ioctl
(
  dss_iface_id_type        iface_id,
  dss_iface_ioctl_type     ioctl_name,
  void                   * argval_ptr,
  sint15                 * dss_errno
)
{
    int rval;

    ds_net_lock();
    rval = dss_iface_ioctl_nolock(iface_id, ioctl_name, argval_ptr, dss_errno);
    ds_net_unlock();

    return rval;
}

#endif /* !FEATURE_DS_NO_DCM */

/*===========================================================================
  FUNCTION  dss_socket
===========================================================================*/
/*!
@brief
  Creates a socket and related data structures, and returns a socket 
  descriptor.

@return
  sint15 - On successful creation of a socket, this function returns socket 
           file descriptor which is a sint15 value between 0x1000 and 0x1FFF.
           On error, returns DSS_ERROR and places the error condition value
           in dss_errno.

  dss_errno Values
  ----------------
  DS_EAFNOSUPPORT     address family not supported
  DS_EBADAPP          invalid application ID
  DS_EPROTOTYPE       specified protocol invalid for socket type
  DS_ESOCKNOSUPPORT   invalid or unsupported socket parameter specified
  DS_EPROTONOSUPPORT  specified protocol not supported
  DS_EMFILE           too many descriptors open.  A socket is already open or
                      has not closed compeletely.

@note

  - Dependencies
    - The function dss_open_netlib() must be called to open the network 
      library.

  - Side Effects
    - None
*/
/*=========================================================================*/
sint15 
dss_socket
(
  sint15 dss_nethandle,                                  /* application ID */
  byte   family,                          /* Address family - AF_INET only */
  byte   type,                                              /* socket type */
  byte   protocol,                                        /* protocol type */
  sint15 *dss_errno                               /* error condition value */
)
{
    int sysfd;
    int dsfd;

    ds_net_lock();

    ds_block_sigio();
    ds_sock_lock();

    if (ds_nethdl_verify(dss_nethandle) < 0) {
        *dss_errno = DS_EBADAPP;
        dsfd = DSS_ERROR;
        goto error;
    }

    if ((dsfd = ds_alloc_sockfd()) < 0) {
        *dss_errno = DS_EMFILE;
        dsfd = DSS_ERROR;
        goto error;
    }

    #ifdef FEATURE_DS_LINUX_NO_TARGET
    if ((sysfd = *dss_errno) < 0) {
        *dss_errno = DS_EBADF;
        dsfd = DSS_ERROR;
        goto error;
    }
    #else
    if ((sysfd = socket(family, type, protocol)) < 0) {
        perror("socket call failed!");
        *dss_errno = dss_get_dss_errno(errno, DS_SOCK_OP_SOCKET);
        ds_free_sockfd(dsfd);
        dsfd = DSS_ERROR;
        goto error;
    }

	#ifndef FEATURE_DS_NO_DCM 

    if (dss_bind_socket_to_net(sysfd, dss_nethandle) == DSS_ERROR) {
        close(sysfd);
        ds_free_sockfd(dsfd);
        *dss_errno = DS_EINVAL;
        dsfd = DSS_ERROR;
        goto error;
    }
	#endif /* !FEATURE_DS_NO_DCM */

    #endif

    ds_sockinfo_set_nethdl(dsfd, dss_nethandle);
    ds_sockinfo_set_state(dsfd, DS_SOCK_IDLE);
    ds_sockinfo_set_sysfd(dsfd, sysfd);
	ds_fdset_add(sysfd);

    ds_nethdl_add_sock_entry(dss_nethandle, dsfd);

    /* Set asynchronous IO mode on socket */
    ds_sock_set_async_io(sysfd);

error:
    ds_sock_unlock();
    ds_unblock_sigio();

    ds_net_unlock();
    return (sint15)dsfd;
}

/*===========================================================================
  FUNCTION  dss_bind
===========================================================================*/
/*!
@brief
  For all client sockets, attaches a local address and port value to the
  socket.  If the call is not explicitly issued, the socket will implicitly
  bind in calls to dss_connect() or dss_sendto().  Note that this function
  does not support binding a local IP address, but rather ONLY a local port
  number.  The local IP address is assigned automatically by the sockets
  library.  The function must receive (as a parameter) a valid socket
  descriptor, implying a previous successful call to dss_socket().

@return
  sint15 - DSS_SUCCESS on success. Otherwise returns DSS_ERROR and places 
           the error condition value in dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_EOPNOTSUPP       operation not supported
  DS_EADDRINUSE       the local address is already in use.
  DS_EINVAL           the socket is already attached to a local name
  DS_EFAULT           invalid address parameter has been specified

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
sint15 
dss_bind
(
  sint15 sockfd,                                      /* socket descriptor */
  struct sockaddr *localaddr,                             /* local address */
  uint16 addrlen,                                     /* length of address */
  sint15 *dss_errno                               /* error condition value */
)
{
    int sysfd;
    sint15 rval = DSS_ERROR;

    ds_block_sigio();
    ds_sock_lock();

    if (ds_sockinfo_verify_dsfd(sockfd) < 0) {
        *dss_errno = DS_EBADF;
        goto error;
    }

    sysfd = ds_sockinfo_get_sysfd(sockfd);
    if (bind(sysfd, localaddr, addrlen) < 0) {
        *dss_errno = dss_get_dss_errno(errno, DS_SOCK_OP_BIND);
        goto error;
    }

    rval = DSS_SUCCESS;

error:
    ds_sock_unlock();
    ds_unblock_sigio();

    return rval;
}

/*===========================================================================
  FUNCTION  dss_listen
===========================================================================*/
/*!
@brief
  For TCP, this starts a passive open for connections.  Upon a
  sucessful connection, the socket callback function is invoked
  asserting DS_ACCEPT_EVENT as TRUE.  The application should respond
  with a call to dss_accept(). If a connection is recieved and there
  are no free queue slots the new connection is rejected
  (ECONNREFUSED).  The backlog queue is for ALL unaccepted sockets
  (half-open, or completely established).

  A listening UDP doesn't make sense, and as such isn't supported.
  DS_EOPNOTSUPP is returned.

@return
  sint15 - DSS_SUCCESS on success. Otherwise returns DSS_ERROR and places 
           the error condition value in dss_errno.

  dss_errno Values
  ----------------
  DS_EWOULDBLOCK      operation would block (PJ: I don't think this CAN happen)
  DS_EBADF            invalid socket descriptor is specfied
  DS_EOPNOTSUPP       The socket is not capable of listening (UDP)
  DS_EFAULT           backlog parameter is invalid
  DS_ENETDOWN         network subsystem unavailable
  DS_NOMEM            not enough memory to establish backlog connections.
  DS_EINVAL           Socket already open, closed, unbound or not one
                      you can listen on.

@note

  - Dependencies
    - Network subsystem must be established and available.

  - Side Effects
    - For TCP, initiates passive open for new connections.
*/
/*=========================================================================*/
sint15 
dss_listen
(
  sint15 sockfd,                                      /* Socket descriptor */
  sint15 backlog,                      /* Number of connections to backlog */
  sint15 *dss_errno                               /* error condition value */
)
{
    int sysfd;
    sint15 rval = DSS_ERROR;

    ds_block_sigio();
    ds_sock_lock();

    if (ds_sockinfo_verify_dsfd(sockfd) < 0) {
        *dss_errno = DS_EBADF;
        goto error;
    }
    sysfd = ds_sockinfo_get_sysfd(sockfd);
    if (listen(sysfd, backlog) < 0) {
        *dss_errno = dss_get_dss_errno(errno, DS_SOCK_OP_LISTEN);
        goto error;
    }

    ds_sockinfo_set_state(sockfd, DS_SOCK_LISTENING);
    rval = DSS_SUCCESS;

error:
    ds_sock_unlock();
    ds_unblock_sigio();

    return rval;
}

/*===========================================================================
  FUNCTION  dss_accept
===========================================================================*/
/*!
@brief
  The accept function is used on listening sockets to respond when
  DS_ACCEPT_EVENT is asserted.  The first backlog queued connection is
  removed from the queue, and bound to a new connected socket (as if
  you called dss_socket).  The newly created socket is in the
  connected state.  The listening socket is unaffect the queue size is
  maintained (ie. there is not need to call listen again.)

@return
  sint15 - socket descriptor of the new socket on success. On error DSS_ERROR
           is returned and error condition value is set in dss_errno.

  dss_errno Values
  ----------------
  DS_EWOULDBLOCK      operation would block
  DS_EBADF            invalid socket descriptor is specfied
  DS_EOPNOTSUPP       The socket is not of type SOCK_STREAM
  DS_EINVAL           Socket is not listening.
  DS_EFAULT           The addr parameter is bogus.
  DS_ENETDOWN         network subsystem unavailable
  DS_NOMEM            not enough memory to establish backlog connections.

@note

  - Dependencies
    - Network subsystem must be established and available.

  - Side Effects
    - The head backlog item from the queue of the listening socket is
      removed from that queue.
*/
/*=========================================================================*/
sint15 
dss_accept
(
  sint15 sockfd,                                      /* Socket descriptor */
  struct sockaddr *remoteaddr,                       /* new remote address */
  uint16 *addrlen,                                   /* length of servaddr */
  sint15 *dss_errno                               /* error condition value */
)
{
    int sysfd;
    int newsysfd;
    int newdsfd;
    int dss_nethandle;
	socklen_t sysaddrlen;

    ds_net_lock();

    ds_block_sigio();
    ds_sock_lock();

    if (ds_sockinfo_verify_dsfd(sockfd) < 0) {
        *dss_errno = DS_EBADF;
        newdsfd = DSS_ERROR;
        goto error;
    }
    sysfd = ds_sockinfo_get_sysfd(sockfd);
	sysaddrlen = sizeof(struct sockaddr_in);
    if ((newsysfd = accept(sysfd, remoteaddr, &sysaddrlen)) < 0) {
        *dss_errno = dss_get_dss_errno(errno, DS_SOCK_OP_ACCEPT);
        newdsfd = DSS_ERROR;
        goto error;
    }

    *addrlen = (uint16)sysaddrlen;

    if ((newdsfd = ds_alloc_sockfd()) < 0) {
        *dss_errno = DS_EMFILE;
        close(newsysfd);
        newdsfd = DSS_ERROR;
        goto error;
    }

    dss_nethandle = ds_sockinfo_get_nethdl(sockfd);
    ds_sockinfo_set_nethdl(newdsfd, dss_nethandle);
    ds_sockinfo_set_state(newdsfd, DS_SOCK_IDLE);
    ds_sockinfo_set_sysfd(newdsfd, newsysfd);
	ds_fdset_add(newsysfd);

    ds_nethdl_add_sock_entry(dss_nethandle, newdsfd);

    /* Set asynchronous IO mode on socket */
    ds_sock_set_async_io(newsysfd);

error:
    ds_sock_unlock();
    ds_unblock_sigio();

    ds_net_unlock();
    return (sint15)newdsfd;
}

/*===========================================================================
  FUNCTION  dss_connect
===========================================================================*/
/*!
@brief
  For TCP, attempts to establish the TCP connection.  Upon
  successful connection, calls the socket callback function asserting that
  the DS_WRITE_EVENT is TRUE.  The implementation does not support connected
  UDP sockets and will return an error.  The function must receive
  (as a parameter) a valid socket descriptor, implying a previous successful
  call to dss_socket().

@return
  sint15 - DSS_SUCCESS on success. On error DSS_ERROR is returned and error 
           condition value is set in dss_errno.

  dss_errno Values
  ----------------
  DS_EWOULDBLOCK      operation would block
  DS_EBADF            invalid socket descriptor is specfied
  DS_ECONNREFUSED     connection attempt refused
  DS_ETIMEDOUT        connection attempt timed out
  DS_EFAULT           addrlen parameter is invalid
  DS_EIPADDRCHANGED   IP address changed due to PPP resync
  DS_EINPROGRESS      connection establishment in progress
  DS_EISCONN          a socket descriptor is specified that is already
                      connected
  DS_ENETDOWN         network subsystem unavailable
  DS_EOPNOTSUPP       invalid server address specified
  DS_EADDRREQ         destination address is required.
  DS_NOMEM            not enough memory to establish connection

@note

  - Dependencies
    - Network subsystem must be established and available.

  - Side Effects
    - For TCP, initiates active open for connection.
*/
/*=========================================================================*/
sint15 
dss_connect
(
  sint15 sockfd,                                      /* Socket descriptor */
  struct sockaddr *servaddr,                        /* destination address */
  uint16 addrlen,                                    /* length of servaddr */
  sint15 *dss_errno                               /* error condition value */
)
{
    int sysfd;
    ds_sock_state_t sockstate;
    int so_err = 0; 
    unsigned int so_err_len = 0;
    sint15 status = DSS_ERROR;

    ds_block_sigio();
    ds_sock_lock();

    if (ds_sockinfo_verify_dsfd(sockfd) < 0) {
        *dss_errno = DS_EBADF;
        goto error;
    }
    sysfd = ds_sockinfo_get_sysfd(sockfd);

    sockstate = ds_sockinfo_get_state(sockfd);

    switch (sockstate) {
    case DS_SOCK_IDLE:
        if (connect(sysfd, servaddr, addrlen) < 0) {
            *dss_errno = dss_get_dss_errno(errno, DS_SOCK_OP_CONNECT);
            if (errno == EINPROGRESS) {
                ds_sockinfo_set_state(sockfd, DS_SOCK_CONNECTING);
                ds_sockinfo_add_wait_ev(sockfd, DS_WRITE_EVENT);
            }
            goto error;
        }
        break;
    case DS_SOCK_CONNECTING:
        if (getsockopt(sysfd, SOL_SOCKET, SO_ERROR, &so_err, (int *)&so_err_len) != 0)
        {
            abort();
        }
        if (so_err != 0) {
            *dss_errno = dss_get_dss_errno(so_err, DS_SOCK_OP_CONNECT);
            goto error;
        }
        ds_sockinfo_set_state(sockfd, DS_SOCK_IDLE);
        *dss_errno = DS_EISCONN;
        goto error;
    default:
        abort();
    }
    status = DSS_SUCCESS;

error:
    ds_sock_unlock();
    ds_unblock_sigio();

    return status;
}

/*===========================================================================
  FUNCTION  dss_write
===========================================================================*/
/*!
@brief
  Sends specified number of bytes in the buffer over the TCP transport.

@return
  sint15 - Number of bytes written, if successful. Otherwise DSS_ERROR is 
           returned and error condition value is set in dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_ENOTCONN         socket not connected
  DS_ECONNRESET       TCP connection reset by server
  DS_ECONNABORTED     TCP connection aborted due to timeout or other failure
  DS_EIPADDRCHANGED   IP address changed, causing TCP connection reset
  DS_EPIPE            broken pipe
  DS_EADDRREQ         destination address required - connectionless socket
                      did not call dss_connect()
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           application buffer no valid part of address space
  DS_EWOULDBLOCK      operation would block

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
sint15 
dss_write
(
  sint15 sockfd,                                      /* socket descriptor */
  const void *buffer,               /* user buffer from which to copy data */
  uint16 nbytes,                /* number of bytes to be written to socket */
  sint15 *dss_errno                               /* error condition value */
)
{
    int sysfd;
    int nwrote;

    ds_block_sigio();
    ds_sock_lock();

    if (ds_sockinfo_verify_dsfd(sockfd) < 0) {
        *dss_errno = DS_EBADF;
        nwrote = DSS_ERROR;
        goto error;
    }
    sysfd = ds_sockinfo_get_sysfd(sockfd);
    if ((nwrote = write(sysfd, buffer, nbytes)) < 0) {
        *dss_errno = dss_get_dss_errno(errno, DS_SOCK_OP_WRITE);
        nwrote = DSS_ERROR;
        goto error;
    } else if (nwrote == 0) {
        ds_log("write returned 0!\n");
    }

    /* Clear pending DS_WRITE_EVENT */
    ds_sockinfo_clr_pend_ev(sockfd, DS_WRITE_EVENT);

error:
    ds_sock_unlock();
    ds_unblock_sigio();

    return (sint15)nwrote;
}

/*===========================================================================
  FUNCTION  dss_read
===========================================================================*/
/*!
@brief
  Reads specified number of bytes into buffer from the TCP transport.

@return
  sint15 - Number of bytes read, if successful. A return of 0 indicates EOF.
           Otherwise DSS_ERROR is returned and error condition value is set 
           in dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_ENOTCONN         socket not connected
  DS_ECONNRESET       TCP connection reset by server
  DS_ECONNABORTED     TCP connection aborted due to timeout or other failure
  DS_EIPADDRCHANGED   IP address changed, causing TCP connection reset
  DS_EPIPE            broken pipe
  DS_EADDRREQ         destination address required - connectionless socket
                      did not call dss_connect()
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           application buffer no valid part of address space
  DS_EWOULDBLOCK      operation would block

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
sint15 
dss_read
(
  sint15 sockfd,                                      /* socket descriptor */
  void   *buffer,                     /* user buffer to which to copy data */
  uint16 nbytes,                 /* number of bytes to be read from socket */
  sint15 *dss_errno                               /* error condition value */
)
{
    int sysfd;
    int nread;

    ds_block_sigio();
    ds_sock_lock();

    if (ds_sockinfo_verify_dsfd(sockfd) < 0) {
        *dss_errno = DS_EBADF;
        nread = DSS_ERROR;
        goto error;
    }
    sysfd = ds_sockinfo_get_sysfd(sockfd);
    if ((nread = read(sysfd, buffer, nbytes)) < 0) {
        *dss_errno = dss_get_dss_errno(errno, DS_SOCK_OP_READ);
        nread = DSS_ERROR;
        goto error;
    } else if (nread == 0) {
        ds_log("read returned 0!\n");
    }

    /* Clear pending DS_READ_EVENT */
    ds_sockinfo_clr_pend_ev(sockfd, DS_READ_EVENT);

error:
    ds_sock_unlock();
    ds_unblock_sigio();

    return (sint15)nread;
}

/*===========================================================================
  FUNCTION  dss_sendto
===========================================================================*/
/*!
@brief
  Sends specified number of bytes from buffer over UDP transport.

@return
  sint15 - Number of bytes written, if successful. Otherwise DSS_ERROR is 
           returned and error condition value is set in dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_EAFNOSUPPORT     address family not supported
  DS_EWOULDBLOCK      operation would block
  DS_EADDRREQ         destination address required
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           application buffer no valid part of address space
  DS_EOPNOSUPPORT     option not supported

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
sint15 
dss_sendto
(
  sint15 sockfd,                                      /* socket descriptor */
  const void *buffer,           /* user buffer from which to copy the data */
  uint16 nbytes,                          /* number of bytes to be written */
  uint32 flags,                                                  /* unused */
  struct sockaddr *toaddr,                          /* destination address */
  uint16 addrlen,                                        /* address length */
  sint15 *dss_errno                               /* error condition value */
)
{
    int sysfd;
    int nwrote;

    ds_block_sigio();
    ds_sock_lock();

    if (ds_sockinfo_verify_dsfd(sockfd) < 0) {
        *dss_errno = DS_EBADF;
        nwrote = DSS_ERROR;
        goto error;
    }
    sysfd = ds_sockinfo_get_sysfd(sockfd);
    if ((nwrote = sendto(sysfd, buffer, nbytes, (int)flags, toaddr, addrlen)) < 0) 
    {
        *dss_errno = dss_get_dss_errno(errno, DS_SOCK_OP_WRITE);
        nwrote = DSS_ERROR;
        goto error;
    } else if (nwrote == 0) {
        ds_log("sendto returned 0!\n");
    }

    /* Clear pending DS_WRITE_EVENT */
    ds_sockinfo_clr_pend_ev(sockfd, DS_WRITE_EVENT);

error:
    ds_sock_unlock();
    ds_unblock_sigio();

    return (sint15)nwrote;
}

/*===========================================================================
  FUNCTION  dss_recvfrom
===========================================================================*/
/*!
@brief
  Reads specified number of bytes into buffer from the UDP transport.

@return
  sint15 - Number of bytes read, if successful. A return of 0 indicates EOF.
           Otherwise DSS_ERROR is returned and error condition value is set 
           in dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_EAFNOSUPPORT     address family not supported
  DS_EWOULDBLOCK      operation would block
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           application buffer no valid part of address space
  DS_EOPNOSUPPORT     option not supported

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
sint15 
dss_recvfrom
(
  sint15 sockfd,                                      /* socket descriptor */
  void   *buffer,               /* user buffer from which to copy the data */
  uint16 nbytes,                          /* number of bytes to be written */
  uint32 flags,                                                  /* unused */
  struct sockaddr *fromaddr,                        /* destination address */
  uint16 *addrlen,                                       /* address length */
  sint15 *dss_errno                               /* error condition value */
)
{
    int sysfd;
    int nread;
	unsigned int fromaddrlen;

    ds_block_sigio();
    ds_sock_lock();

    if (ds_sockinfo_verify_dsfd(sockfd) < 0) {
        *dss_errno = DS_EBADF;
        nread = DSS_ERROR;
        goto error;
    }
    sysfd = ds_sockinfo_get_sysfd(sockfd);
	fromaddrlen = *addrlen;
    if ((nread = recvfrom(sysfd, buffer, nbytes, (int)flags, fromaddr, (int *)&fromaddrlen)) < 0)
    {
        *dss_errno = dss_get_dss_errno(errno, DS_SOCK_OP_READ);
        nread = DSS_ERROR;
        goto error;
    } else if (nread == 0) {
        ds_log("recvfrom returned 0!\n");
    }

    /* Clear pending DS_READ_EVENT */
    ds_sockinfo_clr_pend_ev(sockfd, DS_READ_EVENT);
	*addrlen = (uint16)fromaddrlen;

error:
    ds_sock_unlock();
    ds_unblock_sigio();

    return (sint15)nread;
}

/*===========================================================================
  FUNCTION  dss_close
===========================================================================*/
/*!
@brief
  Non-blocking close of a socket.  Performs all necessary clean-up of data
  structures and frees the socket for re-use.  For TCP initiates the active
  close for connection termination.  Once TCP has closed, the DS_CLOSE_EVENT
  will become TRUE, and the application can call dss_close() again to free
  the socket for re-use.  UDP sockets also need to call this to
  clean-up the socket and free it for re-use.

@return
  sint15 - DSS_SUCCESS on success. Otherwise DSS_ERROR is returned and error 
           condition value is set in dss_errno.

  dss_errno Values
  ----------------
  DS_EWOULDBLOCK      operation would block - TCP close in progress
  DS_EBADF            invalid socket descriptor is specfied

@note

  - Dependencies
    - None

  - Side Effects
    - Initiates active close for TCP connections.
*/
/*=========================================================================*/
sint15 
dss_close
(
  sint15 sockfd,                                      /* socket descriptor */
  sint15 *dss_errno                               /* error condition value */
)
{
    int sysfd;
    int nethdl;
    sint15 retval = DSS_ERROR;

    ds_net_lock();

    ds_block_sigio();
    ds_sock_lock();

    if (ds_sockinfo_verify_dsfd(sockfd) < 0) {
        *dss_errno = DS_EBADF;
        goto error;
    }
    sysfd = ds_sockinfo_get_sysfd(sockfd);
    if (close(sysfd) < 0) {
        *dss_errno = dss_get_dss_errno(errno, DS_SOCK_OP_LISTEN);
        goto error;
    }

    nethdl = ds_sockinfo_get_nethdl(sockfd);
    ds_nethdl_del_sock_entry(nethdl, sockfd);

	ds_fdset_del(sysfd);
    ds_free_sockfd(sockfd);

    retval = DSS_SUCCESS;

error:
    ds_sock_unlock();
    ds_unblock_sigio();

    ds_net_unlock();
    return retval;
}

/*===========================================================================
  FUNCTION  dss_shutdown
===========================================================================*/
/*!
@brief
  Shuts down the connection of the specified socket depending on the
  'how' parameter as follows:

  DSS_SHUT_RD:   Disallow subsequent calls to recv function
  DSS_SHUT_WR:   Disallow subsequent calls to send function
  DSS_SHUT_RDWR: Disallow subseuqnet calls to both recv and send functions

@return
  sint15 - DSS_SUCCESS on success. Otherwise DSS_ERROR is returned and error 
           condition value is set in dss_errno.

  Errno Values
  ------------
  DS_EBADF                invalid socket descriptor is specfied
  DS_ENOTCONN             the socket is not connected
  DS_EINVAL               invalid operation (e.g., how parameter is invalid)
  DS_ENOMEM               insufficient memory available to complete the
                          operation

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
sint15 
dss_shutdown
(
  sint15           sockfd,                  /* socket descriptor           */
  uint16           how,                     /* what action to perform      */
  sint15*          dss_errno                /* error number                */
)
{
    int sysmode;
    int sysfd;
    sint15 rval = DSS_ERROR;

    ds_block_sigio();
    ds_sock_lock();

    if (ds_sockinfo_verify_dsfd(sockfd) < 0) {
        *dss_errno = DS_EBADF;
        goto error;
    }

    sysfd = ds_sockinfo_get_sysfd(sockfd);
    sysmode = ds_get_sys_shut_mode(how);

    if (shutdown(sysfd, sysmode) < 0) {
        *dss_errno = dss_get_dss_errno(errno, DS_SOCK_OP_SHUTDOWN);
        goto error;
    }
    rval = DSS_SUCCESS;

error:
    ds_sock_unlock();
    ds_unblock_sigio();

    return rval;
}

/*===========================================================================
  FUNCTION  dss_setsockopt
===========================================================================*/
/*!
@brief
  Sets the options associated with a socket.

@return
  sint15 - DSS_SUCCESS on success. Otherwise DSS_ERROR is returned and error 
           condition value is set in dss_errno.

  Errno Values
  ------------
  DS_EBADF                invalid socket descriptor is specfied
  DS_ENOPROTOOPT          the option is unknown at the level indicated
  DS_EINVAL               invalid option name or invalid option value
  DS_EFAULT               Invalid buffer or argument

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
sint15 
dss_setsockopt
(
  int sockfd,                            /* socket descriptor              */
  int level,                             /* socket option level            */
  int optname,                           /* option name                    */
  void *optval,                          /* value of the option            */
  uint32 *optlen,                        /* size of the option value       */
  sint15 *dss_errno                      /* error condition value          */
)
{
    int sysfd;
    int sysoptname;
	int syssocklevel;
    sint15 rval = DSS_ERROR;

    ds_block_sigio();
    ds_sock_lock();

    if (ds_sockinfo_verify_dsfd(sockfd) < 0) {
        *dss_errno = DS_EBADF;
        goto error;
    }

    sysfd = ds_sockinfo_get_sysfd(sockfd);
    sysoptname = ds_get_sys_optname(optname, optval);
    syssocklevel = ds_get_sys_socklevel(level);

    if ((sysoptname < 0) || (syssocklevel < 0)) {
        *dss_errno = DS_ENOPROTOOPT;
        goto error;
    }

    if (setsockopt(sysfd, syssocklevel, sysoptname, optval, *optlen) < 0) {
        *dss_errno = dss_get_dss_errno(errno, DS_SOCK_OP_UNSPEC);
        goto error;
    }

    rval = DSS_SUCCESS;

error:
    ds_sock_unlock();
    ds_unblock_sigio();

    return rval;
}

/*===========================================================================
  FUNCTION  dss_async_select
===========================================================================*/
/*!
@brief
  Enables the events to be notified about through the asynchronous
  notification mechanism.  Application specifies a bitmask of events that it
  is interested in, for which it will receive asynchronous notification via
  its application callback function.  This function also performs a real-time
  check to determine if any of the events have already occurred, and if so
  invokes the application callback.

@return
  sint15 - DSS_SUCCESS on success. Otherwise DSS_ERROR is returned and error 
           condition value is set in dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied

@note

  - Dependencies
    - None

  - Side Effects
    - Sets the relevant event mask in the socket control block. Will also
      notify the application via the callback function.
*/
/*=========================================================================*/
sint31 
dss_async_select
(
  sint15 sockfd,                                      /* socket descriptor */
  sint31 interest_mask,                        /* bitmask of events to set */
  sint15 *dss_errno                               /* error condition value */
)
{
    int nethdl;
    unsigned int pend_ev;
    sint31 rval = DSS_ERROR;

    ds_net_lock();

    ds_block_sigio();
    ds_sock_lock();

    if (ds_sockinfo_verify_dsfd(sockfd) < 0) {
        *dss_errno = DS_EBADF;
        goto error;
    }
    ds_sockinfo_add_wait_ev(sockfd, (unsigned int)interest_mask);
    pend_ev = ds_sockinfo_get_pend_ev(sockfd);

    if ((unsigned int)interest_mask & pend_ev) {
        nethdl = ds_sockinfo_get_nethdl(sockfd);
        ds_nethdl_call_sockcb(nethdl, sockfd, pend_ev);
    }
    rval = DSS_SUCCESS;

error:
    ds_sock_unlock();
    ds_unblock_sigio();

    ds_net_unlock();
    return rval;
}

/*===========================================================================
  FUNCTION  dss_async_deselect
===========================================================================*/
/*!
@brief
  Clears events of interest in the socket control block interest mask.  The
  application specifies a bitmask of events that it wishes to clear; events
  for which it will no longer receive notification.

@return
  sint15 - DSS_SUCCESS on success. Otherwise DSS_ERROR is returned and error 
           condition value is set in dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied

@note

  - Dependencies
    - None

  - Side Effects
    - Clears specified events from the relevant event mask.
*/
/*=========================================================================*/
sint15 
dss_async_deselect
(
  sint15 sockfd,                                      /* socket descriptor */
  sint31 clr_interest_mask,                  /* bitmask of events to clear */
  sint15 *dss_errno                               /* error condition value */
)
{
    sint15 rval = DSS_ERROR;

    ds_block_sigio();
    ds_sock_lock();

    if (ds_sockinfo_verify_dsfd(sockfd) < 0) {
        *dss_errno = DS_EBADF;
        goto error;
    }

    ds_sockinfo_clr_wait_ev(sockfd, (unsigned int)clr_interest_mask);
    rval = DSS_SUCCESS;

error:
    ds_sock_unlock();
    ds_unblock_sigio();

    return rval;
}

/*===========================================================================
  FUNCTION  dss_getnextevent
===========================================================================*/
/*!
@brief
  This function performs a real-time check to determine if any of the events
  of interest specified in the socket control block's event mask have
  occurred.  It also clears any bits in the event mask that have occurred.
  The application must re-enable these events through a subsequent call to
  dss_async_select().  The application may pass in a pointer to a single
  socket descriptor to determine if any events have occurred for that socket.

  Alternatively, the application may set this pointer's value to NULL (0)
  (note, not to be confused with a NULL pointer, but rather a pointer whose
  value is 0) in which case the function will return values for the next
  available socket.  The next available socket's descriptor will be placed
  in the socket descriptor pointer, and the function will return.  If no
  sockets are available (no events have occurred across all sockets for
  that application) the pointer value will remain NULL (originally value
  passed in), and the function will return 0, indicating that no events
  have occurred.

@return
  sint15 - Event if one is pending. A value of 0 indicates no pending events. 
           On error DSS_ERROR is returned and error condition value is set 
           in dss_errno.

  dss_errno Values
  ----------------
  DS_EBADAPP           invalid app descriptor is specfied
  DS_EBADF             invalid socket descriptor is specfied

@note

  - Dependencies
    - None

  - Side Effects
    - Clears the bits in the socket control block event mask, corresponding 
      to the events that have occurred.
*/
/*=========================================================================*/
sint31 
dss_getnextevent
(
  sint15 dss_nethandle,                                  /* application ID */
  sint15 *sockfd_ptr,                                 /* socket descriptor */
  sint15 *dss_errno                               /* error condition value */
)
{
    unsigned int next_ev = 0;
	int sockfd;
    ds_dll_el_t * node;
    int dsfd;

    ds_net_lock();

    ds_block_sigio();
    ds_sock_lock();

    if (ds_nethdl_verify(dss_nethandle) < 0) {
        *dss_errno = DS_EBADAPP;
        next_ev = (unsigned int)DSS_ERROR;
        goto error;
    }
    ds_assert(sockfd_ptr);

    sockfd = *sockfd_ptr;
    if (sockfd == 0) {
        node = ds_nethdl_get_sk_lst_hd(dss_nethandle);
        while ((node = ds_dll_next(node, (const void **)&dsfd))) {
            next_ev = dss_getnextevent_for_sock(dsfd);
            if (next_ev != 0) {
                *sockfd_ptr = (sint15)dsfd;
                break;
            }
        }
        if (!node) {
            next_ev = 0;
        }
    } else {
        next_ev = dss_getnextevent_for_sock(sockfd);
    }

error:
    ds_sock_unlock();
    ds_unblock_sigio();

    ds_net_unlock();
    return (sint31)next_ev;
}
