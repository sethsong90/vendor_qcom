/*
 * Copyright (c) 2014 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 *
 * Not a Contribution.
 * Apache license notifications and license are retained
 * for attribution purposes only.
 */

/******************************************************************************
* Copyright (c) 2013, The Linux Foundation. All rights reserved.
* Not a Contribution.
 ******************************************************************************/
/******************************************************************************
 *
 *  Copyright (C) 2010-2013 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/*****************************************************************************
** Debug macro. Uncomment when we get past testing phase.
*****************************************************************************/

/*
 * We use NDEBUG to hide some ramdump debug traces and asserts. We use RAMDUMP to output some additional information.
 * Note: ramdump is not a production level feature.
 */
#ifdef NDEBUG
#undef NDEBUG
#endif

#ifndef RAMDUMP
#define RAMDUMP
#endif

#ifndef RAMDUMP_TEST_POKE
//#define RAMDUMP_TEST_POKE
#endif

/******************************************************************************
 *
 *  This file contains function of the NFC unit to receive/process NCI/VS
 *  commands/responses.
 *
 ******************************************************************************/
#include <string.h>
#include <assert.h>
//#include <stringl/stringl.h>
#include <time.h>       /* time_t, struct tm, difftime, time, mktime */
#include <stdlib.h>
#include "nfc_hal_int.h"
#include "nfc_hal_post_reset.h"
#include "userial.h"
#include "nci_defs.h"
#include "config.h"

#include <DT_Nfc_link.h>
#include <DT_Nfc_types.h>
#include <DT_Nfc_status.h>
#include <DT_Nfc_i2c.h>
#include <DT_Nfc_log.h>
#include <DT_Nfc.h>
#ifdef DTA // <DTA>
#include "dta_flag.h"
int dta_flag_all = 0;
#define LISTEN_MASK 8
#endif // </DTA>
/*****************************************************************************
** Constants and types
*****************************************************************************/
extern char current_mode;
extern sem_t semaphore_sleepcmd_complete;
extern UINT8 wait_reset_rsp;
extern UINT8 reset_status;
extern BOOLEAN is_vsc_prop_sleep_cmd;
/*
 * Identifies RAMDUMP NTF and RSPs' received from NFCC
 */
typedef enum {
    RamDump_Recv_Init_Ntf = 0,
    RamDump_Recv_Init_Rsp,
    RamDump_Recv_Get_Rsp,
    RamDump_Recv_End_Rsp,
    RamDump_Item_Count /* must remain last item in enum list */
} RamDumpState;

/*
 * Ramdump config file name and location
 */
static const char kRamdump_config_filename[] = "/etc/nfc/hardfault.cfg";

/*
 * Ramdump dump file name and location
 */
static const char kRamdump_ramdump_filename[] = "/data/nfc/ramdump_%H_%M__%d_%m_%Y.cfg";

/*
 * Number of NFCC registers to obtain information on. Depends on chip version.
 * Default is version 2.1 with 20 registers.
 */
static UINT16 gMaxRegisterCount = 20;
static UINT16 gNfccChipVersion = 21;
extern uint8_t DT_Nfc_RamdumpPerformed;
BOOLEAN init_ramdump_completed = FALSE;

#define REGISTER_FIELD_LENGTH 4
#define MAX_CONFIG_NAME 256
#define MAX_RECV_PACKET_SIZE 255
/*
 * Link list node of Dump Data blocks.
 */
typedef struct ConfigData_
{
    struct ConfigData_* next;
    /* obtained from config file */
    unsigned int dump_start_addr;
    int dump_length;
    char name[MAX_CONFIG_NAME];
} ConfigData;

/*
 * The Configuration params for a RAMDUMP
 * Ensure (int) ordering so no unnecessary padding bytes
 */
typedef struct
{
    BOOLEAN ramdump_in_progress; /* signifies that this structure is in use */
    FILE* output_file_ptr;
    UINT8* data;
    int data_length;
    int bytes_requested;
    int total_received;
    UINT8* config_buffer;
    UINT8* register_buffer;
    /* no ownership of payload data */
    const UINT8* msg_payload_ptr;
    UINT8 msg_payload_length;
    ConfigData* cd_list;
    ConfigData* cd_current_item;
} RamDump_Data;

static RamDump_Data gRamdump_Data = {0};

/*****************************************************************************
** Local function prototypes
*****************************************************************************/

/* Prototype for processing RamDump events */
#ifndef NDEBUG
void nfc_hal_nci_handle_ramdump_ntf(int mt, int op_code, int pbf, int payload_len, int reason);
static void nfc_hal_nci_handle_ramdump_rsp(int mt, int op_code, int pbf, int payload_len, UINT8 * const payload_ptr);
#else
void nfc_hal_nci_handle_ramdump_ntf();
static void nfc_hal_nci_handle_ramdump_rsp(int op_code, int payload_len, UINT8 * const payload_ptr);
#endif

static void nfc_hal_nci_recv_ramdump_event(RamDumpState state);
static void nfc_hal_nci_recv_init_ramdump_ntf();
static void nfc_hal_nci_recv_init_ramdump_rsp();
static void nfc_hal_nci_recv_get_ramdump_rsp();
static void nfc_hal_nci_recv_end_ramdump_rsp();

/* File scope prototype for tidying after RamDump sequence */
static void nfc_hal_nci_close_ramdump();

/* File scope prototypes for manipulating the data */
static BOOLEAN nfc_hal_nci_parse_ramdump_config_create_dump_area(int buf_len);
static void nfc_hal_nci_invoke_ramdump_upload();
static void nfc_hal_nci_parse_and_dump_ramdump_register_data();
static void nfc_hal_nci_parse_and_dump_ramdump_data();

/*******************************************************************************
**
** Function         nfc_hal_nci_assemble_nci_msg
**
** Description      This function is called to reassemble the received NCI
**                  response/notification packet, if required.
**                  (The data packets are posted to NFC task for reassembly)
**
** Returns          void.
**
*******************************************************************************/
void nfc_hal_nci_assemble_nci_msg (void)
{
    NFC_HDR *p_msg = nfc_hal_cb.ncit_cb.p_rcv_msg;
    UINT8 u8;
    UINT8 *p, *pp;
    UINT8 hdr[2];
    UINT8   *ps, *pd;
    UINT16  size, needed;
    BOOLEAN disp_again = FALSE;

    if ((p_msg == NULL) || (p_msg->len < NCI_MSG_HDR_SIZE))
        return;

#ifdef DISP_NCI
    DISP_NCI ((UINT8 *) (p_msg + 1) + p_msg->offset, (UINT16) (p_msg->len), TRUE);
#endif

    p       = (UINT8 *) (p_msg + 1) + p_msg->offset;
    u8      = *p++;
    /* remove the PBF bit for potential reassembly later */
    hdr[0]  = u8 & ~NCI_PBF_MASK;
    if ((u8 & NCI_MT_MASK) == NCI_MT_DATA)
    {
        /* clear the RFU in octet1 */
        *(p) = 0;
        /* data packet reassembly is performed in NFC task */
        return;
    }
    else
    {
        *(p) &= NCI_OID_MASK;
    }

    hdr[1]  = *p;
    pp = hdr;
    /* save octet0 and octet1 of an NCI header in layer_specific for the received packet */
    STREAM_TO_UINT16 (p_msg->layer_specific, pp);

    if (nfc_hal_cb.ncit_cb.p_frag_msg)
    {
        if (nfc_hal_cb.ncit_cb.p_frag_msg->layer_specific != p_msg->layer_specific)
        {
            /* check if these fragments are of the same NCI message */
            HAL_TRACE_ERROR2 ("nfc_hal_nci_assemble_nci_msg() - different messages 0x%x, 0x%x!!", nfc_hal_cb.ncit_cb.p_frag_msg->layer_specific, p_msg->layer_specific);
            nfc_hal_cb.ncit_cb.nci_ras  |= NFC_HAL_NCI_RAS_ERROR;
        }
        else if (nfc_hal_cb.ncit_cb.nci_ras == 0)
        {
            disp_again = TRUE;
            /* if not previous reassembly error, append the new fragment */
            p_msg->offset   += NCI_MSG_HDR_SIZE;
            p_msg->len      -= NCI_MSG_HDR_SIZE;
            size    = GKI_get_buf_size (nfc_hal_cb.ncit_cb.p_frag_msg);
            needed  = (NFC_HDR_SIZE + nfc_hal_cb.ncit_cb.p_frag_msg->len + nfc_hal_cb.ncit_cb.p_frag_msg->offset + p_msg->len);
            if (size >= needed)
            {
                /* the buffer for reassembly is big enough to append the new fragment */
                ps   = (UINT8 *) (p_msg + 1) + p_msg->offset;
                pd   = (UINT8 *) (nfc_hal_cb.ncit_cb.p_frag_msg + 1) + nfc_hal_cb.ncit_cb.p_frag_msg->offset + nfc_hal_cb.ncit_cb.p_frag_msg->len;
                memcpy (pd, ps, p_msg->len);
                nfc_hal_cb.ncit_cb.p_frag_msg->len  += p_msg->len;
                /* adjust the NCI packet length */
                pd   = (UINT8 *) (nfc_hal_cb.ncit_cb.p_frag_msg + 1) + nfc_hal_cb.ncit_cb.p_frag_msg->offset + 2;
                *pd  = (UINT8) (nfc_hal_cb.ncit_cb.p_frag_msg->len - NCI_MSG_HDR_SIZE);
            }
            else
            {
                nfc_hal_cb.ncit_cb.nci_ras  |= NFC_HAL_NCI_RAS_TOO_BIG;
                HAL_TRACE_ERROR2 ("nfc_hal_nci_assemble_nci_msg() buffer overrun (%d + %d)!!", nfc_hal_cb.ncit_cb.p_frag_msg->len, p_msg->len);
            }
        }
        /* we are done with this new fragment, free it */
        GKI_freebuf (p_msg);
    }
    else
    {
        nfc_hal_cb.ncit_cb.p_frag_msg = p_msg;
    }


    if ((u8 & NCI_PBF_MASK) == NCI_PBF_NO_OR_LAST)
    {
        /* last fragment */
        p_msg               = nfc_hal_cb.ncit_cb.p_frag_msg;
        p                   = (UINT8 *) (p_msg + 1) + p_msg->offset;
        *p                  = u8; /* this should make the PBF flag as Last Fragment */
        nfc_hal_cb.ncit_cb.p_frag_msg  = NULL;

        p_msg->layer_specific = nfc_hal_cb.ncit_cb.nci_ras;
        /* still report the data packet, if the incoming packet is too big */
        if (nfc_hal_cb.ncit_cb.nci_ras & NFC_HAL_NCI_RAS_ERROR)
        {
            /* NFCC reported NCI fragments for different NCI messages and this is the last fragment - drop it */
            HAL_TRACE_ERROR0 ("nfc_hal_nci_assemble_nci_msg() clearing NCI_RAS_ERROR");
            GKI_freebuf (p_msg);
            p_msg = NULL;
        }
#ifdef DISP_NCI
        if ((nfc_hal_cb.ncit_cb.nci_ras == 0) && (disp_again))
        {
            DISP_NCI ((UINT8 *) (p_msg + 1) + p_msg->offset, (UINT16) (p_msg->len), TRUE);
        }
#endif
        /* clear the error flags, so the next NCI packet is clean */
        nfc_hal_cb.ncit_cb.nci_ras = 0;
    }
    else
    {
        /* still reassembling */
        p_msg = NULL;
    }

    nfc_hal_cb.ncit_cb.p_rcv_msg = p_msg;
}

