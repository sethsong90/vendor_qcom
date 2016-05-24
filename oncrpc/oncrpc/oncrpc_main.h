#ifndef ONCRPC_MAIN_H
#define ONCRPC_MAIN_H

/*===========================================================================

                    ONCRPC MAIN OS Header File

Defines the APIs that must be provided 


 Copyright (c) 2007-2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
===========================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

$Header: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_main.h#4 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
05/12/08   ih      Add oncrpc_deinit and oncrpc_main_os_deinit
05/08/08   ih      Add rpc_handle_rpc_msg
04/21/08    sd     Added oncrpc_get_last_sent_msg_info() and 
                   oncrpc_set_last_sent_msg_info()
01/17/07   rr      Add rpc_fake_reply
08/22/07   ptm     Unified access to thread local storage.
07/10/07   ptm     Remove "dword" and unneeded APIs.
04/12/07   RJS     Initial version

===========================================================================*/
#include "oncrpc_task.h"

/*===========================================================================
                      TYPE DECLARATIONS
===========================================================================*/

// Message type classification as received by oncrpc task
typedef enum {
  ONCRPC_MSG_CMD,
  ONCRPC_MSG_REPLY
  } oncrpc_msg_type;


/*===========================================================================
                      PUBLIC FUNCTION DECLARATIONS
===========================================================================*/
void           oncrpc_signal_rpc_thread(uint32 events);
boolean        oncrpc_is_rpc_thread(void);
void           oncrpc_main(void);
void           oncrpc_remote_apis_initialised(void);
void           oncrpc_init(void);
void           oncrpc_deinit(void);
void           oncrpc_reply_init(void);
void           oncrpc_init_mem_crit_sect(oncrpc_crit_sect_ptr* pcs);
void           oncrpc_os_init(void);
void           oncrpc_main_os_init(void);
void           oncrpc_main_os_deinit(void);
void           oncrpc_clean_reply_queue_by_client(  oncrpc_addr_type client_addr );
void           rpc_fake_reply(xdr_s_type *xdr);
void           rpc_handle_rpc_msg(xdr_s_type *xdr);
boolean        oncrpc_get_last_sent_msg_info(uint32 *prog, uint32 *vers,
                                             uint32 *proc);
void           oncrpc_set_last_sent_msg_info(uint32 prog, uint32 vers, 
                                             uint32 proc);
/*===========================================================================
 * End of module
 *=========================================================================*/
#endif /* ONCRPC_MAIN_H */

