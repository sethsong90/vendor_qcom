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

  ds_fmc_app_call_mgr_resp_type        call_mgr_ds;

  fprintf(stderr, "Test to ensure that the CM receives an FMC_ENABLED"
                  " /DISABLED indication when the SM fails to connect to"
                  " the tunnel mgr server\n");

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

  memset(&call_mgr_ds, 0, sizeof(ds_fmc_app_call_mgr_resp_type));

  if(recv(cm_clnt_hdl, &call_mgr_ds, 
          sizeof(ds_fmc_app_call_mgr_resp_type), 0) < 0)
  {
    fprintf(stderr, "Error while receiving cm_info on %d\n",
            cm_clnt_hdl );
  }

  fprintf(stderr, "call_mgr_ds.ds_fmc_app_fmc_bearer_status=%d \n",
	          call_mgr_ds.ds_fmc_app_fmc_bearer_status );

  for(;;)
  {
    fprintf(stderr, "waiting for CM to go down\n");
    sleep(10);
  }

  close(cm_clnt_hdl);
  if(tm_srvr_hdl >=0 )
    close(tm_srvr_hdl);

  return DS_FMC_APP_SUCCESS;

}
