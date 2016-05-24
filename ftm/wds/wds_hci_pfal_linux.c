/*==========================================================================

                     WDS Platform specfic HCI UART/SMD File

Description
  Platform specific routines to configure the UART and send/receive packets
  between UART and SMD channels

# Copyright (c) 2012 by Qualcomm Atheros, Inc..
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.

===========================================================================*/

/*===========================================================================

                         Edit History


when       who     what, where, why
--------   ---     ----------------------------------------------------------
11/18/11  subrsrin  Added support for forwarding packets to the SMD and
                     receiving packets from SMD
5/18/12   ankurn    Added support for ANT+ HCI commands
===========================================================================*/

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/select.h>
#include <termios.h>
#include <pthread.h>
#include <stdio.h>
#include "wds_hci_pfal.h"
#include <math.h>

#define WDS_DEBUG 0
#define MAX(a, b) ((a>b) ? a : b)

/* Reader thread handle */
pthread_t hci_cmd_thread_hdl;

int fd_transport_smd6;
int fd_transport_smd5;
int fd_transport_smd3;
int fd_transport_smd2;
int fd_transport_smd1;

extern int fd_transport_UART;
extern int option_selected;
typedef unsigned char uint8;

/*===========================================================================
FUNCTION   get_acl_length

DESCRIPTION
  Routine to calculate the ACL packet data length

DEPENDENCIES
  NIL

RETURN VALUE
  ACL packet data length

SIDE EFFECTS
  None

===========================================================================*/
int get_acl_pkt_length(uint8 length_acl_msb, uint8 length_acl_lsb)
{
  unsigned short para_acl_length = 0;
  para_acl_length = ((((unsigned short) length_acl_msb) << 8) & 0xFF00) |
                    (((unsigned short) length_acl_lsb) & 0x00FF);
  return((int) para_acl_length);
}

