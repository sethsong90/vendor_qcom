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
 * We use NDEBUG to hide some ramdump debug traces and asserts. We use RAMDUMP to output some additional nformation.
 * Note: ramdump is not a production level feature.
 */
#ifdef NDEBUG
#undef NDEBUG
#endif

#ifndef RAMDUMP
#define RAMDUMP
#endif

/******************************************************************************
 *
 *  Functions for handling NFC HAL NCI Transport events
 *
 ******************************************************************************/
#include <string.h>
#include "nfc_hal_int.h"
#include "nfc_hal_post_reset.h"
#include "userial.h"

#include <DT_Nfc_link.h>
#include <DT_Nfc_types.h>
#include <DT_Nfc_status.h>
#include <DT_Nfc_i2c.h>
#include <DT_Nfc_log.h>
#include <DT_Nfc.h>
#include <time.h>

#ifdef DTA
#include "dta_flag.h"
#endif // </DTA>

/****************************************************************************
** Definitions
****************************************************************************/

/* Default NFC HAL NCI port configuration  */
NFC_HAL_TRANS_CFG_QUALIFIER tNFC_HAL_TRANS_CFG nfc_hal_trans_cfg =
{
    NFC_HAL_SHARED_TRANSPORT_ENABLED,   /* bSharedTransport */
    USERIAL_BAUD_115200,                /* Baud rate */
    USERIAL_FC_HW                       /* Flow control */
};

static DT_Nfc_sConfig_t             gDrvCfg;
void                                *gHwRef;
/* Control block for NFC HAL NCI transport */
#if NFC_DYNAMIC_MEMORY == FALSE
tNFC_HAL_CB nfc_hal_cb;
#endif

extern tNFC_HAL_CFG *p_nfc_hal_cfg;
/*semaphore to keep track the sleep response before shutdown*/
sem_t semaphore_sleepcmd_complete;
BOOLEAN is_vsc_prop_sleep_cmd = FALSE;
/****************************************************************************
** Internal function prototypes
****************************************************************************/
static void nfc_hal_main_userial_cback (tUSERIAL_PORT port, tUSERIAL_EVT evt, tUSERIAL_EVT_DATA *p_data);
static void nfc_hal_main_handle_terminate (void);
static void nfc_hal_main_timeout_cback (void *p_tle);
static BOOLEAN isRamDumpCmd(NFC_HDR *p_msg);

#if (NFC_HAL_DEBUG == TRUE)
const char * const nfc_hal_init_state_str[] =
{
    "IDLE",             /* Initialization is done                */
    "W4_XTAL_SET",      /* Waiting for crystal setting rsp       */
    "W4_POST_XTAL_SET", /* Waiting for crystal reset ntf         */
    "W4_NFCC_ENABLE",   /* Waiting for reset ntf atter REG_PU up */
    "W4_BUILD_INFO",    /* Waiting for build info rsp            */
    "W4_PATCH_INFO",    /* Waiting for patch info rsp            */
    "W4_APP_COMPL",     /* Waiting for complete from application */
    "W4_POST_INIT",     /* Waiting for complete of post init     */
    "W4_CONTROL",       /* Waiting for control release           */
    "W4_PREDISC",       /* Waiting for complete of prediscover   */
    "CLOSING",          /* Shutting down                         */
    "PATCH_DOWNLOAD",   /* Waiting for patch download            */
    "W4_RESET_INIT",    /* Waiting for reset rsp                 */
    "RAMDUMP",          /* Ramdump in progress                   */
};
#endif

/*******************************************************************************
**
** Function         nfc_hal_main_init
**
** Description      This function initializes control block for NFC HAL
**
** Returns          nothing
**
*******************************************************************************/
void nfc_hal_main_init (void)
{
    /* Clear control block */
    memset (&nfc_hal_cb, 0, sizeof (tNFC_HAL_CB));

    nfc_hal_cb.ncit_cb.nci_ctrl_size   = NFC_HAL_NCI_INIT_CTRL_PAYLOAD_SIZE;
    nfc_hal_cb.trace_level             = NFC_HAL_INITIAL_TRACE_LEVEL;
    nfc_hal_cb.timer.p_cback           = nfc_hal_main_timeout_cback;
    nfc_hal_cb.dev_cb.nfcc_sleep_mode  = WAKE_STATE;
    nfc_hal_cb.ncit_cb.hw_error         = FALSE;
}

