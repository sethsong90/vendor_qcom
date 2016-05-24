
/******************************************************************************
  @file    qmi_platform_config.h
  @brief   Platform-specific external QMI definitions.

  DESCRIPTION
  This file contains platform specific configuration definitions
  for QMI interface library.


  INITIALIZATION AND SEQUENCING REQUIREMENTS
  None.

  $Header: //source/qcom/qct/modem/datahlos/interface/qmiifacectls/rel/11.01.01/inc/qmi_platform_config.h#2 $ 
  $DateTime: 2011/04/17 08:15:09 $
  ---------------------------------------------------------------------------
  Copyright (c) 2007 - 2009 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/

#ifndef QMI_PLATFORM_CONFIG_H
#define QMI_PLATFORM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(FEATURE_DSS_LINUX_MULTI_MODEM)
/*---------------------------------------------------------------------------
  Configuration for Linux environment, multi-modem
---------------------------------------------------------------------------*/
/* QMI device IDs to be used for Linux, multi-modem */
#define QMI_PORT_RMNET_1       "rmnet0"
#define QMI_PORT_RMNET_2       "rmnet1"
#define QMI_PORT_RMNET_3       "rmnet2"
#define QMI_PORT_RMNET_4       "rmnet3"
#define QMI_PORT_RMNET_5       "rmnet4"
#define QMI_PORT_RMNET_6       "rmnet5"
#define QMI_PORT_RMNET_7       "rmnet6"
#define QMI_PORT_RMNET_8       "rmnet7"

#define QMI_PORT_RMNET_9       "rmnet_sdio0"
#define QMI_PORT_RMNET_10      "rmnet_sdio1"
#define QMI_PORT_RMNET_11      "rmnet_sdio2"
#define QMI_PORT_RMNET_12      "rmnet_sdio3"
#define QMI_PORT_RMNET_13      "rmnet_sdio4"
#define QMI_PORT_RMNET_14      "rmnet_sdio5"
#define QMI_PORT_RMNET_15      "rmnet_sdio6"
#define QMI_PORT_RMNET_16      "rmnet_sdio7"

/* SIO PORT IDs to be used for these connections */
#define QMI_SIO_PORT_RMNET_1   SIO_PORT_SMD_DATA5
#define QMI_SIO_PORT_RMNET_2   SIO_PORT_SMD_DATA6
#define QMI_SIO_PORT_RMNET_3   SIO_PORT_SMD_DATA7
#define QMI_SIO_PORT_RMNET_4   SIO_PORT_SMD_DATA8
#define QMI_SIO_PORT_RMNET_5   SIO_PORT_SMD_DATA9
#define QMI_SIO_PORT_RMNET_6   SIO_PORT_SMD_DATA10
#define QMI_SIO_PORT_RMNET_7   SIO_PORT_SMD_DATA11
#define QMI_SIO_PORT_RMNET_8   SIO_PORT_SMD_DATA12

#define QMI_SIO_PORT_RMNET_9   SIO_PORT_SDIO_MUX_A2_RMNET_0
#define QMI_SIO_PORT_RMNET_10  SIO_PORT_SDIO_MUX_A2_RMNET_1
#define QMI_SIO_PORT_RMNET_11  SIO_PORT_SDIO_MUX_A2_RMNET_2
#define QMI_SIO_PORT_RMNET_12  SIO_PORT_SDIO_MUX_A2_RMNET_3
#define QMI_SIO_PORT_RMNET_13  SIO_PORT_SDIO_MUX_A2_RMNET_4
#define QMI_SIO_PORT_RMNET_14  SIO_PORT_SDIO_MUX_A2_RMNET_5
#define QMI_SIO_PORT_RMNET_15  SIO_PORT_SDIO_MUX_A2_RMNET_6
#define QMI_SIO_PORT_RMNET_16  SIO_PORT_SDIO_MUX_A2_RMNET_7

/* Number of QMH ifaces supported */
#define DSQMH_MAX_PS_IFACES                    (16)
#define DSQMH_MODEM_CNT                        (2)

/* QMI platform constants definitions */
#define QMI_PLATFORM_MAX_PDP_CONNECTIONS       (16)
#define QMI_PLATFORM_MAX_CONNECTIONS           (16)
#define QMI_PLATFORM_MAX_CONNECTION_NON_BCAST  (16)

/* QMH default supported call_type definition */
#define DS_QMH_DEFAULT_SUPPORTED_CALL_TYPE   (DS_QMH_CALL_TYPE_LOCAL)


#elif defined(FEATURE_DSS_LINUX)
/*---------------------------------------------------------------------------
  Configuration for Linux environment, single-modem
---------------------------------------------------------------------------*/

