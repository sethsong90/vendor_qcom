/*==========================================================================

                     FTM WLAN Source File

# Copyright (c) 2011 by Qualcomm Technologies, Inc.  All Rights Reserved.
# Qualcomm Technologies Proprietary and Confidential.

===========================================================================*/

/*===========================================================================

                         Edit History


when       who       what, where, why
--------   ---       ----------------------------------------------------------
07/11/11   karthikm  Wrapper that contains routines for directing FTM commands
                     sent from host to the IOCTL calls of Atheros driver.
*/

/*
 * Copyright (c) 2006 Atheros Communications Inc.
 * All rights reserved.
 *
 *
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
// </copyright>
//
// <summary>
// 	FTM_WLAN_TCMD
//  Based on athtestcmd.c from AR6003 drop
// </summary>
//
 *
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/if.h>
#include <linux/wireless.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "event.h"
#include "msg.h"
#include "log.h"

#include "diag_lsm.h"
#include "stdio.h"
#include <unistd.h>

#include "diagpkt.h"
#include "diagcmd.h"
#include "diag.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#include "ftm_wlan.h"
#include "testcmd.h"
#include "libtcmd.h"

#define FTM_DEBUG

#ifdef FTM_DEBUG
#define LOG_INFO(X...) printf(X)
#else
#define LOG_INFO(X...) do { } while (0)
#endif

#define INVALID_FREQ    0
#define A_RATE_NUM      28
#define G_RATE_NUM      28
#define RATE_STR_LEN    20

static void rxReport(void *buf);
bool ifs_init[32];

void print_uchar_array(uint8_t *addr, int len)
{
   int i;
   for (i = 0;i< len; i++)
      LOG_INFO("%02X ", addr[i]);
   LOG_INFO("\n");
}

void print_uint16_array(uint16_t *addr, int len)
{
   int i;
   for (i = 0;i< len; i++)
      LOG_INFO("%02X %02X ", addr[i]>>8, addr[i]&0xFF);
   LOG_INFO("\n");
}

/*===========================================================================
FUNCTION   rxReport

DESCRIPTION
  Quick debug routine that will print all the receive statistics

DEPENDENCIES
  NIL

RETURN VALUE
  NIL

SIDE EFFECTS
  NONE

===========================================================================*/
static void rxReport(void *buf)
{
    uint32 pkt;
    int  rssi;
    uint32 crcError;
    uint32 secErr;
    uint16 rateCnt[TCMD_MAX_RATES];
    uint16 rateCntShortGuard[TCMD_MAX_RATES];

    pkt = *(uint32 *)buf;
    rssi = (int)(*((uint32 *)buf + 1));
    crcError = *((uint32 *)buf + 2);
    secErr = *((uint32 *)buf + 3);

    LOG_INFO("total pkt %lu ; crcError pkt %lu ; secErr pkt %lu ;  average rssi %d\n", pkt, crcError, secErr,
          (int)( pkt ? (rssi / (int)pkt) : 0));


    memcpy(rateCnt, ((unsigned char *)buf)+(4*sizeof(uint32)), sizeof(rateCnt));
    memcpy(rateCntShortGuard, ((unsigned char *)buf)+(4*sizeof(uint32))+(TCMD_MAX_RATES * sizeof(uint16)), sizeof(rateCntShortGuard));

    LOG_INFO("1Mbps     %d\n", rateCnt[0]);
    LOG_INFO("2Mbps     %d\n", rateCnt[1]);
    LOG_INFO("5.5Mbps   %d\n", rateCnt[2]);
    LOG_INFO("11Mbps    %d\n", rateCnt[3]);
    LOG_INFO("6Mbps     %d\n", rateCnt[4]);
    LOG_INFO("9Mbps     %d\n", rateCnt[5]);
    LOG_INFO("12Mbps    %d\n", rateCnt[6]);
    LOG_INFO("18Mbps    %d\n", rateCnt[7]);
    LOG_INFO("24Mbps    %d\n", rateCnt[8]);
    LOG_INFO("36Mbps    %d\n", rateCnt[9]);
    LOG_INFO("48Mbps    %d\n", rateCnt[10]);
    LOG_INFO("54Mbps    %d\n", rateCnt[11]);
    LOG_INFO("\n");
    LOG_INFO("HT20 MCS0 6.5Mbps   %d (SGI: %d)\n", rateCnt[12], rateCntShortGuard[12]);
    LOG_INFO("HT20 MCS1 13Mbps    %d (SGI: %d)\n", rateCnt[13], rateCntShortGuard[13]);
    LOG_INFO("HT20 MCS2 19.5Mbps  %d (SGI: %d)\n", rateCnt[14], rateCntShortGuard[14]);
    LOG_INFO("HT20 MCS3 26Mbps    %d (SGI: %d)\n", rateCnt[15], rateCntShortGuard[15]);
    LOG_INFO("HT20 MCS4 39Mbps    %d (SGI: %d)\n", rateCnt[16], rateCntShortGuard[16]);
    LOG_INFO("HT20 MCS5 52Mbps    %d (SGI: %d)\n", rateCnt[17], rateCntShortGuard[17]);
    LOG_INFO("HT20 MCS6 58.5Mbps  %d (SGI: %d)\n", rateCnt[18], rateCntShortGuard[18]);
    LOG_INFO("HT20 MCS7 65Mbps    %d (SGI: %d)\n", rateCnt[19], rateCntShortGuard[19]);
    LOG_INFO("\n");
    LOG_INFO("HT40 MCS0 13.5Mbps    %d (SGI: %d)\n", rateCnt[20], rateCntShortGuard[20]);
    LOG_INFO("HT40 MCS1 27.0Mbps    %d (SGI: %d)\n", rateCnt[21], rateCntShortGuard[21]);
    LOG_INFO("HT40 MCS2 40.5Mbps    %d (SGI: %d)\n", rateCnt[22], rateCntShortGuard[22]);
    LOG_INFO("HT40 MCS3 54Mbps      %d (SGI: %d)\n", rateCnt[23], rateCntShortGuard[23]);
    LOG_INFO("HT40 MCS4 81Mbps      %d (SGI: %d)\n", rateCnt[24], rateCntShortGuard[24]);
    LOG_INFO("HT40 MCS5 108Mbps     %d (SGI: %d)\n", rateCnt[25], rateCntShortGuard[25]);
    LOG_INFO("HT40 MCS6 121.5Mbps   %d (SGI: %d)\n", rateCnt[26], rateCntShortGuard[26]);
    LOG_INFO("HT40 MCS7 135Mbps     %d (SGI: %d)\n", rateCnt[27], rateCntShortGuard[27]);
}

