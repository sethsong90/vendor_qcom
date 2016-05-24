/******************************************************************************

                        D S _ F D _ P A S S . H

******************************************************************************/

/******************************************************************************

  @file    ds_fd_pass.h
  @brief   File Descriptor Passing Routines Header file

  DESCRIPTION
  Header file for using file descriptor passing routines.

  ---------------------------------------------------------------------------
  Copyright (c) 2007 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/dss/inc/ds_fd_pass.h#2 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
11/26/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

#ifndef __DS_FD_PASS_H__
#define __DS_FD_PASS_H__

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <sys/socket.h>
#include <sys/un.h>

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Constant representing the maximum length of unix domain socket pathname
---------------------------------------------------------------------------*/
#define DS_MAX_UDS_PATH_LEN 128

/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  ds_open_uds_srvr
===========================================================================*/
/*!
@brief
  Opens a unix domain socket and binds the socket to the specified pathname
  (server address).

@return
  int - socket descriptor if socket is successfully opened, 
        -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int ds_open_uds_srvr (const char * spath);

/*===========================================================================
  FUNCTION  ds_open_uds_clnt
===========================================================================*/
/*!
@brief
  Opens a unix domain socket and connects the socket to the specified server.

@return
  int - socket descriptor if socket is successfully opened and connected, 
        -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int ds_open_uds_clnt (const char * spath, const char * cpath);

/*===========================================================================
  FUNCTION  ds_connect_to_uds_clnt
===========================================================================*/
/*!
@brief
  Connects the socket to the specified client address.

@return
  int - 0 if socket is successfully connected, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int ds_connect_to_uds_clnt 
(
    int fd, 
    struct sockaddr_un * saddr_clnt, 
    socklen_t saddr_len
);

/*===========================================================================
  FUNCTION  ds_send_fd_over_uds
===========================================================================*/
/*!
@brief
  Sends a message containing the socket descriptor to transport over the 
  specified socket to the connected client. 

@return
  int - 0 if the message is successfully sent, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int ds_send_fd_over_uds (int ufd, int sfd);

/*===========================================================================
  FUNCTION  ds_recv_fd_over_uds
===========================================================================*/
/*!
@brief
  Receives a message containing a socket descriptor over the specified 
  socket from the connected server. 

@return
  int - socket descriptor if the message is successfully received,
        -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int ds_recv_fd_over_uds (int ufd);

/*===========================================================================
  FUNCTION  ds_send_handshake_over_uds
===========================================================================*/
/*!
@brief
  Sends the Handshake message over the specified socket to the connected 
  remote end_point.  

@return
  int - 0 if the message is successfully sent, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int ds_send_handshake_over_uds (int ufd);

/*===========================================================================
  FUNCTION  ds_recv_initial_handshake_over_uds
===========================================================================*/
/*!
@brief
  Receives message containing the initial handshake from the client over 
  the specified socket. 

@return
  int - 0 if the message is successfully received, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int ds_recv_initial_handshake_over_uds 
(
    int ufd, 
    struct sockaddr_un * saddr, 
    socklen_t * saddrlen
);

/*===========================================================================
  FUNCTION  ds_recv_final_handshake_over_uds
===========================================================================*/
/*!
@brief
  Receives message containing the final handshake from the client over 
  the specified socket. 

@return
  int - 0 if the message is successfully received, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int ds_recv_final_handshake_over_uds (int ufd);

#ifdef FEATURE_DS_LINUX_NO_RPC

int ds_open_uds_srvr_for_rpc (void);
int ds_open_uds_clnt_for_rpc (void);
int ds_send_socket_cmd_over_uds (int ufd, const char * spath, int spathlen);
int ds_recv_socket_cmd_over_uds (int ufd, char * spath, int * spathlen);

#endif /* FEATURE_DS_LINUX_NO_RPC */

#endif /* __DS_FD_PASS_H__ */
