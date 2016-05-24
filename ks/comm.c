/*
 * Copyright (C) 2012 Qualcomm Technologies, Inc. All rights reserved.
 *                    Qualcomm Technologies Proprietary/GTDR
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
 *  comm.c : Handles com port connection and data transmission
 * ==========================================================================================
 *   $Header: //source/qcom/qct/core/storage/tools/kickstart/comm.c#7 $
 *   $DateTime: 2010/09/28 12:17:11 $
 *   $Author: niting $
 *
 *  Edit History:
 *  YYYY-MM-DD		who		why
 *  -----------------------------------------------------------------------------
 *  2010-09-28       ng      Added command mode support
 *
 *  Copyright 2012 by Qualcomm Technologies, Inc.  All Rights Reserved.
 *
 *==========================================================================================
 */

#include "comm.h"
#include <termios.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <stdio.h>

/* Main sets this TRUE for verbose logging */
extern int verbose;

extern int using_uart;
extern int using_sdio;
extern int rx_timeout;
extern int using_tty;

extern unsigned int malloc_count;
extern unsigned int free_count;

unsigned int sleep_usec = 5000;
/*  initializes the com port
 *
 *  PARAMETERS:
 *  pComm        - Pointer to the comm structure
 *
 *  RETURN VALUE:
 *  int          - SUCCESS/EFAILED
 */
int init_com_port (struct com_state *pComm)
{
    pComm->port_name                  = "";
    pComm->port_fd                    = INVALID_HANDLE_VALUE;
    pComm->started_raw_image_transfer = EFAILED;
    pComm->recv_buffer = NULL;
    pComm->sahara_raw_tx_buffer = NULL;
    pComm->upload_file_name = NULL;
    pComm->dload_debug_info = NULL;
    pComm->recv_buffer                = malloc (RX_BUFFER_SIZE);
    if (NULL==pComm->recv_buffer) {
        dbg (ERROR, "Memory allocation failure: %s", strerror(errno));
        return EFAILED;
    }
    malloc_count++;
    memset (pComm->recv_buffer, 0, RX_BUFFER_SIZE);
    pComm->sahara_raw_tx_buffer = malloc(MAX_SAHARA_TX_BUFFER_SIZE);
    if (NULL==pComm->sahara_raw_tx_buffer) {
        dbg (ERROR, "Memory allocation failure: %s", strerror(errno));
        return EFAILED;
    }
    malloc_count++;
    memset (pComm->sahara_raw_tx_buffer, 0, MAX_SAHARA_TX_BUFFER_SIZE);
    /*memset the sahara packet buffers*/
    memset(&pComm->sahara_hello_packet_rx,   0, sizeof(struct sahara_packet_hello));
    memset(&pComm->sahara_hello_response_tx, 0, sizeof(struct sahara_packet_hello_resp));
    memset(&pComm->sahara_end_of_image_rx,   0, sizeof(struct sahara_packet_end_image_tx));
    memset(&pComm->sahara_rx_data_payload,   0, sizeof(struct sahara_packet_read_data));
    memset(&pComm->sahara_done_response_rx,  0, sizeof(struct sahara_packet_done_resp));
    memset(&pComm->sahara_reset_response_rx, 0, sizeof(struct sahara_packet_reset_resp));
    return SUCCESS;
}

/*  release com structure resources
 *
 *  PARAMETERS:
 *  pComm        - Pointer to the comm structure
 *
 *  RETURN VALUE:
 *  NONE
 */
void deinit_com_port (struct com_state *pComm, int noclosedevnode)
{

	// there is a chance the COM port will be vanish before I can close it. This is because
	// it will re-enumerate after dbl.mbn is loaded, and will cause a segmentation fault. 

    if (NULL!=pComm) {
        /* disconnect from current port */
        if(noclosedevnode)
            dbg (INFO, "closed successfully (noclosedevnode)");
        else
            disconnect (pComm);

        dbg (INFO, "free the buffer used for storing rxpayload");
        /* free the buffer used for storing rxpayload */
        if (NULL != pComm->recv_buffer) {
            free (pComm->recv_buffer);
            pComm->recv_buffer = NULL;
            free_count++;
        }

        dbg (INFO, "free tx buffer");
        /* free tx buffer */
        if(pComm->sahara_raw_tx_buffer) {
            free(pComm->sahara_raw_tx_buffer);
            pComm->sahara_raw_tx_buffer=NULL;
            free_count++;
        }

        dbg (INFO, "free debug info buffer");
        /* free debug info buffer */
        if(pComm->dload_debug_info) {
            free(pComm->dload_debug_info);
            pComm->dload_debug_info=NULL;
            free_count++;
        }

        dbg (INFO, "free upload file name");
        /* free debug info buffer */
        if(pComm->upload_file_name) {
            free(pComm->upload_file_name);
            pComm->upload_file_name=NULL;
            free_count++;
        }

        free (pComm);
        pComm = NULL;
        free_count++;
    }

	dbg (INFO, "destroy the input file list <id, file_name>");
    /* destroy the input file list <id, file_name>*/
    destroy_input_file_list();

    dbg (INFO, "Malloc count: %d, Free count: %d", malloc_count, free_count);
}


