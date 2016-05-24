/*==========================================================================

                     FTM WLAN Commom Header File

Description
  The header file includes dispatch functions that can be called from ftm_main

# Copyright (c) 2010-2011 by Qualcomm Technologies, Inc.  All Rights Reserved.
# Qualcomm Technologies Proprietary and Confidential.

===========================================================================*/

/*===========================================================================

                         Edit History


when       who       what, where, why
--------   ---       ----------------------------------------------------------
07/11/11   karthikm  Includes functions that can be called from FTM main
========================================================================*/
#ifndef  FTM_WLAN_COMMON_H_
#define  FTM_WLAN_COMMON_H_

#include "ftm_wlan.h"

void* ftm_wlan_dispatch(ftm_wlan_pkt_type *wlan_ftm_pkt, int pkt_len);

#endif /* FTM_WLAN_COMMON_H_ */
