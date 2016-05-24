/*==========================================================================

		 FTM ANT Source File

Description
  FTM platform independent processing of packet data

# Copyright (c) 2010-2012 by Qualcomm Technologies, Inc.  All Rights Reserved.
# Qualcomm Technologies Proprietary and Confidential.

===========================================================================*/

/*===========================================================================

		 Edit History


when       who      what, where, why
--------   ---      ----------------------------------------------------------
05/16/12  ankurn    Adding support for ANT commands
11/28/12  c_ssugas  implements efficent method for Ant cmd transfer
                     and implements Rx thread for event handling.
===========================================================================*/
#include "event.h"
#include "msg.h"
#include "log.h"

#include "diag_lsm.h"
#include "diagpkt.h"
#include "diagcmd.h"
#include "diag.h"
#include "termios.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include "ftm_ant_common.h"

/* Transport file descriptor */
int fd_transport_ant_cmd;
extern int first_ant_command;
/* Reader thread handle */
pthread_t ant_cmd_thread_hdl;
/* Pipe file descriptors for cancelling read operation */
int ant_pipefd[2];

/* Enable FTM_DEBUG to turn on Debug messages  */
//#define FTM_DEBUG

/*===========================================================================
FUNCTION   ftm_ant_readerthread

DESCRIPTION
  Thread Routine to perfom asynchrounous handling of events coming on Smd
  descriptor. It invokes a callback to the FTM ANT layer to intiate a request
  to read event bytes.

DEPENDENCIES
  The LifeTime of ReaderThraad is dependent on the status returned by the
  call to ftm_ant_qcomm_handle_event

RETURN VALUE
  RETURN NULL

SIDE EFFECTS
  None

===========================================================================*/
void *ftm_ant_readerthread(void *ptr)
{
   boolean status = FALSE;
   int retval;
   fd_set readfds;
   int buf;

#ifdef FTM_DEBUG
   printf("ftm_ant_readerthread --> \n");
#endif
   do
   {
      FD_ZERO(&readfds);
      FD_SET(fd_transport_ant_cmd, &readfds);
      FD_SET(ant_pipefd[0],&readfds);
      retval = select(ant_pipefd[0] + 1, &readfds, NULL, NULL, NULL);
      if(retval == -1)
      {
         printf("select failed\n");
         break;
      }
      if(FD_ISSET(ant_pipefd[0],&readfds))
      {
#ifdef FTM_DEBUG
         printf("Pipe descriptor set\n");
#endif
         read(ant_pipefd[0],&buf,1);
         if(buf == 1)
            break;
      }
      if(FD_ISSET(fd_transport_ant_cmd,&readfds))
      {
#ifdef FTM_DEBUG
         printf("Read descriptor set\n");
#endif
         status = ftm_ant_qcomm_handle_event();
         if(TRUE != status)
            break;
      }
   }
   while(1);
#ifdef FTM_DEBUG
   printf("\nReader thread exited\n");
#endif
   return 0;
}

/*===========================================================================
FUNCTION   ftm_ant_open_channel

DESCRIPTION
   Open the SMD transport associated with ANT

DEPENDENCIES
  NIL

RETURN VALUE
  int value indicating success or failure

SIDE EFFECTS
  NONE

===========================================================================*/
static bool ftm_ant_open_channel()
{
   struct termios term_port;
   int opts;

#ifdef FTM_DEBUG
   printf("ftm_ant_open_channel --> \n");
#endif

   fd_transport_ant_cmd = open(APPS_RIVA_ANT_CMD_CH, (O_RDWR));
   if (fd_transport_ant_cmd == -1) {
       printf("Ant Device open Failed= %d\n ", fd_transport_ant_cmd);
       return false;
   }

   // Blocking Read
   opts = fcntl(fd_transport_ant_cmd, F_GETFL);
   if (opts < 0) {
       perror("fcntl(F_GETFL)");
       exit(EXIT_FAILURE);
   }

   opts = opts & (~O_NONBLOCK);
   if (fcntl(fd_transport_ant_cmd, F_SETFL, opts) < 0) {
       perror("fcntl(F_SETFL)");
       exit(EXIT_FAILURE);
   }

   if (tcgetattr(fd_transport_ant_cmd, &term_port) < 0)
       close(fd_transport_ant_cmd);
   cfmakeraw(&term_port);
   if (tcsetattr(fd_transport_ant_cmd, TCSANOW, &term_port) < 0) {
       printf("\n Error while setting attributes\n");
       return false;
   }

   tcflush(fd_transport_ant_cmd, TCIFLUSH);
#ifdef FTM_DEBUG
   printf("ftm_ant_open_channel success \n");
#endif
   if (pipe(ant_pipefd) == -1)
   {
      printf("pipe create error");
      return STATUS_FAIL;
   }
/* Creating read thread which listens for various masks & pkt requests */
   pthread_create( &ant_cmd_thread_hdl, NULL, ftm_ant_readerthread, NULL);
   return true;
}