/*******************************************************************************
**
** Function         nfc_hal_main_open_transport
**
** Description      Open transport and prepare for new incoming message;
**
** Returns          nothing
**
*******************************************************************************/
static void nfc_hal_main_open_transport (void)
{
    tUSERIAL_OPEN_CFG open_cfg;
    NFC_RETURN_CODE open_result = NFC_SUCCESS;

    /* Initialize control block */
    nfc_hal_cb.ncit_cb.rcv_state = NFC_HAL_RCV_IDLE_ST; /* to process packet type */

    if (nfc_hal_cb.ncit_cb.p_rcv_msg)
    {
        GKI_freebuf (nfc_hal_cb.ncit_cb.p_rcv_msg);
        nfc_hal_cb.ncit_cb.p_rcv_msg = NULL;
    }

    /* open transport */
    open_cfg.fmt    = (USERIAL_DATABITS_8 | USERIAL_PARITY_NONE | USERIAL_STOPBITS_1);
    open_cfg.baud   = nfc_hal_trans_cfg.userial_baud;
    open_cfg.fc     = nfc_hal_trans_cfg.userial_fc;
    open_cfg.buf    = USERIAL_BUF_BYTE;


    gDrvCfg.nRef    = 1; //dummy value as not used here
    gDrvCfg.devFile = "/dev/nfc-nci";
    gDrvCfg.phyType = ENUM_LINK_TYPE_I2C;
    if(gHwRef== NULL){
        gHwRef= (void *)malloc(4024);
    }
    open_result = DT_Nfc_Open(&gDrvCfg, &gHwRef, &nfc_hal_main_userial_cback);
    if (open_result == NFC_SUCCESS){
       /* notify transport opened */
       nfc_hal_dm_pre_init_nfcc ();
    }
    else if (open_result == NFC_NO_HW){
       HAL_TRACE_ERROR0 ("DT_Nfc_Open: NO NFCC hardware found");
       nfc_hal_cb.ncit_cb.hw_error = TRUE;
       nfc_hal_main_send_error (HAL_NFC_STATUS_ERR_TRANSPORT);
    }
    else{
       HAL_TRACE_ERROR0 ("DT_Nfc_Open: Fails");
       nfc_hal_cb.ncit_cb.hw_error = TRUE;
       nfc_hal_main_pre_init_done (HAL_NFC_STATUS_FAILED);
    }
}

/*******************************************************************************
**
** Function         nfa_hal_pre_discover_done_cback
**
** Description      Pre-discovery CFG is sent.
**
** Returns          nothing
**
*******************************************************************************/
void nfa_hal_pre_discover_done_cback (tNFC_HAL_NCI_EVT event, UINT16 data_len, UINT8 *p_data)
{
    NFC_HAL_SET_INIT_STATE(NFC_HAL_INIT_STATE_IDLE);
    nfc_hal_main_stop_quick_timer (&nfc_hal_cb.ncit_cb.nci_wait_rsp_timer);
    nfc_hal_cb.p_stack_cback (HAL_NFC_PRE_DISCOVER_CPLT_EVT, HAL_NFC_STATUS_OK);
}

/*******************************************************************************
**
** Function         nfa_hal_send_pre_discover_cfg
**
** Description      sending Pre-discovery CFG
**
** Returns          nothing
**
*******************************************************************************/
void nfa_hal_send_pre_discover_cfg (void)
{
    if (nfc_hal_dm_set_config (p_nfc_hal_pre_discover_cfg [0],
                               &p_nfc_hal_pre_discover_cfg[1],
                                nfa_hal_pre_discover_done_cback) != HAL_NFC_STATUS_OK)
    {
        nfa_hal_pre_discover_done_cback(0, 0, NULL);
    }
}

/*******************************************************************************
**
** Function         nfc_hal_main_send_error
**
** Description      send an Error event to NFC stack
**
** Returns          nothing
**
*******************************************************************************/
void nfc_hal_main_send_error (tHAL_NFC_STATUS status)
{
    /* Notify stack */
    nfc_hal_cb.p_stack_cback(HAL_NFC_ERROR_EVT, status);
}