/*  connect to the specified port
 *
 *  PARAMETERS:
 *  pPort        - Name of port to open (IE: /dev/ttyUSB0)
 *  pComm        - Pointer to the comm structure
 *
 *  RETURN VALUE:
 *  int          - SUCCESS/EFAILED
 */
int connect (char *pPort, struct com_state *pComm)
{
    struct termios tio;

    if (pPort == 0 || pPort[0] == 0)
        return EFAILED;

    /* Close any existing open port */
    if (pComm->port_fd != INVALID_HANDLE_VALUE)
        disconnect (pComm);

    /* Opening the com port */
    /* Change for Android port - Opening in O_SYNC mode since tcdrain() is not supported */
    if (using_sdio)
        pComm->port_fd = open (pPort, O_RDWR | O_SYNC | O_NONBLOCK);
    else
        pComm->port_fd = open (pPort, O_RDWR | O_SYNC);
    if (pComm->port_fd == INVALID_HANDLE_VALUE)
        return EFAILED;

    if(using_uart && using_tty)
    {
        dbg (INFO, "\nUSING UART DETECTED pPort='%s'",pPort);
        dbg (INFO, "\nConfiguring 115200, 8n1\n\n");

        memset(&tio,0,sizeof(tio));
        tio.c_iflag=0;
        tio.c_oflag=0;
        tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
        tio.c_lflag=0;
        tio.c_cc[VMIN]=1;
        tio.c_cc[VTIME]=5;
        cfsetospeed(&tio,B115200);            // 115200 baud
        cfsetispeed(&tio,B115200);            // 115200 baud
        tcsetattr(pComm->port_fd,TCSANOW,&tio);
    }


    /* Clear any contents */
    /* Change for Android port - tcdrain() not supported */
    /* tcdrain (pComm->port_fd); */

    /* Save port name */
    pComm->port_name = pPort;

    return SUCCESS;
}

/*
 *  disconnect com port
 *
 *  PARAMETERS:
 *  pComm         - Pointer to the comm structure
 *
 *  RETURN VALUE:
 *  int               - SUCCESS/EFAILED
 */
int disconnect (struct com_state *pComm)
{
    /* Assume success */
    int rc = SUCCESS;

    if (NULL==pComm || INVALID_HANDLE_VALUE==pComm->port_fd)
        return rc;

    dbg (INFO, "Disconnecting from com port");
    if (pComm->port_fd != INVALID_HANDLE_VALUE) 
	{
	    tcflush (pComm->port_fd, TCIOFLUSH);
        int nClose = close (pComm->port_fd);
	    if (nClose == -1)
	        rc = EFAILED;
		else
	        dbg (INFO, "closed successfully");

        pComm->port_fd = INVALID_HANDLE_VALUE;
    }
    return rc;
}

/*
 *  Configure com port settings
 *
 *  PARAMETERS:
 *  pSettings     - Desired port settings
 *  pComm         - Pointer to the comm structure
 *
 *  RETURN VALUE:
 *  int               - SUCCESS/EFAILED
 */
int configure_settings (struct termios *pSettings, struct com_state *pComm, int protocol_type)
{
    int rc;

    if (pComm->port_fd == INVALID_HANDLE_VALUE || pSettings == 0)
        return EFAILED;

    if (protocol_type != SAHARA_PROTOCOL)
        tcflush (pComm->port_fd, TCIOFLUSH);

    rc = tcsetattr (pComm->port_fd, TCSANOW, pSettings);
    if (rc == -1)
    {
        dbg(ERROR, "String Error: %s", strerror(errno));
        return EFAILED;
    };

    /* Success! */
    return SUCCESS;
}

/*
 *  Return the current port settings
 *
 *  PARAMETERS:
 *  pSettings     - Current port settings
 *  pComm         - Pointer to the comm structure
 *
 *  RETURN VALUE:
 *  int               - SUCCESS/EFAILED
 */
