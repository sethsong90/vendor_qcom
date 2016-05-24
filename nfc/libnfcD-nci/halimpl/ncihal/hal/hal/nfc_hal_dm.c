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
 *  Copyright (C) 2012-2013 Broadcom Corporation
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

/******************************************************************************
 *
 *  Vendor-specific handler for DM events
 *
 ******************************************************************************/
#include <string.h>
#include "nfc_hal_int.h"
#include "nfc_hal_post_reset.h"
#include "userial.h"
#include <string.h>
#include <DT_Nfc_link.h>
#include <DT_Nfc_types.h>
#include <DT_Nfc_status.h>
#include <DT_Nfc_i2c.h>
#include <DT_Nfc_log.h>
#include <DT_Nfc.h>
#include <config.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

#ifdef DTA
#include "dta_flag.h"
#endif // </DTA>

/*****************************************************************************
** Constants and types
*****************************************************************************/

#define NFC_HAL_I93_RW_CFG_LEN              (5)
#define NFC_HAL_I93_RW_CFG_PARAM_LEN        (3)
#define NFC_HAL_I93_AFI                     (0)
#define NFC_HAL_I93_ENABLE_SMART_POLL       (1)
#define PATCH_NOT_UPDATED                    3
#define PATCH_UPDATED                        4
static UINT8 nfc_hal_dm_init_ramdump_cmd[NCI_MSG_HDR_SIZE] =
{
    NCI_MTS_CMD|NCI_GID_PROP,
    NCI_OID_INIT,
    0x00 /* Empty payload */
};

static UINT8 nfc_hal_dm_get_ramdump_cmd[NCI_MSG_HDR_SIZE + NCI_HAL_RAMDATA] =
{
    NCI_MTS_CMD|NCI_GID_PROP,
    NCI_OID_GET,
    NCI_HAL_RAMDATA,        /* Currently 5 bytes in length */
    0x00, 0x00, 0x00, 0x00, /* Start address to begin next RamDump upload */
    0x00                    /* Length of this RamDump data */
};

static UINT8 nfc_hal_dm_end_ramdump_cmd[NCI_MSG_HDR_SIZE] =
{
    NCI_MTS_CMD|NCI_GID_PROP,
    NCI_OID_END,
    0x00 /* Empty payload */
};

static UINT8 nfc_hal_dm_ramdump_reset_nfcc[NCI_MSG_HDR_SIZE + 0x0D] =
{
    NCI_MTS_CMD|NCI_GID_PROP,
    0x00,
    0x0D,
    0x82, 0x78, 0x56, 0x34, 0x12, 0x01, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00
};

#define NFCA_PATCHFILE_V30_LOCATION  "/system/etc/firmware/Signedrompatch_v30.bin"
#define NFCA_PATCHFILE_V20_LOCATION  "/system/etc/firmware/Signedrompatch_v20.bin"
#define NFCA_PATCHFILE_V21_LOCATION  "/system/etc/firmware/Signedrompatch_v21.bin"
#define NFCA_PATCHFILE_V24_LOCATION  "/system/etc/firmware/Signedrompatch_v24.bin"
static UINT8 nfc_hal_dm_i93_rw_cfg[NFC_HAL_I93_RW_CFG_LEN] =
{
    NCI_PARAM_ID_I93_DATARATE,
    NFC_HAL_I93_RW_CFG_PARAM_LEN,
    NFC_HAL_I93_FLAG_DATA_RATE,    /* Bit0:Sub carrier, Bit1:Data rate, Bit4:Enable/Disable AFI */
    NFC_HAL_I93_AFI,               /* AFI if Bit 4 is set in the flag byte */
    NFC_HAL_I93_ENABLE_SMART_POLL  /* Bit0:Enable/Disable smart poll */
};

static UINT8 nfc_hal_dm_set_fw_fsm_cmd[NCI_MSG_HDR_SIZE + 1] =
{
    NCI_MTS_CMD|NCI_GID_PROP,
    NCI_MSG_SET_FWFSM,
    0x01,
    0x00,
};
#define NCI_SET_FWFSM_OFFSET_ENABLE      3

const UINT8 nfc_hal_dm_core_reset_cmd[NCI_MSG_HDR_SIZE + NCI_CORE_PARAM_SIZE_RESET] =
{
    NCI_MTS_CMD|NCI_GID_CORE,
    NCI_MSG_CORE_RESET,
    NCI_CORE_PARAM_SIZE_RESET,
    NCI_RESET_TYPE_RESET_CFG
};

const UINT8 nfc_hal_dm_prop_sleep_cmd[NCI_MSG_HDR_SIZE] =
{
    NCI_MTS_CMD|NCI_GID_PROP,
    0x03,
    0x00,
};
#define NCI_PROP_PARAM_SIZE_XTAL_INDEX      3       /* length of parameters in XTAL_INDEX CMD */

const UINT8 nfc_hal_dm_get_build_info_cmd[NCI_MSG_HDR_SIZE] =
{
    NCI_MTS_CMD|NCI_GID_PROP,
    NCI_MSG_GET_BUILD_INFO,
    0x00
};
#define NCI_BUILD_INFO_OFFSET_HWID  25  /* HW ID offset in build info RSP */

const UINT8 nfc_hal_dm_get_patch_version_cmd [NCI_MSG_HDR_SIZE] =
{
    NCI_MTS_CMD|NCI_GID_PROP,
    NCI_MSG_GET_PATCH_VERSION,
    0x00
};
const UINT8 nfc_hal_dm_core_init_cmd[NCI_MSG_HDR_SIZE + NCI_CORE_PARAM_SIZE_INIT] =
{
    NCI_MTS_CMD|NCI_GID_CORE,
    NCI_MSG_CORE_INIT,
    NCI_CORE_PARAM_SIZE_INIT
};
#define NCI_PATCH_INFO_VERSION_LEN  16  /* Length of patch version string in PATCH_INFO */

const UINT8 nfc_hal_dm_core_con_create_cmd[8] =
{
    0x20, 0x04, 0x05, 0xC1, 0x1, 0xA1, 0x01,0x00
};

const UINT8 nfc_hal_dm_core_con_close_cmd[8] =
{
    0x20, 0x05 , 0x01, 0x01
};

const UINT8 nfc_hal_dm_QC_prop_cmd_patchinfo[4] =
{
    0x2f, 0x01, 0x01, 0x01
};

const UINT8 nfc_hal_dm_QC_prop_cmd_fwversion[4] =
{
    0x2f, 0x01, 0x01, 0x00
};

const UINT8 nfc_hal_dm_prop_region2_enable_debug_off_cmd[5] =
{
    NCI_MTS_CMD|NCI_GID_PROP,
    0x01, 0x02, 0X08, 0X00
};

const UINT8 nfc_hal_dm_prop_region2_enable_debug_on_cmd[5] =
{
    NCI_MTS_CMD|NCI_GID_PROP,
    0x01, 0x02, 0X08, 0X01
};

const UINT8 nfc_hal_dm_prop_region2_control_enable[5] =
{
    NCI_MTS_CMD|NCI_GID_PROP,
    0x01, 0x02, 0X07, 0X01
};
const UINT8 nfc_hal_dm_prop_region2_control_disable[5] =
{
    NCI_MTS_CMD|NCI_GID_PROP,
    0x01, 0x02, 0X08, 0X00
};

UINT16 more_updates = 0;
static UINT8 patch_applied = FALSE, patch_last_data_buff_sent = FALSE;
static UINT8 gen_err_ntf_recieved = FALSE, after_patch_core_reset = FALSE;
UINT8  patch_update = FALSE;
UINT32 patchdatalen = 0;
UINT8 *patchdata = NULL;
UINT32 prepatchdatalen = 0;
UINT8 *prepatchdata = NULL;
UINT8 *pPatch_buff = NULL;
UINT8 op_code1 = 1;
UINT8 wait_reset_rsp = FALSE;
extern UINT8 reset_status;
#define NCI_PATCH_INFO_OFFSET_NVMTYPE  35  /* NVM Type offset in patch info RSP */
/*****************************************************************************
** Extern function prototypes
*****************************************************************************/
extern UINT8 *p_nfc_hal_dm_lptd_cfg;
extern UINT8 *p_nfc_hal_dm_start_up_cfg;
extern UINT8 *p_nfc_hal_dm_start_up_vsc_cfg;
extern tNFC_HAL_CFG *p_nfc_hal_cfg;
/*This flag will be set true if ramdump is being started*/
extern UINT8 reset_status;

/* Ramdump actions */
#ifndef NDEBUG
extern void nfc_hal_nci_handle_ramdump_ntf(int mt, int op_code, int pbf, int payload_len, int reason);
#else
extern void nfc_hal_nci_handle_ramdump_ntf();
#endif

/*****************************************************************************
** Local function prototypes
*****************************************************************************/

/*******************************************************************************
**
** Function         nfc_hal_dm_set_config
**
** Description      Send NCI config items to NFCC
**
** Returns          tHAL_NFC_STATUS
**
*******************************************************************************/
tHAL_NFC_STATUS nfc_hal_dm_set_config (UINT8 tlv_size,
                                       UINT8 *p_param_tlvs,
                                       tNFC_HAL_NCI_CBACK *p_cback)
{
    UINT8  *p_buff, *p;
    UINT8  num_param = 0, param_len, rem_len, *p_tlv;
    UINT16 cmd_len = NCI_MSG_HDR_SIZE + tlv_size + 1;
    tHAL_NFC_STATUS status = HAL_NFC_STATUS_FAILED;

    if ((tlv_size == 0)||(p_param_tlvs == NULL))
    {
        return status;
    }
    p_buff = (UINT8 *) GKI_getbuf ((UINT16)(NCI_MSG_HDR_SIZE + tlv_size));
    if (p_buff != NULL)
    {
        p = p_buff;

        NCI_MSG_BLD_HDR0 (p, NCI_MT_CMD, NCI_GID_CORE);
        NCI_MSG_BLD_HDR1 (p, NCI_MSG_CORE_SET_CONFIG);
        UINT8_TO_STREAM  (p, (UINT8) (tlv_size + 1));

        rem_len = tlv_size;
        p_tlv   = p_param_tlvs;
        while (rem_len > 1)
        {
            num_param++;                /* number of params */

            p_tlv ++;                   /* param type   */
            param_len = *p_tlv++;       /* param length */

            rem_len -= 2;               /* param type and length */
            if (rem_len >= param_len)
            {
                rem_len -= param_len;
                p_tlv   += param_len;   /* next param_type */

                if (rem_len == 0)
                {
                    status = HAL_NFC_STATUS_OK;
                    break;
                }
            }
            else
            {
                /* error found */
                break;
            }
        }

        if (status == HAL_NFC_STATUS_OK)
        {
            UINT8_TO_STREAM (p, num_param);
            ARRAY_TO_STREAM (p, p_param_tlvs, tlv_size);

            nfc_hal_dm_send_nci_cmd (p_buff, cmd_len, p_cback);
        }
        else
        {
            HAL_TRACE_ERROR0 ("nfc_hal_dm_set_config ():Bad TLV");
        }

        GKI_freebuf (p_buff);
    }

    return status;
}

/*******************************************************************************
**
** Function         nfc_hal_dm_set_fw_fsm
**
** Description      Enable or disable FW FSM
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_set_fw_fsm (BOOLEAN enable, tNFC_HAL_NCI_CBACK *p_cback)
{
    if (enable)
        nfc_hal_dm_set_fw_fsm_cmd[NCI_SET_FWFSM_OFFSET_ENABLE] = 0x01; /* Enable, default is disabled */
    else
        nfc_hal_dm_set_fw_fsm_cmd[NCI_SET_FWFSM_OFFSET_ENABLE] = 0x00; /* Disable */

    nfc_hal_dm_send_nci_cmd (nfc_hal_dm_set_fw_fsm_cmd, NCI_MSG_HDR_SIZE + 1, p_cback);
}

/*******************************************************************************
**
** Function         nfc_hal_dm_config_nfcc_cback
**
** Description      Callback for NCI vendor specific command complete
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_config_nfcc_cback (tNFC_HAL_NCI_EVT event, UINT16 data_len, UINT8 *p_data)
{
    if (nfc_hal_cb.dev_cb.next_dm_config == NFC_HAL_DM_CONFIG_NONE)
    {
        nfc_hal_hci_enable ();
    }
    else
    {
        nfc_hal_dm_config_nfcc ();
    }
}

/*******************************************************************************
**
** Function         nfc_hal_dm_send_startup_vsc
**
** Description      Send VS command before NFA start-up
**
** Returns          None
**
*******************************************************************************/
void nfc_hal_dm_send_startup_vsc (void)
{
    UINT8  *p, *p_end;
    UINT16 len;

    HAL_TRACE_DEBUG0 ("nfc_hal_dm_send_startup_vsc ()");

    /* VSC must have NCI header at least */
    if (nfc_hal_cb.dev_cb.next_startup_vsc + NCI_MSG_HDR_SIZE - 1 <= *p_nfc_hal_dm_start_up_vsc_cfg)
    {
        p     = p_nfc_hal_dm_start_up_vsc_cfg + nfc_hal_cb.dev_cb.next_startup_vsc;
        len   = *(p + 2);
        p_end = p + NCI_MSG_HDR_SIZE - 1 + len;

        if (p_end <= p_nfc_hal_dm_start_up_vsc_cfg + *p_nfc_hal_dm_start_up_vsc_cfg)
        {
            /* move to next VSC */
            nfc_hal_cb.dev_cb.next_startup_vsc += NCI_MSG_HDR_SIZE + len;

            /* if this is last VSC */
            if (p_end == p_nfc_hal_dm_start_up_vsc_cfg + *p_nfc_hal_dm_start_up_vsc_cfg)
                nfc_hal_cb.dev_cb.next_dm_config = NFC_HAL_DM_CONFIG_NONE;

            nfc_hal_dm_send_nci_cmd (p, (UINT16)(NCI_MSG_HDR_SIZE + len), nfc_hal_dm_config_nfcc_cback);
            return;
        }
    }

    HAL_TRACE_ERROR0 ("nfc_hal_dm_send_startup_vsc (): Bad start-up VSC");

    NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_IDLE);
    nfc_hal_cb.p_stack_cback (HAL_NFC_POST_INIT_CPLT_EVT, HAL_NFC_STATUS_FAILED);
}

