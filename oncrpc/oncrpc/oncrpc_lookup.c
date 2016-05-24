/*===========================================================================

                   O N C R P C   L O O K U P

  This file is used to implement database to store the following data:
    - Router destination addresses indexed by program and version number.
    - Server callback exit functions indexed by client address.

  The databases are implemented using hashes and linked-list
  model.  The hash table being 2^(x) long where x is a tunable parameter
  set by ONCRPC_LOOKUP_<database>_MASK_SIZE_BITS.   The linked list is defined
  internally (as opposed to using qlist) for efficiency.

  The hash table is adjustable in size as a power of 2, and
  may be changed for performance tuning (speed vs memory usage)
  See the ONCRPC_LOOKUP_<database>_MASK_SIZE_BITS

 Copyright (c) 2007-2009, 2011 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.

===========================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

$Header: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_lookup.c#7 $ $DateTime: 2009/02/24 15:29:36 $ $Author: rruigrok $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
06/08/11   eh      Added server-up for restart notification
02/23/09   rr      Add oncrpc_lookup_get to support deinit
11/11/08    rr     Add oncrpc_lookup_get_prog_vers for restart support
03/19/08   ih      Fix bug in unregister_sever_exit_notification
03/05/08   ih      Added restart daemon support
02/06/08   rr      Remove memset, not needed
10/18/07   ptm     Add oncrpc-lookup-get-status.
10/08/07   rr      Add lookup cb database
04/30/07   rr      Initial lookup database to support rpc_router
===========================================================================*/

/*===========================================================================

                        INCLUDE FILES

===========================================================================*/
#include "comdef.h"
#include "queue.h"
#include "oncrpc.h"
#include "oncrpci.h"
#include "oncrpc_lookup.h"
#include "oncrpc_os.h"
#include "memory.h"


/*===========================================================================
                        LOCAL DEFINITIONS
===========================================================================*/
#define ONCRPC_LOOKUP_PROGRAM_MASK_SIZE_BITS  (6)
#define ONCRPC_LOOKUP_PROGRAM_MASK_SIZE (0x01) << (ONCRPC_LOOKUP_PROGRAM_MASK_SIZE_BITS - 1)
#define ONCRPC_LOOKUP_PROGRAM_MASK ( ( (0x01) << (ONCRPC_LOOKUP_PROGRAM_MASK_SIZE_BITS - 1) ) - 1)
#define ONCRPC_LOOKUP_PROGRAM_TABLE_SIZE (0x01) << (ONCRPC_LOOKUP_PROGRAM_MASK_SIZE_BITS - 1)

#define ONCRPC_LOOKUP_EXITCB_MASK_SIZE_BITS  (6)
#define ONCRPC_LOOKUP_EXITCB_MASK_SIZE (0x01) << (ONCRPC_LOOKUP_EXITCB_MASK_SIZE_BITS - 1)
#define ONCRPC_LOOKUP_EXITCB_MASK ( ( (0x01)  << (ONCRPC_LOOKUP_EXITCB_MASK_SIZE_BITS - 1) ) - 1)
#define ONCRPC_LOOKUP_EXITCB_TABLE_SIZE (0x01) << (ONCRPC_LOOKUP_EXITCB_MASK_SIZE_BITS - 1)

#define ONCRPC_CLEANUP_BAILOUT_TIME_MS (1000)

/*===========================================================================
                        LOCAL DATA DEFINITIONS
===========================================================================*/
/* Structure for callback exit function registration */
typedef struct lookup_exitcb_struct
{
  struct lookup_exitcb_struct  *next;
  struct lookup_exitcb_struct  **prev_next;
  oncrpc_addr_type             client_addr;
  void                         *cb_data;
  oncrpc_client_exit_notification_f_type clnt_exit_cb;
  oncrpc_server_exit_notification_f_type svr_exit_cb;
}lookup_exitcb_type;

/* Structure client lookup */
typedef struct lookup_client_struct
{
  struct lookup_client_struct  *next;
  struct lookup_client_struct  **prev_next;
  uint32 prog;
  uint32 version;
  oncrpc_addr_type dest;
}lookup_client_type;


static oncrpc_crit_sect_ptr   lookup_list_critic_sect_ptr;
static lookup_client_type*    lookup_client_table[ONCRPC_LOOKUP_PROGRAM_TABLE_SIZE];

static oncrpc_crit_sect_ptr   lookup_exitcb_list_critic_sect_ptr;
static lookup_exitcb_type    *lookup_exitcb_table[ONCRPC_LOOKUP_EXITCB_TABLE_SIZE];
static lookup_exitcb_type    *lookup_restartcb_table[ONCRPC_LOOKUP_EXITCB_TABLE_SIZE];
static lookup_exitcb_type    *cleanup_list;
static boolean                timer_started;
static oncrpc_timer_ptr       timer;
static oncrpc_addr_type       bailout_server_addr;

