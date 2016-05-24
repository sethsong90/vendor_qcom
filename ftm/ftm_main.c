/*==========================================================================

                     FTM Main Task Source File

Description
  Unit test component file for regsitering the routines to Diag library
  for BT and FTM commands

# Copyright (c) 2010 by Qualcomm Technologies, Inc.  All Rights Reserved.
# Qualcomm Technologies Proprietary and Confidential.

===========================================================================*/

/*===========================================================================

                         Edit History


when       who     what, where, why
--------   ---     ----------------------------------------------------------
06/18/10   rakeshk  Created a source file to implement routines for
                    registering the callback routines for FM and BT FTM
                    packets
07/06/10   rakeshk  changed the name of FM common header file in inclusion
07/07/10   rakeshk  Removed the sleep and wake in the main thread loop
===========================================================================*/
#include "event.h"
#include "msg.h"
#include "log.h"

#include "diag_lsm.h"
#include "stdio.h"
#include <unistd.h>
#include <pthread.h>


#include "diagpkt.h"
#include "diagcmd.h"
#include "diag.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "ftm_bt.h"
#ifdef ATH_WLAN
#include "ftm_wlan.h"
#endif
#include "ftm_fm_common.h"
#include "ftm_common.h"
#include "ftm_ant_common.h"
#include <sys/time.h>
#include <getopt.h>

#include <dirent.h>
#include <pwd.h>
#include <cutils/sched_policy.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "ftm_nfc.h"


#define FTM_DEBUG
int boardtype = 8660;
int first_ant_command;

#define SHOW_PRIO 1
#define SHOW_TIME 2
#define SHOW_POLICY 4
#define SHOW_CPU  8
#define SHOW_MACLABEL 16


/* Semaphore to monitor the completion of
* the queued command before sending down the
* next HCI cmd
*/
sem_t semaphore_cmd_complete;
/* Semaphore to monitor whether a command
* is queued before proceeding to dequeue
* the HCI packet
*/
sem_t semaphore_cmd_queued;
/* Callback declaration for BT FTM packet processing */
PACK(void *) bt_ftm_diag_dispatch (PACK(void *)req_pkt,
  uint16 pkt_len);
/* Callback declaration for BT FTM packet processing */
PACK(void *) fm_ftm_diag_dispatch (PACK(void *)req_pkt,
  uint16 pkt_len);
/* Callback declaration for ANT FTM packet processing */
PACK(void *) ant_ftm_diag_dispatch (PACK(void *)req_pkt,
  uint16 pkt_len);
#ifdef ATH_WLAN
/* Callback declaration for WLAN FTM packet processing */
PACK(void *) wlan_ftm_diag_dispatch (PACK(void *)req_pkt,
  uint16 pkt_len);

/* Diag pkt table for WLAN */
static const diagpkt_user_table_entry_type wlan_ftm_diag_func_table[] =
{
  {FTM_WLAN_CMD_CODE, FTM_WLAN_CMD_CODE, wlan_ftm_diag_dispatch}
};
#endif

#ifdef NFC_QCA1990
/* Callback declaration for NFC FTM packet processing */
PACK(void *) nfc_ftm_diag_dispatch (PACK(void *)req_pkt,
  uint16 pkt_len);
#endif


/* Diag pkt table for BT */
static const diagpkt_user_table_entry_type bt_ftm_diag_func_table[] =
{
  {FTM_BT_CMD_CODE, FTM_BT_CMD_CODE, bt_ftm_diag_dispatch},
};
/* Diag pkt table for FM */
static const diagpkt_user_table_entry_type fm_ftm_diag_func_table[] =
{
  {FTM_FM_CMD_CODE,FTM_FM_CMD_CODE, fm_ftm_diag_dispatch},
};
/*Diag pkt table for ANT */
static const diagpkt_user_table_entry_type ant_ftm_diag_func_table[] =
{
  {FTM_ANT_CMD_CODE, FTM_ANT_CMD_CODE, ant_ftm_diag_dispatch}
};

#ifdef NFC_QCA1990
/*Diag pkt table for NFC */
static const diagpkt_user_table_entry_type nfc_ftm_diag_func_table[] =
{
  {FTM_NFC_CMD_CODE, FTM_NFC_CMD_CODE, nfc_ftm_diag_dispatch}
};
#endif