int get_settings (struct termios *pSettings, struct com_state *pComm)
{
    int nrc;

    if (pComm->port_fd == INVALID_HANDLE_VALUE || pSettings == 0)
        return EFAILED;

    /* Get the COM port settings */
    nrc = tcgetattr (pComm->port_fd, pSettings);
    if (nrc == -1)
        return EFAILED;

    /* Success! */
    return SUCCESS;
}


/*
 *  Connects and configures the device
 *
 *  PARAMETERS
 *  port_number          - port name of the device
 *  protocol_type        - protocol type (DLOAD_PROTOCOL/SAHARA_PROTOCOL)
 *
 *  RETURN VALUE:
 *  Pointer to the com port structure
 */
struct com_state *connect_and_configure_device (char *port_number, int protocol_type)
{
    /* connect to the devive*/
    struct com_state *m_comm = malloc (sizeof(struct com_state));
    if (NULL==m_comm) {
        dbg (ERROR, "Memory allocation failure: %s", strerror(errno));
        goto exit;
    }
    malloc_count++;

    /*Iniatialize comm port*/
    if (EFAILED == init_com_port (m_comm)) {
        dbg (ERROR, "Error initializing com port");
        goto exit;
    }

    /*connect to the device: "/dev/usb/ttyUSB0"*/
    if (SUCCESS != connect (port_number, m_comm)) {
        dbg (ERROR, "Not Connected to the device: %s, Linux System Error:%s",
             port_number, strerror (errno));
        goto exit;
    }

    if (using_tty)
    {
        /* Configure the com port */
        struct termios settings;
        if (get_settings (&settings, m_comm) == EFAILED)
        {
            dbg (ERROR, "termio settings could not be fetched Linux System Error:%s", strerror (errno));
            goto exit;
        }

        /*CREAD to enable receiver and CLOCAL to say that the device is directly connected to the host*/
        cfmakeraw (&settings);
        settings.c_cflag |= CREAD | CLOCAL;

        /*configure the new settings*/
        if (SUCCESS != configure_settings (&settings, m_comm, protocol_type)) {
            dbg (ERROR, "Device could not be configured: Linux System Errno: %s",
                 strerror (errno));
            goto exit;
        }
    }
    return m_comm;

exit:
    deinit_com_port (m_comm, 0);
    /*
    if (NULL != m_comm) {
        free (m_comm);
        m_comm = NULL;
        free_count++;
    }
    */
    return NULL;
}


/*
 *  Transmit data
 *
 *  PARAMETERS:
 *  buffer        - Data to be transmitted
 *  buffer_size   - size of the data to be transmitted
 *  pComm         - pointer to com structure
 *
 *  RETURN VALUE:
 *  int         - SUCCESS/EFAILED
 */
int tx_data (char *buffer, unsigned long buffer_size, struct com_state *pComm)
{
    int padding = (buffer_size % 4);
    int nRet;
    unsigned long bytes_written = 0;
    char *padding_buffer;
    padding_buffer = NULL;
    if (padding && using_sdio) {
        padding = 4 - padding;
        dbg (INFO, "Size was %d, adding %d bytes of padding", buffer_size, padding);
        padding_buffer = calloc(buffer_size+padding, sizeof(char));
        if (NULL==padding_buffer) {
            dbg (ERROR, "Memory allocation failure: %s", strerror(errno));
            return EFAILED;
        }
        malloc_count++;
        memcpy(padding_buffer, buffer, buffer_size);
        buffer = padding_buffer;
        buffer_size += padding;
    }
    
    if (verbose)
        /* Log the packet size that is sent to the */
        dbg (INFO, "Transmitting %d bytes", buffer_size);
    
    while (bytes_written < buffer_size) {
        /*dbg (INFO, "Calling flush before write");
        tcflush (pComm->port_fd, TCIOFLUSH);*/
        nRet = write (pComm->port_fd, buffer+bytes_written, buffer_size-bytes_written);
        if (nRet < 0) {
            /*dbg (ERROR, "Buffer size: %d, but write returned: %d", buffer_size, nRet);*/
            dbg (ERROR, "Write returned failure %d, errno %d, System error code: %s", nRet, errno, strerror (errno));
            if (NULL!=padding_buffer) {
                free(padding_buffer);
                padding_buffer = NULL;
                free_count++;
            }
            return EFAILED;
        }
        bytes_written += nRet;
    }
    
    if (padding && using_sdio) {
        if (NULL!=padding_buffer) {
            free(padding_buffer);
            padding_buffer = NULL;
            free_count++;
        }
    }
    
    if (!sending_started) {
        sending_started = 1;
        total_bytes_sent = 0;
    }
    total_bytes_sent += buffer_size;
    dbg (INFO, "Total bytes sent so far: %d", total_bytes_sent);
    return SUCCESS;
}