/*===========================================================================
                          MACRO DEFINITIONS
===========================================================================*/
#define GET_CLIENT_TABLE_INDEX(prog)      \
   ((uint32)prog & ONCRPC_LOOKUP_PROGRAM_MASK)


#define GET_EXITCB_TABLE_INDEX(prog)      \
   ((uint32)prog & ONCRPC_LOOKUP_EXITCB_MASK)

void oncrpc_bailout_timer_cb(unsigned long data);

/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_INIT

DESCRIPTION   Initialize the client lookup database

ARGUMENTS     None

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
void oncrpc_lookup_init()
{
  /* Clear the lookup table */
  oncrpc_crit_sect_init(&lookup_list_critic_sect_ptr);
  oncrpc_crit_sect_init(&lookup_exitcb_list_critic_sect_ptr);
  if (!oncrpc_timer_new_cb(&timer, oncrpc_bailout_timer_cb, 0)) {
    ERR_FATAL( "Unable to create timer",0,0,0);
  }
}


/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_GET_DEST

DESCRIPTION   Get the router destination for this client

ARGUMENTS     program and version number to lookup.

RETURN VALUE  pointer to oncrpc_addr_type, address for
              prog, and version number.

SIDE EFFECTS  None
===========================================================================*/
oncrpc_addr_type *oncrpc_lookup_get_dest(uint32 prog, uint32 version)
{
  uint32 index ;
  boolean found = 0;
  lookup_client_type* client_head;
  lookup_client_type* client;

  index = GET_CLIENT_TABLE_INDEX(prog);
  client_head = lookup_client_table[index];

  oncrpc_crit_sect_enter(lookup_list_critic_sect_ptr);

  client = client_head;
  while( client != NULL )
  {
    if( (client->prog == prog) && (client->version == version) )
    {
      found = 1;
      break;
    }
    client=client->next;
  }

  oncrpc_crit_sect_leave(lookup_list_critic_sect_ptr);
  if( found )
  {
    return &client->dest;
  }
  else
  {
    return NULL;
  }
}


/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_GET

DESCRIPTION   Get a client from list

ARGUMENTS     pointer to oncrpc_addr_type, prog and vers

RETURN VALUE  TRUE if the database is not empty, FALSE otherwise

SIDE EFFECTS  None
===========================================================================*/
boolean oncrpc_lookup_get(oncrpc_addr_type *addr, uint32 *prog, uint32 *vers)
{
  uint32 index=0;
  boolean found;
  lookup_client_type* client_head=0;

  oncrpc_crit_sect_enter(lookup_list_critic_sect_ptr);
  while( (client_head == 0) && (index < ONCRPC_LOOKUP_PROGRAM_TABLE_SIZE) )
  {
     client_head = lookup_client_table[index++];
  }

  if(client_head)
  {
    *addr = client_head->dest;
    *prog = client_head->prog;
    *vers = client_head->version;
     found=TRUE;
  }
  else
  {
     found = FALSE;
  }

  oncrpc_crit_sect_leave(lookup_list_critic_sect_ptr);
  return found;
}


/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_GET_PROG_VERS

DESCRIPTION   Get the program and version from destination.
              This is a hash/link-list table. In this reverse search, we must
              go through all the link list tables for all the hash values.

ARGUMENTS     destination address

RETURN VALUE  boolean TRUE if found, FALSE if not found. Sets the prog
              and version values.

SIDE EFFECTS  None
===========================================================================*/
uint32 oncrpc_lookup_get_prog_vers(
   oncrpc_addr_type addr,
   uint32 *prog,
   uint32 *version
   )
{
  uint32 index ;
  boolean found = 0;
  lookup_client_type* client;

  oncrpc_crit_sect_enter(lookup_list_critic_sect_ptr);
  for( index=0;(index < ONCRPC_LOOKUP_PROGRAM_TABLE_SIZE) && !found ; index++ )
  {
    client = lookup_client_table[index];
    while( client != NULL )
    {
      if( (client->dest == addr ) )
      {
          *prog = client->prog;
          *version = client->version;
          oncrpc_crit_sect_leave(lookup_list_critic_sect_ptr);
          return TRUE;
      }
      client=client->next;
    }
  }
  oncrpc_crit_sect_leave(lookup_list_critic_sect_ptr);
  return FALSE;
}



/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_ADD_DEST

DESCRIPTION   Add a router destination for this client

