#ifndef DS_UTILS_MEM_CONFIG_H
#define DS_UTILS_MEM_CONFIG_H

/*==========================================================================*/ 
/*! 
  @file 
  ds_Utils_MemConfig.h

  @brief
  This file is used to configure the number of buffers used for ds Utils
  items. 

  @details
  The buffer numbers are currently configured to suit the AMSS environment.
  Here, there is only one instance of the DSNET/DSSOCK library that is
  statically linked with all applications. AMSS supports around 20 
  applications and around 50 sockets. 

  For Linux/WinMobile environment, DSSOCK/DSNET are linked with each 
  application as a library. Hence all the buffers would need correct
  configuring for 3rd party OS platforms.
  
  @see ps_mem_get_buf()

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/inc/ds_Utils_MemConfig.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-06-27 hm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "customer.h"
#include "comdef.h"

/*===========================================================================

                     PUBLIC DATA DEFINITIONS

===========================================================================*/

/*! 
  This constant defines the number of signal objects. The same number is
  used for signalctl objects as well 

  @details
  Signals are used by the following objects:
    Object          Num events    Num Objects (approx)
    Network         6             20
    Network1x       3             4
    MTPD            1             2
    NetworkMBMS     1             2
    MCastSession    2             32
    IPFilter        1             20
    QoSDefault      3             20
    QoSSecondary    3             16
    PhysLink        1             16
    Socket          5             50

    Since all signals for all objects would not be used at the same time,
    we are currently setting the number of signals to 500 as the safe bet.
*/

#ifndef FEATURE_DATA_PS_LOW_MEM_CHIPSET
  #define MAX_SIGNAL_OBJS           (600)
#else  /* FEATURE_DATA_PS_LOW_MEM_CHIPSET */
  #define MAX_SIGNAL_OBJS           (300)
#endif /* FEATURE_DATA_PS_LOW_MEM_CHIPSET */


#endif /* DS_UTILS_MEM_CONFIG_H */


