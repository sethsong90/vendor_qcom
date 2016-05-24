/*=============================================================================
 *  FILE :
 *  sahara_protocol.c
 *
 *  DESCRIPTION:
 *
 *  Implements the Sahara protocol
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
 *  =============================================================================
 *
 *
 *  sahara_protocol.c : Implements the Sahara protocol.
 * ==========================================================================================
 *   $Header: //source/qcom/qct/core/storage/tools/kickstart/sahara_protocol.c#13 $
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

#include "sahara_protocol.h"
#include <dirent.h>
#include <sys/select.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <malloc.h>
#include "kickstart_utils.h"

#define BITS_IN_BYTE 8
#define MILI_IN_SEC 1000LL
#define NANO_SEC_PER_USEC 1000LL
#define NANO_SEC_PER_MSEC 1000000LL
#define NANO_SEC_PER_SEC 1000000000LL
#define MICRO_SEC_PER_SEC 1000000LL

#define MAXMEMREAD 1048576
#define MAXDUMPFAIL 5

extern unsigned int malloc_count;
extern unsigned int free_count;

/*======Unexported Functions=================================================*/
/*Helper function which checks if a successful ack is received or not*/
static int is_ack_successful (int status);
/*Helper function for starting the raw data transfer*/
static int start_image_transfer (struct com_state *m_comm);
/*Helper function to send reset command to the target*/
static int send_reset_command (struct com_state *m_comm);
/*Helper function which checks if end of image transfer command is received or not*/
static int is_end_of_image_rx (unsigned int command);
/*Helper function to send done command after uploading the image to the target*/
static int send_done_packet (struct com_state *m_comm);
/*Helper function to send cmd_exec commands*/
static int send_cmd_exec_commands (struct com_state *m_comm);
/*Helper function to open files for writing - used for memory debug dumps*/
static int open_debug_file(const char *file_name);
/*Helper function to check if the received buffer contains a valid memory table*/
static int is_valid_memory_table(unsigned int memory_table_size);
/*Helper function to send a memory read packet*/
static int send_memory_read_packet (struct com_state *m_comm, unsigned int memory_table_address, unsigned int memory_table_length);
/*Helper function to calculate the throughput of sending a file across*/
void time_throughput_calculate(struct timeval time1, struct timeval time2, unsigned int size_bytes);

char *szSaharaState[13]={ "SAHARA_WAIT_HELLO",
                          "SAHARA_WAIT_COMMAND",
                          "SAHARA_WAIT_RESET_RESP",
                          "SAHARA_WAIT_DONE_RESP",
                          "SAHARA_WAIT_MEMORY_READ",
                          "SAHARA_WAIT_CMD_EXEC_RESP",
                          "SAHARA_WAIT_MEMORY_TABLE",
                          "SAHARA_WAIT_MEMORY_REGION",
                          "UNKNOWN","UNKNOWN","UNKNOWN","UNKNOWN","UNKNOWN"};


/* added vars to record throughput */
int images_transfer_started;
unsigned int overall_size, image_size;
double image_transfer_secs;
struct timeval chunk_time_start, chunk_time_end;
struct timeval overall_time_start, overall_time_end;   

/*===========================================================================
 *  METHOD:
 *  start_sahara_based_transfer
 *
 *  DESCRIPTION:
 *  Starts the sahara based transfer
 *
 *  PARAMETERS
 *  m_comm         [ I ] - Pointer to the comm port structure
 *  sahara_mode    [ I ] - Forced mode to send to target
 *
 *  RETURN VALUE:
 *  SUCCESS/EFAILED
 *  ===========================================================================*/
