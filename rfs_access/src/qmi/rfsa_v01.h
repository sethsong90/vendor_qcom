#ifndef RFSA_SERVICE_H
#define RFSA_SERVICE_H
/**
  @file rfsa_v01.h
  
  @brief This is the public header file which defines the rfsa service Data structures.

  This header file defines the types and structures that were defined in 
  rfsa. It contains the constant values defined, enums, structures,
  messages, and service message IDs (in that order) Structures that were 
  defined in the IDL as messages contain mandatory elements, optional 
  elements, a combination of mandatory and optional elements (mandatory 
  always come before optionals in the structure), or nothing (null message)
   
  An optional element in a message is preceded by a uint8_t value that must be
  set to true if the element is going to be included. When decoding a received
  message, the uint8_t values will be set to true or false by the decode
  routine, and should be checked before accessing the values that they
  correspond to. 
   
  Variable sized arrays are defined as static sized arrays with an unsigned
  integer (32 bit) preceding it that must be set to the number of elements
  in the array that are valid. For Example:
   
  uint32_t test_opaque_len;
  uint8_t test_opaque[16];
   
  If only 4 elements are added to test_opaque[] then test_opaque_len must be
  set to 4 before sending the message.  When decoding, the _len value is set 
  by the decode routine and should be checked so that the correct number of 
  elements in the array will be accessed. 

*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved. 
 Qualcomm Technologies Proprietary and Confidential.

  $Header$
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====* 
 *THIS IS AN AUTO GENERATED FILE. DO NOT ALTER IN ANY WAY 
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/* This file was generated with Tool version 5.5
   It was generated on: Mon Aug 27 2012
   From IDL File: rfsa_v01.idl */

/** @defgroup rfsa_qmi_consts Constant values defined in the IDL */
/** @defgroup rfsa_qmi_msg_ids Constant values for QMI message IDs */
/** @defgroup rfsa_qmi_enums Enumerated types used in QMI messages */
/** @defgroup rfsa_qmi_messages Structures sent as QMI messages */
/** @defgroup rfsa_qmi_aggregates Aggregate types used in QMI messages */
/** @defgroup rfsa_qmi_accessor Accessor for QMI service object */
/** @defgroup rfsa_qmi_version Constant values for versioning information */

#include <stdint.h>
#include "qmi_idl_lib.h"
#include "common_v01.h"