/*******************************************************************************
**
** Function         nfc_hal_dm_config_nfcc
**
** Description      Send VS config before NFA start-up
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_config_nfcc (void)
{
    HAL_TRACE_DEBUG1 ("nfc_hal_dm_config_nfcc (): next_dm_config = %d", nfc_hal_cb.dev_cb.next_dm_config);

    if ((p_nfc_hal_dm_start_up_cfg[0]))
    {
        nfc_hal_cb.dev_cb.next_dm_config = NFC_HAL_DM_CONFIG_NONE;
        if (nfc_hal_dm_set_config (p_nfc_hal_dm_start_up_cfg[0],
                                   &p_nfc_hal_dm_start_up_cfg[1],
                                   nfc_hal_dm_config_nfcc_cback) == HAL_NFC_STATUS_OK)
        {
            return;
        }
        else
        {
            NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_IDLE);
            nfc_hal_cb.p_stack_cback (HAL_NFC_POST_INIT_CPLT_EVT, HAL_NFC_STATUS_FAILED);
            return;
        }
    }

}

/*******************************************************************************
**
** Function         nfc_hal_dm_send_reset_cmd
**
** Description      Send CORE RESET CMD
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_send_reset_cmd (void)
{
    /* Proceed with start up sequence: send CORE_RESET_CMD */
    NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_NFCC_ENABLE);

    nfc_hal_dm_send_nci_cmd (nfc_hal_dm_core_reset_cmd, NCI_MSG_HDR_SIZE + NCI_CORE_PARAM_SIZE_RESET, NULL);
}

/*******************************************************************************
**
** Function         nfc_hal_dm_send_init_ramdump_cmd
**
** Description      Send NCI Initiate RAMDUMP command
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_send_prop_init_ramdump_cmd (void)
{
    HAL_TRACE_DEBUG0("nfc_hal_dm_send_prop_init_ramdump_cmd: RAMDUMP: Sending NFCC Init CMD");
    nfc_hal_dm_send_nci_cmd (nfc_hal_dm_init_ramdump_cmd, NCI_MSG_HDR_SIZE, NULL);
}

/*******************************************************************************
**
** Function         nfc_hal_dm_send_get_ramdump_cmd
**
** Description      Send NCI Get RAMDUMP command
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_send_prop_get_ramdump_cmd (int ramdump_start_addr, int ramdump_length)
{
    HAL_TRACE_DEBUG0("nfc_hal_dm_send_prop_get_ramdump_cmd: RAMDUMP: Sending NFCC Get CMD");

    /* We are not interpreting the addr, this is why we havn't used unsigned int */
    nfc_hal_dm_get_ramdump_cmd[3] = (ramdump_start_addr >> 24) & 0xFF;
    nfc_hal_dm_get_ramdump_cmd[4] = (ramdump_start_addr >> 16) & 0xFF;
    nfc_hal_dm_get_ramdump_cmd[5] = (ramdump_start_addr >> 8) & 0xFF;
    nfc_hal_dm_get_ramdump_cmd[6] = ramdump_start_addr & 0xFF;

    nfc_hal_dm_get_ramdump_cmd[7] = ramdump_length & 0xFF;
    nfc_hal_dm_send_nci_cmd (nfc_hal_dm_get_ramdump_cmd, NCI_MSG_HDR_SIZE + NCI_HAL_RAMDATA, NULL);
}