/*******************************************************************************
**
** Function         nfc_hal_main_userial_cback
**
** Description      USERIAL callback for NCI transport
**
** Returns          nothing
**
*******************************************************************************/
static void nfc_hal_main_userial_cback (tUSERIAL_PORT port, tUSERIAL_EVT evt, tUSERIAL_EVT_DATA *p_data)
{
    if (evt == USERIAL_RX_READY_EVT)
    {
        /* Notify transport task of serial port event */
        GKI_send_event (NFC_HAL_TASK, NFC_HAL_TASK_EVT_DATA_RDY);
    }
    else if (evt == USERIAL_TX_DONE_EVT)
    {
        /* Serial driver has finshed sending data from USERIAL_Write */
        /* Currently, no action is needed for this event */
    }
    else if (evt == USERIAL_ERR_EVT)
    {
        HAL_TRACE_ERROR0 ("nfc_hal_main_userial_cback: USERIAL_ERR_EVT. Notifying NFC_TASK of transport error");
        if (nfc_hal_cb.ncit_cb.nci_wait_rsp != NFC_HAL_WAIT_RSP_NONE)
        {
            nfc_hal_main_stop_quick_timer (&nfc_hal_cb.ncit_cb.nci_wait_rsp_timer);
            nfc_hal_nci_cmd_timeout_cback ((void *)&nfc_hal_cb.ncit_cb.nci_wait_rsp_timer);
        }
        else
        {
            nfc_hal_main_send_error (HAL_NFC_STATUS_ERR_TRANSPORT);
        }
    }
    else if (evt == USERIAL_WAKEUP_EVT)
    {
        HAL_TRACE_DEBUG1 ("nfc_hal_main_userial_cback: USERIAL_WAKEUP_EVT: %d", p_data->sigs);
    }
    else
    {
        HAL_TRACE_DEBUG1 ("nfc_hal_main_userial_cback: unhandled userial evt: %i", evt);
    }
}

/*******************************************************************************
**
** Function         nfc_hal_main_pre_init_done
**
** Description      notify complete of pre-initialization
**
** Returns          nothing
**
*******************************************************************************/
void nfc_hal_main_pre_init_done (tHAL_NFC_STATUS status)
{
    HAL_TRACE_DEBUG1 ("nfc_hal_main_pre_init_done () status = %d", status);

    if (status != HAL_NFC_STATUS_OK)
    {
        nfc_hal_main_handle_terminate ();


        gDrvCfg.nRef        = 0;
        gDrvCfg.devFile     = "/dev/nfc-nci";
        gDrvCfg.phyType     = ENUM_LINK_TYPE_NONE;
        DT_Nfc_Close(&gDrvCfg);
    }

    /* Notify NFC Task the status of initialization */
    nfc_hal_cb.p_stack_cback (HAL_NFC_OPEN_CPLT_EVT, status);
}

/*******************************************************************************
**
** Function         nfc_hal_main_timeout_cback
**
** Description      callback function for timeout
**
** Returns          void
**
*******************************************************************************/
static void nfc_hal_main_timeout_cback (void *p_tle)
{
    TIMER_LIST_ENT  *p_tlent = (TIMER_LIST_ENT *) p_tle;

    HAL_TRACE_DEBUG0 ("nfc_hal_main_timeout_cback ()");

    switch (p_tlent->event)
    {
    case NFC_HAL_TTYPE_POWER_CYCLE:
        nfc_hal_main_open_transport ();
        break;

    case NFC_HAL_TTYPE_NFCC_ENABLE:
        /* NFCC should have enabled now, notify transport openned */
        nfc_hal_dm_pre_init_nfcc ();
        break;

    default:
        HAL_TRACE_DEBUG1 ("nfc_hal_main_timeout_cback: unhandled timer event (0x%04x)", p_tlent->event);
        break;
    }
}