/*****************************************************************************
**
** Function         nfc_hal_nci_receive_nci_msg
**
** Description
**      Handle incoming data (NCI events) from the serial port.
**
**      If there is data waiting from the serial port, this funciton reads the
**      data and parses it. Once an entire NCI message has been read, it sends
**      the message the the NFC_TASK for processing
**
*****************************************************************************/
static BOOLEAN nfc_hal_nci_receive_nci_msg (tNFC_HAL_NCIT_CB *p_cb)
{
        UINT16      len = 0;
        BOOLEAN     msg_received = FALSE;

        HAL_TRACE_DEBUG1 ("nfc_hal_nci_receive_nci_msg+ 0x%08X", p_cb);
        /* Start of new message. Allocate a buffer for message */
        if ((p_cb->p_rcv_msg = (NFC_HDR *) GKI_getpoolbuf (NFC_HAL_NCI_POOL_ID)) != NULL)
        {
               /* Initialize NFC_HDR */
               p_cb->p_rcv_msg->len    = 0;
               p_cb->p_rcv_msg->event  = 0;
               p_cb->p_rcv_msg->offset = 0;
               p_cb->rcv_len = MAX_RECV_PACKET_SIZE;
               /* Read in the rest of the message */
               len = DT_Nfc_Read(USERIAL_NFC_PORT, ((UINT8 *) (p_cb->p_rcv_msg + 1) + p_cb->p_rcv_msg->offset + p_cb->p_rcv_msg->len),  p_cb->rcv_len);
               p_cb->p_rcv_msg->len    = len;
               if (len == 0){
                  GKI_freebuf(p_cb->p_rcv_msg);
                  GKI_TRACE_ERROR_0("nfc_hal_nci_receive_nci_msg: Read Length = 0 so freeing pool !!\n");
               }
        }
        else{
           p_cb->p_rcv_msg->len    = 0;
           GKI_TRACE_ERROR_0("nfc_hal_nci_receive_nci_msg: Unable to get pool buffer, ensure len = 0 \n");
        }
        /* Check if we read in entire message yet */
        if ( p_cb->p_rcv_msg->len != 0)
        {
           msg_received    = TRUE;
           p_cb->rcv_state = NFC_HAL_RCV_IDLE_ST;
        }
        return msg_received;
}
/*****************************************************************************
**
** Function         nfc_hal_nci_receive_bt_msg
**
** Description
**      Handle incoming BRCM specific data from the serial port.
**
**      If there is data waiting from the serial port, this funciton reads the
**      data and parses it. Once an entire message has been read, it returns
**      TRUE.
**
*****************************************************************************/
static BOOLEAN nfc_hal_nci_receive_bt_msg (tNFC_HAL_NCIT_CB *p_cb, UINT8 byte)
{
    UINT16  len;
    BOOLEAN msg_received = FALSE;

    switch (p_cb->rcv_state)
    {
    case NFC_HAL_RCV_BT_MSG_ST:

        /* Initialize rx parameters */
        p_cb->rcv_state = NFC_HAL_RCV_BT_HDR_ST;
        p_cb->rcv_len   = HCIE_PREAMBLE_SIZE;

        if ((p_cb->p_rcv_msg = (NFC_HDR *) GKI_getpoolbuf (NFC_HAL_NCI_POOL_ID)) != NULL)
        {
            /* Initialize NFC_HDR */
            p_cb->p_rcv_msg->len    = 0;
            p_cb->p_rcv_msg->event  = 0;
            p_cb->p_rcv_msg->offset = 0;

            *((UINT8 *) (p_cb->p_rcv_msg + 1) + p_cb->p_rcv_msg->offset + p_cb->p_rcv_msg->len++) = byte;
        }
        else
        {
            HAL_TRACE_ERROR0 ("[nfc] Unable to allocate buffer for incoming NCI message.");
        }
        p_cb->rcv_len--;
        break;

    case NFC_HAL_RCV_BT_HDR_ST:
        if (p_cb->p_rcv_msg)
        {
            *((UINT8 *) (p_cb->p_rcv_msg + 1) + p_cb->p_rcv_msg->offset + p_cb->p_rcv_msg->len++) = byte;
        }
        p_cb->rcv_len--;

        /* Check if we received entire preamble yet */
        if (p_cb->rcv_len == 0)
        {
            /* Received entire preamble. Length is in the last byte(s) of the preamble */
            p_cb->rcv_len = byte;

            /* Verify that buffer is big enough to fit message */
            if ((sizeof (NFC_HDR) + HCIE_PREAMBLE_SIZE + byte) > GKI_get_buf_size (p_cb->p_rcv_msg))
            {
                /* Message cannot fit into buffer */
                GKI_freebuf (p_cb->p_rcv_msg);
                p_cb->p_rcv_msg     = NULL;

                HAL_TRACE_ERROR0 ("Invalid length for incoming BT HCI message.");
            }

            /* Message length is valid */
            if (byte)
            {
                /* Read rest of message */
                p_cb->rcv_state = NFC_HAL_RCV_BT_PAYLOAD_ST;
            }
            else
            {
                /* Message has no additional parameters. (Entire message has been received) */
                msg_received    = TRUE;
                p_cb->rcv_state = NFC_HAL_RCV_IDLE_ST;  /* Next, wait for packet type of next message */
            }
        }
        break;

    case NFC_HAL_RCV_BT_PAYLOAD_ST:
        p_cb->rcv_len--;
        if (p_cb->p_rcv_msg)
        {
            *((UINT8 *) (p_cb->p_rcv_msg + 1) + p_cb->p_rcv_msg->offset + p_cb->p_rcv_msg->len++) = byte;

            if (p_cb->rcv_len > 0)
            {
                /* Read in the rest of the message */
                len = DT_Nfc_Read (USERIAL_NFC_PORT, ((UINT8 *) (p_cb->p_rcv_msg + 1) + p_cb->p_rcv_msg->offset + p_cb->p_rcv_msg->len),  p_cb->rcv_len);
                p_cb->p_rcv_msg->len    += len;
                p_cb->rcv_len           -= len;
            }
        }

        /* Check if we read in entire message yet */
        if (p_cb->rcv_len == 0)
        {
            msg_received        = TRUE;
            p_cb->rcv_state     = NFC_HAL_RCV_IDLE_ST;      /* Next, wait for packet type of next message */
        }
        break;
    }

    /* If we received entire message */
#if (NFC_HAL_TRACE_PROTOCOL == TRUE)
    if (msg_received && p_cb->p_rcv_msg)
    {
        /* Display protocol trace message */
        DispHciEvt (p_cb->p_rcv_msg);
    }
#endif

    return msg_received;
}

/*******************************************************************************
**
** Function         nfc_hal_nci_proc_rx_bt_msg
**
** Description      Received BT message from NFCC
**
**                  Notify command complete if initializing NFCC
**                  Forward BT message to NFC task
**
** Returns          void
**
*******************************************************************************/
static void nfc_hal_nci_proc_rx_bt_msg (void)
{
    UINT8   *p;
    NFC_HDR *p_msg;
    UINT16  opcode, old_opcode;
    tNFC_HAL_BTVSC_CPLT       vcs_cplt_params;
    tNFC_HAL_BTVSC_CPLT_CBACK *p_cback = NULL;

    /* if complete BT message is received successfully */
    if (nfc_hal_cb.ncit_cb.p_rcv_msg)
    {
        p_msg   = nfc_hal_cb.ncit_cb.p_rcv_msg;
        HAL_TRACE_DEBUG1 ("nfc_hal_nci_proc_rx_bt_msg (): GOT an BT msgs init_sta:%d", nfc_hal_cb.dev_cb.initializing_state);
        HAL_TRACE_DEBUG2 ("event: 0x%x, wait_rsp:0x%x", p_msg->event, nfc_hal_cb.ncit_cb.nci_wait_rsp);
        /* increase the cmd window here */
        if (nfc_hal_cb.ncit_cb.nci_wait_rsp == NFC_HAL_WAIT_RSP_PROP)
        {
            p = (UINT8 *) (p_msg + 1) + p_msg->offset;
            if (*p == HCI_COMMAND_COMPLETE_EVT)
            {
                p  += 3; /* code, len, cmd window */
                STREAM_TO_UINT16 (opcode, p);
                p   = nfc_hal_cb.ncit_cb.last_hdr;
                STREAM_TO_UINT16 (old_opcode, p);
                if (opcode == old_opcode)
                {
                    nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;
                    p_cback = (tNFC_HAL_BTVSC_CPLT_CBACK *)nfc_hal_cb.ncit_cb.p_vsc_cback;
                    nfc_hal_cb.ncit_cb.p_vsc_cback  = NULL;
                    nfc_hal_main_stop_quick_timer (&nfc_hal_cb.ncit_cb.nci_wait_rsp_timer);
                }
            }
        }

        /* if initializing NFCC */
        if ((nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_W4_APP_COMPLETE) ||
            (nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_W4_CONTROL_DONE))
        {
            /* this is command complete event for baud rate update or download patch */
            p = (UINT8 *) (p_msg + 1) + p_msg->offset;

            p += 1;    /* skip opcode */
            STREAM_TO_UINT8  (vcs_cplt_params.param_len, p);

            p += 1;    /* skip num command packets */
            STREAM_TO_UINT16 (vcs_cplt_params.opcode, p);

            vcs_cplt_params.param_len -= 3;
            vcs_cplt_params.p_param_buf = p;

            if (nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_W4_CONTROL_DONE)
            {
                NFC_HAL_SET_INIT_STATE(NFC_HAL_INIT_STATE_IDLE);
                nfc_hal_cb.p_stack_cback (HAL_NFC_RELEASE_CONTROL_EVT, HAL_NFC_STATUS_OK);
            }
            if (p_cback)
            {
                nfc_hal_cb.ncit_cb.p_vsc_cback = NULL;
                (*p_cback) (&vcs_cplt_params);
            }

            /* do not BT send message to NFC task */
            GKI_freebuf (p_msg);
        }
        else
        {
            /* do not BT send message to NFC task */
            GKI_freebuf(nfc_hal_cb.ncit_cb.p_rcv_msg);
        }
        nfc_hal_cb.ncit_cb.p_rcv_msg = NULL;
    }
}

/*****************************************************************************
**
** Function         nfc_hal_nci_receive_msg
**
** Description
**      Handle incoming data (NCI events) from the serial port.
**
**      If there is data waiting from the serial port, this funciton reads the
**      data and parses it. Once an entire NCI message has been read, it sends
**      the message the the NFC_TASK for processing
**
*****************************************************************************/
BOOLEAN nfc_hal_nci_receive_msg (void)
{
    tNFC_HAL_NCIT_CB *p_cb = &(nfc_hal_cb.ncit_cb);
    BOOLEAN msg_received = FALSE;

    msg_received = nfc_hal_nci_receive_nci_msg (p_cb);
    if(nfc_hal_cb.wait_sleep_rsp)
    {
        HAL_TRACE_DEBUG0("posting semaphore_sleepcmd_complete \n");
        sem_post(&semaphore_sleepcmd_complete);
        nfc_hal_cb.wait_sleep_rsp = FALSE;
    }
    return msg_received;
}