/*******************************************************************************
**
** Function         nfc_hal_dm_send_end_ramdump_cmd
**
** Description      Send NCI End RAMDUMP command
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_send_prop_end_ramdump_cmd (void)
{
    HAL_TRACE_DEBUG0("nfc_hal_dm_send_prop_end_ramdump_cmd: RAMDUMP: Sending NFCC End CMD");
    nfc_hal_dm_send_nci_cmd (nfc_hal_dm_end_ramdump_cmd, NCI_MSG_HDR_SIZE, NULL);
}


/*******************************************************************************
**
** Function         nfc_hal_dm_send_prop_reset_nfcc_ramdump_poke
**
** Description      Sends the NFCC a poke msg with a misalligned address
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_send_prop_reset_nfcc_ramdump_poke (void)
{
    HAL_TRACE_DEBUG0("nfc_hal_dm_send_prop_reset_nfcc_ramdump_poke: RAMDUMP: Sending NFCC poke to cause reset");
    nfc_hal_dm_send_nci_cmd (nfc_hal_dm_ramdump_reset_nfcc, NCI_MSG_HDR_SIZE + 0x0D, NULL);
}


/*******************************************************************************
**
** Function         nfc_hal_dm_send_prop_sleep_cmd
**
** Description      Send CORE RESET CMD
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_send_prop_sleep_cmd (void)
{
    if ( !nfc_hal_cb.dev_cb.nfcc_sleep_mode)
    {
        nfc_hal_dm_send_nci_cmd (nfc_hal_dm_prop_sleep_cmd, NCI_MSG_HDR_SIZE, NULL);
        nfc_hal_cb.dev_cb.nfcc_sleep_mode = SLEEP_STATE;
        HAL_TRACE_DEBUG0("nfc_hal_cb.dev_cb.nfcc_sleep_mode=1");
    }
}

int nfc_hal_dm_get_nfc_sleep_state (void)
{
    return nfc_hal_cb.dev_cb.nfcc_sleep_mode;
}
/*******************************************************************************
**
** Function         nfc_hal_send_data
**
** Description      send HAL data to the NFCC
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_send_data(const UINT8 *p_data, UINT16 len, tNFC_HAL_NCI_CBACK *p_cback)
{
    nfc_hal_dm_send_nci_cmd (p_data,len,NULL);
}
/*******************************************************************************
**
** Function         nfc_hal_dm_verify_nvm_file
**
** Description      verifies the nvm file if its format is fine or not.
**
** Returns          void
**
*******************************************************************************/
int nfc_hal_dm_verify_nvm_file(void)
{
    UINT8 ch = 0,char_cnt = 0, k = 0, j = 0;
    UINT32 num_of_entries = 0, data_byte = 0, i = 0;
    char data_buff[5] = {0}, more_entries_flag = FALSE, num_of_update[5] = {0}, cnt = 0;
    fpos_t position = 0;

    HAL_TRACE_DEBUG0("nfc_hal_dm_verify_nvm_file: Start");
    /* check the number of updates and further file format*/
    while(ch != '\n')
    {
        ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
        num_of_update[cnt++] = ch;
    }
    nfc_hal_cb.nvm.no_of_updates = atoi(num_of_update);
    if(nfc_hal_cb.nvm.no_of_updates == 0)
    {
        /* Number of updates found zero*/
        return FALSE;
    }
    HAL_TRACE_DEBUG1("NVM update: number of updates available is %d",nfc_hal_cb.nvm.no_of_updates);
    for(i = 0; i < nfc_hal_cb.nvm.no_of_updates; i++)
    {
        if(fgetc(nfc_hal_cb.nvm.p_Nvm_file) != '\n')
        {
            HAL_TRACE_DEBUG0("NVM update: file format is ..cmd should be from next line");
            return FALSE;
        }
        data_byte = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
        if((data_byte == 32) || (data_byte == 16) || (data_byte == 8) ||
           (data_byte == 2) || (data_byte == 1) || (data_byte == 0))
        {
            /*check if it is followed by space and proper format address*/
            if(fgetc(nfc_hal_cb.nvm.p_Nvm_file) == 32)
            {
                /*check if address is in proper format*/
                if(fgetc(nfc_hal_cb.nvm.p_Nvm_file) == '0')
                {
MORE_ENTRIES:
                    if(fgetc(nfc_hal_cb.nvm.p_Nvm_file) == 'x')
                    {
                        /*address format check*/
                        for(j = 0; j < 4; j++)
                        {
                             /*pass 8 address chars and check address format is fine*/
                             ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                             if((ch == 0x20 /*space*/))
                             {
                                 /* 0x20 may be valid bytes in addr so do further checks*/
                                 fgetpos (nfc_hal_cb.nvm.p_Nvm_file, &position);
                                 /* check if there is any space after this 0x20 byte(space) encountered*/
                                 if((fgetc(nfc_hal_cb.nvm.p_Nvm_file) != 0x20))
                                 {
                                     /*it may be a space. check the next byte to see if it is non zero i.e num_of_entries*/
                                     if(fgetc(nfc_hal_cb.nvm.p_Nvm_file) != 0x00)
                                     {
                                         if(fgetc(nfc_hal_cb.nvm.p_Nvm_file) == 0x20 /*space after num_of_entries*/)
                                         {
                                             /* check if after this some valid data values are there */
                                             if((fgetc(nfc_hal_cb.nvm.p_Nvm_file) == '0') && (fgetc(nfc_hal_cb.nvm.p_Nvm_file) == 'x'))
                                             {
                                                 HAL_TRACE_DEBUG1("NVM update: address format is not 32 bit...Currupt update num is %d",i+1);
                                                 if(more_entries_flag == TRUE)
                                                 {
                                                     /*one of extra updates is currupt*/
                                                     more_entries_flag = FALSE;
                                                     more_updates = 0;
                                                 }
                                                 return FALSE;
                                             }
                                         }
                                     }
                                 }
                                 fsetpos (nfc_hal_cb.nvm.p_Nvm_file, &position);
                             }
                        }
                        /*check space before num of entries*/
                        if(fgetc(nfc_hal_cb.nvm.p_Nvm_file) == 0x20)
                        {
                            num_of_entries = fgetc(nfc_hal_cb.nvm.p_Nvm_file);

                            /* check data format in case of poke cmd only.Because peek cmd has no data value in it*/
                            if((data_byte==32) || (data_byte==16) || (data_byte ==8))
                            {
                                for(j=0;j<num_of_entries;j++)
                                {
                                    /*check space after num_of_entries and then for subsequent data*/
                                    if(fgetc(nfc_hal_cb.nvm.p_Nvm_file) == 0x20)
                                    {
                                        /*check data format is fine.0x...format*/
                                        if(fgetc(nfc_hal_cb.nvm.p_Nvm_file) == '0')
                                        {
                                            if(fgetc(nfc_hal_cb.nvm.p_Nvm_file) == 'x')
                                            {
                                                if(data_byte ==32)
                                                {
                                                    /*traverse 8 chars of data */
                                                    for(k=0;k<4;k++)
                                                    {
                                                        ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                                                    }
                                                }
                                                else if(data_byte == 16)
                                                {
                                                    /*traverse 4 chars of data */
                                                    for(k=0;k<2;k++)
                                                    {
                                                       ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                                                    }
                                                }
                                                else if(data_byte == 8)
                                                {
                                                    /*traverse 2 chars of data */
                                                    for(k=0;k<1;k++)
                                                    {
                                                        ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                HAL_TRACE_DEBUG1("NVM update: data format wrong...needed format 0x...Currupt update num is %d",i+1);
                                                if(more_entries_flag == TRUE)
                                                {
                                                    /*one of extra updates is currupt*/
                                                    more_entries_flag = FALSE;
                                                    more_updates = 0;
                                                }
                                                return FALSE;
                                            }
                                        }
                                        else
                                        {
                                              HAL_TRACE_DEBUG1("NVM update: data format wrong...needed format 0x...Currupt update num is %d",i+1);
                                              if(more_entries_flag == TRUE)
                                              {
                                                  /*one of extra updates is currupt*/
                                                  more_entries_flag = FALSE;
                                                  more_updates = 0;
                                              }
                                              return FALSE;
                                        }
                                    }
                                    else
                                    {
                                        HAL_TRACE_DEBUG1("NVM update: data format wrong...needed space after number of entries  .Currupt update num is %d",i+1);
                                        if(more_entries_flag == TRUE)
                                        {
                                            /*one of extra updates is currupt*/
                                            more_entries_flag = FALSE;
                                            more_updates = 0;
                                        }
                                        return FALSE;
                                    }
                                }
                                if(more_entries_flag == TRUE)
                                {
                                    break;
                                }
                            }
                        }
                        else
                        {
                            HAL_TRACE_DEBUG1("NVM update: data format wrong...needed space before number of entries .Currupt update num is %d",i+1);
                            if(more_entries_flag == TRUE)
                            {
                                /*one of extra updates is currupt*/
                                more_entries_flag = FALSE;
                                more_updates = 0;
                            }
                            return FALSE;
                        }
                    }
                    else
                    {
                        HAL_TRACE_DEBUG1("NVM update: addr format wrong..wrong nvm cmd format .Currupt update num is %d",i+1);
                        if(more_entries_flag == TRUE)
                        {
                            /*one of extra updates is currupt*/
                            more_entries_flag = FALSE;
                            more_updates = 0;
                        }
                        return FALSE;
                    }
                }
                else
                {
                    HAL_TRACE_DEBUG1("NVM update: addr format wrong..wrong nvm cmd format .Currupt update num is %d",i+1);
                    if(more_entries_flag == TRUE)
                    {
                        /*one of extra updates is currupt*/
                         more_entries_flag = FALSE;
                         more_updates = 0;
                    }
                    return FALSE;
                }
            }
            else
            {
                  HAL_TRACE_DEBUG1("NVM update: Space is missing in cmd..wrong nvm cmd format .Currupt update num is %d",i+1);
                  if(more_entries_flag == TRUE)
                  {
                      /*one of extra updates is currupt*/
                       more_entries_flag = FALSE;
                       more_updates = 0;
                  }
                  return FALSE;
            }
        }
        else
        {
            /*cmd format is wrong*/
            return FALSE;
        }
    }

    /* check if further updates are there .means more than specified in file*/
    fgetc(nfc_hal_cb.nvm.p_Nvm_file); /*pass new line feed ch*/
    data_byte = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
    if((data_byte== 32) || (data_byte== 16) || (data_byte== 8) ||
       (data_byte== 2) || (data_byte== 1) || (data_byte== 0))
    {
        /*check against the space in the end*/
        if((fgetc(nfc_hal_cb.nvm.p_Nvm_file) == 32) && (fgetc(nfc_hal_cb.nvm.p_Nvm_file)== '0'))
        {
            more_entries_flag = TRUE;
            more_updates++;
            HAL_TRACE_DEBUG1("NVM update: more updates are available...Verify again %d",more_updates);
            goto MORE_ENTRIES;
        }
        else
        {
            HAL_TRACE_DEBUG0("NVM update: Nvm file verfied ..no more updates are availabe");
        }
    }
    return TRUE;
}
/*******************************************************************************
**
** Function         nfc_hal_dm_check_nvm_file
**
** Description      Checks if the NVM update file is available in specified dir
**                  if yes then reads the updates one by one besdies deciding the
**                  number of updates.
**
** Returns          int(TRUE if file is present or vice versa)
**
*******************************************************************************/
int nfc_hal_dm_check_nvm_file(UINT8 *nvmupdatebuff,UINT8 *nvmupdatebufflen)
{
    UINT32 patchdatalength  = 0,datalen = 0, no_of_bytes_to_read = 0;
    UINT8 ch = 0, i = 0, no_of_octets = 0, addr_buf[4] = {0};
    UINT8 octet_cnt = 0, j = 0, tmp_databuff[100] = {0}, k = 0;
    UINT8 num_of_update[5] = {0}, cnt = 0;
    char pNvmfilepath[100] = {0};
    fpos_t position = 0;

    if(GetStrValue("NVM_FILE_PATH", &pNvmfilepath[0], sizeof(pNvmfilepath)))
    {
        HAL_TRACE_DEBUG1("NVM_FILE_PATH found: %s",pNvmfilepath);
    }
    else
    {
        HAL_TRACE_DEBUG0("NVM_FILE_PATH not found");
    }

    if(!nfc_hal_cb.nvm.p_Nvm_file)
    {
        /*nvm update file is opened only once */
        nfc_hal_cb.nvm.p_Nvm_file = fopen(pNvmfilepath,"rb");
        if(nfc_hal_cb.nvm.p_Nvm_file)
        {
            /*verify if file format and entries are fine*/
            if(nfc_hal_dm_verify_nvm_file())
            {
                fclose(nfc_hal_cb.nvm.p_Nvm_file);
                nfc_hal_cb.nvm.p_Nvm_file = fopen(pNvmfilepath,"rb");
                while(ch != '\n')
                {
                    ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                    num_of_update[cnt++] = ch;
                }
                nfc_hal_cb.nvm.no_of_updates =atoi(num_of_update);
                if(more_updates)
                {
                    nfc_hal_cb.nvm.no_of_updates += more_updates;
                    more_updates =0;
                    HAL_TRACE_DEBUG1("NVM update:But Actual total Updates in nvm file are %d",nfc_hal_cb.nvm.no_of_updates);
                }
                HAL_TRACE_DEBUG0("NVM update: nvm file verification passed");
                nfc_hal_cb.nvm.nvm_updated = TRUE;
            }
            else
            {
                fclose(nfc_hal_cb.nvm.p_Nvm_file);
                nfc_hal_cb.nvm.no_of_updates = 0;
                HAL_TRACE_DEBUG0("NVM update: nvm file verification failed");
                return FALSE;
            }
        }
        else
        {
            /*NVM Update file is not present*/
            HAL_TRACE_DEBUG0("NVM Update file is not present");
            return FALSE;
        }
    }
    if(nfc_hal_cb.nvm.p_Nvm_file)
    {
        ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);       // discard LF (new line char)
        ch = 0;
        /* read first the type of data i.e 32 bit or 16 bit or 8 bit.*/
        nvmupdatebuff[i++] = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
        /*discard first space character*/
        ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
        /*Now read the address*/
        ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);    // discard 0
        ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);    // discard x
        while(i != 5)
        {
            nvmupdatebuff[i++] = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
            addr_buf[i-2] = nvmupdatebuff[i-1];
        }
        /*Reverse address bytes*/
        nvmupdatebuff[1] = addr_buf[3];
        nvmupdatebuff[2] = addr_buf[2];
        nvmupdatebuff[3] = addr_buf[1];
        nvmupdatebuff[4] = addr_buf[0];
        /* discard second space*/
        ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
        /* read number of items and calculate total octets to be read*/
        nvmupdatebuff[i++] = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
        /* check how many entries are to be read*/
        if((nvmupdatebuff[0] == 32) || (nvmupdatebuff[0] == 2))
        { /*32 bits*/
            if(nvmupdatebuff[0] == 32)
            {
                nvmupdatebuff[0] = 0x82;
                no_of_octets = nvmupdatebuff[i-1]*4;
                octet_cnt = 4;
            }
            else
            {
                nvmupdatebuff[0] = 0x02;
                no_of_octets = 0;
            }
        }
        else if((nvmupdatebuff[0] == 16) || (nvmupdatebuff[0] == 1))
        { /*16 bits*/
            if(nvmupdatebuff[0] == 16)
            {
                nvmupdatebuff[0] = 0x81;
                no_of_octets = nvmupdatebuff[i-1]*2;
                octet_cnt = 2;
            }
            else
            {
                nvmupdatebuff[0] = 0x01;
                no_of_octets = 0;
            }
        }
        else
        { /* 8 bits*/
            if(nvmupdatebuff[0] == 8)
            {
                nvmupdatebuff[0] = 0x80;
                no_of_octets = nvmupdatebuff[i-1];
                octet_cnt = 1;
            }
            else
            {
                nvmupdatebuff[0] = 0x00;
                no_of_octets = 0;
            }
        }
        if(no_of_octets != 0)
        {
            for(j = 0; j < no_of_octets+1; j++)
            {
                if(nvmupdatebuff[0] == 0x82)
                {
                    if((octet_cnt == 4) )
                    {
                        /* fill data in little endian format in nvmupdatebuff*/
                        if(k==4)
                        {
                            for(; k > 0 ; k--)
                                nvmupdatebuff[i++] = tmp_databuff[k-1];
                        }
                        k = 0;
                        /*check if all values are filled */
                        if(j == no_of_octets)
                            break;
                        /*discard the unnecessary chararcters*/
                        ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                        if(ch == 32)
                        {
                            // spcae is given , needs to be discarded
                            fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                            fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                            octet_cnt = 0;
                        }
                        else
                        {
                            // no spcae given , discard only x.0 discarded already.
                            fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                            octet_cnt = 0;
                        }
                    }
                }
                if(nvmupdatebuff[0] == 0x81)
                {
                    if((octet_cnt == 2) )
                    {
                        /* fill data in little endian format in nvmupdatebuff*/
                        if(k == 2)
                        {
                            for(; k > 0; k--)
                                nvmupdatebuff[i++] = tmp_databuff[k-1];
                        }
                        k= 0;
                        /*check if all values are filled */
                        if(j == no_of_octets)
                            break;
                        /*discard the unnecessary chararcters*/
                        ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                        if(ch == 32)
                        {
                            // spcae is given , needs to be discarded
                            fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                            fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                            octet_cnt=0;
                        }
                        else
                        {
                            // no spcae given , discard only x.0 discarded already.
                            fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                            octet_cnt=0;
                        }
                    }
                }
                if(nvmupdatebuff[0] == 0x80)
                {
                    if((octet_cnt == 1) )
                    {
                        /* fill data in little endian format in nvmupdatebuff*/
                        if(k == 1)
                        {
                            nvmupdatebuff[i++] = tmp_databuff[k-1];
                        }
                        k = 0;
                        /*check if all values are filled */
                        if(j == no_of_octets)
                            break;
                        /*discard the unnecessary chararcters*/
                        ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                        if(ch == 32)
                        {
                            // space is given , needs to be discarded
                            fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                            fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                            octet_cnt=0;
                        }
                        else
                        {
                            // no spcae given , discard only x.0 discarded already.
                            fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                            octet_cnt=0;
                        }
                    }
                }
                tmp_databuff[k++] = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                octet_cnt++;
            }
        }
        *nvmupdatebufflen = i-1;
        return TRUE;
    }
    return FALSE;
}
/*******************************************************************************
**
** Function         nfc_hal_dm_frame_mem_access_cmd
**
** Description     Prepares the NCI POKE Cmd with the provided update data
** Returns         void
**
*******************************************************************************/
void nfc_hal_dm_frame_mem_access_cmd(UINT8 *nvmcmd,UINT8 *nvmupdatebuff,UINT8 *nvmcmdlen)
{
    UINT8 num_of_data_bytes = 0,j=0,datalen=0;

    if(nvmupdatebuff[0] == 0x80)
    {
        /* all data is 8 bit long only*/
        num_of_data_bytes = nvmupdatebuff[5];
    }
    else if(nvmupdatebuff[0] == 0x81)
    {
        /* Half Word*/
        num_of_data_bytes = nvmupdatebuff[5]*2;
    }
    else
    {
        /* full Word*/
        if((nvmupdatebuff[0] != 0x02) && (nvmupdatebuff[0] != 0x01) && (nvmupdatebuff[0] != 0x00))
        {
            num_of_data_bytes = nvmupdatebuff[5]*4;
        }
   }

   nvmcmd[datalen++] = 0x2F;
   nvmcmd[datalen++] = 0x00;
   nvmcmd[datalen++] = NUM_OF_BYTES_ACCESS_FLAG + NUM_OF_BYTES_START_ADDR + \
                           NUM_OF_BYTES_NUMBER_OF_ITEMS + NUM_OF_BYTES_ADD_DELTA+ \
                           NUM_OF_BYTES_ACCESS_DELAY+num_of_data_bytes;

    /* access flags*/
   nvmcmd[datalen++] = nvmupdatebuff[0];

   /* fill addr + number of items*/
   for(;datalen<(NUM_OF_BYTES_START_ADDR+NUM_OF_BYTES_NUMBER_OF_ITEMS+4);datalen++)
       nvmcmd[datalen] = nvmupdatebuff[++j];

   nvmcmd[datalen++] = 0x01;  // Address Delta
   nvmcmd[datalen++] = 0x64;  // delay first byte
   nvmcmd[datalen++] = 0x00;  // delay second byte

   if((nvmupdatebuff[0] == 0x80) || (nvmupdatebuff[0] == 0x81) || (nvmupdatebuff[0] == 0x82))
   {
       /* fill data bytes of POKE Cmd*/
       for(j=0;j<num_of_data_bytes;j++)
           nvmcmd[datalen++] = nvmupdatebuff[j+6];
   }

   *nvmcmdlen = datalen;
}
/*******************************************************************************
**
** Function         nfc_hal_dm_check_fused_nvm_file
**
** Description      Checks if the NVM update file is available in specified dir
**                  if yes then reads updates one by one.
**
** Returns          int(TRUE if file is present or vice versa)
**
*******************************************************************************/
int nfc_hal_dm_check_fused_nvm_file(UINT8 *nvmupdatebuff,UINT8 *nvmupdatebufflen)
{
    UINT32 patchdatalength  = 0,datalen=0,no_of_bytes_to_read=0;
    UINT8 ch = 0,i=0,no_of_octets=0,octet_cnt=0,j=0,tmp_databuff[100]={0},k=0;
    UINT8 num_of_update[5]={0},cnt=0;
    fpos_t position=0;
    UINT8 nvm_read_req = FALSE;
    char pNvmfilepath[100] = {0};

    if(GetStrValue("FUSED_NVM_FILE_PATH", &pNvmfilepath[0], sizeof(pNvmfilepath)))
    {
        HAL_TRACE_DEBUG1("FUSED_NVM_FILE_PATH found: %s",pNvmfilepath);
    }
    else
    {
        HAL_TRACE_DEBUG0("FUSED_NVM_FILE_PATH not found");
    }

    if(!nfc_hal_cb.nvm.p_Nvm_file)
    {
        /*nvm update file is opened only once */
        nfc_hal_cb.nvm.p_Nvm_file = fopen(pNvmfilepath,"rb");
        if(nfc_hal_cb.nvm.p_Nvm_file)
        {
            while((ch != '\n'))
            {
                ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
                num_of_update[cnt++] = ch;
            }
            nfc_hal_cb.nvm.no_of_updates = atoi((const char*)num_of_update);
            nfc_hal_cb.nvm.nvm_updated = TRUE;
        }
        else
        {
            /*NVM Update file is not present*/
            HAL_TRACE_DEBUG0("NVM Update file is not present");
            return FALSE;
        }
    }

    if(nfc_hal_cb.nvm.p_Nvm_file)
    {
        ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);       // discard LF (new line char)

     //   HAL_TRACE_DEBUG1("Test11 ch=%X",ch);

        ch = 0;

        /* read entry type.*/
        nvmupdatebuff[i++] = fgetc(nfc_hal_cb.nvm.p_Nvm_file);

        if(nvmupdatebuff[i-1] == 6)
        {
            /*discard first space character*/
            ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
            /*NVM Read request.Read extra subcode byte from file*/
            nvmupdatebuff[i++] = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
            nvm_read_req = TRUE;
        }

        /*discard first space character*/
        ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
        /*Now read the Tag/Register index*/
        ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);    // discard 0
        ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);    // discard x

        if(nvm_read_req != TRUE)
        {
            while(i !=5)
                nvmupdatebuff[i++] = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
        }
        else
        {
            while(i !=6)
                nvmupdatebuff[i++] = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
        }

        if(nvm_read_req != TRUE)
        {
            /* discard second space*/
            ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);
            /*Now read the value*/
            ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);    // discard 0
            ch = fgetc(nfc_hal_cb.nvm.p_Nvm_file);    // discard x

            while(i != 9)
                nvmupdatebuff[i++] = fgetc(nfc_hal_cb.nvm.p_Nvm_file);

        }
        return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