ARGUMENTS     prog: program number
              version: program version
              dest: destination address

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
void oncrpc_lookup_add_dest(
  uint32 prog,
  uint32 version,
  oncrpc_addr_type dest
  )
{
  uint32 index ;
  lookup_client_type* new_client;

  index = GET_CLIENT_TABLE_INDEX(prog);
  oncrpc_crit_sect_enter(lookup_list_critic_sect_ptr);

  /**************************************************************
  * Client Not Found, Create a new client for this program
  * and version number.
  **************************************************************/

  new_client = oncrpc_mem_alloc(sizeof(lookup_client_type));
  if(new_client == NULL)
  {
    ERR("oncrpc_lookup_add_dest: Memory allocation failed for new_client\n", 0, 0, 0);
    oncrpc_crit_sect_leave(lookup_list_critic_sect_ptr);
    return;
  }
  new_client->prog=prog;
  new_client->version=version;
  new_client->dest = dest;

  /* Always put new clients in the table (head)        */
  /* The previous pointer to next is the head pointer! */
  new_client->prev_next = &lookup_client_table[index];
  /* The next pointer is what the head used to point to */
  /* It's null if this is the first one */
  new_client->next = lookup_client_table[index];

  if( lookup_client_table[index] )
  {
    lookup_client_table[index]->prev_next = &new_client->next;
  }
  lookup_client_table[index] = new_client;
  oncrpc_crit_sect_leave(lookup_list_critic_sect_ptr);
}

/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_REMOVE_CLIENT

DESCRIPTION   Remove an entry from the lookup database.

ARGUMENTS     prog: program number
              version: program version

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
void oncrpc_lookup_remove_client(uint32 prog, uint32 version)
{
  uint32 index = GET_CLIENT_TABLE_INDEX(prog);
  boolean found = 0;
  lookup_client_type* client_head = lookup_client_table[index];
  lookup_client_type* client;

  oncrpc_crit_sect_enter(lookup_list_critic_sect_ptr);

  client = client_head;
  while( client != NULL )
  {
    if( (client->prog == prog) && (client->version == version) )
    {
      found = 1;
      break;
    }
    client=client->next;
  }
  if( found )
  {
    *client->prev_next = client->next;
    if( client->next )
    {
      (client->next)->prev_next = client->prev_next;
    }
  }
  oncrpc_mem_free(client);
  oncrpc_crit_sect_leave(lookup_list_critic_sect_ptr);
}


/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_REMOVE_CLIENTS_AT_ADDR

DESCRIPTION   Remove all entries from the lookup database that
              are served by this address.

ARGUMENTS     client_to_remove, address of client

RETURN VALUE  number of clients removed.

SIDE EFFECTS  All programs at client address will be removed.
===========================================================================*/
uint32 oncrpc_lookup_remove_clients_at_addr(oncrpc_addr_type client_to_remove)
{
  lookup_client_type* client;
  lookup_client_type* next_client;
  uint32 index;
  uint32 num_clients_removed=0;

  oncrpc_crit_sect_enter(lookup_list_critic_sect_ptr);
  for( index = 0; index < ONCRPC_LOOKUP_PROGRAM_TABLE_SIZE; index ++ )
  {
    client = lookup_client_table[index];
    while( client != NULL )
    {
      next_client= client->next;
      if( client->dest == client_to_remove )
      {
        *client->prev_next = client->next;
        if( client->next )
        {
          (client->next)->prev_next = client->prev_next;
        }
        num_clients_removed++;
        oncrpc_mem_free(client);
      }

      client= next_client;

    }
  }
  oncrpc_crit_sect_leave(lookup_list_critic_sect_ptr);
  return num_clients_removed;
}

/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_FLUSH_DATABASE

DESCRIPTION   Remove all client entries from database

ARGUMENTS     N/A

RETURN VALUE  number of clients removed

SIDE EFFECTS  Entire database will be flushed.
===========================================================================*/
uint32 oncrpc_lookup_flush_database()
{
  lookup_client_type* client;
  lookup_client_type* next_client;
  uint32 index;
  uint32 num_clients_removed=0;

  oncrpc_crit_sect_enter(lookup_list_critic_sect_ptr);
  for( index = 0; index < ONCRPC_LOOKUP_PROGRAM_TABLE_SIZE; index ++ )
  {
    client = lookup_client_table[index];
    while( client != NULL )
    {
      next_client= client->next;
      *client->prev_next = client->next;
      if( client->next )
      {
        (client->next)->prev_next = client->prev_next;
      }
      num_clients_removed ++;
      oncrpc_mem_free(client);
      client= next_client;
    }

  }
  oncrpc_crit_sect_leave(lookup_list_critic_sect_ptr);
  return num_clients_removed;
}