/*******************************************************************************
**
** Function         nfc_hal_nci_preproc_rx_nci_msg
**
** Description      NFCC sends NCI message to DH while initializing NFCC
**                  processing low power mode
**
** Returns          TRUE, if NFC task need to receive NCI message
**
*******************************************************************************/
BOOLEAN nfc_hal_nci_preproc_rx_nci_msg (NFC_HDR *p_msg)
{
    UINT8 *p, *pp, cid;
    UINT8 mt, pbf, gid, op_code;
    UINT8 payload_len;
    UINT16 data_len;
    UINT8 nvmupdatebuff[260]={0},nvmdatabufflen=0;
    UINT8 *nvmcmd = NULL, nvmcmdlen = 0;
    UINT32 nvm_update_flag = 0;
    UINT32 pm_flag = 0, region2_enable = 0;
    UINT8 *p1,fused_nvm_flag=FALSE,updates_available=FALSE;

    HAL_TRACE_DEBUG0 ("nfc_hal_nci_preproc_rx_nci_msg()");
    GetNumValue("NVM_UPDATE_ENABLE_FLAG", &nvm_update_flag, sizeof(nvm_update_flag));
    GetNumValue("PM_ENABLE_FLAG", &pm_flag, sizeof(pm_flag));
    GetNumValue("FUSED_NVM_UPDATE_ENABLE_FLAG", &fused_nvm_flag, sizeof(fused_nvm_flag));

#ifdef DTA // <DTA>
    GetNumValue("DTA_MODE", &dta_flag_all, sizeof(dta_flag_all));
    if (dta_flag_all !=0){
        HAL_TRACE_DEBUG0 ("DTA MODE SET");
    }
    else{
        HAL_TRACE_DEBUG0 ("NORMAL MODE");
    }
#endif // </DTA>

    HAL_TRACE_DEBUG1 ("wait_reset_rsp : %d",wait_reset_rsp);
    if(wait_reset_rsp)
    {
        p1 = (UINT8 *) (p_msg + 1) + p_msg->offset;
        NCI_MSG_PRS_HDR0 (p1, mt, pbf, gid);
        NCI_MSG_PRS_HDR1 (p1, op_code);
        if((gid == NCI_GID_CORE) && (op_code == NCI_MSG_CORE_CONN_CREDITS) && (mt == NCI_MT_NTF))
        {
            HAL_TRACE_DEBUG0 ("core_con_credite ntf ignored...");
            //wait_reset_rsp = FALSE;
            return FALSE;
        }
        if((gid == NCI_GID_CORE) && (op_code == NCI_MSG_CORE_RESET) && (mt == NCI_MT_RSP))
        {
            HAL_TRACE_DEBUG0 (" core reset rsp recieved...");
            wait_reset_rsp = FALSE;
        }
    }
    if (nfc_hal_cb.propd_sleep)
    {
        p1 = (UINT8 *) (p_msg + 1) + p_msg->offset;
        NCI_MSG_PRS_HDR0 (p1, mt, pbf, gid);
        HAL_TRACE_DEBUG0 ("nfc_hal_nci_preproc_rx_nci_msg() propd_sleep");
        if ((mt == NCI_MT_RSP && gid == NCI_GID_PROP))
        {
            nfc_hal_cb.propd_sleep = SLEEP_OFF;
            /*Keep Track that NFCC is sleeping */
            nfc_hal_cb.is_sleeping = TRUE;
            nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;
            nfc_hal_main_stop_quick_timer (&nfc_hal_cb.ncit_cb.nci_wait_rsp_timer);
            return FALSE;
        }
    }
    /*
     * Initializing NFCC and not in ramdump mode.
     * If in ramdump mode then the NFCC probably crashed during initialisation so we need to handle it here.
     * The NFCC should be in ramdump state so no other messages should come through until we power cycle it.
     */
    if (    (nfc_hal_cb.dev_cb.initializing_state != NFC_HAL_INIT_STATE_IDLE)
        &&  (nfc_hal_cb.dev_cb.initializing_state != NFC_HAL_INIT_STATE_RAMDUMP))
    {
        HAL_TRACE_DEBUG0 ("nfc_hal_nci_preproc_rx_nci_msg: Initializing NFCC");
        nfc_hal_dm_proc_msg_during_init (p_msg);
        /* do not send message to NFC task while initializing NFCC */
        return  FALSE;
    }
    else
    {
        /*
         * If we are in NFC_HAL_INIT_STATE_RAMDUMP state we should receive no other msgs other than for a ramdump.
         * So even if we are NOT in NFC_HAL_INIT_STATE_IDLE state, processing should continue as expected.
         * When we exit NFC_HAL_INIT_STATE_RAMDUMP state processing should also continue as expected.
         */
        p = (UINT8 *) (p_msg + 1) + p_msg->offset;
        pp = p;
        NCI_MSG_PRS_HDR0 (p, mt, pbf, gid);
        NCI_MSG_PRS_HDR1 (p, op_code);
        payload_len = *p++;
        int reason = *p;

        if (mt == NCI_MT_DATA)
        {
            if (nfc_hal_cb.hci_cb.hcp_conn_id)
            {
                NCI_DATA_PRS_HDR(pp, pbf, cid, data_len);
                if (cid == nfc_hal_cb.hci_cb.hcp_conn_id)
                {
                    nfc_hal_hci_handle_hcp_pkt_from_hc (pp);
                }

            }
        }
        if (gid == NCI_GID_PROP && mt == NCI_MT_RSP && (op_code == NCI_OID_INIT || op_code == NCI_OID_GET || op_code == NCI_OID_END))
        {
            HAL_TRACE_DEBUG0 ("nfc_hal_nci_preproc_rx_nci_msg: RAMDUMP: Got a NFCC NCI_MT_RSP");
#ifndef NDEBUG
            nfc_hal_nci_handle_ramdump_rsp(mt, op_code, pbf, payload_len, p);
#else
            nfc_hal_nci_handle_ramdump_rsp(op_code, payload_len, p);
#endif
            /*
             * Indicate no further processing as we are in ram dump.
             * i.e. do not send message to NFC task.
             */
            if(!init_ramdump_completed)
            {
                /*Send first time TRUE to upper layer to clear timer*/
                init_ramdump_completed = TRUE;
                return TRUE;
            }
            else
               return FALSE;
        }

        if ((gid == NCI_GID_PROP)&&(op_code == NCI_MSG_PROP_SLEEP))
        {
            nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;
            nfc_hal_cb.is_sleeping = TRUE;
            nfc_hal_main_stop_quick_timer (&nfc_hal_cb.ncit_cb.nci_wait_rsp_timer);
            HAL_TRACE_DEBUG0 ("NCI_MSG_PROP_SLEEP received");
            fused_nvm_flag = FALSE;

            if(is_vsc_prop_sleep_cmd)
            {
                is_vsc_prop_sleep_cmd = FALSE;
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }

        if ((gid == NCI_GID_PROP) && ((op_code == NCI_MSG_PROP_MEMACCESS) || (fused_nvm_flag == TRUE)))
        {
            if (mt == NCI_MT_NTF)
            {
                if (op_code == NCI_MSG_HCI_NETWK)
                {
                    nfc_hal_hci_handle_hci_netwk_info ((UINT8 *) (p_msg + 1) + p_msg->offset);
                }
            }
            /* Checking the rsp of NVM update cmd*/
            if(nvm_update_flag)
            {
                if(current_mode != FTM_MODE)
                {
                    /*To check if first fused nvm update entry is sent*/
                    if(nfc_hal_cb.nvm.is_started != TRUE)
                        return TRUE;
                    if(nfc_hal_cb.nvm.no_of_updates > 0)
                    {
                        if(mt == NCI_MT_RSP)
                        {
                            /*Check if NVM update file is available*/
                            if(fused_nvm_flag == TRUE)
                            {
                                updates_available = nfc_hal_dm_check_fused_nvm_file(nvmupdatebuff,&nvmdatabufflen);
                            }
                            else
                            {
                                updates_available = nfc_hal_dm_check_nvm_file(nvmupdatebuff,&nvmdatabufflen);
                            }

                            if((updates_available == TRUE)
                               && (nfc_hal_cb.nvm.no_of_updates > 0))
                            {
                                /* frame cmd now*/
                                nvmcmd = (UINT8*)malloc(nvmdatabufflen + 10);
                                if(nvmcmd)
                                {
                                    if(fused_nvm_flag == TRUE)
                                    {
                                        nfc_hal_dm_frame_fused_mem_access_cmd(nvmcmd,nvmupdatebuff,&nvmcmdlen);
                                    }
                                    else
                                    {
                                        nfc_hal_dm_frame_mem_access_cmd(nvmcmd,nvmupdatebuff,&nvmcmdlen);
                                    }
                                    /* send nvm update cmd(NCI POKE) to NFCC*/
                                    HAL_TRACE_DEBUG1 ("nfc_hal_cb.nvm.no_of_updates remained %d ",nfc_hal_cb.nvm.no_of_updates);
                                    nfc_hal_cb.nvm.no_of_updates--;
                                    nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;
                                    nfc_hal_dm_send_nci_cmd (nvmcmd, nvmcmdlen, NULL);
                                    free(nvmcmd);
                                    if(nfc_hal_cb.nvm.no_of_updates == 0)
                                    {
                                        /*all updates sent so close file again*/
                                        fclose( nfc_hal_cb.nvm.p_Nvm_file);
                                        nfc_hal_cb.nvm.p_Nvm_file = NULL;
                                        nfc_hal_cb.nvm.nvm_updated = TRUE;
                                        nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;
                                        /* Return TRUE now as all updates sent so that NFA may send further commands.*/
                                        return TRUE;
                                    }
                                    /* Return FALSE from here so that prop cmd's rsp is not reported to upper layers(e.g NFA)*/
                                    return FALSE;
                                }
                                /* Mem allocation failed*/
                                return FALSE;
                            }
                        }
                    }
                    else
                    {
                        NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_POST_INIT_DONE);
                        nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;
                        nfc_hal_dm_config_nfcc ();
                    }
                }
            }
        }
        else if (gid == NCI_GID_RF_MANAGE)
        {
            if (mt == NCI_MT_NTF)
            {
                if (op_code == NCI_MSG_RF_INTF_ACTIVATED)
                {
                    nfc_hal_cb.act_interface = NCI_INTERFACE_MAX + 1;
                    nfc_hal_cb.listen_mode_activated = FALSE;
                    nfc_hal_cb.kovio_activated = FALSE;
                    /*check which interface is activated*/
                    if((*(p+1) == NCI_INTERFACE_NFC_DEP))
                    {
                        if((*(p+3) == NCI_DISCOVERY_TYPE_LISTEN_F) ||
                           (*(p+3) == NCI_DISCOVERY_TYPE_LISTEN_A) ||
                           (*(p+3) == NCI_DISCOVERY_TYPE_POLL_F)   ||
                           (*(p+3) == NCI_DISCOVERY_TYPE_POLL_A)
                           )
                        {
                            nfc_hal_cb.act_interface = NCI_INTERFACE_NFC_DEP;
                            nfc_hal_cb.listen_setConfig_rsp_cnt = 0;
                        }
                    }
                    if((*(p+3) == NCI_DISCOVERY_TYPE_POLL_KOVIO))
                    {
                        HAL_TRACE_DEBUG0 ("Kovio Tag activated");
                        nfc_hal_cb.kovio_activated = TRUE;
                    }
                    if((*(p+3) == NCI_DISCOVERY_TYPE_LISTEN_F) ||
                       (*(p+3) == NCI_DISCOVERY_TYPE_LISTEN_A) ||
                       (*(p+3) == NCI_DISCOVERY_TYPE_LISTEN_B)
                      )
                    {
                        HAL_TRACE_DEBUG0 ("Listen mode activated");
#ifdef DTA // <DTA>
                        if(dta_flag_all) {
                            nfc_hal_cb.dev_cb.nfcc_sleep_mode = WAKE_STATE;
                        }
#endif // </DTA>
                        nfc_hal_cb.listen_mode_activated = TRUE;
                    }
                    if ((nfc_hal_cb.max_rf_credits) && (payload_len > 5))
                    {
                        /* API used wants to limit the RF data credits */
                        p += 5; /* skip RF disc id, interface, protocol, tech&mode, payload size */
                        if (*p > nfc_hal_cb.max_rf_credits)
                        {
                            HAL_TRACE_DEBUG2 ("RfDataCredits %d->%d", *p, nfc_hal_cb.max_rf_credits);
                            *p = nfc_hal_cb.max_rf_credits;
                        }
                    }
#ifdef DTA // <DTA>
                    if(dta_flag_all) {
                        HAL_TRACE_DEBUG1("nfc_hal_cb.listen_mode_activated : %d",nfc_hal_cb.listen_mode_activated);
                        if(nfc_hal_cb.listen_mode_activated != TRUE)
                        {
                            HAL_TRACE_DEBUG0 ("wake up nfcc \n");
                            nfc_hal_cb.is_sleeping = FALSE;
                            if(nfc_hal_cb.dev_cb.nfcc_sleep_mode != SLEEP_STATE)
                            {
                                HAL_TRACE_DEBUG0 ("Nfcc is already awake...1");
                            }
                            nfc_hal_dm_set_nfc_wake (NFC_HAL_ASSERT_NFC_WAKE);
                        }
                        else
                        {
                            if((*(p + 1) == NCI_INTERFACE_EE_DIRECT_RF))
                            {
                                nfc_hal_cb.act_interface = NCI_INTERFACE_EE_DIRECT_RF;
                            }
                        }
                    } else {
#endif // </DTA>
                    if((*(p + 1) != NCI_INTERFACE_EE_DIRECT_RF))
                    {
                        HAL_TRACE_DEBUG0 ("wake up nfcc \n");
                        nfc_hal_cb.is_sleeping = FALSE;
                if(nfc_hal_cb.dev_cb.nfcc_sleep_mode != SLEEP_STATE)
                {
                    HAL_TRACE_DEBUG0 ("Nfcc is already awake...2");
                }
                        nfc_hal_dm_set_nfc_wake (NFC_HAL_ASSERT_NFC_WAKE);
                    }
                    else
                    {
                        nfc_hal_cb.act_interface = NCI_INTERFACE_EE_DIRECT_RF;
            }
#ifdef DTA // <DTA>
                    }
#endif // </DTA>
                }
                if (op_code == NCI_MSG_RF_DEACTIVATE)
                {
                    if(nfc_hal_cb.act_interface == NCI_INTERFACE_NFC_DEP)
                    {
#ifdef DTA // <DTA>
                        if ( (!dta_flag_all) && (nfc_hal_cb.listen_mode_activated == FALSE) )
                            /* if not in DTA mode and not in Listen Mode (i.e. Poll Mode) */
#else
                        if ( nfc_hal_cb.listen_mode_activated == FALSE )
                            /* if not in Listen Mode (i.e. Poll Mode) */
#endif // </DTA>
                        {
                            HAL_TRACE_DEBUG0 ("NFC-DEP interface activated. Force wake up again to set RF_FIELD_INFO_EVT");
                            nfc_hal_cb.dev_cb.nfcc_sleep_mode = SLEEP_STATE;
                            nfc_hal_dm_set_nfc_wake (NFC_HAL_ASSERT_NFC_WAKE);
                        }
                    }
                    else
                    {
                        if(nfc_hal_cb.dev_cb.nfcc_sleep_mode == SLEEP_STATE)
                        {
                            HAL_TRACE_DEBUG0 ("Nfcc is already in sleep state...");
                        }
                        HAL_TRACE_DEBUG1 ("nfc_hal_cb.listen_mode_activated=%X",nfc_hal_cb.listen_mode_activated);
                        if((*(p) == NCI_DEACTIVATE_TYPE_DISCOVERY) &&
                           (!nfc_hal_cb.listen_mode_activated) &&
                           ( nfc_hal_cb.act_interface != NCI_INTERFACE_EE_DIRECT_RF)
                            &&(!nfc_hal_cb.kovio_activated)
                          )
                        {
                            nfc_hal_cb.dev_cb.nfcc_sleep_mode = WAKE_STATE;
                            nfc_hal_dm_send_prop_sleep_cmd ();
                            nfc_hal_cb.propd_sleep = SLEEP_ON;
                        }
                    }
                }
            }
            if (pm_flag)
            {
                if (mt == NCI_MT_RSP)
                {
                    if (op_code == NCI_MSG_RF_DISCOVER/*   ||
                        op_code == NCI_MSG_RF_DEACTIVATE*/
                        )
                    {
                        if (*p  == 0x00) //status good
                        {
                            nfc_hal_dm_send_prop_sleep_cmd ();
                            nfc_hal_cb.propd_sleep = SLEEP_ON;
                            nfc_hal_cb.init_sleep_done = 1;
                        }
                    }
#ifdef DTA // <DTA>
                 if(dta_flag_all == 0) {
#endif // </DTA>
                    if ( (op_code == NCI_MSG_RF_DEACTIVATE) && (*p  == 0x00))
                    {
                        if (nfc_hal_cb.deact_type == 0x00)
                        {
                            HAL_TRACE_DEBUG0 ("NCI_MSG_RF_DEACTIVATE RSP in IDLE..send sleep");
                            nfc_hal_dm_send_prop_sleep_cmd ();
                            nfc_hal_cb.propd_sleep = SLEEP_ON;
                            nfc_hal_cb.init_sleep_done = 1;
                        }
                    }
#ifdef DTA // <DTA>
                 }
#endif // </DTA>
                }
            }

#ifdef RAMDUMP_TEST_POKE
            /*
             * Simulate a NFCC reset for test purposes.
             * Ensure we receive a RSP for a CORE_INIT_CMD. We can only get the NFCC into a RAMDUMP state when it is in the correct state!
             */
            static BOOLEAN reset = TRUE;
            if (reset && (mt == NCI_MT_RSP) && (nfc_hal_cb.dev_cb.initializing_state != NFC_HAL_INIT_STATE_RAMDUMP))
            {
                HAL_TRACE_DEBUG0 ("nfc_hal_nci_preproc_rx_nci_msg: RAMDUMP: Receiving RSP to CORE_INIT_CMD, NFCC in correct state to request a ramdump. Simulating the NFCC reset...");
                reset = FALSE;
                /* Reset nfcc for testing ramdump state sequence */
                nfc_hal_dm_send_prop_reset_nfcc_ramdump_poke();
            }
#endif
        }
        else if (gid == NCI_GID_CORE)
        {
            if (mt == NCI_MT_RSP)
            {
                if(op_code == NCI_MSG_CORE_SET_CONFIG)
                {
                    if(nfc_hal_cb.act_interface == NCI_INTERFACE_NFC_DEP)
                    {
                        nfc_hal_cb.listen_setConfig_rsp_cnt++;
                        if(nfc_hal_cb.listen_setConfig_rsp_cnt == 2)
                        {
                            /* This is to take care of sleep to be sent once
                              all config cmds are sent from JNI;
                              if we change JNI config cmds then this will change
                              */
                            HAL_TRACE_DEBUG0 ("Sending sleep command ..");
                            nfc_hal_dm_send_prop_sleep_cmd ();
                            nfc_hal_cb.propd_sleep = SLEEP_ON;
                            nfc_hal_cb.listen_setConfig_rsp_cnt = 0;
                            nfc_hal_cb.act_interface = NCI_INTERFACE_MAX + 1;
                        }
                    }
                }
                if (op_code == NCI_MSG_CORE_CONN_CREATE)
                {
                    if (nfc_hal_cb.hci_cb.b_wait_hcp_conn_create_rsp)
                    {
                        p++; /* skip status byte */
                        nfc_hal_cb.hci_cb.b_wait_hcp_conn_create_rsp = FALSE;
                        p++; /* skip buff size */
                        p++; /* num of buffers */
                        nfc_hal_cb.hci_cb.hcp_conn_id = *p;
                    }
                }
                if(current_mode != FTM_MODE)
                {
                    GetNumValue("REGION2_ENABLE", &region2_enable, sizeof(region2_enable));
                    if(region2_enable)
                    {
                        if(op_code == NCI_MSG_CORE_RESET)
                        {
                            /*Send NciRegionControlEnable command every time after CORE_RESET cmd*/
                            HAL_TRACE_DEBUG0 ("Sending NciRegionControlEnable command..");
                            nfc_hal_dm_send_prop_nci_region2_control_enable_cmd(REGION2_CONTROL_ENABLE);
                        }
                        if(op_code == NCI_MSG_CORE_INIT)
                        {
                            nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;
                        }
                    }
                }

                if(current_mode != FTM_MODE)
                {
                    if(nvm_update_flag)
                    {
                        if (op_code == NCI_MSG_CORE_INIT)
                        {
                            HAL_TRACE_DEBUG0 ("Second CORE_INIT Rsp recieved...checking nvm file again");
                            if(fused_nvm_flag == TRUE)
                            {
                                updates_available = nfc_hal_dm_check_fused_nvm_file(nvmupdatebuff,&nvmdatabufflen);
                            }
                            else
                            {
                                updates_available = nfc_hal_dm_check_nvm_file(nvmupdatebuff,&nvmdatabufflen);
                            }
                            if((updates_available == TRUE) && (nfc_hal_cb.nvm.no_of_updates > 0))
                            {
                                /* frame cmd now*/
                                nvmcmd = (UINT8*)malloc(nvmdatabufflen + 10);
                                if(nvmcmd)
                                {
                                    if(fused_nvm_flag == TRUE)
                                    {
                                        nfc_hal_dm_frame_fused_mem_access_cmd(nvmcmd,nvmupdatebuff,&nvmcmdlen);
                                    }
                                    else
                                    {
                                        nfc_hal_dm_frame_mem_access_cmd(nvmcmd,nvmupdatebuff,&nvmcmdlen);
                                    }
                                    /* send nvm update cmd to NFCC*/
                                    HAL_TRACE_DEBUG1 ("nfc_hal_cb.nvm.no_of_updates remained %d ",nfc_hal_cb.nvm.no_of_updates);
                                    nfc_hal_cb.nvm.is_started = TRUE;
                                    nfc_hal_cb.nvm.no_of_updates--;
                                    nfc_hal_dm_send_nci_cmd (nvmcmd, nvmcmdlen, NULL);
                                    free(nvmcmd);
                                    if(nfc_hal_cb.nvm.no_of_updates == 0)
                                    {
                                        /*all updates sent so close file again*/
                                        HAL_TRACE_DEBUG0 ("nfc_hal_nci_preproc_rx_nci_msg() : in NCI_MT_RSP");
                                        fclose( nfc_hal_cb.nvm.p_Nvm_file);
                                        nfc_hal_cb.nvm.p_Nvm_file = NULL;
                                        nfc_hal_cb.nvm.nvm_updated = TRUE;
                                        nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;
                                    }
                                    return TRUE;
                                }
                                /*Memory allocation failed*/
                                return FALSE;
                            }
                        }
                    }
                }
            }
            else if ((mt == NCI_MT_NTF) && (op_code == NCI_MSG_CORE_RESET) && (NCI_HAL_RAMDUMP_REASON == reason))
            {
                HAL_TRACE_DEBUG0 ("nfc_hal_nci_preproc_rx_nci_msg: RAMDUMP: Got a NFCC CORE_RESET_NTF");
#ifndef NDEBUG
                nfc_hal_nci_handle_ramdump_ntf(mt, op_code, pbf, payload_len, reason);
#else
                nfc_hal_nci_handle_ramdump_ntf();
#endif
                /*
                 * Indicate no further processing as we are in ram dump.
                 * i.e. do not send message to NFC task.
                 */

                reset_status = TRUE;
                return FALSE;
            }
#ifdef RAMDUMP
            else if ((mt == NCI_MT_NTF) && (op_code == NCI_MSG_CORE_RESET))
            {
                /* Nothing to be sent back to NFCC */
                HAL_TRACE_DEBUG1 ("nfc_hal_nci_preproc_rx_nci_msg: RAMDUMP: Got a NFCC CORE_RESET_NTF. Ramdump is 'NOT' available, reason given is 0x%x", reason);
            }
#endif
        }
    }

    if (nfc_hal_cb.dev_cb.power_mode == NFC_HAL_POWER_MODE_FULL)
    {
        if (nfc_hal_cb.dev_cb.snooze_mode != NFC_HAL_LP_SNOOZE_MODE_NONE)
        {
            /* extend idle timer */
            nfc_hal_dm_power_mode_execute (NFC_HAL_LP_RX_DATA_EVT);
        }
    }

    return TRUE;
}

