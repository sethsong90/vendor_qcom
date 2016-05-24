#ifndef ONCRPC_LOOKUP_H
#define ONCRPC_LOOKUP_H
 /*===========================================================================

                    O N C R P C   L O O K U P

   This file is used to implement oncrpc client lookup database, which
   caches the ProgramNumber and ProgramVersion to RouterDestination
   lookups.
   Tuning.

   The database lookup is implemented using a hash - single linked-list
   model.  The hash table being 2^(x) long where x is a tunable parameter
   set by ONCRPC_LOOKUP_PROGRAM_MASK_SIZE_BITS.   The linked list is defined
   internally (as opposed to using qlist) for efficiency.

 Copyright (c) 2007-2009, 2011 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.

===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/inc/oncrpc/oncrpc_lookup.h#3 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
06/08/11    eh     Added server-up for restart notification
02/23/09    rr     Add oncrpc_lookup_get to support deinit
11/11/08    rr     Add oncrpc_lookup_get_prog_vers for restart support
03/05/08    ih     Restart daemon support
09/13/07    rr     Add support for removing clients and flush database
05/04/07    rr     Initial version, created to support rpc_router

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/


typedef void (*oncrpc_server_exit_notification_f_type)
(
 void *data,
 void *exit_handle
);

#define oncrpc_exitcb_type oncrpc_client_exit_notification_f_type
typedef void (*oncrpc_client_exit_notification_f_type)( void *data );


typedef boolean (*oncrpc_compare_f_type)(void *d1, void *d2);

/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_INIT

DESCRIPTION   Initialize the client lookup database

ARGUMENTS     None

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
void oncrpc_lookup_init(void);


/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_GET_DEST

DESCRIPTION   Get the router destination for this client

ARGUMENTS     program and version number to lookup.

RETURN VALUE  pointer to oncrpc_addr_type, address for
              prog, and version number.

SIDE EFFECTS  None
===========================================================================*/
oncrpc_addr_type *oncrpc_lookup_get_dest(uint32 prog, uint32 version);


/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_GET

DESCRIPTION   Get a client from list

ARGUMENTS     pointer to oncrpc_addr_type, prog and vers

RETURN VALUE  TRUE if the database is not empty, FALSE otherwise

SIDE EFFECTS  None
===========================================================================*/
boolean oncrpc_lookup_get(oncrpc_addr_type *addr, uint32 *prog, uint32 *vers);

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
);


/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_REMOVE_CLIENT

DESCRIPTION   Remove an entry from the lookup database.

ARGUMENTS     prog: program number
              version: program version

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
void oncrpc_lookup_remove_client(uint32 prog, uint32 version);

/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_REMOVE_CLIENTS_AT_ADDR

DESCRIPTION   Remove all entries from the lookup database that
              are served by this address.

ARGUMENTS     client_to_remove, address of client

RETURN VALUE  number of clients removed.

SIDE EFFECTS  All programs at client address will be removed.
===========================================================================*/
uint32 oncrpc_lookup_remove_clients_at_addr(oncrpc_addr_type client_to_remove);


/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_FLUSH_DATABASE

DESCRIPTION   Remove all client entries from database

ARGUMENTS     N/A

RETURN VALUE  number of clients removed

SIDE EFFECTS  Entire database will be flushed.
===========================================================================*/
uint32 oncrpc_lookup_flush_database(void);


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
   );


/*===========================================================================
FUNCTION      ONCRPC_REGISTER_CLIENT_EXIT_NOTIFICATION_CB

DESCRIPTION   Add a callback exit funtion entry.
              Will scan database first to determine if a current identical
			  exit function matches with the same source, function and data.
			  If the entry does not exist, it will be added.

ARGUMENTS     xdr    : Used to find message source.
              cb_func: Callback function to call on exit
              cb_data: Data pointer to callback with the exit function

RETURN VALUE  N/A

SIDE EFFECTS  Memory allocation
===========================================================================*/
#define oncrpc_register_exitcb oncrpc_register_client_exit_notification_cb
void oncrpc_register_client_exit_notification_cb(
  xdr_s_type         *xdr,
  oncrpc_client_exit_notification_f_type  cb_func,
  void               *cb_data
);

/*===========================================================================
FUNCTION      ONCRPC_UNREGISTER_CLIENT_EXIT_NOTIFICATION_CB

DESCRIPTION   Remove a callback exit funtion entry
              Will remove all callback exit functions that match the
              message source address and the callback function pointer.

ARGUMENTS     xdr    : Used to find message source.
              cb_func: Callback function to call on exit

RETURN VALUE  N/A

SIDE EFFECTS  Memory free for entries removed.
===========================================================================*/
#define oncrpc_unregister_exitcb oncrpc_unregister_client_exit_notification_cb
void *oncrpc_unregister_client_exit_notification_cb(
  xdr_s_type            *xdr,
    oncrpc_client_exit_notification_f_type  cb_func,
  void                  *cb_data,
  oncrpc_compare_f_type  compare
  );



/*===========================================================================
FUNCTION      ONCRPC_LOOKUP_CB_EXECUTE_EXIT

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
  oncrpc_addr_type *client_addr
  );

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
  );

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
);

/*===========================================================================
FUNCTION      ONCRPC_UNREGISTER_SERVER_EXIT_NOTIFICATION_CB

DESCRIPTION   Remove a callback exit funtion entry
              Will remove all callback exit functions that match the
              message source address and the callback function pointer.

ARGUMENTS     xdr    : Used to find message source.
              cb_func: Callback function to call on exit

RETURN VALUE  N/A

SIDE EFFECTS  Memory free for entries removed.
===========================================================================*/
void *oncrpc_unregister_server_exit_notification_cb(
  uint32 prog,
  uint32 ver,
  oncrpc_server_exit_notification_f_type cb_func,
  void                  *cb_data,
  oncrpc_compare_f_type  compare
);

/*===========================================================================
FUNCTION      ONCRPC_CLEANUP_DONE

DESCRIPTION   The client cleanup callback uses this function to signal that
              is has cleaned up after a server termination/restart. When all
              clients have signaled, call oncrpc_daemon_cleanup_done.

ARGUMENTS     Handle to client cleanup data structure

RETURN VALUE  None

SIDE EFFECTS  None
===========================================================================*/
void oncrpc_cleanup_done(void *exit_handle);

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
);

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
);

#endif /* ONCRPC_LOOKUP_H */
