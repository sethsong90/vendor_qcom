/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in6.h>

#include "ds_fmc_app.h"

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif /*INET_ADDRSRLEN*/

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

static int tm_srvr_hdl = 0;
static int cm_clnt_hdl = 0;

/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/
/*===========================================================================
  FUNCTION  dfa_test_open_uds_srvr
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
static int
dfa_test_open_uds_srvr (const char * spath)
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
    memset(&saddr_my, 0, sizeof(struct sockaddr_un));

    /* Set address family to unix domain sockets */
    saddr_my.sun_family = AF_UNIX;

    fprintf(stderr, "dfa_test_open_uds_clnt: spath=%s, len %d\n",
	          spath, strlen(spath) );

    /* Set address to the requested pathname */
    memcpy(saddr_my.sun_path, spath, strlen(spath));

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
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        return fd;
    }

    fprintf(stderr, "dfa_test_open_uds_srvr: saddr_my=%s, saddr_len=%d\n",
	           saddr_my.sun_path, saddr_len );

    /* Bind socket to the server address */
    if (bind(fd, (struct sockaddr *)&saddr_my, saddr_len) < 0) {
      fprintf(stderr, "dfa_test_open_uds_srvr: bind failed, handle=0x%08x\n",
	          fd );
        close(fd);
        return -1;
    }

    fprintf(stderr, "dfa_test_open_uds_srvr: bind successful, handle=0x%08x\n",
	          fd );

    if( listen(fd, 0) < 0 ) {
      fprintf(stderr, "dfa_test_open_uds_srvr: listen failed, handle=0x%08x\n",
	          fd );
        close(fd);
        return -1;
    }

    fprintf(stderr, "dfa_test_open_uds_srvr: listen successful, handle=0x%08x\n",
	          fd );
    /* Return socket descriptor */
    return fd;
}

/*===========================================================================
  FUNCTION  dfa_test_open_uds_clnt
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
static int 
dfa_test_open_uds_clnt (const char * spath, const char * cpath)
{
    int fd;
    struct sockaddr_un saddr_to;
    struct sockaddr_un saddr_my;
    size_t saddr_len;

    /* Open unix domain socket of datagram type */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        return fd;
    }

    /* Delete local socket pathname if already exists. This may happen if 
    ** the client process earlier died before deleting the pathname. If we 
    ** don't do this, bind to this address will fail.
    */
    unlink(cpath);

    /* Initialize memory for the local address struct */
    memset(&saddr_my, 0, sizeof(struct sockaddr_un));

    /* Set address family in local address struct to unix domain sockets */
    saddr_my.sun_family = AF_UNIX;

    fprintf(stderr, "dfa_test_open_uds_clnt: cpath=%s, len %d\n",
	          cpath, strlen(cpath) );

    fprintf(stderr, "dfa_test_open_uds_clnt: spath=%s, len %d\n",
	          spath, strlen(spath) );

    /* Set local address to the specified client-side pathname */
    (void)memcpy(saddr_my.sun_path, cpath, strlen(cpath));

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

    fprintf(stderr, "dfa_test_open_uds_clnt: saddr_my=%s, saddr_len %d\n",
	           saddr_my.sun_path, saddr_len );


    /* Bind socket to the local address */
    if (bind(fd, (struct sockaddr *)&saddr_my, saddr_len) < 0) {
      fprintf(stderr, "dfa_test_open_uds_clnt: bind failed, handle=0x%08x\n",
	          fd );
        close(fd);
        return -1;
    }

    fprintf(stderr, "dfa_test_open_uds_clnt: bind successful, handle=0x%08x\n",
	          fd );

    /* Initialize memory for the remote address struct */
    memset(&saddr_to, 0, sizeof(struct sockaddr_un));

    /* Set address family in remote address struct to unix domain sockets */
    saddr_to.sun_family = AF_UNIX;

    /* Set remote address to the specified server-side pathname */
    (void)memcpy(saddr_to.sun_path, spath, strlen(spath));

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

    fprintf(stderr, "dfa_test_open_uds_clnt: saddr_to=%s, saddr_len %d\n",
	           saddr_to.sun_path, saddr_len );

    /* Connect socket to the given server address */
    if (connect(fd, (struct sockaddr *)&saddr_to, saddr_len) < 0) {
      fprintf(stderr, "dfa_test_open_uds_clnt: connect failed, handle=0x%08x\n",
	          fd );
        close(fd);
        return -1;
    }

    fprintf(stderr, "dfa_test_open_uds_clnt: connect successful, handle=0x%08x\n",
	          fd );

    /* Return socket descriptor */
    return fd;
}


