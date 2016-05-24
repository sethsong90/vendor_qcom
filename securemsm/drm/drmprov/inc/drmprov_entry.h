#ifndef __DRMPROV_ENTRY_H_
#define __DRMPROV_ENTRY_H_
/*===========================================================================
Copyright (c) 2011-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  $Header:

when        who     what, where, why
--------   ---     ----------------------------------------------------------
01/17/13   rk      Modified generic prov APIs to handle data larger than TZ_PR_PROV_PKG_SIZE
12/03/12   cz      Move drmprov apis to the drmprov_clnt.h file
08/08/12   rk      Initial Version.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
/** @addtogroup tz_playready
  @} */
#include "comdef.h"
#include "stdlib.h"
#include "app_main.h"

#define DRMPROV_CREATE_CMD(x)     (SVC_DRMPROV_ID | x)

//#define SINGLE_TIME_PROV

/**
  Commands for :
  1) TZ Services requested by HLOS
  2) HLOS services requested by TZ
 */
typedef enum
{
   /* HLOS to DRMPROV commands - 
  ** Following commands represent services that HLOS could request from TZ.
  ** This is the traditional use case where HLOS will be the client and TZ will service the following requests.
  */
  DRMPROV_CMD_INVALID           = DRMPROV_CREATE_CMD(0x00000000),
  DRMPROV_CMD_SAVE_KEY,               /**< generic cmd of prvisioning keys using sfs */
  DRMPROV_CMD_VERIFY_KEY,             /**< generic cmd of verification of provisioned keys using sfs */
  DRMPROV_CMD_PROV_MANT,
  DRMPROV_CMD_PROV_DATA,
  DRMPROV_CMD_FINALIZE_KEY_PROV, 
  DRMPROV_CMD_UNKNOWN           = DRMPROV_CREATE_CMD(0x7FFFFFFF)
} drmprov_cmd_type;

/** Command structure for key provisioning maintenance
*/
typedef struct tz_prov_maintenance_req_s
{
  /** First 4 bytes are always command id */
  drmprov_cmd_type        prov_cmd;
  /** Number of bytes of the message */
  uint32                  msg_len;
  /** The address of maintenace message */
  uint8                   msg_data[TZ_CM_PROV_PKG_SIZE];
} __attribute__ ((packed)) tz_prov_maintenance_req_t;

typedef struct tz_prov_maintenance_rsp_s
{
  /** First 4 bytes are always command id */
  drmprov_cmd_type        prov_cmd;
  /**<-- Return value for maintenance */
  int32                   ret;
} __attribute__ ((packed)) tz_prov_maintenance_rsp_t;


/** Command structure for key provisioning
*/
typedef struct tz_prov_provision_req_s
{
  /** First 4 bytes are always command id */
  drmprov_cmd_type        prov_cmd;
  /** Number of bytes of the message */
  uint32                  msg_len;
  /** The address of provisioning message */
  uint8                   msg_data[TZ_CM_PROV_PKG_SIZE];
} __attribute__ ((packed)) tz_prov_provision_req_t;

typedef struct tz_prov_provision_rsp_s
{
  /** First 4 bytes are always command id */
  drmprov_cmd_type        prov_cmd;
  /** Return vaule of provisioning */
  char                    prt_path[TZ_CM_MAX_NAME_LEN];
  /** Return vaule of provisioning */
  int32                   ret;
} __attribute__ ((packed)) tz_prov_provision_rsp_t;


/** Command structure for saving generic drm prov keys
*/
typedef struct tz_drm_save_key_req_s
{
  /** First 4 bytes should always be command id */
  drmprov_cmd_type            pr_cmd;
  /** Length of the feature name */
  uint32                    feature_name_len;
  /** Length of the file name */
  uint32                    file_name_len;
  /** Length of the data */
  uint32                    msg_len;
  /** feature name */
  uint8                     feature_name[TZ_CM_MAX_NAME_LEN];
  /** file name */
  uint8                     file_name[TZ_CM_MAX_NAME_LEN];
  /** data for the keys */
  uint8                     msg_data[TZ_CM_PROV_PKG_SIZE];
  /** Indicates whether file to be created with O_TRUNC */
  uint8                     file_truncation_flag;
} __attribute__ ((packed)) tz_drm_save_key_req_t;

typedef struct tz_drm_save_key_rsp_s
{
  /** First 4 bytes should always be command id */
  drmprov_cmd_type            pr_cmd;
  /** Path to the created file */
  uint8                     prt_path[TZ_CM_MAX_NAME_LEN];
  /**<-- E_SUCCESS for success and E_FAILURE for failure */
  long                      ret;
} __attribute__ ((packed)) tz_drm_save_key_rsp_t;

/** Command structure for verifying generic drm prov keys
*/
typedef struct tz_drm_verify_key_req_s
{
  /** First 4 bytes should always be command id */
  drmprov_cmd_type          pr_cmd;
  /** Length of the feature name */
  uint32                    feature_name_len;
  /** Length of the file name */
  uint32                    file_name_len;
  /** Length of the data */
  uint32                    msg_len;
  /** feature name */
  uint8                     feature_name[TZ_CM_MAX_NAME_LEN];
  /** file name */
  uint8                     file_name[TZ_CM_MAX_NAME_LEN];
  /** data for the keys */
  uint8                     msg_data[TZ_CM_PROV_PKG_SIZE];
} __attribute__ ((packed)) tz_drm_verify_key_req_t;

typedef struct tz_drm_verify_key_rsp_s
{
  /** First 4 bytes should always be command id */
  drmprov_cmd_type          pr_cmd;
  /** Path to the created file */
  uint8                     prt_path[TZ_CM_MAX_NAME_LEN];
  /**<-- E_SUCCESS for success and E_FAILURE for failure */
  long                      ret;
} __attribute__ ((packed)) tz_drm_verify_key_rsp_t;

/** Command structure for finalizing key provisioning
*/
typedef struct tz_prov_finalize_provision_req_s
{
  /** First 4 bytes are always command id */
  drmprov_cmd_type        cmd_id;

} __attribute__ ((packed)) tz_prov_finalize_provision_req_t;

typedef struct tz_prov_finalize_provision_rsp_s
{
  /** First 4 bytes are always command id */
  drmprov_cmd_type        cmd_id;

  /** Return vaule of provisioning */
  int32                   ret;
} __attribute__ ((packed)) tz_prov_finalize_provision_rsp_t;

#endif /* __TZ_SERVICE_H_ */