/*===========================================================================
FUNCTION      ONCRPC_REGISTER_CLIENT_EXIT_NOTIFICATION_CB

DESCRIPTION   Add a callback exit funtion entry using the source addr.
              Will scan database first to determine if a current identical
			  exit function matches with the same source, function and data.
			  If the entry does not exist, it will be added.

ARGUMENTS     xdr    : Used to find message source.
              cb_func: Callback function to call on exit
              cb_data: Data pointer to callback with the exit function

RETURN VALUE  N/A

SIDE EFFECTS  Memory allocation
===========================================================================*/
void oncrpc_register_client_exit_notification_cb(
  xdr_s_type         *xdr,
  oncrpc_client_exit_notification_f_type  cb_func,
  void               *cb_data
)
{
  oncrpc_addr_type client_addr = xdr->msg_source;
  uint32 index ;
  lookup_exitcb_type* new_entry;
  lookup_exitcb_type* entry;

  index = GET_EXITCB_TABLE_INDEX(client_addr);
  oncrpc_crit_sect_enter(lookup_exitcb_list_critic_sect_ptr);
  entry = lookup_exitcb_table[index];
  while( entry != NULL )
  {
    if(  (entry->client_addr == client_addr) && (entry->clnt_exit_cb == cb_func) && (entry->cb_data == cb_data ) )
    {
      /* The entry already exists, no need to add a 2nd entry */
      oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
      return;
    }
    entry=entry->next;
  }


  /**************************************************************
  * Client Not Found, Create a new client for this program
  * and version number.
  **************************************************************/

  new_entry = oncrpc_mem_alloc(sizeof(lookup_exitcb_type));
  if(new_entry == NULL)
  {
    ERR("oncrpc_register_client_exit_notification_cb: Memory allocation failed for new_entry\n", 0, 0, 0);
    oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
    return;
  }
  new_entry->client_addr = client_addr;
  new_entry->clnt_exit_cb = cb_func;
  new_entry->svr_exit_cb = NULL;
  new_entry->cb_data = cb_data;
  /* Always put new clients in the table (head)        */
  /* The previous pointer to next is the head pointer! */
  new_entry->prev_next = &lookup_exitcb_table[index];
  /* The next pointer is what the head used to point to */
  /* It's null if this is the first one */
  new_entry->next = lookup_exitcb_table[index];

  if( lookup_exitcb_table[index] )
  {
    lookup_exitcb_table[index]->prev_next = &new_entry->next;
  }
  lookup_exitcb_table[index] = new_entry;
  oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
}

/*===========================================================================
FUNCTION      ONCRPC_UNREGISTER_CLIENT_EXIT_NOTIFICATION_CB

DESCRIPTION   Remove a callback exit funtion entry
              Will remove all callback exit functions that match the
              message source address and the callback function pointer.

ARGUMENTS     xdr    : Used to find message source.
              cb_func: Callback function to call on exit

RETURN VALUE  void * pointer to cb data

SIDE EFFECTS  Memory free for entries removed.
===========================================================================*/
void *oncrpc_unregister_client_exit_notification_cb(
  xdr_s_type       *xdr,
  oncrpc_client_exit_notification_f_type cb_func,
  void                  *cb_data,
  oncrpc_compare_f_type  compare
  )
{
  oncrpc_addr_type client_addr = xdr->msg_source;
  uint32              index = GET_EXITCB_TABLE_INDEX(client_addr);
  lookup_exitcb_type* entry;
  lookup_exitcb_type* entry_head = lookup_exitcb_table[index];
  void               *rval = NULL;

  oncrpc_crit_sect_enter(lookup_exitcb_list_critic_sect_ptr);
  entry = entry_head;

  while( entry != NULL )
  {
    if( (entry->client_addr == client_addr) &&
        (entry->clnt_exit_cb == cb_func) &&
        (compare(entry->cb_data, cb_data)) )
    {
      *entry->prev_next = entry->next;
      if( entry->next )
      {
        (entry->next)->prev_next = entry->prev_next;
      }
      rval = entry->cb_data;
      oncrpc_mem_free(entry);
      break;
    }
    entry=entry->next;
  }
  oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);

  return rval;
}

