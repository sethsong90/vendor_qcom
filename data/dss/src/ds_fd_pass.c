/******************************************************************************

                        D S _ F D _ P A S S . C

******************************************************************************/

/******************************************************************************

  @file    ds_fd_pass.c
  @brief   File Descriptor Passing Routines

  DESCRIPTION
  Implementation of file descriptor passing routines.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2008,2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/dss/src/ds_fd_pass.c#3 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/04/08   vk         Added support for client control of logging in dss
03/24/08   vk         Incorporated code review comments
11/30/07   vk         Cleaned up lint warnings
11/26/07   vk         Added function headers and other comments
11/06/07   vk         Using safe string functions
09/28/07   vk         Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "stringl.h"
#include "ds_linux.h"
#include "ds_fd_pass.h"
#include "ds_util.h"

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

#ifdef FEATURE_DS_LINUX_NO_RPC
#define DS_RPC_UDS_SRVR_PATH "/tmp/vaibhavk_test_rpc_server.socket"
#endif

#ifndef FEATURE_DS_LINUX_NO_RPC
//static char Socket_msg[] = "SOCKET";
#endif


/*--------------------------------------------------------------------------- 
   Constant representing maximum length of message received/sent over unix 
   domain socket
---------------------------------------------------------------------------*/
#define MAXCMDLEN 128

/*--------------------------------------------------------------------------- 
   Constant string representing the message contents of a Recv FD message
---------------------------------------------------------------------------*/
static const char Recvfd_msg[] = "RECVFD";

