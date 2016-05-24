/*===========================================================================
 *  FILE:
 *  sahara_protocol.h
 *
 *  DESCRIPTION:
 *  Sahara protocol interface.
 *
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
 *  sahara_protocol.h : Sahara protocol interface.
 * ==========================================================================================
 *   $Header: //source/qcom/qct/core/storage/tools/kickstart/sahara_protocol.h#10 $
 *   $DateTime: 2010/11/02 20:10:26 $
 *   $Author: abrahma $
 *
 *  Edit History:
 *  YYYY-MM-DD		who		why
 *  -----------------------------------------------------------------------------
 *  2010-09-28       ng      Added command mode support
 *
 *  Copyright 2012 by Qualcomm Technologies, Inc.  All Rights Reserved.
 *
 *==========================================================================================
 */
#ifndef SAHARA_PROTOCOL_H
#define SAHARA_PROTOCOL_H

#include "common_protocol_defs.h"
#include "sahara_packet.h"
#include "comm.h"
#include "kickstart_utils.h"

/******************************************************************************
* Name: start_sahara_based_transfer
*
* Description:
*    This function transfers the input image to the target using Sahara protocol
*
* Arguments:
*    m_comm     -   Pointer to the com structure
*    file_name  -   Input file name or directory name
* Returns:
*    int        -   SUCCESS/EFAILED
*
* Note:
*
******************************************************************************/
int start_sahara_based_transfer
(
  struct com_state *m_comm,
  enum boot_sahara_mode sahara_mode,
  char *PathToSaveFiles,
  int memdebugImage
);

#endif