/* QMI connection ID definitions */
#define QMI_PORT_RMNET_1       "rmnet0"
#define QMI_PORT_RMNET_2       "rmnet1"
#define QMI_PORT_RMNET_3       "rmnet2"
#define QMI_PORT_RMNET_4       "rmnet3"
#define QMI_PORT_RMNET_5       "rmnet4"
#define QMI_PORT_RMNET_6       "rmnet5"
#define QMI_PORT_RMNET_7       "rmnet6"
#define QMI_PORT_RMNET_8       "rmnet7"

#define QMI_PORT_RMNET_9       "unused"
#define QMI_PORT_RMNET_10      "unused"
#define QMI_PORT_RMNET_11      "unused"
#define QMI_PORT_RMNET_12      "unused"
#define QMI_PORT_RMNET_13      "unused"
#define QMI_PORT_RMNET_14      "unused"
#define QMI_PORT_RMNET_15      "unused"
#define QMI_PORT_RMNET_16      "unused"

/* SIO PORT IDs to be used for these connections */
#define QMI_SIO_PORT_RMNET_1   SIO_PORT_SMD_DATA5
#define QMI_SIO_PORT_RMNET_2   SIO_PORT_SMD_DATA6
#define QMI_SIO_PORT_RMNET_3   SIO_PORT_SMD_DATA7
#define QMI_SIO_PORT_RMNET_4   SIO_PORT_SMD_DATA8
#define QMI_SIO_PORT_RMNET_5   SIO_PORT_SMD_DATA9
#define QMI_SIO_PORT_RMNET_6   SIO_PORT_SMD_DATA10
#define QMI_SIO_PORT_RMNET_7   SIO_PORT_SMD_DATA11
#define QMI_SIO_PORT_RMNET_8   SIO_PORT_SMD_DATA12

#define QMI_SIO_PORT_RMNET_9   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_10  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_11  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_12  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_13  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_14  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_15  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_16  SIO_PORT_NULL

/* Number of QMH ifaces supported */
#define DSQMH_MAX_PS_IFACES                    (8)
#define DSQMH_MODEM_CNT                        (1)

/* QMI platform constants definitions */
#define QMI_PLATFORM_MAX_PDP_CONNECTIONS       (8)
#define QMI_PLATFORM_MAX_CONNECTIONS           (8)
#define QMI_PLATFORM_MAX_CONNECTION_NON_BCAST  (8)

/* QMH default supported call_type definition */
#define DS_QMH_DEFAULT_SUPPORTED_CALL_TYPE   (DS_QMH_CALL_TYPE_LOCAL)

#elif defined(FEATURE_DATA_FUSION_MDM_TYPE_2)
/*---------------------------------------------------------------------------
  Configuration for SVLTE type-2 fusion architecture (MDM)
---------------------------------------------------------------------------*/

/* QMI connection ID definitions */
#define QMI_PORT_RMNET_1       "rmnet0"
#define QMI_PORT_RMNET_2       "rmnet0"
#define QMI_PORT_RMNET_3       "unused"
#define QMI_PORT_RMNET_4       "unused"
#define QMI_PORT_RMNET_5       "unused"
#define QMI_PORT_RMNET_6       "unused"
#define QMI_PORT_RMNET_7       "unused"
#define QMI_PORT_RMNET_8       "unused"

#define QMI_PORT_RMNET_9       "unused"
#define QMI_PORT_RMNET_10      "unused"
#define QMI_PORT_RMNET_11      "unused"
#define QMI_PORT_RMNET_12      "unused"
#define QMI_PORT_RMNET_13      "unused"
#define QMI_PORT_RMNET_14      "unused"
#define QMI_PORT_RMNET_15      "unused"
#define QMI_PORT_RMNET_16      "unused"

/* SIO Data ports be used for these connections */
#define QMI_SIO_PORT_RMNET_1   SIO_PORT_DATA_MUX_1
#define QMI_SIO_PORT_RMNET_2   SIO_PORT_DATA_MUX_1
#define QMI_SIO_PORT_RMNET_3   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_4   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_5   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_6   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_7   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_8   SIO_PORT_NULL

#define QMI_SIO_PORT_RMNET_9   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_10  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_11  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_12  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_13  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_14  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_15  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_16  SIO_PORT_NULL

/* Number of QMH ifaces supported */
#define DSQMH_MAX_PS_IFACES                    (2)
#define DSQMH_MODEM_CNT                        (1)

/* QMI platform constants definitions */
#define QMI_PLATFORM_MAX_PDP_CONNECTIONS       (2)
#define QMI_PLATFORM_MAX_CONNECTIONS           (2)
#define QMI_PLATFORM_MAX_CONNECTION_NON_BCAST  (2)

/* QMH default supported call_type definition */
#define DS_QMH_DEFAULT_SUPPORTED_CALL_TYPE  \
  (DS_QMH_CALL_TYPE_LOCAL | DS_QMH_CALL_TYPE_RMNET_TETHERED)

#elif defined(FEATURE_DATA_FUSION_MDM)
/*---------------------------------------------------------------------------
  Configuration for SVLTE type-1 fusion architecture (MDM)
---------------------------------------------------------------------------*/