/*--------------------------------------------------------------------------- 
   Constant string representing the message contents of a Handshake message
---------------------------------------------------------------------------*/
static const char Handshake_msg[] = "HANDSHAKE";

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
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
int
ds_open_uds_srvr (const char * spath)
{
    int fd;
    struct sockaddr_un saddr_my; 
    size_t saddr_len;

    /* Delete if pathname already exists. This can happen if the server died 
    ** before the pathname could be deleted. If we don't delete, bind to this 
    ** address will fail.
    */
    unlink(spath);

    /* Initialize address structure */
    memset(&saddr_my, 0, sizeof(saddr_my));

    /* Set address family to unix domain sockets */
    saddr_my.sun_family = AF_UNIX;

    /* Set address to the requested pathname */
    (void)strlcpy(saddr_my.sun_path, spath, sizeof(saddr_my.sun_path));

    /* Get length of pathname */
    saddr_len = strlen(spath);

    /* Truncate address to the maximum allowed */
    if (saddr_len > sizeof(saddr_my.sun_path)) {
        saddr_len = sizeof(saddr_my.sun_path);
    }

    /* Get total size of the address (pathname + size of everything before it 
    ** in the unix domain address struct). 
    */
    saddr_len += (size_t)offsetof(struct sockaddr_un, sun_path);

    /* Open socket of Datagram type */
    if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        return fd;
    }

    /* Bind socket to the server address */
    if (bind(fd, (struct sockaddr *)&saddr_my, saddr_len) < 0) {
        close(fd);
        return -1;
    }

    /* Return socket descriptor */
    return fd;
}

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
int 
ds_open_uds_clnt (const char * spath, const char * cpath)
{
    int fd;
    struct sockaddr_un saddr_to;
    struct sockaddr_un saddr_my;
    size_t saddr_len;

    /* Open unix domain socket of datagram type */
    if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        return fd;
    }

    /* Delete local socket pathname if already exists. This may happen if 
    ** the client process earlier died before deleting the pathname. If we 
    ** don't do this, bind to this address will fail.
    */
    unlink(cpath);

    /* Initialize memory for the local address struct */
    memset(&saddr_my, 0, sizeof(saddr_my));

    /* Set address family in local address struct to unix domain sockets */
    saddr_my.sun_family = AF_UNIX;

    /* Set local address to the specified client-side pathname */
    (void)strlcpy(saddr_my.sun_path, cpath, sizeof(saddr_my.sun_path));

    /* Set pathname length */
    saddr_len = strlen(cpath);

    /* Truncate pathname length to the maximum allowed */
    if (saddr_len > sizeof(saddr_my.sun_path)) {
        saddr_len = sizeof(saddr_my.sun_path);
    }

    /* Get total size of the address (pathname + size of everything before it 
    ** in the unix domain address struct). 
    */
    saddr_len += offsetof(struct sockaddr_un, sun_path);

    /* Bind socket to the local address */
    if (bind(fd, (struct sockaddr *)&saddr_my, saddr_len) < 0) {
        close(fd);
        return -1;
    }

    /* Initialize memory for the remote address struct */
    memset(&saddr_to, 0, sizeof(saddr_to));

    /* Set address family in remote address struct to unix domain sockets */
    saddr_to.sun_family = AF_UNIX;

    /* Set remote address to the specified server-side pathname */
    (void)strlcpy(saddr_to.sun_path, spath, sizeof(saddr_to.sun_path));

    /* Set pathname length */
    saddr_len = strlen(spath);

    /* Truncate pathname length to the maximum allowed */
    if (saddr_len > sizeof(saddr_to.sun_path)) {
        saddr_len = sizeof(saddr_to.sun_path);
    }

    /* Get total size of the address (pathname + size of everything before it 
    ** in the unix domain address struct). 
    */
    saddr_len += (size_t)offsetof(struct sockaddr_un, sun_path);

    /* Connect socket to the given server address */
    if (connect(fd, (struct sockaddr *)&saddr_to, saddr_len) < 0) {
        close(fd);
        return -1;
    }

    /* Return socket descriptor */
    return fd;
}

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
int
ds_connect_to_uds_clnt 
(
    int fd, 
    struct sockaddr_un * saddr_clnt, 
    socklen_t saddr_len
)
{
    /* Connect socket to the specified client address */
    if (connect(fd, (struct sockaddr *)saddr_clnt, saddr_len) < 0) {
        return -1;
    }

    return 0;
}

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
int
ds_send_fd_over_uds (int ufd, int sfd)
{
    struct iovec iov[1];
    struct msghdr msg;
    struct cmsghdr * cmsgp;
    size_t cmsglen;

    /* Verify that descriptors are valid before proceeding */
    if ((ufd < 0) || (sfd < 0)) {
        return -1;
    }

    /* Set message buffer pointer and length to the desired values */
    iov[0].iov_base = (char *)Recvfd_msg;
    iov[0].iov_len = sizeof(Recvfd_msg);

    /* Set iov ptr and iov length in the msghdr struct */
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    /* Msg name and name length are set to NULL as the socket is already 
    ** connected. 
    */
    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    /* Control message length is message of hdr plus size of int, i.e. size of 
    ** descriptor. 
    */
    cmsglen = CMSG_LEN(sizeof(int));

    /* Allocate memory for control message */
    if ((cmsgp = malloc(cmsglen)) == NULL) {
        ds_log("cannot malloc!\n");
        return -1;
    }

    /* Set fields appropriately in the control message struct */
    cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type = SCM_RIGHTS;
    cmsgp->cmsg_len = cmsglen;

    /* Set control message ptr and length in the msghdr struct */
    msg.msg_control = cmsgp;
    msg.msg_controllen = cmsglen;

    /* Set data value in the control message to the descriptor to pass */
    *((int *)CMSG_DATA(cmsgp)) = sfd;

    /* Send message over the unix domain socket */
    if (sendmsg(ufd, &msg, 0) != (int)sizeof(Recvfd_msg)) {
        ds_log("sendmsg failed!\n");
        return -1;
    }

    /* Free memory allocated for the control message */
    free(cmsgp);

    return 0;
}

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
int 
ds_recv_fd_over_uds (int ufd)
{
    int rfd = -1;
    char buf[MAXCMDLEN];
    int nread;
    struct iovec iov[1];
    struct msghdr msg;
    struct cmsghdr * cmsgp;
    size_t cmsglen;

    /* Verify descriptor before proceeding */
    if (ufd < 0) {
        return -1;
    }

    /* Set message buffer ptr and buffer size in iov[0] */
    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf);

    /* Set iov ptr and length in msghdr struct */
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    /* Name and name length are not used as the socket is connected */
    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    /* Get length of control message needed, which is size of control message 
    ** header plus space for the file descriptor. 
    */
    cmsglen = CMSG_LEN(sizeof(int));

    /* Allocate memory for the control message */
    if ((cmsgp = malloc(cmsglen)) == NULL) {
        ds_log("cannot malloc!\n");
        return -1;
    }

    /* Set control msg ptr and length in msghdr struct */
    msg.msg_control = cmsgp;
    msg.msg_controllen = cmsglen;

    /* Block on receive to receive the file descriptor */
    if ((nread = recvmsg(ufd, &msg, 0)) <= 0) {
        ds_log("recvmsg failed!\n");
    } else if (nread != (int)sizeof(Recvfd_msg)) {
        ds_log("received message length doesn't match!\n");
    } else {
        /* Received message of expected length. Verify that the message 
        ** contents are valid. 
        */
        if (strncmp(buf, Recvfd_msg, sizeof(Recvfd_msg)) != 0) {
            ds_log("received garbled message!\n");
        } else {
            /* Everything looks's good! Read descriptor fron control message */
            rfd = *((int *)CMSG_DATA(cmsgp));
        }
    }

    /* Free memory allocated for control message */
    free(cmsgp);

    /* Return the received descriptor */
    return rfd;
}

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
int
ds_send_handshake_over_uds (int ufd)
{
    /* Send well known 'Handshake' message over unix domain socket */
    if (send(ufd, Handshake_msg, sizeof(Handshake_msg), 0) 
        != (int)sizeof(Handshake_msg)) 
    {
        ds_log("error sending handshake cmd!\n");
        return -1;
    }

    return 0;
}

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
int
ds_recv_initial_handshake_over_uds 
(
    int ufd, 
    struct sockaddr_un * clntaddr, 
    socklen_t * addrlen
)
{
    char buf[sizeof(Handshake_msg)];

    /* Block on receiving 'Handshake' message over the unix domain socket. 
    ** Save address of sender in clntaddr struct.
    */
    memset( buf, 0x0, sizeof(buf) );
    if (recvfrom(ufd, buf, sizeof(buf), 0, (struct sockaddr *)clntaddr, addrlen)
        != (int)sizeof(Handshake_msg)) 
    {
        ds_log("error receiving handshake cmd!\n");
        return -1;
    }

    /* Verify that message received is a 'Handshake' message */
    if (strncmp(buf, Handshake_msg, sizeof(buf)) != 0) {
        ds_log("error receiving handshake cmd - length doesn't match!\n");
        return -1;
    }

    /* Return 0 to indicate that a 'Handshake' message was received */
    return 0;
}

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
int
ds_recv_final_handshake_over_uds (int ufd)
{
    char buf[sizeof(Handshake_msg)];

    /* Block on receiving a 'Handshake' message over the unix domain socket */
    memset( buf, 0x0, sizeof(buf) );
    if (recv(ufd, buf, sizeof(buf), 0) != (int)sizeof(Handshake_msg)) 
    {
        ds_log("error receiving handshake cmd!\n");
        return -1;
    }

    /* Verify that a valid handshake message was received */
    if (strncmp(buf, Handshake_msg, sizeof(buf)) != 0) {
        ds_log("error receiving handshake cmd - length doesn't match!\n");
        return -1;
    }

    /* Return 0 to indicate that a 'Handshake' message was received */
    return 0;
}