int start_sahara_based_transfer
(
  struct com_state *m_comm,
  enum boot_sahara_mode sahara_mode,
  char *PathToSaveFiles,
  int memdebugImage
)
{
    int retval;
    struct sahara_packet_hello_resp helloRx;
    char*  id_mapped_file;
    unsigned int cmds_executed = EFAILED;
    if (memdebugImage != -1)
        cmds_executed = SUCCESS;
    int num_debug_entries;
    int num_dump_failures;
    char full_filename[2048];

    images_transfer_started = 0;
    overall_size = 0;
    sending_started = 0;
    receiving_started = 0;
    id_mapped_file = NULL;
    helloRx.mode = sahara_mode;
    num_debug_entries = -1;
    num_dump_failures = 0;
    m_comm->sahara_state = SAHARA_WAIT_HELLO;

    while (1)
    {
        switch (m_comm->sahara_state)
        {
        case SAHARA_WAIT_HELLO:
          dbg (EVENT, "STATE <-- SAHARA_WAIT_HELLO");
          if (SUCCESS != rx_data((char*)&m_comm->sahara_hello_packet_rx,  sizeof(m_comm->sahara_hello_packet_rx), m_comm, 0)) {
            return EFAILED;
          }

          /*Reset the raw image transfer started flag to EFAILED */
          m_comm->started_raw_image_transfer = EFAILED;
           /*Check if the recieved command is a hello command */
          if (SAHARA_HELLO_ID != m_comm->sahara_hello_packet_rx.command) {
                dbg (ERROR, "Received a different command: %x while waiting for hello packet", m_comm->sahara_hello_packet_rx.command);
                /*set the state to SAHARA_WAIT_RESET_RESP*/
                dbg (EVENT, "SENDING --> SAHARA_RESET");
                if (SUCCESS != send_reset_command (m_comm)) {
                    dbg (ERROR, "Sending RESET packet failed");
                    return EFAILED;
                }
                /*set the state to SAHARA_WAIT_RESET_RESP*/
                dbg (EVENT, "STATE <-- SAHARA_WAIT_RESET_RESP");
                m_comm->sahara_state = SAHARA_WAIT_RESET_RESP;
            }
 	      else {
              dbg (EVENT, "RECEIVED <-- SAHARA_HELLO");
              dbg (EVENT, "SENDING --> SAHARA_HELLO_RESPONSE");
              /*Recieved hello, send the hello response*/
              /*Create a Hello request*/
	          memset(&helloRx, 0, sizeof(struct sahara_packet_hello_resp));
              helloRx.command = SAHARA_HELLO_RESP_ID;
              helloRx.length = sizeof(struct sahara_packet_hello_resp);
              helloRx.version = SAHARA_VERSION;
              helloRx.version_supported = SAHARA_VERSION_SUPPORTED;

              if (!(m_comm->sahara_hello_packet_rx.version <= SAHARA_VERSION && m_comm->sahara_hello_packet_rx.version >= SAHARA_VERSION_SUPPORTED)) {
 	              dbg (ERROR, "Invalid packet version %d. Current Sahara version: %d, version supported: %d", m_comm->sahara_hello_packet_rx.version, SAHARA_VERSION, SAHARA_VERSION_SUPPORTED);
 	              helloRx.status = SAHARA_NAK_INVALID_TARGET_PROTOCOL;
                  
                  dbg(ERROR, "Hello packet version: %d", m_comm->sahara_hello_packet_rx.version);
                  dbg(ERROR, "Hello packet length: %d", m_comm->sahara_hello_packet_rx.length);
                  dbg(ERROR, "Hello packet ver supported: %d", m_comm->sahara_hello_packet_rx.version_supported);
                  dbg(ERROR, "Hello packet cmd packet length: %d", m_comm->sahara_hello_packet_rx.cmd_packet_length);
                  dbg(ERROR, "Hello packet command: %d", m_comm->sahara_hello_packet_rx.command);
                  dbg(ERROR, "Hello packet mode: %d", m_comm->sahara_hello_packet_rx.mode);
			  }
			  else {
                  helloRx.status = SAHARA_STATUS_SUCCESS;
              }

              /* Checks whether the mode required by the user has been sent */
              /* if it has not been sent, it is sent out in the first hello response packet from this state */
              /* once that mode has been sent to target, the received mode value is echoed back for future responses */
              if ((sahara_mode < SAHARA_MODE_LAST) && (cmds_executed == EFAILED)) 
			  {
                  helloRx.mode  = sahara_mode;
                  cmds_executed = SUCCESS;
              }
              else 
			  {
                  helloRx.mode = m_comm->sahara_hello_packet_rx.mode;
              }
              m_comm->prev_mode = m_comm->sahara_hello_packet_rx.mode;

			  dbg(INFO, "\n\nsahara_mode                         = %u",sahara_mode);
			  dbg(INFO, "\nm_comm->sahara_hello_packet_rx.mode = %u",m_comm->sahara_hello_packet_rx.mode);
			  dbg(INFO, "\nhelloRx.mode                        = %u\n",helloRx.mode);

              /*Send the Hello  Resonse Request*/
              if (SUCCESS != tx_data ((char *)&helloRx, sizeof(struct sahara_packet_hello_resp), m_comm)) 
			  {
                dbg (ERROR, "Tx Sahara Data Failed ");
                return EFAILED;
              }
              m_comm->sahara_state = SAHARA_WAIT_COMMAND;
              use_wakelock(WAKELOCK_ACQUIRE);
             }
              break;

        case SAHARA_WAIT_COMMAND:
            if (SUCCESS != rx_data((char*)&m_comm->sahara_rx_data_payload,  sizeof(m_comm->sahara_rx_data_payload), m_comm, 1)) {
               return EFAILED;
            }
            dbg (INFO, "STATE <-- SAHARA_WAIT_COMMAND");
            /*Check if it is  an end of image Tx*/
            if (SUCCESS == is_end_of_image_rx (m_comm->sahara_rx_data_payload.command)) {
                dbg (EVENT, "RECEIVED <-- SAHARA_END_IMAGE_TX");

                /*copy the payload to sahara_end_of_image_rx to get the status*/
                memcpy(&m_comm->sahara_end_of_image_rx, &m_comm->sahara_rx_data_payload, sizeof(m_comm->sahara_end_of_image_rx));

                if (SUCCESS == is_ack_successful (m_comm->sahara_end_of_image_rx.status)) {
                    if (m_comm->sahara_end_of_image_rx.image_id == memdebugImage)
                        cmds_executed = EFAILED;
                    dbg (EVENT, "SENDING --> SAHARA_DONE");
                    send_done_packet (m_comm);
                    m_comm->sahara_state = SAHARA_WAIT_DONE_RESP;
                }
                else {
                    dbg (EVENT, "SENDING --> SAHARA_RESET");
                    if (SUCCESS != send_reset_command (m_comm)) {
                        dbg (ERROR, "Sending RESET packet failed");
                        return EFAILED;
                    }
                    /*set the state to SAHARA_WAIT_RESET_RESP*/
                    m_comm->sahara_state = SAHARA_WAIT_RESET_RESP;
                }
            }
            else if (SAHARA_READ_DATA_ID == m_comm->sahara_rx_data_payload.command) {
                dbg (EVENT, "RECEIVED <-- SAHARA_READ_DATA");
                if (EFAILED == m_comm->started_raw_image_transfer) {
                    /*Get the name of the file the target is requesting*/
                    id_mapped_file = get_file_from_file_list(m_comm->sahara_rx_data_payload.image_id);
                    dbg (STATUS, "Requested ID %d, file \"%s\"", m_comm->sahara_rx_data_payload.image_id, id_mapped_file);
                    if (NULL == id_mapped_file) {
                        dbg (ERROR, "Matching input image not found ");
                        if (SUCCESS != send_reset_command (m_comm)) {
                            dbg (ERROR, "Sending RESET packet failed");
                        }
                        m_comm->sahara_state = SAHARA_WAIT_COMMAND;
                    }else {
                        /*Load the image*/
                        dbg (INFO, "Uploading file: \"%s\"", id_mapped_file);
                        m_comm->fd = open_file(id_mapped_file);
                        m_comm->started_raw_image_transfer = SUCCESS;
                        image_size = 0;
                        gettimeofday(&chunk_time_start, NULL);
                    }
                }
                /*start the data transfer */
                if (NULL!=id_mapped_file) {
                    if (SUCCESS != start_image_transfer (m_comm)) {
                        dbg (ERROR, "start_image_transfer failed");
                        return EFAILED;
                    }
                }
            }
            else if (SAHARA_MEMORY_DEBUG_ID == m_comm->sahara_rx_data_payload.command) {
                dbg (EVENT, "RECEIVED <-- SAHARA_MEMORY_DEBUG");

                /*copy the payload to sahara_end_of_image_rx to get the status*/
                memcpy(&m_comm->sahara_memory_debug_rx, &m_comm->sahara_rx_data_payload, sizeof(m_comm->sahara_memory_debug_rx));
                dbg (INFO, "Memory Table Address: %d, Memory Table Length: %d", m_comm->sahara_memory_debug_rx.memory_table_addr, m_comm->sahara_memory_debug_rx.memory_table_length);

                if (SUCCESS != is_valid_memory_table(m_comm->sahara_memory_debug_rx.memory_table_length)) {
                    dbg (ERROR, "Invalid memory table received");
                    if (SUCCESS != send_reset_command (m_comm)) {
                        dbg (ERROR, "Sending RESET packet failed");
                        return EFAILED;
                    }
                    m_comm->sahara_state = SAHARA_WAIT_RESET_RESP;
                    break;
                }

                dbg (EVENT, "SENDING --> SAHARA_MEMORY_READ");
                if (SUCCESS != send_memory_read_packet(m_comm, m_comm->sahara_memory_debug_rx.memory_table_addr, m_comm->sahara_memory_debug_rx.memory_table_length)) {
                    dbg (ERROR, "Sending MEMORY_READ packet failed");
                    return EFAILED;
                }
                dbg (EVENT, "Sent SAHARA_MEMORY_READ, address %08X, length %d", m_comm->sahara_memory_debug_rx.memory_table_addr, m_comm->sahara_memory_debug_rx.memory_table_length);
                m_comm->recv_buffer= realloc(m_comm->recv_buffer, m_comm->sahara_memory_debug_rx.memory_table_length);
                if (NULL==m_comm->recv_buffer) {
                    dbg (ERROR, "Memory allocation failure: %s", strerror(errno));
                    return EFAILED;
                }
                m_comm->recv_buffer_size = m_comm->sahara_memory_debug_rx.memory_table_length;
                dbg (EVENT, "STATE <-- SAHARA_WAIT_MEMORY_TABLE");
                m_comm->sahara_state = SAHARA_WAIT_MEMORY_TABLE;
            }
            else if (SAHARA_CMD_READY_ID == m_comm->sahara_rx_data_payload.command) {
                dbg (EVENT, "RECEIVED <-- SAHARA_CMD_READY");

                if (SUCCESS != send_cmd_exec_commands (m_comm)) {
                    dbg (ERROR, "Send CMD_EXEC packet failed");

                    if (SUCCESS != send_reset_command (m_comm)) {
                        dbg (ERROR, "Sending RESET packet failed");
                        return EFAILED;
                    }
                    /*set the state to SAHARA_WAIT_RESET_RESP*/
                    dbg (EVENT, "STATE <-- SAHARA_WAIT_RESET_RESP");
                    m_comm->sahara_state = SAHARA_WAIT_RESET_RESP;
                }
                else {
                    m_comm->sahara_state = SAHARA_WAIT_HELLO;
                }
            }
            else if (SAHARA_HELLO_ID == m_comm->sahara_rx_data_payload.command) {
                dbg (INFO, "Received HELLO command out of sequence. Discarding.");
                use_wakelock(WAKELOCK_RELEASE);
                m_comm->sahara_state = SAHARA_WAIT_HELLO;
            }
            else {
                dbg (ERROR, "Received an unknown command: %d ", m_comm->sahara_rx_data_payload.command);
                /*set the state to SAHARA_WAIT_RESET_RESP*/
                dbg (EVENT, "SENDING --> SAHARA_RESET");
                if (SUCCESS != send_reset_command (m_comm)) {
                    dbg (ERROR, "Sending RESET packet failed");
                    return EFAILED;
                }
                /*set the state to SAHARA_WAIT_RESET_RESP*/
                dbg (EVENT, "STATE <-- SAHARA_WAIT_RESET_RESP");
                m_comm->sahara_state = SAHARA_WAIT_RESET_RESP;
            }
            break;

        case SAHARA_WAIT_MEMORY_TABLE:
            if (SUCCESS != rx_data((char*)m_comm->recv_buffer, m_comm->sahara_memory_debug_rx.memory_table_length, m_comm, 0)) {
               return EFAILED;
            }

            m_comm->dload_debug_info = (struct dload_debug_type *)realloc(m_comm->dload_debug_info, m_comm->sahara_memory_debug_rx.memory_table_length);
            if (NULL==m_comm->dload_debug_info) {
                dbg (ERROR, "Memory allocation failure: %s", strerror(errno));
                return EFAILED;
            }
            unsigned int i;

            dbg(INFO, "Memory Debug table received");
            for(i = 0; i < m_comm->sahara_memory_debug_rx.memory_table_length/sizeof(struct dload_debug_type); i++) 
            {
                //dbg (INFO,"Performing memcpy %i of %i",i+1,m_comm->sahara_memory_debug_rx.memory_table_length/sizeof(struct dload_debug_type));
                //dbg (INFO,"sizeof(m_comm->dload_debug_info[i])=%u",sizeof(m_comm->dload_debug_info[i]));
                //dbg (INFO,"sizeof(struct dload_debug_type)=%u\n",sizeof(struct dload_debug_type));

                memcpy(&(m_comm->dload_debug_info[i]), (struct dload_debug_type *)((m_comm->recv_buffer) + (i*sizeof(struct dload_debug_type))), sizeof(struct dload_debug_type));
                dbg(EVENT, "0x%.8X, len=%.8X, \"%s\", \"%s\"",m_comm->dload_debug_info[i].mem_base, m_comm->dload_debug_info[i].length, m_comm->dload_debug_info[i].filename, m_comm->dload_debug_info[i].desc);
            }
            num_debug_entries = i-1;

            dbg (INFO,"\nnum_debug_entries=%i",num_debug_entries);

            if(PathToSaveFiles!=NULL) {
                if(create_path(PathToSaveFiles, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
                    dbg (ERROR, "Could not create directory \"%s\" to save files into", PathToSaveFiles);
                    return EFAILED;
                }
            }

            /* no break; Simply falling into next state */
            /* no break; Simply falling into next state */
            /* no break; Simply falling into next state */
            dbg (INFO,"\n\nNOT BREAKING, falling into case SAHARA_WAIT_MEMORY_REGION  *************");

        case SAHARA_WAIT_MEMORY_REGION:
            /* TODO: Add check for invalid raw data or end of image tx ? how ? */

            /* Since previous state case block falls into this block, check whether state
             * is actually SAHARA_WAIT_MEMORY_REGION or not
             */

            dbg (INFO,"Inside case SAHARA_WAIT_MEMORY_REGION");
            dbg (INFO,"m_comm->sahara_state=%u, SAHARA_WAIT_MEMORY_REGION=%u, ",m_comm->sahara_state,SAHARA_WAIT_MEMORY_REGION);

            if (m_comm->sahara_state == SAHARA_WAIT_MEMORY_REGION) 
            {
                if (SUCCESS != rx_data((char*)m_comm->recv_buffer, m_comm->recv_buffer_size, m_comm, 0)) {
                    num_dump_failures++;
                    if (num_dump_failures == MAXDUMPFAIL) {
                        return EFAILED;
                    }
                    dbg (WARN, "Retrying request for RAM dump chunk.");
                }
                else {
                    num_dump_failures = 0;
                    dbg(EVENT, "Received: %d bytes", m_comm->recv_buffer_size);

                    m_comm->dload_debug_info[num_debug_entries].mem_base += m_comm->recv_buffer_size;
                    m_comm->dload_debug_info[num_debug_entries].length -= m_comm->recv_buffer_size;

                    dbg (EVENT, "Writing to disk");
                    retval = write(m_comm->fd, m_comm->recv_buffer, m_comm->recv_buffer_size);
                    if (retval<0) {
                        dbg (ERROR, "file write failed: %s", strerror(errno));
                        return EFAILED;
                    }
                    if (retval != m_comm->recv_buffer_size) {
                        dbg (WARN, "Wrote only %d of %d bytes", retval, m_comm->recv_buffer_size);
                    }
                    dbg (EVENT, "Successfully wrote to disk");

                    if (m_comm->dload_debug_info[num_debug_entries].length == 0)
                    {
                        dbg(STATUS, "Received file \"%s\"", full_filename);
                        num_debug_entries--;
                        close_file(m_comm->fd);
                        gettimeofday(&chunk_time_end, NULL);
                        image_transfer_secs = ((double)(chunk_time_end.tv_sec - chunk_time_start.tv_sec)) + ((double)(chunk_time_end.tv_usec - chunk_time_start.tv_usec))/MICRO_SEC_PER_SEC;
                        dbg (STATUS, "%d bytes transferred in %.3fs (%.2f MBps)", image_size, image_transfer_secs, (image_size/image_transfer_secs)/MAX_SAHARA_TX_BUFFER_SIZE);
                        if (num_debug_entries >= 0)
                        {
                            if(PathToSaveFiles!=NULL)
                            {
                                if (strlcpy(full_filename,PathToSaveFiles, sizeof(full_filename)) >= sizeof(full_filename)) {
                                    dbg (ERROR, "String was truncated while copying");
                                    return EFAILED;
                                }
                                if (strlcat(full_filename,upload_prefix_string, sizeof(full_filename)) >= sizeof(full_filename)) {
                                    dbg (ERROR, "String was truncated while concatenating");
                                    return EFAILED;
                                }
                                if (strlcat(full_filename,m_comm->dload_debug_info[num_debug_entries].filename, sizeof(full_filename)) >= sizeof(full_filename)) {
                                    dbg (ERROR, "String was truncated while concatenating");
                                    return EFAILED;
                                }
                            }
                            else {
                                if (strlcpy(full_filename,upload_prefix_string, sizeof(full_filename)) >= sizeof(full_filename)) {
                                    dbg (ERROR, "String was truncated while copying");
                                    return EFAILED;
                                }
                                if (strlcat(full_filename,m_comm->dload_debug_info[num_debug_entries].filename, sizeof(full_filename)) >= sizeof(full_filename)) {
                                    dbg (ERROR, "String was truncated while copying");
                                    return EFAILED;
                                }
                            }
                            dbg (EVENT,"Saving \"%s\"", full_filename );
                            m_comm->fd = open_debug_file( full_filename );

                            //m_comm->fd = open_debug_file(m_comm->dload_debug_info[num_debug_entries].filename);
                            image_size = 0;
                            gettimeofday(&chunk_time_start, NULL);
                        }
                    }
                }
            }
            else 
            {
                dbg (INFO,"ELSE m_comm->sahara_state=%u, SAHARA_WAIT_MEMORY_REGION=%u, ",m_comm->sahara_state,SAHARA_WAIT_MEMORY_REGION);

                dbg (EVENT, "STATE <-- SAHARA_WAIT_MEMORY_REGION");
                m_comm->sahara_state = SAHARA_WAIT_MEMORY_REGION;

                dbg (INFO,"num_debug_entries=%i",num_debug_entries);

                if (num_debug_entries >= 0) 
				{
                    if(PathToSaveFiles!=NULL)
                    {
						if (strlcpy(full_filename,PathToSaveFiles, sizeof(full_filename)) >= sizeof(full_filename)) {
                            dbg (ERROR, "String was truncated while copying");
                            return EFAILED;
                        }
                        if (strlcat(full_filename,upload_prefix_string, sizeof(full_filename)) >= sizeof(full_filename)) {
                            dbg (ERROR, "String was truncated while concatenating");
                            return EFAILED;
                        }
                        if (strlcat(full_filename,m_comm->dload_debug_info[num_debug_entries].filename, sizeof(full_filename)) >= sizeof(full_filename)) {
                            dbg (ERROR, "String was truncated while concatenating");
                            return EFAILED;
                        }
                    }
                    else {
                        if (strlcpy(full_filename,upload_prefix_string, sizeof(full_filename)) >= sizeof(full_filename)) {
                            dbg (ERROR, "String was truncated while copying");
                            return EFAILED;
                        }
                        if (strlcat(full_filename,m_comm->dload_debug_info[num_debug_entries].filename, sizeof(full_filename)) >= sizeof(full_filename)) {
                            dbg (ERROR, "String was truncated while copying");
                            return EFAILED;
                        }
                    }

					dbg (EVENT,"Saving \"%s\"", full_filename );
                    m_comm->fd = open_debug_file( full_filename );
                    image_size = 0;
                    gettimeofday(&chunk_time_start, NULL);
                }
                else
                {
                    dbg (INFO,"NOT if (num_debug_entries >= 0) ");
                }
            }
            int memory_table_addr;
            int memory_table_length;


            if (num_debug_entries >= 0)
            {
		        memory_table_addr = m_comm->dload_debug_info[num_debug_entries].mem_base;
                memory_table_length = m_comm->dload_debug_info[num_debug_entries].length < MAXMEMREAD ? m_comm->dload_debug_info[num_debug_entries].length : MAXMEMREAD;

                dbg (EVENT, "SENDING --> SAHARA_MEMORY_READ");
                if (SUCCESS != send_memory_read_packet(m_comm, memory_table_addr, memory_table_length)) 
                {
                    dbg (ERROR, "Sending MEMORY_READ packet failed");
                    return EFAILED;
                }
                else
                    dbg (EVENT, "Sent SAHARA_MEMORY_READ, address %08X, length %d", memory_table_addr, memory_table_length);

                image_size += memory_table_length;
                //m_comm->recv_buffer= realloc(m_comm->recv_buffer, memory_table_length);
                free(m_comm->recv_buffer);
                free_count++;
                m_comm->recv_buffer_size = 0;
                m_comm->recv_buffer = valloc(memory_table_length);
                if (NULL==m_comm->recv_buffer) {
                    dbg (ERROR, "Memory allocation failure: %s", strerror(errno));
                    return EFAILED;
                }
                malloc_count++;
                m_comm->recv_buffer_size = memory_table_length;
                
/*
                if (num_debug_entries == 0) 
                {
                    dbg(EVENT, "num_debug_entries is now zero. Sending RESET...");
                    if (SUCCESS != send_reset_command (m_comm)) {
                        dbg (ERROR, "Sending RESET packet failed");
                        return EFAILED;
                    }
                    m_comm->sahara_state = SAHARA_WAIT_RESET_RESP;
                }
*/
            }
            else {
                dbg(EVENT, "num_debug_entries not >=0");
                dbg (STATUS, "Successfully downloaded files from target ");
                if (sahara_mode != SAHARA_MODE_MEMORY_DEBUG) {
                    m_comm->sahara_cmd_switch_mode_tx.command = SAHARA_CMD_SWITCH_MODE_ID;
                    m_comm->sahara_cmd_switch_mode_tx.length = sizeof(struct sahara_packet_cmd_switch_mode);
                    m_comm->sahara_cmd_switch_mode_tx.mode = SAHARA_MODE_IMAGE_TX_PENDING;
                    if (SUCCESS != tx_data ((char *)&(m_comm->sahara_cmd_switch_mode_tx), sizeof(struct sahara_packet_cmd_switch_mode), m_comm)) {
                        dbg (ERROR, "Sending CMD_SWITCH_MODE packet failed");
                        return EFAILED;
                    }
                    use_wakelock(WAKELOCK_RELEASE);
                    m_comm->sahara_state = SAHARA_WAIT_HELLO;
                }
                else {
                    dbg (EVENT, "SENDING --> SAHARA_RESET");
                    if (SUCCESS != send_reset_command (m_comm)) {
                        dbg (ERROR, "Sending RESET packet failed");
                        return EFAILED;
                    }
                    use_wakelock(WAKELOCK_RELEASE);
                    m_comm->sahara_state = SAHARA_WAIT_RESET_RESP;
                }
            }
            break;

        case SAHARA_WAIT_DONE_RESP:
          if (SUCCESS != rx_data((char*)&m_comm->sahara_done_response_rx,  sizeof(m_comm->sahara_done_response_rx), m_comm, 0)) {
            return EFAILED;
          }
          gettimeofday(&chunk_time_end, NULL);
          image_transfer_secs = ((double)(chunk_time_end.tv_sec - chunk_time_start.tv_sec)) + ((double)(chunk_time_end.tv_usec - chunk_time_start.tv_usec))/MICRO_SEC_PER_SEC;
          dbg (STATUS, "%d bytes transferred in %.3fs (%.2f MBps)", image_size, image_transfer_secs, (image_size/image_transfer_secs)/MAX_SAHARA_TX_BUFFER_SIZE);
          dbg (INFO,"SAHARA_WAIT_DONE_RESP recieved with SAHARA_MODE_IMAGE_TX_PENDING=0x%.8X\n",SAHARA_MODE_IMAGE_TX_PENDING);
          dbg (EVENT, "STATE <-- SAHARA_WAIT_DONE_RESP");

          use_wakelock(WAKELOCK_RELEASE);
          if (SAHARA_MODE_IMAGE_TX_PENDING == m_comm->sahara_done_response_rx.image_tx_status) {
               dbg (INFO, "Still More images to be uploaded, entering Hello wait state");
               m_comm->sahara_state = SAHARA_WAIT_HELLO;
          }
          else if (SAHARA_MODE_IMAGE_TX_COMPLETE == m_comm->sahara_done_response_rx.image_tx_status) {
               dbg (EVENT,"\n\nSuccessfully uploaded all images\n");
               //delete_file_from_list(id_mapped_file); // taken care of in destroy_input_file_list()
               
               gettimeofday(&overall_time_end, NULL);
               dbg(INFO, "Calculating overall throughput..");
               time_throughput_calculate(overall_time_start, overall_time_end, overall_size);
               return SUCCESS;
          }
          else {
	       dbg (ERROR, "Received unrecognized status %d at SAHARA_WAIT_DONE_RESP state",  m_comm->sahara_done_response_rx.image_tx_status);
	       return EFAILED;
          }

          /*delete the file from the file list*/
          //delete_file_from_list(id_mapped_file); // taken care of in destroy_input_file_list()
          break;

        case SAHARA_WAIT_RESET_RESP:
          if (SUCCESS == rx_data((char*)&m_comm->sahara_reset_response_rx,  sizeof(m_comm->sahara_reset_response_rx), m_comm, 0)) {
                if (SAHARA_RESET_RESP_ID != m_comm->sahara_reset_response_rx.command) {
                    dbg (INFO,"Waiting for reset response code %i, received %i instead.", SAHARA_RESET_RESP_ID, m_comm->sahara_reset_response_rx.command);
                    continue;
                }
          }
          else{
              dbg(ERROR, "read failed: Linux system error: %s", strerror(errno));
              return EFAILED;
          }
          dbg (EVENT, "RECEIVED <-- SAHARA_RESET_RESP");

          if(sahara_mode != helloRx.mode) // m_comm->sahara_hello_packet_rx.mode
          {
              // we wanted to load the build, but we were forced to do a memory dump
              LOGI("EXITING WITH 1280");
              deinit_com_port (m_comm, 0);
              exit(5);
          }
          else
              return SUCCESS;

        default:
          dbg (ERROR, "Unrecognized state %d",  m_comm->sahara_state);
	      return EFAILED;
        } /* end switch */
    } /* end while (1) */
}

/*===========================================================================
 *  METHOD:
 *  is_end_of_image_rx
 *
 *  DESCRIPTION:
 *  Checks the stautus field and responds or logs the error condition
 *
 *  PARAMETERS
 *  m_comm         - Pointer to the comm port object
 *
 *
 *  RETURN VALUE:
 *  int            SUCCESS/EFAILED
 *  ===========================================================================*/
static int is_end_of_image_rx (unsigned int command)
{
    if (command == SAHARA_END_IMAGE_TX_ID)
        return SUCCESS;
    return EFAILED;
}

/*===========================================================================
 *  METHOD:
 *  send_reset_command
 *
 *  DESCRIPTION:
 *  Sends reset commmand to target
 *
 *  PARAMETERS
 *  m_comm       - Pointer to the comm port object
 *
 *
 *  RETURN VALUE:
 *  int           SUCCESS/EFAILED
 *  ===========================================================================*/
static int send_reset_command (struct com_state *m_comm)
{
    struct sahara_packet_reset resetTx;

    resetTx.command = SAHARA_RESET_ID;
    resetTx.length = sizeof(struct sahara_packet_reset);

    /* Send the Reset Request */
    if (SUCCESS != tx_data ((char *)&resetTx, sizeof(struct sahara_packet_reset), m_comm)) {
        fprintf (stderr, "Tx Sahara Data Failed ");
        return EFAILED;
    }

    return SUCCESS;
}
/*===========================================================================
 *  METHOD:
 *  is_ack_successful
 *
 *  DESCRIPTION:
 *  Checks the stautus field and responds or logs the error condition
 *
 *  PARAMETERS
 *  status      4 byte status field from the rx payload
 *
 *
 *  RETURN VALUE:
 *  int           SUCCESS/EFAILED
 *  ===========================================================================*/
static int is_ack_successful (int status)
{
    switch (status) {

    /*Success*/
    case SAHARA_STATUS_SUCCESS :
        dbg (INFO, "SAHARA_STATUS_SUCCESS");
        return SUCCESS;

    /*Invalid command received in current state*/
    case SAHARA_NAK_INVALID_CMD:
        dbg (ERROR, "SAHARA_NAK_INVALID_CMD");
        break;

    /*Protocol mismatch between host and target*/
    case SAHARA_NAK_PROTOCOL_MISMATCH:
        dbg (ERROR, "SAHARA_NAK_PROTOCOL_MISMATCH");
        break;

    /*Invalid target protocol version*/
    case SAHARA_NAK_INVALID_TARGET_PROTOCOL:
        dbg (ERROR, "SAHARA_NAK_INVALID_TARGET_PROTOCOL");
        break;

    /*Invalid host protocol version*/
    case SAHARA_NAK_INVALID_HOST_PROTOCOL:
        dbg (ERROR, "SAHARA_NAK_INVALID_HOST_PROTOCOL");
        break;

    /*Invalid packet size received*/
    case SAHARA_NAK_INVALID_PACKET_SIZE:
        dbg (ERROR, "SAHARA_NAK_INVALID_PACKET_SIZE");
        break;

    /*Unexpected image ID received*/
    case SAHARA_NAK_UNEXPECTED_IMAGE_ID:
        dbg (ERROR, "SAHARA_NAK_UNEXPECTED_IMAGE_ID");
        break;

    /*Invalid image header size received*/
    case SAHARA_NAK_INVALID_HEADER_SIZE:
        dbg (ERROR, "SAHARA_NAK_INVALID_HEADER_SIZE");
        break;

    /*Invalid image data size received*/
    case SAHARA_NAK_INVALID_DATA_SIZE:
        dbg (ERROR, "SAHARA_NAK_INVALID_DATA_SIZE");
        break;

    /*Unsupported image type received*/
    case SAHARA_NAK_INVALID_IMAGE_TYPE:
        dbg (ERROR, "SAHARA_NAK_INVALID_IMAGE_TYPE");
        break;

    /*Invalid tranmission length*/
    case SAHARA_NAK_INVALID_TX_LENGTH:
        dbg (ERROR, "SAHARA_NAK_INVALID_TX_LENGTH");
        break;

    /*Invalid reception length*/
    case SAHARA_NAK_INVALID_RX_LENGTH :
        dbg (ERROR, "SAHARA_NAK_INVALID_RX_LENGTH");
        break;

    /*General transmission or reception error*/
    case  SAHARA_NAK_GENERAL_TX_RX_ERROR:
        dbg (ERROR, "SAHARA_NAK_GENERAL_TX_RX_ERROR");
        break;

    /*Error while transmitting READ_DATA packet*/
    case SAHARA_NAK_READ_DATA_ERROR:
        dbg (ERROR, "SAHARA_NAK_READ_DATA_ERROR");
        break;

    /*Cannot receive specified number of program headers*/
    case SAHARA_NAK_UNSUPPORTED_NUM_PHDRS:
        dbg (ERROR, "SAHARA_NAK_UNSUPPORTED_NUM_PHDRS");
        break;

    /*Invalid data length received for program headers*/
    case SAHARA_NAK_INVALID_PDHR_SIZE:
        dbg (ERROR, "SAHARA_NAK_INVALID_PDHR_SIZE");
        break;

    /*Multiple shared segments found in ELF image*/
    case SAHARA_NAK_MULTIPLE_SHARED_SEG:
        dbg (ERROR, "SAHARA_NAK_MULTIPLE_SHARED_SEG");
        break;

    /*Uninitialized program header location*/
    case SAHARA_NAK_UNINIT_PHDR_LOC:
        dbg (ERROR, "SAHARA_NAK_UNINIT_PHDR_LOC");
        break;

    /* Invalid destination address*/
    case  SAHARA_NAK_INVALID_DEST_ADDR:
       dbg (ERROR, "SAHARA_NAK_INVALID_DEST_ADDR");
       break;

    /* Invalid data size receieved in image header*/
    case SAHARA_NAK_INVALID_IMG_HDR_DATA_SIZE:
       dbg (ERROR, "SAHARA_NAK_INVALID_IMG_HDR_DATA_SIZE");
       break;

    /* Invalid ELF header received*/
    case SAHARA_NAK_INVALID_ELF_HDR:
       dbg (ERROR, "SAHARA_NAK_INVALID_ELF_HDR");
       break;

    /* Unknown host error received in HELLO_RESP*/
    case SAHARA_NAK_UNKNOWN_HOST_ERROR:
       dbg (ERROR, "SAHARA_NAK_UNKNOWN_HOST_ERROR");
       break;

    // Timeout while receiving data
    case SAHARA_NAK_TIMEOUT_RX:
       dbg (ERROR, "SAHARA_NAK_TIMEOUT_RX");
       break;

    // Timeout while transmitting data
    case SAHARA_NAK_TIMEOUT_TX:
       dbg (ERROR, "SAHARA_NAK_TIMEOUT_TX");
       break;

    // Invalid mode received from host
    case SAHARA_NAK_INVALID_HOST_MODE:
       dbg (ERROR, "SAHARA_NAK_INVALID_HOST_MODE");
       break;

    // Invalid memory read access
    case SAHARA_NAK_INVALID_MEMORY_READ:
       dbg (ERROR, "SAHARA_NAK_INVALID_MEMORY_READ");
       break;

    // Host cannot handle read data size requested
    case SAHARA_NAK_INVALID_DATA_SIZE_REQUEST:
       dbg (ERROR, "SAHARA_NAK_INVALID_DATA_SIZE_REQUEST");
       break;

    // Memory debug not supported
    case SAHARA_NAK_MEMORY_DEBUG_NOT_SUPPORTED:
       dbg (ERROR, "SAHARA_NAK_MEMORY_DEBUG_NOT_SUPPORTED");
       break;

    // Invalid mode switch
    case SAHARA_NAK_INVALID_MODE_SWITCH:
       dbg (ERROR, "SAHARA_NAK_INVALID_MODE_SWITCH");
       break;

    // Failed to execute command
    case SAHARA_NAK_CMD_EXEC_FAILURE:
       dbg (ERROR, "SAHARA_NAK_CMD_EXEC_FAILURE");
       break;

    // Invalid parameter passed to command execution
    case SAHARA_NAK_EXEC_CMD_INVALID_PARAM:
       dbg (ERROR, "SAHARA_NAK_EXEC_CMD_INVALID_PARAM");
       break;

    // Unsupported client command received
    case SAHARA_NAK_EXEC_CMD_UNSUPPORTED:
       dbg (ERROR, "SAHARA_NAK_EXEC_CMD_UNSUPPORTED");
       break;

    // Invalid client command received for data response
    case SAHARA_NAK_EXEC_DATA_INVALID_CLIENT_CMD:
       dbg (ERROR, "SAHARA_NAK_EXEC_DATA_INVALID_CLIENT_CMD");
       break;

    // Failed to authenticate hash table
    case SAHARA_NAK_HASH_TABLE_AUTH_FAILURE:
       dbg (ERROR, "SAHARA_NAK_HASH_TABLE_AUTH_FAILURE");
       break;

    // Failed to verify hash for a given segment of ELF image
    case SAHARA_NAK_HASH_VERIFICATION_FAILURE:
       dbg (ERROR, "SAHARA_NAK_HASH_VERIFICATION_FAILURE");
       break;

    // Failed to find hash table in ELF image
    case SAHARA_NAK_HASH_TABLE_NOT_FOUND:
       dbg (ERROR, "SAHARA_NAK_HASH_TABLE_NOT_FOUND");
       break;

    case SAHARA_NAK_LAST_CODE:
        dbg (ERROR, "SAHARA_NAK_LAST_CODE");
        break;

    default:
        dbg (ERROR, "Invalid status field %d", status);
        break;
    }
    return EFAILED;
}


/*===========================================================================
 *  METHOD:
 *  start_image_transfer
 *
 *  DESCRIPTION:
 *  starts the raw image transfer
 *
 *  PARAMETERS
 *  m_comm         - Pointer to the comm structure
 *
 *
 *  RETURN VALUE:
 *  int            SUCCESS/EFAILED
 *  ===========================================================================*/
static int start_image_transfer (struct com_state *m_comm)
{
    int retval = 0;
    int is_reg_file = is_regular_file(m_comm->fd);

    if (is_reg_file == -1) {
        dbg (ERROR, "Could not get file info.");
        return EFAILED;
    }

    dbg (INFO, "start_image_transfer: ");
    /*check if the input request is valid*/
    if ((0 == m_comm->sahara_rx_data_payload.data_length) ||
        (is_reg_file == 1  &&  m_comm->sahara_rx_data_payload.data_length > get_file_size(m_comm->fd))) {
          dbg (ERROR, "Invalid length %d bytes request to be transmitted", m_comm->sahara_rx_data_payload.data_length);
          return EFAILED;
    }

    if (m_comm->sahara_rx_data_payload.data_length > MAX_SAHARA_TX_BUFFER_SIZE) {
       m_comm->sahara_raw_tx_buffer= realloc(m_comm->sahara_raw_tx_buffer, m_comm->sahara_rx_data_payload.data_length);
       if (NULL==m_comm->sahara_raw_tx_buffer) {
          dbg (ERROR, "Memory allocation failure: %s", strerror(errno));
          return EFAILED;
       }
    }

    /*read data to buffer based on the offset*/
    lseek(m_comm->fd, m_comm->sahara_rx_data_payload.data_offset, SEEK_SET);
    retval = read(m_comm->fd, m_comm->sahara_raw_tx_buffer, m_comm->sahara_rx_data_payload.data_length);
    if (retval<0) {
          dbg (ERROR, "file read failed: %s", strerror(errno));
          return EFAILED;
    }
    if (retval != m_comm->sahara_rx_data_payload.data_length) {
        dbg (ERROR, "Read %d bytes, but was asked for %d bytes", retval, m_comm->sahara_rx_data_payload.data_length);
    }

    if (!images_transfer_started) {
        gettimeofday(&overall_time_start, NULL);
        images_transfer_started = 1;
    }

    /*send the image data*/
    if (SUCCESS != tx_data (m_comm->sahara_raw_tx_buffer, m_comm->sahara_rx_data_payload.data_length, m_comm)) {
        dbg (ERROR, "Tx Sahara Image Failed");
        return EFAILED;
    }
    
    overall_size += m_comm->sahara_rx_data_payload.data_length;
    image_size += m_comm->sahara_rx_data_payload.data_length;
    
    return SUCCESS;
}

/*===========================================================================
 *  METHOD:
 *  send_done_packet
 *
 *  DESCRIPTION:
 *  Sends the DONE command
 *
 *  PARAMETERS
 *  m_comm          - Pointer to the comm structure
 *
 *
 *  RETURN VALUE:
 *  int            SUCCESS/EFAILED
 *  ===========================================================================*/
static int send_done_packet (struct com_state *m_comm)
{
    struct sahara_packet_done txDonePacket;

    txDonePacket.command = SAHARA_DONE_ID;
    txDonePacket.length = sizeof(struct sahara_packet_done);
    /*Send the image data*/
    if (SUCCESS != tx_data ((char *)&txDonePacket, txDonePacket.length, m_comm)) {
        dbg (ERROR, "Sending DONE packet failed");
        return EFAILED;
    }
    return SUCCESS;
}

/*===========================================================================
 *  METHOD:
 *  send_memory_read_packet
 *
 *  DESCRIPTION:
 *  Sends the MEMORY_READ command
 *
 *  PARAMETERS
 *  m_comm          - Pointer to the comm structure
 *
 *
 *  RETURN VALUE:
 *  int            SUCCESS/EFAILED
 *  ===========================================================================*/
static int send_memory_read_packet (struct com_state *m_comm, unsigned int memory_table_address, unsigned int memory_table_length)
{
    struct sahara_packet_memory_read memReadPacket;

    memReadPacket.command = SAHARA_MEMORY_READ_ID;
    memReadPacket.length = sizeof(memReadPacket);
    memReadPacket.memory_addr = memory_table_address;
    memReadPacket.memory_length = memory_table_length;

    /* Send the Memory Read packet */
    if (SUCCESS != tx_data ((char *)&memReadPacket, memReadPacket.length, m_comm)) {
        dbg (ERROR, "Sending MEMORY_READ packet failed");
        return EFAILED;
    }
    return SUCCESS;
}


/*===========================================================================
 *  METHOD:
 *  is_valid_memory_table
 *
 *  DESCRIPTION:
 *  Checks validity of memory table received from target
 *
 *  PARAMETERS
 *  memory_table_size     - memory table size
 *
 *
 *  RETURN VALUE:
 *  int            SUCCESS/EFAILED
 *  ===========================================================================*/
static int is_valid_memory_table(unsigned int memory_table_size)
{
    if (memory_table_size % sizeof(struct dload_debug_type) == 0) {
    	return SUCCESS;
	}
    return EFAILED;
}

/*===========================================================================
 *  METHOD:
 *  open debug file
 *
 *  DESCRIPTION:
 *  wrapper for file open system call, to open file for writing
 *
 *  PARAMETERS
 *  file_name  - file to be opened
 *
 *  RETURN VALUE:
 *  int        - -1 or valid file descriptor
 *  ===========================================================================*/
static int open_debug_file(const char *file_name)
{
    int fd = -1;
    int flags = O_CREAT | O_WRONLY | O_TRUNC;
    struct stat fileInfo;

    if (file_name == 0 || file_name[0] == 0) {
        dbg (ERROR, "File not found");
        return -1;
    }
    if (stat(file_name, &fileInfo) == 0 && S_ISBLK(fileInfo.st_mode)) {
        // If the file is a block device
        flags = flags | O_DIRECT;
    }

    /* Open the file */
    fd = open (file_name,
               flags,
               S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (fd == -1) {
        dbg (ERROR, "\nUnable to open file \"%s\".  Error %d: %s\n",
             file_name,
             errno,
             strerror (errno));
        return -1;;
    }
    return fd;
}

/*===========================================================================
 *  METHOD:
 *  send_cmd_exec_commands
 *
 *  DESCRIPTION:
 *  Sends the CMD_EXEC commands and retrieves responses
 *
 *  PARAMETERS
 *  m_comm          - Pointer to the comm structure
 *
 *
 *  RETURN VALUE:
 *  int            SUCCESS/EFAILED
 *  ===========================================================================*/
static int send_cmd_exec_commands (struct com_state *m_comm)
{
    fd_set rfds;
    struct timeval tv;
    int retval;
    int print_index = 0;

    /* commands to execute */
    unsigned int exec_cmd_index = 0;
    /* response buffer for command execution - hardcode to 4KB */
    unsigned char exec_cmds_resp[4096];

    struct sahara_packet_cmd_exec txCmdExecPacket;
    struct sahara_packet_cmd_exec_data txCmdExecDataPacket;
    struct sahara_packet_cmd_switch_mode txCmdSwitchModePacket;

    for (exec_cmd_index = SAHARA_EXEC_CMD_NOP;
         exec_cmd_index < SAHARA_EXEC_CMD_LAST;
         exec_cmd_index++)
    {
        txCmdExecPacket.command = SAHARA_CMD_EXEC_ID;
        txCmdExecPacket.length = sizeof(struct sahara_packet_cmd_exec);
        txCmdExecPacket.client_command = exec_cmd_index;

        dbg (EVENT, "SENDING --> SAHARA_CMD_EXEC");

        /*Send the image data*/
        if (SUCCESS != tx_data ((char *)&txCmdExecPacket, txCmdExecPacket.length, m_comm)) {
            dbg (ERROR, "Sending CMD_EXEC packet failed");
            return EFAILED;
        }

        /*set the state to SAHARA_WAIT_CMD_EXEC_RESP*/
        dbg (EVENT, "STATE <-- SAHARA_WAIT_CMD_EXEC_RESP");
        m_comm->sahara_state = SAHARA_WAIT_CMD_EXEC_RESP;

        /*Init read file descriptor */
        FD_ZERO (&rfds);
        FD_SET (m_comm->port_fd, &rfds);

        /* time out initializtion. */
        tv.tv_sec = 60;
        tv.tv_usec = 0;

        retval = select (m_comm->port_fd + 1, &rfds, NULL, NULL, &tv);
        /*Check if timeout had occured*/
        if (0 == retval) {
            dbg (WARN, "Timeout Occured, No response or command came from the target !!!");
            return EFAILED;
        }

        /* Retreive CMD_EXEC_RESP */
        if (SUCCESS != rx_data((char*)&m_comm->sahara_cmd_exec_response_rx,
                              sizeof(m_comm->sahara_cmd_exec_response_rx),
                              m_comm, 0))
        {
           return EFAILED;
        }

        if (m_comm->sahara_cmd_exec_response_rx.command == SAHARA_CMD_EXEC_RESP_ID) {
            dbg (EVENT, "RECEIVED <-- SAHARA_CMD_EXEC_RESP");
            if (m_comm->sahara_cmd_exec_response_rx.resp_length > 0) {
                txCmdExecDataPacket.command = SAHARA_CMD_EXEC_DATA_ID;
                txCmdExecDataPacket.length = sizeof(struct sahara_packet_cmd_exec_data);
                txCmdExecDataPacket.client_command = exec_cmd_index;

                dbg (EVENT, "SENDING --> SAHARA_CMD_EXEC_DATA");

                /*Send the packet data*/
                if (SUCCESS != tx_data ((char *)&txCmdExecDataPacket,
                                        txCmdExecDataPacket.length, m_comm)) {
                    dbg (ERROR, "Sending CMD_EXEC_DATA packet failed");
                    return EFAILED;
                }

                /* Get response data */
                retval = select (m_comm->port_fd + 1, &rfds, NULL, NULL, &tv);
                /*Check if timeout had occured*/
                if (0 == retval) {
                    dbg (WARN, "Timeout Occured, No response or command came from the target !!!");
                    return EFAILED;
                }

                /* Retreive data */
                if (SUCCESS != rx_data((char*)exec_cmds_resp,
                                      m_comm->sahara_cmd_exec_response_rx.resp_length,
                                      m_comm, 0))
                {
                   return EFAILED;
                }

                dbg (EVENT, "RECEIVED <-- RAW_DATA");

                /* Print data */
                for (print_index = 0;
                     print_index < m_comm->sahara_cmd_exec_response_rx.resp_length;
                     print_index++)
                {
                    dbg (INFO, "Received Byte: 0x%02x", exec_cmds_resp[print_index]);
                }
            }
        }
        else {
            dbg (ERROR, "Received an unknown command: %d ", m_comm->sahara_rx_data_payload.command);
            return EFAILED;
        }
    }

    /* Send command switch mode */
    txCmdSwitchModePacket.command = SAHARA_CMD_SWITCH_MODE_ID;
    txCmdSwitchModePacket.length = sizeof(struct sahara_packet_cmd_switch_mode);
    txCmdSwitchModePacket.mode = m_comm->prev_mode;

    dbg (EVENT, "SENDING --> SAHARA_CMD_SWITCH_MODE");

    /*Send the packet data*/
    if (SUCCESS != tx_data ((char *)&txCmdSwitchModePacket, txCmdSwitchModePacket.length, m_comm)) {
        dbg (ERROR, "Sending CMD_SWITCH_MODE packet failed");
        return EFAILED;
    }

    return SUCCESS;
}

void time_throughput_calculate(struct timeval time1, struct timeval time2, unsigned int size_bytes)
{
    if (size_bytes == 0) {
        dbg(INFO, "Cannot calculate throughput, size is 0");
        return;
    }
    long total_ms;
    double throughput;
    long long t1, t2, dif;
    int bits = size_bytes * BITS_IN_BYTE;

    t1 = (time1.tv_sec*NANO_SEC_PER_SEC)+(time1.tv_usec*NANO_SEC_PER_USEC);
    t2 = (time2.tv_sec*NANO_SEC_PER_SEC)+(time2.tv_usec*NANO_SEC_PER_USEC);      
    dif = t2-t1;
    /* dbg(INFO, "total SECONDS = %d\n", (int)(time2.tv_sec-time1.tv_sec)); */

    total_ms = (long)(dif / NANO_SEC_PER_MSEC);
    /* dbg(INFO, "total_ms = %ld\n", total_ms); */

    throughput = ((double)bits * MILI_IN_SEC) / total_ms;
    dbg(INFO, "Throughput = %f\n", throughput);
}
