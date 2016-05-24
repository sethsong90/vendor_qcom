#ifndef RMTFS_SERVICE_H
#define RMTFS_SERVICE_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        R E M O T E _ S T O R A G E _ V 0 1  . H

GENERAL DESCRIPTION
  This is the public header file which defines the rmtfs service Data structures.

  Copyright (c) 2011 Qualcomm Technologies, Inc.  All Rights Reserved. 
 Qualcomm Technologies Proprietary and Confidential.

  $Header$
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====* 
 *THIS IS AN AUTO GENERATED FILE. DO NOT ALTER IN ANY WAY 
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*
 * This header file defines the types and structures that were defined in 
 * rmtfs. It contains the constant values defined, enums, structures,
 * messages, and service message IDs (in that order) Structures that were 
 * defined in the IDL as messages contain mandatory elements, optional 
 * elements, a combination of mandatory and optional elements (mandatory 
 * always come before optionals in the structure), or nothing (null message)
 *  
 * An optional element in a message is preceded by a uint8_t value that must be
 * set to true if the element is going to be included. When decoding a received
 * message, the uint8_t values will be set to true or false by the decode
 * routine, and should be checked before accessing the values that they
 * correspond to. 
 *  
 * Variable sized arrays are defined as static sized arrays with an unsigned
 * integer (32 bit) preceding it that must be set to the number of elements
 * in the array that are valid. For Example:
 *  
 * uint32_t test_opaque_len;
 * uint8_t test_opaque[16];
 *  
 * If only 4 elements are added to test_opaque[] then test_opaque_len must be
 * set to 4 before sending the message.  When decoding, the _len value is set 
 * by the decode routine and should be checked so that the correct number of 
 * elements in the array will be accessed. 
 */

/* This file was generated with Tool version 02.04 
   It was generated on: Fri Mar 25 2011
   From IDL File: remote_storage_v01.idl */

#include <stdint.h>
#include "qmi_idl_lib.h"
#include "common_v01.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Version Number of the IDL used to generate this file */
#define RMTFS_V01_IDL_MAJOR_VERS 01
#define RMTFS_V01_IDL_MINOR_VERS 01
#define RMTFS_V01_IDL_TOOL_VERS 02

/* Const Definitions */


/*  Max file path size for open request */
#define RMTFS_MAX_FILE_PATH_V01 255

/*  Max iovec entries for read or write iovec requests */
#define RMTFS_MAX_IOVEC_ENTRIES_V01 255

/*  Max caller ID's that can be specified in force sync indication */
#define RMTFS_MAX_CALLER_ID_V01 10

typedef struct {

  /* Mandatory */
  /*  Path to the partition on eMMC device on APPS which modem wants to open */
  char path[RMTFS_MAX_FILE_PATH_V01 + 1];
}rmtfs_open_req_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;
  /*   Standard response type. */

  /* Optional */
  /*  Client ID */
  uint8_t caller_id_valid;	/* Must be set to true if caller_id is being passed */
  uint32_t caller_id;
}rmtfs_open_resp_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  /*  Client ID */
  uint32_t caller_id;
}rmtfs_close_req_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;
  /*   Standard response type. */
}rmtfs_close_resp_msg_v01;  /* Message */

typedef struct {

  uint32_t sector_addr;
  /*   Sector address offset to access */

  uint32_t data_phy_addr_offset;
  /*   Physical address offset of the data */

  uint32_t num_sector;
  /*   Number of sectors to read or write */
}rmtfs_iovec_desc_type_v01;  /* Type */

typedef enum {
  RMTFS_DIRECTION_ENUM_MIN_ENUM_VAL_V01 = -2147483647, /*To force a 32 bit signed enum.  Do not change or use*/
  RMTFS_DIRECTION_READ_V01 = 0x00,
  RMTFS_DIRECTION_WRITE_V01 = 0x01,
  RMTFS_DIRECTION_ENUM_MAX_ENUM_VAL_V01 = 2147483647 /*To force a 32 bit signed enum.  Do not change or use*/
}rmtfs_direction_enum_v01;

