/*===========================================================================
 *  FILE:
 *  common_protocol_defs.h
 *
 *  DESCRIPTION:
 *  Declaration of common protocol declarions and system header includes
 *
 *  Copyright (C) 2012 Qualcomm Technologies, Inc. All rights reserved.
 *                  Qualcomm Technologies Proprietary/GTDR
 *
 *  All data and information contained in or disclosed by this document is
 *  confidential and proprietary information of Qualcomm Technologies, Inc. and all
 *  rights therein are expressly reserved.  By accepting this material the
 *  recipient agrees that this material and the information contained therein
 *  is held in confidence and in trust and will not be used, copied, reproduced
 *  in whole or in part, nor its contents revealed in any manner to others
 *  without the express written permission of Qualcomm Technologies, Inc.
 *  ===========================================================================
 *
 *
 *  common_protocol_defs.h : Declaration of common protocol declarions and system header includes.
 * ==========================================================================================
 *   $Header: //source/qcom/qct/core/storage/tools/kickstart/common_protocol_defs.h#6 $
 *   $DateTime: 2010/05/03 22:12:02 $
 *   $Author: niting $
 *
 *  Edit History:
 *  YYYY-MM-DD		who		why
 *  -----------------------------------------------------------------------------
 *
 *  Copyright 2012 by Qualcomm Technologies, Inc.  All Rights Reserved.
 *
 *==========================================================================================
 */

#ifndef COMMON_PROTOCOL_DEFS
#define COMMON_PROTOCOL_DEFS

/*==========================================================================
 * Includes that are used in the protocol
 * =========================================================================*/
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "kickstart_log.h"
#include "kickstart_utils.h"


/*Possible images types that a target can request the host to send while speaking Sahara */
enum image_type {
  NONE_IMG        = 0x00,
  OEM_SBL_IMG     = 0x01,
  AMSS_IMG        = 0x02,
  QCSBL_IMG       = 0x03,
  HASH_IMG        = 0x04,
  NANDPRG_IMG     = 0x05,
  CFG_DATA        = 0x06,
  NORPRG_IMG      = 0x07,
  HOSTDL_IMG      = 0x08,
  FSBL_IMG        = 0x09,
  DBL_IMG         = 0x15,
  DBL_IMG_OLD     = 0x0A,
  OSBL_IMG        = 0x0B,
  APPS_IMG        = 0x0C,
  APPSBL_IMG      = 0x0D,
  DSP1_IMG        = 0x0E,
  DSP2_IMG        = 0x0F,
  EHOSTDL_IMG     = 0x10,
  RAMFS1_IMG      = 0x11,
  RAMFS2_IMG      = 0x12,
  ADSP_Q5_IMG     = 0x13,
  APPS_KERNEL_IMG = 0x14,
  MAX_IMG         = 0x7FFFFFFF
};


/*Enum containing the two supported protocol types */
enum ProtocolType {
    DLOAD_PROTOCOL,
    SAHARA_PROTOCOL
};

/*custom min function*/
#define __min(a, b)  ((a) < (b) ? (a) : (b))

/*Maximum size of a dload packet, it can't be more that 4kilobytes, since the worst case hdlce
 *coded buffer would be just 3kilobytes*/
#define  MAX_DMPACKET 4096

/*endian conversion*/
#define  FLOPW(ray, val) \
    (ray)[0] = ((val) / 256); \
    (ray)[1] = ((val) & 0xFF)

/*Macros for function return values */
#define EFAILED               1
#define SUCCESS               0

/*Invalid handle for com port*/
#define INVALID_HANDLE_VALUE  -1

#endif