/*===========================================================================
FUNCTION      ONCRPC_REGISTER_SERVER_EXIT_NOTIFICATION_CB

DESCRIPTION   Add a callback cleanup funtion entry using the dest addr.
              Will scan database first to determine if a current identical
			  exit function matches with the same source, function and data.
			  If the entry does not exist, it will be added.

ARGUMENTS     xdr    : Used to find message source.
              cb_func: Callback function to call on exit
              cb_data: Data pointer to callback with the exit function

RETURN VALUE  N/A

SIDE EFFECTS  Memory allocation
===========================================================================*/
void oncrpc_register_server_exit_notification_cb(
  uint32 prog,
  uint32 ver,
  oncrpc_server_exit_notification_f_type cb_func,
  void   *cb_data
  )
{
  xdr_s_type *xdr;
  oncrpc_addr_type *dest_ptr, client_addr;
  uint32 index ;
  lookup_exitcb_type* new_entry;
  lookup_exitcb_type* entry;

  xdr = rpc_clnt_lookup2(prog, ver, RPC_CLNT_LOOKUP_TIMEOUT);
  dest_ptr = oncrpc_lookup_get_dest(prog, ver);

  if(dest_ptr == NULL) {
    ERR_FATAL("oncrpc_lookup_dest() returned NULL!", 0,0,0);
  }

  client_addr = *dest_ptr;
  index = GET_EXITCB_TABLE_INDEX(client_addr);
  oncrpc_crit_sect_enter(lookup_exitcb_list_critic_sect_ptr);
  entry = lookup_exitcb_table[index];

  while( entry != NULL )
  {
    if(  (entry->client_addr == client_addr) && (entry->svr_exit_cb == cb_func) && (entry->cb_data == cb_data ) )
    {
      /* The entry already exists, no need to add a 2nd entry */
      oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
      return;
    }
    entry=entry->next;
  }

  /**************************************************************
  * Client Not Found, Create a new client for this program
  * and version number.
  **************************************************************/

  new_entry = oncrpc_mem_alloc(sizeof(lookup_exitcb_type));
  if(new_entry == NULL)
  {
    ERR("oncrpc_register_server_exit_notification_cb: Memory allocation failed for new_entry\n", 0, 0, 0);
    oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
    return;
  }
  new_entry->client_addr = client_addr;
  new_entry->svr_exit_cb = cb_func;
  new_entry->clnt_exit_cb = NULL;
  new_entry->cb_data = cb_data;
  /* Always put new clients in the table (head)        */
  /* The previous pointer to next is the head pointer! */
  new_entry->prev_next = &lookup_exitcb_table[index];
  /* The next pointer is what the head used to point to */
  /* It's null if this is the first one */
  new_entry->next = lookup_exitcb_table[index];

  if( lookup_exitcb_table[index] )
  {
    lookup_exitcb_table[index]->prev_next = &new_entry->next;
  }
  lookup_exitcb_table[index] = new_entry;
  oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
}

/*===========================================================================
FUNCTION      ONCRPC_UNREGISTER_SERVER_EXIT_NOTIFICATION_CB

DESCRIPTION   Remove a callback exit funtion entry
              Will remove all callback exit functions that match the
              message source address and the callback function pointer.

ARGUMENTS     xdr    : Used to find message source.
              cb_func: Callback function to call on exit

RETURN VALUE  void * pointer to cb data

SIDE EFFECTS  Memory free for entries removed.
===========================================================================*/
void *oncrpc_unregister_server_exit_notification_cb(
  uint32 prog,
  uint32 ver,
  oncrpc_server_exit_notification_f_type cb_func,
  void                  *cb_data,
  oncrpc_compare_f_type  compare
  )
{
  xdr_s_type *xdr;
  oncrpc_addr_type client_addr, *dest_ptr;
  uint32              index;
  lookup_exitcb_type* entry;
  lookup_exitcb_type* entry_head;
  void               *rval = NULL;

  xdr = rpc_clnt_lookup2(prog, ver, RPC_CLNT_LOOKUP_TIMEOUT);
  dest_ptr = oncrpc_lookup_get_dest(prog, ver);

  if(dest_ptr == NULL) {
    ERR_FATAL("oncrpc_lookup_dest() returned NULL!", 0,0,0);
  }

  client_addr = *dest_ptr;

  index = GET_EXITCB_TABLE_INDEX(client_addr);
  entry_head = lookup_exitcb_table[index];

  oncrpc_crit_sect_enter(lookup_exitcb_list_critic_sect_ptr);
  entry = entry_head;

  while( entry != NULL )
  {
    if( (entry->client_addr == client_addr) &&
        (entry->svr_exit_cb == cb_func) &&
        (compare(entry->cb_data, cb_data)) )
    {
      *entry->prev_next = entry->next;
      if( entry->next )
      {
        (entry->next)->prev_next = entry->prev_next;
      }
      rval = entry->cb_data;
      oncrpc_mem_free(entry);
      break;
    }
    entry=entry->next;
  }
  oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);

  return rval;
}