/*******************************************************************************
**
** Function         nfc_hal_nci_add_nfc_pkt_type
**
** Description      Add packet type (HCIT_TYPE_NFC)
**
** Returns          TRUE, if NFCC can receive NCI message
**
*******************************************************************************/
void nfc_hal_nci_add_nfc_pkt_type (NFC_HDR *p_msg)
{
    UINT8   *p;
    UINT8   hcit;

    /* add packet type in front of NCI header */
    if (p_msg->offset > 0)
    {
        p_msg->offset--;
        p_msg->len++;

        p  = (UINT8 *) (p_msg + 1) + p_msg->offset;
        *p = HCIT_TYPE_NFC;
    }
    else
    {
        HAL_TRACE_ERROR0 ("nfc_hal_nci_add_nfc_pkt_type () : No space for packet type");
        hcit = HCIT_TYPE_NFC;
        DT_Nfc_Write(USERIAL_NFC_PORT, &hcit, 1);
    }
}

/*******************************************************************************
**
** Function         nci_brcm_check_cmd_create_hcp_connection
**
** Description      Check if this is command to create HCP connection
**
** Returns          None
**
*******************************************************************************/
static void nci_brcm_check_cmd_create_hcp_connection (NFC_HDR *p_msg)
{
    UINT8 *p;
    UINT8 mt, pbf, gid, op_code;

    nfc_hal_cb.hci_cb.b_wait_hcp_conn_create_rsp = FALSE;

    p = (UINT8 *) (p_msg + 1) + p_msg->offset;

    if (nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_IDLE)
    {
        NCI_MSG_PRS_HDR0 (p, mt, pbf, gid);
        NCI_MSG_PRS_HDR1 (p, op_code);

        if (gid == NCI_GID_CORE)
        {
            if (mt == NCI_MT_CMD)
            {
                if (op_code == NCI_MSG_CORE_CONN_CREATE)
                {
                    if (  ((NCI_CORE_PARAM_SIZE_CON_CREATE + 4) == *p++)
                        &&(NCI_DEST_TYPE_NFCEE == *p++)
                        &&(1 == *p++)
                        &&(NCI_CON_CREATE_TAG_NFCEE_VAL == *p++)
                        &&(2 == *p++)  )
                    {
                        p++;
                        if (NCI_NFCEE_INTERFACE_HCI_ACCESS == *p)
                        {
                            nfc_hal_cb.hci_cb.b_wait_hcp_conn_create_rsp = TRUE;
                            return;
                        }
                    }

                }
            }
        }
    }
}

