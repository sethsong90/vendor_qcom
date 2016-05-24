#ifndef RPC_ROUTER_OS_H
#define RPC_ROUTER_OS_H

/*===========================================================================

                       R P C   R O U T E R  O S

                        H E A D E R    F I L E

DESCRIPTION
  This file contains types and declarations for the RPC ROUTER OS abstraction.
  
-----------------------------------------------------------------------------
  Copyright (c) 2007-2008, Qualcomm Technologies, Inc.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
      * Neither the name of the Qualcomm Technologies, Inc. nor the names of its
      contributors may be used to endorse or promote products derived from this
      software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------

===========================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

$Header$ $DateTime$ $Author$

when       who     what, where, why
--------   ---     ----------------------------------------------------------
12/12/07   al      Added rpc_router_os_sem_lock_uninterruptible
12/11/07   al      Added RPC_ROUTER_AUTO_ADDRESS feature to generate new
                   client ids within router
10/22/07   sh      added uninterruptible signal wait
05/02/07   sh      added threads
04/30/07   sh      added signals, updated sem prototypes
04/12/07   rr      Initial Draft
===========================================================================*/

/*===========================================================================
                          INCLUDE FILES
===========================================================================*/
#if defined(__linux__)
#include "rpc_router_os_lnx.h"
#else
#include "customer.h"
#ifdef FEATURE_WINCE
#include "rpc_router_os_wm.h"
#else /* AMSS */
#include "rpc_router_os_rex.h"
#endif
#endif

#if !defined(__linux__) || defined(__KERNEL__)
/*===========================================================================
                        TYPE DECLARATIONS
===========================================================================*/
typedef void (*rpc_router_thread_fn_type)(void *);

/*===========================================================================
FUNCTION      rpc_router_os_sem_unlock

DESCRIPTION   Unlock (take) semaphore
              
ARGUMENTS     rpc_router_os_sem *, os-specific type must be defined in the
              os-specific header file.

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
void rpc_router_os_sem_unlock(rpc_router_os_sem *sem);

/*===========================================================================
FUNCTION      rpc_router_os_sem_lock

DESCRIPTION   Lock (take) semaphore
              
ARGUMENTS     rpc_router_os_sem *, os-specific type must be defined in the
              os-specific header file.

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
int rpc_router_os_sem_lock(rpc_router_os_sem *sem);

/*===========================================================================
FUNCTION rpc_router_os_sem_lock_uninterruptible

DESCRIPTION
   Acquires the semaphore.

DEPENDENCIES

RETURN VALUE
   RPC_ROUTER_STATUS_SUCCESS

SIDE EFFECTS
===========================================================================*/
int rpc_router_os_sem_lock_uninterruptible(rpc_router_os_sem *sem);

/*===========================================================================
FUNCTION      rpc_router_os_sem_try_lock

DESCRIPTION   Try to lock (take) semaphore, if already locked return immediately
              
ARGUMENTS     rpc_router_os_sem *, os-specific type must be defined in the
              os-specific header file.

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
int rpc_router_os_sem_try_lock(rpc_router_os_sem *sem);

/*===========================================================================
FUNCTION      rpc_router_os_sem_init

DESCRIPTION   Initialize semaphore
              
ARGUMENTS     rpc_router_os_sem *, os-specific type must be defined in the
              os-specific header file.

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
void rpc_router_os_sem_init(rpc_router_os_sem *sem, void *param);

/*===========================================================================
FUNCTION      rpc_router_os_sem_deinit

DESCRIPTION   Deinitialize semaphore
              
ARGUMENTS     rpc_router_os_sem *, os-specific type must be defined in the
              os-specific header file.

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
void rpc_router_os_sem_deinit(rpc_router_os_sem *sem);

/*===========================================================================
FUNCTION      rpc_router_os_signal_init

DESCRIPTION   initialize a manual-reset signal /event
              
ARGUMENTS     rpc_router_os_signal *, os-specific type must be defined in the
              os-specific header file.

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
void rpc_router_os_signal_init(rpc_router_os_signal *sig, void *param);

/*===========================================================================
FUNCTION      rpc_router_os_signal_wait

DESCRIPTION   waits on a signal/event
              
ARGUMENTS     rpc_router_os_signal *, os-specific type must be defined in the
              os-specific header file.

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
int rpc_router_os_signal_wait(rpc_router_os_signal *sig, rpc_router_ms_type timeout);

/*===========================================================================
FUNCTION      rpc_router_os_signal_wait_uninterruptible

DESCRIPTION   waits on a signal/event disregarding any external signals
              
ARGUMENTS     rpc_router_os_signal *, os-specific type must be defined in the
              os-specific header file.

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
int rpc_router_os_signal_wait_uninterruptible(rpc_router_os_signal *sig, rpc_router_ms_type timeout);

/*===========================================================================
FUNCTION      rpc_router_os_signal_set

DESCRIPTION   sets a signal/event
              
ARGUMENTS     rpc_router_os_signal *, os-specific type must be defined in the
              os-specific header file.

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
void rpc_router_os_signal_set(rpc_router_os_signal *sig);

/*===========================================================================
FUNCTION      rpc_router_os_signal_clear

DESCRIPTION   clears a signal/event
              
ARGUMENTS     rpc_router_os_signal *, os-specific type must be defined in the
              os-specific header file.

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
void rpc_router_os_signal_clear(rpc_router_os_signal *sig);

/*===========================================================================
FUNCTION      rpc_router_os_signal_deinit

DESCRIPTION   Deinitialize signal/event
              
ARGUMENTS     rpc_router_os_signal *, os-specific type must be defined in the
              os-specific header file.

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
void rpc_router_os_signal_deinit(rpc_router_os_signal *sig);

/*===========================================================================
FUNCTION      rpc_router_os_thread_create

DESCRIPTION   creates and starts a thread
              
ARGUMENTS     rpc_router_os_thread *, os-specific type must be defined in the
              os-specific header file.

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
int rpc_router_os_thread_create(rpc_router_os_thread *thd, rpc_router_thread_fn_type function, void *param);

/*===========================================================================
FUNCTION      rpc_router_os_thread_close

DESCRIPTION   cleans up the os thread type including it's handle
              
ARGUMENTS     rpc_router_os_thread *, os-specific type must be defined in the
              os-specific header file.

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
int rpc_router_os_thread_close(rpc_router_os_thread *thd);

/*===========================================================================
FUNCTION      rpc_router_os_next_client_id_candidate

DESCRIPTION   returns the next candidate for a client id.
              
ARGUMENTS     pointer to a 32-bit unsigned integer that will hold the client
              id candidate.

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
void rpc_router_os_next_client_id_candidate(uint32 *client_id);

#endif /* !defined(__linux__) || defined(__KERNEL__) */

#endif  /* RPC_ROUTER_OS_H */