#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup rfsa_qmi_version 
    @{ 
  */ 
/** Major Version Number of the IDL used to generate this file */
#define RFSA_V01_IDL_MAJOR_VERS 0x01
/** Revision Number of the IDL used to generate this file */
#define RFSA_V01_IDL_MINOR_VERS 0x00
/** Major Version Number of the qmi_idl_compiler used to generate this file */
#define RFSA_V01_IDL_TOOL_VERS 0x05
/** Maximum Defined Message ID */
#define RFSA_V01_MAX_MESSAGE_ID 0x0026;
/** 
    @} 
  */


/** @addtogroup rfsa_qmi_consts 
    @{ 
  */

/**  Max file path size for open request */
#define RFSA_MAX_FILE_PATH_V01 255

/**  Max iovec entries for read or write iovec requests */
#define RFSA_MAX_IOVEC_ENTRIES_V01 50
/**
    @}
  */

/** @addtogroup rfsa_qmi_aggregates
    @{
  */
typedef struct {

  /*  size of actual read */
  uint32_t count;

  /*  the physical address of the shared memory used to store the data */
  uint64_t buffer;
}rfsa_file_content_v01;  /* Type */
/**
    @}
  */

/**  Enumeration for the type of access to check  */
typedef uint64_t rfsa_access_flag_mask_v01;
#define RFSA_ACCESS_FLAG_READ_V01 ((rfsa_access_flag_mask_v01)0x01ull) 
#define RFSA_ACCESS_FLAG_WRITE_V01 ((rfsa_access_flag_mask_v01)0x02ull) 
#define RFSA_ACCESS_FLAG_APPEND_V01 ((rfsa_access_flag_mask_v01)0x04ull) 
#define RFSA_ACCESS_FLAG_CREATE_V01 ((rfsa_access_flag_mask_v01)0x08ull) 
#define RFSA_ACCESS_FLAG_TRUNC_V01 ((rfsa_access_flag_mask_v01)0x10ull) 
/** @addtogroup rfsa_qmi_messages
    @{
  */
/** Request Message; Gets information about a file. Also check to see if the file 
              has the correct permission set for openeing */
typedef struct {

  /* Mandatory */
  /*  path to the file */
  char filename[RFSA_MAX_FILE_PATH_V01 + 1];
}rfsa_file_stat_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup rfsa_qmi_messages
    @{
  */
/** Response Message; Gets information about a file. Also check to see if the file 
              has the correct permission set for openeing */
typedef struct {

  /* Mandatory */
  /*  QMI result code */
  qmi_response_type_v01 resp;

  /* Mandatory */
  /*  file access permission flags */
  rfsa_access_flag_mask_v01 flags;
  /**<   Indicates file access type to be used when opening the file \n
      - 0x01 -- File can be opened for READ operation \n
      - 0x02 -- File can be opened for WRITE operation \n
      - 0x08 -- If a file does not exist, file can be created \n
      - 0x10 -- File does exist and file can be truncated \n */

  /* Optional */
  /*  size of file */
  uint8_t size_valid;  /**< Must be set to true if size is being passed */
  uint32_t size;
}rfsa_file_stat_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup rfsa_qmi_messages
    @{
  */
/** Request Message; Create an empty file container, or to truncate existing file
              before write operation. */
typedef struct {

  /* Mandatory */
  /*  path to the file */
  char filename[RFSA_MAX_FILE_PATH_V01 + 1];

  /* Mandatory */
  /*  file access permission flags */
  rfsa_access_flag_mask_v01 flags;
  /**<   Indicates file access type to be used when opening the file \n
      - 0x08 -- If a file does not exist, create a new file \n 
      - 0x10 -- If the file exists, truncate the file to 0 byte \n */
}rfsa_file_create_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup rfsa_qmi_messages
    @{
  */
/** Response Message; Create an empty file container, or to truncate existing file
              before write operation. */
typedef struct {

  /* Mandatory */
  /*  QMI result code */
  qmi_response_type_v01 resp;
}rfsa_file_create_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup rfsa_qmi_messages
    @{
  */
/** Request Message; Client can request the server to look up the share memory buffer
              allocated to the particular client and return the address.   */
typedef struct {

  /* Mandatory */
  /*  Client ID */
  uint32_t client_id;

  /* Mandatory */
  /*  size of the buffer to request */
  uint32_t size;
}rfsa_get_buff_addr_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup rfsa_qmi_messages
    @{
  */
/** Response Message; Client can request the server to look up the share memory buffer
              allocated to the particular client and return the address.   */
typedef struct {

  /* Mandatory */
  /*  QMI result code */
  qmi_response_type_v01 resp;

  /* Optional */
  /*  Buffer Address */
  uint8_t address_valid;  /**< Must be set to true if address is being passed */
  uint64_t address;
}rfsa_get_buff_addr_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup rfsa_qmi_messages
    @{
  */
/** Request Message; Client can release the buffer server have allocated from the 
              shared memory buffer. */
typedef struct {

  /* Mandatory */
  /*  Client ID */
  uint32_t client_id;

  /* Mandatory */
  /*  Buffer Address */
  uint64_t address;
}rfsa_release_buff_addr_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup rfsa_qmi_messages
    @{
  */
/** Response Message; Client can release the buffer server have allocated from the 
              shared memory buffer. */
typedef struct {

  /* Mandatory */
  /*  QMI result code */
  qmi_response_type_v01 resp;
}rfsa_release_buff_addr_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup rfsa_qmi_messages
    @{
  */
/** Request Message; Reads data from a file.  */
typedef struct {

  /* Mandatory */
  /*  Client ID */
  uint32_t client_id;

  /* Mandatory */
  /*  Path to the file  */
  char filename[RFSA_MAX_FILE_PATH_V01 + 1];

  /* Mandatory */
  /*  file offset to start */
  uint32_t offset;

  /* Mandatory */
  /*  size to read */
  uint32_t size;
}rfsa_file_read_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup rfsa_qmi_messages
    @{
  */
/** Response Message; Reads data from a file.  */
typedef struct {

  /* Mandatory */
  /*  QMI result code */
  qmi_response_type_v01 resp;

  /* Optional */
  /*  info about the actual read */
  uint8_t data_valid;  /**< Must be set to true if data is being passed */
  rfsa_file_content_v01 data;
}rfsa_file_read_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup rfsa_qmi_aggregates
    @{
  */
typedef struct {

  uint32_t file_offset;
  /**<   File offset to access */

  uint32_t buff_addr_offset;
  /**<   Data buffer's offset address */

  uint32_t size;
  /**<   Number of bytes to read */
}rfsa_iovec_desc_type_v01;  /* Type */
/**
    @}
  */

/** @addtogroup rfsa_qmi_messages
    @{
  */
/** Request Message; Reads data from a file in multiple pieces.  */
typedef struct {

  /* Mandatory */
  /*  Client ID */
  uint32_t client_id;

  /* Mandatory */
  /*  Path to the file  */
  char filename[RFSA_MAX_FILE_PATH_V01 + 1];

  /* Mandatory */
  /*  IOVEC structure array */
  uint32_t iovec_struct_len;  /**< Must be set to # of elements in iovec_struct */
  rfsa_iovec_desc_type_v01 iovec_struct[RFSA_MAX_IOVEC_ENTRIES_V01];
}rfsa_iovec_file_read_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup rfsa_qmi_messages
    @{
  */
/** Response Message; Reads data from a file in multiple pieces.  */
typedef struct {

  /* Mandatory */
  /*  QMI result code */
  qmi_response_type_v01 resp;
}rfsa_iovec_file_read_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup rfsa_qmi_messages
    @{
  */
/** Request Message; Write data to a file in multiple pieces.  */
typedef struct {

  /* Mandatory */
  /*  Client ID */
  uint32_t client_id;

  /* Mandatory */
  /*  Path to the file  */
  char filename[RFSA_MAX_FILE_PATH_V01 + 1];

  /* Mandatory */
  /*  file open flags
 IOVEC structure array */
  uint32_t iovec_struct_len;  /**< Must be set to # of elements in iovec_struct */
  rfsa_iovec_desc_type_v01 iovec_struct[RFSA_MAX_IOVEC_ENTRIES_V01];
}rfsa_iovec_file_write_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup rfsa_qmi_messages
    @{
  */
/** Response Message; Write data to a file in multiple pieces.  */
typedef struct {

  /* Mandatory */
  /*  QMI result code */
  qmi_response_type_v01 resp;
}rfsa_iovec_file_write_resp_msg_v01;  /* Message */
/**
    @}
  */

/*Service Message Definition*/
/** @addtogroup rfsa_qmi_msg_ids
    @{
  */
#define QMI_RFSA_FILE_STAT_REQ_MSG_V01 0x0020
#define QMI_RFSA_FILE_STAT_RESP_MSG_V01 0x0020
#define QMI_RFSA_FILE_CREATE_REQ_MSG_V01 0x0021
#define QMI_RFSA_FILE_CREATE_RESP_MSG_V01 0x0021
#define QMI_RFSA_FILE_READ_REQ_MSG_V01 0x0022
#define QMI_RFSA_FILE_READ_RESP_MSG_V01 0x0022
#define QMI_RFSA_GET_BUFF_ADDR_REQ_MSG_V01 0x0023
#define QMI_RFSA_GET_BUFF_ADDR_RESP_MSG_V01 0x0023
#define QMI_RFSA_RELEASE_BUFF_ADDR_REQ_MSG_V01 0x0024
#define QMI_RFSA_RELEASE_BUFF_ADDR_RESP_MSG_V01 0x0024
#define QMI_RFSA_IOVEC_FILE_READ_REQ_MSG_V01 0x0025
#define QMI_RFSA_IOVEC_FILE_READ_RESP_MSG_V01 0x0025
#define QMI_RFSA_IOVEC_FILE_WRITE_REQ_MSG_V01 0x0026
#define QMI_RFSA_IOVEC_FILE_WRITE_RESP_MSG_V01 0x0026
/**
    @}
  */

/* Service Object Accessor */
/** @addtogroup wms_qmi_accessor 
    @{
  */
/** This function is used internally by the autogenerated code.  Clients should use the
   macro rfsa_get_service_object_v01( ) that takes in no arguments. */
qmi_idl_service_object_type rfsa_get_service_object_internal_v01
 ( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version );
 
/** This macro should be used to get the service object */ 
#define rfsa_get_service_object_v01( ) \
          rfsa_get_service_object_internal_v01( \
            RFSA_V01_IDL_MAJOR_VERS, RFSA_V01_IDL_MINOR_VERS, \
            RFSA_V01_IDL_TOOL_VERS )
/** 
    @} 
  */


#ifdef __cplusplus
}
#endif
#endif