/*******************************************************************************
**
** Function         nfc_hal_nci_send_cmd
**
** Description      Send NCI command to the transport
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_nci_send_cmd (NFC_HDR *p_buf)
{
    BOOLEAN continue_to_process = TRUE;
    UINT8   *ps, *pd;
    UINT16  max_len;
    UINT16  buf_len, offset;
    UINT8   *p;
    UINT8   hdr[NCI_MSG_HDR_SIZE];
    UINT8   nci_ctrl_size = nfc_hal_cb.ncit_cb.nci_ctrl_size;
    UINT8   delta = 0;
    UINT8 *p1;
    UINT8 mt, pbf, gid, op_code;
    UINT8 payload_len;
    if (  (nfc_hal_cb.hci_cb.hcp_conn_id == 0)
        &&(nfc_hal_cb.nvm_cb.nvm_type != NCI_SPD_NVM_TYPE_NONE)  )
        nci_brcm_check_cmd_create_hcp_connection ((NFC_HDR*) p_buf);

    /* check low power mode state */
    continue_to_process = nfc_hal_dm_power_mode_execute (NFC_HAL_LP_TX_DATA_EVT);

    if (!continue_to_process)
    {
        /* save the command to be sent until NFCC is free. */
        nfc_hal_cb.ncit_cb.p_pend_cmd   = p_buf;
        return;
    }

    max_len = nci_ctrl_size + NCI_MSG_HDR_SIZE;
    buf_len = p_buf->len;
    offset  = p_buf->offset;
#ifdef DISP_NCI
    if (buf_len > max_len)
    {
        /* this command needs to be fragmented. display the complete packet first */
        DISP_NCI ((UINT8 *) (p_buf + 1) + p_buf->offset, p_buf->len, FALSE);
    }
#endif
    ps      = (UINT8 *) (p_buf + 1) + p_buf->offset;
    memcpy (hdr, ps, NCI_MSG_HDR_SIZE);
    while (buf_len > max_len)
    {
        HAL_TRACE_DEBUG2 ("buf_len (%d) > max_len (%d)", buf_len, max_len);
        /* the NCI command is bigger than the NFCC Max Control Packet Payload Length
         * fragment the command */

        p_buf->len  = max_len;
        ps   = (UINT8 *) (p_buf + 1) + p_buf->offset;
        /* mark the control packet as fragmented */
        *ps |= NCI_PBF_ST_CONT;
        /* adjust the length of this fragment */
        ps  += 2;
        *ps  = nci_ctrl_size;

        /* add NCI packet type in front of message */

        /* send this fragment to transport */
        p = (UINT8 *) (p_buf + 1) + p_buf->offset;

#ifdef DISP_NCI
        delta = p_buf->len - max_len;
        DISP_NCI (p + delta, (UINT16) (p_buf->len - delta), FALSE);
#endif

        DT_Nfc_Write (USERIAL_NFC_PORT, p, p_buf->len);
        /* adjust the len and offset to reflect that part of the command is already sent */
        buf_len -= nci_ctrl_size;
        offset  += nci_ctrl_size;
        HAL_TRACE_DEBUG2 ("p_buf->len: %d buf_len (%d)", p_buf->len, buf_len);
        p_buf->len      = buf_len;
        p_buf->offset   = offset;
        pd   = (UINT8 *) (p_buf + 1) + p_buf->offset;
        /* restore the NCI header */
        memcpy (pd, hdr, NCI_MSG_HDR_SIZE);
        pd  += 2;
        *pd  = (UINT8) (p_buf->len - NCI_MSG_HDR_SIZE);
    }

    HAL_TRACE_DEBUG1 ("p_buf->len: %d", p_buf->len);

    /* add NCI packet type in front of message */

    /* send this fragment to transport */
    p = (UINT8 *) (p_buf + 1) + p_buf->offset;

    p1 = (UINT8 *) (p_buf + 1) + p_buf->offset;
    NCI_MSG_PRS_HDR0 (p1, mt, pbf, gid);
    NCI_MSG_PRS_HDR1 (p1, op_code);
    payload_len = *p1++;
    if (op_code == NCI_MSG_RF_DEACTIVATE)
    {
        if(*p1  == 0x00)
        {
           HAL_TRACE_DEBUG2 ("send deactivate in idle NCI_MSG_RF_DEACTIVATE: %d,payload_len=%d", *p1,payload_len);
        }
         nfc_hal_cb.deact_type = *p1;
    }
#ifdef DISP_NCI
    delta = p_buf->len - buf_len;
    DISP_NCI (p + delta, (UINT16) (p_buf->len - delta), FALSE);
#endif
    DT_Nfc_Write (USERIAL_NFC_PORT, p, p_buf->len);

    GKI_freebuf (p_buf);
}

/*******************************************************************************
**
** Function         nfc_hal_nci_cmd_timeout_cback
**
** Description      callback function for timeout
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_nci_cmd_timeout_cback (void *p_tle)
{
    TIMER_LIST_ENT  *p_tlent = (TIMER_LIST_ENT *)p_tle;

    HAL_TRACE_DEBUG0 ("nfc_hal_nci_cmd_timeout_cback ()");

    nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;

    if (p_tlent->event == NFC_HAL_TTYPE_NCI_WAIT_RSP)
    {
        if (nfc_hal_cb.dev_cb.initializing_state <= NFC_HAL_INIT_STATE_W4_PATCH_INFO)
        {
            NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_IDLE);
            nfc_hal_cb.ncit_cb.hw_error = TRUE;
            nfc_hal_main_pre_init_done (HAL_NFC_STATUS_ERR_CMD_TIMEOUT);
        }
        else if (nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_W4_APP_COMPLETE)
        {
            if (nfc_hal_cb.prm.state != NFC_HAL_PRM_ST_IDLE)
            {
                nfc_hal_prm_process_timeout (NULL);
            }
            else
            {
                NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_IDLE);
                nfc_hal_cb.ncit_cb.hw_error = TRUE;
                nfc_hal_main_pre_init_done (HAL_NFC_STATUS_ERR_CMD_TIMEOUT);
            }
        }
        else if (nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_W4_POST_INIT_DONE)
        {
            NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_IDLE);
            nfc_hal_cb.ncit_cb.hw_error = TRUE;
            nfc_hal_cb.p_stack_cback (HAL_NFC_POST_INIT_CPLT_EVT, HAL_NFC_STATUS_ERR_CMD_TIMEOUT);
        }
        else if (nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_W4_CONTROL_DONE)
        {
            NFC_HAL_SET_INIT_STATE(NFC_HAL_INIT_STATE_IDLE);
            nfc_hal_cb.ncit_cb.hw_error = TRUE;
            nfc_hal_cb.p_stack_cback (HAL_NFC_RELEASE_CONTROL_EVT, HAL_NFC_STATUS_ERR_CMD_TIMEOUT);
        }
        else if (nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_W4_PREDISCOVER_DONE)
        {
            NFC_HAL_SET_INIT_STATE(NFC_HAL_INIT_STATE_IDLE);
            nfc_hal_cb.ncit_cb.hw_error = TRUE;
            nfc_hal_cb.p_stack_cback (HAL_NFC_PRE_DISCOVER_CPLT_EVT, HAL_NFC_STATUS_ERR_CMD_TIMEOUT);
        }
        else if (nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_RAMDUMP)
        {
            /* tidy up ramdump memory so no dangling pointers */
            HAL_TRACE_DEBUG0 ("nfc_hal_nci_cmd_timeout_cback: RAMDUMP: RSP timer expired! Enabling PowerCycle");
            nfc_hal_nci_close_ramdump();

            /* Reset initialisation variables. Should be part of power cycle routine! */
            nfc_hal_cb.dev_cb.patch_applied = FALSE;
            nfc_hal_cb.dev_cb.pre_init_done = FALSE;
            nfc_hal_cb.dev_cb.pre_patch_signature_chk = FALSE;
            nfc_hal_cb.dev_cb.patch_signature_chk = FALSE;
            nfc_hal_cb.dev_cb.pre_patch_applied = FALSE;
            nfc_hal_cb.dev_cb.pre_patch_file_available = FALSE;
            nfc_hal_cb.dev_cb.fw_version_chk = FALSE;

            DT_Nfc_RamdumpPerformed = TRUE;
            GKI_send_event (NFC_HAL_TASK, NFC_HAL_TASK_EVT_POWER_CYCLE);
        }
    }
}


/*******************************************************************************
**
** Function         HAL_NfcSetMaxRfDataCredits
**
** Description      This function sets the maximum RF data credit for HAL.
**                  If 0, use the value reported from NFCC.
**
** Returns          none
**
*******************************************************************************/
void HAL_NfcSetMaxRfDataCredits (UINT8 max_credits)
{
    HAL_TRACE_DEBUG2 ("HAL_NfcSetMaxRfDataCredits %d->%d", nfc_hal_cb.max_rf_credits, max_credits);
    nfc_hal_cb.max_rf_credits   = max_credits;
}


/*******************************************************************************
**
** Function         nfc_hal_nci_handle_ramdump_ntf
**
** Description      On receiving a ramdump notification performs some initial checks before processing.
**
** Precondition     The reason code is 0xA0
**
** Returns          none
**
*******************************************************************************/
#ifndef NDEBUG
void nfc_hal_nci_handle_ramdump_ntf(int mt, int op_code, int pbf, int payload_len, int reason)
#else
void nfc_hal_nci_handle_ramdump_ntf()
#endif
{
    /*
     * Two scenarios for arriving here;
     *
     *   1) We have issued a CORE_RESET_CMD, received back a CORE_RESET_NTF
     *   2) The NSFCC has reset with an internal error, receive a CORE_RESET_NTF
     *
     *   GID = CORE_ = 0x00
     *   60 00 RESET_NTF (see NFCForum-TS-NCI-1.0, Table 102, GID & OID definitions and NFC Middleware, chapter 4 NCI Summary)
     *   MT = 011b, i.e. (NCI_MT_NTF << NCI_MT_SHIFT) = 0x60
     *   OID = 000000b
     *
     * In either case, the reason code of the notification determines the scenario.
     * Segmentation will not occur on messages of maximum size 255 octets.
     *
     * Reason Code
     * 0x00         Unspecified reason
     * 0x01-0x9F    RFU
     * 0xA0         HardFault with RAM Dump available (see NFC Middleware/Post Mortem Debug HLD, Table 5.1)
     *
     * Configuration Status
     * 0x00         NCI RF Configuration has been kept
     * 0x01         NCI RF Configuration has been reset
     * 0x02-0xFF    RFU
     */
    HAL_TRACE_DEBUG0 ("nfc_hal_nci_handle_ramdump_ntf: RAMDUMP: Received ramdump NTF from NFCC");
    if(gRamdump_Data.ramdump_in_progress)
    {
        HAL_TRACE_DEBUG0 ("nfc_hal_nci_handle_ramdump_ntf: RAMDUMP: RamDump already in progress. Returning.");
        return;
    }
    /* Autonomous NFCC reset completed. (see Table 2: MT values) */
    /* Indicate Ramdump in progress */
    gRamdump_Data.ramdump_in_progress = TRUE;
    const int kRamDump_notif_payload_len = 2;
#ifdef RAMDUMP
    HAL_TRACE_DEBUG3 ("nfc_hal_nci_handle_ramdump_ntf: RAMDUMP: **DEBUG** pbf == %d (expecting == 0), reason == 0x%X (expecting 0x%X)", pbf, reason, NCI_HAL_RAMDUMP_REASON);
    HAL_TRACE_DEBUG2 ("nfc_hal_nci_handle_ramdump_ntf: RAMDUMP: **DEBUG** payload_len == %d (expecting <= %d)", payload_len, nfc_hal_cb.ncit_cb.nci_ctrl_size);
    HAL_TRACE_DEBUG2 ("nfc_hal_nci_handle_ramdump_ntf: RAMDUMP: **DEBUG** payload_len == %d (expecting == %d)", payload_len, kRamDump_notif_payload_len);
    assert(pbf == 0); /* No segmentation */
    assert(payload_len <= nfc_hal_cb.ncit_cb.nci_ctrl_size);
    assert(payload_len == kRamDump_notif_payload_len);
    assert(reason == NCI_HAL_RAMDUMP_REASON);
#endif
    HAL_TRACE_DEBUG0 ("nfc_hal_nci_handle_ramdump_ntf: RAMDUMP: NFCC ramdump is available");
    /*
     * 4.1 Reset of NFCC (from the standard)
     * The NFCC MAY also reset itself (without having received a CORE_RESET_CMD); e.g., in the case of an internal error.
     * In these cases, the NFCC SHALL inform the DH with the CORE_RESET_NTF.
     *
     * The Reason code SHALL reflect the internal reset reason and the Configuration Status the status of the NCI RF Configuration.
     * We are interested in code 0xA0 that states internal error.
     */

    /* Initiate a RAMDUMP if available */
    nfc_hal_nci_recv_ramdump_event(RamDump_Recv_Init_Ntf);
}

