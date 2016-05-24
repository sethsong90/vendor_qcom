/*===========================================================================
 *  FILE:
 *  sahara_packet.h
 *
 *  DESCRIPTION:
 *  Sahara protocol states and structure declaration.
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
 *  sahara_packet.h : Sahara protocol states and structure declaration.
 * ==========================================================================================
 *   $Header: //source/qcom/qct/core/storage/tools/kickstart/sahara_packet.h#8 $
 *   $DateTime: 2010/09/28 12:17:11 $
 *   $Author: niting $
 *
 *  Edit History:
 *  YYYY-MM-DD		who		why
 *  -----------------------------------------------------------------------------
 *  2010-09-28       ng      Added command mode support
 *  2010-10-18       ab      Added memory debug mode support
 *
 *  Copyright 2012 by Qualcomm Technologies, Inc.  All Rights Reserved.
 *
 *==========================================================================================
 */

#ifndef SAHARA_PACKET_H
#define SAHARA_PACKET_H


/*===========================================================================
 *
 *                          INCLUDE FILES
 *
 *  ===========================================================================*/

/*===========================================================================
 *
 *                     PUBLIC DATA DECLARATIONS
 *
 *  ===========================================================================*/
// Sahara Protocol Version
#define SAHARA_VERSION 2
#define SAHARA_VERSION_SUPPORTED 2

/*Maximum 1 megabyte tx buffer size*/
#define MAX_SAHARA_TX_BUFFER_SIZE 0x100000

// Sahara command IDs
enum boot_sahara_cmd_id
{
  SAHARA_NO_CMD_ID          = 0x00,
  SAHARA_HELLO_ID           = 0x01, // sent from target to host
  SAHARA_HELLO_RESP_ID      = 0x02, // sent from host to target
  SAHARA_READ_DATA_ID       = 0x03, // sent from target to host
  SAHARA_END_IMAGE_TX_ID    = 0x04, // sent from target to host
  SAHARA_DONE_ID            = 0x05, // sent from host to target
  SAHARA_DONE_RESP_ID       = 0x06, // sent from target to host
  SAHARA_RESET_ID           = 0x07, // sent from host to target
  SAHARA_RESET_RESP_ID      = 0x08, // sent from target to host
  SAHARA_MEMORY_DEBUG_ID    = 0x09, // sent from target to host
  SAHARA_MEMORY_READ_ID     = 0x0A, // sent from host to target
  SAHARA_CMD_READY_ID       = 0x0B, // sent from target to host
  SAHARA_CMD_SWITCH_MODE_ID = 0x0C, // sent from host to target
  SAHARA_CMD_EXEC_ID        = 0x0D, // sent from host to target
  SAHARA_CMD_EXEC_RESP_ID   = 0x0E, // sent from target to host
  SAHARA_CMD_EXEC_DATA_ID   = 0x0F, // sent from host to target

  // place all new commands above this
  SAHARA_LAST_CMD_ID,
  SAHARA_MAX_CMD_ID             = 0x7FFFFFFF // To ensure 32-bits wide
};

enum boot_sahara_image {
    SAHARA_IMAGE_TYPE_BINARY = 0,               /* Binary format */
    SAHARA_IMAGE_TYPE_ELF,                      /* ELF format */
    SAHARA_IMAGE_UNKNOWN = 0x7FFFFFFF           /* To ensure 32-bits wide */
};

// Status codes for Sahara
enum boot_sahara_status
{
  // Success
  SAHARA_STATUS_SUCCESS =                     0x00,

  // Invalid command received in current state
  SAHARA_NAK_INVALID_CMD =                    0x01,

  // Protocol mismatch between host and target
  SAHARA_NAK_PROTOCOL_MISMATCH =              0x02,

  // Invalid target protocol version
  SAHARA_NAK_INVALID_TARGET_PROTOCOL =        0x03,

  // Invalid host protocol version
  SAHARA_NAK_INVALID_HOST_PROTOCOL =          0x04,

  // Invalid packet size received
  SAHARA_NAK_INVALID_PACKET_SIZE =            0x05,

  // Unexpected image ID received
  SAHARA_NAK_UNEXPECTED_IMAGE_ID =            0x06,

  // Invalid image header size received
  SAHARA_NAK_INVALID_HEADER_SIZE =            0x07,

  // Invalid image data size received
  SAHARA_NAK_INVALID_DATA_SIZE =              0x08,

  // Invalid image type received
  SAHARA_NAK_INVALID_IMAGE_TYPE =             0x09,

  // Invalid tranmission length
  SAHARA_NAK_INVALID_TX_LENGTH =              0x0A,

  // Invalid reception length
  SAHARA_NAK_INVALID_RX_LENGTH =              0x0B,

  // General transmission or reception error
  SAHARA_NAK_GENERAL_TX_RX_ERROR =            0x0C,

  // Error while transmitting READ_DATA packet
  SAHARA_NAK_READ_DATA_ERROR =                0x0D,