ftm_wlan_rx_rsp_type *g_rsp = NULL;
TCMD_ID tcmd = TCMD_CONT_RX_ID;
uint32 mode = 0;

/*===========================================================================
FUNCTION   ftm_wlan_tcmd_rx

DESCRIPTION
   Call back handler

DEPENDENCIES
  NIL

RETURN VALUE
  NONE

SIDE EFFECTS
  NONE

===========================================================================*/

void ftm_wlan_tcmd_rx(void *buf, int len)
{
   // Build the response to be sent
   LOG_INFO("Rx call back received with len %d\n",len);

   if ( tcmd == TCMD_CONT_RX_ID )
   {
      struct TCMD_CONT_RX_REPORT *report = &((TCMD_CONT_RX *) buf)->u.report;

      g_rsp = (ftm_wlan_rx_rsp_type*)diagpkt_subsys_alloc( DIAG_SUBSYS_FTM
                              , FTM_WLAN_CMD_CODE
                              , (sizeof(ftm_wlan_gen_rsp_type) + sizeof(struct TCMD_CONT_RX_REPORT)) );

      memcpy(g_rsp->data, report, sizeof(struct TCMD_CONT_RX_REPORT));

      if (mode == TCMD_CONT_RX_REPORT) {
          rxReport((void *)report);
      } else if (mode == TCMD_CONT_RX_GETMAC) {
          TC_CMDS *tCmd = (uint8_t *)buf;

          LOG_INFO("length %d version %d act %d\n", tCmd->hdr.u.parm.length, 
                   tCmd->hdr.u.parm.version, tCmd->hdr.act);

          LOG_INFO("MAC address : %02x:%02x:%02x:%02x:%02x:%02x\n", 
                   tCmd->buf[0], tCmd->buf[1], tCmd->buf[2], 
                   tCmd->buf[3], tCmd->buf[4], tCmd->buf[5]);
      }
   }
   else if ( tcmd == TC_CMDS_ID )
   {
      TC_CMDS *tCmd = (uint8_t*)buf;

      LOG_INFO("length %d version %d act %d\n",tCmd->hdr.u.parm.length,tCmd->hdr.u.parm.version,tCmd->hdr.act);

      g_rsp = (ftm_wlan_rx_rsp_type*)diagpkt_subsys_alloc( DIAG_SUBSYS_FTM
					      ,FTM_WLAN_CMD_CODE
                      , (sizeof(ftm_wlan_gen_rsp_type) + sizeof(TC_CMDS)));

      memcpy(g_rsp->data, (void *)tCmd, sizeof(TC_CMDS));

      if ( mode == TC_CMDS_READTHERMAL ) {
           LOG_INFO("Chip Thermal value:%d\n", tCmd->buf[0]);
      }
   }
   else if ( tcmd == TC_CMD_TLV_ID )
   {
      g_rsp = (ftm_wlan_rx_rsp_type*)diagpkt_subsys_alloc( DIAG_SUBSYS_FTM
                              , FTM_WLAN_CMD_CODE
                              , sizeof(ftm_wlan_gen_rsp_type) + len);

      print_uchar_array((uint8_t*)buf, len);
      memcpy(g_rsp->data, buf, len);

      LOG_INFO("tcmd_rx TC_CMD_TLV_ID length %d\n", len);
   }
   else
   {
      g_rsp = (ftm_wlan_rx_rsp_type*)diagpkt_subsys_alloc( DIAG_SUBSYS_FTM
                              , FTM_WLAN_CMD_CODE
                              , sizeof(ftm_wlan_gen_rsp_type));

      LOG_INFO("Unknown TCMD response\n");
   }

   g_rsp->result = FTM_ERR_CODE_PASS;
}