/*******************************************************************************
**
** Function         nfc_hal_main_handle_terminate
**
** Description      Handle NFI transport shutdown
**
** Returns          nothing
**
*******************************************************************************/
static void nfc_hal_main_handle_terminate (void)
{
    NFC_HDR *p_msg;

    /* dequeue and free buffer */
    if (nfc_hal_cb.ncit_cb.p_pend_cmd != NULL)
    {
        GKI_freebuf (nfc_hal_cb.ncit_cb.p_pend_cmd);
        nfc_hal_cb.ncit_cb.p_pend_cmd = NULL;
    }

    /* Free unsent nfc rx buffer */
    if (nfc_hal_cb.ncit_cb.p_rcv_msg)
    {
        GKI_freebuf (nfc_hal_cb.ncit_cb.p_rcv_msg);
        nfc_hal_cb.ncit_cb.p_rcv_msg  = NULL;
    }

    /* Free buffer for pending fragmented response/notification */
    if (nfc_hal_cb.ncit_cb.p_frag_msg)
    {
        GKI_freebuf (nfc_hal_cb.ncit_cb.p_frag_msg);
        nfc_hal_cb.ncit_cb.p_frag_msg = NULL;
    }

    /* Free buffers in the tx mbox */
    while ((p_msg = (NFC_HDR *) GKI_read_mbox (NFC_HAL_TASK_MBOX)) != NULL)
    {
        GKI_freebuf (p_msg);
    }

    /* notify closing transport */
    nfc_hal_dm_shutting_down_nfcc ();
}

/*******************************************************************************
**
** Function         nfc_hal_main_start_quick_timer
**
** Description      Start a timer for the specified amount of time.
**                  NOTE: The timeout resolution depends on including modules.
**                  QUICK_TIMER_TICKS_PER_SEC should be used to convert from
**                  time to ticks.
**
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_main_start_quick_timer (TIMER_LIST_ENT *p_tle, UINT16 type, UINT32 timeout)
{
    NFC_HDR *p_msg;

    /* if timer list is currently empty, start periodic GKI timer */
    if (nfc_hal_cb.quick_timer_queue.p_first == NULL)
    {
        /* if timer starts on other than NCIT task (script wrapper) */
        if(GKI_get_taskid () != NFC_HAL_TASK)
        {
            /* post event to start timer in NCIT task */
            if ((p_msg = (NFC_HDR *) GKI_getbuf (NFC_HDR_SIZE)) != NULL)
            {
                p_msg->event = NFC_HAL_EVT_TO_START_QUICK_TIMER;
                GKI_send_msg (NFC_HAL_TASK, NFC_HAL_TASK_MBOX, p_msg);
            }
        }
        else
        {
            GKI_start_timer (NFC_HAL_QUICK_TIMER_ID, ((GKI_SECS_TO_TICKS (1) / QUICK_TIMER_TICKS_PER_SEC)), TRUE);
        }
    }

    GKI_remove_from_timer_list (&nfc_hal_cb.quick_timer_queue, p_tle);

    p_tle->event = type;
    p_tle->ticks = timeout; /* Save the number of ticks for the timer */

    GKI_add_to_timer_list (&nfc_hal_cb.quick_timer_queue, p_tle);
}

/*******************************************************************************
**
** Function         nfc_hal_main_stop_quick_timer
**
** Description      Stop a timer.
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_main_stop_quick_timer (TIMER_LIST_ENT *p_tle)
{
    GKI_remove_from_timer_list (&nfc_hal_cb.quick_timer_queue, p_tle);

    /* if timer list is empty stop periodic GKI timer */
    if (nfc_hal_cb.quick_timer_queue.p_first == NULL)
    {
        GKI_stop_timer (NFC_HAL_QUICK_TIMER_ID);
    }
}

/*******************************************************************************
**
** Function         nfc_hal_main_process_quick_timer_evt
**
** Description      Process quick timer event
**
** Returns          void
**
*******************************************************************************/
static void nfc_hal_main_process_quick_timer_evt (void)
{
    TIMER_LIST_ENT  *p_tle;

    GKI_update_timer_list (&nfc_hal_cb.quick_timer_queue, 1);

    while ((nfc_hal_cb.quick_timer_queue.p_first) && (!nfc_hal_cb.quick_timer_queue.p_first->ticks))
    {
        p_tle = nfc_hal_cb.quick_timer_queue.p_first;
        GKI_remove_from_timer_list (&nfc_hal_cb.quick_timer_queue, p_tle);

        if (p_tle->p_cback)
        {
            (*p_tle->p_cback) (p_tle);
        }
    }

    /* if timer list is empty stop periodic GKI timer */
    if (nfc_hal_cb.quick_timer_queue.p_first == NULL)
    {
        GKI_stop_timer (NFC_HAL_QUICK_TIMER_ID);
    }
}