**
** Function         nfc_hal_dm_frame_fused_mem_access_cmd
**
** Description     Prepares the fused NVM NCI Cmd with the provided update data
**
**
** Returns         void
**
*******************************************************************************/
void nfc_hal_dm_frame_fused_mem_access_cmd(UINT8 *nvmcmd,UINT8 *nvmupdatebuff,UINT8 *nvmcmdlen)
{
    UINT8 num_of_data_bytes = 0,j=0,datalen=0;

   nvmcmd[datalen++] = 0x2F;
   nvmcmd[datalen++] = 0x01;

   if(nvmupdatebuff[0] == 6)
   {
       nvmcmd[datalen++] = NUM_OF_BYTES_READ_SUB_OPCODE + NUM_OF_BYTES_START_ADDR;

       for(;datalen<(NUM_OF_BYTES_READ_SUB_OPCODE + NUM_OF_BYTES_START_ADDR+3);datalen++)
           nvmcmd[datalen] = nvmupdatebuff[j++];

   }
   else
   {
       //value 4 bytes include by multiplying 2 in tag/register index bytes
       nvmcmd[datalen++] = NUM_OF_BYTES_WRITE_SUB_OPCODE + (NUM_OF_BYTES_START_ADDR*2);

       for(;datalen<(NUM_OF_BYTES_WRITE_SUB_OPCODE + (NUM_OF_BYTES_START_ADDR*2)+3);datalen++)
           nvmcmd[datalen] = nvmupdatebuff[j++];

   }
   *nvmcmdlen = datalen;
}
/*******************************************************************************
** Function         nfc_hal_dm_send_get_build_info_cmd
**
** Description      Send NCI_MSG_GET_BUILD_INFO CMD
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_send_get_build_info_cmd (void)
{
    NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_BUILD_INFO);

    /* get build information to find out HW */
    nfc_hal_dm_send_nci_cmd (nfc_hal_dm_get_build_info_cmd, NCI_MSG_HDR_SIZE, NULL);
}