/*===========================================================================
FUNCTION   ftm_log_send_msg

DESCRIPTION
  Processes the buffer sent and sends it to the libdiag for sending the Cmd
  response

DEPENDENCIES
  NIL

RETURN VALUE
  NIL

SIDE EFFECTS
  None

===========================================================================*/
void ftm_ant_log_send_msg(const uint8 *pEventBuf,int event_bytes)
{
   int result = log_status(LOG_FTM_VER_2_C);
   ftm_ant_log_pkt_type*  ftm_ant_log_pkt_ptr = NULL;

   if((pEventBuf == NULL) || (event_bytes == 0))
      return;
#ifdef FTM_DEBUG
   printf("ftm_ant_log_send_msg --> \n");
#endif
   if(result == 1)
   {
      ftm_ant_log_pkt_ptr = (ftm_ant_log_pkt_type *)log_alloc(LOG_FTM_VER_2_C,
	  FTM_ANT_LOG_HEADER_SIZE + (event_bytes-1));
      if(ftm_ant_log_pkt_ptr != NULL)
      {
         /* FTM ANT Log PacketID */
         ftm_ant_log_pkt_ptr->ftm_log_id = FTM_ANT_LOG_PKT_ID;
         memcpy((void *)ftm_ant_log_pkt_ptr->data,(void *)pEventBuf,event_bytes);
         log_commit( ftm_ant_log_pkt_ptr );
      }
   }
}

/*===========================================================================
FUNCTION   ftm_ant_dispatch

DESCRIPTION
  Dispatch routine for the various FM Rx/Tx commands. Copies the data into
  a global union data structure before calling the processing routine

DEPENDENCIES
  NIL

RETURN VALUE
  A Packed structre pointer including the response to the FTM FM packet

SIDE EFFECTS
  None

===========================================================================*/
void * ftm_ant_dispatch(ftm_ant_pkt_type *ant_ftm_pkt, uint16 pkt_len)
{
   ftm_ant_generic_sudo_res *rsp;
   int err;
   int data_len = ant_ftm_pkt->cmd_data_len;
   bool resp = false;
#ifdef FTM_DEBUG
   printf("ftm_ant_dispatch --> \n");
#endif
   if (first_ant_command == 0) {
       first_ant_command = 1;
       ftm_ant_open_channel();
   }

   rsp = (ftm_ant_generic_sudo_res*)diagpkt_subsys_alloc( DIAG_SUBSYS_FTM
                                                , FTM_ANT_CMD_CODE
                                                , sizeof(ftm_ant_generic_sudo_res)
                                                );


   /* Send the packet to controller and send a dummy response back to host*/
   err = write(fd_transport_ant_cmd, ant_ftm_pkt->data, data_len);
   if (err == data_len)
       rsp->result = FTM_ANT_SUCCESS;
   else {
       rsp->result	= FTM_ANT_FAIL;
       printf("FTM ANT write fail len: %d\n", err);
   }
   return(void *) rsp;
}

/*===========================================================================
FUNCTION   ftm_bt_hci_qcomm_handle_event

DESCRIPTION
 Routine called by the HAL layer reader thread to process the HCI events
 The post conditions of each event is covered in a state machine pattern

DEPENDENCIES
  NIL

RETURN VALUE
  RETURN VALUE
  FALSE = failure, else TRUE

SIDE EFFECTS
  None

===========================================================================*/

boolean ftm_ant_qcomm_handle_event ()
{
   boolean status = TRUE;
   int nbytes,len =0;
   ftm_ant_generic_res *res = (ftm_ant_generic_res *)diagpkt_subsys_alloc(
                                   DIAG_SUBSYS_FTM
                                   , FTM_ANT_CMD_CODE
                                   , sizeof(ftm_ant_generic_res)
                               );
#ifdef FTM_DEBUG
   printf("ftm_ant_hci_qcomm_handle_event --> \n");
#endif

   /* Read length of Ant Resp event*/
   nbytes = read(fd_transport_ant_cmd, (void *)res->evt, 1);
   if(nbytes <= 0) {
      status = FALSE;
      printf("ftm_ant_qcomm_handle_event read fail len=%d\n", nbytes);
      return status;
   }
   len = res->evt[0];
#ifdef FTM_DEBUG
   printf("length of event =%d\n",len);
#endif
   /* Read out the Ant Resp event*/
   nbytes = read(fd_transport_ant_cmd, (void *)res->evt, len);
   if (nbytes != len) {
       res->result = FTM_ANT_FAIL;
       status = FALSE;
       printf("ftm_ant_qcomm_handle_event read fail len=%d\n", nbytes);
   }
   else  {
       res->result = FTM_ANT_SUCCESS;
       ftm_ant_log_send_msg(res->evt, nbytes);
       tcflush(fd_transport_ant_cmd, TCIOFLUSH);
   }
   return status;
}