/*******************************************************************************
**
** Function         nfc_hal_send_nci_msg_to_nfc_task
**
** Description      This function is called to send nci message to nfc task
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_send_nci_msg_to_nfc_task (NFC_HDR * p_msg)
{
#ifdef NFC_HAL_SHARED_GKI
    /* Using shared NFC/HAL GKI resources - send message buffer directly to NFC_TASK for processing */
    p_msg->event = BT_EVT_TO_NFC_NCI;
    GKI_send_msg (NFC_TASK, NFC_MBOX_ID, p_msg);
#else
    /* Send NCI message to the stack */
    nfc_hal_cb.p_data_cback (p_msg->len, (UINT8 *) ((p_msg + 1)
                                 + p_msg->offset));
    GKI_freebuf(p_msg);
#endif
}

/*******************************************************************************
**
** Function         nfc_hal_send_credit_ntf_for_cid
**
** Description      This function is called to send credit ntf
**                  for the specified connection id to nfc task
**
** Returns          void
**
*******************************************************************************/
static void nfc_hal_send_credit_ntf_for_cid (UINT8 cid)
{
    NFC_HDR  *p_msg;
    UINT8    *p, *ps;

    /* Start of new message. Allocate a buffer for message */
    if ((p_msg = (NFC_HDR *) GKI_getpoolbuf (NFC_HAL_NCI_POOL_ID)) != NULL)
    {
        /* Initialize NFC_HDR */
        p_msg->len    = NCI_DATA_HDR_SIZE + 0x03;
        p_msg->event  = 0;
        p_msg->offset = 0;
        p_msg->layer_specific = 0;

        p = (UINT8 *) (p_msg + 1) + p_msg->offset;
        ps = p;
        NCI_MSG_BLD_HDR0(p, NCI_MT_NTF, NCI_GID_CORE);
        NCI_MSG_BLD_HDR1(p, NCI_MSG_CORE_CONN_CREDITS);
        UINT8_TO_STREAM (p, 0x03);

        /* Number of credit entries */
        *p++ = 0x01;
        /* Connection id of the credit ntf */
        *p++ = cid;
        /* Number of credits */
        *p = 0x01;
#ifdef DISP_NCI
        DISP_NCI (ps, (UINT16) p_msg->len, TRUE);
#endif
        nfc_hal_send_nci_msg_to_nfc_task (p_msg);
    }
    else
    {
        HAL_TRACE_ERROR0 ("Unable to allocate buffer for Sending credit ntf to stack");
    }
}

/*******************************************************************************
**
** Function         nfc_hal_main_send_message
**
** Description      This function is calledto send an NCI message.
**
** Returns          void
**
*******************************************************************************/
static void nfc_hal_main_send_message (NFC_HDR *p_msg)
{
    UINT8   *ps, *pp, cid, pbf;
    UINT16  len = p_msg->len;
    UINT16  data_len;
#ifdef DISP_NCI
    UINT8   delta;
#endif

    HAL_TRACE_DEBUG1 ("nfc_hal_main_send_message() ls:0x%x", p_msg->layer_specific);
    if (  (p_msg->layer_specific == NFC_HAL_WAIT_RSP_CMD)
        ||(p_msg->layer_specific == NFC_HAL_WAIT_RSP_VSC)  )
    {
        nfc_hal_nci_send_cmd (p_msg);
    }
    else
    {
        /* NFC task has fragmented the data packet to the appropriate size
         * and data credit is available; just send it */

        /* add NCI packet type in front of message */

        /* send this packet to transport */
        ps = (UINT8 *) (p_msg + 1) + p_msg->offset;
        pp = ps + 1;
#ifdef DISP_NCI
        delta = p_msg->len - len;
        DISP_NCI (ps + delta, (UINT16) (p_msg->len - delta), FALSE);
#endif
        if (nfc_hal_cb.hci_cb.hcp_conn_id)
        {
            NCI_DATA_PRS_HDR(pp, pbf, cid, data_len);
            if (cid == nfc_hal_cb.hci_cb.hcp_conn_id)
            {
                if (nfc_hal_hci_handle_hcp_pkt_to_hc (pp))
                {
                    HAL_TRACE_DEBUG0 ("nfc_hal_main_send_message() - Drop rsp to Fake cmd, Fake credit ntf");
                    GKI_freebuf (p_msg);
                    nfc_hal_send_credit_ntf_for_cid (cid);
                    return;
                }
            }

        }
        /* check low power mode state
        if (nfc_hal_dm_power_mode_execute (NFC_HAL_LP_TX_DATA_EVT))
        {
            USERIAL_Write (USERIAL_NFC_PORT, ps, p_msg->len);
        }
        else
        {
            HAL_TRACE_ERROR0 ("nfc_hal_main_send_message(): drop data in low power mode");
        }*/
        DT_Nfc_Write (USERIAL_NFC_PORT, ps, p_msg->len);
        GKI_freebuf (p_msg);
    }
}

