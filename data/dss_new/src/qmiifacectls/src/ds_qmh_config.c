/*==========================================================================*/
/*!
  @file 
  ds_qmh_config.c

  @brief
  This file provides configuration definitions for DS_QMH module.

  Copyright (c) 2011 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*==========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datahlos/interface/qmiifacectls/rel/11.01.01/src/ds_qmh_config.c#3 $
  $DateTime: 2011/04/17 08:15:09 $$Author: hmurari $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2011-04-11 hm  Multi-modem support merged from linux QMH
  2011-03-11 hm  Created module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Include Files
---------------------------------------------------------------------------*/
#include "comdef.h"
#include "ds_qmh_config.h"
#include "qmi_platform_config.h"
#include "ds_qmhi.h"
#ifndef FEATURE_DSS_LINUX
#include "sio.h"
#endif

/*---------------------------------------------------------------------------
  Declare SIO port/QMI device/processor ID mapping.
  Each IFACE phys link is statically mapped to one SIO port, the same one
  used by the corresponding RmNET IFACE on the Modem processor.
  Currently upto two modems are supported (MSM and MDM)
---------------------------------------------------------------------------*/
typedef struct {

#ifndef FEATURE_DSS_LINUX
  sio_port_id_type        sio_port_id;
#endif

  char                   *qmi_dev_id;
  uint32                  proc_id;
} dsqmh_config_port_map_type;

#ifndef FEATURE_DSS_LINUX
const static dsqmh_config_port_map_type dsqmh_config_port_map[] =
{
  /* 1st remote processor */
   { QMI_SIO_PORT_RMNET_1,   QMI_PORT_RMNET_1,   ACL_PROC_QCTMSM0 } 
  ,{ QMI_SIO_PORT_RMNET_2,   QMI_PORT_RMNET_2,   ACL_PROC_QCTMSM0 } 
  ,{ QMI_SIO_PORT_RMNET_3,   QMI_PORT_RMNET_3,   ACL_PROC_QCTMSM0 } 
  ,{ QMI_SIO_PORT_RMNET_4,   QMI_PORT_RMNET_4,   ACL_PROC_QCTMSM0 } 
  ,{ QMI_SIO_PORT_RMNET_5,   QMI_PORT_RMNET_5,   ACL_PROC_QCTMSM0 } 
  ,{ QMI_SIO_PORT_RMNET_6,   QMI_PORT_RMNET_6,   ACL_PROC_QCTMSM0 } 
  ,{ QMI_SIO_PORT_RMNET_7,   QMI_PORT_RMNET_7,   ACL_PROC_QCTMSM0 } 
  ,{ QMI_SIO_PORT_RMNET_8,   QMI_PORT_RMNET_8,   ACL_PROC_QCTMSM0 } 

  /* 2nd remote processor */
  ,{ QMI_SIO_PORT_RMNET_9,   QMI_PORT_RMNET_9,   ACL_PROC_QCTMSM1 } 
  ,{ QMI_SIO_PORT_RMNET_10,  QMI_PORT_RMNET_10,  ACL_PROC_QCTMSM1 } 
  ,{ QMI_SIO_PORT_RMNET_11,  QMI_PORT_RMNET_11,  ACL_PROC_QCTMSM1 } 
  ,{ QMI_SIO_PORT_RMNET_12,  QMI_PORT_RMNET_12,  ACL_PROC_QCTMSM1 } 
  ,{ QMI_SIO_PORT_RMNET_13,  QMI_PORT_RMNET_13,  ACL_PROC_QCTMSM1 } 
  ,{ QMI_SIO_PORT_RMNET_14,  QMI_PORT_RMNET_14,  ACL_PROC_QCTMSM1 } 
  ,{ QMI_SIO_PORT_RMNET_15,  QMI_PORT_RMNET_15,  ACL_PROC_QCTMSM1 } 
  ,{ QMI_SIO_PORT_RMNET_16,  QMI_PORT_RMNET_16,  ACL_PROC_QCTMSM1 } 
};
#else
static const dsqmh_config_port_map_type dsqmh_config_port_map[] =
{
  /* 1st remote processor */
   { QMI_PORT_RMNET_0,   ACL_PROC_QCTMSM0 } 
  ,{ QMI_PORT_RMNET_1,   ACL_PROC_QCTMSM0 } 
  ,{ QMI_PORT_RMNET_2,   ACL_PROC_QCTMSM0 } 
  ,{ QMI_PORT_RMNET_3,   ACL_PROC_QCTMSM0 } 
  ,{ QMI_PORT_RMNET_4,   ACL_PROC_QCTMSM0 } 
  ,{ QMI_PORT_RMNET_5,   ACL_PROC_QCTMSM0 } 
  ,{ QMI_PORT_RMNET_6,   ACL_PROC_QCTMSM0 } 
  ,{ QMI_PORT_RMNET_7,   ACL_PROC_QCTMSM0 } 

  /* 2nd remote processor */
  ,{ QMI_PORT_RMNET_SDIO_0,   ACL_PROC_QCTMSM1 } 
  ,{ QMI_PORT_RMNET_SDIO_1,  ACL_PROC_QCTMSM1 } 
  ,{ QMI_PORT_RMNET_SDIO_2,  ACL_PROC_QCTMSM1 } 
  ,{ QMI_PORT_RMNET_SDIO_3,  ACL_PROC_QCTMSM1 } 
  ,{ QMI_PORT_RMNET_SDIO_4,  ACL_PROC_QCTMSM1 } 
  ,{ QMI_PORT_RMNET_SDIO_5,  ACL_PROC_QCTMSM1 } 
  ,{ QMI_PORT_RMNET_SDIO_6,  ACL_PROC_QCTMSM1 } 
  ,{ QMI_PORT_RMNET_SDIO_7,  ACL_PROC_QCTMSM1 } 
};
#endif


