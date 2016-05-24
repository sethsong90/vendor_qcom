/*===========================================================================

                         Q C M A P_ S T A _ I N T E R F A C E . C

DESCRIPTION

  The Data Services Qualcomm Mobile Access Point STA interface source file.

EXTERNALIZED FUNCTIONS

  Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
===========================================================================*/
/*===========================================================================

when        who    what, where, why
--------    ---    ---------------------------------------------------------- 
12/06/12    cp     Created module
===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>

#include "comdef.h"
#include "customer.h"
#include "amssassert.h"
#include "qcmap_cm_api.h"
#include "ds_Utils_DebugMsg.h"

/* Socket used for STA events posting. */
unsigned int sta_qcmap_sockfd;

#define NUM_ARGS_EXPECTED 3

#define STA_ASSOCIATED "CONNECTED"
#define STA_DISASSOCIATED "DISCONNECTED"

int create_sta_socket(unsigned int *sockfd)
{

  if ((*sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == QCMAP_CM_ERROR)
  {
    LOG_MSG_ERROR("Error creating socket, errno: %d", errno, 0, 0);
    return QCMAP_CM_ERROR;
  }

  if(fcntl(*sockfd, F_SETFD, FD_CLOEXEC) < 0)
  {
    LOG_MSG_ERROR("Couldn't set Close on Exec, errno: %d", errno, 0, 0);
  }

  return QCMAP_CM_ENOERROR;
}

int main(int argc, char **argv)
{
  qcmap_sta_buffer_t qcmap_sta_buffer;
  int numBytes=0, len;
  struct sockaddr_un sta_qcmap;

  if ( argc != NUM_ARGS_EXPECTED )
  {
    LOG_MSG_ERROR("QCMAP STA Interface called with less number of arguements",
                   0, 0, 0);
    exit(1);
  }

  LOG_MSG_INFO1("Got %s event on STA interface %s", argv[2], argv[1], 0);

  if ( strncmp(argv[2], STA_ASSOCIATED, strlen(STA_ASSOCIATED)) == 0 )
  {
    qcmap_sta_buffer.event = STA_CONNECTED;
  }
  else if ( strncmp(argv[2], STA_DISASSOCIATED, strlen(STA_DISASSOCIATED)) == 0 )
  {
    qcmap_sta_buffer.event = STA_DISCONNECTED;
  }
  else
  {
    LOG_MSG_INFO1("QCMAP STA Interface: unsupported event %s",
                   argv[2], 0, 0);
    exit(1);
  }

  if ( create_sta_socket(&sta_qcmap_sockfd) != QCMAP_CM_ENOERROR )
  {
    LOG_MSG_ERROR("QCMAP STA Interface: failed to create client sta sockfd.",
                   0, 0, 0);
    exit(1);
  }

  /* Send the event to DS QCMAP server socket. */
  sta_qcmap.sun_family = AF_UNIX;
  strcpy(sta_qcmap.sun_path, QCMAP_STA_UDS_FILE);
  len = strlen(sta_qcmap.sun_path) + sizeof(sta_qcmap.sun_family);

  /* update the STA cookie. */
  qcmap_sta_buffer.sta_cookie = 0xDCDCDCDC;

  if ((numBytes = sendto(sta_qcmap_sockfd, (void *)&qcmap_sta_buffer, sizeof(qcmap_sta_buffer), 0,
             (struct sockaddr *)&sta_qcmap, len)) == -1)
  {
    LOG_MSG_ERROR("Send Failed from sta interface context", 0, 0, 0);
    close(sta_qcmap_sockfd);
    exit(1);
  }

  close(sta_qcmap_sockfd);

  return 0;

}