void nfc_hal_main_check_vsc_power_cmd(NFC_HDR *p_msg)
{
    UINT8   *pp = NULL;
    /* Check if it is prop sleep coming from JNI */
    pp = (UINT8 *) (p_msg + 1) + p_msg->offset;
    if((pp[0] == NCI_PROP_CMD) && (pp[1] == NCI_SLEEP_CMD) && (pp[2] == NCI_SLEEP_PL))
    {
        HAL_TRACE_DEBUG0 ("it is VSC sleep from JNI");
        is_vsc_prop_sleep_cmd = TRUE;
    }
}

/*******************************************************************************
**
** Function         nfc_hal_main_task
**
** Description      NFC HAL NCI transport event processing task
**
** Returns          0
**
*******************************************************************************/
UINT32 nfc_hal_main_task (UINT32 param)
{
    UINT16   event;
    UINT8    byte;
    UINT8    num_interfaces;
    UINT8    *p;
    NFC_HDR  *p_msg;
    BOOLEAN  free_msg;
    struct timespec time_sec;
    int sem_status;
    HAL_TRACE_DEBUG0 ("NFC_HAL_TASK started");

    gDrvCfg.nRef    = 0;
    gDrvCfg.devFile = "/dev/nfc-nci";
    gDrvCfg.phyType = ENUM_LINK_TYPE_NONE;
    /* Main loop */
    while (TRUE)
    {
        event = GKI_wait (0xFFFF, 0);

        /* Handle NFC_HAL_TASK_EVT_INITIALIZE (for initializing NCI transport) */
        if (event & NFC_HAL_TASK_EVT_INITIALIZE)
        {
            HAL_TRACE_DEBUG0 ("NFC_HAL_TASK got NFC_HAL_TASK_EVT_INITIALIZE signal. Opening NFC transport...");

            nfc_hal_main_open_transport ();
        }

        /* Check for terminate event */
        if (event & NFC_HAL_TASK_EVT_TERMINATE)
        {
            HAL_TRACE_DEBUG0 ("NFC_HAL_TASK got NFC_HAL_TASK_EVT_TERMINATE");
            if(nfc_hal_cb.init_sleep_done)
            {
                HAL_TRACE_DEBUG0("initializing sem for sleep cmd completeion before shutdown \n");
                /*this semaphore is used to keep track of the sleep rsp before closing the transport*/
                if(sem_init(&semaphore_sleepcmd_complete, 0, 0) != 0)
                {
                    HAL_TRACE_DEBUG0("semaphore_sleepcmd_complete creation failed \n");
                }
            }
            nfc_hal_main_handle_terminate ();
            if(nfc_hal_cb.init_sleep_done)
            {
                if (clock_gettime(CLOCK_REALTIME, &time_sec) == -1)
                {
                    HAL_TRACE_DEBUG0("get clock_gettime error");
                }
#ifdef DTA // <DTA>
                if(dta_flag_all) {
                    HAL_TRACE_DEBUG1("waiting for semaphore_sleepcmd_complete %lu\n",time_sec.tv_sec);
                    time_sec.tv_sec += 2; //1 in !DTA
                } else {
#endif // </DTA>
                time_sec.tv_sec += 1;
                HAL_TRACE_DEBUG0("waiting for semaphore_sleepcmd_complete \n");
#ifdef DTA // <DTA>
                }
#endif // </DTA>
                nfc_hal_cb.wait_sleep_rsp = 1;
                sem_status = sem_timedwait(&semaphore_sleepcmd_complete,&time_sec);
                if(sem_status == -1)
                {
                    HAL_TRACE_DEBUG0("sleep command timed out\n");
                }
                nfc_hal_cb.init_sleep_done = 0;
                sem_destroy(&semaphore_sleepcmd_complete);
            }
            DT_Nfc_Close(&gDrvCfg);
            if (nfc_hal_cb.p_stack_cback)
            {
                nfc_hal_cb.p_stack_cback (HAL_NFC_CLOSE_CPLT_EVT, HAL_NFC_STATUS_OK);
                nfc_hal_cb.p_stack_cback = NULL;
            }
            continue;
        }

        /* Check for power cycle event */
        if (event & NFC_HAL_TASK_EVT_POWER_CYCLE)
        {
            HAL_TRACE_DEBUG0 ("NFC_HAL_TASK got NFC_HAL_TASK_EVT_POWER_CYCLE");
            nfc_hal_main_handle_terminate ();
            DT_Nfc_Close(&gDrvCfg);
            /* power cycle timeout */
            nfc_hal_main_start_quick_timer (&nfc_hal_cb.timer, NFC_HAL_TTYPE_POWER_CYCLE,
                                            (NFC_HAL_POWER_CYCLE_DELAY*QUICK_TIMER_TICKS_PER_SEC)/1000);
            continue;
        }

        /* NCI message ready to be sent to NFCC */
        if (event & NFC_HAL_TASK_EVT_MBOX)
        {
            while ((p_msg = (NFC_HDR *) GKI_read_mbox (NFC_HAL_TASK_MBOX)) != NULL)
            {
                free_msg = TRUE;
                switch (p_msg->event & NFC_EVT_MASK)
                {
                case NFC_HAL_EVT_TO_NFC_NCI:
                    HAL_TRACE_DEBUG0 ("nfc_hal_main_task(): Processing NFC_HAL_EVT_TO_NFC_NCI msg");
                    if (nfc_hal_dm_get_nfc_sleep_state())
                    {
                        nfc_hal_dm_set_nfc_wake (NFC_HAL_ASSERT_NFC_WAKE);
                    }
                    if ((nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_RAMDUMP) && !isRamDumpCmd(p_msg))
                    {
                        /* if in ramdump state ignore sending other cmds */
                        HAL_TRACE_DEBUG0 ("nfc_hal_main_task(): RAMDUMP: NFCC reset in progress. Only ramdump commands will be sent. Ignoring...");
                        break; /* out of switch */
                    }
#ifdef RAMDUMP
                    else if (nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_RAMDUMP)
                    {
                        /* If in ramdump state ramdump cmds should not come through this logic! */
                        HAL_TRACE_DEBUG0 ("nfc_hal_main_task(): RAMDUMP: NFCC reset in progress. WARNING: Only ramdump commands should be sent but not through this logic. Something has gone wrong !!!");
                    }
                    else
                    {
                        /* Cmd being sent from upper layers down to NFCC */
                        HAL_TRACE_DEBUG0 ("nfc_hal_main_task(): RAMDUMP: NOT in ramdump state. Sending some command.");
                        isRamDumpCmd(p_msg);
                    }
#endif

                    nfc_hal_main_check_vsc_power_cmd(p_msg);
                    if (is_vsc_prop_sleep_cmd) {
                       if (nfc_hal_cb.dev_cb.nfcc_sleep_mode != SLEEP_STATE) {
                            nfc_hal_cb.dev_cb.nfcc_sleep_mode = SLEEP_STATE;
                        }
                    }
                    nfc_hal_main_send_message (p_msg);
                    /* do not free buffer. NCI VS code may keep it for processing later */
                    if(is_vsc_prop_sleep_cmd)
                    {
                        free_msg = TRUE;
                    }
                    else
                    {
                        free_msg = FALSE;
                    }

                    break;

                case NFC_HAL_EVT_POST_CORE_RESET:
                    NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_IDLE);
                    /* set NCI Control packet size from CORE_INIT_RSP */
                    p = (UINT8 *) (p_msg + 1) + p_msg->offset + NCI_MSG_HDR_SIZE;
                    p += 5;
                    STREAM_TO_UINT8 (num_interfaces, p);
                    p += (num_interfaces + 3);
                    nfc_hal_cb.ncit_cb.nci_ctrl_size = *p;

                    /* start post initialization */
                    nfc_hal_cb.dev_cb.next_dm_config = NFC_HAL_DM_CONFIG_LPTD;
                    nfc_hal_cb.dev_cb.next_startup_vsc = 1;

                    if(nfc_hal_cb.nvm.no_of_updates == 0)
                    {
                        NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_POST_INIT_DONE);
                        nfc_hal_dm_config_nfcc ();
                    }
                    break;

                case NFC_HAL_EVT_TO_START_QUICK_TIMER:
                    GKI_start_timer (NFC_HAL_QUICK_TIMER_ID, ((GKI_SECS_TO_TICKS (1) / QUICK_TIMER_TICKS_PER_SEC)), TRUE);
                    break;

                case NFC_HAL_EVT_HCI:
                    nfc_hal_hci_evt_hdlr ((tNFC_HAL_HCI_EVENT_DATA *) p_msg);
                    break;

                case NFC_HAL_EVT_PRE_DISCOVER:
                    NFC_HAL_SET_INIT_STATE(NFC_HAL_INIT_STATE_W4_PREDISCOVER_DONE);
                    nfa_hal_send_pre_discover_cfg ();
                    break;

                case NFC_HAL_EVT_CONTROL_GRANTED:
                    nfc_hal_dm_send_pend_cmd ();
                    break;

                default:
                    break;
                }

                if (free_msg)
                    GKI_freebuf (p_msg);
            }
        }
        if ((event & NFC_HAL_TASK_EVT_DATA_RDY) || ( DT_Unprocessed_Data() > 0))
        {
            do{
                if (nfc_hal_nci_receive_msg ())
                {
                    /* complete of receiving NCI message */
                    nfc_hal_nci_assemble_nci_msg ();
                    if (nfc_hal_cb.ncit_cb.p_rcv_msg)
                    {
                        if (nfc_hal_nci_preproc_rx_nci_msg (nfc_hal_cb.ncit_cb.p_rcv_msg))
                        {
                            /* Send NCI message to the stack */
                            nfc_hal_cb.p_data_cback(nfc_hal_cb.ncit_cb.p_rcv_msg->len, (UINT8 *)((nfc_hal_cb.ncit_cb.p_rcv_msg + 1)
                                                             + nfc_hal_cb.ncit_cb.p_rcv_msg->offset));
                        }
                    }

                    if (nfc_hal_cb.ncit_cb.p_rcv_msg)
                    {
                        GKI_freebuf(nfc_hal_cb.ncit_cb.p_rcv_msg);
                        nfc_hal_cb.ncit_cb.p_rcv_msg = NULL;
                    }
                }
            }while(DT_Unprocessed_Data() > 0);
        }
        /* Process quick timer tick */
        if (event & NFC_HAL_QUICK_TIMER_EVT_MASK)
        {
            nfc_hal_main_process_quick_timer_evt ();
        }
    }

    HAL_TRACE_DEBUG0 ("nfc_hal_main_task terminated");

    GKI_exit_task (GKI_get_taskid ());
    return 0;
}