/*
 *  Receive data
 *
 *  PARAMETERS:
 *  buffer        - Data to be received
 *  buffer_size   - size of the data to be received
 *  pComm         - pointer to com structure
 *
 *  RETURN VALUE:
 *  int         - SUCCESS/EFAILED
 */
int rx_data(char *buffer, unsigned long buffer_size, struct com_state *pComm, int is_command_packet)
{

    dbg(INFO, "Entered rx_data. Buffer size (i.e. expected length): %d", buffer_size);
    fd_set rfds;
    struct timeval tv;
    int retval;
    int nRet = 0;
    unsigned long bytes_read = 0;
    unsigned int command_packet_length;
    unsigned int read_fail = 0;
    unsigned int first_read_chunk = 1;
    int padding = 0;
    char padding_buffer[3];
    if (using_sdio) {
        padding = (buffer_size % 4);
        if (padding) {
            dbg (ERROR, "Padding expected. Buffer size %d", buffer_size);
            padding = 4 - padding;
        }
    }

    do {
        if (first_read_chunk || !using_sdio)
        {
            /*Init read file descriptor */
            FD_ZERO (&rfds);
            FD_SET (pComm->port_fd, &rfds);

            /* time out initializtion. */
            tv.tv_sec  = rx_timeout >= 0 ? rx_timeout : 0;
            tv.tv_usec = 0;

            retval = select (pComm->port_fd + 1, &rfds, NULL, NULL, ((rx_timeout >= 0) ? (&tv) : (NULL)));
            if (0 == retval) {
                dbg (ERROR, "Timeout Occured, No response or command came from the target !!!");
                return EFAILED;
            }
            if (retval < 0) {
                dbg (ERROR, "select returned error: %s", strerror (errno));
                return EFAILED;
            }
        }
        if (bytes_read < buffer_size)
            nRet = read (pComm->port_fd, buffer+bytes_read, MIN((buffer_size - bytes_read), MAX_READ));
        else if (bytes_read < (buffer_size + padding))
            nRet = read (pComm->port_fd, padding_buffer, MIN(((buffer_size + padding) - bytes_read), sizeof(padding_buffer)));
        if (first_read_chunk)
            first_read_chunk = 0;
        
        if (nRet <= 0) {
            read_fail++;
            if (read_fail == MAX_READ_RETRY) {
                dbg (ERROR, "Max read failures reached");
                tcflush (pComm->port_fd, TCIOFLUSH);
                return EFAILED;
            }
            if (nRet < 0) {
                if (errno != EAGAIN) {
                    dbg (ERROR, "Read/Write File descriptor returned error: %s, error code %d", strerror (errno), nRet);
                    return EFAILED;
                }
            }

            if (0 == nRet) {
                dbg (ERROR, "Zero length packet received or hardware connection went off");
                /* Pressing on despite read() failures was added to handle SDIO issues when
                 * collecting RAM dumps. Hence, making this conditional on SDIO otherwise
                 * retrying despite losing the connection floods the logs
                */
                if (!using_sdio)
                    return EFAILED;
            }
            usleep(sleep_usec);
            continue;
        }
        read_fail = 0;

        bytes_read += nRet;
        dbg (INFO, "Received %d bytes", nRet);
        if(is_command_packet) {
            if (bytes_read >= 8) {
                memcpy(&command_packet_length, buffer+4, sizeof(command_packet_length));
                buffer_size = command_packet_length;
                dbg(INFO, "Expected length changed to %d", buffer_size);
                is_command_packet = 0;
                if (using_sdio) {
                    padding = (buffer_size % 4);
                    if (padding) {
                        dbg (ERROR, "Padding expected. Buffer size %d", buffer_size);
                        padding = 4 - padding;
                    }
                }
            }
        }
        /*dbg (INFO, "%d of %d bytes read", bytes_read, buffer_size);*/
    } while (bytes_read < (buffer_size + padding));

    if (!receiving_started) {
        receiving_started = 1;
        total_bytes_recd = 0;
    }
    total_bytes_recd += (buffer_size + padding);
    /*dbg (INFO, "Total bytes received so far: %d", total_bytes_recd);*/
    return SUCCESS;
}

/*
 *  get_port_name
 *
 *  PARAMETERS:
 *  pComm       - pointer to the com structure
 *
 *  RETURN VALUE:
 *  char*       - port name
 */
char *get_port_name (struct com_state *pComm)
{
    return pComm->port_name;
}