/*===========================================================================
  FUNCTION  main
===========================================================================*/
/*!
@brief
  Main function to be executed.

@return
  int - DS_FMC_APP_SUCCESS on success, DS_FMC_APP_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
main(int argc, char ** argv)
{
  (void)argc; (void)argv;

  ds_fmc_app_fmc_bearer_status_type_t bearer_val = 
                                  DS_FMC_APP_FMC_BEARER_ENABLED;

  ds_fmc_app_fmc_bearer_status_type_t  recv_buf;

  ds_fmc_app_tunnel_mgr_ds_type        tunnel_mgr_ds;

  ds_fmc_app_call_mgr_resp_type        call_mgr_ds;

  int tunnel_mgr_client_fd;

  struct sockaddr_in *addr = NULL;

  char   addr_buf[INET6_ADDRSTRLEN];

  memset((void*) &tunnel_mgr_ds, 0, sizeof(ds_fmc_app_tunnel_mgr_ds_type));

  addr = (struct sockaddr_in*) (&tunnel_mgr_ds.tunnel_dest_ip);

  fprintf(stderr, "Test case to indicate TunnelMgr disabling FMC when SM is "
     "in the ENABLING state and is waiting for the Tunnel Mgr's response\n");

  /* Open the TUNNEL MGR server mimicking IMS behaviour */
  if((tm_srvr_hdl = dfa_test_open_uds_srvr(
                    DS_FMC_APP_TUNNEL_MGR_CONN_SOCKET_PATH)) < 0)
  {
    fprintf(stderr, "Error on dfa_test_open_uds_srvr\n");
    return DS_FMC_APP_FAILURE;
  }

  fprintf(stderr, "dfa_test_open_uds_srvr successful, handle=0x%08x\n",
	          tm_srvr_hdl );

  if((cm_clnt_hdl = dfa_test_open_uds_clnt( 
                    DS_FMC_APP_CALL_MGR_CONN_SOCKET_PATH,
                    DS_FMC_APP_CALL_MGR_CLIENT_SOCKET_PATH )) < 0 )
  {
    close(tm_srvr_hdl);
    fprintf(stderr, "Error on dfa_test_open_uds_clnt\n");
    return DS_FMC_APP_FAILURE;
  }

  fprintf(stderr, "dfa_test_open_uds_clnt successful, handle=0x%08x\n",
	          cm_clnt_hdl );

  /*Trigger the state machine*/
  if( send(cm_clnt_hdl, &bearer_val, 
      sizeof(ds_fmc_app_fmc_bearer_status_type_t), 0 ) < 0)
  {
    fprintf(stderr, "send on cm_clnt_hdl=0x%08x failed\n",
	          cm_clnt_hdl );
    close(cm_clnt_hdl);
    if(tm_srvr_hdl >=0 )
      close(tm_srvr_hdl);
    return DS_FMC_APP_FAILURE;
  }

  fprintf(stderr, "send %c on cm_clnt_hdl=0x%08x success\n",
	          bearer_val, cm_clnt_hdl );

  //Accept the tunnel_mgr_client connection

  tunnel_mgr_client_fd = (accept(tm_srvr_hdl, NULL, NULL));

  if(tunnel_mgr_client_fd < 0)
  {
    fprintf(stderr, "accept on tm_srvr_hdl=0x%08x failed\n",
	          tm_srvr_hdl );
    close(cm_clnt_hdl);
    close(tm_srvr_hdl);
    return DS_FMC_APP_FAILURE;
  }

  fprintf(stderr, "accept on tm_srvr_hdl=0x%08x success,"
                  " tunnel_mgr_client_fd %d\n", tm_srvr_hdl,
                    tunnel_mgr_client_fd );

  /*Receive Tunnel Manager trigger from SM*/
  if( recv(tunnel_mgr_client_fd, &recv_buf, 
      sizeof(ds_fmc_app_fmc_bearer_status_type_t), 0 ) < 0)
  {
    fprintf(stderr, "recv on tm_srvr_hdl=0x%08x failed\n",
	          tm_srvr_hdl );
    close(tunnel_mgr_client_fd);
    close(cm_clnt_hdl);
    close(tm_srvr_hdl);
    return DS_FMC_APP_FAILURE;
  }

  /* Verify whether what was sent by CM client was received by
     the TM server */

  fprintf(stderr, "recv_buf: %d\n", recv_buf );

  if(recv_buf != bearer_val)
  {
    fprintf(stderr, "recv_buf != bearer_val: %d\n",
	          recv_buf );
    close(tunnel_mgr_client_fd);
    close(cm_clnt_hdl);
    close(tm_srvr_hdl);
    return DS_FMC_APP_FAILURE;
  }

  /*Send Tunnel manager response*/

  memset((void*) &tunnel_mgr_ds, 0, sizeof(ds_fmc_app_tunnel_mgr_ds_type));

  tunnel_mgr_ds.is_tunnel_set = 0;

  if(send(tunnel_mgr_client_fd, &tunnel_mgr_ds, 
          sizeof(ds_fmc_app_tunnel_mgr_ds_type), 0) < 0)
  {
    fprintf(stderr, "Error while sending tunnel info on %d\n",
            tunnel_mgr_client_fd );
    close(tunnel_mgr_client_fd);
    close(cm_clnt_hdl);
    close(tm_srvr_hdl);
    return DS_FMC_APP_FAILURE;
  }

  fprintf(stderr, "completed resetting tunnel info on %d\n",
            tunnel_mgr_client_fd );

  memset(&call_mgr_ds, 0, sizeof(ds_fmc_app_call_mgr_resp_type));

  //Receive response on the CM, to indicate disabling tunnel

  if(recv(cm_clnt_hdl, &call_mgr_ds, 
          sizeof(ds_fmc_app_call_mgr_resp_type), 0) < 0)
  {
    fprintf(stderr, "Error while sending tunnel info on %d\n",
            tunnel_mgr_client_fd );
  }

  addr = (struct sockaddr_in*) &(call_mgr_ds.tunnel_dest_ip);

  switch(call_mgr_ds.tunnel_dest_ip.ss_family)
  {
    case AF_INET:
      addr = (struct sockaddr_in*) &(call_mgr_ds.tunnel_dest_ip);
      break;
    case AF_INET6:
    default:
      break;
  }

  memset(&addr_buf, 0, INET_ADDRSTRLEN);

  fprintf(stderr, "SIN_FAMILY of call_mgr_ds %d\n",
          addr->sin_family );

  inet_ntop(addr->sin_family, &(addr->sin_addr),
            addr_buf, INET_ADDRSTRLEN);

  fprintf(stderr,"Getting Bearer type %d of call_mgr_ds\n"
                 "Getting IP address %s of call_mgr_ds\n",
          call_mgr_ds.ds_fmc_app_fmc_bearer_status,
          addr_buf);

  sleep(3);

  close(tunnel_mgr_client_fd);
  close(cm_clnt_hdl);
  close(tm_srvr_hdl);
  
  return DS_FMC_APP_SUCCESS;

}
