#ifndef DS_QMH_CONFIG_H
#define DS_QMH_CONFIG_H
/*==========================================================================*/
/*!
  @file 
  ds_qmh_config.h

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

  $Header: //source/qcom/qct/modem/datahlos/interface/qmiifacectls/rel/11.01.01/inc/ds_qmh_config.h#3 $
  $DateTime: 2011/04/17 08:15:09 $$Author: hmurari $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2011-03-11 hm  Created module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Include Files
---------------------------------------------------------------------------*/
#include "comdef.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_FRAMEWORK
#include "dsut.h"
#endif 

#ifndef FEATURE_DSS_LINUX
#include "sio.h"
#endif
/*===========================================================================

                            PUBLIC DATA DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  QMI Proxy iface call types. 
---------------------------------------------------------------------------*/
#define DS_QMH_CALL_TYPE_LOCAL                (0x01UL)
#define DS_QMH_CALL_TYPE_RMNET_TETHERED       (0x02UL)
#define DS_QMH_CALL_TYPE_RMNET_EMBEDDED       (0x04UL)

/*---------------------------------------------------------------------------
  QMI Proxy iface supported target configurations
---------------------------------------------------------------------------*/
typedef enum 
{
  DS_QMH_TARGET_CONFIG_DUAL_PROC_BMP,
  DS_QMH_TARGET_CONFIG_LINUX_SINGLE_MODEM,
  DS_QMH_TARGET_CONFIG_LINUX_MULTI_MODEM,
  DS_QMH_TARGET_CONFIG_SVLTE_TYPE_1_FUSION,
  DS_QMH_TARGET_CONFIG_SVLTE_TYPE_2_FUSION
} dsqmh_target_config_enum_type;


/*===========================================================================

                          PUBLIC FUNCTION DEFINITIONS

===========================================================================*/

/*!
  @brief
  Sets the call types that can be supported using QMH module. 

  @detail
  QMH can support different call types depending on its configuration. 

  DS_QMH_CALL_TYPE_LOCAL           - QMH supports local embedded calls only 
                                     (through DSS APIS)
  DS_QMH_CALL_TYPE_RMNET_TETHERED  - QMH supports calls through RMNET for 
                                     tethered calls only. 
  DS_QMH_CALL_TYPE_RMNET_EMBEDDED  - QMH supports calls through RMNET for 
                                     embedded calls only. 

  More than one call type may be supported (Call types can be OR'ed). 

  @param[in]  call_types - Specifed the call type to be supported.

  @return     None.
*/
void dsqmh_config_set_supported_call_types
(
  uint32 call_types
);


/*!
  @brief
  Gets the supported call types. 

  @detail
  QMH can support different call types depending on its configuration. 

  DS_QMH_CALL_TYPE_LOCAL           - QMH supports local embedded calls only 
                                     (through DSS APIS)
  DS_QMH_CALL_TYPE_RMNET_TETHERED  - QMH supports calls through RMNET for 
                                     tethered calls only. 
  DS_QMH_CALL_TYPE_RMNET_EMBEDDED  - QMH supports calls through RMNET for 
                                     embedded calls only. 

  More than one call type may be supported (Call types can be OR'ed). 

  @param      None.
  @return     Supported call type mask
*/
uint32 dsqmh_config_get_supported_call_types
(
  void 
);

/*!
  @brief
  Get the target configuration. 

  @detail
  QMH can support different target configurations. 

  DS_QMH_TARGET_CONFIG_DUAL_PROC_BMP
  DS_QMH_TARGET_CONFIG_SVLTE_TYPE_1_FUSION
  DS_QMH_TARGET_CONFIG_SVLTE_TYPE_2_FUSION (Only control port support)
  DS_QMH_TARGET_CONFIG_LINUX_SINGLE_MODEM
  DS_QMH_TARGET_CONFIG_LINUX_MULTI_MODEM

  @param      None.
  @return     target configuration.
*/
dsqmh_target_config_enum_type dsqmh_config_get_target_config
(
  void 
);


/*!
  @brief
  Get the SIO port corresponding to the QMH instance.

  @detail
  Each QMH instance is statically mapped to a SIO port. This function
  returns the corresponding SIO port.

  @param      QMH instance 
  @return     SIO port associated with QMH instance.
*/
#ifndef FEATURE_DSS_LINUX
sio_port_id_type dsqmh_config_get_sio_port
(
  uint32 qmh_iface_inst 
);
#endif
/*!
  @brief
  Get the QMIMSGLIB device ID corresponding to QMH instance. 

  @param      QMH instance 
  @return     QMIMSGLIB device ID.
*/
char *dsqmh_config_get_qmi_device_id
(
  uint32 qmh_iface_inst 
);

/*!
  @brief
  Get the processor ID corresponding to the QMH instance.

  @details
  In case of multi-modem (Linux env), QMH can talk to 2 modems. However
  a set of QMH instances are statically mapped to each processor. This
  method returns the corresponding processor ID with the QMH instance.

  @param      QMH instance 
  @return     Processor ID.
*/
uint32 dsqmh_config_get_proc_id
(
  uint32 qmh_iface_inst 
);


/*!
  @brief
  Get the QMH iface instance give proc ID and RM iface handle.

  @details
  RM iface handle returned during routing on a certain modem is
  0 based index identifying the RM iface on that modem. Since QMH 
  instances can map to both processors, this method is used to
  get the QMH instance on a given processor.

  @param      QMH instance 
  @return     Processor ID.
*/
int32 dsqmh_config_get_iface_inst_from_rm_handle
(
  int32     rm_handle,
  uint32    proc_id
);


#ifdef __cplusplus
}
#endif


#endif /* DS_QMH_CONFIG_H */