/*******************************************************************************
**
** Function         nfc_hal_nci_handle_ramdump_rsp
**
** Description      On receiving a ramdump responce performs some initial checks before processing.
**
** Returns          none
**
*******************************************************************************/
#ifndef NDEBUG
void nfc_hal_nci_handle_ramdump_rsp(int mt, int op_code, int pbf, int payload_len, UINT8 * const payload_ptr)
#else
void nfc_hal_nci_handle_ramdump_rsp(int op_code, int payload_len, UINT8 * const payload_ptr)
#endif
{
#ifdef RAMDUMP
    HAL_TRACE_DEBUG2 ("nfc_hal_nci_handle_ramdump_rsp: RAMDUMP: Receiving MT %d, OID %d from NFCC during ramdump communication", mt, op_code);
    HAL_TRACE_DEBUG3 ("nfc_hal_nci_handle_ramdump_rsp: RAMDUMP: **DEBUG** op_code-%d = %d (expecting < %d)", NCI_OID_INIT, op_code-NCI_OID_INIT, RamDump_Item_Count-1);
    HAL_TRACE_DEBUG1 ("nfc_hal_nci_handle_ramdump_rsp: RAMDUMP: **DEBUG** pbf = %d (expecting 0)", pbf);
    HAL_TRACE_DEBUG2 ("nfc_hal_nci_handle_ramdump_rsp: RAMDUMP: **DEBUG** payload_len = %d (expecting <= %d)", payload_len, nfc_hal_cb.ncit_cb.nci_ctrl_size);

    assert((op_code-NCI_OID_INIT) < RamDump_Item_Count-1);
    assert(pbf == 0); /* No segmentation */
    assert(payload_len <= nfc_hal_cb.ncit_cb.nci_ctrl_size);
#else
    HAL_TRACE_DEBUG1 ("nfc_hal_nci_handle_ramdump_rsp: RAMDUMP: Receiving OID %d from NFCC during ramdump communication", op_code);
#endif

    /* Not interested in reason or config status */
    /*
     * Determine what state we should be in for this event.
     * Correlate the opcode to the correct RAMDUMP state.
     */
    static const RamDumpState prop_ramdump_state[RamDump_Item_Count-1] = {RamDump_Recv_Init_Rsp, RamDump_Recv_Get_Rsp, RamDump_Recv_End_Rsp};
    if (RamDump_Recv_Init_Rsp == prop_ramdump_state[op_code-NCI_OID_INIT] || RamDump_Recv_Get_Rsp == prop_ramdump_state[op_code-NCI_OID_INIT])
    {
        /* remember the payload attributes for data extraction later */
        gRamdump_Data.msg_payload_ptr = payload_ptr;
        gRamdump_Data.msg_payload_length = payload_len;
    }
    nfc_hal_nci_recv_ramdump_event(prop_ramdump_state[op_code-NCI_OID_INIT]);
}

/*******************************************************************************
**
** Function         nfc_hal_nci_recv_ramdump_response
**
** Description      On receiving a notification from the NFCC we need to ascertain what next needs to be done.
**
**
** Returns          none
**
*******************************************************************************/
void nfc_hal_nci_recv_ramdump_event(RamDumpState state)
{
    /*
     * NFCC will reset its internal state only if it does not receive the expected event pertaining to that internal state.
     *
     * It should NEVER happen that these values do not correlate as that would mean our RAMDUMP state machine is incorrect!
     * If it does happen the default behaviour id to Reset the device!
     */
    HAL_TRACE_DEBUG1 ("nfc_hal_nci_recv_ramdump_event: RAMDUMP: Passed in state %d.", state);

    /*
     * Stop NFC RSP timer from expiring.
     * This will avoid any already sent CMD from not receiving a response.
     * The timer will be started automatically when we send our ramdump commands but will NOT be associated with that previous CMD.
     */
    HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_ramdump_event: RAMDUMP: Stopping timer for outstanding RSPs");

    /*
     * Stop the NFC RSP timer and indicate no RSP outstanding (So we can continue to send CMDs).
     * Note: Any other CMD that has been sent prior to here and after the ramdump NTF has been received will effectively be deadlocked!
     * The upper layers should handle this case of them never receiving a RSP as the NFCC will not remember the CMD after it's reset.
     */
    nfc_hal_main_stop_quick_timer (&nfc_hal_cb.ncit_cb.nci_wait_rsp_timer);
    nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;
    /*
     * We are called from nfc_hal_nci_preproc_rx_nci_msg() when receiving RAMDUMP events from the NFCC
     */

    static void (* const stateFunctionPtr[])(void) = {nfc_hal_nci_recv_init_ramdump_ntf,    /* RamDump_Recv_Init_Ntf */
                                                         nfc_hal_nci_recv_init_ramdump_rsp, /* RamDump_Recv_Init_Rsp */
                                                         nfc_hal_nci_recv_get_ramdump_rsp,  /* RamDump_Recv_Get_Rsp */
                                                         nfc_hal_nci_recv_end_ramdump_rsp}; /* RamDump_Recv_End_Rsp */

    /*
     * The first time we are called will be in due to a NTF being received from the NFCC.
     * All other times are due to RSP being received from the NFCC.
     * We do not have a state machine, we instead respond to the NFCC's state machine requests (NTF and RSPs').
     * Within our function that handles the NFCC event, depending on the payload we will respond accordingly.
     */
    /* handle the state */
    stateFunctionPtr[state]();
}

/*******************************************************************************
**
** Function         nfc_hal_nci_recv_init_ramdump_ntf
**
** Description      On receiving CORE_RESET_NTF notification from NFCC
**                  we need to initiate a RAMDUMP. This involves configuration files and a file for saving uploaded data.
**
** Returns          none
**
*******************************************************************************/
void nfc_hal_nci_recv_init_ramdump_ntf()
{
    HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_init_ramdump_ntf: RAMDUMP: Received a CORE_RESET_NTF from NFCC");

    BOOLEAN error = FALSE;

    /*
     * Stop packets being sent to NFCC by anyone else.
     * Stop packets being received from NFCC to anyone else.
     */
    HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_init_ramdump_ntf: RAMDUMP: Entering state NFC_HAL_INIT_STATE_RAMDUMP");
    NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_RAMDUMP);

    /***
     * Create a buffer large enough to store the config data.
     */
    FILE * config_file_ptr = fopen (kRamdump_config_filename, "r");
    if (config_file_ptr == NULL)
    {
        HAL_TRACE_DEBUG1 ("nfc_hal_nci_recv_init_ramdump_ntf: RAMDUMP: **ERROR** Unable to open config file \"%s\".", kRamdump_config_filename);
        error = TRUE;
    }
    if (!error && fseek(config_file_ptr, 0L, SEEK_END) != 0)
    {
        HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_init_ramdump_ntf: RAMDUMP: **ERROR** Unable to position to end of config file.");
        error = TRUE;
    }
    size_t sz = 0;
    if (!error && ((sz = ftell(config_file_ptr)) <= 0))
    {
        HAL_TRACE_DEBUG1 ("nfc_hal_nci_recv_init_ramdump_ntf: RAMDUMP: **ERROR** Config file error obtaining size, or empty (size reported is %d)", sz);
        error = TRUE;
    }
    if (!error && fseek(config_file_ptr, 0L, SEEK_SET) != 0)
    {
        HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_init_ramdump_ntf: RAMDUMP: **ERROR** Unable to position to beginning of config file.");
        error = TRUE;
    }
    gRamdump_Data.config_buffer = (UINT8*) malloc(sz);
    if (!error && gRamdump_Data.config_buffer == NULL)
    {
        HAL_TRACE_DEBUG1 ("nfc_hal_nci_recv_init_ramdump_ntf: RAMDUMP: **ERROR** Unable to malloc config buffer of %d length.", sz);
        error = TRUE;
    }

    /* copy the data*/
    size_t bytes_read = 0;
    if (!error && (((bytes_read = fread (gRamdump_Data.config_buffer , sizeof(char), sz, config_file_ptr)) == 0) || (bytes_read != sz)))
    {
        HAL_TRACE_DEBUG2 ("nfc_hal_nci_recv_init_ramdump_ntf: RAMDUMP: **ERROR** Wrong number of expected bytes read from config file (expected %d, read %d), or file empty", sz, bytes_read);
        error = TRUE;
    }
    if (!error && fclose (config_file_ptr) == EOF)
    {
        HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_init_ramdump_ntf: RAMDUMP: **WARNING** Unable to close config file. Continuing...");
    }
    /***
     * Create the register buffer.
     */
    /* Get NFCC version */
    UINT16 chip_version = DT_Get_Nfcc_Version(NFCC_CHIP_VERSION_REG);
    UINT16 chip_version_major = ((chip_version >> 4)&(0xF));
    GKI_delay( 2 );
    UINT16 chip_revid = DT_Get_Nfcc_Version(NFCC_CHIP_REVID_REG);
    UINT16 metal_version = (chip_revid & (0xF));
    /* Based on the NFCC version we get sent some data that we need to parse, make room for it */
    if (chip_version_major == 2 && metal_version == 4)
    {
        gMaxRegisterCount = 21;
        gNfccChipVersion = 24;
    }
    HAL_TRACE_DEBUG3("nfc_hal_nci_recv_init_ramdump_ntf: RAMDUMP: Gathering Register data. NFCC chip version = %d.%d. Register count is %d", chip_version_major, metal_version, gMaxRegisterCount);
    const int kMax_register_size = gMaxRegisterCount*REGISTER_FIELD_LENGTH;
    gRamdump_Data.register_buffer = (UINT8*) malloc(kMax_register_size);
    if (!error && gRamdump_Data.register_buffer == NULL)
    {
        HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_init_ramdump_ntf: RAMDUMP: **ERROR** Unable to malloc ramdump register buffer.");
        error = TRUE;
    }
    /***
     * Create/open with truncate, the output file.
     */
#define MAX_FILENAME_SIZE 256
    char buffer[MAX_FILENAME_SIZE];
    memset(buffer, 0, MAX_FILENAME_SIZE); /* zero out */
    /* file time stamp */
    time_t now;
    memset(buffer, 0, MAX_FILENAME_SIZE); /* zero out */
    now = time(NULL);
    if (!error && now != -1)
    {
       if (strftime(buffer, MAX_FILENAME_SIZE, kRamdump_ramdump_filename, gmtime(&now)) == 0)
       {
           HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_init_ramdump_ntf: RAMDUMP: **WARNING** Unable to read timestamp. Continuing...");
       }
    }
    /* create/truncate the file */
    gRamdump_Data.output_file_ptr = fopen (buffer, "w+");
    if (!error && gRamdump_Data.output_file_ptr == NULL)
    {
        HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_init_ramdump_ntf: RAMDUMP: **ERROR** Unable to create/open ramdump file.");
        error = TRUE;
    }
    /***
     * Extract the config data.
     * This involves creating a linked list of Data segment nodes.
     */
    if (!error)
    {
        error = nfc_hal_nci_parse_ramdump_config_create_dump_area(sz);
    }
    /***
     * Initiate or stop the ramdump process
     */
    if (!error)
    {
        /*
         * We have succesfully setup memory, files & variables to begin a RAMPDUMP process.
         * Initiate the RAMDUMP process.
         */
        HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_init_ramdump_ntf: RAMDUMP: Initiating ramdump communication with NFCC. Sending NFCC PROP_INIT_RAMDUMP_CMD");
        nfc_hal_dm_send_prop_init_ramdump_cmd();
    }
    else
    {
        /* End the RamDump prematurely */
        HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_init_ramdump_ntf: RAMDUMP: Prematurely ending ramdump communication with NFCC. Sending PROP END CMD.");
        nfc_hal_dm_send_prop_end_ramdump_cmd();
    }
}