  // Cannot receive specified number of program headers
  SAHARA_NAK_UNSUPPORTED_NUM_PHDRS =          0x0E,

  // Invalid data length received for program headers
  SAHARA_NAK_INVALID_PDHR_SIZE =              0x0F,

  // Multiple shared segments found in ELF image
  SAHARA_NAK_MULTIPLE_SHARED_SEG =            0x10,

  // Uninitialized program header location
  SAHARA_NAK_UNINIT_PHDR_LOC =                0x11,

  // Invalid destination address
  SAHARA_NAK_INVALID_DEST_ADDR =              0x12,

  // Invalid data size receieved in image header
  SAHARA_NAK_INVALID_IMG_HDR_DATA_SIZE =      0x13,

  // Invalid ELF header received
  SAHARA_NAK_INVALID_ELF_HDR =                0x14,

  // Unknown host error received in HELLO_RESP
  SAHARA_NAK_UNKNOWN_HOST_ERROR =             0x15,

  // Timeout while receiving data
  SAHARA_NAK_TIMEOUT_RX =                     0x16,

  // Timeout while transmitting data
  SAHARA_NAK_TIMEOUT_TX =                     0x17,

  // Invalid mode received from host
  SAHARA_NAK_INVALID_HOST_MODE =              0x18,

  // Invalid memory read access
  SAHARA_NAK_INVALID_MEMORY_READ =            0x19,

  // Host cannot handle read data size requested
  SAHARA_NAK_INVALID_DATA_SIZE_REQUEST =      0x1A,

  // Memory debug not supported
  SAHARA_NAK_MEMORY_DEBUG_NOT_SUPPORTED =     0x1B,

  // Invalid mode switch
  SAHARA_NAK_INVALID_MODE_SWITCH =            0x1C,

  // Failed to execute command
  SAHARA_NAK_CMD_EXEC_FAILURE =               0x1D,

  // Invalid parameter passed to command execution
  SAHARA_NAK_EXEC_CMD_INVALID_PARAM =         0x1E,

  // Unsupported client command received
  SAHARA_NAK_EXEC_CMD_UNSUPPORTED =           0x1F,

  // Invalid client command received for data response
  SAHARA_NAK_EXEC_DATA_INVALID_CLIENT_CMD =   0x20,

  // Failed to authenticate hash table
  SAHARA_NAK_HASH_TABLE_AUTH_FAILURE =        0x21,

  // Failed to verify hash for a given segment of ELF image
  SAHARA_NAK_HASH_VERIFICATION_FAILURE =      0x22,

  // Failed to find hash table in ELF image
  SAHARA_NAK_HASH_TABLE_NOT_FOUND =           0x23,

  // Place all new error codes above this
  SAHARA_NAK_LAST_CODE,

  SAHARA_NAK_MAX_CODE = 0x7FFFFFFF // To ensure 32-bits wide
};

// Status of all image transfers
enum boot_sahara_mode
{
  SAHARA_MODE_IMAGE_TX_PENDING  = 0x0,
  SAHARA_MODE_IMAGE_TX_COMPLETE = 0x1,
  SAHARA_MODE_MEMORY_DEBUG      = 0x2,
  SAHARA_MODE_COMMAND           = 0x3,

  // place all new commands above this
  SAHARA_MODE_LAST,
  SAHARA_MODE_MAX = 0x7FFFFFFF
};

// Executable commands when target is in command mode
enum boot_sahara_exec_cmd_id
{
  SAHARA_EXEC_CMD_NOP              = 0x00,
  SAHARA_EXEC_CMD_SERIAL_NUM_READ  = 0x01,
  SAHARA_EXEC_CMD_MSM_HW_ID_READ   = 0x02,
  SAHARA_EXEC_CMD_OEM_PK_HASH_READ = 0x03,

  // place all new commands above this
  SAHARA_EXEC_CMD_LAST,
  SAHARA_EXEC_CMD_MAX = 0x7FFFFFFF
};


/* ============================================================================= */
/* Sahara protocol packet defintions */
/* ============================================================================= */

// HELLO command packet type - sent from target to host
//   indicates start of protocol on target side
struct sahara_packet_hello
{
  unsigned int command;                 // command ID
  unsigned int length;                  // packet length incl command and length
  unsigned int version;                 // target protocol version number
  unsigned int version_supported;       // minimum protocol version number supported
                                        // on target
  unsigned int cmd_packet_length;       // maximum packet size supported for command
                                        // packets
  unsigned int mode;                    // expected mode of target operation
  unsigned int reserved0;               // reserved field
  unsigned int reserved1;               // reserved field
  unsigned int reserved2;               // reserved field
  unsigned int reserved3;               // reserved field
  unsigned int reserved4;               // reserved field
  unsigned int reserved5;               // reserved field
};

