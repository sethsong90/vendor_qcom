/*
 * Copyright (C) 2012 Qualcomm Technologies, Inc. All rights reserved.
 *                 Qualcomm Technologies Proprietary/GTDR
 *
 * All data and information contained in or disclosed by this document is
 * confidential and proprietary information of Qualcomm Technologies, Inc. and all
 * rights therein are expressly reserved.  By accepting this material the
 * recipient agrees that this material and the information contained therein
 * is held in confidence and in trust and will not be used, copied, reproduced
 * in whole or in part, nor its contents revealed in any manner to others
 * without the express written permission of Qualcomm Technologies, Inc.
 *
 *
 *  comm.h : Defines protocol states, and send/receive functions and parameters.
 * ==========================================================================================
 *   $Header: //source/qcom/qct/core/storage/tools/kickstart/comm.h#6 $
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


#ifndef COMM_H
#define COMM_H

#include <termios.h>
#include "sahara_packet.h"
#include "common_protocol_defs.h"

/* Receive buffer size for both DMSS download protocol and the Sahara
 * protocol*/
#define RX_BUFFER_SIZE 512

/* Maximum length of a filename, in bytes */
#define MAX_FILE_NAME_SIZE 128

#define MAX_READ_RETRY 1000
#define MAX_READ 1048576
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

/* Dmss download protocol commands */
enum DLOAD_CMDS {
    DL_PARAM_REQ,
    ACK,
    NAK,
    WRITE,
    GO
};
/*
enum DloadCommands
{
    ILLEGAL,
    WRITE,
    ACK,
    NAK,
    ERASE,
    GO,
    NOOP,
    PARAMREQUEST,
    PARAMRESPONSE,
    MEMDUMP,
    RESET,
    UNLOCK,
    SWVERREQUEST,
    SWVERRESPONSE,
    POWERDOWN,
    WRITEWITH32BITADDR,
    MEMDEBUGQUERY,
    MEMDEBUGINFO,
    MEMREADREQUEST,
    MEMREADRESPONSE,
};
*/

/* Sahara Protocol states */
enum {
    SAHARA_WAIT_HELLO,
    SAHARA_WAIT_COMMAND,
    SAHARA_WAIT_RESET_RESP,
    SAHARA_WAIT_DONE_RESP,
    SAHARA_WAIT_MEMORY_READ,
    SAHARA_WAIT_CMD_EXEC_RESP,
    SAHARA_WAIT_MEMORY_TABLE,
    SAHARA_WAIT_MEMORY_REGION,
};

unsigned int total_bytes_sent;
unsigned int total_bytes_recd;
int sending_started;
int receiving_started;

struct com_state {
    /* name of current port */
    char *port_name;

    /* handle to COM port */
    int port_fd;

    /* handle to input image*/
    int fd;

    /* receive buffer */
    unsigned char *recv_buffer;

    /* receive buffer size */
    unsigned int recv_buffer_size;

    /* contains the file of the image uploaded in Sahara mode */
    char *upload_file_name;

    /* true if raw data transfer has started in Sahara mode */
    int started_raw_image_transfer;

    /* Sahara protocol read data payload */
    struct sahara_packet_read_data sahara_rx_data_payload;

    /* Sahara packet hello */
    struct sahara_packet_hello sahara_hello_packet_rx;

    /* Sahara hello response */
    struct sahara_packet_hello_resp sahara_hello_response_tx;

    /* End image transfer */
    struct sahara_packet_end_image_tx sahara_end_of_image_rx;

    /* Done response type */
    struct sahara_packet_done_resp sahara_done_response_rx;

    /* reset response type */
    struct sahara_packet_reset_resp sahara_reset_response_rx;

    /* command exec response type */
    struct sahara_packet_cmd_exec_resp sahara_cmd_exec_response_rx;

    /* command switch mode type */
    struct sahara_packet_cmd_switch_mode sahara_cmd_switch_mode_tx;

    /* memory debug type */
    struct sahara_packet_memory_debug sahara_memory_debug_rx;

    /* memory debug information array */
    struct dload_debug_type *dload_debug_info;

    /* buffer for sahara raw transfer */
    char* sahara_raw_tx_buffer;

    /* Sahara state */
    int sahara_state;

    /* Previous mode before command mode switch */
    unsigned int prev_mode;
};


/*
 * Initialize the given communication structure
 *
 * Arguments:
 *    com_state*  - Pointer to the com_state structure
 *
 * Returns: SUCCESS/EFAILED
 */
int init_com_port (struct com_state *);

/*
 * uninitialize the given communication structure
 *
 * Arguments:
 *    com_state*  - Pointer to the com_state structure
 *
 * Returns:
 *     NONE
 */
void deinit_com_port (struct com_state *, int noclosedevnode);

/*
 * Establish a connection to the device and gets a handle to it
 *
 * Arguments:
 *    pPort   - name of the port
 *    com_state*  - Pointer to the com_state structure
 *
 * Returns: SUCCESS/EFAILED
 */
int connect (char *pPort, struct com_state *);

 /*
  * close the com port connection
  *
  * Arguments:
  *    com_state*  - Pointer to the com_state structure
  *
  * Returns: SUCCESS/EFAILED
  */
int disconnect (struct com_state *);


/* configure the com port
 *
 * Arguments:
 *    pSettings   - term io settings
 *    com_state*  - Pointer to the com_state structure
 *
 * Returns: SUCCESS/EFAILED
 */
int configure_settings (struct termios *pSettings, struct com_state *, int protocol_type);

/* get the current settings of the com port
 *
 * Arguments:
 *    pSettings  - pointer to the termios
 *    com_state*     - Pointer to the com_state structure
 *
 * Returns: SUCCESS/EFAILED
 */
int get_settings (struct termios *pSettings, struct com_state *);

/* transmit data
 *
 * Arguments:
 *    buffer      - tx payload
 *    buffer_size - size of the payload
 *    com_state*  - Pointer to the com_state structure
 *
 * Returns: SUCCESS/EFAILED
 */
int tx_data (char *buffer, unsigned long buffer_size, struct com_state *);

/* Receive data
 *
 * Arguments:
 *    buffer      - rx payload
 *    buffer_size - size of the payload
 *    com_state*  - Pointer to the com_state structure
 *
 * Returns: SUCCESS/EFAILED
 */
int rx_data(char *buffer, unsigned long buffer_size, struct com_state *pComm, int is_command_packet);

/* connect and configure com port
 *
 * Arguments:
 *    port_number   - device name
 *    protocol_type - dload/sahara protocol
 *
 * Returns: SUCCESS/EFAILED
 */
struct com_state *connect_and_configure_device (char *port_number, int protocol_type );

#endif
