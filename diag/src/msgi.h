//# Copyright (c) 2007-2011, 2013 by Qualcomm Technologies, Inc.  All Rights Reserved.
//# Qualcomm Technologies Proprietary and Confidential.

#ifndef MSG_I_H

#include "msg.h"

#define MAX_SSID_PER_RANGE	200
/*
 * Each row contains First (uint32_t), Last (uint32_t), Actual
 * last (uint32_t) values along with the range of SSIDs
 * (MAX_SSID_PER_RANGE*uint32_t).
 * And there are MSG_MASK_TBL_CNT rows.
 */
#define MSG_MASK_SIZE		((MAX_SSID_PER_RANGE+3) * 4 * MSG_MASK_TBL_CNT)

typedef struct
{
    uint16 ssid_first;      /* Start of range of supported SSIDs */
    uint16 ssid_last;       /* Last SSID in range */
    //uint32 rt_length;
    /* Array of (ssid_last - ssid_first + 1) masks */
    uint32* rt_mask_array;
}
msg_mask_read_buffer_type;

unsigned char read_mask[MSG_MASK_SIZE];

//#define DIAG_MSG_MASK_SHAREMAP_NAME _T("Diag_F3Msg_Mask_Shared")
// typedef struct
// {
//     uint16 ssid_first;      /* Start of range of supported SSIDs */
//     uint16 ssid_last;       /* Last SSID in range */
//     //uint32 rt_length;
//     /* Array of (ssid_last - ssid_first + 1) masks */
//     uint32 rt_mask_array[1];
// }
// msg_mask_rt_type;

// typedef struct
// {
//    uint16 ssid_first;
//    uint16 ssid_last;
// } msg_mask_size_type;

/*  WM7: This will be contiguous in memory, so it can be shared between processes. */
//byte* msg_mask;

/* This table will provide an extensible way to compute the size of the
shared msg mask */

// const msg_mask_size_type msg_mask_size_tbl[] = {
//   {
//     MSG_SSID_GEN_FIRST,
//     MSG_SSID_GEN_LAST
//   }
//
//   ,
//   {
//     MSG_SSID_ONCRPC,
//     MSG_SSID_ONCRPC_LAST,
//   }
//
// #ifdef FEATURE_IS2000
//   ,
//   {
//     MSG_SSID_1X,
//     MSG_SSID_1X_LAST
//   }
// #endif /* FEATURE_IS2000 */
//
// #ifdef FEATURE_HDR
//   ,
//   {
//     MSG_SSID_HDR_PROT,
//     MSG_SSID_HDR_LAST
//   }
// #endif /* FEATURE_HDR */
//
// #if defined (FEATURE_WCDMA) || defined (FEATURE_GSM)
//   ,
//   {
//     MSG_SSID_UMTS,
//     MSG_SSID_UMTS_LAST
//   }
// #endif /* defined (FEATURE_WCDMA) || defined (FEATURE_GSM) */
//
// #ifdef FEATURE_GSM
//   ,
//   {
//     MSG_SSID_GSM,
//     MSG_SSID_GSM_LAST
//   }
// #endif /* FEATURE_GSM */
//
// #ifdef FEATURE_WLAN
//   ,
//   {
//     MSG_SSID_WLAN,
//     MSG_SSID_WLAN_LAST
//   }
// #endif /* FEATURE_WLAN */
//
// #if defined (FEATURE_DS) || defined (FEATURE_DATA)
//   ,
//   {
//     MSG_SSID_DS,
//     MSG_SSID_DS_LAST
//   }
// #endif /* defined (FEATURE_DS) || defined (FEATURE_DATA) */
//
//   ,
//   {
//     MSG_SSID_SEC,
//     MSG_SSID_SEC_LAST
//   }
//
//   ,
//   {
//     MSG_SSID_APPS,
//     MSG_SSID_APPS_LAST
//   }
//
//   ,
//   {
//     MSG_SSID_ADSPTASKS,
//     MSG_SSID_ADSPTASKS_LAST
//   }
//
//   ,
//   {
//     MSG_SSID_L4LINUX_KERNEL,
//     MSG_SSID_L4LINUX_LAST
//   }
//
//   ,
//   {
//     MSG_SSID_L4IGUANA_IGUANASERVER,
//     MSG_SSID_L4IGUANA_LAST
//   }
//
//   ,
//   {
//     MSG_SSID_L4AMSS_QDIAG,
//     MSG_SSID_L4AMSS_LAST
//   }
//
// #ifdef FEATURE_HIT
//   ,
//   {
//     MSG_SSID_HIT,
//     MSG_SSID_HIT_LAST
//   }
//
// #endif /* FEATURE_HIT */
//
//   ,
//   {
//     MSG_SSID_UMB,
//     MSG_SSID_UMB_LAST
//   }
//
// };

//#define MSG_MASK_TBL_CNT (sizeof (msg_mask_size_tbl) / sizeof (msg_mask_size_tbl[0]))

#endif /* MSG_I_H */
