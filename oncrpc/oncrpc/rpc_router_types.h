#ifndef RPC_ROUTER_TYPES_H
#define RPC_ROUTER_TYPES_H
/*===========================================================================

                       R P C   R O U T E R  T Y P E S

                           H E A D E R    F I L E

DESCRIPTION
  This file contains general data types and definitions for RPC Router
  
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
01/29/08    sh     Support for rpc daemon
10/11/07    sh     Switched back to using uint32 for router address fields
04/30/07    sh     First revision   
===========================================================================*/


/*===========================================================================
                          INCLUDE FILES
===========================================================================*/
#include "rpc_router_os.h"


/*===========================================================================
                CONSTANT / MACRO DACLARATIONS
===========================================================================*/
/*---------------------------------------------------------------------------
  Error Codes
---------------------------------------------------------------------------*/
#define RPC_ROUTER_STATUS_UNKNOWN        (0)
#define RPC_ROUTER_STATUS_SUCCESS        (RPC_ROUTER_STATUS_UNKNOWN + 1)
#define RPC_ROUTER_STATUS_FAILURE        (RPC_ROUTER_STATUS_UNKNOWN + 2)
#define RPC_ROUTER_STATUS_INVALID_PARAM  (RPC_ROUTER_STATUS_UNKNOWN + 3)
#define RPC_ROUTER_STATUS_TIMEOUT        (RPC_ROUTER_STATUS_UNKNOWN + 4)
#define RPC_ROUTER_STATUS_INVALID_MEM    (RPC_ROUTER_STATUS_UNKNOWN + 5)
#define RPC_ROUTER_STATUS_NO_MEM         (RPC_ROUTER_STATUS_UNKNOWN + 6)
#define RPC_ROUTER_STATUS_IO_ERROR       (RPC_ROUTER_STATUS_UNKNOWN + 7)
#define RPC_ROUTER_STATUS_INTERRUPTED    (RPC_ROUTER_STATUS_UNKNOWN + 8)
#define RPC_ROUTER_STATUS_BUSY           (RPC_ROUTER_STATUS_UNKNOWN + 9)
#define RPC_ROTUER_STATUS_LAST           (RPC_ROUTER_STATUS_UNKNOWN + 10)

#define RPC_ROUTER_MAX_MSG_SIZE (512)


/*===========================================================================
                        TYPE DECLARATIONS
===========================================================================*/
/* Transport types */
typedef enum
{
  RPC_ROUTER_XPORT_NONE=0,
  RPC_ROUTER_XPORT_LOCAL,
  RPC_ROUTER_XPORT_SMD,
  RPC_ROUTER_XPORT_SMMS, 
  RPC_ROUTER_XPORT_SOCKETS,
  RPC_ROUTER_XPORT_ALL
}rpc_router_channel_type;

typedef struct
{
    uint32  version;            /* The version of the router */
    uint32  processor_id;       /* The processor id where the router resides */
    uint32  client_id;          /* The client id of the router */
    uint32  clients_cnt;        /* The number of clients registered with the router */
    uint32  max_pkt_cnt;        /* The maximum number of packets in the queue of the router's reader thread */
    uint32  g_max_pkt_cnt;      /* The global maximum number of packets in queues of all clients */
}rpc_router_stats_type;

typedef struct
{
    uint32  client_id;          /* The client id of the calling process */
    uint32  server_cnt;         /* The number of servers registered by the calling process */
    uint32  droppped_pkt_cnt;   /* The number of packets dropped for this client */
    uint32  queue_size;         /* The size of the queue for the calling process */
    uint32  max_pkt_cnt;        /* The maximum number of packets in the queue of the calling process */
}rpc_router_client_stats_type; 

typedef struct 
{
   uint32                 processor_id;
   uint32                 client_id;
}rpc_router_address_type;

typedef struct
{
   uint32                   xp;
   uint32                   port;
}rpc_router_xport_address_type;

typedef struct
{
   uint32                 prog;
   uint32                 ver;
}rpc_router_program_version_type;

typedef struct
{
   rpc_router_address_type          addr;
   rpc_router_program_version_type  prog_ver;
}rpc_router_routing_record_type;

typedef struct
{
    rpc_router_program_version_type pv;
    uint32                          timeout;
}rpc_router_dest_lookup_type;

typedef union
{
   rpc_router_program_version_type  input;
}rpc_router_ioctl_server_type;

typedef union
{
   rpc_router_xport_address_type    input;
}rpc_router_ioctl_xport_type;

typedef union
{
   rpc_router_dest_lookup_type  input;
   rpc_router_address_type      output;
}rpc_router_ioctl_get_dest_type;

typedef struct
{
   /* in : number of records allocated in "records"                        */
   /* out: number of records actually filled in "records"                  */
   uint32                          size;
   /* out: total number of records in the routing table                    */
   uint32                          total;
   /* out: routing table records                                           */
   /* NOTE: the true size of the records array is determined at allocation */
   /*       time.                                                          */
   /* Example: to allocate this struct for holding 10 records:             */
   /*                                                                      */
   /* malloc(sizeof(rpc_router_ioctl_get_routing_table_type) +             */
   /*        9 * sizeof(rpc_router_routing_record_type))                   */
   rpc_router_routing_record_type  records[1];
}rpc_router_ioctl_get_routing_table_type;

typedef enum
{
  PROCESSOR_RESTART,
  PROCESS_TERMINATION
}rpc_router_dead_client_msg_type;

typedef struct
{
  rpc_router_address_type    client_addr;
  uint32                     has_servers;    
}rpc_router_dead_client_record_type;

#endif