/*******************************************************************************
**
** Function         nfc_hal_nci_invoke_ramdump_upload
**
** Description      Initiate the next ramdump upload.
**                  Ask for a max of 252 buffers to avoid segmentation.
**                  This means we manage the request and assembly of those parts
**                  ourselves, however, this saves on memory and copying.
**                  If we didn't care about segmentation the Reader thread would
**                  assemble the fragments into one big buffer then pass it to us.
**                  We do not get ownership of this buffer and would need to copy
**                  it locally as the Reader thread needs its limited buffers for
**                  other async events.
**                  Alternatively, create the local buffer as we know its max size
**                  and copy the fragments into it as they arrive. This saves one
**                  big copy and two large buffers required in the segmentation
**                  solution.
**
** Returns          none
**
*******************************************************************************/
static void nfc_hal_nci_invoke_ramdump_upload()
{
    /* check for config errors */
    if (gRamdump_Data.cd_current_item == NULL)
    {
        HAL_TRACE_DEBUG0("nfc_hal_nci_invoke_ramdump_upload: RAMDUMP: Config node non-existant! Ending ramdump communications.");
        nfc_hal_dm_send_prop_end_ramdump_cmd();
        return;
    }
    HAL_TRACE_DEBUG3 ("nfc_hal_nci_invoke_ramdump_upload: RAMDUMP: Initiating ramdump upload for item %p, NAME %s, total LENGTH %d bytes",
                        gRamdump_Data.cd_current_item, gRamdump_Data.cd_current_item->name, gRamdump_Data.cd_current_item->dump_length);
    /* to avoid segmentation we limit the number of bytes requested in each payload */
    int length_left = gRamdump_Data.cd_current_item->dump_length - gRamdump_Data.total_received;
    length_left = (length_left < nfc_hal_cb.ncit_cb.nci_ctrl_size) ? length_left : nfc_hal_cb.ncit_cb.nci_ctrl_size;

    HAL_TRACE_DEBUG1 ("nfc_hal_nci_invoke_ramdump_upload: RAMDUMP: length_left = %d",length_left);
    //assert (length_left % 4 == 0); /* requirement! */
    gRamdump_Data.bytes_requested = length_left;
    HAL_TRACE_DEBUG2 ("nfc_hal_nci_invoke_ramdump_upload: RAMDUMP: Sending NFCC PROP_GET_RAMDUMP_CMD requesting %d (of %d) bytes in this segment", length_left, (gRamdump_Data.cd_current_item->dump_length - gRamdump_Data.total_received));
    nfc_hal_dm_send_prop_get_ramdump_cmd((gRamdump_Data.cd_current_item->dump_start_addr + gRamdump_Data.total_received), length_left); /* **TBD: CHECK HOW START ADDRESS IS PACKED. WE ARE BIG ENDIAN ON ARM!!! */
}

/*******************************************************************************
**
** Function         nfc_hal_nci_recv_init_ramdump_rsp
**
** Description      Perform action following receiving PROP_INIT_RAMDUMP_RSP notification from NFCC.
**
** Returns          none
**
*******************************************************************************/
void nfc_hal_nci_recv_init_ramdump_rsp()
{
    HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_init_ramdump_rsp: RAMDUMP: Received a PROP_INIT_RAMDUMP_RSP from NFCC");

    /* copy the event payload register data */
    memcpy(gRamdump_Data.register_buffer, gRamdump_Data.msg_payload_ptr, gRamdump_Data.msg_payload_length);
    HAL_TRACE_DEBUG1 ("nfc_hal_nci_recv_init_ramdump_rsp: RAMDUMP: Saved %d register data bytes to buffer.", gRamdump_Data.msg_payload_length);
    /***
     * Parse and place the data into the output file.
     */
    nfc_hal_nci_parse_and_dump_ramdump_register_data();
    /* We are finished with the register data as we now have it in the output file */
    free (gRamdump_Data.register_buffer);
    gRamdump_Data.register_buffer = NULL;
    /***
     * Start the RAMDUMP upload
     */

    /* start from beginning */
    gRamdump_Data.total_received = 0;
    gRamdump_Data.cd_current_item = gRamdump_Data.cd_list;
    HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_init_ramdump_rsp: RAMDUMP: Requesting first segment of data for initial config file entry");
    nfc_hal_nci_invoke_ramdump_upload();
}

/*******************************************************************************
**
** Function         nfc_hal_nci_recv_get_ramdump_rsp
**
** Description      Perform action following receiving PROP_GET_RAMDUMP_RSP notification from NFCC.
**
** Returns          none
**
*******************************************************************************/
void nfc_hal_nci_recv_get_ramdump_rsp()
{
    HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_get_ramdump_rsp: RAMDUMP: Received a PROP_GET_RAMDUMP_RSP from NFCC");
    /*
     * Send the following;
     * Start Address    4    32 bit start address for data to upload. This value is interpreted in Big Endian format
     * Length           1    Number of bytes starting (inclusive) at Start Address to transfer
     *                       This value is required to be a multiple of 4 (as we always transfer 32 bit words) between 4 and 252
     *
     */
    HAL_TRACE_DEBUG3 ("nfc_hal_nci_recv_get_ramdump_rsp: RAMDUMP: Received %d bytes expecting %d bytes for '%s'",
                        gRamdump_Data.msg_payload_length, gRamdump_Data.bytes_requested, gRamdump_Data.cd_current_item->name);

    assert(gRamdump_Data.msg_payload_length == gRamdump_Data.bytes_requested);
    assert((gRamdump_Data.total_received + gRamdump_Data.msg_payload_length) <= gRamdump_Data.cd_current_item->dump_length);

    /* copy the event payload register data for later expansion */
    memcpy(gRamdump_Data.data + gRamdump_Data.total_received, gRamdump_Data.msg_payload_ptr, gRamdump_Data.msg_payload_length);

    HAL_TRACE_DEBUG4 ("nfc_hal_nci_recv_get_ramdump_rsp: RAMDUMP: Buffer start %p + offset %d = %p (buffer boundary %p)",
                        gRamdump_Data.data, gRamdump_Data.total_received, (gRamdump_Data.data + gRamdump_Data.total_received), (gRamdump_Data.data + gRamdump_Data.data_length));
    HAL_TRACE_DEBUG1 ("nfc_hal_nci_recv_get_ramdump_rsp: RAMDUMP: Saved %d register data bytes to buffer", gRamdump_Data.msg_payload_length);
                        gRamdump_Data.total_received += gRamdump_Data.msg_payload_length;

    /* Are there more segments for this dump?  */
    if ((gRamdump_Data.cd_current_item->dump_length - gRamdump_Data.total_received) > 0)
    {
        /***
         * Obtain next segment
         */
        HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_get_ramdump_rsp: RAMDUMP: Requesting next segment of data...");
        nfc_hal_nci_invoke_ramdump_upload();
    }
    else
    {
        /***
         * Segments complete! Process the data.
         * Merge the ramdump data into the output file.
         */
        nfc_hal_nci_parse_and_dump_ramdump_data();

        /***
         * Are there further ram dumps to upload?
         */
        if (gRamdump_Data.cd_current_item->next != NULL)
        {
        /***
         * Start the upload process again
         */
            gRamdump_Data.total_received = 0;
            gRamdump_Data.cd_current_item = gRamdump_Data.cd_current_item->next;
            HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_get_ramdump_rsp: RAMDUMP: Requesting first segment of data for next config file entry");
            nfc_hal_nci_invoke_ramdump_upload();
        }
        else
        {
            /***
             * No more dumps to upload. Initiate end of session
             */
            HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_get_ramdump_rsp: RAMDUMP: Obtained all ramdump data. Sending NFCC PROP_END_RAMDUMP_CMD");
            nfc_hal_dm_send_prop_end_ramdump_cmd();
        }
    }
}

/*******************************************************************************
**
** Function         nfc_hal_nci_recv_end_ramdump_rsp
**
** Description      Perform action following receiving PROP_END_RAMDUMP_RSP notification from NFCC.
**
** Returns          none
**
*******************************************************************************/
void nfc_hal_nci_recv_end_ramdump_rsp()
{
    HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_end_ramdump_rsp: RAMDUMP: Ending ramdump and restarting services");

    /* Indicate Ramdump complete by cleaning up */
    nfc_hal_nci_close_ramdump();

    /*
     * Power cycle the NFC Service.
     * Start pre-initializing NFCC.
     */
    HAL_TRACE_DEBUG0 ("nfc_hal_nci_recv_end_ramdump_rsp: RAMDUMP: Ending ramdump cycle. Enabling PowerCycle");
    /* Reset initialisation variables. Should be part of power cycle routine! */
    nfc_hal_cb.dev_cb.patch_applied = FALSE;
    nfc_hal_cb.dev_cb.pre_init_done = FALSE;
    nfc_hal_cb.dev_cb.pre_patch_signature_chk = FALSE;
    nfc_hal_cb.dev_cb.patch_signature_chk = FALSE;
    nfc_hal_cb.dev_cb.pre_patch_applied = FALSE;
    nfc_hal_cb.dev_cb.pre_patch_file_available = FALSE;
    nfc_hal_cb.dev_cb.fw_version_chk = FALSE;

    DT_Nfc_RamdumpPerformed = TRUE;
    GKI_send_event (NFC_HAL_TASK, NFC_HAL_TASK_EVT_POWER_CYCLE);
}