/*=========================================================================
FUNCTION   fm_ftm_diag_dispatch

DESCRIPTION
  Processes the request packet and sends it to the FTM FM layer for further
  processing

DEPENDENCIES
  NIL

RETURN VALUE
  pointer to FTM FM Response packet

SIDE EFFECTS
  None

===========================================================================*/
PACK(void *) fm_ftm_diag_dispatch
(
  PACK(void *)req_pkt,
  uint16 pkt_len
)
{
 PACK(void *)rsp = NULL;
#ifdef FTM_DEBUG
 printf("FM I2C Send Response = %d\n",pkt_len);
#endif
 // Allocate the same length as the request.
 rsp = ftm_fm_dispatch(req_pkt,pkt_len);
 return rsp;
}

/*===========================================================================
FUNCTION   ant_ftm_diag_dispatch

DESCRIPTION
  Processes the request packet and sends it to the FTM ANT layer for further
  processing
DEPENDENCIES
 NIL

RETURN VALUE
  pointer to FTM ANT  Response packet

SIDE EFFECTS
  None

===========================================================================*/
PACK(void *) ant_ftm_diag_dispatch
(
  PACK(void *)req_pkt,
  uint16 pkt_len
)
{
 PACK(void *)rsp = NULL;
 #ifdef FTM_DEBUG
 printf("ANT diag dispatch send response = %d\n", pkt_len);
 #endif
// Allocate the same length as the request.
 rsp = ftm_ant_dispatch(req_pkt,pkt_len);
 return rsp;
}

/*===========================================================================
FUNCTION   bt_ftm_diag_dispatch

DESCRIPTION
  Processes the request packet and sends it to the FTM BT layer for further
  processing

DEPENDENCIES
  NIL

RETURN VALUE
  pointer to FTM BT Response packet

SIDE EFFECTS
  None

===========================================================================*/
PACK(void *) bt_ftm_diag_dispatch
(
  PACK(void *)req_pkt,
  uint16 pkt_len
)
{
  PACK(void *)rsp = NULL;
  boolean status = TRUE;
#ifdef FTM_DEBUG
  printf("Send Response = %d\n",pkt_len);
#endif
  // Allocate the same length as the request.
  rsp = diagpkt_subsys_alloc (DIAG_SUBSYS_FTM, FTM_BT_CMD_CODE, pkt_len);

  if (rsp != NULL)
  {
    memcpy ((void *) rsp, (void *) req_pkt, pkt_len);
  }
  /* Spurious incoming request packets are occasionally received
   * by DIAG_SUBSYS_FTM which needs to be ignored and accordingly responded.
   * TODO: Reason for these spurious incoming request packets is yet to be
   *       found, though its always found to be corresponding to this majic
   *       length of 65532.
   */
  if (pkt_len == 65532)
  {
    printf("\nIgnore spurious DIAG packet processing & respond immediately");
  }
  else
  {
#ifdef FTM_DEBUG
    printf("Insert BT packet = %d\n",pkt_len);
#endif
    /* add the BT packet into the Cmd Queue
     * and notify the main thread its queued
     */
    status = qinsert_cmd((ftm_bt_pkt_type *)req_pkt);
    if(status == TRUE)
      sem_post(&semaphore_cmd_queued);
#ifdef FTM_DEBUG
    printf("Insert BT packet done\n");
#endif
  }
  return (rsp);
}

#ifdef ATH_WLAN
/*===========================================================================
FUNCTION   wlan_ftm_diag_dispatch

DESCRIPTION
  Processes the request packet and sends it to the FTM WLAN layer for further
  processing

DEPENDENCIES
  NIL

RETURN VALUE
  pointer to FTM WLAN Response packet

SIDE EFFECTS
  None

===========================================================================*/

PACK(void *) wlan_ftm_diag_dispatch
(
  PACK(void *)req_pkt,
  uint16 pkt_len
)
{
    PACK(void *)rsp = NULL;
#ifdef FTM_DEBUG
    printf("WLAN Send Response = %d\n", pkt_len);
#endif

    rsp = ftm_wlan_dispatch(req_pkt, pkt_len);

    return rsp;
}
#endif

