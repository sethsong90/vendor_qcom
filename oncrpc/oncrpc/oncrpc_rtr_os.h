#ifndef ONCRPC_RTR_OS_H
#define ONCRPC_RTR_OS_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                    O N C R P C _ R T R _ O S . H

GENERAL DESCRIPTION

  This is the ONCRPC transport OS abstration

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2007-2010 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_rtr_os.h#6 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
11/11/08     rr    Add xprtrtr_os_process_restart for modem restart handling
05/02/07     ih    Initial version

===========================================================================*/

#define XPORT_HANDLE_TYPE int32

/*===========================================================================
FUNCTION      xprtrtr_os_init

DESCRIPTION   Initialization function, os specific.

ARGUMENTS     N/A

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
void xprtrtr_os_init(void);

/*===========================================================================
FUNCTION      xprtrtr_os_open

DESCRIPTION   Open a port to the router.  The router will open a port and
              assign a handle.  This handle is to be use in all subsequent
              operations on the rpc_router for this port.

ARGUMENTS     

RETURN VALUE  File descriptor

SIDE EFFECTS  None
===========================================================================*/
extern XPORT_HANDLE_TYPE xprtrtr_os_open( const char *program_version );

/*===========================================================================
FUNCTION      xprtrtr_os_close

DESCRIPTION   Close an opened port to the router.

ARGUMENTS     handle - File handle

RETURN VALUE  None

SIDE EFFECTS  None
===========================================================================*/
extern void xprtrtr_os_close( XPORT_HANDLE_TYPE handle );

/*===========================================================================
FUNCTION      xprtrtr_os_read

DESCRIPTION   Read data from RPC router

ARGUMENTS     handle - client handle returned from xprtrtr_os_open
              buf - user-space double pointer to consume data from this
              function
              size - maximum size of data to read

RETURN VALUE  number of bytes read, -1 on error

SIDE EFFECTS  
===========================================================================*/
extern int xprtrtr_os_read
(
   XPORT_HANDLE_TYPE *handle,
   char **buf,
   uint32 size
);

/*===========================================================================
FUNCTION      xprtrtr_os_write

DESCRIPTION   Write data to RPC router

ARGUMENTS     handle - client handle returned from xprtrtr_os_open
              buf - user-space pointer to copy data
              size - maximum size of data to read

RETURN VALUE  number of bytes read, -1 on error

SIDE EFFECTS  
===========================================================================*/
extern int xprtrtr_os_write
(
   XPORT_HANDLE_TYPE handle,
   const char *buf,
   uint32 size
);

/*===========================================================================
FUNCTION      xprtrtr_os_control

DESCRIPTION   Send IOCTL to RPC router

ARGUMENTS     handle - client handle returned from xprtrtr_os_open
              cmd - IOCTL command
              arg - IOCTL argument

RETURN VALUE  0 on success, -1 on error

SIDE EFFECTS  
===========================================================================*/
extern int xprtrtr_os_control
(
   XPORT_HANDLE_TYPE handle,
   const uint32 cmd,
   void *arg
);



/*===========================================================================
FUNCTION      xprtrtr_os_access

DESCRIPTION   File access function

ARGUMENTS     filename - name of file to determine access
              
RETURN VALUE  0 on success, -1 on error

SIDE EFFECTS  
===========================================================================*/
extern int xprtrtr_os_access
(   
   const char *filename   
);


/*===========================================================================
FUNCTION     xprtrtr_os_process_restart

DESCRIPTION   Handle Restart for file handles that are active and 
              pending restart.

ARGUMENTS     N/A

RETURN VALUE  N/A

SIDE EFFECTS  Files will restart reading
===========================================================================*/
void xprtrtr_os_process_restart(uint32 handle );

/*===========================================================================
FUNCTION      xprtrtr_os_deinit

DESCRIPTION   Deinitialization function, os specific

ARGUMENTS     None

RETURN VALUE  None

SIDE EFFECTS
===========================================================================*/
void xprtrtr_os_deinit( void );

#endif /* ! ONCRPC_RTR_OS_H */