typedef struct {

  /* Mandatory */
  /*  Client ID */
  uint32_t caller_id;

  /* Mandatory */
  /*  Direction of operation */
  rmtfs_direction_enum_v01 direction;

  /* Mandatory */
  /*  Iovec Structure array */
  uint32_t iovec_struct_len;	/* Must be set to # of elements in iovec_struct */
  rmtfs_iovec_desc_type_v01 iovec_struct[RMTFS_MAX_IOVEC_ENTRIES_V01];

  /* Mandatory */
  /*  Indicate force sync */
  uint8_t is_force_sync;
}rmtfs_rw_iovec_req_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;
  /*   Standard response type. */
}rmtfs_rw_iovec_resp_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  /*  Client ID */
  uint32_t caller_id;

  /* Mandatory */
  /*  Size of buffer to be allocated */
  uint32_t buff_size;
}rmtfs_alloc_buff_req_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;
  /*   Standard response type. */

  /* Optional */
  /*  Address of the buffer allocated */
  uint8_t buff_address_valid;	/* Must be set to true if buff_address is being passed */
  uint64_t buff_address;
}rmtfs_alloc_buff_resp_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  /*  Client ID */
  uint32_t caller_id;
}rmtfs_get_dev_error_req_msg_v01;  /* Message */

typedef enum {
  RMTFS_DEV_ERROR_ENUM_MIN_ENUM_VAL_V01 = -2147483647, /*To force a 32 bit signed enum.  Do not change or use*/
  RMTFS_DEV_ERROR_NONE_V01 = 0,
  RMTFS_DEV_ERROR_DEVICE_NOT_FOUND_V01 = 1,
  RMTFS_DEV_ERROR_PARTITION_NOT_FOUND_V01 = 2,
  RMTS_DEV_ERROR_RW_FAILURE_V01 = 3,
  RMTS_DEV_ERROR_PARAM_ERROR_V01 = 4,
  RMTS_DEV_ERROR_OTHER_V01 = 5,
  RMTFS_DEV_ERROR_ENUM_MAX_ENUM_VAL_V01 = 2147483647 /*To force a 32 bit signed enum.  Do not change or use*/
}rmtfs_dev_error_enum_v01;

typedef struct {

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;
  /*   Standard response type. */

  /* Optional */
  /*  Device Error  */
  uint8_t status_valid;	/* Must be set to true if status is being passed */
  rmtfs_dev_error_enum_v01 status;
}rmtfs_get_dev_error_resp_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  /*  List of caller ids for which force sync to be issued */
  uint32_t caller_id_len;	/* Must be set to # of elements in caller_id */
  uint32_t caller_id[RMTFS_MAX_CALLER_ID_V01];
}rmtfs_force_sync_ind_msg_v01;  /* Message */

/*Service Message Definition*/
#define QMI_RMTFS_OPEN_REQ_V01 0x0001
#define QMI_RMTFS_OPEN_RESP_V01 0x0001
#define QMI_RMTFS_CLOSE_REQ_V01 0x0002
#define QMI_RMTFS_CLOSE_RESP_V01 0x0002
#define QMI_RMTFS_RW_IOVEC_REQ_V01 0x0003
#define QMI_RMTFS_RW_IOVEC_RESP_V01 0x0003
#define QMI_RMTFS_ALLOC_BUFF_REQ_V01 0x0004
#define QMI_RMTFS_ALLOC_BUFF_RESP_V01 0x0004
#define QMI_RMTFS_GET_DEV_ERROR_REQ_V01 0x0005
#define QMI_RMTFS_GET_DEV_ERROR_RESP_V01 0x0005
#define QMI_RMTFS_FORCE_SYNC_IND_V01 0x0006

/* Service Object Accessor */
/* This function is used internally by the autogenerated code.  Clients should use the
   macro defined below that takes in no arguments. */
qmi_idl_service_object_type rmtfs_get_service_object_internal_v01
 ( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version );
 
/* This macro should be used to get the service object */
#define rmtfs_get_service_object_v01( ) \
          rmtfs_get_service_object_internal_v01( \
            RMTFS_V01_IDL_MAJOR_VERS, RMTFS_V01_IDL_MINOR_VERS, \
            RMTFS_V01_IDL_TOOL_VERS )


#ifdef __cplusplus
}
#endif
#endif