/*******************************************************************************
**
** Function         nfc_hal_dm_proc_msg_during_init
**
** Description      Process NCI message while initializing NFCC
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_proc_msg_during_init (NFC_HDR *p_msg)
{
    UINT8 *p;
    UINT8 reset_reason, reset_type;
    UINT8 mt, pbf, gid, op_code;
    UINT8 *p_old, old_gid, old_oid, old_mt;
    UINT8 u8;
    tNFC_HAL_NCI_CBACK *p_cback = NULL;
    UINT8   chipverlen;
    UINT8   chipverstr[NCI_SPD_HEADER_CHIPVER_LEN];
    UINT16  xtal_freq;
    UINT8 len = 0,nvmcmdlen = 0;
    UINT8 nvmupdatebuff[260] = {0}, nvmdatabufflen = 0;
    UINT8 *nvmcmd = NULL;
    UINT32 patch_update_flag = 0, nvm_update_flag = 0;
    UINT32 patch_version = 0,fused_nvm_flag=0;
    char patchfilepath[50] = {0}, prepatchfilepath[50] = {0};
    size_t str_len = 0;

    HAL_TRACE_DEBUG1 ("nfc_hal_dm_proc_msg_during_init(): init state:%d", nfc_hal_cb.dev_cb.initializing_state);
    GetNumValue("PATCH_UPDATE_ENABLE_FLAG", &patch_update_flag, sizeof(patch_update_flag));
    GetNumValue("NVM_UPDATE_ENABLE_FLAG", &nvm_update_flag, sizeof(nvm_update_flag));
    GetNumValue("FUSED_NVM_UPDATE_ENABLE_FLAG", &fused_nvm_flag, sizeof(fused_nvm_flag));
    if(nfc_hal_cb.dev_cb.store_path == FALSE)
    {
        /*Select patch file based on chip revision*/
        if((nfc_hal_cb.dev_cb.nfcc_chip_version == 3) && (nfc_hal_cb.dev_cb.nfcc_chip_metal_version == 0))
        {
            /*NFCC 3.0*/
            HAL_TRACE_DEBUG0("NFCC 3.0");
            if(GetStrValue("FW_PATCH_30", &patchfilepath[0], sizeof(patchfilepath)))
            {
                HAL_TRACE_DEBUG1("FW_PATCH_30 found: %s",patchfilepath);
            }
        }
        else if((nfc_hal_cb.dev_cb.nfcc_chip_version == 2) && (nfc_hal_cb.dev_cb.nfcc_chip_metal_version == 0))
        {
            /*NFCC 2.0*/
            HAL_TRACE_DEBUG0("NFCC 2.0");
            if(GetStrValue("FW_PATCH_20", &patchfilepath[0], sizeof(patchfilepath)))
            {
                HAL_TRACE_DEBUG1("FW_PATCH_20 found: %s",patchfilepath);
            }
        }
        else if((nfc_hal_cb.dev_cb.nfcc_chip_version == 2) && (nfc_hal_cb.dev_cb.nfcc_chip_metal_version == 1))
        {
            /*NFCC 2.1*/
            HAL_TRACE_DEBUG0("NFCC 2.1");
            if(GetStrValue("FW_PATCH_21", &patchfilepath[0], sizeof(patchfilepath)))
            {
                HAL_TRACE_DEBUG1("FW_PATCH_21 found: %s",patchfilepath);
            }
        }
        else if((nfc_hal_cb.dev_cb.nfcc_chip_version == 2) && (nfc_hal_cb.dev_cb.nfcc_chip_metal_version == 4))
        {
            /*NFCC 2.4*/
            HAL_TRACE_DEBUG0("NFCC 2.4");
            if(GetStrValue("FW_PATCH_24", &patchfilepath[0], sizeof(patchfilepath)))
            {
                HAL_TRACE_DEBUG1("FW_PATCH_24 found: %s",patchfilepath);
            }
        }
        /* Make this flag true so that if file is available or not , next time the patch will not be read*/
        nfc_hal_cb.dev_cb.store_path = TRUE;
    }

    if(GetStrValue("FW_PRE_PATCH", &prepatchfilepath[0], sizeof(prepatchfilepath)))
    {
        HAL_TRACE_DEBUG1("FW_PRE_PATCH found: %s",prepatchfilepath);
    }
    else
    {
        HAL_TRACE_DEBUG0("FW_PRE_PATCH not found");
    }



    HAL_TRACE_DEBUG2("PATCH_UPDATE_ENABLE_FLAG= %d :: NVM_UPDATE_ENABLE_FLAG=%d",patch_update_flag,nvm_update_flag);

    HAL_TRACE_DEBUG1 ("nfc_hal_dm_proc_msg_during_init(): init state:%d", nfc_hal_cb.dev_cb.initializing_state);

    p = (UINT8 *) (p_msg + 1) + p_msg->offset;
    len = p_msg->len - 2; // removing headers
    NCI_MSG_PRS_HDR0 (p, mt, pbf, gid);
    NCI_MSG_PRS_HDR1 (p, op_code);

    /* check if waiting for this response */
    if (  (nfc_hal_cb.ncit_cb.nci_wait_rsp == NFC_HAL_WAIT_RSP_CMD)
        ||(nfc_hal_cb.ncit_cb.nci_wait_rsp == NFC_HAL_WAIT_RSP_VSC)  )
    {
        if (mt == NCI_MT_RSP)
        {
            p_old = nfc_hal_cb.ncit_cb.last_hdr;
            NCI_MSG_PRS_HDR0 (p_old, old_mt, pbf, old_gid);
            old_oid = ((*p_old) & NCI_OID_MASK);
            /* make sure this is the RSP we are waiting for before updating the command window */
            if ((old_gid == gid) && (old_oid == op_code))
            {
                nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;
                p_cback = (tNFC_HAL_NCI_CBACK *)nfc_hal_cb.ncit_cb.p_vsc_cback;
                nfc_hal_cb.ncit_cb.p_vsc_cback  = NULL;
                nfc_hal_main_stop_quick_timer (&nfc_hal_cb.ncit_cb.nci_wait_rsp_timer);
            }
        }
    }

    if (gid == NCI_GID_CORE)
    {
        if (op_code == NCI_MSG_CORE_RESET)
        {
            if (mt == NCI_MT_RSP)
            {
                if ( nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_W4_RE_INIT)
                {
                   // nfc_hal_dm_send_nci_cmd (nfc_hal_dm_get_patch_version_cmd, NCI_MSG_HDR_SIZE, nfc_hal_cb.p_reinit_cback);
                }
                else
                {
                    /*patch update mechanism*/
                    if(nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_W4_NFCC_ENABLE)
                    {
                        if(patch_update_flag == TRUE)
                        {
                            /* First find if PrePatch file is available or not and if yes then read Prepatch data*/
                            nfc_hal_cb.dev_cb.pre_patch_file_available = nfc_hal_patch_read(prepatchfilepath,&prepatchdata,&prepatchdatalen);
                            if(nfc_hal_cb.dev_cb.pre_patch_file_available)
                            {
                                HAL_TRACE_DEBUG1 ("PATCH Update: Pre patch file length is ==%d \n\n",prepatchdatalen);
                            }
                            else
                            {
                                HAL_TRACE_DEBUG0("PATCH Update: Pre patch file is not available");
                            }
                            HAL_TRACE_DEBUG1("patchfilepath: %s",patchfilepath);
                            nfc_hal_cb.dev_cb.patch_file_available = nfc_hal_patch_read(patchfilepath,&patchdata,&patchdatalen);
                            if(nfc_hal_cb.dev_cb.patch_file_available)
                            {
                                HAL_TRACE_DEBUG1 ("PATCH Update: Found file based on chip version . Patch file length is ==%d \n\n",patchdatalen);
                            }
                            else
                            {
                                /* case : if FW patch is not available at the path provided in the conf file.Then look for patch
                                          at default location*/
                                HAL_TRACE_DEBUG0("FW_PATCH path not found in config file .Reading default one");
                                /*Select patch file based on chip revision*/
                                if((nfc_hal_cb.dev_cb.nfcc_chip_version == 3) && (nfc_hal_cb.dev_cb.nfcc_chip_metal_version == 0))
                                {
                                    /*NFCC 3.0*/
                                    str_len = strlen(NFCA_PATCHFILE_V30_LOCATION);
                                    memcpy(patchfilepath,NFCA_PATCHFILE_V30_LOCATION,str_len);
                                }
                                else if((nfc_hal_cb.dev_cb.nfcc_chip_version == 2) && (nfc_hal_cb.dev_cb.nfcc_chip_metal_version == 0))
                                {
                                    /*NFCC 2.0*/
                                    str_len = strlen(NFCA_PATCHFILE_V20_LOCATION);
                                    memcpy(patchfilepath,NFCA_PATCHFILE_V20_LOCATION,str_len);
                                }
                                else if((nfc_hal_cb.dev_cb.nfcc_chip_version == 2) && (nfc_hal_cb.dev_cb.nfcc_chip_metal_version == 1))
                                {
                                    /*NFCC 2.1*/
                                    str_len = strlen(NFCA_PATCHFILE_V21_LOCATION);
                                    memcpy(patchfilepath,NFCA_PATCHFILE_V21_LOCATION,str_len);
                                }
                                else if((nfc_hal_cb.dev_cb.nfcc_chip_version == 2) && (nfc_hal_cb.dev_cb.nfcc_chip_metal_version == 4))
                                {
                                    /*NFCC 2.4*/
                                    str_len = strlen(NFCA_PATCHFILE_V24_LOCATION);
                                    memcpy(patchfilepath,NFCA_PATCHFILE_V24_LOCATION,str_len);
                                }

                                nfc_hal_cb.dev_cb.patch_file_available = nfc_hal_patch_read(patchfilepath,&patchdata,&patchdatalen);

                                if(nfc_hal_cb.dev_cb.patch_file_available)
                                {
                                    HAL_TRACE_DEBUG1 ("PATCH Update: Patch file length is ==%d \n\n",patchdatalen);
                                }
                                else
                                {
                                    HAL_TRACE_DEBUG0("PATCH Update: Patch file is not available");
                                }
                            }

                            /*Check if Prepatch file is relevent for patch file if prepatch exist */
                            if((nfc_hal_cb.dev_cb.pre_patch_file_available) && (nfc_hal_cb.dev_cb.patch_file_available))
                            {
                                patch_update = FALSE; /* Reset flag for further test*/
                                HAL_TRACE_DEBUG0("PATCH Update : Validating Prepatch and patch files");
                                patch_update = nfc_hal_patch_validate(patchdata,patchdatalen,prepatchdata,prepatchdatalen);
                            }
                            else
                            {
                                patch_update = TRUE;
                            }
                            if((patch_update == TRUE) && (nfc_hal_cb.dev_cb.patch_file_available == TRUE))
                            {
                                /* Files validated ,start patch update process*/
                                nfc_hal_cb.dev_cb.patch_applied = FALSE;
                                nfc_hal_cb.dev_cb.pre_patch_applied = FALSE;
                                HAL_TRACE_DEBUG0("PATCH Update : Validation Done, Patch Update starts..Creating Dynamic connection..");
                                NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_FOR_PATCH_DNLD);
                                nfc_hal_dm_send_nci_cmd (nfc_hal_dm_core_init_cmd, NCI_MSG_HDR_SIZE, NULL);
                            }
                            else
                            {
                                HAL_TRACE_DEBUG0("PATCH Update : Validation Failed, Patch Update Cancled..Doing normal Initialization");
                                patch_update = FALSE;
                                NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_APP_COMPLETE);
                                HAL_NfcPreInitDone (HAL_NFC_STATUS_OK);
                            }
                        }
                        else
                        {
                            HAL_TRACE_DEBUG0("Patch flag is disbaled...Preinit Done");
                            NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_APP_COMPLETE);
                            HAL_NfcPreInitDone (HAL_NFC_STATUS_OK);
                        }
                    }
                }
            }
            else
            {
                /* Call reset notification callback */
                UINT8 payload_len = *p++;
                STREAM_TO_UINT8 (reset_reason, p);
                STREAM_TO_UINT8 (reset_type, p);
                HAL_TRACE_DEBUG2("RESET NTF Recieved before Pre Init: reset_reason = %d  reset_type = %d",reset_reason,reset_type);
                /*  Note -- NFCC will reset itself  once  the  patch  data  is sent  to  it completly. So it  will  send the
                            CORE_RESET_NTF  to  DH  after  patch  is  sent and  applied .So  the rsp to  CORE_CON_CLOSE  cmd
                            will not come back on actual  target board  .
                            GEN_PROP_CMD can be sent to enquire the ECDSA signature just after RESET_NTF without sending
                            CORE_INIT again.
               */
               HAL_TRACE_DEBUG1(" nfc_hal_cb.dev_cb.initializing_state : %d",nfc_hal_cb.dev_cb.initializing_state);

               if (NCI_HAL_RAMDUMP_REASON == reset_reason)
               {
                   HAL_TRACE_DEBUG0 ("nfc_hal_dm_proc_msg_during_init: RAMDUMP: A ramdump is available");
                   reset_status = TRUE;

#ifndef NDEBUG
                   nfc_hal_nci_handle_ramdump_ntf(mt, op_code, pbf, payload_len, reset_reason);
#else
                   nfc_hal_nci_handle_ramdump_ntf();
#endif
               }
#ifdef RAMDUMP
               else
               {
                   /* Nothing to be sent back to NFCC */
                   HAL_TRACE_DEBUG1 ("nfc_hal_dm_proc_msg_during_init: RAMDUMP: NFCC initialisation NTF received. No ramdump available as expected. Reason code 0x%x", reset_reason);
               }
#endif

               if((nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_W4_PATCH_INFO))
               {
                   /*Send Gen_Prop_Cmd only when init state is NFC_HAL_INIT_FOR_PATCH_DNLD.
                     means patch download is going on*/
                   HAL_TRACE_DEBUG0(" sending prop cmd for patch signature after patch applied");
                   nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;
                   nfc_hal_dm_send_nci_cmd (nfc_hal_dm_QC_prop_cmd_patchinfo, 4, NULL);
               }
            }
        }

        if(patch_update_flag == TRUE)
        {
            if(op_code == NCI_MSG_CORE_INIT)
            {
                if (mt == NCI_MT_RSP)
                {
                    if((nvm_update_flag == TRUE) && (fused_nvm_flag == FALSE))
                    {
                        /*Check if NVM update file is available*/
                        if(nfc_hal_dm_check_nvm_file(nvmupdatebuff,&nvmdatabufflen) && (nfc_hal_cb.nvm.no_of_updates >0))
                        {
                            /*nvm update file is present . Send update before patch data*/
                            /* frame cmd now*/
                            nvmcmd= (UINT8*)malloc(nvmdatabufflen+15);
                            if(nvmcmd)
                            {
                                nfc_hal_dm_frame_mem_access_cmd(nvmcmd,nvmupdatebuff,&nvmcmdlen);
                            }
                            else
                            {
                                /*mem allocation failed */
                                return;
                            }
                            /* send nvm update cmd to NFCC*/
                            nfc_hal_cb.nvm.no_of_updates--;
                            nfc_hal_dm_send_nci_cmd (nvmcmd, nvmcmdlen, NULL);
                            free(nvmcmd);
                            nvmcmd = NULL;
                            if(nfc_hal_cb.nvm.no_of_updates == 0)
                            {
                                /* All updates sent so close file now*/
                                fclose(nfc_hal_cb.nvm.p_Nvm_file);
                                nfc_hal_cb.nvm.p_Nvm_file = NULL;
                                nfc_hal_cb.nvm.nvm_updated = TRUE;
                            }
                        }
                        else
                        {
                            /* Patch download seq going on.dynamic connection
                               creation with NFCC for patch download*/
                            HAL_TRACE_DEBUG0("PATCH Update : Sending Prop Cmd to check FW version");
                            NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_PATCH_INFO);
                            nfc_hal_cb.nvm.nvm_updated = FALSE;
                            nfc_hal_dm_send_nci_cmd (nfc_hal_dm_QC_prop_cmd_fwversion, 4, NULL);
                        }
                    }
                    else
                    {
                    /* Patch download seq going on.dynamic connection creation with NFCC for patch download*/
                    HAL_TRACE_DEBUG0("PATCH Update : Sending Prop Cmd to check FW version");
                    NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_PATCH_INFO);
                    nfc_hal_cb.nvm.nvm_updated = FALSE;
                    nfc_hal_dm_send_nci_cmd (nfc_hal_dm_QC_prop_cmd_fwversion, 4, NULL);
                    }
                }
            }
            else if(op_code == NCI_MSG_CORE_CONN_CREATE)
            {
                /* connection created so start sending patch data */
                if((!nfc_hal_cb.dev_cb.pre_patch_applied) && (nfc_hal_cb.dev_cb.pre_patch_file_available))
                {
                    HAL_TRACE_DEBUG0("PATCH Update : Sending Prepatch data");
                    pPatch_buff = (UINT8*)malloc((p[len-3]+NCI_MSG_HDR_SIZE));
                    pPatch_buff[0] = 0;
                    pPatch_buff[0] = (NCI_MT_DATA|0x10)|p[len-1];
                    pPatch_buff[1] = 0x00;
                    if(p[len-3]>0xFC)
                    {
                        pPatch_buff[2] = 0xFC;
                    }
                    else
                    {
                        pPatch_buff[2] = p[len-3];
                    }
                    // calculate number of buffers which will be sent
                    nfc_hal_cb.dev_cb.number_of_patch_data_buffers = (prepatchdatalen)% (pPatch_buff[2]);
                    if(nfc_hal_cb.dev_cb.number_of_patch_data_buffers != 0)
                    {
                        if(!((prepatchdatalen)/(pPatch_buff[2])))
                        {
                            /*This is the case when patch data length will be less than 252*/
                            len = nfc_hal_cb.dev_cb.number_of_patch_data_buffers;
                        }
                        else
                        {
                            len = pPatch_buff[2];
                        }
                        nfc_hal_cb.dev_cb.number_of_patch_data_buffers = ((prepatchdatalen)/(pPatch_buff[2])+1);
                    }
                    else
                    {
                        nfc_hal_cb.dev_cb.number_of_patch_data_buffers = ((prepatchdatalen)/(pPatch_buff[2]));
                    }

                    HAL_TRACE_DEBUG2("PATCH Update : First packet length is  %d and number of data packets are %d",len,nfc_hal_cb.dev_cb.number_of_patch_data_buffers);
                    memcpy(pPatch_buff+3,patchdata,len);
                    nfc_hal_cb.dev_cb.patch_data_offset += pPatch_buff[2];
                    nfc_hal_cb.dev_cb.number_of_patch_data_buffers--;
                    nfc_hal_send_data(pPatch_buff,(len+NCI_MSG_HDR_SIZE),NULL);
                }
                else if(!nfc_hal_cb.dev_cb.patch_applied)
                {
                    HAL_TRACE_DEBUG1("PATCH Update : Sending Patch data %d",nfc_hal_cb.dev_cb.pre_patch_file_available);
                    if(nfc_hal_cb.dev_cb.pre_patch_file_available == FALSE)
                    {
                        HAL_TRACE_DEBUG0("PATCH Update : malloc");
                        /* acquire memory only when memory has not been acquired for pre patch data send in case when it is not vailable*/
                        pPatch_buff = (UINT8*)malloc((p[len-3]+NCI_MSG_HDR_SIZE));
                    }
                    pPatch_buff[0] = 0;
                    pPatch_buff[0] = (NCI_MT_DATA|0x10)|p[len-1];           // Data packet header with PBF 1 and Conn ID
                    pPatch_buff[1] = 0x00;
                    if(p[len-3]>0xFC)
                    {
                        pPatch_buff[2] = 0xFC;
                    }
                    else
                    {
                        pPatch_buff[2] = p[len-3];
                    }
                    // calculate number of buffers which will be sent
                    nfc_hal_cb.dev_cb.number_of_patch_data_buffers = (patchdatalen)% (pPatch_buff[2]);
                    if(nfc_hal_cb.dev_cb.number_of_patch_data_buffers != 0)
                    {
                        if(!((patchdatalen)/(pPatch_buff[2])))
                        {
                            /*This is the case when patch data length will be less than 252*/
                            len = nfc_hal_cb.dev_cb.number_of_patch_data_buffers;
                        }
                        else
                        {
                            len = pPatch_buff[2];
                        }
                        nfc_hal_cb.dev_cb.number_of_patch_data_buffers = ((patchdatalen)/(pPatch_buff[2])+1);
                    }
                    else
                    {
                        nfc_hal_cb.dev_cb.number_of_patch_data_buffers = ((patchdatalen)/(pPatch_buff[2]));
                    }
                    HAL_TRACE_DEBUG2("PATCH Update : First packet length is  %d and number of data packets are %d",len,nfc_hal_cb.dev_cb.number_of_patch_data_buffers);
                    memcpy(pPatch_buff+3,patchdata,len);
                    nfc_hal_cb.dev_cb.patch_data_offset += pPatch_buff[2];
                    nfc_hal_cb.dev_cb.number_of_patch_data_buffers--;
                    nfc_hal_send_data(pPatch_buff,(len+NCI_MSG_HDR_SIZE),NULL);
                }
            }
            else if(op_code == NCI_MSG_CORE_CONN_CREDITS)
            {
                // Patch data transfered
                NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_PATCH_INFO);
                nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;
                if((!nfc_hal_cb.dev_cb.pre_patch_applied) && (nfc_hal_cb.dev_cb.pre_patch_file_available))
                {
                    if((p[len-1] == 0x01 /*NFCC_Credits_Avail*/) && (nfc_hal_cb.dev_cb.number_of_patch_data_buffers != 0))
                    {
                        if(nfc_hal_cb.dev_cb.number_of_patch_data_buffers >= 2)
                        {
                            memcpy(pPatch_buff+3,(prepatchdata+nfc_hal_cb.dev_cb.patch_data_offset),pPatch_buff[2]);
                            nfc_hal_cb.dev_cb.number_of_patch_data_buffers--;
                            nfc_hal_cb.dev_cb.patch_data_offset += pPatch_buff[2];
                            nfc_hal_send_data(pPatch_buff,(pPatch_buff[2]+NCI_MSG_HDR_SIZE),NULL);
                        }
                        else
                        {
                            /*calculate last buffer length*/
                            len = (prepatchdatalen)% (pPatch_buff[2]);
                            *pPatch_buff &= 0x01;   //last packet
                            *(pPatch_buff+2) = len;
                            memcpy(pPatch_buff + 3, (prepatchdata+nfc_hal_cb.dev_cb.patch_data_offset), len);
                            HAL_TRACE_DEBUG2("PATCH Update : Last packet length is  %d and number of data packets are %d",len,nfc_hal_cb.dev_cb.number_of_patch_data_buffers);
                            nfc_hal_cb.dev_cb.number_of_patch_data_buffers--;
                            nfc_hal_send_data(pPatch_buff,(pPatch_buff[2]+NCI_MSG_HDR_SIZE),NULL);
                            /* close connection now*/
                            nfc_hal_cb.dev_cb.pre_patch_applied = FALSE;
                            nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;
                            HAL_TRACE_DEBUG0("PATCH Update : Last packet sent.Close connection");
                            nfc_hal_dm_send_nci_cmd (nfc_hal_dm_core_con_close_cmd, 4, NULL);
                        }
                    }
                }
                else
                {
                    /* Further patch data send*/
                    if((p[len-1] == 0x01 /*NFCC_Credits_Avail*/) && (nfc_hal_cb.dev_cb.number_of_patch_data_buffers != 0))
                    {
                        if(nfc_hal_cb.dev_cb.number_of_patch_data_buffers >= 2)
                        {
                            memcpy(pPatch_buff+3,(patchdata+nfc_hal_cb.dev_cb.patch_data_offset),pPatch_buff[2]);
                            nfc_hal_cb.dev_cb.number_of_patch_data_buffers--;
                            nfc_hal_cb.dev_cb.patch_data_offset += pPatch_buff[2];
                            nfc_hal_send_data(pPatch_buff,(pPatch_buff[2]+NCI_MSG_HDR_SIZE),NULL);
                        }
                        else
                        {
                            /*calculate last buffer length*/
                            len = (patchdatalen)% (pPatch_buff[2]);
                            *pPatch_buff &= 0x01;   //last packet
                            *(pPatch_buff+2)= len ;
                            memcpy(pPatch_buff+3,(patchdata+nfc_hal_cb.dev_cb.patch_data_offset),len);
                            HAL_TRACE_DEBUG2("PATCH Update : Last packet length is  %d and number of data packets are %d",len,nfc_hal_cb.dev_cb.number_of_patch_data_buffers);
                            nfc_hal_cb.dev_cb.number_of_patch_data_buffers--;
                            nfc_hal_send_data(pPatch_buff,(pPatch_buff[2]+NCI_MSG_HDR_SIZE),NULL);
                            /* close connection now*/
                            nfc_hal_cb.dev_cb.patch_applied = FALSE;
                            HAL_TRACE_DEBUG0("PATCH Update : Last packet sent.Close connection");
                            nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;
                            nfc_hal_cb.dev_cb.patch_dnld_conn_close_delay = TRUE;
                            nfc_hal_dm_send_nci_cmd (nfc_hal_dm_core_con_close_cmd, 4, NULL);
                        }
                    }
                }
            }
            else if(op_code == NCI_MSG_CORE_CONN_CLOSE)
            {
               HAL_TRACE_DEBUG0("PATCH Update : Pre patch data sending dynamic connection closed...Sending prop cmd to check pre patch signature again");
            }
        }
        else
        {
            if (p_cback)
            {
                (*p_cback) ((tNFC_HAL_NCI_EVT) (op_code),
                            p_msg->len,
                            (UINT8 *) (p_msg + 1) + p_msg->offset);
            }
        }
    }

    if(patch_update_flag == TRUE)
    {
        if (gid == NCI_GID_PROP) /* this is for download patch */
        {
            if((nvm_update_flag == TRUE) && (fused_nvm_flag == FALSE))
            {
                /* check if more nvm updates are to be sent */
                if(nfc_hal_cb.nvm.no_of_updates > 0)
                {
                    if(nfc_hal_dm_check_nvm_file(nvmupdatebuff,&nvmdatabufflen) && (nfc_hal_cb.nvm.no_of_updates > 0))
                    {
                        /* frame cmd now*/
                        nvmcmd= (UINT8*)malloc(nvmdatabufflen+15);
                        if(nvmcmd)
                        {
                            nfc_hal_dm_frame_mem_access_cmd(nvmcmd,nvmupdatebuff,&nvmcmdlen);
                            /* send nvm update cmd to NFCC*/
                            nfc_hal_cb.nvm.no_of_updates--;
                            nfc_hal_dm_send_nci_cmd (nvmcmd, nvmcmdlen, NULL);
                            free(nvmcmd);
                            nvmcmd = NULL;
                            if(nfc_hal_cb.nvm.no_of_updates == 0)
                            {
                                /* All updates sent so close file now*/
                                fclose(nfc_hal_cb.nvm.p_Nvm_file);
                                nfc_hal_cb.nvm.p_Nvm_file = NULL;
                                nfc_hal_cb.nvm.nvm_updated = TRUE;
                            }
                        }
                        return;
                    }
                }
            }
            if(nfc_hal_cb.nvm.nvm_updated == TRUE)
            {
                /* Patch download seq going on after nvm update.dynamic connection creation with NFCC for patch download*/
                HAL_TRACE_DEBUG0("PATCH Update : Sending Prop Cmd to check patch signature");
                NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_PATCH_INFO);
                nfc_hal_cb.nvm.nvm_updated = FALSE;
                nfc_hal_cb.dev_cb.fw_version_chk = TRUE;
                nfc_hal_dm_send_nci_cmd (nfc_hal_dm_QC_prop_cmd_patchinfo, 4, NULL);
                return;
            }

            if(!nfc_hal_cb.dev_cb.patch_applied)
            {
                if(nfc_hal_cb.dev_cb.fw_version_chk)
                {
                    if((!nfc_hal_cb.dev_cb.pre_patch_applied) && (nfc_hal_cb.dev_cb.pre_patch_file_available))
                    {
                        HAL_TRACE_DEBUG0("PATCH Update : Checking Prepatch Signature");
                        patch_update = nfc_hal_check_fw_signature(p,len,prepatchdata,prepatchdatalen);
                        nfc_hal_cb.dev_cb.pre_patch_signature_chk = TRUE;
                    }
                    else
                    {
                        /*Pre patch applied .Now checking patch signature*/
                        HAL_TRACE_DEBUG0("PATCH Update : Checking patch Signature");
                        patch_update = nfc_hal_check_fw_signature(p,len,patchdata,patchdatalen);
                        nfc_hal_cb.dev_cb.patch_signature_chk = TRUE;
                    }
                    if(patch_update == PATCH_NOT_UPDATED)
                    {
                        /*create a dynamic logical connection with the NFCC to update the pre patch*/
                        if(nfc_hal_cb.dev_cb.pre_patch_file_available)
                        {
                            HAL_TRACE_DEBUG0("PATCH Update : Prepatch file signature different..Create dynamic connection to update prepatch");
                        }
                        else
                        {
                            HAL_TRACE_DEBUG0("PATCH Update : Patch file signature different..Create dynamic connection to update patch");
                        }
                        NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_FOR_PATCH_DNLD);
                        nfc_hal_dm_send_nci_cmd (nfc_hal_dm_core_con_create_cmd, 8, NULL);
                    }
                    else
                    {
                        /* prepatch applied successfully .open connection to apply patch mow */
                        if((!nfc_hal_cb.dev_cb.pre_patch_applied) && (nfc_hal_cb.dev_cb.pre_patch_file_available))
                        {
                            HAL_TRACE_DEBUG0("PATCH Update : Prepatch applied successfully..Create dynamic connection to update patch");
                            nfc_hal_cb.dev_cb.pre_patch_applied = TRUE;
                            nfc_hal_cb.dev_cb.number_of_patch_data_buffers = 0;
                            nfc_hal_cb.dev_cb.patch_data_offset = 0;
                            NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_FOR_PATCH_DNLD);
                            nfc_hal_dm_send_nci_cmd (nfc_hal_dm_core_con_create_cmd, 8, NULL);
                        }
                        else
                        {
                            HAL_TRACE_DEBUG0("PATCH Update : Patch applied successfully..Pre Init Done");
                            nfc_hal_cb.dev_cb.patch_applied = FALSE;
                            nfc_hal_cb.dev_cb.pre_init_done = TRUE;
                            nfc_hal_cb.dev_cb.pre_patch_signature_chk = FALSE;
                            nfc_hal_cb.dev_cb.patch_signature_chk = FALSE;
                            nfc_hal_cb.dev_cb.patch_data_offset = 0;
                            nfc_hal_cb.dev_cb.number_of_patch_data_buffers = 0;
                            if ( pPatch_buff != NULL)
                            {
                                free(pPatch_buff);
                                pPatch_buff = NULL;
                            }
                            if ( prepatchdata != NULL)
                            {
                                free(prepatchdata);
                                prepatchdata = NULL;
                            }
                            if ( patchdata != NULL)
                            {
                                free(patchdata);
                                patchdata = NULL;
                            }
                            prepatchdatalen = 0;
                            patchdatalen = 0;
                            NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_APP_COMPLETE);
                            HAL_NfcPreInitDone (HAL_NFC_STATUS_OK);
                        }
                    }
                }
                if(!nfc_hal_cb.dev_cb.fw_version_chk)
                {
                    HAL_TRACE_DEBUG0("PATCH Update : Checking FW Version");
                    patch_update = nfc_hal_check_firmware_version(p,len,patchdata,patchdatalen);
                    if(patch_update)
                    {
                        /*Now send again the prop command to enquire about the patch info from NFCC*/
                        HAL_TRACE_DEBUG0("PATCH Update : FW Version matched..Sending prop cmd again to verify signature");
                        nfc_hal_cb.dev_cb.fw_version_chk = TRUE;
                        patch_update = FALSE;          /*Reset flag for further signature test*/
                        NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_PATCH_INFO);
                        nfc_hal_dm_send_nci_cmd (nfc_hal_dm_QC_prop_cmd_patchinfo, 4, NULL);
                    }
                    else
                    {
                        /*FW version is different from th NFCC.Wrong file.Discard it and continue with normal initialization*/
                        HAL_TRACE_DEBUG0("PATCH Update : FW Version not matched..Start normal initialization");
                        nfc_hal_cb.dev_cb.pre_patch_applied = FALSE;
                        nfc_hal_cb.dev_cb.patch_applied = FALSE;
                        nfc_hal_cb.dev_cb.pre_init_done = TRUE;
                        nfc_hal_cb.dev_cb.pre_patch_signature_chk = FALSE;
                        nfc_hal_cb.dev_cb.patch_signature_chk = FALSE;
                        nfc_hal_cb.dev_cb.patch_data_offset = 0;
                        nfc_hal_cb.dev_cb.number_of_patch_data_buffers = 0;
                        if (pPatch_buff != NULL)
                        {
                            free(pPatch_buff);
                            pPatch_buff = NULL;
                        }
                        if (prepatchdata != NULL)
                        {
                            free(prepatchdata);
                            prepatchdata = NULL;
                        }
                        if (patchdata != NULL)
                        {
                            free(patchdata);
                            patchdata = NULL;
                        }
                        prepatchdatalen = 0;
                        patchdatalen = 0;
                        NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_APP_COMPLETE);
                        HAL_NfcPreInitDone (HAL_NFC_STATUS_OK);
                    }
                }
                if(nfc_hal_cb.dev_cb.pre_init_done == TRUE)
                {
                    nfc_hal_cb.dev_cb.fw_version_chk = FALSE;
                }
            }
        }
    }

    if (mt == NCI_MT_NTF)
        op_code |= NCI_NTF_BIT;
    else
        op_code |= NCI_RSP_BIT;

    if (  (op_code == NFC_VS_GET_BUILD_INFO_EVT)
          &&(nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_W4_BUILD_INFO)  )
    {
        p += NCI_BUILD_INFO_OFFSET_HWID;
        STREAM_TO_UINT32 (nfc_hal_cb.dev_cb.m_hw_id, p);
        NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_PATCH_INFO);
        nfc_hal_dm_send_nci_cmd (nfc_hal_dm_get_patch_version_cmd, NCI_MSG_HDR_SIZE, NULL);
    }
    else if (  (op_code == NFC_VS_GET_PATCH_VERSION_EVT)
               &&(nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_W4_PATCH_INFO)  )
    {
        p += NCI_PATCH_INFO_OFFSET_NVMTYPE;
        NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_APP_COMPLETE);
        nfc_hal_post_reset_init (nfc_hal_cb.dev_cb.m_hw_id, *p);
    }
    else if (p_cback)
    {
        (*p_cback) ((tNFC_HAL_NCI_EVT) (op_code),
                    p_msg->len,
                    (UINT8 *) (p_msg + 1) + p_msg->offset);
    }
    else if (op_code == NFC_VS_SEC_PATCH_AUTH_EVT)
    {
        HAL_TRACE_DEBUG0 ("signature!!");
        nfc_hal_prm_nci_command_complete_cback ((tNFC_HAL_NCI_EVT) (op_code),
                                                p_msg->len,
                                                (UINT8 *) (p_msg + 1) + p_msg->offset);
    }
}