/*******************************************************************************
**
** Function         nfc_hal_nci_parse_ramdump_config_create_dump_area
**
** Description      Looks into the config buffer and pulls out the START addrs and LENGTHs.
**
** Returns          Largest buffer length
**
*******************************************************************************/
static BOOLEAN nfc_hal_nci_parse_ramdump_config_create_dump_area(int buf_len)
{
    HAL_TRACE_DEBUG1 ("nfc_hal_nci_parse_ramdump_config_create_dump_area: RAMDUMP: Extracting config data and creating data buffers, buffer size %d", buf_len);
    /**
     *
     * int GetStrValue(const char* name, char* p_value, unsigned long len);
     * int GetNumValue(const char* name, void* p_value, unsigned long len);
     *
     * Format of a line from the config file, e.g. 'NAME “P1 stack” START 0x10000 LENGTH 512'
     */
    static const char kRamdump_config_line_format[] = "%*[\t\r\n ]NAME %s START 0x%x LENGTH %d%n";
    static const char kRamdump_config_alt1_line_format[] = "NAME %s START 0x%x LENGTH %d%n";
    static const char kRamdump_config_alt2_line_format[] = "NAME %s START 0x%x LENGTH %d%n%*[\t\r\n ]";
    static const char kRamdump_config_alt3_line_format[] = "%*[\t\r\n ]NAME %s START 0x%x LENGTH %d%n%*[\t\r\n ]";
    static const char kRamdump_config_alt4_line_format[] = "%*[\t\r\n0 ]";
    /*
     * We test against this to determine if we want to parse anymore. Should never get data less than this length as a START address will always be at least 8 digits (4 bytes).
     * The most we can expect from this string is six digits;
     * I.e. 1 char from NAME (if name is one char), 3 chars from LENGTH (if length is one digit). Add these to 2 chars from START, gives us 6 digits for an address!
     */
    int format_len = strlen(kRamdump_config_alt1_line_format) + 2; /* 6 digits + 2 = 8 digits for the START address */

    const int kExpected_config_inputs = 3;
    BOOLEAN error = FALSE;
    int items_filled = 0;
    int max_length = 0;
    int buffer_offset = 0;
    ConfigData** cd_pptr = &gRamdump_Data.cd_list;
    *cd_pptr == NULL;
#ifdef RAMDUMP
    int count = 0;
#endif
    /* Begin parsing */
    do
    {
        BOOLEAN parsed_something = TRUE;
        unsigned int dump_start_addr = 0;
        int dump_length = 0;
        char name[MAX_CONFIG_NAME];
#ifdef RAMDUMP
        HAL_TRACE_DEBUG0 ("-------------------");
        HAL_TRACE_DEBUG1 ("%s\n", gRamdump_Data.config_buffer + buffer_offset);
        HAL_TRACE_DEBUG0 ("-------------------");
#endif
        int bytes_read = 0;
        int matched_scan = 1;
        if ((items_filled = sscanf ((const char *)gRamdump_Data.config_buffer + buffer_offset, kRamdump_config_line_format, &name[0], &dump_start_addr, &dump_length, &bytes_read)) <= 0)
        {
            ++matched_scan;
            if ((items_filled = sscanf ((const char *)gRamdump_Data.config_buffer + buffer_offset, kRamdump_config_alt1_line_format, &name[0], &dump_start_addr, &dump_length, &bytes_read)) <= 0)
            {
                ++matched_scan;
                if ((items_filled = sscanf ((const char *)gRamdump_Data.config_buffer + buffer_offset, kRamdump_config_alt2_line_format, &name[0], &dump_start_addr, &dump_length, &bytes_read)) <= 0)
                {
                    ++matched_scan;
                    if ((items_filled =	sscanf ((const char *)gRamdump_Data.config_buffer + buffer_offset, kRamdump_config_alt3_line_format, &name[0], &dump_start_addr, &dump_length, &bytes_read)) <= 0)
                    {
                        ++matched_scan;
                        if ((items_filled =	sscanf ((const char *)gRamdump_Data.config_buffer + buffer_offset, kRamdump_config_alt4_line_format)) <= 0)
                        {
                            if (items_filled <= 0 || items_filled == EOF)
                            {
                                HAL_TRACE_DEBUG0("nfc_hal_nci_parse_ramdump_config_create_dump_area: RAMDUMP: can not parse rest of file");
                                parsed_something = FALSE;
                            }
                        }
                    }
                }
            }
        }
        buffer_offset += bytes_read;
        if (parsed_something)
        {
            HAL_TRACE_DEBUG3 ("*** Bytes read %d, Buffer length %d, Buffer offset %d", bytes_read, buf_len, buffer_offset);
            HAL_TRACE_DEBUG3 ("*** Found string: \"NAME %s START 0x%X LENGTH %d\"", name, dump_start_addr, dump_length);
            HAL_TRACE_DEBUG1 ("*** Using scan pattern %d", matched_scan);
            if (dump_length != 0)
            {
                /* obtain largest buffer length */
                max_length = (dump_length > max_length) ? dump_length : max_length;
                /* create the node */
                HAL_TRACE_DEBUG0("Creating config node...");
                if ((*cd_pptr = (ConfigData*) malloc(sizeof(ConfigData))) == NULL)
                {
                    HAL_TRACE_DEBUG0 ("nfc_hal_nci_parse_ramdump_config_create_dump_area: **ERROR** RAMDUMP: Unable to create place holder for buffer");
                    error = TRUE;
                }
                if (!error)
                {
                    (*cd_pptr)->dump_start_addr = dump_start_addr;
                    (*cd_pptr)->dump_length = dump_length;
                    memcpy((*cd_pptr)->name, name, MAX_CONFIG_NAME);
                    HAL_TRACE_DEBUG0("nfc_hal_nci_parse_ramdump_config_create_dump_area: RAMDUMP: Inserting node into config list");
                    /* tail */
                    (*cd_pptr)->next = NULL;
                    cd_pptr = &((*cd_pptr)->next); /* the reason we use a **, make the inserted node's next ptr the head */

#ifdef RAMDUMP
                    ++count;
#endif
                }
            }
            else
            {
                HAL_TRACE_DEBUG0("nfc_hal_nci_parse_ramdump_config_create_dump_area: RAMDUMP: **WARNING** Zero LENGTH found. CFG is not well formed! Ignoring entry...");
            }
        }
    } while (!error && (items_filled == kExpected_config_inputs) && ((buf_len-buffer_offset) >= format_len));
    /*
     * Check for config errors, a config with only a zero LENGTH entry or a comment.
     * We do this because if we send an INIT CMD to the NFCC and we have no nodes, the NFCC is in 'RAM Dump Requested' state, and
     * we can not end the sequence as the design states the NFCC will only accept ramdump GETs.
     * However, we also can not send GETs with zero LENGTHS!
     */
    if (gRamdump_Data.cd_list == NULL)
    {
        HAL_TRACE_DEBUG0("nfc_hal_nci_parse_ramdump_config_create_dump_area: RAMDUMP: No config nodes created!");
        error = TRUE;
    }
#ifdef RAMDUMP
    if (!error)
    {
        ConfigData* node_ptr = gRamdump_Data.cd_list;
        int new_count = 0;
        while(node_ptr != NULL)
        {
            node_ptr = node_ptr->next;
            ++new_count;
        }
        if (new_count != count)
        {
            HAL_TRACE_DEBUG2("nfc_hal_nci_parse_ramdump_config_create_dump_area: RAMDUMP: **ERROR** Nodes count do not match (expected %d, got %d)", count, new_count);
        }
    }
#endif
    /* create the largest payload data buffer */
    if (!error && (gRamdump_Data.data = (UINT8*) malloc(max_length)) == NULL)
    {
        HAL_TRACE_DEBUG1 ("nfc_hal_nci_parse_ramdump_config_create_dump_area: RAMDUMP: **ERROR** Unable to malloc ramdump dump buffer of %d bytes", max_length);
        error = TRUE;
    }
    else
    {
        gRamdump_Data.data_length = max_length;
    }
    /* We no longer need the config memory area */
    free(gRamdump_Data.config_buffer);
    gRamdump_Data.config_buffer = NULL;
    /* if any error, say on the nth + 1 created node, we cleanup later */
    return error;
}

/*******************************************************************************
**
** Function         nfc_hal_nci_parse_and_dump_ramdump_register_data
**
** Description      Looks into the register buffer and merges the data into the required format into the output file.
**
** Returns          none
**
*******************************************************************************/
void nfc_hal_nci_parse_and_dump_ramdump_register_data()
{
    HAL_TRACE_DEBUG0 ("nfc_hal_nci_parse_and_dump_ramdump_register_data: RAMDUMP: Dump the register data to output");
    /*
     * We know the registers and their order from the standard. If the length changes in any way we are out of sync.
     * When we come to output this register data we depend on the standard's definition for the correlation.
     * At runtime we can only ensure we have the correct amount of data. Ouput what we have.
     *
     * Format of a line from the register file, e.g. 'R0 0x%x |' the continuation bar is replaced by newline for the last entry.
     * Note: Before printing your hex values you must arrange them into the correct Endianness
     */
    static const char kRamdump_register_dump_format[] =	"%s 0x%02x%02x%02x%02x\n";
#define MAX_REGISTER_COUNT 50
    char* kRamdump_register_names[MAX_REGISTER_COUNT] = {"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10", "R11", "R12", "MSP", "PSP", "LR", "PC", "xPSR"};
    if(gNfccChipVersion == 24)
	{
        kRamdump_register_names[18] = "HARD_FAULT_LR";
        kRamdump_register_names[19] = "PRIMASK";
        kRamdump_register_names[20] = "CONTROL";
    }
    else
    {
        /* gNfccChipVersion == 21 - default case, collect minimum */
        kRamdump_register_names[18] = "PRIMASK";
        kRamdump_register_names[19] = "CONTROL";
    }

    int buffer_offset = 0;
    int i = 0;
    for (; i < gMaxRegisterCount; ++i)
    {
        /* register_buffer is a UINT8*. Cast the ptr as an int* and use ptr arithmetic to obtain the offset */
        if (fprintf (gRamdump_Data.output_file_ptr, kRamdump_register_dump_format, kRamdump_register_names[i], gRamdump_Data.register_buffer[buffer_offset+3],
                                                                                                               gRamdump_Data.register_buffer[buffer_offset+2],
                                                                                                               gRamdump_Data.register_buffer[buffer_offset+1],
                                                                                                               gRamdump_Data.register_buffer[buffer_offset]) < 0)
        {
            HAL_TRACE_DEBUG1 ("nfc_hal_nci_parse_and_dump_ramdump_register_data: RAMDUMP: Problem writing register data for register \"%s\" to output file", kRamdump_register_names[i]);
        }
        buffer_offset += 4;
    }
    if (fflush (gRamdump_Data.output_file_ptr) != 0)
    {
        HAL_TRACE_DEBUG0 ("nfc_hal_nci_parse_and_dump_ramdump_register_data: RAMDUMP: Problem flushing output file"); /**TBD IF THERE IS NO DATA WRITEN IS THAT AN ERROR CONDITION???? SHOULDNT BE */
    }
}

/*******************************************************************************
**
** Function         nfc_hal_nci_parse_and_dump_ramdump_data
**
** Description      For the current dump, looks into the data buffer and merges the data into the required format into the output file.
**
** Returns          none
**
*******************************************************************************/
void nfc_hal_nci_parse_and_dump_ramdump_data()
{
    HAL_TRACE_DEBUG0 ("nfc_hal_nci_parse_and_dump_ramdump_data: RAMDUMP: Dump ramdump data to file");
    /*
     * Format of a line from the ramdump file, e.g. 'NAME \"%s\" START 0x%x DATA 0x%x'
     * Note: Before printing your hex values you must arrange them into the correct Endianness
     */
    if (fprintf (gRamdump_Data.output_file_ptr, "NAME \"%s\" START 0x%x DATA 0x", gRamdump_Data.cd_current_item->name, gRamdump_Data.cd_current_item->dump_start_addr) < 0)
    {
        HAL_TRACE_DEBUG1 ("nfc_hal_nci_parse_and_dump_ramdump_data: RAMDUMP: Problem writing dump header for NAME \"%s\" to output file", gRamdump_Data.cd_current_item->name);
    }
    /* followed by the data */
    int i = 0;
    for (; i < gRamdump_Data.cd_current_item->dump_length; ++i)
    {
        if (fprintf (gRamdump_Data.output_file_ptr, "%02x", gRamdump_Data.data[i]) < 0)
        {
            HAL_TRACE_DEBUG1 ("nfc_hal_nci_parse_and_dump_ramdump_data: RAMDUMP: Problem writing dump data for NAME \"%s\" to output file", gRamdump_Data.cd_current_item->name);
        }
    }
    /* Lets separate our data entries one per line */
    if (fprintf (gRamdump_Data.output_file_ptr, "\n") < 0)
    {
        HAL_TRACE_DEBUG1 ("nfc_hal_nci_parse_and_dump_ramdump_register_data: RAMDUMP: Problem writing terminal for register data name %s to output file", gRamdump_Data.cd_current_item->name);
    }
    if (fflush (gRamdump_Data.output_file_ptr) != 0)
    {
        HAL_TRACE_DEBUG0 ("nfc_hal_nci_parse_and_dump_ramdump_data: RAMDUMP: Problem flushing output file");
    }
}

/*******************************************************************************
**
** Function         nfc_hal_nci_close_ramdump
**
** Description      Housekeeping function to tidy data and files.
**
** Returns          none
**
*******************************************************************************/
void nfc_hal_nci_close_ramdump()
{
    HAL_TRACE_DEBUG0 ("nfc_hal_nci_close_ramdump: RAMDUMP: Performing cleanup");
    if (gRamdump_Data.ramdump_in_progress)
    {
        if (gRamdump_Data.output_file_ptr && fclose (gRamdump_Data.output_file_ptr) == EOF)
        {
            HAL_TRACE_DEBUG0 ("nfc_hal_nci_close_ramdump: RAMDUMP: Unable to close ramdump file. Continuing...");
        }
        else
        {
            HAL_TRACE_DEBUG0 ("nfc_hal_nci_close_ramdump: RAMDUMP: ramdump file closed.");
        }
        /* house keeping */
        if (gRamdump_Data.data != NULL)
        {
            HAL_TRACE_DEBUG0 ("nfc_hal_nci_close_ramdump: RAMDUMP: freeing gRamdump_Data.data");
            free (gRamdump_Data.data);
            gRamdump_Data.data = NULL;
        }
        /* release the config data linked list */
        if (gRamdump_Data.cd_list != NULL)
        {
            HAL_TRACE_DEBUG0 ("nfc_hal_nci_close_ramdump: RAMDUMP: freeing gRamdump_Data.cd_list");
            ConfigData* node = gRamdump_Data.cd_list;
            while (node != NULL)
            {
                ConfigData* next_node = node->next;
                HAL_TRACE_DEBUG2("freeing config node '%s', length %d...", node->name, node->dump_length);
                free(node);
                node = NULL;
                node = next_node;
            }
        }
        /* Depending on when we are called, this part may already be freed */
        if (gRamdump_Data.register_buffer != NULL)
        {
            HAL_TRACE_DEBUG0 ("nfc_hal_nci_close_ramdump: RAMDUMP: freeing gRamdump_Data.register_buffer");
            free (gRamdump_Data.register_buffer);
            gRamdump_Data.register_buffer = NULL;
        }
        /* Depending on when we are called, this part may already be freed */
        if (gRamdump_Data.config_buffer != NULL)
        {
            HAL_TRACE_DEBUG0 ("nfc_hal_nci_close_ramdump: RAMDUMP: freeing gRamdump_Data.config_buffer");
            free(gRamdump_Data.config_buffer);
            gRamdump_Data.config_buffer = NULL;
        }
    }
    /* Zero out. May not be last ram dump */
    memset(&gRamdump_Data, 0, sizeof(RamDump_Data));
    HAL_TRACE_DEBUG0 ("nfc_hal_nci_close_ramdump: RAMDUMP: Cleanup complete!");
}