#ifdef NFC_QCA1990
/*===========================================================================
FUNCTION   nfcs_ftm_diag_dispatch

DESCRIPTION
  Processes the request packet and sends it to the FTM NFC layer for further
  processing
DEPENDENCIES
 NIL

RETURN VALUE


SIDE EFFECTS


===========================================================================*/
PACK(void *) nfc_ftm_diag_dispatch
(
  PACK(void *)req_pkt,
  uint16 pkt_len
)
{
    PACK(void *)rsp = NULL;
    boolean status = TRUE;
#ifdef FTM_DEBUG
    printf(" NFC Send Response = %d\n",pkt_len);
#endif

    /*now send the incoming nfc diag command packet to the nfc ftm layer to
      get it processed*/
    rsp = ftm_nfc_dispatch(req_pkt, pkt_len);

    /* send same response as recieved back*/
    return rsp;
}
#endif
/*===========================================================================
FUNCTION   main

DESCRIPTION
  Initialises the Diag library and registers the PKT table for FM and BT
  and daemonises

DEPENDENCIES
  NIL

RETURN VALUE
  NIL, Error in the event buffer will mean a NULL App version and Zero HW
  version

SIDE EFFECTS
  None

===========================================================================*/

int main(int argc, char *argv[])
{
  boolean bInit_Success = FALSE;
  struct timespec ts;
  int sem_status;
  int index;
  char c;

  static struct option options[] =
  {
    {"board-type", required_argument, NULL, 'b'},
  };

  c = getopt_long (argc, argv, ":b",
        options, &index);

  if(c == 'b')
  {
    boardtype = atoi(optarg);
  }

#ifdef FTM_DEBUG
  printf("\nFTM Daemon calling LSM init \n");
#endif
  bInit_Success = Diag_LSM_Init(NULL);

  if(!bInit_Success)
  {
    printf("FTMDaemon: Diag_LSM_Init() failed.");
    return -1;
  }

  printf("FTMDaemon: Diag_LSM_Init succeeded. \n");

  DIAGPKT_DISPATCH_TABLE_REGISTER ( DIAG_SUBSYS_FTM ,
                                      bt_ftm_diag_func_table
                                    );
  DIAGPKT_DISPATCH_TABLE_REGISTER ( DIAG_SUBSYS_FTM ,
                                      fm_ftm_diag_func_table
                                    );
#ifdef ATH_WLAN
  DIAGPKT_DISPATCH_TABLE_REGISTER ( DIAG_SUBSYS_FTM,
                                      wlan_ftm_diag_func_table
                                    );
#endif
 DIAGPKT_DISPATCH_TABLE_REGISTER ( DIAG_SUBSYS_FTM ,
                                     ant_ftm_diag_func_table
                                    );

#ifdef NFC_QCA1990
  DIAGPKT_DISPATCH_TABLE_REGISTER ( DIAG_SUBSYS_FTM ,
                                      nfc_ftm_diag_func_table
                                    );
#endif

  sem_init(&semaphore_cmd_complete,0, 1);
  sem_init(&semaphore_cmd_queued,0,0);
  first_ant_command = 0;
#ifdef FTM_DEBUG
 printf("Initialised the BT FTM cmd queue handlers \n");
#endif

  do
  {
    /* We have the freedom to send the first request without wating
     * for a command complete
     */
#ifdef FTM_DEBUG
     printf("Wait on cmd complete from the previous command\n");
#endif
     if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
       printf("get clock_gettime error");
     ts.tv_sec += 5;
     /*we wait for 5 secs for a command already queued for
     * transmision
     */
     sem_status = sem_timedwait(&semaphore_cmd_complete,&ts);
     if(sem_status == -1)
     {
       printf("Command complete timed out\n");
       ftm_bt_err_timedout();
     }
#ifdef FTM_DEBUG
     printf("Waiting on next Cmd to be queued\n");
#endif
     sem_wait(&semaphore_cmd_queued);
     dequeue_send();
   }
   while(1);
#ifdef FTM_DEBUG
  printf("\nFTMDaemon Deinit the LSM\n");
#endif
  // Clean up before exiting
  Diag_LSM_DeInit();

  return 0;

}