#define DSQMH_SMD_PORT_MAP_SIZE \
  (sizeof(dsqmh_config_port_map)/sizeof(dsqmh_config_port_map_type))

#define DSQMH_CONFIG_NUM_CONNS_QCTMSM0 (8)
#define DSQMH_CONFIG_NUM_CONNS_QCTMSM1 (8)


/*---------------------------------------------------------------------------
  Different QMH configurations supported.
---------------------------------------------------------------------------*/
#if defined(FEATURE_DSS_LINUX_MULTI_MODEM)
  
  static dsqmh_target_config_enum_type dsqmh_target_config = 
    DS_QMH_TARGET_CONFIG_LINUX_MULTI_MODEM;

#elif defined(FEATURE_DSS_LINUX)

  static dsqmh_target_config_enum_type dsqmh_target_config = 
    DS_QMH_TARGET_CONFIG_LINUX_SINGLE_MODEM;

#elif defined(FEATURE_DATA_FUSION_MDM_TYPE_2)

  static dsqmh_target_config_enum_type dsqmh_target_config = 
    DS_QMH_TARGET_CONFIG_SVLTE_TYPE_2_FUSION;

#elif defined(FEATURE_DATA_FUSION_MDM)

  static dsqmh_target_config_enum_type dsqmh_target_config = 
    DS_QMH_TARGET_CONFIG_SVLTE_TYPE_1_FUSION;

#else

  static dsqmh_target_config_enum_type dsqmh_target_config = 
    DS_QMH_TARGET_CONFIG_DUAL_PROC_BMP;

#endif 

void dsqmh_config_set_supported_call_types
(
  uint32 call_types
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  dsqmh_state_info.supported_call_types = call_types;

}

uint32 dsqmh_config_get_supported_call_types
(
  void 
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return dsqmh_state_info.supported_call_types;

}

dsqmh_target_config_enum_type dsqmh_config_get_target_config
(
  void 
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return dsqmh_target_config;

}

#ifndef FEATURE_DSS_LINUX
sio_port_id_type dsqmh_config_get_sio_port
(
  uint32 qmh_iface_inst 
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (qmh_iface_inst < DSQMH_SMD_PORT_MAP_SIZE)
  {
    return dsqmh_config_port_map[qmh_iface_inst].sio_port_id;
  }
  else
  {
    return SIO_PORT_NULL;  
  }
}
#endif

char *dsqmh_config_get_qmi_device_id
(
  uint32 qmh_iface_inst 
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (qmh_iface_inst < DSQMH_SMD_PORT_MAP_SIZE)
  {
    return dsqmh_config_port_map[qmh_iface_inst].qmi_dev_id;
  }
  else
  {
    return NULL;  
  }

}


uint32 dsqmh_config_get_proc_id
(
  uint32 qmh_iface_inst 
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (qmh_iface_inst < DSQMH_SMD_PORT_MAP_SIZE)
  {
    return dsqmh_config_port_map[qmh_iface_inst].proc_id;
  }
  else
  {
    return 0;  
  }

}

int32 dsqmh_config_get_iface_inst_from_rm_handle
(
  int32     rm_handle,
  uint32    proc_id
)
{
  int32     qmh_iface_inst = -1;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 > rm_handle)
  {
    LOG_MSG_INFO1 ("RM handle no match", 0, 0, 0);
    return -1;    
  }

  if (ACL_PROC_QCTMSM0 == proc_id)
  {
    if (rm_handle < DSQMH_CONFIG_NUM_CONNS_QCTMSM0)
    {
      qmh_iface_inst = rm_handle;
    }
  }
  else if (ACL_PROC_QCTMSM1 == proc_id)
  {
    if (rm_handle < DSQMH_CONFIG_NUM_CONNS_QCTMSM1)
    {
      qmh_iface_inst = rm_handle + DSQMH_CONFIG_NUM_CONNS_QCTMSM0;
    }
  }

  return qmh_iface_inst;

}