/*===========================================================================
FUNCTION   isResponseNeeded

DESCRIPTION
   Do we need a response for the command

DEPENDENCIES
  NIL

RETURN VALUE
  boolean response required/not

SIDE EFFECTS
  NONE

===========================================================================*/
static bool isResponseNeeded(void *buf)
{
   bool respNeeded = false;

   tcmd = * ((uint32 *) buf);
   mode = * ((uint32 *) buf + 1);

/// Insert commands which need response
   switch (tcmd)
   {
   case TC_CMD_TLV_ID:
       respNeeded = true;
       break;
   case TCMD_CONT_RX_ID:
      switch (mode)
      {
         case TCMD_CONT_RX_REPORT:
         case TCMD_CONT_RX_GETMAC:
            respNeeded = true;
         break;
      }
      break;
   case TC_CMDS_ID:
      switch (mode)
      {
         case TC_CMDS_READTHERMAL:
         case TC_CMDS_EFUSEDUMP:
         case TC_CMDS_EFUSEWRITE:
         case TC_CMDS_OTPSTREAMWRITE:
         case TC_CMDS_OTPDUMP:
	    respNeeded = true; //TC_CMDS_EFUSEDUMP, TC_CMDS_EFUSEWRITE, TC_CMDS_OTPSTREAMWRITE, TC_CMDS_OTPDUMP, TC_CMDS_READTHERMAL
         break;
      }
      break;
   default:
      break;
   }

   if (respNeeded)
   {
      LOG_INFO("cmdID %d response needed\n", tcmd);
   }
   else
   {
      LOG_INFO("cmdID %d response not needed\n", tcmd);
   }

   return respNeeded;
}

/*===========================================================================
FUNCTION   ftm_wlan_dispatch

DESCRIPTION
  WLAN FTM dispatch routine. Main entry point routine for WLAN FTM for
  AR6003

DEPENDENCIES
  NIL

RETURN VALUE
  Returns back buffer that is meant to be passed to the diag callback

SIDE EFFECTS
  NONE

===========================================================================*/
void* ftm_wlan_dispatch(ftm_wlan_pkt_type *wlan_ftm_pkt, int pkt_len)
{
   unsigned int cmd = *((uint32*)wlan_ftm_pkt->data);
   ftm_wlan_gen_rsp_type *rsp;
   int data_len = pkt_len - sizeof(diagpkt_subsys_header_v2_type) - 4;
   char ifname[IFNAMSIZ];
   char devicenumstr[2];
   bool resp = false;

   memset(ifname, '\0', 8);
   strcpy(ifname, "wlan");
   sprintf(devicenumstr, "%d", wlan_ftm_pkt->wlandeviceno);
   strcat(ifname, devicenumstr);

   LOG_INFO("Command ID rec'd: 0x%X length %d\n", cmd,data_len);

   print_uchar_array((uint8_t*)(wlan_ftm_pkt->data), data_len);

   g_rsp = NULL;

   rsp = (ftm_wlan_gen_rsp_type*)diagpkt_subsys_alloc( DIAG_SUBSYS_FTM
			      , FTM_WLAN_CMD_CODE
			      , sizeof(ftm_wlan_gen_rsp_type)
			      );

   rsp->result = FTM_ERR_CODE_PASS;

   if (ifs_init[wlan_ftm_pkt->wlandeviceno])
   {
          /* already initialized this interface */
   }
   else
   {
       if (tcmd_tx_init(ifname, ftm_wlan_tcmd_rx))
       {
           LOG_INFO("Couldn't init tcmd transport!\n");
           rsp->result = FTM_ERR_CODE_IOCTL_FAIL;
           goto err_out;
       }

       LOG_INFO("tcmd: initialized interface: %s\n", ifname);
       ifs_init[wlan_ftm_pkt->wlandeviceno] = true;
   }

   resp = isResponseNeeded( (void*)wlan_ftm_pkt->data);

   if (tcmd_tx(wlan_ftm_pkt->data, data_len, resp))
   {
        LOG_INFO("TCMD timed out!\n");
        rsp->result = FTM_ERR_CODE_IOCTL_FAIL;
        goto err_out;
   }

   if (resp)
   {
       if (g_rsp)
       {
          diagpkt_free(rsp);
          rsp = g_rsp;
       }
       else
       {
           LOG_INFO("No response got probably timing out.... \n");
           rsp->result = FTM_ERR_CODE_IOCTL_FAIL;
       }
  }

err_out:
    return (void *) rsp;
}