/*===========================================================================
FUNCTION      ONCRPC_REGISTER_SERVER_RESTART_NOTIFICATION_CB

DESCRIPTION   Add a restart-notification callback entry using the dest addr.
              Will scan database first to determine if a current identical
              callback matches with the same source, function and data.
              If the entry does not exist, it will be added.

ARGUMENTS     xdr    : Used to find message source.
              cb_func: Callback function to call for restart server-up
              cb_data: Opaque data pointer passed into callback

RETURN VALUE  N/A

SIDE EFFECTS  Memory allocation
===========================================================================*/
void oncrpc_register_server_restart_notification_cb(
  uint32 prog,
  uint32 ver,
  oncrpc_server_exit_notification_f_type cb_func,
  void   *cb_data
  )
{
  xdr_s_type *xdr;
  oncrpc_addr_type *dest_ptr, client_addr;
  uint32 index ;
  lookup_exitcb_type* new_entry;
  lookup_exitcb_type* entry;

  xdr = rpc_clnt_lookup2(prog, ver, RPC_CLNT_LOOKUP_TIMEOUT);
  dest_ptr = oncrpc_lookup_get_dest(prog, ver);

  if(dest_ptr == NULL) {
    ERR_FATAL("oncrpc_lookup_dest() returned NULL!", 0,0,0);
  }

  client_addr = *dest_ptr;
  index = GET_EXITCB_TABLE_INDEX(client_addr);
  oncrpc_crit_sect_enter(lookup_exitcb_list_critic_sect_ptr);
  entry = lookup_restartcb_table[index];

  while( entry != NULL )
  {
    if(  (entry->client_addr == client_addr) && (entry->svr_exit_cb == cb_func) && (entry->cb_data == cb_data ) )
    {
      /* The entry already exists, no need to add a 2nd entry */
      oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
      return;
    }
    entry=entry->next;
  }

  /**************************************************************
  * Client Not Found, Create a new client for this program
  * and version number.
  **************************************************************/

  new_entry = oncrpc_mem_alloc(sizeof(lookup_exitcb_type));
  if(new_entry == NULL)
  {
    ERR("oncrpc_register_server_restart_notification_cb: Memory allocation failed for new_entry\n", 0, 0, 0);
    oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
    return;
  }
  new_entry->client_addr = client_addr;
  new_entry->svr_exit_cb = cb_func;
  new_entry->clnt_exit_cb = NULL;
  new_entry->cb_data = cb_data;
  /* Always put new clients in the table (head)        */
  /* The previous pointer to next is the head pointer! */
  new_entry->prev_next = &lookup_restartcb_table[index];
  /* The next pointer is what the head used to point to */
  /* It's null if this is the first one */
  new_entry->next = lookup_restartcb_table[index];

  if( lookup_restartcb_table[index] )
  {
    lookup_restartcb_table[index]->prev_next = &new_entry->next;
  }
  lookup_restartcb_table[index] = new_entry;
  oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
}

/*===========================================================================
FUNCTION      ONCRPC_UNREGISTER_SERVER_RESTART_NOTIFICATION_CB

DESCRIPTION   Remove a restart-notification callback entry.
              Will remove all restart-notification callbacks that match the
              message source address and the callback function pointer.

ARGUMENTS     xdr    : Used to find message source.
              cb_func: Callback function to call for restart server-up

RETURN VALUE  void * pointer to cb data

SIDE EFFECTS  Memory free for entries removed.
===========================================================================*/
void *oncrpc_unregister_server_restart_notification_cb(
  uint32 prog,
  uint32 ver,
  oncrpc_server_exit_notification_f_type cb_func,
  void                  *cb_data,
  oncrpc_compare_f_type  compare
  )
{
  xdr_s_type *xdr;
  oncrpc_addr_type client_addr, *dest_ptr;
  uint32              index;
  lookup_exitcb_type* entry;
  lookup_exitcb_type* entry_head;
  void               *rval = NULL;

  xdr = rpc_clnt_lookup2(prog, ver, RPC_CLNT_LOOKUP_TIMEOUT);
  dest_ptr = oncrpc_lookup_get_dest(prog, ver);

  if(dest_ptr == NULL) {
    ERR_FATAL("oncrpc_lookup_dest() returned NULL!", 0,0,0);
  }

  client_addr = *dest_ptr;

  index = GET_EXITCB_TABLE_INDEX(client_addr);
  entry_head = lookup_restartcb_table[index];

  oncrpc_crit_sect_enter(lookup_exitcb_list_critic_sect_ptr);
  entry = entry_head;

  while( entry != NULL )
  {
    if( (entry->client_addr == client_addr) &&
        (entry->svr_exit_cb == cb_func) &&
        (compare(entry->cb_data, cb_data)) )
    {
      *entry->prev_next = entry->next;
      if( entry->next )
      {
        (entry->next)->prev_next = entry->prev_next;
      }
      rval = entry->cb_data;
      oncrpc_mem_free(entry);
      break;
    }
    entry=entry->next;
  }
  oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);

  return rval;
}

/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_EXECUTE_EXITCBS

DESCRIPTION   Execute a callback exit function.
              This is called when a client has terminated and we need
              to execute all the callback exit functions registered
              to this client.

              Will remove all callback exit functions that match the
              message source address and the callback function pointer.

ARGUMENTS     client_addr_ptr: pointer to the callback client address.

RETURN VALUE  num_entries_removed: number of entries removed.