/*******************************************************************************
**
** Function         nfc_hal_dm_send_nci_cmd
**
** Description      Send NCI command to NFCC while initializing NFCC
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_send_nci_cmd (const UINT8 *p_data, UINT16 len, tNFC_HAL_NCI_CBACK *p_cback)
{
    NFC_HDR *p_buf;
    UINT8  *ps;
    UINT32 cmd_timeout_val=0;

    HAL_TRACE_DEBUG1 ("nfc_hal_dm_send_nci_cmd (): nci_wait_rsp = 0x%x", nfc_hal_cb.ncit_cb.nci_wait_rsp);

    if (nfc_hal_cb.ncit_cb.nci_wait_rsp != NFC_HAL_WAIT_RSP_NONE)
    {
        HAL_TRACE_ERROR0 ("nfc_hal_dm_send_nci_cmd(): no command window");
        return;
    }
    p_buf = (NFC_HDR *)GKI_getpoolbuf (NFC_HAL_NCI_POOL_ID);
    if (p_buf != NULL)
    {
        nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_VSC;

        p_buf->offset = NFC_HAL_NCI_MSG_OFFSET_SIZE;
        p_buf->event  = NFC_HAL_EVT_TO_NFC_NCI;
        p_buf->len    = len;

        memcpy ((UINT8*) (p_buf + 1) + p_buf->offset, p_data, len);

        /* Keep a copy of the command and send to NCI transport */

        /* save the message header to double check the response */
        ps   = (UINT8 *)(p_buf + 1) + p_buf->offset;
        memcpy(nfc_hal_cb.ncit_cb.last_hdr, ps, NFC_HAL_SAVED_HDR_SIZE);
        memcpy(nfc_hal_cb.ncit_cb.last_cmd, ps + NCI_MSG_HDR_SIZE, NFC_HAL_SAVED_CMD_SIZE);

        /* save the callback for NCI VSCs */
        nfc_hal_cb.ncit_cb.p_vsc_cback = (void *)p_cback;

        nfc_hal_nci_send_cmd (p_buf);

        if(nfc_hal_cb.dev_cb.patch_dnld_conn_close_delay)
        {
            GetNumValue("PATCH_DNLD_NFC_HAL_CMD_TOUT", &cmd_timeout_val, sizeof(cmd_timeout_val));
            if(cmd_timeout_val)
            {
                HAL_TRACE_DEBUG1 ("nfc_hal_dm_send_nci_cmd (): cmd_timeout_val=%d",cmd_timeout_val);
                /* start NFC command-timeout timer for patch download with timeout value from conf file*/
                nfc_hal_main_start_quick_timer (&nfc_hal_cb.ncit_cb.nci_wait_rsp_timer, (UINT16)(NFC_HAL_TTYPE_NCI_WAIT_RSP),
                                            (cmd_timeout_val) * QUICK_TIMER_TICKS_PER_SEC / 1000);
            }
            else
            {
                /* start NFC command-timeout timer for patch download with default timeout value of 4 sec*/
                nfc_hal_main_start_quick_timer (&nfc_hal_cb.ncit_cb.nci_wait_rsp_timer, (UINT16)(NFC_HAL_TTYPE_NCI_WAIT_RSP),
                                            ((UINT32) NFC_HAL_CMD_TOUT*2) * QUICK_TIMER_TICKS_PER_SEC / 1000);
            }
            nfc_hal_cb.dev_cb.patch_dnld_conn_close_delay = FALSE;
        }
        else
        {
            /* start NFC command-timeout timer */
            nfc_hal_main_start_quick_timer (&nfc_hal_cb.ncit_cb.nci_wait_rsp_timer, (UINT16)(NFC_HAL_TTYPE_NCI_WAIT_RSP),
                                        ((UINT32) NFC_HAL_CMD_TOUT) * QUICK_TIMER_TICKS_PER_SEC / 1000);
        }
    }
}
/*******************************************************************************
**
** Function         nfc_hal_dm_send_pend_cmd
**
** Description      Send a command to NFCC
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_send_pend_cmd (void)
{
    NFC_HDR *p_buf = nfc_hal_cb.ncit_cb.p_pend_cmd;
    UINT8  *p;

    if (p_buf == NULL)
        return;

    /* check low power mode state */
    if (!nfc_hal_dm_power_mode_execute (NFC_HAL_LP_TX_DATA_EVT))
    {
        return;
    }

    if (nfc_hal_cb.ncit_cb.nci_wait_rsp == NFC_HAL_WAIT_RSP_PROP)
    {
#if (NFC_HAL_TRACE_PROTOCOL == TRUE)
        DispHciCmd (p_buf);
#endif

        /* save the message header to double check the response */
        p = (UINT8 *)(p_buf + 1) + p_buf->offset;
        memcpy(nfc_hal_cb.ncit_cb.last_hdr, p, NFC_HAL_SAVED_HDR_SIZE);

        /* add packet type for BT message */
        p_buf->offset--;
        p_buf->len++;

        p  = (UINT8 *) (p_buf + 1) + p_buf->offset;
        *p = HCIT_TYPE_COMMAND;

        DT_Nfc_Write (USERIAL_NFC_PORT, p, p_buf->len);

        GKI_freebuf (p_buf);
        nfc_hal_cb.ncit_cb.p_pend_cmd = NULL;

        /* start NFC command-timeout timer */
        nfc_hal_main_start_quick_timer (&nfc_hal_cb.ncit_cb.nci_wait_rsp_timer, (UINT16)(NFC_HAL_TTYPE_NCI_WAIT_RSP),
                                        ((UINT32) NFC_HAL_CMD_TOUT) * QUICK_TIMER_TICKS_PER_SEC / 1000);

    }
}