/*===========================================================================
FUNCTION   wds_readerthread

DESCRIPTION
  Thread Routine to perform asynchronous handling of events received at SMD
  descriptor. It writes the event bytes read at SMD interface to the UART
  interface.

DEPENDENCIES
  NIL

RETURN VALUE
  RETURN NIL

SIDE EFFECTS
  None

===========================================================================*/
void *wds_readerthread(void *ptr)
{
  int retval, i;
  fd_set readfds;
  uint8 bufPtr[SMD_BUFF_SIZE];
  int lsb,msb;
  uint8 bufResult[SMD_BUFF_SIZE];
  int event_para_length, acl_para_length, total_acl_length, data_para_length;
  int total_event_length, total_data_length;
  int write_bytes =0;
  int ntotal, nbytes;
  int bytes_count = 0;
  int temp;
  int event_bytes_count_processed = 0;
  int acl_bytes_count_processed = 0;
  int data_bytes_count_processed = 0;

  do
  {
    FD_ZERO(&readfds);
    FD_SET(fd_transport_smd6, &readfds);
    FD_SET(fd_transport_smd5, &readfds);
    FD_SET(fd_transport_smd3, &readfds);
    FD_SET(fd_transport_smd2, &readfds);
    FD_SET(fd_transport_smd1, &readfds);

    retval = select(MAX(MAX(MAX((MAX(fd_transport_smd1, fd_transport_smd2)),
                    fd_transport_smd3), fd_transport_smd5), fd_transport_smd6) + 1,
                    &readfds, NULL, NULL, NULL);

    if(retval == -1) {
        if(WDS_DEBUG)
            printf("select failed\n");
        break;
    }
    //If it is a BT event pkt
    if(FD_ISSET(fd_transport_smd3,&readfds)) {
      nbytes = 0;
      ntotal = 0;
      printf("\n Reading from SMD 3 interface\n");
      do
      {
        nbytes = read(fd_transport_smd3, (char *)(&(bufPtr[ntotal])), 1);
        ntotal += nbytes;
      } while ((nbytes > 0) && (ntotal < sizeof(bufPtr)));

      if(nbytes < 0) {
        if(WDS_DEBUG) {
          printf("\n Error while reading from SMD3 interface %d\n", nbytes);
          perror("The error is");
          printf("errno = %d.\n", errno);
        }
        if (errno == EINVAL)
          continue;
      }
      if(WDS_DEBUG) {
        printf("\n The event data read from SMD3 interface \n");
        for(i=0; i<ntotal; i++)
          printf(" %02X", bufPtr[i]);
      }

      if(ntotal > 0) {
        while(ntotal > event_bytes_count_processed)
        {
          //calculate event length
          event_para_length = bufPtr[1];
          total_event_length = event_para_length + BT_CMD_PKT_HDR_LEN;
          event_bytes_count_processed += total_event_length;
          bytes_count=0;

          bufResult[bytes_count++] = BT_EVT_PKT_ID;
          for(i=0; i<total_event_length; i++)
            bufResult[bytes_count++]= bufPtr[i];

          //write back to UART port
          write_bytes = write(fd_transport_UART, &bufResult, bytes_count);
          if(ntotal > event_bytes_count_processed)
          {
            for(i=0; i < ntotal; i++)
              bufPtr[i] = bufPtr[i+total_event_length];
          }
        }
        event_bytes_count_processed = 0;
      }
    }

    //If it is a FM event pkt
    if(FD_ISSET(fd_transport_smd1,&readfds)) {
      nbytes = 0;
      ntotal = 0;
      event_bytes_count_processed = 0;
      printf("\n Reading from SMD 1 interface\n");
      do
      {
        nbytes = read(fd_transport_smd1, (char *)(&(bufPtr[ntotal])), 1);
        ntotal += nbytes;
      } while ((nbytes > 0) && (ntotal < sizeof(bufPtr)));

      if (nbytes < 0) {
        if(WDS_DEBUG) {
          printf("\n Error while reading from SMD1 interface %d\n", nbytes);
          perror("The error is");
          printf("errno = %d.\n", errno);
        }
        if (errno == EINVAL)
          continue;
      }

      if(ntotal > 0) {
        while(ntotal > event_bytes_count_processed)
        {
          //calculate event length
          event_para_length = bufPtr[1];
          total_event_length = event_para_length + FM_CMD_PKT_HDR_LEN;
          event_bytes_count_processed += total_event_length;
          bytes_count=0;

          bufResult[bytes_count++] = FM_EVT_PKT_ID;
          for(i=0; i<total_event_length; i++)
            bufResult[bytes_count++]= bufPtr[i];

          //write back to UART port
          write_bytes = write(fd_transport_UART, &bufResult, bytes_count);
          if(ntotal > event_bytes_count_processed) {
            for(i=0; i<ntotal; i++)
              bufPtr[i] = bufPtr[i+total_event_length];
          }
        }
        event_bytes_count_processed = 0;
      }
    }

    //if it is an ACL pkt
    if(FD_ISSET(fd_transport_smd2,&readfds)) {
      nbytes = 0;
      ntotal = 0;
      event_bytes_count_processed = 0;
      printf("\n Reading from SMD 2 interface\n");
      do
      {
        nbytes = read(fd_transport_smd2, (char *)(&(bufPtr[ntotal])), 1);
        ntotal += nbytes;
      } while ((nbytes > 0) && (ntotal < sizeof(bufPtr)));

      if (nbytes < 0) {
        if(WDS_DEBUG) {
          printf("\n Error while reading from SMD2 interface %d\n", nbytes);
          perror("The error is");
          printf("errno = %d.\n", errno);
        }
        if (errno == EINVAL)
          continue;
      }
      if(WDS_DEBUG) {
        if(ntotal > 0) {
          printf("\n The ACL data obtained before processing \n");
          for(i=0; i< ntotal; i++)
            printf("%02X", bufPtr[i]);
        }
      }

      if(ntotal > 0) {
        while(ntotal > acl_bytes_count_processed)
          {
          //find length of ACL pkt to read
          acl_para_length = get_acl_pkt_length(bufPtr[3], bufPtr[2]);
          total_acl_length = acl_para_length + ACL_PKT_HDR_LEN;
          acl_bytes_count_processed += total_acl_length;
          bytes_count=0;

          bufResult[bytes_count++] = BT_ACL_DATA_PKT_ID;
          for(i=0; i<total_acl_length; i++)
            bufResult[bytes_count++]= bufPtr[i];

          //write back to UART port
          write_bytes = write(fd_transport_UART, &bufResult, bytes_count);
          if(ntotal > acl_bytes_count_processed) {
            for(i=0; i<ntotal; i++)
              bufPtr[i] = bufPtr[i+total_acl_length];
             }
          }
          acl_bytes_count_processed = 0;
	}
      }

      //If it is an ANT DATA pkt
      if (FD_ISSET(fd_transport_smd6, &readfds)) {
          nbytes = 0;
          ntotal = 0;
          do {
                nbytes = read(fd_transport_smd6, (char *)(&(bufPtr[ntotal])), 1);
                ntotal += nbytes;
          } while ((nbytes > 0) && (ntotal < sizeof(bufPtr)));

          if (nbytes < 0) {
             if (WDS_DEBUG) {
                 printf("\n Error while reading from SMD6 interface %d\n", nbytes);
                 perror("The error is");
                 printf("errno = %d.\n", errno);
             }
             if (errno == EINVAL)
                 continue;
          }
          if (ntotal > 0) {
              data_bytes_count_processed = 0;

              while (ntotal > data_bytes_count_processed) {
                  data_para_length = bufPtr[0];
                  total_data_length = data_para_length + ANT_DATA_PKT_HDR_LEN;
                  data_bytes_count_processed += total_data_length;
                  bytes_count = 0;
                  printf("\nANT Data: ");
                  bufResult[bytes_count++] = ANT_DATA_PKT_ID;
                  for (i = 0; i<total_data_length; i++) {
                      bufResult[bytes_count++] = bufPtr[i];
                      printf("%02x ", bufPtr[i]);
                  }
                  printf("\n");

                  //write back to UART port
                  write_bytes = write(fd_transport_UART, &bufResult, bytes_count);
                  if (ntotal > data_bytes_count_processed) {
                      for (i = 0; i < ntotal; i++)
                           bufPtr[i] = bufPtr[i+total_data_length];
                  }
              }
          }
      }

      //If it is a ANT event pkt
      if (FD_ISSET(fd_transport_smd5, &readfds)) {
          nbytes = 0;
          ntotal = 0;
          do {
              nbytes = read(fd_transport_smd5, (char *)(&(bufPtr[ntotal])), 1);
              ntotal += nbytes;
          } while ((nbytes > 0) && (ntotal < sizeof(bufPtr)));

          if (nbytes < 0) {
              if (WDS_DEBUG) {
                  printf("Error while reading from SMD5 interface %d\n", nbytes);
                  perror("The error is");
                  printf("errno = %d.\n", errno);
              }
              if (errno == EINVAL)
                  continue;
          }

          if (ntotal > 0) {
              while (ntotal > event_bytes_count_processed) {
                  //calculate event length
                  event_para_length = bufPtr[0];
                  total_event_length = event_para_length + ANT_CMD_PKT_HDR_LEN;
                  event_bytes_count_processed += total_event_length;
                  bytes_count = 0;

                  bufResult[bytes_count++] = ANT_EVT_PKT_ID;
                  printf("ANT Event: ");
                  for (i = 0; i<total_event_length; i++) {
                      printf("%02x ", bufPtr[i]);
                      bufResult[bytes_count++] = bufPtr[i];
                  }
                  printf("\n\n");

                  //write back to UART port
                  write_bytes = write(fd_transport_UART, &bufResult, bytes_count);
                  if (ntotal > event_bytes_count_processed) {
                      for (i = 0; i < ntotal; i++) {
                          bufPtr[i] = bufPtr[i+total_event_length];
                      }
                  }
              }
              event_bytes_count_processed = 0;
          }
      }
  }
  while(1);
  printf("\nReader thread exited\n");
  return 0;
}