/* QMI connection ID definitions */
#define QMI_PORT_RMNET_1       "rmnet0"
#define QMI_PORT_RMNET_2       "unused"
#define QMI_PORT_RMNET_3       "unused"
#define QMI_PORT_RMNET_4       "unused"
#define QMI_PORT_RMNET_5       "unused"
#define QMI_PORT_RMNET_6       "unused"
#define QMI_PORT_RMNET_7       "unused"
#define QMI_PORT_RMNET_8       "unused"

#define QMI_PORT_RMNET_9       "unused"
#define QMI_PORT_RMNET_10      "unused"
#define QMI_PORT_RMNET_11      "unused"
#define QMI_PORT_RMNET_12      "unused"
#define QMI_PORT_RMNET_13      "unused"
#define QMI_PORT_RMNET_14      "unused"
#define QMI_PORT_RMNET_15      "unused"
#define QMI_PORT_RMNET_16      "unused"

/* SIO PORT IDs to be used for these connections */
#define QMI_SIO_PORT_RMNET_1   SIO_PORT_DATA_MUX_1
#define QMI_SIO_PORT_RMNET_2   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_3   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_4   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_5   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_6   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_7   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_8   SIO_PORT_NULL

#define QMI_SIO_PORT_RMNET_9   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_10  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_11  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_12  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_13  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_14  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_15  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_16  SIO_PORT_NULL

/* Number of QMH ifaces supported */
#define DSQMH_MAX_PS_IFACES                    (1)
#define DSQMH_MODEM_CNT                        (1)

/* QMI platform constants definitions */
#define QMI_PLATFORM_MAX_PDP_CONNECTIONS       (1)
#define QMI_PLATFORM_MAX_CONNECTIONS           (1)
#define QMI_PLATFORM_MAX_CONNECTION_NON_BCAST  (1)

/* QMH default supported call_type definition */
#define DS_QMH_DEFAULT_SUPPORTED_CALL_TYPE  \
  (DS_QMH_CALL_TYPE_LOCAL | DS_QMH_CALL_TYPE_RMNET_TETHERED)


#else
/*---------------------------------------------------------------------------
  Configuration for dual-proc application processor (BMP/RIM APROC)
---------------------------------------------------------------------------*/

/* QMI connection ID definitions */
#define QMI_PORT_RMNET_1 "rmnet0"
#define QMI_PORT_RMNET_2 "rmnet1"
#define QMI_PORT_RMNET_3 "rmnet2"
#define QMI_PORT_RMNET_4 "rmnet3"
#define QMI_PORT_RMNET_5 "rmnet4"
#define QMI_PORT_RMNET_6       "unused"
#define QMI_PORT_RMNET_7       "unused"
#define QMI_PORT_RMNET_8       "unused"

#define QMI_PORT_RMNET_9       "unused"
#define QMI_PORT_RMNET_10      "unused"
#define QMI_PORT_RMNET_11      "unused"
#define QMI_PORT_RMNET_12      "unused"
#define QMI_PORT_RMNET_13      "unused"
#define QMI_PORT_RMNET_14      "unused"
#define QMI_PORT_RMNET_15      "unused"
#define QMI_PORT_RMNET_16      "unused"

/* SIO PORT IDs to be used for these connections */
#define QMI_SIO_PORT_RMNET_1   SIO_PORT_SMD_DATA5
#define QMI_SIO_PORT_RMNET_2   SIO_PORT_SMD_DATA6
#define QMI_SIO_PORT_RMNET_3   SIO_PORT_SMD_DATA7
#define QMI_SIO_PORT_RMNET_4   SIO_PORT_SMD_DATA8
#define QMI_SIO_PORT_RMNET_5   SIO_PORT_SMD_DATA9
#define QMI_SIO_PORT_RMNET_6   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_7   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_8   SIO_PORT_NULL

#define QMI_SIO_PORT_RMNET_9   SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_10  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_11  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_12  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_13  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_14  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_15  SIO_PORT_NULL
#define QMI_SIO_PORT_RMNET_16  SIO_PORT_NULL

/* Number of QMH ifaces supported */
#define DSQMH_MAX_PS_IFACES                    (5)
#define DSQMH_MODEM_CNT                        (1)

/* QMI platform constants definitions */
#define QMI_PLATFORM_MAX_PDP_CONNECTIONS       (3)
#define QMI_PLATFORM_MAX_CONNECTIONS           (5)
#define QMI_PLATFORM_MAX_CONNECTION_NON_BCAST  (3)

/* QMH default supported call_type definition */
#define DS_QMH_DEFAULT_SUPPORTED_CALL_TYPE   (DS_QMH_CALL_TYPE_LOCAL)

#endif /* end of all configurations */

#ifdef __cplusplus
}
#endif

#endif /* QMI_PLATFORM_CONFIG_H */