/*******************************************************************************
**
** Function         nfc_hal_dm_send_bt_cmd
**
** Description      Send BT message to NFCC while initializing NFCC
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_send_bt_cmd (const UINT8 *p_data, UINT16 len, tNFC_HAL_BTVSC_CPLT_CBACK *p_cback)
{
    NFC_HDR *p_buf;

    HAL_TRACE_DEBUG1 ("nfc_hal_dm_send_bt_cmd (): nci_wait_rsp = 0x%x", nfc_hal_cb.ncit_cb.nci_wait_rsp);

    if (nfc_hal_cb.ncit_cb.nci_wait_rsp != NFC_HAL_WAIT_RSP_NONE)
    {
        HAL_TRACE_ERROR0 ("nfc_hal_dm_send_bt_cmd(): no command window");
        return;
    }
    p_buf = (NFC_HDR *) GKI_getpoolbuf (NFC_HAL_NCI_POOL_ID);
    if (p_buf != NULL)
    {
        nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_PROP;

        p_buf->offset = NFC_HAL_NCI_MSG_OFFSET_SIZE;
        p_buf->len    = len;

        memcpy ((UINT8*) (p_buf + 1) + p_buf->offset, p_data, len);

        /* save the callback for NCI VSCs)  */
        nfc_hal_cb.ncit_cb.p_vsc_cback = (void *)p_cback;

        nfc_hal_cb.ncit_cb.p_pend_cmd = p_buf;
        if (nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_IDLE)
        {
            NFC_HAL_SET_INIT_STATE(NFC_HAL_INIT_STATE_W4_CONTROL_DONE);
            nfc_hal_cb.p_stack_cback (HAL_NFC_REQUEST_CONTROL_EVT, HAL_NFC_STATUS_OK);
            return;
        }

        nfc_hal_dm_send_pend_cmd();
    }
}

/*******************************************************************************
**
** Function         nfc_hal_dm_set_nfc_wake
**
** Description      Set NFC_WAKE line
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_set_nfc_wake (UINT8 cmd)
{
    HAL_TRACE_DEBUG1 ("nfc_hal_dm_set_nfc_wake () %s",
                      (cmd == NFC_HAL_ASSERT_NFC_WAKE ? "ASSERT" : "DEASSERT"));

    if (cmd == NFC_HAL_ASSERT_NFC_WAKE)
    {
        HAL_TRACE_DEBUG1 ("NFC_HAL_ASSERT_NFC_WAKE() cmd = %d", cmd);
        if(nfc_hal_cb.dev_cb.nfcc_sleep_mode == SLEEP_STATE)
        {
            DT_Set_Power(NFCC_REG_WAKE);
            HAL_TRACE_DEBUG0 ("Nfcc Wake up done.");
            nfc_hal_cb.dev_cb.nfcc_sleep_mode = WAKE_STATE;
        }
    }
    else if (cmd == NFC_HAL_DEASSERT_NFC_WAKE)
    {
        HAL_TRACE_DEBUG1 ("NFC_HAL_DEASSERT_NFC_WAKE() cmd = %d", cmd);
        nfc_hal_dm_send_prop_sleep_cmd ();
    }
    else
    {
        nfc_hal_cb.dev_cb.nfcc_sleep_mode = WAKE_STATE;
        HAL_TRACE_DEBUG1 ("nfc_hal_dm_set_nfc_wake() cmd = %d", cmd);
    }
}

/*******************************************************************************
**
** Function         nfc_hal_dm_power_mode_execute
**
** Description      If snooze mode is enabled in full power mode,
**                     Assert NFC_WAKE before sending data
**                     Deassert NFC_WAKE when idle timer expires
**
** Returns          TRUE if DH can send data to NFCC
**
*******************************************************************************/
BOOLEAN nfc_hal_dm_power_mode_execute (tNFC_HAL_LP_EVT event)
{
    BOOLEAN send_to_nfcc = FALSE;

    HAL_TRACE_DEBUG1 ("nfc_hal_dm_power_mode_execute () event = %d", event);

    if (nfc_hal_cb.dev_cb.power_mode == NFC_HAL_POWER_MODE_FULL || nfc_hal_cb.dev_cb.nfcc_sleep_mode )
    {
        /* if idle timer is not running */
        if ( nfc_hal_cb.dev_cb.nfcc_sleep_mode == SLEEP_STATE)
        {
            HAL_TRACE_DEBUG1 ("nfc_hal_dm_power_mode_execute ()wake up = %d", NFC_HAL_ASSERT_NFC_WAKE);
            nfc_hal_dm_set_nfc_wake (NFC_HAL_ASSERT_NFC_WAKE);
        }
        send_to_nfcc = TRUE;
    }
    return send_to_nfcc;
}

/*******************************************************************************
**
** Function         nci_nfc_lp_timeout_cback
**
** Description      callback function for low power timeout
**
** Returns          void
**
*******************************************************************************/
static void nci_nfc_lp_timeout_cback (void *p_tle)
{
    HAL_TRACE_DEBUG0 ("nci_nfc_lp_timeout_cback ()");

    nfc_hal_dm_power_mode_execute (NFC_HAL_LP_TIMEOUT_EVT);
}
/*********************************************************************************************
**
** Function         nfc_hal_dm_send_prop_nci_region2_control_enable_cmd
**
** Description      Sends NCI command to NFCC to inform that region2 enable/disable
**                  will be controlled by DH.
**
** Returns          void
**
*********************************************************************************************/
void nfc_hal_dm_send_prop_nci_region2_control_enable_cmd(UINT8 debaug_status)
{
    switch(debaug_status)
    {
        case REGION2_CONTROL_ENABLE:
            nfc_hal_dm_send_nci_cmd(nfc_hal_dm_prop_region2_control_enable, NCI_MSG_HDR_SIZE+2, NULL);
            break;
        case REGION2_CONTROL_DISABLE:
            nfc_hal_dm_send_nci_cmd(nfc_hal_dm_prop_region2_control_disable, NCI_MSG_HDR_SIZE+2, NULL);
            break;
    }
}

/*******************************************************************************
**
** Function         nfc_hal_dm_send_prop_nci_region2_enable_cmd
**
** Description      Sends NCI command to NFCC to put it in Region 2
**                  operating mode.
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_send_prop_nci_region2_enable_cmd(UINT8 debaug_status)
{

    switch(debaug_status)
    {
        case REGION2_DEBUG_ENABLE:
            nfc_hal_dm_send_nci_cmd(nfc_hal_dm_prop_region2_enable_debug_on_cmd, NCI_MSG_HDR_SIZE+2, NULL);
            break;
        case REGION2_DEBUG_DISABLE:
            nfc_hal_dm_send_nci_cmd(nfc_hal_dm_prop_region2_enable_debug_off_cmd, NCI_MSG_HDR_SIZE+2, NULL);
            break;
    }
    nfc_hal_cb.propd_sleep = SLEEP_ON;
}

/****************************************************************************************************
**
** Function         nfc_hal_store_info
**
** Description      This will store region 2 enable information in a binary  file /data/nfc/ dir
**                  in android file system.
**
** Returns          void
**
******************************************************************************************************/
void nfc_hal_store_info(UINT8 operation)
{
    FILE * pFile;
    char buffer[1];
    switch(operation)
    {
        case STORE_INFO_DEBUG_ENABLE:
            buffer[0] = '1';
            HAL_TRACE_DEBUG0 ("Setting Region2 enable info");
            break;
        case DEVICE_POWER_CYCLED:
            HAL_TRACE_DEBUG0 ("Setting Region2 disable info");
            buffer[0] = '0';
            break;
        case STROE_INFO_NFC_DISABLED:
            HAL_TRACE_DEBUG0 ("Setting nfc disabled by User info");
            buffer[0] = '2';
            break;
        case NFCSERVICE_WATCHDOG_TIMER_EXPIRED:
            HAL_TRACE_DEBUG0 ("TEST111");
            buffer[0] = '4';
            break;
        default:
            break;
    }
    pFile = fopen ("/data/nfc/nvstorage.bin", "wb");
    if(pFile != NULL)
    {
        fwrite (buffer , sizeof(char), sizeof(buffer), pFile);
        fclose (pFile);
    }
    else
    {
        HAL_TRACE_DEBUG0 ("/data/nfc/nvstorage.bin open failed");
    }
}