/*===========================================================================
FUNCTION   wds_pkt_dispatch

DESCRIPTION
  Routine to write data into SMD interfaces

DEPENDENCIES
  NIL

RETURN VALUE
  RETURN NIL

SIDE EFFECTS
  None

===========================================================================*/
void wds_pkt_dispatch(void *bufResult, int length, int pkt_type)
{
  int nbytes;
  //BT Cmd pkt
  if(pkt_type == BT_CMD_PKT)
    nbytes = write(fd_transport_smd3, bufResult, length);

  //FM Cmd pkt
  else if(pkt_type == FM_CMD_PKT)
    nbytes = write(fd_transport_smd1, bufResult, length);

  //ACL pkt
  else if(pkt_type == BT_ACL_PKT)
    nbytes = write(fd_transport_smd2, bufResult, length);

  //ANT CMD pkt
  else if (pkt_type == ANT_CMD_PKT)
    nbytes = write(fd_transport_smd5, bufResult, length);

   //ANT DATA pkt
  else if (pkt_type == ANT_DATA_PKT)
    nbytes = write(fd_transport_smd6, bufResult, length);

}

/*===========================================================================
FUNCTION   set_port_raw_mode

DESCRIPTION
  Routine to set the port interface to raw mode

DEPENDENCIES
  NIL

RETURN VALUE
  RETURN NIL

SIDE EFFECTS
  None

===========================================================================*/
void set_port_raw_mode(int fd_transport)
{
  struct termios term_port;
  if(tcgetattr(fd_transport, &term_port) < 0)
    close(fd_transport);

  cfmakeraw(&term_port);
  if(tcsetattr(fd_transport, TCSANOW, &term_port) < 0)
    if(WDS_DEBUG)
       printf("\n Error while setting attributes\n");

  tcflush(fd_transport, TCIFLUSH);
}