#ifdef FEATURE_DS_LINUX_TEST_OS

int
ds_open_uds_srvr_for_rpc (void)
{
    int fd;
    struct stat statbuf;

	unlink(DS_RPC_UDS_SRVR_PATH);

    fd = ds_open_uds_srvr(DS_RPC_UDS_SRVR_PATH);
    if (fd > 0) {
        if (fstat(fd, &statbuf) < 0) {
            perror("fstat on uds for rpc server failed!\n");
            return fd;
        }
        if (fchmod(fd, statbuf.st_mode | S_IROTH | S_IWOTH) < 0) {
            perror("fchmod failed for uds for rpc server!\n");
        }
    }
    return fd;
}

int
ds_open_uds_clnt_for_rpc (void)
{
    return ds_open_uds_clnt(DS_RPC_UDS_SRVR_PATH);
}

int
ds_send_socket_cmd_over_uds (int ufd, const char * spath, int spathlen)
{
    int nwrote;
    if ((nwrote = send(ufd, Socket_msg, sizeof(Socket_msg), 0)) != sizeof(Socket_msg)) {
        ds_log("error sending socket cmd!\n");
        return -1;
    }
    if ((nwrote = send(ufd, spath, spathlen, 0)) != spathlen) {
        ds_log("error sending server path!\n");
        return -1;
    }
    return 0;
}

int
ds_recv_socket_cmd_over_uds (int ufd, char * spath, int * spathlen)
{
    int nread;
    char buf[sizeof(Socket_msg)];

    if ((nread = recv(ufd, buf, sizeof(buf), 0)) != sizeof(Socket_msg)) {
        ds_log("error receiving socket cmd!\n");
        return -1;
    }
    if (strncmp(buf, Socket_msg, sizeof(buf)) != 0) {
        ds_log("error receiving socket cmd - length doesn't match!\n");
        return -1;
    }
    if ((nread = recv(ufd, spath, *spathlen, 0)) <= 0) {
        ds_log("error receiving server path!\n");
        return -1;
    }
    *spathlen = nread;
    return 0;
}

#endif /* FEATURE_DS_LINUX_TEST_OS */