SIDE EFFECTS  Memory free for entries removed.
===========================================================================*/
uint32 oncrpc_lookup_execute_exitcbs(
    oncrpc_addr_type *client_addr_ptr
  )
{
  oncrpc_addr_type client_addr = *client_addr_ptr;
  uint32 index = GET_EXITCB_TABLE_INDEX(client_addr);
  lookup_exitcb_type* entry;
  lookup_exitcb_type* next_entry;
  lookup_exitcb_type* entry_head = lookup_exitcb_table[index];
  uint32 num_entries_removed=0;
  boolean found_server = FALSE;

  oncrpc_crit_sect_enter(lookup_exitcb_list_critic_sect_ptr);
  entry = entry_head;
  while( entry != NULL )
  {
    next_entry=entry->next;
    if(entry->client_addr == client_addr )
    {
      *entry->prev_next = entry->next;
      if( entry->next )
      {
        (entry->next)->prev_next = entry->prev_next;
      }


      if(entry->svr_exit_cb)
      {
        found_server = TRUE;

        /* Add to the front of the cleanup_list */
        if(cleanup_list == NULL)
        {
          /* First item in the list */
          cleanup_list = entry;
          entry->next = NULL;
          entry->prev_next = &entry->next;
        }
        else
        {
          /* List exits, add to front */
          entry->next = cleanup_list;
          cleanup_list->prev_next = &entry->next;
          cleanup_list = entry;
        }

        /* Start bailout timer */
        if(!timer_started)
        {
          /* Serialize the processing of timers in case we get two messages
           * in a row
           */
          bailout_server_addr = client_addr;
          timer_started = TRUE;
          oncrpc_timer_set(timer, ONCRPC_CLEANUP_BAILOUT_TIME_MS);
        }
        /* Release critical section before invoking the callback */
        oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
        entry->svr_exit_cb(entry, entry->cb_data);
        oncrpc_crit_sect_enter(lookup_exitcb_list_critic_sect_ptr);

        /* Note that memory is not freed here. Done later in the cleanup_done
         * function
         */
      }
      else if(entry->clnt_exit_cb)
      {
        /* Release critical section before invoking the callback */
        oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
        entry->clnt_exit_cb(entry->cb_data);
        oncrpc_crit_sect_enter(lookup_exitcb_list_critic_sect_ptr);
        oncrpc_mem_free(entry);
      }

      num_entries_removed++;
    }

    entry=next_entry;

  }
  if(!found_server) {
    RPC_MSG_HIGH("No stack cleanup needed. Call daemon cleanup done.\n",0,0,0);
    /* No clean up to be done. Just ack the daemon */

   /*
     DAEMON not supported on LINUX
       oncrpc_daemon_cleanup_done((uint32)(client_addr & 0xffffffff),
                               (uint32)((client_addr>>32) & 0xffffffff));
   */
  }
  oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
  return num_entries_removed;
}

/*===========================================================================
FUNCTION      oncrpc_lookup_execute_restartcbs

DESCRIPTION   Execute a callback restart function.

              This is called after a client has terminated and is then
              restarted.

              Will remove all callback exit functions that match the
              message source address and the callback function pointer.

ARGUMENTS     client_addr_ptr: pointer to the callback client address.

RETURN VALUE  num_entries_removed: number of entries removed.

SIDE EFFECTS  Memory free for entries removed.
===========================================================================*/
uint32 oncrpc_lookup_execute_restartcbs(
    oncrpc_addr_type *client_addr_ptr
  )
{
  oncrpc_addr_type client_addr = *client_addr_ptr;
  uint32 index = GET_EXITCB_TABLE_INDEX(client_addr);
  lookup_exitcb_type* entry;
  lookup_exitcb_type* next_entry;
  lookup_exitcb_type* entry_head = lookup_restartcb_table[index];
  uint32 num_entries_removed=0;
  boolean found_server = FALSE;

  oncrpc_crit_sect_enter(lookup_exitcb_list_critic_sect_ptr);
  entry = entry_head;
  while( entry != NULL )
  {
    next_entry=entry->next;
    if(entry->client_addr == client_addr )
    {
      *entry->prev_next = entry->next;
      if( entry->next )
      {
        (entry->next)->prev_next = entry->prev_next;
      }

      if(entry->svr_exit_cb)
      {
        found_server = TRUE;

        /* Release critical section before invoking the callback */
        oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
        entry->svr_exit_cb(entry, entry->cb_data);
        oncrpc_crit_sect_enter(lookup_exitcb_list_critic_sect_ptr);

        /* free entry immediately - client should not call
         * oncrpc_cleanup_done() for up notifications */
        oncrpc_mem_free(entry);
      }
      else if(entry->clnt_exit_cb)
      {
        /* Release critical section before invoking the callback */
        oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
        entry->clnt_exit_cb(entry->cb_data);
        oncrpc_crit_sect_enter(lookup_exitcb_list_critic_sect_ptr);
        oncrpc_mem_free(entry);
      }
      num_entries_removed++;
    }
    entry=next_entry;
  }

  oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
  return num_entries_removed;
}