// HELLO_RESP command packet type - sent from host to target
//   response to hello, protocol version running on host and status sent
struct sahara_packet_hello_resp
{
  unsigned int command;                 // command ID
  unsigned int length;                  // packet length incl command and length
  unsigned int version;                 // host protocol version number
  unsigned int version_supported;       // minimum protocol version number supported
                                        // on host
  unsigned int status;                  // OK or error condition
  unsigned int mode;                    // mode of operation for target to execute
  unsigned int reserved0;               // reserved field
  unsigned int reserved1;               // reserved field
  unsigned int reserved2;               // reserved field
  unsigned int reserved3;               // reserved field
  unsigned int reserved4;               // reserved field
  unsigned int reserved5;               // reserved field
};

// READ_DATA command packet type - sent from target to host
//   sends data segment offset and length to be read from current host
//   image file
struct sahara_packet_read_data
{
  unsigned int command;                 // command ID
  unsigned int length;                  // packet length incl command and length
  unsigned int image_id;                // ID of image to be transferred
  unsigned int data_offset;             // offset into image file to read data from
  unsigned int data_length;             // length of data segment to be retreived
                                        // from image file
};

// END_IMAGE_TX command packet type - sent from target to host
//   indicates end of a single image transfer and status of transfer
struct sahara_packet_end_image_tx
{
  unsigned int command;                 // command ID
  unsigned int length;                  // packet length incl command and length
  unsigned int image_id;                // ID of image to be transferred
  unsigned int status;                  // OK or error condition
};

// DONE packet type - sent from host to target
//   indicates end of single image transfer
struct sahara_packet_done
{
  unsigned int command;                 // command ID
  unsigned int length;                  // packet length incl command and length
};

// DONE_RESP packet type - sent from target to host
//   indicates end of all image transfers
struct sahara_packet_done_resp
{
  unsigned int command;                 // command ID
  unsigned int length;                  // packet length incl command and length
  unsigned int image_tx_status;         // indicates if all images have been
                                        // transferred;
                                        // 0 = IMAGE_TX_PENDING
                                        // 1 = IMAGE_TX_COMPLETE
};

// RESET packet type - sent from host to target
//   indicates to target to reset
struct sahara_packet_reset
{
  unsigned int command;                 // command ID
  unsigned int length;                  // packet length incl command and length
};

// RESET_RESP packet type - sent from target to host
//   indicates to host that target has reset
struct sahara_packet_reset_resp
{
  unsigned int command;                 // command ID
  unsigned int length;                  // packet length incl command and length
};

// MEMORY_DEBUG packet type - sent from target to host
//   sends host the location and length of memory region table
struct sahara_packet_memory_debug
{
  unsigned int command;                 // command ID
  unsigned int length;                  // packet length incl command and length
  unsigned int memory_table_addr;       // location of memory region table
  unsigned int memory_table_length;     // length of memory table
};

// MEMORY_READ packet type - sent from host to target
//   sends memory address and length to read from target memory
struct sahara_packet_memory_read
{
  unsigned int command;                 // command ID
  unsigned int length;                  // packet length incl command and length
  unsigned int memory_addr;             // memory location to read from
  unsigned int memory_length;           // length of data to send
};

// CMD_READY packet type - sent from target to host
//   indicates to host that target is ready to accept commands
struct sahara_packet_cmd_ready
{
  unsigned int command;                 // command ID
  unsigned int length;                  // packet length incl command and length
};

// CMD_SWITCH_MODE packet type - sent from host to target
//   indicates to target to switch modes
struct sahara_packet_cmd_switch_mode
{
  unsigned int command;                 // command ID
  unsigned int length;                  // packet length incl command and length
  unsigned int mode;                    // mode of operation for target to execute
};

// CMD_EXEC packet type - sent from host to target
//   indicates to target to execute given client_command
struct sahara_packet_cmd_exec
{
  unsigned int command;                 // command ID
  unsigned int length;                  // packet length incl command and length
  unsigned int client_command;          // command ID for target Sahara client to
                                        // execute
};

// CMD_EXEC_RESP packet type - sent from host to target
//   indicates to host that target has successfully executed command
//     and length of data response
struct sahara_packet_cmd_exec_resp
{
  unsigned int command;                 // command ID
  unsigned int length;                  // packet length incl command and length
  unsigned int client_command;          // command ID for target Sahara client to
                                        // execute
  unsigned int resp_length;             // length of response returned from command
                                        // execution
};

// CMD_EXEC_DATA packet type - sent from target to host
//   indicates that host is ready to receive data after command execution
struct sahara_packet_cmd_exec_data
{
  unsigned int command;                 // command ID
  unsigned int length;                  // packet length incl command and length
  unsigned int client_command;          // command ID for target Sahara client to
                                        // execute
};

#define DLOAD_DEBUG_STRLEN_BYTES 20
struct dload_debug_type
{
  unsigned char save_pref;
  unsigned int  mem_base;
  unsigned int  length;
  char          desc[DLOAD_DEBUG_STRLEN_BYTES];
  char          filename[DLOAD_DEBUG_STRLEN_BYTES];
};

#endif  /* SAHARA_PACKET_H */