/***********************************************************************************************************
**
** Function         nfc_hal_retrieve_info
**
** Description      This will retrieve region 2 enable/disable and shutdown reason information in a binary  file /data/nfc/ dir
**                  in android file system.
**
** Returns          stored value
**
***********************************************************************************************************/
char nfc_hal_retrieve_info(void)
{
    FILE * pFile;
    long lSize;
    char * buffer;
    char ret_val = 0;
    size_t result;

    pFile = fopen ("/data/nfc/nvstorage.bin" , "rb");
    if (pFile==NULL)
    {
        HAL_TRACE_DEBUG0 ("/data/nfc/nvstorage.bin file open failed");
       return ERROR;
    }
    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
    rewind (pFile);

    buffer = (char*) malloc (sizeof(char)*lSize);
    if (buffer == NULL)
    {
        HAL_TRACE_DEBUG0 ("mem allocation failed ");
        fclose (pFile);
        return ERROR;
    }

    result = fread (buffer,1,lSize,pFile);
    if (result != lSize)
    {
        HAL_TRACE_DEBUG0 ("Read opration failed");
    }
    ret_val = buffer[0];

    HAL_TRACE_DEBUG1 ("%d ",buffer[0]);

    fclose (pFile);
    free (buffer);
    return ret_val;
}

/*******************************************************************************
**
** Function         nfc_hal_dm_pre_init_nfcc
**
** Description      This function initializes Broadcom specific control blocks for
**                  NCI transport
**
** Returns          void
**
*******************************************************************************/

void nfc_hal_dm_pre_init_nfcc (void)
{
    int sem_status;
    UINT8  stored_info = 0;
    UINT32 region2_enable = 0, debug_enable = 0;
    HAL_TRACE_DEBUG0 ("nfc_hal_dm_pre_init_nfcc ()");

    stored_info = nfc_hal_retrieve_info();
    HAL_TRACE_DEBUG1("stored_info=%d ",stored_info);
    if((reset_status != TRUE) || (stored_info == NFCSERVICE_WATCHDOG_TIMER_EXPIRED))
    {
        if((stored_info == STORE_INFO_DEBUG_ENABLE) ||
           (stored_info == STROE_INFO_NFC_DISABLED) ||
           (stored_info == NFCSERVICE_WATCHDOG_TIMER_EXPIRED))
        {
            if(nfc_hal_cb.dev_cb.nfcc_sleep_mode != SLEEP_STATE)
            {
                /*Since wake to be done but last time DH has not sent wake up so set nfcc_sleep_mode to
                  so wake up can be done*/
                HAL_TRACE_DEBUG0("nfc_hal_cb.dev_cb.nfcc_sleep_mode = SLEEP_STATE");
                nfc_hal_cb.dev_cb.nfcc_sleep_mode = SLEEP_STATE;
            }
            nfc_hal_dm_set_nfc_wake (NFC_HAL_ASSERT_NFC_WAKE);
            GKI_delay(10);
            /*Write 0 again in nv file to indicate that nfc disable info
              has been read which was updated last time*/
            nfc_hal_store_info(REMOVE_INFO);
        }
    }
    HAL_TRACE_DEBUG0 ("nfc_hal_dm_pre_init_nfcc () send_reset_cmd ");
    nfc_hal_dm_send_reset_cmd ();
    wait_reset_rsp = TRUE;
    nfc_post_reset_cb.dev_init_config.flags = 1;
}

/*******************************************************************************
**
** Function         nfc_hal_dm_shutting_down_nfcc
**
** Description      This function initializes Broadcom specific control blocks for
**                  NCI transport
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_shutting_down_nfcc (void)
{
    UINT32 debug_enable = 0,region2_enable = 0;
    UINT8 status = 0,region2_info_test = 0;
    HAL_TRACE_DEBUG0 ("nfc_hal_dm_shutting_down_nfcc ()");

    nfc_hal_cb.dev_cb.initializing_state = NFC_HAL_INIT_STATE_CLOSING;

    nfc_hal_cb.ncit_cb.nci_wait_rsp = NFC_HAL_WAIT_RSP_NONE;
    nfc_hal_cb.hci_cb.hcp_conn_id = 0;

    nfc_hal_cb.dev_cb.power_mode  = NFC_HAL_POWER_MODE_FULL;
    nfc_hal_cb.dev_cb.snooze_mode = NFC_HAL_LP_SNOOZE_MODE_NONE;

    /* Stop all timers */
    nfc_hal_main_stop_quick_timer (&nfc_hal_cb.ncit_cb.nci_wait_rsp_timer);
    nfc_hal_main_stop_quick_timer (&nfc_hal_cb.dev_cb.lp_timer);
    nfc_hal_main_stop_quick_timer (&nfc_hal_cb.prm.timer);
    nfc_hal_main_stop_quick_timer (&nfc_hal_cb.hci_cb.hci_timer);
    nfc_hal_main_stop_quick_timer (&nfc_hal_cb.timer);
}

/*******************************************************************************
**
** Function         nfc_hal_dm_init
**
** Description      This function initializes Broadcom specific control blocks for
**                  NCI transport
**
** Returns          void
**
*******************************************************************************/
void nfc_hal_dm_init (void)
{
    HAL_TRACE_DEBUG0 ("nfc_hal_dm_init ()");

    nfc_hal_cb.dev_cb.lp_timer.p_cback = nci_nfc_lp_timeout_cback;

    nfc_hal_cb.ncit_cb.nci_wait_rsp_timer.p_cback = nfc_hal_nci_cmd_timeout_cback;

    nfc_hal_cb.hci_cb.hci_timer.p_cback = nfc_hal_hci_timeout_cback;

    nfc_hal_cb.pre_discover_done        = FALSE;

    nfc_post_reset_cb.spd_nvm_detection_cur_count = 0;
    nfc_post_reset_cb.spd_skip_on_power_cycle     = FALSE;

}

/*******************************************************************************
**
** Function         HAL_NfcDevInitDone
**
** Description      Notify that pre-initialization of NFCC is complete
**
** Returns          void
**
*******************************************************************************/
void HAL_NfcPreInitDone (tHAL_NFC_STATUS status)
{
    HAL_TRACE_DEBUG1 ("HAL_NfcPreInitDone () status=%d", status);

    if (nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_W4_APP_COMPLETE)
    {
        NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_IDLE);

        nfc_hal_main_pre_init_done (status);
    }
}

/*******************************************************************************
**
** Function         HAL_NfcReInit
**
** Description      This function is called to restart initialization after REG_PU
**                  toggled because of failure to detect NVM type or download patchram.
**
** Note             This function should be called only during the HAL init process
**
** Returns          HAL_NFC_STATUS_OK if successfully initiated
**                  HAL_NFC_STATUS_FAILED otherwise
**
*******************************************************************************/
tHAL_NFC_STATUS HAL_NfcReInit (void)
{
    tHAL_NFC_STATUS status = HAL_NFC_STATUS_FAILED;

    HAL_TRACE_DEBUG1 ("HAL_NfcReInit () init st=0x%x", nfc_hal_cb.dev_cb.initializing_state);
    if (nfc_hal_cb.dev_cb.initializing_state == NFC_HAL_INIT_STATE_W4_APP_COMPLETE)
    {
        {
            /* Wait for NFCC to enable - Core reset notification */
            NFC_HAL_SET_INIT_STATE (NFC_HAL_INIT_STATE_W4_NFCC_ENABLE);

        }

        status = HAL_NFC_STATUS_OK;
    }
    return status;
}

/*******************************************************************************
**
** Function         nfc_hal_dm_set_snooze_mode_cback
**
** Description      This is baud rate update complete callback.
**
** Returns          void
**
*******************************************************************************/
static void nfc_hal_dm_set_snooze_mode_cback (tNFC_HAL_BTVSC_CPLT *pData)
{
    UINT8             status = pData->p_param_buf[0];
    tHAL_NFC_STATUS   hal_status;
    tHAL_NFC_STATUS_CBACK *p_cback;

    /* if it is completed */
    if (status == HCI_SUCCESS)
    {
        /* update snooze mode */
        nfc_hal_cb.dev_cb.snooze_mode = nfc_hal_cb.dev_cb.new_snooze_mode;

        nfc_hal_dm_set_nfc_wake (NFC_HAL_ASSERT_NFC_WAKE);

        if ( nfc_hal_cb.dev_cb.snooze_mode != NFC_HAL_LP_SNOOZE_MODE_NONE)
        {
            /* start idle timer */
            nfc_hal_main_start_quick_timer (&nfc_hal_cb.dev_cb.lp_timer, 0x00,
                                            ((UINT32) NFC_HAL_LP_IDLE_TIMEOUT) * QUICK_TIMER_TICKS_PER_SEC / 1000);
        }
        else
        {
            nfc_hal_main_stop_quick_timer (&nfc_hal_cb.dev_cb.lp_timer);
        }
        hal_status = HAL_NFC_STATUS_OK;
    }
    else
    {
        hal_status = HAL_NFC_STATUS_FAILED;
    }

    if (nfc_hal_cb.dev_cb.p_prop_cback)
    {
        p_cback = nfc_hal_cb.dev_cb.p_prop_cback;
        nfc_hal_cb.dev_cb.p_prop_cback = NULL;
        (*p_cback) (hal_status);
    }
}

/*******************************************************************************
**
** Function         HAL_NfcSetSnoozeMode
**
** Description      Set snooze mode
**                  snooze_mode
**                      NFC_HAL_LP_SNOOZE_MODE_NONE - Snooze mode disabled
**                      NFC_HAL_LP_SNOOZE_MODE_UART - Snooze mode for UART
**                      NFC_HAL_LP_SNOOZE_MODE_SPI_I2C - Snooze mode for SPI/I2C
**
**                  idle_threshold_dh/idle_threshold_nfcc
**                      Idle Threshold Host in 100ms unit
**
**                  nfc_wake_active_mode/dh_wake_active_mode
**                      NFC_HAL_LP_ACTIVE_LOW - high to low voltage is asserting
**                      NFC_HAL_LP_ACTIVE_HIGH - low to high voltage is asserting
**
**                  p_snooze_cback
**                      Notify status of operation
**
** Returns          tHAL_NFC_STATUS
**
*******************************************************************************/
tHAL_NFC_STATUS HAL_NfcSetSnoozeMode (UINT8 snooze_mode,
                                      UINT8 idle_threshold_dh,
                                      UINT8 idle_threshold_nfcc,
                                      UINT8 nfc_wake_active_mode,
                                      UINT8 dh_wake_active_mode,
                                      tHAL_NFC_STATUS_CBACK *p_snooze_cback)
{
    UINT8 cmd[NFC_HAL_BT_HCI_CMD_HDR_SIZE + HCI_BRCM_WRITE_SLEEP_MODE_LENGTH];
    UINT8 *p;

    HAL_TRACE_API1 ("HAL_NfcSetSnoozeMode (): snooze_mode = %d", snooze_mode);

    nfc_hal_cb.dev_cb.new_snooze_mode      = snooze_mode;
    nfc_hal_cb.dev_cb.nfc_wake_active_mode = nfc_wake_active_mode;
    nfc_hal_cb.dev_cb.p_prop_cback         = p_snooze_cback;

    p = cmd;

    /* Add the HCI command */
    UINT16_TO_STREAM (p, HCI_BRCM_WRITE_SLEEP_MODE);
    UINT8_TO_STREAM  (p, HCI_BRCM_WRITE_SLEEP_MODE_LENGTH);

    memset (p, 0x00, HCI_BRCM_WRITE_SLEEP_MODE_LENGTH);

    UINT8_TO_STREAM  (p, snooze_mode);          /* Sleep Mode               */

    UINT8_TO_STREAM  (p, idle_threshold_dh);    /* Idle Threshold Host      */
    UINT8_TO_STREAM  (p, idle_threshold_nfcc);  /* Idle Threshold HC        */
    UINT8_TO_STREAM  (p, nfc_wake_active_mode); /* BT Wake Active Mode      */
    UINT8_TO_STREAM  (p, dh_wake_active_mode);  /* Host Wake Active Mode    */

    nfc_hal_dm_send_bt_cmd (cmd,
                            NFC_HAL_BT_HCI_CMD_HDR_SIZE + HCI_BRCM_WRITE_SLEEP_MODE_LENGTH,
                            nfc_hal_dm_set_snooze_mode_cback);
    return NCI_STATUS_OK;
}