/*===========================================================================
FUNCTION   wds_initiate_thread

DESCRIPTION
  Routine to initiate the reader thread to monitor the SMD interfaces

DEPENDENCIES
  NIL

RETURN VALUE
  RETURN NIL

SIDE EFFECTS
  None

===========================================================================*/
void wds_initiate_thread()
{
    if (option_selected == ANT_ONLY || option_selected == ALL_CHANNELS) {
        fd_transport_smd5 = open(APPS_RIVA_ANT_CMD, (O_RDWR | O_NONBLOCK));
        set_port_raw_mode(fd_transport_smd5);

        fd_transport_smd6 = open(APPS_RIVA_ANT_DATA, (O_RDWR | O_NONBLOCK));
        set_port_raw_mode(fd_transport_smd6);
    }

    if (option_selected == BT_ONLY || option_selected == ALL_CHANNELS) {
        fd_transport_smd3 = open(APPS_RIVA_BT_CMD_CH, (O_RDWR | O_NONBLOCK));
        set_port_raw_mode(fd_transport_smd3);

        fd_transport_smd2 = open(APPS_RIVA_BT_ACL_CH, (O_RDWR | O_NONBLOCK));
        set_port_raw_mode(fd_transport_smd2);
    }

    if (option_selected == FM_ONLY || option_selected == ALL_CHANNELS) {
        fd_transport_smd1 = open(APPS_RIVA_FM_CMD_CH, (O_RDWR | O_NONBLOCK));
        set_port_raw_mode(fd_transport_smd1);
    }

    pthread_create( &hci_cmd_thread_hdl, NULL, wds_readerthread, NULL);
}

/*===========================================================================
FUNCTION   init_uart

DESCRIPTION
  Routine to set the properties of UART port

DEPENDENCIES
  NIL

RETURN VALUE
  File descriptor of the UART port

SIDE EFFECTS
  None

===========================================================================*/
int init_uart(char *uart_tty_interface)
{
  struct termios term_uart;
  int fd_transport;
  fd_transport = open(uart_tty_interface, (O_RDWR | O_NONBLOCK));
  if (fd_transport == -1) {
    printf("\n Unable to open port :: %s \n", uart_tty_interface);
    perror("\n The error is \n");
    printf("\n errno = %d.\n", errno);
  }
  else
    printf("\n Port %s opened:: \n", uart_tty_interface);

  if (tcgetattr(fd_transport, &term_uart) < 0)
    close(fd_transport);

  cfmakeraw(&term_uart);

  if ((term_uart.c_lflag & ECHO) == ECHO)
    if (WDS_DEBUG)
      printf("\nEcho set, need to disable \n");

  term_uart.c_lflag = term_uart.c_lflag & ((tcflag_t)(~ECHO));
  cfsetospeed(&term_uart, B115200);
  cfsetispeed(&term_uart, B115200);
  if (tcsetattr(fd_transport, TCSANOW, &term_uart) < 0)
    if(WDS_DEBUG)
      printf("\n Error while setting attributes\n");

  tcflush(fd_transport, TCIFLUSH);

  if ((term_uart.c_lflag & ECHO) == ECHO)
    if (WDS_DEBUG)
      printf("\n Repeat .... Echo set, need to disable \n");

  return fd_transport;
}