/*******************************************************************************
**
** Function         HAL_NfcSetTraceLevel
**
** Description      This function sets the trace level for HAL.  If called with
**                  a value of 0xFF, it simply returns the current trace level.
**
** Returns          The new or current trace level
**
*******************************************************************************/
UINT8 HAL_NfcSetTraceLevel (UINT8 new_level)
{
    if (new_level != 0xFF)
        nfc_hal_cb.trace_level = new_level;

    return (nfc_hal_cb.trace_level);
}

/*******************************************************************************
**
** Function         isRamDumpCmd
**
** Description      Determines if the msg is a ramdump command.
**                  This should NEVER be true as ramdump CMDs should not pass through here.
**                  This layer is higher!
**
** Returns          TRUE is the msg is a Ramdump cmd, FALSE otherwise
**
*******************************************************************************/
BOOLEAN isRamDumpCmd(NFC_HDR *p_msg)
{
    HAL_TRACE_DEBUG0 ("isRamDumpCmd(): RAMDUMP: Checking if command is a ramdump cmd.");

    BOOLEAN ret = FALSE;

    UINT8 *p = (UINT8 *) (p_msg + 1) + p_msg->offset;
    if ((*p & (NCI_MTS_CMD | NCI_GID_PROP)) == (NCI_MTS_CMD | NCI_GID_PROP))
    {
        HAL_TRACE_DEBUG4 ("isRamDumpCmd(): RAMDUMP: Msg (0x%02x%02x%02x) is a ramdump cmd (starting with 0x%2x)", *p, *(p+1), *(p+2), (NCI_MTS_CMD | NCI_GID_PROP));
        ret = TRUE;
    }
#ifdef RAMDUMP
    else
    {
        HAL_TRACE_DEBUG0 ("isRamDumpCmd(): RAMDUMP: Msg is NOT a ramdump cmd");
    }
#endif
    return ret;
}