/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_GET_STATUS

DESCRIPTION   Returns the number of currently registered callbacks.

ARGUMENTS     None

RETURN VALUE  Number of currently registered callbacks.

SIDE EFFECTS  None
===========================================================================*/
int oncrpc_lookup_get_status( void )
{
  lookup_client_type *client;
  int32               idx;
  int32               cnt;

  cnt = 0;

  oncrpc_crit_sect_enter(lookup_list_critic_sect_ptr);

  for( idx = 0; idx < ONCRPC_LOOKUP_PROGRAM_TABLE_SIZE; idx++ )
  {
    client = lookup_client_table[idx];

    while( client != NULL )
    {
      client=client->next;
      cnt++;
    }
  }

  oncrpc_crit_sect_leave(lookup_list_critic_sect_ptr);

  return (int) cnt;
} /* oncrpc_lookup_get_status */

/*===========================================================================
FUNCTION      ONCRPC_CLEANUP_DONE

DESCRIPTION   The client cleanup callback uses this function to signal that
              is has cleaned up after a server termination/restart. When all
              clients have signaled, call oncrpc_daemon_cleanup_done.

ARGUMENTS     Terminated server's address

RETURN VALUE  None

SIDE EFFECTS  None
===========================================================================*/
void oncrpc_cleanup_done(void *exit_handle)
{
  uint32 index ;
  lookup_exitcb_type *entry, *self_entry = (lookup_exitcb_type *)exit_handle;
  oncrpc_addr_type svr_addr = self_entry->client_addr;

  oncrpc_crit_sect_enter(lookup_exitcb_list_critic_sect_ptr);
  /* Remove self from cleanup waiting list */
  if(cleanup_list == self_entry)
  {
    cleanup_list=self_entry->next;
  }
  else
  {
    entry = cleanup_list;
    while(entry)
    {
      if(entry == self_entry)
      {
        *self_entry->prev_next = self_entry->next;
        break;
      }
      entry = entry->next;
    }
  }

  index = GET_EXITCB_TABLE_INDEX(self_entry->client_addr);
  entry = lookup_exitcb_table[index];

  oncrpc_mem_free(self_entry);

  while( entry != NULL )
  {
    if((entry->client_addr == svr_addr) && entry->svr_exit_cb)
    {
      /* There are still callbacks to be invoked, or we are waiting for clients
       * to check in (cleanup_list non-empty), do nothing
       */
      oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
      return;
    }
    entry=entry->next;
  }

  if(cleanup_list)
  {
    /* Still waiting on other clients */
    oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);
    return;
  }


  RPC_MSG_HIGH("Done stack cleanup. Call daemon cleanup done.\n",0,0,0);

  /*
   * Cancel the timer since everyone has responded
   */
  oncrpc_timer_clr(timer);
  timer_started = FALSE;
  oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);

 /*
  DAEMON not supported on LINUX
  oncrpc_daemon_cleanup_done( (uint32)(svr_addr & 0xffffffff),
                              (uint32)((svr_addr>>32) & 0xffffffff));
*/
}


/*===========================================================================
FUNCTION      ONCRPC_BAILOUT_TIMER_CB

DESCRIPTION   Cleans up local data if the bailout timer expires and signals
              daemon that the cleanup is done

ARGUMENTS     Data passed to callback registration

RETURN VALUE  None

SIDE EFFECTS  None
===========================================================================*/
void oncrpc_bailout_timer_cb(unsigned long data)
{
  lookup_exitcb_type *prev, *entry = cleanup_list;
  /*
  oncrpc_addr_type svr_addr = bailout_server_addr;
*/

  MSG_ERROR("ONCRPC stack bailout timer fired!\n",0,0,0);

  oncrpc_crit_sect_enter(lookup_exitcb_list_critic_sect_ptr);
  /* Delete the clients that we are waiting on */
  while(entry)
  {
    MSG_ERROR("Callback at 0x%lx not responding!\n",(uint32)entry->svr_exit_cb,0,0);
    prev = entry;
    entry = entry->next;
    oncrpc_mem_free(prev);
  }

  cleanup_list = NULL;

  oncrpc_timer_clr(timer);
  timer_started = FALSE;
  oncrpc_crit_sect_leave(lookup_exitcb_list_critic_sect_ptr);

   /*
   DAEMON not supported on LINUX
  oncrpc_daemon_cleanup_done( (uint32)(svr_addr & 0xffffffff),
                              (uint32)((svr_addr>>32) & 0xffffffff));
   */
}
