/*===========================================================================

  Copyright (c) 2010-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

  Export of this technology or software is regulated by the U.S. Government.
  Diversion contrary to U.S. law prohibited.

  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  Qualcomm Technologies, Inc. and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of Qualcomm Technologies, Inc.

===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header: //depot/asic/sandbox/users/micheleb/ril/qcril_uim_file.c#6 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
10/08/13   vdc     Return FALSE when pointer is NULL while composing APDU data
08/29/13   yt      Allow P3 value to be more than 255 for READ/WRITE requests
08/12/13   at      Added support for Long APDU indication
05/02/13   at      Update P3 sanity check for qcril_uim_request_send_apdu
04/17/13   yt      Fix critical KW errors
01/29/13   yt      Support for third SIM card slot
01/14/13   yt      Fix critical KW errors
12/19/12   at      Move to newer qcril_event_queue API
12/05/12   at      Fix for status byte in get file attributes response
11/27/12   at      Move an older QMI_UIM API to newer QCCI based one
10/25/12   at      Explicit check for P1, P2 & P3 in RIL_SIM_IO_v6 request
10/08/12   at      Support for ISIM Authentication API
08/21/12   at      Fix for sending the correct slot in open logical channel
07/26/12   at      Request FCP template while opening channel for ICC cards
06/29/12   at      Error code mapping for open logical channel response
05/10/12   at      Support for fetching IMSI_M from RUIM App
04/10/12   at      Updated session type and request id in PIN2 cmd callback
03/22/12   at      Replacing malloc()/free() with qcril_malloc()/qcril_free()
03/20/12   tl      Transition to QCCI
02/15/12   at      Support for fetching IMSI_M in RIL_REQUEST_GET_IMSI
11/30/11   at      Send CM_UPDATE_FDN_STATUS based on correct RILD instance;
                   Made get fdn status asynchrounous
08/19/11   yt      Fixed Klocwork errors
07/08/11   at      Converting AID to binary in logical_channel request
05/18/11   yt      Update QCRIL_CM with FDN status at power up
03/30/11   at      Support for logical channel & send apdu commands
03/22/11   at      Support for ril.h v6
03/02/11   at      Fixed response parameter if PIN2 verification failed
01/18/11   at      Removed slot id parameter from all requests
11/12/10   at      Added support for UIM queue implementation
11/08/10   at      Fix for sending proper error codes for PIN2 verification
11/02/10   at      Sending prov session in SIM_IO PIN2 check for 2G cards
10/06/10   at      Support for handling instance_id passed in requests
09/09/10   at      Changed the way sessions are fetched, added handling for
                   opening & closing non-prov session on demand
08/25/10   at      Added featurization for async SIM_IO calls and check for
                   sw1 in case of read record, write binary & write record
08/03/10   at      APIs using aidPtr to be handled properly
07/13/10   at      Added support for set and get FDN status
05/13/10   at      Fixed compile errors & clean up for merging with mainline
04/13/10   mib     Initial version
===========================================================================*/

/*===========================================================================

                           INCLUDE FILES

===========================================================================*/

#if defined (FEATURE_QCRIL_UIM_QMI)

#include "ril.h"
#include "IxErrno.h"
#include "qcrili.h"
#include "qcril_arb.h"
#include "qcril_log.h"
#include "qcril_reqlist.h"
#include "qcril_uim.h"
#include "qcril_uim_util.h"
#include "qcril_uim_file.h"
#include "qcril_uim_queue.h"
#include "qcril_uim_qcci.h"
#include <string.h>


/*===========================================================================

                           DEFINES

===========================================================================*/
/* SIM IO commands */
#define SIM_CMD_READ_BINARY                 176
#define SIM_CMD_READ_RECORD                 178
#define SIM_CMD_GET_RESPONSE                192
#define SIM_CMD_UPDATE_BINARY               214
#define SIM_CMD_RETRIEVE_DATA               203
#define SIM_CMD_SET_DATA                    219
#define SIM_CMD_UPDATE_RECORD               220
#define SIM_CMD_STATUS                      242

#define QCRIL_UIM_IMSI_PATH_SIZE            4
#define QCRIL_UIM_FILEID_EF_IMSI            0x6F07
#define QCRIL_UIM_FILEID_EF_IMSI_M          0x6F22
#define QCRIL_UIM_IMSI_M_RAW_SIZE           10
#define QCRIL_UIM_IMSI_M_PARSED_SIZE        16

#define QCRIL_UIM_APDU_MIN_SIZE             4
#define QCRIL_UIM_APDU_MIN_SIZE_PLUS_P3     5

/* Get file attributes response defines */
/* Default value: not invalidated, readable and updatable when invalidated */
#define QCRIL_UIM_GET_RESPONSE_MIN_SIZE     14
#define QCRIL_UIM_GET_RESPONSE_FILE_STATUS  0X05
#define QCRIL_UIM_TAG_FCP_TEMPLATE          0x62
#define QCRIL_UIM_TAG_LIFE_CYCLE_STATUS     0x8A


/*===========================================================================

                               INTERNAL FUNCTIONS

===========================================================================*/

/*=========================================================================

  FUNCTION:  qcril_uim_clone_read_transparent_request

===========================================================================*/
/*!
    @brief
    Allocates the memory and clones a read transparent request.
    This function is used when PIN2 needs to be verified before
    executing the read transparent. The function allocates a single
    block of memory to execute the complete copy.

    @return
    Buffer with cloned request
*/
/*=========================================================================*/
static qcril_uim_pin2_original_request_type* qcril_uim_clone_read_transparent_request
(
  qcril_instance_id_e_type                     instance_id,
  qcril_modem_id_e_type                        modem_id,
  RIL_Token                                    token,
  const qmi_uim_read_transparent_params_type * read_params_ptr
)
{
  uint32                                 tot_size  = 0;
  uint16                                 req_size  = 0;
  uint16                                 aid_size  = 0;
  uint16                                 path_size = 0;
  qcril_uim_pin2_original_request_type * ret_ptr   = NULL;

  if(read_params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL read_params_ptr");
    QCRIL_ASSERT(0);
    return NULL;
  }

  /* Calculate the size of each block, aligning to word */
  req_size = qcril_uim_align_size(sizeof(qcril_uim_pin2_original_request_type));
  if (read_params_ptr->session_info.aid.data_ptr != NULL)
  {
    aid_size = qcril_uim_align_size(read_params_ptr->session_info.aid.data_len);
  }
  if (read_params_ptr->file_id.path.data_ptr != NULL)
  {
    path_size = qcril_uim_align_size(read_params_ptr->file_id.path.data_len);
  }
  tot_size = req_size + aid_size + path_size;

  /* Allocate memory */
  ret_ptr = (qcril_uim_pin2_original_request_type*)qcril_malloc(tot_size);

  if (ret_ptr == NULL)
  {
    return NULL;
  }

  memset(ret_ptr, 0, tot_size);

  /* Generic fields */
  ret_ptr->size         = tot_size;
  ret_ptr->cmd          = QCRIL_UIM_ORIG_SIM_IO_READ_BINARY;
  ret_ptr->token        = token;
  ret_ptr->instance_id  = instance_id;
  ret_ptr->modem_id     = modem_id;

  /* Session */
  ret_ptr->data.read_transparent.session_info.session_type = read_params_ptr->session_info.session_type;
  ret_ptr->data.read_transparent.session_info.aid.data_len = read_params_ptr->session_info.aid.data_len;
  if (read_params_ptr->session_info.aid.data_len > 0 &&
      read_params_ptr->session_info.aid.data_ptr != NULL)
  {
    ret_ptr->data.read_transparent.session_info.aid.data_ptr = (uint8*)ret_ptr + req_size;
    memcpy(ret_ptr->data.read_transparent.session_info.aid.data_ptr,
           read_params_ptr->session_info.aid.data_ptr,
           read_params_ptr->session_info.aid.data_len);
  }

  /* Path */
  ret_ptr->data.read_transparent.file_id.file_id = read_params_ptr->file_id.file_id;
  ret_ptr->data.read_transparent.file_id.path.data_len = read_params_ptr->file_id.path.data_len;
  if (read_params_ptr->file_id.path.data_len > 0 &&
      read_params_ptr->file_id.path.data_ptr != NULL)
  {
    ret_ptr->data.read_transparent.file_id.path.data_ptr = (uint8*)ret_ptr + req_size + aid_size;
    memcpy(ret_ptr->data.read_transparent.file_id.path.data_ptr,
           read_params_ptr->file_id.path.data_ptr,
           read_params_ptr->file_id.path.data_len);
  }

  /* Other read transparent parameters */
  ret_ptr->data.read_transparent.offset = read_params_ptr->offset;
  ret_ptr->data.read_transparent.length = read_params_ptr->length;

  return ret_ptr;
} /* qcril_uim_clone_read_transparent_request */


/*=========================================================================

  FUNCTION:  qcril_uim_clone_read_record_request

===========================================================================*/
/*!
    @brief
    Allocates the memory and clones a read record request.
    This function is used when PIN2 needs to be verified before
    executing the read record. The function allocates a single
    block of memory to execute the complete copy.

    @return
    Buffer with cloned request
*/
/*=========================================================================*/
static qcril_uim_pin2_original_request_type* qcril_uim_clone_read_record_request
(
  qcril_instance_id_e_type                instance_id,
  qcril_modem_id_e_type                   modem_id,
  RIL_Token                               token,
  const qmi_uim_read_record_params_type * read_params_ptr
)
{
  uint32                                 tot_size  = 0;
  uint16                                 req_size  = 0;
  uint16                                 aid_size  = 0;
  uint16                                 path_size = 0;
  qcril_uim_pin2_original_request_type * ret_ptr   = NULL;

  if(read_params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL read_params_ptr");
    QCRIL_ASSERT(0);
    return NULL;
  }

  /* Calculate the size of each block, aligning to word */
  req_size = qcril_uim_align_size(sizeof(qcril_uim_pin2_original_request_type));
  if (read_params_ptr->session_info.aid.data_ptr != NULL)
  {
    aid_size = qcril_uim_align_size(read_params_ptr->session_info.aid.data_len);
  }
  if (read_params_ptr->file_id.path.data_ptr != NULL)
  {
    path_size = qcril_uim_align_size(read_params_ptr->file_id.path.data_len);
  }
  tot_size = req_size + aid_size + path_size;

  /* Allocate memory */
  ret_ptr = (qcril_uim_pin2_original_request_type*)qcril_malloc(tot_size);

  if (ret_ptr == NULL)
  {
    return NULL;
  }

  memset(ret_ptr, 0, tot_size);

  /* Generic fields */
  ret_ptr->size         = tot_size;
  ret_ptr->cmd          = QCRIL_UIM_ORIG_SIM_IO_READ_RECORD;
  ret_ptr->token        = token;
  ret_ptr->instance_id  = instance_id;
  ret_ptr->modem_id     = modem_id;

  /* Session */
  ret_ptr->data.read_record.session_info.session_type = read_params_ptr->session_info.session_type;
  ret_ptr->data.read_record.session_info.aid.data_len = read_params_ptr->session_info.aid.data_len;
  if (read_params_ptr->session_info.aid.data_len > 0 &&
      read_params_ptr->session_info.aid.data_ptr != NULL)
  {
    ret_ptr->data.read_record.session_info.aid.data_ptr = (uint8*)ret_ptr + req_size;
    memcpy(ret_ptr->data.read_record.session_info.aid.data_ptr,
           read_params_ptr->session_info.aid.data_ptr,
           read_params_ptr->session_info.aid.data_len);
  }

  /* Path */
  ret_ptr->data.read_record.file_id.file_id = read_params_ptr->file_id.file_id;
  ret_ptr->data.read_record.file_id.path.data_len = read_params_ptr->file_id.path.data_len;
  if (read_params_ptr->file_id.path.data_len > 0 &&
      read_params_ptr->file_id.path.data_ptr != NULL)
  {
    ret_ptr->data.read_record.file_id.path.data_ptr = (uint8*)ret_ptr + req_size + aid_size;
    memcpy(ret_ptr->data.read_record.file_id.path.data_ptr,
           read_params_ptr->file_id.path.data_ptr,
           read_params_ptr->file_id.path.data_len);
  }

  /* Other read transparent parameters */
  ret_ptr->data.read_record.record = read_params_ptr->record;
  ret_ptr->data.read_record.length = read_params_ptr->length;

  return ret_ptr;
} /* qcril_uim_clone_read_record_request */


/*=========================================================================

  FUNCTION:  qcril_uim_clone_write_transparent_request

===========================================================================*/
/*!
    @brief
    Allocates the memory and clones a write transparent request.
    This function is used when PIN2 needs to be verified before
    executing the write transparent. The function allocates a single
    block of memory to execute the complete copy.

    @return
    Buffer with cloned request
*/
/*=========================================================================*/
static qcril_uim_pin2_original_request_type* qcril_uim_clone_write_transparent_request
(
  qcril_instance_id_e_type                      instance_id,
  qcril_modem_id_e_type                         modem_id,
  RIL_Token                                     token,
  const qmi_uim_write_transparent_params_type * write_params_ptr
)
{
  uint32                                 tot_size  = 0;
  uint16                                 req_size  = 0;
  uint16                                 aid_size  = 0;
  uint16                                 path_size = 0;
  uint16                                 data_size = 0;
  qcril_uim_pin2_original_request_type * ret_ptr   = NULL;

  if(write_params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL write_params_ptr");
    QCRIL_ASSERT(0);
    return NULL;
  }

  /* Calculate the size of each block, aligning to word */
  req_size = qcril_uim_align_size(sizeof(qcril_uim_pin2_original_request_type));
  if (write_params_ptr->session_info.aid.data_ptr != NULL)
  {
    aid_size = qcril_uim_align_size(write_params_ptr->session_info.aid.data_len);
  }
  if (write_params_ptr->file_id.path.data_ptr != NULL)
  {
    path_size = qcril_uim_align_size(write_params_ptr->file_id.path.data_len);
  }
  if (write_params_ptr->data.data_ptr != NULL)
  {
    data_size = qcril_uim_align_size(write_params_ptr->data.data_len);
  }
  tot_size = req_size + aid_size + path_size + data_size;

  /* Allocate memory */
  ret_ptr = (qcril_uim_pin2_original_request_type*)qcril_malloc(tot_size);

  if (ret_ptr == NULL)
  {
    return NULL;
  }

  memset(ret_ptr, 0, tot_size);

  /* Generic fields */
  ret_ptr->size         = tot_size;
  ret_ptr->cmd          = QCRIL_UIM_ORIG_SIM_IO_UPDATE_BINARY;
  ret_ptr->token        = token;
  ret_ptr->instance_id  = instance_id;
  ret_ptr->modem_id     = modem_id;

  /* Session */
  ret_ptr->data.write_transparent.session_info.session_type = write_params_ptr->session_info.session_type;
  ret_ptr->data.write_transparent.session_info.aid.data_len = write_params_ptr->session_info.aid.data_len;
  if (write_params_ptr->session_info.aid.data_len > 0 &&
      write_params_ptr->session_info.aid.data_ptr != NULL)
  {
    ret_ptr->data.write_transparent.session_info.aid.data_ptr = (uint8*)ret_ptr + req_size;
    memcpy(ret_ptr->data.write_transparent.session_info.aid.data_ptr,
           write_params_ptr->session_info.aid.data_ptr,
           write_params_ptr->session_info.aid.data_len);
  }

  /* Path */
  ret_ptr->data.write_transparent.file_id.file_id = write_params_ptr->file_id.file_id;
  ret_ptr->data.write_transparent.file_id.path.data_len = write_params_ptr->file_id.path.data_len;
  if (write_params_ptr->file_id.path.data_len > 0 &&
      write_params_ptr->file_id.path.data_ptr != NULL)
  {
    ret_ptr->data.write_transparent.file_id.path.data_ptr = (uint8*)ret_ptr + req_size + aid_size;
    memcpy(ret_ptr->data.write_transparent.file_id.path.data_ptr,
           write_params_ptr->file_id.path.data_ptr,
           write_params_ptr->file_id.path.data_len);
  }

  /* Data */
  ret_ptr->data.write_record.data.data_len = write_params_ptr->data.data_len;
  if (write_params_ptr->data.data_len > 0 &&
      write_params_ptr->data.data_ptr != NULL)
  {
    ret_ptr->data.write_transparent.data.data_ptr = (uint8*)ret_ptr + req_size + aid_size + path_size;
    memcpy(ret_ptr->data.write_transparent.data.data_ptr,
           write_params_ptr->data.data_ptr,
           write_params_ptr->data.data_len);
  }

  /* Other write transparent parameters */
  ret_ptr->data.write_transparent.offset = write_params_ptr->offset;

  return ret_ptr;
} /* qcril_uim_clone_write_transparent_request */


/*=========================================================================

  FUNCTION:  qcril_uim_clone_write_record_request

===========================================================================*/
/*!
    @brief
    Allocates the memory and clones a write record request.
    This function is used when PIN2 needs to be verified before
    executing the write record. The function allocates a single
    block of memory to execute the complete copy.

    @return
    Buffer with cloned request
*/
/*=========================================================================*/
static qcril_uim_pin2_original_request_type* qcril_uim_clone_write_record_request
(
  qcril_instance_id_e_type                 instance_id,
  qcril_modem_id_e_type                    modem_id,
  RIL_Token                                token,
  const qmi_uim_write_record_params_type * write_params_ptr
)
{
  uint32                                 tot_size  = 0;
  uint16                                 req_size  = 0;
  uint16                                 aid_size  = 0;
  uint16                                 path_size = 0;
  uint16                                 data_size = 0;
  qcril_uim_pin2_original_request_type * ret_ptr   = NULL;

  if(write_params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL write_params_ptr");
    QCRIL_ASSERT(0);
    return NULL;
  }

  /* Calculate the size of each block, aligning to word */
  req_size = qcril_uim_align_size(sizeof(qcril_uim_pin2_original_request_type));
  if (write_params_ptr->session_info.aid.data_ptr != NULL)
  {
    aid_size = qcril_uim_align_size(write_params_ptr->session_info.aid.data_len);
  }
  if (write_params_ptr->file_id.path.data_ptr != NULL)
  {
    path_size = qcril_uim_align_size(write_params_ptr->file_id.path.data_len);
  }
  if (write_params_ptr->data.data_ptr != NULL)
  {
    data_size = qcril_uim_align_size(write_params_ptr->data.data_len);
  }
  tot_size = req_size + aid_size + path_size + data_size;

  /* Allocate memory */
  ret_ptr = (qcril_uim_pin2_original_request_type*)qcril_malloc(tot_size);

  if (ret_ptr == NULL)
  {
    return NULL;
  }

  memset(ret_ptr, 0, tot_size);

  /* Generic fields */
  ret_ptr->size         = tot_size;
  ret_ptr->cmd          = QCRIL_UIM_ORIG_SIM_IO_UPDATE_RECORD;
  ret_ptr->token        = token;
  ret_ptr->instance_id  = instance_id;
  ret_ptr->modem_id     = modem_id;

  /* Session */
  ret_ptr->data.write_record.session_info.session_type = write_params_ptr->session_info.session_type;
  ret_ptr->data.write_record.session_info.aid.data_len = write_params_ptr->session_info.aid.data_len;
  if (write_params_ptr->session_info.aid.data_len > 0 &&
      write_params_ptr->session_info.aid.data_ptr != NULL)
  {
    ret_ptr->data.write_record.session_info.aid.data_ptr = (uint8*)ret_ptr + req_size;
    memcpy(ret_ptr->data.write_record.session_info.aid.data_ptr,
           write_params_ptr->session_info.aid.data_ptr,
           write_params_ptr->session_info.aid.data_len);
  }

  /* Path */
  ret_ptr->data.write_record.file_id.file_id = write_params_ptr->file_id.file_id;
  ret_ptr->data.write_record.file_id.path.data_len = write_params_ptr->file_id.path.data_len;
  if (write_params_ptr->file_id.path.data_len > 0 &&
      write_params_ptr->file_id.path.data_ptr != NULL)
  {
    ret_ptr->data.write_record.file_id.path.data_ptr = (uint8*)ret_ptr + req_size + aid_size;
    memcpy(ret_ptr->data.write_record.file_id.path.data_ptr,
           write_params_ptr->file_id.path.data_ptr,
           write_params_ptr->file_id.path.data_len);
  }

  /* Data */
  ret_ptr->data.write_record.data.data_len = write_params_ptr->data.data_len;
  if (write_params_ptr->data.data_len > 0 &&
      write_params_ptr->data.data_ptr != NULL)
  {
    ret_ptr->data.write_record.data.data_ptr = (uint8*)ret_ptr + req_size + aid_size + path_size;
    memcpy(ret_ptr->data.write_record.data.data_ptr,
           write_params_ptr->data.data_ptr,
           write_params_ptr->data.data_len);
  }

  /* Other write record parameters */
  ret_ptr->data.write_record.record = write_params_ptr->record;

  return ret_ptr;
} /* qcril_uim_clone_write_record_request */


/*=========================================================================

  FUNCTION:  qcril_uim_clone_set_fdn_status_request

===========================================================================*/
/*!
    @brief
    Allocates the memory and clones a fdn status request.
    This function is used when PIN2 needs to be verified before
    executing the write record. The function allocates a single
    block of memory to execute the complete copy.

    @return
    Buffer with cloned request
*/
/*=========================================================================*/
static qcril_uim_pin2_original_request_type* qcril_uim_clone_set_fdn_status_request
(
  qcril_instance_id_e_type                       instance_id,
  qcril_modem_id_e_type                          modem_id,
  RIL_Token                                      token,
  const qmi_uim_set_service_status_params_type * service_status_params_ptr
)
{
  uint32                                 tot_size  = 0;
  uint16                                 req_size  = 0;
  uint16                                 aid_size  = 0;
  qcril_uim_pin2_original_request_type * ret_ptr   = NULL;

  if(service_status_params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL service_status_params_ptr");
    QCRIL_ASSERT(0);
    return NULL;
  }

  /* Calculate the size of each block, aligning to word */
  req_size = qcril_uim_align_size(sizeof(qcril_uim_pin2_original_request_type));
  if (service_status_params_ptr->session_info.aid.data_ptr != NULL)
  {
    aid_size = qcril_uim_align_size(service_status_params_ptr->session_info.aid.data_len);
  }

  tot_size = req_size + aid_size;

  /* Allocate memory */
  ret_ptr = (qcril_uim_pin2_original_request_type*)qcril_malloc(tot_size);

  if (ret_ptr == NULL)
  {
    return NULL;
  }

  memset(ret_ptr, 0, tot_size);

  /* Generic fields */
  ret_ptr->size         = tot_size;
  ret_ptr->cmd          = QCRIL_UIM_ORIG_SET_SERVICE_STATUS_FDN;
  ret_ptr->token        = token;
  ret_ptr->instance_id  = instance_id;
  ret_ptr->modem_id     = modem_id;

  /* Session */
  ret_ptr->data.set_service_status.session_info.session_type = service_status_params_ptr->session_info.session_type;
  ret_ptr->data.set_service_status.session_info.aid.data_len = service_status_params_ptr->session_info.aid.data_len;
  if (service_status_params_ptr->session_info.aid.data_len > 0 &&
      service_status_params_ptr->session_info.aid.data_ptr != NULL)
  {
    ret_ptr->data.set_service_status.session_info.aid.data_ptr = (uint8*)ret_ptr + req_size;
    memcpy(ret_ptr->data.set_service_status.session_info.aid.data_ptr,
           service_status_params_ptr->session_info.aid.data_ptr,
           service_status_params_ptr->session_info.aid.data_len);
  }

  /* Service status */
  ret_ptr->data.set_service_status.fdn_status = service_status_params_ptr->fdn_status;

  return ret_ptr;
} /* qcril_uim_clone_set_fdn_status_request */


/*=========================================================================

  FUNCTION:  qmi_uim_internal_pin2_callback

===========================================================================*/
/*!
    @brief
    Callback for the PIN2 verification before executing another
    function that accesses the files on the card (read or write).
    This function posts an event, so it can be processed in the
    QCRIL context.

    @return
    None
*/
/*=========================================================================*/
static void qmi_uim_internal_pin2_callback
(
  int                            user_handle,
  qmi_service_id_type            service_id,
  qmi_uim_rsp_data_type        * rsp_data,
  void                         * user_data
)
{
  qcril_uim_pin2_original_request_type * original_request_ptr = NULL;
  IxErrnoType                            result               = E_FAILURE;

  (void)user_handle;
  (void)service_id;

  if(rsp_data == NULL || user_data == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL pointer");
    QCRIL_ASSERT(0);
    return;
  }

  /* Retrieve the original request */
  original_request_ptr = (qcril_uim_pin2_original_request_type*)user_data;
  if(original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL pointer");
    QCRIL_ASSERT(0);
    return;
  }
  if(original_request_ptr->size == 0)
  {
    QCRIL_LOG_ERROR("%s", "original_request_ptr->size is 0");
    qcril_free(original_request_ptr);
    original_request_ptr = NULL;
    QCRIL_ASSERT(0);
    return;
  }

  /* Verify that the response is for a PIN verification. This should
     always happen as we use this callback only for this purpose */
  if(rsp_data->rsp_id != QMI_UIM_SRVC_VERIFY_PIN_RSP_MSG)
  {
    QCRIL_LOG_ERROR("invalid rsp_id 0x%x", rsp_data->rsp_id);
    qcril_free(original_request_ptr);
    original_request_ptr = NULL;
    QCRIL_ASSERT(0);
    return;
  }

  /* Store the PIN2 result in the same structure with the original
     request and send it to QCRIL context to be processed */
  original_request_ptr->pin2_result      = rsp_data->qmi_err_code;
  original_request_ptr->pin2_num_retries = rsp_data->rsp_data.verify_pin_rsp.num_retries;

  QCRIL_LOG_INFO( "%s qcril_event_queue\n", __FUNCTION__);
  result = qcril_event_queue( original_request_ptr->instance_id,
                              original_request_ptr->modem_id,
                              QCRIL_DATA_NOT_ON_STACK,
                              QCRIL_EVT_INTERNAL_UIM_VERIFY_PIN_COMMAND_CALLBACK,
                              (void *)original_request_ptr,
                              original_request_ptr->size,
                              original_request_ptr->token);
  if (result != E_SUCCESS)
  {
    QCRIL_LOG_ERROR( " qcril_event_queue failed, result: 0x%x\n", result);
    /* Free allocated memory in case event queueing fails */
    qcril_free(original_request_ptr);
    original_request_ptr = NULL;
  }
} /* qmi_uim_internal_pin2_callback */


/*=========================================================================

  FUNCTION:  qcril_uim_get_file_status_byte

===========================================================================*/
/*!
    @brief
    This function parses the raw response data for get file attibutes to
    check for the Life cycle status integer. This byte is mapped to the
    file status byte coding defined in 51.011, Section 9.2.1

    @return
    Mapped File status byte
*/
/*=========================================================================*/
static uint8 qcril_uim_get_file_status_byte
(
  uint16                         data_len,
  const uint8                  * data_ptr
)
{
  uint16 data_index = 0;
  uint8  file_status_byte = QCRIL_UIM_GET_RESPONSE_FILE_STATUS;

  if ((data_ptr == NULL) || (data_len == 0))
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, returning default value 0x05");
    return file_status_byte;
  }

  /* Parse through the raw response to find the TLV */
  if (data_ptr[data_index] != QCRIL_UIM_TAG_FCP_TEMPLATE)
  {
    QCRIL_LOG_ERROR("Invalid FCP template tag: 0x%X", data_ptr[data_index]);
    return file_status_byte;
  }

  /* Byte 2 & 3 has the total length of FCP Template
     We need to udpate the data_index appropriately  */
  if (data_len >= 2 && data_ptr[1] <= 0x7F)
  {
    if (data_len < (data_ptr[1] + 2))
    {
      return file_status_byte;
    }
    data_len    = data_ptr[1] + 2;
    data_index += 2;
  }
  else if (data_len >= 3 && data_ptr[1] == 0x81)
  {
    if (data_len < (data_ptr[2] + 3))
    {
      return file_status_byte;
    }
    data_len   = data_ptr[2] + 3;
    data_index += 3;
  }
  else
  {
    return file_status_byte;
  }

  /* Parse through the raw response to find the TLV */
  while ((data_index+1) < data_len)
  {
    uint8 tag_len   = 0;
    uint8 read_byte = 0;

    switch (data_ptr[data_index])
    {
      case QCRIL_UIM_TAG_LIFE_CYCLE_STATUS:
        /* Sanity check the length */
        tag_len = data_ptr[data_index+1];
        if (((data_index + 2 + tag_len) > data_len) ||
             (tag_len != 1))
        {
          /* Incorrect data length, cannot parse further */
          QCRIL_LOG_ERROR("Incorrect tag length, cannot parse further: 0x%X \n",
                          tag_len);
          return file_status_byte;
        }
        read_byte = data_ptr[data_index+2];
        QCRIL_LOG_INFO( "Life cycle status integer byte: 0x%X", read_byte);
        /* Check for bit b1 and these condition:
           1. b3 should be set for operational state
           2. b4 - b8 should be 0s */
        if ((read_byte <= 0x07) && (read_byte >= 0x04))
        {
          if (read_byte & 0x01)
          {
            /* Not Invalidated, readable/updatable when invalidated */
            file_status_byte = 0x05;
          }
          else
          {
            /* Invalidated, not readable/updatable when invalidated */
            file_status_byte = 0x00;
          }
        }
        /* Nothing else to do, return */
        return file_status_byte;

      default:
        /* Tag that we are not interested in, move to next one if avaliable */
        tag_len = data_ptr[data_index+1];
        data_index += tag_len + 2;
        break;
    }
  }

  return file_status_byte;
} /* qcril_uim_get_file_status_byte */


#ifdef FEATURE_QCRIL_UIM_QMI_APDU_ACCESS
/*=========================================================================

  FUNCTION:  qcril_uim_compose_apdu_data

===========================================================================*/
/*!
    @brief
    Function to compose raw APDU command. Composed data pointer and length
    are updated based on the request.

    @return
    TRUE if successful, FALSE otherwise.
*/
/*=========================================================================*/
static boolean qcril_uim_compose_apdu_data
(
  qmi_uim_data_type       * apdu_data_ptr,
  const RIL_SIM_IO_v6     * request_io_ptr
)
{
  qmi_uim_data_type     binary_apdu_data;
  uint16                total_apdu_len = 0;

  if ((request_io_ptr == NULL) ||
      (apdu_data_ptr == NULL) ||
      (apdu_data_ptr->data_ptr == NULL) ||
      (apdu_data_ptr->data_len == 0))
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, cannot proceed");
    QCRIL_ASSERT(0);
    return FALSE;
  }

  memset(apdu_data_ptr->data_ptr, 0, apdu_data_ptr->data_len);
  memset(&binary_apdu_data, 0, sizeof(qmi_uim_data_type));

  total_apdu_len = apdu_data_ptr->data_len;

  /* Update mandatory parameters - CLA, INS, P1 & P2 */
  if (total_apdu_len >= QCRIL_UIM_APDU_MIN_SIZE)
  {
    apdu_data_ptr->data_ptr[0] = (uint8)(request_io_ptr->cla & 0xFF);
    apdu_data_ptr->data_ptr[1] = (uint8)(request_io_ptr->command & 0xFF);
    apdu_data_ptr->data_ptr[2] = (uint8)(request_io_ptr->p1 & 0xFF);
    apdu_data_ptr->data_ptr[3] = (uint8)(request_io_ptr->p2 & 0xFF);
    apdu_data_ptr->data_len = QCRIL_UIM_APDU_MIN_SIZE;
  }

  /* Update P3 parameter if valid */
  if (total_apdu_len >= QCRIL_UIM_APDU_MIN_SIZE_PLUS_P3)
  {
    apdu_data_ptr->data_ptr[4] = (uint8)(request_io_ptr->p3 & 0xFF);
    apdu_data_ptr->data_len = QCRIL_UIM_APDU_MIN_SIZE_PLUS_P3;
  }

  /* Update data parameter if valid */
  if (total_apdu_len > QCRIL_UIM_APDU_MIN_SIZE_PLUS_P3)
  {
    if ((request_io_ptr->data == NULL) || (strlen(request_io_ptr->data) == 0))
    {
      QCRIL_LOG_ERROR("%s", "Mismatch in total_apdu_len & input APDU data!");
      return FALSE;
    }

    binary_apdu_data.data_ptr = qcril_uim_alloc_hexstring_to_bin(request_io_ptr->data,
                                                                 &binary_apdu_data.data_len);
    if (binary_apdu_data.data_ptr == NULL)
    {
      QCRIL_LOG_ERROR("%s", "Unable to convert input APDU data!");
      return FALSE;
    }

    /* Update data parameter if valid */
    if (binary_apdu_data.data_len <= (total_apdu_len - QCRIL_UIM_APDU_MIN_SIZE_PLUS_P3))
    {
      memcpy(&apdu_data_ptr->data_ptr[5], binary_apdu_data.data_ptr, binary_apdu_data.data_len);
      apdu_data_ptr->data_len = QCRIL_UIM_APDU_MIN_SIZE_PLUS_P3 + binary_apdu_data.data_len;
    }

    /* Free temp buffer */
    qcril_free(binary_apdu_data.data_ptr);
    binary_apdu_data.data_ptr = NULL;
  }

  return TRUE;
} /* qcril_uim_compose_apdu_data */
#endif /* FEATURE_QCRIL_UIM_QMI_APDU_ACCESS */


/*=========================================================================

  FUNCTION:  qcril_uim_read_binary

===========================================================================*/
/*!
    @brief
    Function to read transparent

    @return
    None
*/
/*=========================================================================*/
static void qcril_uim_read_binary
(
  qcril_instance_id_e_type  instance_id,
  RIL_Token                 token,
  const RIL_SIM_IO_v6*      request_io_ptr
)
{
  qcril_modem_id_e_type                 modem_id = QCRIL_MAX_MODEM_ID - 1;
  int                                   res;
  RIL_Errno                             err;
  qmi_uim_read_transparent_params_type  read_params;
  uint8                                 aid[QCRIL_UIM_MAX_AID_SIZE];
  uint8                                 path[QCRIL_UIM_MAX_PATH_SIZE];
  uint16                                first_level_df_path = 0;
  qcril_uim_original_request_type     * callback_request_ptr = NULL;
  uint8                                 slot = QCRIL_UIM_INVALID_SLOT_INDEX_VALUE;
#ifndef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  qcril_uim_callback_params_type        callback_params;
#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

  if(request_io_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, cannot proceed");
    QCRIL_ASSERT(0);
    return;
  }

  memset(&read_params, 0, sizeof(qmi_uim_read_transparent_params_type));

  /* Fetch slot info */
  slot = qcril_uim_instance_id_to_slot(instance_id);
  if(slot >= QMI_UIM_MAX_CARD_COUNT)
  {
    QCRIL_LOG_ERROR("Invalid slot %d", slot);
    QCRIL_ASSERT(0);
    return;
  }

  /* Sanity check */
  if ((request_io_ptr->p1 < 0) || (request_io_ptr->p1 > 0xFF) ||
      (request_io_ptr->p2 < 0) || (request_io_ptr->p2 > 0xFF) ||
      (request_io_ptr->p3 < 0))
  {
    QCRIL_LOG_ERROR( "Unsupported case, P1: 0x%X, P2: 0x%X, P3: 0x%X \n",
                     request_io_ptr->p1, request_io_ptr->p2, request_io_ptr->p3);
    qcril_uim_response(instance_id, token, RIL_E_REQUEST_NOT_SUPPORTED,
                       NULL, 0, TRUE, NULL);
    return;
  }

  /* File id */
  err = qcril_uim_extract_file_id(request_io_ptr,
                                  &read_params.file_id,
                                  path,
                                  sizeof(path));
  if (err != RIL_E_SUCCESS)
  {
    qcril_uim_response(instance_id, token, err, NULL, 0, TRUE,
                       "error in qcril_uim_extract_file_id");
    return;
  }

  /* Extract the first level DF */
  if ((read_params.file_id.path.data_len >= 4) && (read_params.file_id.path.data_ptr))
  {
    /* Interested only in 3rd & 4th bytes */
    first_level_df_path = (*(read_params.file_id.path.data_ptr + 2) << 8) |
                          (*(read_params.file_id.path.data_ptr + 3));
  }

  /* Session information */
  err = qcril_uim_extract_session_type(slot,
                                       request_io_ptr->aidPtr,
                                       first_level_df_path,
                                       &read_params.session_info,
                                       aid,
                                       sizeof(aid));
  if (err != RIL_E_SUCCESS)
  {
    qcril_uim_response(instance_id, token, err, NULL, 0, TRUE,
                       "error in qcril_uim_extract_session_type");
    return;
  }

  QCRIL_LOG_INFO( "Session type found: %d", read_params.session_info.session_type);

  /* Keep track of non-prov session, will be removed in response handling */
  if ((read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_1) ||
      (read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_2) ||
      (read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_3))
  {
    err = qcril_uim_add_non_provisioning_session(&read_params.session_info,
                                                 token);
    if (err != RIL_E_SUCCESS)
    {
      QCRIL_LOG_ERROR("%s", "Error in adding non prov session!");
    }
  }

  /* Read parameters: length and offset */
  read_params.offset = (request_io_ptr->p1) << 8 | (request_io_ptr->p2);
  read_params.length = request_io_ptr->p3;

  if (request_io_ptr->pin2 != NULL)
  {
    qmi_uim_verify_pin_params_type         verify_pin_params;
    qcril_uim_pin2_original_request_type * original_request_ptr = NULL;

    memset(&verify_pin_params, 0, sizeof(qmi_uim_verify_pin_params_type));

    /* Session information: same as for read, except card sessions */
    verify_pin_params.session_info = read_params.session_info;

    /* We try to fake a GSM/CDMA first level DF path only for PIN2 verification */
    if ((read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
        (read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
        (read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
    {
      err = qcril_uim_extract_session_type(slot,
                                           request_io_ptr->aidPtr,
                                           QCRIL_UIM_FILEID_DF_CDMA,
                                           &verify_pin_params.session_info,
                                           NULL,
                                           0);
      if ((err != RIL_E_SUCCESS) ||
          (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
          (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
          (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
      {
        err = qcril_uim_extract_session_type(slot,
                                             request_io_ptr->aidPtr,
                                             QCRIL_UIM_FILEID_DF_GSM,
                                             &verify_pin_params.session_info,
                                             NULL,
                                             0);
        if ((err != RIL_E_SUCCESS) ||
            (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
            (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
            (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
        {
          goto send_read_binary_error;
        }
      }
      QCRIL_LOG_INFO( "Provisioning session type found: %d",
                      verify_pin_params.session_info.session_type);
      /* Use this session type until the modem resolves card session issue */
      read_params.session_info = verify_pin_params.session_info;
    }

    /* PIN id: PIN2 */
    verify_pin_params.pin_id = QMI_UIM_PIN_ID_PIN2;

    /* PIN value */
    verify_pin_params.pin_data.data_len = (uint16)(strlen(request_io_ptr->pin2));
    verify_pin_params.pin_data.data_ptr = (uint8*)request_io_ptr->pin2;

    /* Allocate original request */
    original_request_ptr = qcril_uim_clone_read_transparent_request(instance_id,
                                                                    modem_id,
                                                                    token,
                                                                    &read_params);

    if (original_request_ptr == NULL)
    {
      QCRIL_LOG_ERROR("%s", "error allocating memory for clone_read_transparent_request!");
      goto send_read_binary_error;
    }

    QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "verify pin" );
    res = qcril_qmi_uim_verify_pin(qcril_uim.qmi_handle,
                                   &verify_pin_params,
                                   qmi_uim_internal_pin2_callback,
                                   (void*)original_request_ptr,
                                   NULL);

    if (res < 0)
    {
      qcril_free(original_request_ptr);
      QCRIL_LOG_ERROR("%s", "error in qmi_uim_verify_pin!");
      goto send_read_binary_error;
    }

    /* original_request_ptr is freed when the PIN2 callback is received */
    return;
  }

  /* Allocate original request, it is freed in qmi_uim_callback */
  callback_request_ptr = qcril_uim_allocate_orig_request(instance_id,
                                                         modem_id,
                                                         token,
                                                         request_io_ptr->command,
                                                         read_params.session_info.session_type);
  if (callback_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "error allocating memory for callback_request_ptr!");
    goto send_read_binary_error;
  }

  /* No pin2 was provided: proceed with read transparent */
  QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "read transparent" );
#ifdef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  if (qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_READ_TRANSPARENT,
                                   qcril_uim.qmi_handle,
                                   &read_params,
                                   qmi_uim_callback,
                                   (void*)callback_request_ptr) >= 0)
  {
    return;
  }
#else
  memset(&callback_params, 0, sizeof(qcril_uim_callback_params_type));

  if (qcril_qmi_uim_read_transparent(qcril_uim.qmi_handle,
                                     &read_params,
                                     NULL,
                                     (void*)callback_request_ptr,
                                     &callback_params.qmi_rsp_data) >= 0)
  {
    callback_params.orig_req_data = callback_request_ptr;
    qcril_uim_read_binary_resp(&callback_params);

    /* Client needs to free the memory for raw data */
    if(callback_params.qmi_rsp_data.rsp_data.read_transparent_rsp.content.data_ptr)
    {
      qcril_free(callback_params.qmi_rsp_data.rsp_data.read_transparent_rsp.content.data_ptr);
    }
    return;
  }
#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

send_read_binary_error:
  /* Remove the non-prov session based on the last token */
  qcril_uim_remove_non_provisioning_session(token);
  qcril_uim_response(instance_id, token, RIL_E_GENERIC_FAILURE,
                     NULL, 0, TRUE, "error in qcril_qmi_uim_read_transparent");
  /* Clean up any original request if allocated */
  if (callback_request_ptr)
  {
    qcril_free(callback_request_ptr);
    callback_request_ptr = NULL;
  }

} /* qcril_uim_read_binary */


/*=========================================================================

  FUNCTION:  qcril_uim_read_record

===========================================================================*/
/*!
    @brief
    Function to read record

    @return
    None
*/
/*=========================================================================*/
static void qcril_uim_read_record
(
  qcril_instance_id_e_type  instance_id,
  RIL_Token                 token,
  const RIL_SIM_IO_v6*      request_io_ptr
)
{
  qcril_modem_id_e_type             modem_id = QCRIL_MAX_MODEM_ID - 1;
  int                               res;
  RIL_Errno                         err;
  qmi_uim_read_record_params_type   read_params;
  uint8                             aid[QCRIL_UIM_MAX_AID_SIZE];
  uint8                             path[QCRIL_UIM_MAX_PATH_SIZE];
  uint16                            first_level_df_path = 0;
  qcril_uim_original_request_type * callback_request_ptr = NULL;
  uint8                             slot =  QCRIL_UIM_INVALID_SLOT_INDEX_VALUE;
#ifndef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  qcril_uim_callback_params_type    callback_params;
#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

  if(request_io_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, cannot proceed");
    QCRIL_ASSERT(0);
    return;
  }

  memset(&read_params, 0, sizeof(qmi_uim_read_record_params_type));

  /* Fetch slot info */
  slot = qcril_uim_instance_id_to_slot(instance_id);
  if(slot >= QMI_UIM_MAX_CARD_COUNT)
  {
    QCRIL_LOG_ERROR("Invalid slot %d", slot);
    QCRIL_ASSERT(0);
    return;
  }

  /* Sanity check */
  if ((request_io_ptr->p1 < 0) || (request_io_ptr->p1 > 0xFF) ||
      (request_io_ptr->p2 < 0) || (request_io_ptr->p2 > 0xFF) ||
      (request_io_ptr->p3 < 0) || (request_io_ptr->p3 > 0xFF))
  {
    QCRIL_LOG_ERROR( "Unsupported case, P1: 0x%X, P2: 0x%X, P3: 0x%X \n",
                     request_io_ptr->p1, request_io_ptr->p2, request_io_ptr->p3);
    qcril_uim_response(instance_id, token, RIL_E_REQUEST_NOT_SUPPORTED,
                       NULL, 0, TRUE, NULL);
    return;
  }

  /* File id */
  err = qcril_uim_extract_file_id(request_io_ptr,
                                  &read_params.file_id,
                                  path,
                                  sizeof(path));
  if (err != RIL_E_SUCCESS)
  {
    qcril_uim_response(instance_id, token, err, NULL, 0, TRUE,
                       "error in qcril_uim_extract_file_id");
    return;
  }

  /* Extract the first level DF */
  if ((read_params.file_id.path.data_len >= 4) && (read_params.file_id.path.data_ptr))
  {
    /* Interested only in 3rd & 4th bytes */
    first_level_df_path = (*(read_params.file_id.path.data_ptr + 2) << 8) |
                          (*(read_params.file_id.path.data_ptr + 3));
  }

  /* Session information */
  err = qcril_uim_extract_session_type(slot,
                                       request_io_ptr->aidPtr,
                                       first_level_df_path,
                                       &read_params.session_info,
                                       aid,
                                       sizeof(aid));
  if (err != RIL_E_SUCCESS)
  {
    qcril_uim_response(instance_id, token, err, NULL, 0, TRUE,
                       "error in qcril_uim_extract_session_type");
    return;
  }

  QCRIL_LOG_INFO( "Session type found: %d", read_params.session_info.session_type);

  /* Keep track of non-prov session, will be removed in response handling */
  if ((read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_1) ||
      (read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_2) ||
      (read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_3))
  {
    err = qcril_uim_add_non_provisioning_session(&read_params.session_info,
                                                 token);
    if (err != RIL_E_SUCCESS)
    {
      QCRIL_LOG_ERROR("%s", "Error in adding non prov session!");
    }
  }

  /* Read parameters: length and record */
  read_params.record = request_io_ptr->p1;
  read_params.length = request_io_ptr->p3;

  /* p2 can take values 2, 3, 4, per 3GPP TS 51.011, however QCRIL
   * does not support next record (2) or previous record (3) reads */
  if (request_io_ptr->p2 != 4)
  {
    QCRIL_LOG_ERROR( "%s: unsupported case P2 = %d\n", __FUNCTION__, request_io_ptr->p2);
    qcril_uim_response(instance_id, token, RIL_E_REQUEST_NOT_SUPPORTED,
                       NULL, 0, TRUE, NULL);
    return;
  }

  if (request_io_ptr->pin2 != NULL)
  {
    qmi_uim_verify_pin_params_type         verify_pin_params;
    qcril_uim_pin2_original_request_type * original_request_ptr = NULL;

    memset(&verify_pin_params, 0, sizeof(qmi_uim_verify_pin_params_type));

    /* Session information: same as for read, except card sessions */
    verify_pin_params.session_info = read_params.session_info;

    /* We try to fake a GSM/CDMA first level DF path only for PIN2 verification */
    if ((read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
        (read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
        (read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
    {
      err = qcril_uim_extract_session_type(slot,
                                           request_io_ptr->aidPtr,
                                           QCRIL_UIM_FILEID_DF_CDMA,
                                           &verify_pin_params.session_info,
                                           NULL,
                                           0);
      if ((err != RIL_E_SUCCESS) ||
          (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
          (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
          (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
      {
        err = qcril_uim_extract_session_type(slot,
                                             request_io_ptr->aidPtr,
                                             QCRIL_UIM_FILEID_DF_GSM,
                                             &verify_pin_params.session_info,
                                             NULL,
                                             0);
        if ((err != RIL_E_SUCCESS) ||
            (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
            (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
            (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
        {
          goto send_read_record_error;
        }
      }
      QCRIL_LOG_INFO( "Provisioning session type found: %d",
                      verify_pin_params.session_info.session_type);
      /* Use this session type until the modem resolves card session issue */
      read_params.session_info = verify_pin_params.session_info;
    }

    /* PIN id: PIN2 */
    verify_pin_params.pin_id = QMI_UIM_PIN_ID_PIN2;

    /* PIN value */
    verify_pin_params.pin_data.data_len = (uint16)(strlen(request_io_ptr->pin2));
    verify_pin_params.pin_data.data_ptr = (uint8*)request_io_ptr->pin2;

    /* Allocate original request */
    original_request_ptr = qcril_uim_clone_read_record_request(instance_id,
                                                               modem_id,
                                                               token,
                                                               &read_params);

    if (original_request_ptr == NULL)
    {
      QCRIL_LOG_ERROR("%s", "error allocating memory for clone_read_record_request!");
      goto send_read_record_error;
    }

    QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "verify pin" );
    res = qcril_qmi_uim_verify_pin(qcril_uim.qmi_handle,
                                   &verify_pin_params,
                                   qmi_uim_internal_pin2_callback,
                                   (void*)original_request_ptr,
                                   NULL);

    if (res < 0)
    {
      qcril_free(original_request_ptr);
      QCRIL_LOG_ERROR("%s", "error in qmi_uim_verify_pin!");
      goto send_read_record_error;
    }

    /* original_request_ptr is freed when the PIN2 callback is received */
    return;
  }

  /* Allocate original request, it is freed in qmi_uim_callback */
  callback_request_ptr = qcril_uim_allocate_orig_request(instance_id,
                                                         modem_id,
                                                         token,
                                                         request_io_ptr->command,
                                                         read_params.session_info.session_type);
  if (callback_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "error allocating memory for callback_request_ptr!");
    goto send_read_record_error;
  }

  /* No pin2 was provided: proceed with read transparent */
  QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "read record" );
#ifdef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  if (qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_READ_RECORD,
                                   qcril_uim.qmi_handle,
                                   &read_params,
                                   qmi_uim_callback,
                                   (void*)callback_request_ptr) >= 0)
  {
    return;
  }
#else
  memset(&callback_params, 0, sizeof(qcril_uim_callback_params_type));

  if (qcril_qmi_uim_read_record(qcril_uim.qmi_handle,
                                &read_params,
                                NULL,
                                (void*)callback_request_ptr,
                                &callback_params.qmi_rsp_data) >= 0)
  {
    callback_params.orig_req_data = callback_request_ptr;
    qcril_uim_read_record_resp(&callback_params);

    /* Client needs to free the memory for raw data */
    if(callback_params.qmi_rsp_data.rsp_data.read_record_rsp.content.data_ptr)
    {
      qcril_free(callback_params.qmi_rsp_data.rsp_data.read_record_rsp.content.data_ptr);
    }
    return;
  }
#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

send_read_record_error:
  /* Remove the non-prov session based on the last token */
  qcril_uim_remove_non_provisioning_session(token);
  qcril_uim_response(instance_id, token, RIL_E_GENERIC_FAILURE,
                     NULL, 0, TRUE, "error in qcril_qmi_uim_read_record");
  /* Clean up any original request if allocated */
  if (callback_request_ptr)
  {
    qcril_free(callback_request_ptr);
    callback_request_ptr = NULL;
  }

} /* qcril_uim_read_record */


/*=========================================================================

  FUNCTION:  qcril_uim_update_binary

===========================================================================*/
/*!
    @brief
    Function to write transparent

    @return
    None
*/
/*=========================================================================*/
static void qcril_uim_update_binary
(
  qcril_instance_id_e_type  instance_id,
  RIL_Token                 token,
  const RIL_SIM_IO_v6*      request_io_ptr
)
{
  qcril_modem_id_e_type                  modem_id = QCRIL_MAX_MODEM_ID - 1;
  int                                    res;
  RIL_Errno                              err;
  qmi_uim_write_transparent_params_type  write_params;
  uint8                                  aid[QCRIL_UIM_MAX_AID_SIZE];
  uint8                                  path[QCRIL_UIM_MAX_PATH_SIZE];
  uint16                                 first_level_df_path = 0;
  qcril_uim_original_request_type      * callback_request_ptr = NULL;
  uint8                                  slot = QCRIL_UIM_INVALID_SLOT_INDEX_VALUE;
#ifndef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  qcril_uim_callback_params_type         callback_params;
#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

  if(request_io_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, cannot proceed");
    QCRIL_ASSERT(0);
    return;
  }

  memset(&write_params, 0, sizeof(qmi_uim_write_transparent_params_type));

  /* Fetch slot info */
  slot = qcril_uim_instance_id_to_slot(instance_id);
  if(slot >= QMI_UIM_MAX_CARD_COUNT)
  {
    QCRIL_LOG_ERROR("Invalid slot %d", slot);
    QCRIL_ASSERT(0);
    return;
  }

  /* Sanity check */
  if ((request_io_ptr->p1 < 0) || (request_io_ptr->p1 > 0xFF) ||
      (request_io_ptr->p2 < 0) || (request_io_ptr->p2 > 0xFF) ||
      (request_io_ptr->p3 < 0))
  {
    QCRIL_LOG_ERROR( "Unsupported case, P1: 0x%X, P2: 0x%X, P3: 0x%X \n",
                     request_io_ptr->p1, request_io_ptr->p2, request_io_ptr->p3);
    qcril_uim_response(instance_id, token, RIL_E_REQUEST_NOT_SUPPORTED,
                       NULL, 0, TRUE, NULL);
    return;
  }

  /* File id */
  err = qcril_uim_extract_file_id(request_io_ptr,
                                  &write_params.file_id,
                                  path,
                                  sizeof(path));
  if (err != RIL_E_SUCCESS)
  {
    qcril_uim_response(instance_id, token, err, NULL, 0, TRUE,
                       "error in qcril_uim_extract_file_id");
    return;
  }

  /* Extract the first level DF */
  if ((write_params.file_id.path.data_len >= 4) && (write_params.file_id.path.data_ptr))
  {
    /* Interested only in 3rd & 4th bytes */
    first_level_df_path = (*(write_params.file_id.path.data_ptr + 2) << 8) |
                          (*(write_params.file_id.path.data_ptr + 3));
  }

  /* Session information */
  err = qcril_uim_extract_session_type(slot,
                                       request_io_ptr->aidPtr,
                                       first_level_df_path,
                                       &write_params.session_info,
                                       aid,
                                       sizeof(aid));
  if (err != RIL_E_SUCCESS)
  {
    qcril_uim_response(instance_id, token, err, NULL, 0, TRUE,
                       "error in qcril_uim_extract_session_type");
    return;
  }

  QCRIL_LOG_INFO( "Session type found: %d", write_params.session_info.session_type);

  /* Keep track of non-prov session, will be removed in response handling */
  if ((write_params.session_info.session_type == QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_1) ||
      (write_params.session_info.session_type == QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_2) ||
      (write_params.session_info.session_type == QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_3))
  {
    err = qcril_uim_add_non_provisioning_session(&write_params.session_info,
                                                 token);
    if (err != RIL_E_SUCCESS)
    {
      QCRIL_LOG_ERROR("%s", "Error in adding non prov session!");
    }
  }

  /* Data */
  write_params.data.data_ptr = qcril_uim_alloc_hexstring_to_bin(request_io_ptr->data,
                                                                &write_params.data.data_len);
  if (write_params.data.data_ptr == NULL)
  {
    qcril_uim_response(instance_id, token, RIL_E_GENERIC_FAILURE, NULL, 0, TRUE,
                       "data_ptr is NULL");
    return;
  }
  if (write_params.data.data_len != request_io_ptr->p3)
  {
    qcril_free(write_params.data.data_ptr);
    qcril_uim_response(instance_id, token, RIL_E_GENERIC_FAILURE, NULL, 0, TRUE,
                       "data_len mismatch");
    return;
  }

  /* Write parameters: length and offset */
  write_params.offset = (request_io_ptr->p1) << 8 | (request_io_ptr->p2);

  if (request_io_ptr->pin2 != NULL)
  {
    qmi_uim_verify_pin_params_type         verify_pin_params;
    qcril_uim_pin2_original_request_type * original_request_ptr = NULL;

    memset(&verify_pin_params, 0, sizeof(qmi_uim_verify_pin_params_type));

    /* Session information: same as for write, except card sessions */
    verify_pin_params.session_info = write_params.session_info;

    /* We try to fake a GSM/CDMA first level DF path only for PIN2 verification */
    if ((write_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
        (write_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
        (write_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
    {
      err = qcril_uim_extract_session_type(slot,
                                           request_io_ptr->aidPtr,
                                           QCRIL_UIM_FILEID_DF_CDMA,
                                           &verify_pin_params.session_info,
                                           NULL,
                                           0);
      if ((err != RIL_E_SUCCESS) ||
          (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
          (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
          (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
      {
        err = qcril_uim_extract_session_type(slot,
                                             request_io_ptr->aidPtr,
                                             QCRIL_UIM_FILEID_DF_GSM,
                                             &verify_pin_params.session_info,
                                             NULL,
                                             0);
        if ((err != RIL_E_SUCCESS) ||
            (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
            (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
            (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
        {
          qcril_free(write_params.data.data_ptr);
          goto send_update_binary_error;
        }
      }
      QCRIL_LOG_INFO( "Provisioning session type found: %d",
                      verify_pin_params.session_info.session_type);
      /* Use this session type until the modem resolves card session issue */
      write_params.session_info = verify_pin_params.session_info;
    }

    /* PIN id: PIN2 */
    verify_pin_params.pin_id = QMI_UIM_PIN_ID_PIN2;

    /* PIN value */
    verify_pin_params.pin_data.data_len = (uint16)(strlen(request_io_ptr->pin2));
    verify_pin_params.pin_data.data_ptr = (uint8*)request_io_ptr->pin2;

    /* Allocate original request */
    original_request_ptr = qcril_uim_clone_write_transparent_request(instance_id,
                                                                     modem_id,
                                                                     token,
                                                                     &write_params);

    if (original_request_ptr == NULL)
    {
      qcril_free(write_params.data.data_ptr);
      goto send_update_binary_error;
    }

    QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "verify pin" );
    res = qcril_qmi_uim_verify_pin(qcril_uim.qmi_handle,
                                   &verify_pin_params,
                                   qmi_uim_internal_pin2_callback,
                                   (void*)original_request_ptr,
                                   NULL);

    qcril_free(write_params.data.data_ptr);

    if (res < 0)
    {
      qcril_free(original_request_ptr);
      goto send_update_binary_error;
    }

    /* original_request_ptr is freed when the PIN2 callback is received */
    return;
  }

  /* Allocate original request, it is freed in qmi_uim_callback */
  callback_request_ptr = qcril_uim_allocate_orig_request(instance_id,
                                                         modem_id,
                                                         token,
                                                         request_io_ptr->command,
                                                         write_params.session_info.session_type);
  if (callback_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "error allocating memory for callback_request_ptr!");
    qcril_free(write_params.data.data_ptr);
    goto send_update_binary_error;
  }

  /* No pin2 was provided: proceed with write transparent */
  QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "write transparent" );
#ifdef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  res = qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_WRITE_TRANSPARENT,
                                     qcril_uim.qmi_handle,
                                     &write_params,
                                     qmi_uim_callback,
                                     (void*)callback_request_ptr);

  qcril_free(write_params.data.data_ptr);
  write_params.data.data_ptr = NULL;

  if (res >= 0)
  {
    return;
  }
#else
  memset(&callback_params, 0, sizeof(qcril_uim_callback_params_type));

  res = qcril_qmi_uim_write_transparent(qcril_uim.qmi_handle,
                                        &write_params,
                                        NULL,
                                        (void*)callback_request_ptr,
                                        &callback_params.qmi_rsp_data);

  qcril_free(write_params.data.data_ptr);
  write_params.data.data_ptr = NULL;

  if (res >= 0)
  {
    callback_params.orig_req_data = callback_request_ptr;
    qcril_uim_update_binary_resp(&callback_params);
    return;
  }
#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

send_update_binary_error:
  /* On error, clean up & remove non-prov session based on the last token */
  qcril_uim_remove_non_provisioning_session(token);
  qcril_uim_response(instance_id, token, RIL_E_GENERIC_FAILURE, NULL, 0, TRUE,
                     "error in qcril_qmi_uim_write_transparent");
  /* Clean up any original request if allocated */
  if (callback_request_ptr)
  {
    qcril_free(callback_request_ptr);
    callback_request_ptr = NULL;
  }

} /* qcril_uim_update_binary */


/*=========================================================================

  FUNCTION:  qcril_uim_update_record

===========================================================================*/
/*!
    @brief
    Function to write record

    @return
    None
*/
/*=========================================================================*/
static void qcril_uim_update_record
(
  qcril_instance_id_e_type  instance_id,
  RIL_Token                 token,
  const RIL_SIM_IO_v6*      request_io_ptr
)
{
  qcril_modem_id_e_type             modem_id = QCRIL_MAX_MODEM_ID - 1;
  int                               res;
  RIL_Errno                         err;
  qmi_uim_write_record_params_type  write_params;
  uint8                             aid[QCRIL_UIM_MAX_AID_SIZE];
  uint8                             path[QCRIL_UIM_MAX_PATH_SIZE];
  uint16                            first_level_df_path = 0;
  qcril_uim_original_request_type * callback_request_ptr = NULL;
  uint8                             slot = QCRIL_UIM_INVALID_SLOT_INDEX_VALUE;
#ifndef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  qcril_uim_callback_params_type    callback_params;
#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

  if(request_io_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, cannot proceed");
    QCRIL_ASSERT(0);
    return;
  }

  memset(&write_params, 0, sizeof(qmi_uim_write_record_params_type));

  /* Fetch slot info */
  slot = qcril_uim_instance_id_to_slot(instance_id);
  if(slot >= QMI_UIM_MAX_CARD_COUNT)
  {
    QCRIL_LOG_ERROR("Invalid slot %d", slot);
    QCRIL_ASSERT(0);
    return;
  }

  /* Sanity check */
  if ((request_io_ptr->p1 < 0) || (request_io_ptr->p1 > 0xFF) ||
      (request_io_ptr->p2 < 0) || (request_io_ptr->p2 > 0xFF) ||
      (request_io_ptr->p3 < 0) || (request_io_ptr->p3 > 0xFF))
  {
    QCRIL_LOG_ERROR( "Unsupported case, P1: 0x%X, P2: 0x%X, P3: 0x%X \n",
                     request_io_ptr->p1, request_io_ptr->p2, request_io_ptr->p3);
    qcril_uim_response(instance_id, token, RIL_E_REQUEST_NOT_SUPPORTED,
                       NULL, 0, TRUE, NULL);
    return;
  }

  /* File id */
  err = qcril_uim_extract_file_id(request_io_ptr,
                                  &write_params.file_id,
                                  path,
                                  sizeof(path));
  if (err != RIL_E_SUCCESS)
  {
    qcril_uim_response(instance_id, token, err, NULL, 0, TRUE,
                       "error in qcril_uim_extract_file_id");
    return;
  }

  /* Extract the first level DF */
  if ((write_params.file_id.path.data_len >= 4) && (write_params.file_id.path.data_ptr))
  {
    /* Interested only in 3rd & 4th bytes */
    first_level_df_path = (*(write_params.file_id.path.data_ptr + 2) << 8) |
                          (*(write_params.file_id.path.data_ptr + 3));
  }

  /* Session information */
  err = qcril_uim_extract_session_type(slot,
                                       request_io_ptr->aidPtr,
                                       first_level_df_path,
                                       &write_params.session_info,
                                       aid,
                                       sizeof(aid));
  if (err != RIL_E_SUCCESS)
  {
    qcril_uim_response(instance_id, token, err, NULL, 0, TRUE,
                       "error in qcril_uim_extract_session_type");
    return;
  }

  QCRIL_LOG_INFO( "Session type found: %d", write_params.session_info.session_type);

  /* Keep track of non-prov session, will be removed in response handling */
  if ((write_params.session_info.session_type == QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_1) ||
      (write_params.session_info.session_type == QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_2) ||
      (write_params.session_info.session_type == QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_3))
  {
    err = qcril_uim_add_non_provisioning_session(&write_params.session_info,
                                                 token);
    if (err != RIL_E_SUCCESS)
    {
      QCRIL_LOG_ERROR("%s", "Error in adding non prov session!");
    }
  }

  /* Data */
  write_params.data.data_ptr = qcril_uim_alloc_hexstring_to_bin(request_io_ptr->data,
                                                                &write_params.data.data_len);
  if (write_params.data.data_ptr == NULL)
  {
    qcril_uim_response(instance_id, token, RIL_E_GENERIC_FAILURE, NULL, 0, TRUE,
                       "data_ptr is NULL");
    return;
  }
  if (write_params.data.data_len != request_io_ptr->p3)
  {
    qcril_free(write_params.data.data_ptr);
    qcril_uim_response(instance_id, token, RIL_E_GENERIC_FAILURE, NULL, 0, TRUE,
                       "data_len mismatch");
    return;
  }

  /* Write parameters: length and offset */
  write_params.record = request_io_ptr->p1;

  /* p2 can take values 2, 3, 4, per 3GPP TS 51.011, however QCRIL
   * does not support next record (2) updates */
  if ((request_io_ptr->p2 != 4) && (request_io_ptr->p2 != 3))
  {
    qcril_free(write_params.data.data_ptr);
    QCRIL_LOG_ERROR( "%s: unsupported case P2 = %d\n", __FUNCTION__, request_io_ptr->p2);
    qcril_uim_response(instance_id, token,
                       RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0, TRUE, NULL);
    return;
  }

  if (request_io_ptr->pin2 != NULL)
  {
    qmi_uim_verify_pin_params_type         verify_pin_params;
    qcril_uim_pin2_original_request_type * original_request_ptr = NULL;

    memset(&verify_pin_params, 0, sizeof(qmi_uim_verify_pin_params_type));

    /* Session information: same as for write, except card sessions */
    verify_pin_params.session_info = write_params.session_info;

    /* We try to fake a GSM/CDMA first level DF path */
    if ((write_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
        (write_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
        (write_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
    {
      err = qcril_uim_extract_session_type(slot,
                                           request_io_ptr->aidPtr,
                                           QCRIL_UIM_FILEID_DF_CDMA,
                                           &verify_pin_params.session_info,
                                           NULL,
                                           0);
      if ((err != RIL_E_SUCCESS) ||
          (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
          (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
          (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
      {
        err = qcril_uim_extract_session_type(slot,
                                             request_io_ptr->aidPtr,
                                             QCRIL_UIM_FILEID_DF_GSM,
                                             &verify_pin_params.session_info,
                                             NULL,
                                             0);
        if ((err != RIL_E_SUCCESS) ||
            (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
            (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
            (verify_pin_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
        {
          qcril_free(write_params.data.data_ptr);
          goto send_update_record_error;
        }
      }
      QCRIL_LOG_INFO( "Provisioning session type found: %d",
                      verify_pin_params.session_info.session_type);
      /* Use this session type until the modem resolves card session issue */
      write_params.session_info = verify_pin_params.session_info;
    }

    /* PIN id: PIN2 */
    verify_pin_params.pin_id = QMI_UIM_PIN_ID_PIN2;

    /* PIN value */
    verify_pin_params.pin_data.data_len = (uint16)(strlen(request_io_ptr->pin2));
    verify_pin_params.pin_data.data_ptr = (uint8*)request_io_ptr->pin2;

    /* Allocate original request */
    original_request_ptr = qcril_uim_clone_write_record_request(instance_id,
                                                                modem_id,
                                                                token,
                                                                &write_params);

    if (original_request_ptr == NULL)
    {
      qcril_free(write_params.data.data_ptr);
      goto send_update_record_error;
    }

    QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "verify pin" );
    res = qcril_qmi_uim_verify_pin(qcril_uim.qmi_handle,
                                   &verify_pin_params,
                                   qmi_uim_internal_pin2_callback,
                                   (void*)original_request_ptr,
                                   NULL);

    qcril_free(write_params.data.data_ptr);
    write_params.data.data_ptr = NULL;

    if (res < 0)
    {
      qcril_free(original_request_ptr);
      goto send_update_record_error;
    }

    /* original_request_ptr is freed when the PIN2 callback is received */
    return;
  }

  /* Allocate original request, it is freed in qmi_uim_callback */
  callback_request_ptr = qcril_uim_allocate_orig_request(instance_id,
                                                         modem_id,
                                                         token,
                                                         request_io_ptr->command,
                                                         write_params.session_info.session_type);
  if (callback_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "error allocating memory for callback_request_ptr!");
    qcril_free(write_params.data.data_ptr);
    goto send_update_record_error;
  }

  /* No pin2 was provided: proceed with write record */
  QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "write record" );
#ifdef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  res = qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_WRITE_RECORD,
                                     qcril_uim.qmi_handle,
                                     &write_params,
                                     qmi_uim_callback,
                                     (void*)callback_request_ptr);

  qcril_free(write_params.data.data_ptr);
  write_params.data.data_ptr = NULL;

  if (res >= 0)
  {
    return;
  }
#else
  memset(&callback_params, 0, sizeof(qcril_uim_callback_params_type));

  res = qcril_qmi_uim_write_record(qcril_uim.qmi_handle,
                                   &write_params,
                                   NULL,
                                   (void*)callback_request_ptr,
                                   &callback_params.qmi_rsp_data);

  qcril_free(write_params.data.data_ptr);
  write_params.data.data_ptr = NULL;

  if (res >= 0)
  {
    callback_params.orig_req_data = callback_request_ptr;
    qcril_uim_update_record_resp(&callback_params);
    return;
  }

#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

send_update_record_error:
  /* Remove the non-prov session based on the last token */
  qcril_uim_remove_non_provisioning_session(token);
  qcril_uim_response(instance_id, token, RIL_E_GENERIC_FAILURE,
                     NULL, 0, TRUE, "error in qcril_qmi_uim_write_record");
  /* Clean up any original request if allocated */
  if (callback_request_ptr)
  {
    qcril_free(callback_request_ptr);
    callback_request_ptr = NULL;
  }

} /* qcril_uim_update_record */


/*=========================================================================

  FUNCTION:  qcril_uim_get_response

===========================================================================*/
/*!
    @brief
    Function to get the file attributes

    @return
    None
*/
/*=========================================================================*/
static void qcril_uim_get_response
(
  qcril_instance_id_e_type  instance_id,
  RIL_Token                 token,
  const RIL_SIM_IO_v6*      request_io_ptr
)
{
  qcril_modem_id_e_type                    modem_id = QCRIL_MAX_MODEM_ID - 1;
  RIL_Errno                                err;
  qmi_uim_get_file_attributes_params_type  get_attr_params;
  uint8                                    aid[QCRIL_UIM_MAX_AID_SIZE];
  uint8                                    path[QCRIL_UIM_MAX_PATH_SIZE];
  uint16                                   first_level_df_path = 0;
  qcril_uim_original_request_type        * callback_request_ptr = NULL;
  uint8                                    slot = QCRIL_UIM_INVALID_SLOT_INDEX_VALUE;
#ifndef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  qcril_uim_callback_params_type           callback_params;
#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

  if(request_io_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, cannot proceed");
    QCRIL_ASSERT(0);
    return;
  }

  QCRIL_LOG_INFO( "%s Enter \n", __FUNCTION__);

  memset(&get_attr_params, 0, sizeof(qmi_uim_get_file_attributes_params_type));

  /* Fetch slot info */
  slot = qcril_uim_instance_id_to_slot(instance_id);
  if(slot >= QMI_UIM_MAX_CARD_COUNT)
  {
    QCRIL_LOG_ERROR("Invalid slot %d", slot);
    QCRIL_ASSERT(0);
    return;
  }

  /* File id */
  err = qcril_uim_extract_file_id(request_io_ptr,
                                  &get_attr_params.file_id,
                                  path,
                                  sizeof(path));
  if (err != RIL_E_SUCCESS)
  {
    qcril_uim_response(instance_id, token,
                       err, NULL, 0, TRUE, "error in qcril_uim_extract_file_id");
    return;
  }

  /* Extract the first level DF */
  if ((get_attr_params.file_id.path.data_len >= 4) && (get_attr_params.file_id.path.data_ptr))
  {
    /* Interested only in 3rd & 4th bytes */
    first_level_df_path = (*(get_attr_params.file_id.path.data_ptr + 2) << 8) |
                          (*(get_attr_params.file_id.path.data_ptr + 3));
  }

  /* Session information */
  err = qcril_uim_extract_session_type(slot,
                                       request_io_ptr->aidPtr,
                                       first_level_df_path,
                                       &get_attr_params.session_info,
                                       aid,
                                       sizeof(aid));
  if (err != RIL_E_SUCCESS)
  {
    qcril_uim_response(instance_id, token, err, NULL, 0, TRUE,
                       "error in qcril_uim_extract_session_type");
    return;
  }

  QCRIL_LOG_INFO( "Session type found: %d", get_attr_params.session_info.session_type);

  /* Keep track of non-prov session, will be removed in response handling */
  if ((get_attr_params.session_info.session_type == QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_1) ||
      (get_attr_params.session_info.session_type == QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_2) ||
      (get_attr_params.session_info.session_type == QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_3))
  {
    err = qcril_uim_add_non_provisioning_session(&get_attr_params.session_info,
                                                 token);
    if (err != RIL_E_SUCCESS)
    {
      QCRIL_LOG_ERROR("%s", "Error in adding non prov session!");
    }
  }

  /* Allocate original request, it is freed in qmi_uim_callback */
  callback_request_ptr = qcril_uim_allocate_orig_request(instance_id,
                                                         modem_id,
                                                         token,
                                                         request_io_ptr->command,
                                                         get_attr_params.session_info.session_type);
  if (callback_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "error allocating memory for callback_request_ptr!");
    goto send_get_response_error;
  }

  /* In case of get_file_attributes, the FW cannot pass
     an optional PIN2 */

  QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "get file attributes" );
#ifdef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  if (qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_GET_RESPONSE,
                                   qcril_uim.qmi_handle,
                                   &get_attr_params,
                                   qmi_uim_callback,
                                   (void*)callback_request_ptr) >= 0)
  {
    return;
  }
#else
  memset(&callback_params, 0, sizeof(qcril_uim_callback_params_type));

  if (qcril_qmi_uim_get_file_attributes(qcril_uim.qmi_handle,
                                        &get_attr_params,
                                        NULL,
                                        (void*)callback_request_ptr,
                                        &callback_params.qmi_rsp_data) >= 0)
  {
    callback_params.orig_req_data = callback_request_ptr;
    qcril_uim_get_response_resp(&callback_params);

    /* Client needs to free the memory for raw data */
    if(callback_params.qmi_rsp_data.rsp_data.get_file_attributes_rsp.raw_value.data_ptr)
    {
      qcril_free(callback_params.qmi_rsp_data.rsp_data.get_file_attributes_rsp.raw_value.data_ptr);
    }
    return;
  }
#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

send_get_response_error:
  /* Remove the non-prov session based on the last token */
  qcril_uim_remove_non_provisioning_session(token);
  qcril_uim_response(instance_id, token, RIL_E_GENERIC_FAILURE,
                     NULL, 0, TRUE, "error in qcril_qmi_uim_get_file_attributes");
  /* Clean up any original request if allocated */
  if (callback_request_ptr)
  {
    qcril_free(callback_request_ptr);
    callback_request_ptr = NULL;
  }

} /* qcril_uim_get_response */


/*===========================================================================

  FUNCTION:  qcril_uim_request_get_fdn_status

===========================================================================*/
/*!
    @brief
    Handles QCRIL_EVT_INTERNAL_MMGSDI_GET_FDN_STATUS request from QCRIL.
    This is due to handling of RIL_REQUEST_QUERY_FACILITY_LOCK with facility
    string "FD" from the framework.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_request_get_fdn_status
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type       *const ret_ptr /*!< Output parameter */
)
{
  qcril_modem_id_e_type                     modem_id  = QCRIL_MAX_MODEM_ID - 1;
  uint8                                  ** in_ptr    = NULL;
  RIL_Errno                                 err;
  uint8                                     slot      = QCRIL_UIM_INVALID_SLOT_INDEX_VALUE;
  uint16                                    aid_size  = 0;
  uint16                                    first_level_df_path = 0;
  qmi_uim_get_service_status_params_type    service_status_params;
  qcril_uim_original_request_type         * callback_request_ptr = NULL;
#ifndef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  qcril_uim_callback_params_type            callback_params;
#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

  if(params_ptr == NULL || ret_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, cannot process response");
    QCRIL_ASSERT(0);
    return;
  }

  memset(&service_status_params, 0, sizeof(qmi_uim_get_service_status_params_type));

  /* Parse input pin info */
  in_ptr = (uint8 **)(params_ptr->data);

  /* Sanity checks
     in_ptr[0]: facility string code
     in_ptr[1]: password
     in_ptr[2]: service class bit (unused)
     in_ptr[3]: AID value */
  if(in_ptr == NULL || in_ptr[0] == NULL)
  {
    qcril_uim_response(params_ptr->instance_id, params_ptr->t,
                       RIL_E_GENERIC_FAILURE, NULL, 0,
                       TRUE, "NULL pointer in facilty string" );
    QCRIL_ASSERT(0);
    return;
  }

  QCRIL_LOG_INFO( "qcril_uim_request_get_fdn_status(%s, %s, %s)\n",
                  in_ptr[0],
                  (in_ptr[1] != NULL) ? (const char *)in_ptr[1] : "NULL",
                  (in_ptr[3] != NULL) ? (const char *)in_ptr[3] : "NULL" );

  /* Add entry to ReqList */
  QCRIL_UIM_ADD_ENTRY_TO_REQUEST_LIST(params_ptr);

  /* Check facility string */
  if (memcmp(in_ptr[0], "FD", 2) != 0)
  {
    qcril_uim_response(params_ptr->instance_id, params_ptr->t,
                       RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0,
                       TRUE, "unsupported facilty string" );
    return;
  }

  /* Fetch slot info */
  slot = qcril_uim_instance_id_to_slot(params_ptr->instance_id);
  if(slot >= QMI_UIM_MAX_CARD_COUNT)
  {
    QCRIL_LOG_ERROR("Invalid slot %d", slot);
    QCRIL_ASSERT(0);
    return;
  }

  /* Update the file path based on passed aid pointer */
  aid_size = (in_ptr[3] == NULL) ? 0 : strlen((const char *)in_ptr[3]);
  first_level_df_path = (aid_size == 0) ?
                         QCRIL_UIM_FILEID_DF_CDMA : QCRIL_UIM_FILEID_ADF_USIM_CSIM;

  /* Extract session type, we need prov session for getting FDN service status */
  err = qcril_uim_extract_session_type(slot,
                                       (const char *)in_ptr[3],
                                       first_level_df_path,
                                       &service_status_params.session_info,
                                       NULL,
                                       0);
  if ((err != RIL_E_SUCCESS) ||
      (service_status_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
      (service_status_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
      (service_status_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
  {
    if (first_level_df_path == QCRIL_UIM_FILEID_DF_CDMA)
    {
      err = qcril_uim_extract_session_type(slot,
                                           (const char *)in_ptr[3],
                                           QCRIL_UIM_FILEID_DF_GSM,
                                           &service_status_params.session_info,
                                           NULL,
                                           0);
    }
    if ((err != RIL_E_SUCCESS) ||
        (service_status_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
        (service_status_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
        (service_status_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
    {
      qcril_uim_response(params_ptr->instance_id, params_ptr->t,
                         RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0,
                         TRUE, "error extracting session info");
      return;
    }
  }

  QCRIL_LOG_INFO( "Session type found: %d", service_status_params.session_info.session_type);

  /* Capabilities mask paramter */
  service_status_params.mask = QMI_UIM_CAPS_MASK_SERVICE_FDN;

  /* Allocate original request, it is freed in qmi_uim_callback */
  callback_request_ptr = qcril_uim_allocate_orig_request(params_ptr->instance_id,
                                                         modem_id,
                                                         params_ptr->t,
                                                         params_ptr->event_id,
                                                         service_status_params.session_info.session_type);
  if (callback_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "error allocating memory for callback_request_ptr!");
    goto send_get_fdn_error;
  }

  QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "get service status" );
#ifdef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  if (qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_GET_FDN,
                                   qcril_uim.qmi_handle,
                                   &service_status_params,
                                   qmi_uim_callback,
                                   (void*)callback_request_ptr) >= 0)
  {
    return;
  }
#else
  memset(&callback_params, 0, sizeof(qcril_uim_callback_params_type));

  if (qcril_qmi_uim_get_service_status(qcril_uim.qmi_handle,
                                       &service_status_params,
                                       NULL,
                                       (void*)callback_request_ptr,
                                       &callback_params.qmi_rsp_data) >= 0)
  {
    callback_params.orig_req_data = callback_request_ptr;
    qcril_uim_get_fdn_status_resp(&callback_params);
    return;
  }
#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

send_get_fdn_error:
  qcril_uim_response(params_ptr->instance_id, params_ptr->t, RIL_E_GENERIC_FAILURE,
                     NULL, 0, TRUE, "error in qcril_qmi_uim_get_service_status");
  /* Clean up any original request if allocated */
  if (callback_request_ptr)
  {
    qcril_free(callback_request_ptr);
    callback_request_ptr = NULL;
  }

} /* qcril_uim_request_get_fdn_status */


/*===========================================================================

  FUNCTION:  qcril_uim_request_set_fdn_status

===========================================================================*/
/*!
    @brief
    Handles QCRIL_EVT_INTERNAL_MMGSDI_SET_FDN_STATUS request from QCRIL.
    This is due to handling of RIL_REQUEST_SET_FACILITY_LOCK with facility
    string "FD" from the framework.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_request_set_fdn_status
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type       *const ret_ptr /*!< Output parameter */
)
{
  qcril_modem_id_e_type                     modem_id  = QCRIL_MAX_MODEM_ID - 1;
  uint8                                  ** in_ptr    = NULL;
  int                                       res       = 0;
  RIL_Errno                                 err;
  uint8                                     slot      = QCRIL_UIM_INVALID_SLOT_INDEX_VALUE;
  uint16                                    aid_size  = 0;
  uint16                                    first_level_df_path = 0;
  qmi_uim_set_service_status_params_type    service_status_params;
  qcril_uim_original_request_type         * callback_request_ptr = NULL;
#ifndef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  qcril_uim_callback_params_type            callback_params;
#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

  if(params_ptr == NULL || ret_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, cannot process response");
    QCRIL_ASSERT(0);
    return;
  }

  memset(&service_status_params, 0, sizeof(qmi_uim_set_service_status_params_type));

  /* Parse input pin info */
  in_ptr = (uint8 **)(params_ptr->data);

  /* Sanity checks
     in_ptr[0]: facility string code
     in_ptr[1]: lock/unlock
	 in_ptr[2]: password
     in_ptr[3]: service class bit (unused)
     in_ptr[4]: AID value */
  if(in_ptr == NULL || in_ptr[0] == NULL || in_ptr[1] == NULL)
  {
    qcril_uim_response(params_ptr->instance_id, params_ptr->t,
                       RIL_E_GENERIC_FAILURE, NULL, 0,
                       TRUE, "NULL pointer in facilty string" );
    QCRIL_ASSERT(0);
    return;
  }

  QCRIL_LOG_INFO( "qcril_uim_request_set_fdn_status(%s, %s, %s, %s)\n",
                  in_ptr[0],
                  in_ptr[1],
                  (in_ptr[2] != NULL) ? (const char *)in_ptr[2] : "NULL",
                  (in_ptr[4] != NULL) ? (const char *)in_ptr[4] : "NULL");

  /* Add entry to ReqList */
  QCRIL_UIM_ADD_ENTRY_TO_REQUEST_LIST(params_ptr);

  /* Check facility string */
  if (memcmp(in_ptr[0], "FD", 2) != 0)
  {
    qcril_uim_response(params_ptr->instance_id, params_ptr->t,
                       RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0,
                       TRUE, "unsupported facilty string" );
    return;
  }

  /* Fetch slot info */
  slot = qcril_uim_instance_id_to_slot(params_ptr->instance_id);
  if(slot >= QMI_UIM_MAX_CARD_COUNT)
  {
    QCRIL_LOG_ERROR("Invalid slot %d", slot);
    QCRIL_ASSERT(0);
    return;
  }

  /* Update the file path based on passed aid_ptr */
  aid_size = (in_ptr[4] == NULL) ? 0 : strlen((const char *)in_ptr[4]);
  first_level_df_path = (aid_size == 0) ?
                         QCRIL_UIM_FILEID_DF_CDMA : QCRIL_UIM_FILEID_ADF_USIM_CSIM;

  /* Extract session type, we need prov session for setting FDN status */
  err = qcril_uim_extract_session_type(slot,
                                       (const char *)in_ptr[4],
                                       first_level_df_path,
                                       &service_status_params.session_info,
                                       NULL,
                                       0);
  if ((err != RIL_E_SUCCESS) ||
      (service_status_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
      (service_status_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
      (service_status_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
  {
    if (first_level_df_path == QCRIL_UIM_FILEID_DF_CDMA)
    {
      err = qcril_uim_extract_session_type(slot,
                                           (const char *)in_ptr[4],
                                           QCRIL_UIM_FILEID_DF_GSM,
                                           &service_status_params.session_info,
                                           NULL,
                                           0);
    }
    if ((err != RIL_E_SUCCESS) ||
        (service_status_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_1) ||
        (service_status_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_2) ||
        (service_status_params.session_info.session_type == QMI_UIM_SESSION_TYPE_CARD_SLOT_3))
    {
      qcril_uim_response(params_ptr->instance_id, params_ptr->t,
                         RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0,
                         TRUE, "error extracting session info");
      return;
    }
  }

  QCRIL_LOG_INFO( "Session type found: %d", service_status_params.session_info.session_type);

  /* service status parameter */
  if (*in_ptr[1] == '0')
  {
    service_status_params.fdn_status = QMI_UIM_SERVICE_DISABLE;
  }
  else if (*in_ptr[1] == '1')
  {
    service_status_params.fdn_status = QMI_UIM_SERVICE_ENABLE;
  }
  else
  {
    qcril_uim_response(params_ptr->instance_id, params_ptr->t,
                       RIL_E_GENERIC_FAILURE, NULL, 0,
                       TRUE, "invalid input paramter data[1]");
    return;
  }

  /* If PIN2 was provided, verify PIN2 */
  if (in_ptr[2] != NULL)
  {
    qmi_uim_verify_pin_params_type         verify_pin_params;
    qcril_uim_pin2_original_request_type * original_request_ptr = NULL;

    memset(&verify_pin_params, 0, sizeof(qmi_uim_verify_pin_params_type));

    /* Session information */
    verify_pin_params.session_info = service_status_params.session_info;

    /* PIN id: PIN2 */
    verify_pin_params.pin_id = QMI_UIM_PIN_ID_PIN2;

    /* PIN value */
    verify_pin_params.pin_data.data_len = (uint16)(strlen((const char*)in_ptr[2]));
    verify_pin_params.pin_data.data_ptr = (uint8*)in_ptr[2];

    /* Allocate original request */
    original_request_ptr = qcril_uim_clone_set_fdn_status_request(params_ptr->instance_id,
                                                                  modem_id,
                                                                  params_ptr->t,
                                                                  &service_status_params);

    if (original_request_ptr == NULL)
    {
      QCRIL_LOG_ERROR("%s", "error allocating memory for clone_set_fdn_status_request!");
      goto send_set_fdn_error;
    }

    QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "verify pin" );
    res = qcril_qmi_uim_verify_pin(qcril_uim.qmi_handle,
                                   &verify_pin_params,
                                   qmi_uim_internal_pin2_callback,
                                   (void*)original_request_ptr,
                                   NULL);

    if (res < 0)
    {
      qcril_free(original_request_ptr);
      QCRIL_LOG_ERROR("%s", "error in qmi_uim_verify_pin!");
      goto send_set_fdn_error;
    }

    /* original_request_ptr is freed when the PIN2 callback is received */
    return;
  }

  /* Allocate original request, it is freed in qmi_uim_callback */
  callback_request_ptr = qcril_uim_allocate_orig_request(params_ptr->instance_id,
                                                         modem_id,
                                                         params_ptr->t,
                                                         params_ptr->event_id,
                                                         service_status_params.session_info.session_type);
  if (callback_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "error allocating memory for callback_request_ptr!");
    goto send_set_fdn_error;
  }

  callback_request_ptr->data.fdn_status = service_status_params.fdn_status;

  /* No pin2 provided, so proceed with set service status */
  QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "set service status" );
#ifdef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  if (qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_SET_FDN,
                                   qcril_uim.qmi_handle,
                                   &service_status_params,
                                   qmi_uim_callback,
                                   (void*)callback_request_ptr) >= 0)
  {
    return;
  }
#else
  memset(&callback_params, 0, sizeof(qcril_uim_callback_params_type));

  if (qcril_qmi_uim_set_service_status(qcril_uim.qmi_handle,
                                       &service_status_params,
                                       NULL,
                                       (void*)callback_request_ptr,
                                       &callback_params.qmi_rsp_data) >= 0)
  {
    callback_params.orig_req_data = callback_request_ptr;
    qcril_uim_set_fdn_status_resp(&callback_params);
    return;
  }
#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

send_set_fdn_error:
  qcril_uim_response(params_ptr->instance_id, params_ptr->t, RIL_E_GENERIC_FAILURE,
                     NULL, 0, TRUE, "error in qcril_qmi_uim_set_service_status");
  /* Clean up any original request if allocated */
  if (callback_request_ptr)
  {
    qcril_free(callback_request_ptr);
    callback_request_ptr = NULL;
  }

} /* qcril_uim_request_set_fdn_status */


/*=========================================================================

  FUNCTION:  qcril_uim_get_status

===========================================================================*/
/*!
    @brief
    Function to send card status

    @return
    None
*/
/*=========================================================================*/
static void qcril_uim_get_status
(
  qcril_instance_id_e_type  instance_id,
  RIL_Token                 token,
  const RIL_SIM_IO_v6*      request_io_ptr
)
{
  if(request_io_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, cannot proceed");
    QCRIL_ASSERT(0);
    return;
  }

  /* Currently not supported */
  QCRIL_LOG_ERROR( "%s", "NOTIMPL: qcril_uim_get_status\n");
  qcril_uim_response(instance_id, token,
                     RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0, TRUE, NULL);
} /* qcril_uim_get_status */


/*=========================================================================

  FUNCTION:  qcril_uim_read_binary_resp

===========================================================================*/
/*!
    @brief
    Process the response for read binary.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_read_binary_resp
(
  const qcril_uim_callback_params_type * const params_ptr
)
{
  RIL_Token                         token;
  RIL_Errno                         ril_err;
  RIL_SIM_IO_Response               ril_response;
  qcril_uim_original_request_type * original_request_ptr = NULL;

  if(params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL params_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  /* Retreive original request */
  original_request_ptr = (qcril_uim_original_request_type*)params_ptr->orig_req_data;
  if(original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL original_request_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  ril_err = (params_ptr->qmi_rsp_data.qmi_err_code == 0) ? RIL_E_SUCCESS : RIL_E_GENERIC_FAILURE;
  token   = original_request_ptr->token;

  QCRIL_LOG_DEBUG( "qcril_uim_read_binary_resp: token=%d qmi_err_code=%d \n",
                    qcril_log_get_token_id(token),
                    params_ptr->qmi_rsp_data.qmi_err_code);

  memset(&ril_response, 0, sizeof(RIL_SIM_IO_Response));

  ril_response.sw1 = params_ptr->qmi_rsp_data.rsp_data.read_transparent_rsp.sw1;
  ril_response.sw2 = params_ptr->qmi_rsp_data.rsp_data.read_transparent_rsp.sw2;

  ril_response.simResponse = qcril_uim_alloc_bin_to_hexstring(
                                params_ptr->qmi_rsp_data.rsp_data.read_transparent_rsp.content.data_ptr,
                                params_ptr->qmi_rsp_data.rsp_data.read_transparent_rsp.content.data_len);

  QCRIL_LOG_DEBUG( "RIL_SIM_IO_Response: sw1=%d sw2=%d data=%s\n",
                    ril_response.sw1, ril_response.sw2,
                    ril_response.simResponse != NULL ? ril_response.simResponse : "NULL");

  /* Remove the non-prov session based on the last token */
  qcril_uim_remove_non_provisioning_session(token);

  /* Workaround until the modem build is ready */
  if (ril_response.sw1 == 0)
  {
    if (params_ptr->qmi_rsp_data.qmi_err_code == RIL_E_SUCCESS)
    {
      ril_response.sw1 = 0x90;
      ril_response.sw2 = 0x00;
    }
    else
    {
      ril_response.sw1 = 0x94;
      ril_response.sw2 = 0x04;
    }
  }

  /* Generate response */
  qcril_uim_response(original_request_ptr->instance_id,
                     token,
                     ril_err,
                     &ril_response,
                     sizeof(RIL_SIM_IO_Response),
                     TRUE,
                     NULL);

  if (ril_response.simResponse)
  {
    qcril_free(ril_response.simResponse);
    ril_response.simResponse = NULL;
  }

  /* Free memory allocated originally in the request */
  qcril_free(original_request_ptr);
  original_request_ptr = NULL;

} /* qcril_uim_read_binary_resp */


/*=========================================================================

  FUNCTION:  qcril_uim_read_record_resp

===========================================================================*/
/*!
    @brief
    Process the response for read record.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_read_record_resp
(
  const qcril_uim_callback_params_type * const params_ptr
)
{
  RIL_Token                         token;
  RIL_Errno                         ril_err;
  RIL_SIM_IO_Response               ril_response;
  qcril_uim_original_request_type * original_request_ptr = NULL;

  if(params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL params_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  /* Retrieve original request */
  original_request_ptr = (qcril_uim_original_request_type*)params_ptr->orig_req_data;
  if(original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL original_request_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  ril_err = (params_ptr->qmi_rsp_data.qmi_err_code == 0) ? RIL_E_SUCCESS : RIL_E_GENERIC_FAILURE;
  token   = original_request_ptr->token;

  QCRIL_LOG_DEBUG( "qcril_uim_read_record_resp: token=%d qmi_err_code=%d \n",
                    qcril_log_get_token_id(token),
                    params_ptr->qmi_rsp_data.qmi_err_code);

  memset(&ril_response, 0, sizeof(RIL_SIM_IO_Response));

  ril_response.sw1 = params_ptr->qmi_rsp_data.rsp_data.read_record_rsp.sw1;
  ril_response.sw2 = params_ptr->qmi_rsp_data.rsp_data.read_record_rsp.sw2;

  ril_response.simResponse = qcril_uim_alloc_bin_to_hexstring(
                                params_ptr->qmi_rsp_data.rsp_data.read_record_rsp.content.data_ptr,
                                params_ptr->qmi_rsp_data.rsp_data.read_record_rsp.content.data_len);

  QCRIL_LOG_DEBUG( "RIL_SIM_IO_Response: sw1=0x%X sw2=0x%X data=%s\n",
                    ril_response.sw1, ril_response.sw2,
                    ril_response.simResponse != NULL ? ril_response.simResponse : "NULL");

  /* Remove the non-prov session based on the last token */
  qcril_uim_remove_non_provisioning_session(token);

  /* Workaround until the modem build is ready */
  if (ril_response.sw1 == 0)
  {
    if (params_ptr->qmi_rsp_data.qmi_err_code == RIL_E_SUCCESS)
    {
      ril_response.sw1 = 0x90;
      ril_response.sw2 = 0x00;
    }
    else
    {
      ril_response.sw1 = 0x94;
      ril_response.sw2 = 0x04;
    }
  }

  /* Generate response */
  qcril_uim_response(original_request_ptr->instance_id,
                     token,
                     ril_err,
                     &ril_response,
                     sizeof(RIL_SIM_IO_Response),
                     TRUE,
                     NULL);

  if (ril_response.simResponse)
  {
    qcril_free(ril_response.simResponse);
    ril_response.simResponse = NULL;
  }

  /* Free memory allocated originally in the request */
  qcril_free(original_request_ptr);
  original_request_ptr = NULL;

} /* qcril_uim_read_record_resp */


/*=========================================================================

  FUNCTION:  qcril_uim_update_binary_resp

===========================================================================*/
/*!
    @brief
    Process the response for write transparent.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_update_binary_resp
(
  const qcril_uim_callback_params_type * const params_ptr
)
{
  RIL_Token                         token;
  RIL_Errno                         ril_err;
  RIL_SIM_IO_Response               ril_response;
  qcril_uim_original_request_type * original_request_ptr = NULL;

  if(params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL params_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  /* Retrieve original request */
  original_request_ptr = (qcril_uim_original_request_type*)params_ptr->orig_req_data;
  if(original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL original_request_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  ril_err = (params_ptr->qmi_rsp_data.qmi_err_code == 0) ? RIL_E_SUCCESS : RIL_E_GENERIC_FAILURE;
  token   = original_request_ptr->token;

  QCRIL_LOG_DEBUG( "qcril_uim_update_binary_resp: token=%d qmi_err_code=%d \n",
                    qcril_log_get_token_id(token),
                    params_ptr->qmi_rsp_data.qmi_err_code);

  memset(&ril_response, 0, sizeof(RIL_SIM_IO_Response));

  ril_response.sw1 = params_ptr->qmi_rsp_data.rsp_data.write_transparent_rsp.sw1;
  ril_response.sw2 = params_ptr->qmi_rsp_data.rsp_data.write_transparent_rsp.sw2;

  ril_response.simResponse = NULL;

  QCRIL_LOG_DEBUG( "RIL_SIM_IO_Response: sw1=%d sw2=%d data=%s\n",
                    ril_response.sw1, ril_response.sw2,
                    ril_response.simResponse != NULL ? ril_response.simResponse : "NULL");

  /* Remove the non-prov session based on the last token */
  qcril_uim_remove_non_provisioning_session(token);

  /* Workaround until the modem build is ready */
  if (ril_response.sw1 == 0)
  {
    if (params_ptr->qmi_rsp_data.qmi_err_code == RIL_E_SUCCESS)
    {
      ril_response.sw1 = 0x90;
      ril_response.sw2 = 0x00;
    }
    else
    {
      ril_response.sw1 = 0x94;
      ril_response.sw2 = 0x04;
    }
  }

  /* Generate response */
  qcril_uim_response(original_request_ptr->instance_id,
                     token,
                     ril_err,
                     &ril_response,
                     sizeof(RIL_SIM_IO_Response),
                     TRUE,
                     NULL);

  /* Free memory allocated originally in the request */
  qcril_free(original_request_ptr);
  original_request_ptr = NULL;

} /* qcril_uim_update_binary_resp */


/*=========================================================================

  FUNCTION:  qcril_uim_update_record_resp

===========================================================================*/
/*!
    @brief
    Process the response for write record.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_update_record_resp
(
  const qcril_uim_callback_params_type * const params_ptr
)
{
  RIL_Token                         token;
  RIL_Errno                         ril_err;
  RIL_SIM_IO_Response               ril_response;
  qcril_uim_original_request_type * original_request_ptr = NULL;

  if(params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL params_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  /* Retrieve original request */
  original_request_ptr = (qcril_uim_original_request_type*)params_ptr->orig_req_data;
  if(original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL original_request_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  ril_err = (params_ptr->qmi_rsp_data.qmi_err_code == 0) ? RIL_E_SUCCESS : RIL_E_GENERIC_FAILURE;
  token   = original_request_ptr->token;

  QCRIL_LOG_DEBUG( "qcril_uim_update_record_resp: token=%d qmi_err_code=%d \n",
                    qcril_log_get_token_id(token),
                    params_ptr->qmi_rsp_data.qmi_err_code);

  memset(&ril_response, 0, sizeof(RIL_SIM_IO_Response));

  ril_response.sw1 = params_ptr->qmi_rsp_data.rsp_data.write_record_rsp.sw1;
  ril_response.sw2 = params_ptr->qmi_rsp_data.rsp_data.write_record_rsp.sw2;

  ril_response.simResponse = NULL;

  QCRIL_LOG_DEBUG( "RIL_SIM_IO_Response: sw1=%d sw2=%d data=%s\n",
                    ril_response.sw1, ril_response.sw2,
                    ril_response.simResponse != NULL ? ril_response.simResponse : "NULL");

  /* Remove the non-prov session based on the last token */
  qcril_uim_remove_non_provisioning_session(token);

  /* Workaround until the modem build is ready */
  if (ril_response.sw1 == 0)
  {
    if (params_ptr->qmi_rsp_data.qmi_err_code == RIL_E_SUCCESS)
    {
      ril_response.sw1 = 0x90;
      ril_response.sw2 = 0x00;
    }
    else
    {
      ril_response.sw1 = 0x94;
      ril_response.sw2 = 0x04;
    }
  }

  /* Generate response */
  qcril_uim_response(original_request_ptr->instance_id,
                     token,
                     ril_err,
                     &ril_response,
                     sizeof(RIL_SIM_IO_Response),
                     TRUE,
                     NULL);

  /* Free memory allocated originally in the request */
  qcril_free(original_request_ptr);
  original_request_ptr = NULL;

} /* qcril_uim_update_record_resp */


/*=========================================================================

  FUNCTION:  qcril_uim_get_response_resp

===========================================================================*/
/*!
    @brief
    Process the response for get file attributes.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_get_response_resp
(
  const qcril_uim_callback_params_type * const params_ptr
)
{
  RIL_Token                         token;
  RIL_Errno                         ril_err;
  RIL_SIM_IO_Response               ril_response;
  uint8                             icc_attributes[QCRIL_UIM_GET_RESPONSE_MIN_SIZE+1];
  qcril_uim_original_request_type * original_request_ptr = NULL;
  uint8                             slot = QCRIL_UIM_INVALID_SLOT_INDEX_VALUE;

  if(params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL params_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  /* Retrieve original request */
  original_request_ptr = (qcril_uim_original_request_type*)params_ptr->orig_req_data;
  if(original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL original_request_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  ril_err = (params_ptr->qmi_rsp_data.qmi_err_code == 0) ? RIL_E_SUCCESS : RIL_E_GENERIC_FAILURE;
  token   = original_request_ptr->token;
  slot    = qcril_uim_instance_id_to_slot(original_request_ptr->instance_id);
  if(slot >= QMI_UIM_MAX_CARD_COUNT)
  {
    QCRIL_LOG_ERROR("Invalid slot %d", slot);
    QCRIL_ASSERT(0);
    return;
  }

  QCRIL_LOG_DEBUG( "qcril_uim_get_response_resp: token=%d qmi_err_code=%d \n",
                    qcril_log_get_token_id(token),
                    params_ptr->qmi_rsp_data.qmi_err_code);

  memset(&ril_response, 0, sizeof(RIL_SIM_IO_Response));

  ril_response.sw1 = params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.sw1;
  ril_response.sw2 = params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.sw2;

  ril_response.simResponse = NULL;

  /* Compose file attributes in ICC format */
  if (params_ptr->qmi_rsp_data.qmi_err_code == 0)
  {
    memset(icc_attributes, 0x00, sizeof(icc_attributes));

    /* If it is a 2G card, just use the raw data if available */
    if ((qcril_uim_find_app_in_slot(slot, QMI_UIM_APP_SIM) ||
         qcril_uim_find_app_in_slot(slot, QMI_UIM_APP_RUIM)) &&
        (params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.raw_value.data_ptr != NULL) &&
        (params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.raw_value.data_len >= QCRIL_UIM_GET_RESPONSE_MIN_SIZE))
    {
      /* Copy necessary data after sanity check */
      uint16 data_len = (params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.raw_value.data_len <= sizeof(icc_attributes)) ?
                          params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.raw_value.data_len : sizeof(icc_attributes);
      memcpy(icc_attributes,
             params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.raw_value.data_ptr,
             data_len);
    }
    else
    {
      /* File size */
      icc_attributes[2] = (uint8)(params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.file_size >> 8);
      icc_attributes[3] = (uint8)(params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.file_size & 0xFF);
      /* File id */
      icc_attributes[4] = (uint8)(params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.file_id >> 8);
      icc_attributes[5] = (uint8)(params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.file_id & 0xFF);
      /* File type: EF */
      icc_attributes[6] = 0x04;
      switch (params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.file_type)
      {
        case QMI_UIM_FILE_TYPE_LINEAR_FIXED:
          /* Linear fixed */
          icc_attributes[13] = 0x01;
          /* Record size */
          icc_attributes[14] =
            (uint8)(params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.record_size);
          break;
        case QMI_UIM_FILE_TYPE_CYCLIC:
          /* Cyclic */
          icc_attributes[13] = 0x03;
          /* Increase allowed */
          icc_attributes[7] = 1;
          /* Record size */
          icc_attributes[14] =
            (uint8)(params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.record_size);
          break;
        case QMI_UIM_FILE_TYPE_TRANSPARENT:
          /* Transparent */
          icc_attributes[13] = 0x00;
          break;
        default:
          /* non EF! */
          break;
      }
      /* Security attributes - need to be implemented */
      /* File status byte */
      icc_attributes[11] = qcril_uim_get_file_status_byte(
                             params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.raw_value.data_len,
                             params_ptr->qmi_rsp_data.rsp_data.get_file_attributes_rsp.raw_value.data_ptr);

      /* Hardcoded to 2 because we only simulate byte 13 and 14 */
      icc_attributes[12] = 2;
    }

    ril_response.simResponse = qcril_uim_alloc_bin_to_hexstring(
                                  icc_attributes,
                                  sizeof(icc_attributes));
  }

  QCRIL_LOG_DEBUG( "RIL_SIM_IO_Response: sw1=%d sw2=%d data=%s\n",
                    ril_response.sw1, ril_response.sw2,
                    ril_response.simResponse != NULL ? ril_response.simResponse : "NULL");

  /* Remove the non-prov session based on the last token */
  qcril_uim_remove_non_provisioning_session(token);

  /* Workaround until the modem build is ready */
  if (ril_response.sw1 == 0)
  {
    if (params_ptr->qmi_rsp_data.qmi_err_code == RIL_E_SUCCESS)
    {
      ril_response.sw1 = 0x90;
      ril_response.sw2 = 0x00;
    }
    else
    {
      ril_response.sw1 = 0x94;
      ril_response.sw2 = 0x04;
    }
  }

  /* Generate response */
  qcril_uim_response(original_request_ptr->instance_id,
                     token,
                     ril_err,
                     &ril_response,
                     sizeof(RIL_SIM_IO_Response),
                     TRUE,
                     NULL);

  if (ril_response.simResponse)
  {
    qcril_free(ril_response.simResponse);
    ril_response.simResponse = NULL;
  }

  /* Free memory allocated originally in the request */
  qcril_free(original_request_ptr);
  original_request_ptr = NULL;

} /* qcril_uim_get_response_resp */


/*=========================================================================

  FUNCTION:  qcril_uim_parse_gw_imsi

===========================================================================*/
/*!
    @brief
    Parses the data present in EF IMSI for SIM/USIM and packages the
    response in a null terminated ASCII string format.

    The EF structure is defined in 31.102 section 4.2.2:

    <TABLE>
      <TR>
        <TH> Byte(s) </TH>
        <TH> Description </TH>
      </TR>
      <TR>
        <TH> 1 </TH>
        <TD> IMSI length (bytes) </TD>
      </TR>
      <TR>
        <TH> 2 </TH>
        <TD> Bits 0-3: Unused  <BR> Bits 4-7: Digit 1   </TD>
      </TR>
      <TR>
        <TH> 3 to 9  </TH>
        <TD> Bits 0-3: Digit n <BR> Bits 4-7: Digit n+1 </TD>
      </TR>
    </TABLE>

    @return
    Pointer to the memory that contains the IMSI in ASCII string format
*/
/*=========================================================================*/
static char * qcril_uim_parse_gw_imsi
(
  const unsigned char * raw_imsi_ptr,
  unsigned short        raw_imsi_len,
  int                 * parsed_imsi_len_ptr
)
{
  int             src             = 0;
  int             dst             = 0;
  char          * parsed_imsi_ptr = NULL;

  /* Sanity check on input parameters */
  if ((raw_imsi_ptr == NULL) || (parsed_imsi_len_ptr == NULL))
  {
    QCRIL_LOG_ERROR( "Invalid input: raw_imsi_ptr 0x%x, parsed_imsi_len_ptr 0x%x\n",
                     raw_imsi_ptr, parsed_imsi_len_ptr);
    return NULL;
  }

  /* Check for the length of IMSI bytes in the first byte */
  *parsed_imsi_len_ptr = *raw_imsi_ptr;
  if (*parsed_imsi_len_ptr >= raw_imsi_len)
  {
    QCRIL_LOG_ERROR( "Invalid data length %d\n", *parsed_imsi_len_ptr);
    *parsed_imsi_len_ptr = 0;
    return NULL;
  }

  /* Allocate required amount of memory for IMSI in ASCII string format,
     note that it is freed by the caller */
  parsed_imsi_ptr = qcril_malloc((2 * (*parsed_imsi_len_ptr)));
  if (parsed_imsi_ptr == NULL)
  {
    QCRIL_LOG_ERROR( "%s: memory allocation failed\n", __FUNCTION__);
    *parsed_imsi_len_ptr = 0;
    return NULL;
  }

  /* Compose IMSI */
  memset(parsed_imsi_ptr, 0, (2 * (*parsed_imsi_len_ptr)));
  for (src = 1, dst = 0;
      (src <= (*parsed_imsi_len_ptr)) && (dst < ((*parsed_imsi_len_ptr) * 2));
       src++)
  {
    QCRIL_LOG_VERBOSE( "IMSI[%d] src=%4x, dst=", src, raw_imsi_ptr[src]);

    /* Only process lower part of byte for second and subsequent bytes */
    if (src > 1)
    {
      parsed_imsi_ptr[dst] = qcril_uim_bin_to_hexchar(raw_imsi_ptr[src] & 0x0F);
      QCRIL_LOG_VERBOSE( "%c", parsed_imsi_ptr[dst]);
      dst++;
    }
    /* Process upper part of byte for all bytes */
    parsed_imsi_ptr[dst] = qcril_uim_bin_to_hexchar(raw_imsi_ptr[src] >> 4);
    QCRIL_LOG_VERBOSE( "%c\n", parsed_imsi_ptr[dst]);
    dst++;
  }

  /* Update IMSI length in bytes - parsed IMSI in ASCII is raw times 2 */
  *parsed_imsi_len_ptr *= 2;

  return parsed_imsi_ptr;
} /* qcril_uim_parse_gw_imsi */


/*=========================================================================

  FUNCTION:  qcril_uim_parse_1x_imsi

===========================================================================*/
/*!
    @brief
    Parses the data present in EF IMSI_M for RUIM/CSIM and packages the
    response in a null terminated ASCII string format.

    The EF structure is defined in C.S0065-0 section 5.2.2:

    <TABLE>
      <TR>
        <TH> Byte(s) </TH>
        <TH> Description </TH>
      </TR>
      <TR>
        <TH> 1 </TH>
        <TD> IMSI_M_CLASS (1 byte) </TD>
      </TR>
      <TR>
        <TH> 2 to 3 </TH>
        <TD> IMSI_M_S2 from IMSI_M_S (2 bytes) </TD>
      </TR>
      <TR>
        <TH> 4 to 6 </TH>
        <TD> IMSI_M_S1 from IMSI_M_S (3 bytes)  </TD>
      </TR>
      <TR>
        <TH> 7 </TH>
        <TD> IMSI_M_11_12 (1 byte)  </TD>
      </TR>
      <TR>
        <TH> 8 </TH>
        <TD> IMSI_M_PROGRAMMED/IMSI_M_ADDR_NUM (1 byte)  </TD>
      </TR>
      <TR>
        <TH> 9 to 10  </TH>
        <TD> MCC_M (2 bytes)  </TD>
      </TR>
    </TABLE>

    @return
    Pointer to the memory that contains the IMSI in ASCII string format
*/
/*=========================================================================*/
static char * qcril_uim_parse_1x_imsi
(
  const unsigned char * raw_imsi_ptr,
  unsigned short        raw_imsi_len,
  int                 * parsed_imsi_len_ptr
)
{
  uint8     i             = 0;
  uint16    mcc           = 0;
  uint8     mnc           = 0;
  uint32    min1          = 0;
  uint16    min2          = 0;
  uint16    second_three  = 0;
  uint8     thousands     = 0xFF;
  uint16    last_three    = 0;
  uint8     min_to_num[]  = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' };
  uint8     bcd_to_num[]  = { 0xFF, '1', '2', '3', '4', '5', '6', '7', '8',
                              '9', '0', 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

  char          * parsed_imsi_ptr = NULL;

  /* Sanity check on input parameters */
  if ((raw_imsi_ptr == NULL) || (parsed_imsi_len_ptr == NULL))
  {
    QCRIL_LOG_ERROR( "Invalid input: raw_imsi_ptr 0x%x, parsed_imsi_len_ptr 0x%x\n",
                     raw_imsi_ptr, parsed_imsi_len_ptr);
    return NULL;
  }

  /* Check for the minumim length of IMSI_M expected */
  if (raw_imsi_len < QCRIL_UIM_IMSI_M_RAW_SIZE)
  {
    QCRIL_LOG_ERROR( "Invalid data length %d\n", raw_imsi_len);
    return NULL;
  }

  /* Sanity check for IMSI_M_PROGRAMMED indicator */
  if ((raw_imsi_ptr[7] & 0x80) == 0)
  {
    QCRIL_LOG_ERROR("%s", "IMSI_M has not been programmed\n");
    return NULL;
  }

  /* Update parsed length - null terminated ASCII string length */
  *parsed_imsi_len_ptr = QCRIL_UIM_IMSI_M_PARSED_SIZE;

  /* Allocate the number of bytes */
  parsed_imsi_ptr = qcril_malloc(*parsed_imsi_len_ptr);
  if (parsed_imsi_ptr == NULL)
  {
    QCRIL_LOG_ERROR( "%s", "Memory allocation failed for parsed_imsi_ptr! \n");
    *parsed_imsi_len_ptr = 0;
    return NULL;
  }

  memset(parsed_imsi_ptr, 0, *parsed_imsi_len_ptr);

  /* Parse certain parameters */
  mcc           = (((raw_imsi_ptr[9] & 0x03) << 8) + raw_imsi_ptr[8]);
  mnc           = raw_imsi_ptr[6] & 0x7F;
  min1          = ((raw_imsi_ptr[5] <<16) +(raw_imsi_ptr[4] <<8) + raw_imsi_ptr[3]);
  min2          = ((raw_imsi_ptr[2] <<8)  + raw_imsi_ptr[1]);
  /* left 10 bits */
  second_three  = (min1 & 0x00FFC000) >> 14;
  /* middle 4 bits */
  thousands     = (min1 & 0x00003C00) >> 10;
  /* right 10 bits */
  last_three    = (min1 & 0x000003FF);
  thousands     = bcd_to_num[thousands];

  QCRIL_LOG_DEBUG("mcc %d, mnc %d, min2 %d, thousands %d, second_three %d, last_three %d\n",
                  mcc, mnc, min2, thousands, second_three, last_three);

  if ((mcc > 999) || (mnc > 99) || (min2 > 999) || (thousands == 0xFF) ||
      (second_three > 999) || (last_three > 999))
  {
    QCRIL_LOG_ERROR("%s", "Invalid data while parsing IMSI_M \n");
    qcril_free(parsed_imsi_ptr);
    *parsed_imsi_len_ptr = 0;
    return NULL;
  }

  /* Construct ASCII IMSI_M, format:
     <3_digit_MCC><2_digit_11_12_digits><LS_10_digits_IMSI> */
  /* Update MCC - 3 digits */
  parsed_imsi_ptr[i++] = min_to_num[mcc/100];
  mcc %= 100;
  parsed_imsi_ptr[i++] = min_to_num[mcc/10];
  parsed_imsi_ptr[i++] = min_to_num[mcc%10];
  /* Update MNC - 2 digits */
  parsed_imsi_ptr[i++] = min_to_num[mnc/10];
  parsed_imsi_ptr[i++] = min_to_num[mnc%10];
  /* Update the first 3 digits of IMSI */
  parsed_imsi_ptr[i++] = min_to_num[min2/100];
  min2 %= 100;
  parsed_imsi_ptr[i++] = min_to_num[min2/10];
  parsed_imsi_ptr[i++] = min_to_num[min2%10];
  /* Update the last 7 digits of IMSI */
  parsed_imsi_ptr[i++] = min_to_num[second_three/100];
  second_three %= 100;
  parsed_imsi_ptr[i++] = min_to_num[second_three/10];
  parsed_imsi_ptr[i++] = min_to_num[second_three%10];
  parsed_imsi_ptr[i++] = thousands;
  parsed_imsi_ptr[i++] = min_to_num[last_three/100];
  last_three %= 100;
  parsed_imsi_ptr[i++] = min_to_num[last_three/10];
  parsed_imsi_ptr[i++] = min_to_num[last_three%10];

  QCRIL_LOG_VERBOSE( "Parsed IMSI_M value:[%s]", parsed_imsi_ptr);

  return parsed_imsi_ptr;
} /* qcril_uim_parse_1x_imsi */


/*=========================================================================

  FUNCTION:  qcril_uim_get_imsi_resp

===========================================================================*/
/*!
    @brief
    Processes the response for RIL_REQUEST_GET_IMSI. IMSI/IMSI_M is stored
    as a transparent file on the SIM/USIM/CSIM. This information is contained
    within the data field returned in callback from QMI. The EF structure
    and coding is different between SIM/USIM and RUIM/CSIM and are defined
    in the respective Specifications.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_get_imsi_resp
(
  const qcril_uim_callback_params_type * const params_ptr
)
{
  RIL_Token                         token       = 0;
  RIL_Errno                         ril_err     = RIL_E_GENERIC_FAILURE;
  char                            * imsi_ptr    = NULL;
  int                               imsi_length = 0;
  qcril_uim_original_request_type * original_request_ptr = NULL;
  int                               is_gwl      = FALSE;

  if(params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL params_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  /* Retrieve original request */
  original_request_ptr = (qcril_uim_original_request_type*)params_ptr->orig_req_data;
  if(original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL original_request_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  ril_err = (params_ptr->qmi_rsp_data.qmi_err_code == 0) ? RIL_E_SUCCESS : RIL_E_GENERIC_FAILURE;
  token   = (RIL_Token)original_request_ptr->token;

  QCRIL_LOG_DEBUG( "qcril_uim_get_imsi_resp: token=%d, qmi_err_code=%d, session_type=%d \n",
                    qcril_log_get_token_id(token),
                    params_ptr->qmi_rsp_data.qmi_err_code,
                    original_request_ptr->session_type);

  /* Based on the type of request, fetch appropriate IMSI */
  if (ril_err == RIL_E_SUCCESS)
  {
    if((original_request_ptr->session_type == QMI_UIM_SESSION_TYPE_PRI_GW_PROV) ||
       (original_request_ptr->session_type == QMI_UIM_SESSION_TYPE_SEC_GW_PROV) ||
       (original_request_ptr->session_type == QMI_UIM_SESSION_TYPE_TER_GW_PROV))
    {
      imsi_ptr = qcril_uim_parse_gw_imsi(
                  params_ptr->qmi_rsp_data.rsp_data.read_transparent_rsp.content.data_ptr,
                  params_ptr->qmi_rsp_data.rsp_data.read_transparent_rsp.content.data_len,
                  &imsi_length);
      is_gwl = TRUE;
    }
    else if((original_request_ptr->session_type == QMI_UIM_SESSION_TYPE_PRI_1X_PROV) ||
            (original_request_ptr->session_type == QMI_UIM_SESSION_TYPE_SEC_1X_PROV) ||
            (original_request_ptr->session_type == QMI_UIM_SESSION_TYPE_TER_1X_PROV))
    {
      imsi_ptr = qcril_uim_parse_1x_imsi(
                  params_ptr->qmi_rsp_data.rsp_data.read_transparent_rsp.content.data_ptr,
                  params_ptr->qmi_rsp_data.rsp_data.read_transparent_rsp.content.data_len,
                  &imsi_length);
      is_gwl = FALSE;
    }
  }

  qcril_common_update_current_imsi( imsi_ptr, is_gwl );

  if (imsi_ptr == NULL)
  {
    imsi_length = 0;
    ril_err = RIL_E_GENERIC_FAILURE;
  }

  /* No need to call qcril_uim_remove_non_provisioning_session since this
     response happens only for a prov session, continue to generate response */
  qcril_uim_response(original_request_ptr->instance_id,
                     token,
                     ril_err,
                     imsi_ptr,
                     imsi_length,
                     TRUE,
                     NULL);

  if (imsi_ptr)
  {
    qcril_free(imsi_ptr);
    imsi_ptr = NULL;
  }

  /* Free memory allocated originally in the request */
  qcril_free(original_request_ptr);
  original_request_ptr = NULL;

} /* qcril_uim_get_imsi_resp */


/*=========================================================================

  FUNCTION:  qcril_uim_get_fdn_status_resp

===========================================================================*/
/*!
    @brief
    Process the response for get FDN status.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_get_fdn_status_resp
(
  const qcril_uim_callback_params_type * const params_ptr
)
{
  RIL_Token                         token;
  RIL_Errno                         ril_err;
  int                               ret_value            = 0;
  boolean                           cm_indication        = FALSE;
  qcril_uim_original_request_type * original_request_ptr = NULL;

  if(params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL params_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  /* Retrieve original request */
  original_request_ptr = (qcril_uim_original_request_type*)params_ptr->orig_req_data;
  if(original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL original_request_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  ril_err = (params_ptr->qmi_rsp_data.qmi_err_code == 0) ? RIL_E_SUCCESS : RIL_E_GENERIC_FAILURE;
  token   = original_request_ptr->token;

  QCRIL_LOG_DEBUG( "qcril_uim_get_fdn_status_resp: token=%d qmi_err_code=%d \n",
                    qcril_log_get_token_id(token),
                    params_ptr->qmi_rsp_data.qmi_err_code);

  /* Map the response */
  if (params_ptr->qmi_rsp_data.qmi_err_code == 0)
  {
    switch (params_ptr->qmi_rsp_data.rsp_data.get_service_status_rsp.fdn_status)
    {
      case 0:
        ret_value = 2;
        QCRIL_LOG_INFO( "%s", "FDN service not available\n" );
        break;
      case 1:
        ret_value = 0;
        QCRIL_LOG_INFO( "%s", "FDN is available but disabled\n" );
        break;
      case 2:
        ret_value     = 1;
        cm_indication = TRUE;
        QCRIL_LOG_INFO( "%s", "FDN is available and enabled\n" );
        break;
      default:
        ril_err = RIL_E_GENERIC_FAILURE;
        QCRIL_LOG_ERROR( "FDN status unknown 0x%x\n",
                         params_ptr->qmi_rsp_data.rsp_data.get_service_status_rsp.fdn_status );
        break;
    }
  }

  /* No need to call qcril_uim_remove_non_provisioning_session since this
     response happens only for a prov session, continue to generate response */
  if (original_request_ptr->request_id == QCRIL_EVT_INTERNAL_MMGSDI_GET_FDN_STATUS)
  {
    qcril_uim_response(original_request_ptr->instance_id,
                       token,
                       ril_err,
                       &ret_value,
                       sizeof(int),
                       TRUE,
                       NULL);
  }
  else if ((original_request_ptr->request_id == QCRIL_EVT_CM_UPDATE_FDN_STATUS) &&
           (ril_err == RIL_E_SUCCESS))
  {
    /* Send updated FDN status to QCRIL_CM based on the request id */
    if (qcril_process_event(original_request_ptr->instance_id,
                            original_request_ptr->modem_id,
                            QCRIL_EVT_CM_UPDATE_FDN_STATUS,
                            (void *) &cm_indication,
                            sizeof(boolean),
                            token) != E_SUCCESS )
    {
      QCRIL_LOG_ERROR("%s","Internal QCRIL CM Event processing Failed for FDN status update!");
    }
  }

  /* Free memory allocated originally in the request */
  qcril_free(original_request_ptr);
  original_request_ptr = NULL;

} /* qcril_uim_get_fdn_status_resp */


/*=========================================================================

  FUNCTION:  qcril_uim_set_fdn_status_resp

===========================================================================*/
/*!
    @brief
    Process the response for set FDN status.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_set_fdn_status_resp
(
  const qcril_uim_callback_params_type * const params_ptr
)
{
  RIL_Token                         token;
  RIL_Errno                         ril_err;
  qcril_uim_original_request_type * original_request_ptr = NULL;
  boolean                           cm_indication = FALSE;

  if(params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL params_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  /* Retrieve original request */
  original_request_ptr = (qcril_uim_original_request_type*)params_ptr->orig_req_data;
  if(original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL original_request_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  ril_err = (params_ptr->qmi_rsp_data.qmi_err_code == 0) ? RIL_E_SUCCESS : RIL_E_GENERIC_FAILURE;
  token   = original_request_ptr->token;

  if(ril_err == RIL_E_SUCCESS)
  {
    qcril_instance_id_e_type      instance_id = original_request_ptr->instance_id;

    /* Find the correct RILD instance */
#ifdef FEATURE_QCRIL_UIM_QMI_RPC_QCRIL
#ifdef FEATURE_QCRIL_DSDS
    if (qcril_arb_lookup_instance_id_from_session_type(original_request_ptr->session_type,
                                                       &instance_id) != E_SUCCESS)
    {
      QCRIL_LOG_ERROR( "%s", "qcril_instance_id lookup failed!\n");
    }
#else
    QCRIL_LOG_INFO( "%s\n", "Using original request instance_id");
#endif
#else
    instance_id = qmi_ril_get_process_instance_id();
#endif

    QCRIL_LOG_DEBUG("instance_id: %d", instance_id);

    if(original_request_ptr->data.fdn_status == QMI_UIM_SERVICE_ENABLE)
    {
      cm_indication = TRUE;
    }

    /* Send updated FDN status to CM*/
    if ( qcril_process_event(instance_id,
                             original_request_ptr->modem_id,
                             QCRIL_EVT_CM_UPDATE_FDN_STATUS,
                             (void *) &cm_indication,
                             sizeof(boolean),
                             token ) != E_SUCCESS )
    {
      QCRIL_LOG_ERROR("%s","Internal QCRIL CM Event processing Failed for FDN status update!");
    }
  }

  QCRIL_LOG_DEBUG( "qcril_uim_set_fdn_status_resp: token=%d qmi_err_code=%d \n",
                    qcril_log_get_token_id(token),
                    params_ptr->qmi_rsp_data.qmi_err_code);

  /* No need to call qcril_uim_remove_non_provisioning_session since this
     response happens only for a prov session, continue to generate response */
  qcril_uim_response(original_request_ptr->instance_id,
                     token,
                     ril_err,
                     NULL,
                     0,
                     TRUE,
                     NULL);

  /* Free memory allocated originally in the request */
  qcril_free(original_request_ptr);
  original_request_ptr = NULL;

} /* qcril_uim_set_fdn_status_resp */


#ifdef FEATURE_QCRIL_UIM_QMI_APDU_ACCESS
/*=========================================================================

  FUNCTION:  qcril_uim_logical_channel_resp

===========================================================================*/
/*!
    @brief
    Process the response for logical channel command.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_logical_channel_resp
(
  const qcril_uim_callback_params_type * const params_ptr
)
{
  RIL_Token                         token;
  RIL_Errno                         ril_err;
  int                               channel_id           = 0;
  qcril_uim_original_request_type * original_request_ptr = NULL;

  if(params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL params_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  /* Retrieve original request */
  original_request_ptr = (qcril_uim_original_request_type*)params_ptr->orig_req_data;
  if(original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL original_request_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  ril_err = (params_ptr->qmi_rsp_data.qmi_err_code == 0) ? RIL_E_SUCCESS : RIL_E_GENERIC_FAILURE;
  token   = original_request_ptr->token;

  QCRIL_LOG_DEBUG( "qcril_uim_logical_channel_resp: token=%d qmi_err_code=%d request_id=%d \n",
                    qcril_log_get_token_id(token),
                    params_ptr->qmi_rsp_data.qmi_err_code,
                    original_request_ptr->request_id );

  /* No need to call qcril_uim_remove_non_provisioning_session since we do not try
     to open one in the request, continue to generate response */

  /* For open channel request, we send channel id in response, for
        close channel request, we send NULL in response */
  if (original_request_ptr->request_id == RIL_REQUEST_SIM_OPEN_CHANNEL)
  {
    if (params_ptr->qmi_rsp_data.qmi_err_code == 0)
    {
      channel_id = params_ptr->qmi_rsp_data.rsp_data.logical_channel_rsp.channel_id;
      QCRIL_LOG_DEBUG( "qcril_uim_logical_channel_resp: channel_id=%d \n",
                       channel_id );
      qcril_uim_response(original_request_ptr->instance_id,
                         token,
                         ril_err,
                         &channel_id,
                         sizeof(int),
                         TRUE,
                         NULL);
    }
    else
    {
      /* On error, map to appropriate ril error code */
      switch (params_ptr->qmi_rsp_data.qmi_err_code)
      {
        case QMI_SERVICE_ERR_INSUFFICIENT_RESOURCES:
          ril_err = RIL_E_MISSING_RESOURCE;
          break;

        case QMI_SERVICE_ERR_INCOMPATIBLE_STATE:
        case QMI_SERVICE_ERR_SIM_FILE_NOT_FOUND:
          ril_err = RIL_E_NO_SUCH_ELEMENT;
          break;

        default:
          ril_err = RIL_E_GENERIC_FAILURE;
          break;
      }

      /* Send NULL in response */
      qcril_uim_response(original_request_ptr->instance_id,
                         token,
                         ril_err,
                         NULL,
                         0,
                         TRUE,
                         NULL);
    }
  }
  else if (original_request_ptr->request_id == RIL_REQUEST_SIM_CLOSE_CHANNEL)
  {
    qcril_uim_response(original_request_ptr->instance_id,
                       token,
                       ril_err,
                       NULL,
                       0,
                       TRUE,
                       NULL);
  }

  /* Free memory allocated originally in the request */
  qcril_free(original_request_ptr);
  original_request_ptr = NULL;

} /* qcril_uim_logical_channel_resp */


/*=========================================================================

  FUNCTION:  qcril_uim_send_apdu_resp

===========================================================================*/
/*!
    @brief
    Process the response for send apdu command.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_send_apdu_resp
(
  const qcril_uim_callback_params_type * const params_ptr
)
{
  RIL_Token                         token;
  RIL_Errno                         ril_err;
  unsigned short                    apdu_len = 0;
  RIL_SIM_IO_Response               ril_response;
  qcril_uim_original_request_type * original_request_ptr = NULL;
  uint8                             slot = QCRIL_UIM_INVALID_SLOT_INDEX_VALUE;

  if(params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL params_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  /* Retrieve original request */
  original_request_ptr = (qcril_uim_original_request_type*)params_ptr->orig_req_data;
  if(original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL original_request_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  ril_err = (params_ptr->qmi_rsp_data.qmi_err_code == 0) ? RIL_E_SUCCESS : RIL_E_GENERIC_FAILURE;
  token   = original_request_ptr->token;

  QCRIL_LOG_DEBUG( "qcril_uim_send_apdu_resp: token=%d qmi_err_code=%d \n",
                    qcril_log_get_token_id(token),
                    params_ptr->qmi_rsp_data.qmi_err_code );

  /* Special case for long APDUs - we send the response of the long APDU
     stream after we get all the chunks in the corresponding SEND_APDU_INDs */
  if (params_ptr->qmi_rsp_data.qmi_err_code == QMI_SERVICE_ERR_INSUFFICIENT_RESOURCES)
  {
    QCRIL_LOG_DEBUG( "qcril_uim_send_apdu_resp: total_len=0x%x bytes, token=0x%x",
                     params_ptr->qmi_rsp_data.rsp_data.send_apdu_rsp.total_len,
                     params_ptr->qmi_rsp_data.rsp_data.send_apdu_rsp.token );

    /* Fetch slot info */
    slot = qcril_uim_instance_id_to_slot(original_request_ptr->instance_id);
    if(slot >= QMI_UIM_MAX_CARD_COUNT)
    {
      QCRIL_LOG_ERROR("Invalid slot %d, cannot store IND", slot);
      QCRIL_ASSERT(0);
      return;
    }

    /* Store only if the Long APDU response TLV is valid. We also need to check
       and handle cases where the INDs might have come earlier than this response.
       Note that original_request_ptr will be freed when we get SEND_APDU_INDs */
    if (qcril_uim.long_apdu_info[slot].in_use == TRUE)
    {
      /* If Indication already came, we need to check incoming info */
      if ((qcril_uim.long_apdu_info[slot].token          ==
             params_ptr->qmi_rsp_data.rsp_data.send_apdu_rsp.token) &&
          (qcril_uim.long_apdu_info[slot].total_apdu_len ==
             params_ptr->qmi_rsp_data.rsp_data.send_apdu_rsp.total_len))
      {
        /* If Indication already came & incoming info matches,
           nothing else to do */
        return;
      }

      /* Error condition - mismatch in data, send error if there was any previous
         request & store the current response's token */
      QCRIL_LOG_ERROR("Mismatch with global data, token: 0x%x, total_apdu_len: 0x%x",
                      qcril_uim.long_apdu_info[slot].token,
                      qcril_uim.long_apdu_info[slot].total_apdu_len);
      /* Send error for any pending response & proceed to store current resp info */
      qcril_uim_cleanup_long_apdu_info(slot);
    }

    /* Store response info. We return after successfully storing since
       we expect subsequent INDs */
    if (params_ptr->qmi_rsp_data.rsp_data.send_apdu_rsp.total_len > 0)
    {
      QCRIL_LOG_INFO("Storing long_apdu_info for slot:0x%x", slot);
      qcril_uim.long_apdu_info[slot].in_use = TRUE;
      qcril_uim.long_apdu_info[slot].token = params_ptr->qmi_rsp_data.rsp_data.send_apdu_rsp.token;
      qcril_uim.long_apdu_info[slot].total_apdu_len = params_ptr->qmi_rsp_data.rsp_data.send_apdu_rsp.total_len;
      qcril_uim.long_apdu_info[slot].original_request_ptr = original_request_ptr;
      return;
    }
  }

  memset(&ril_response, 0, sizeof(RIL_SIM_IO_Response));

  if (ril_err == RIL_E_SUCCESS)
  {
    /* SW1 & SW2 are the last 2 bytes of the APDU response */
    if (params_ptr->qmi_rsp_data.rsp_data.send_apdu_rsp.apdu_response.data_len >= 2)
    {
      apdu_len = params_ptr->qmi_rsp_data.rsp_data.send_apdu_rsp.apdu_response.data_len;
      ril_response.sw1 = params_ptr->qmi_rsp_data.rsp_data.send_apdu_rsp.apdu_response.data_ptr[apdu_len-2];
      ril_response.sw2 = params_ptr->qmi_rsp_data.rsp_data.send_apdu_rsp.apdu_response.data_ptr[apdu_len-1];

      ril_response.simResponse = qcril_uim_alloc_bin_to_hexstring(
                                    params_ptr->qmi_rsp_data.rsp_data.send_apdu_rsp.apdu_response.data_ptr,
                                    apdu_len - 2);
    }
    else
    {
      ril_err = RIL_E_GENERIC_FAILURE;
      QCRIL_LOG_ERROR( "Invalid apdu_response.data_len: 0x%x\n",
                       params_ptr->qmi_rsp_data.rsp_data.send_apdu_rsp.apdu_response.data_len );
    }
  }

  QCRIL_LOG_DEBUG( "RIL_SIM_IO_Response: sw1=0x%X sw2=0x%X data=%s\n",
                    ril_response.sw1, ril_response.sw2,
                    ril_response.simResponse != NULL ? ril_response.simResponse : "NULL");

  /* No need to call qcril_uim_remove_non_provisioning_session since we do not try
     to open one in the request, continue to generate response */
  qcril_uim_response(original_request_ptr->instance_id,
                     token,
                     ril_err,
                     &ril_response,
                     sizeof(RIL_SIM_IO_Response),
                     TRUE,
                     NULL);

  if (ril_response.simResponse)
  {
    qcril_free(ril_response.simResponse);
    ril_response.simResponse = NULL;
  }

  /* Free memory allocated originally in the request */
  qcril_free(original_request_ptr);
  original_request_ptr = NULL;

} /* qcril_uim_send_apdu_resp */


/*===========================================================================

  FUNCTION:  qcril_uim_process_send_apdu_ind

===========================================================================*/
/*!
    @brief
    Function for processing send APDU indication. Based on the data received
    in the send APDU response, this routine is responsible for concatenating
    all the chunks of the APDU indication & preparing & sending one long APDU
    to the client.

    @return
    None.
*/
/*=========================================================================*/
void qcril_uim_process_send_apdu_ind
(
  const qcril_uim_indication_params_type  * ind_param_ptr,
  qcril_request_return_type               * const ret_ptr /*!< Output parameter */
)
{
  uint16                            remaining_len = 0;
  uint16                            stored_len    = 0;
  boolean                           send_response = FALSE;
  qmi_uim_send_apdu_ind_type      * apdu_ind_ptr  = NULL;
  uint8                             slot = QCRIL_UIM_INVALID_SLOT_INDEX_VALUE;

  if(ind_param_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL pointer");
    QCRIL_ASSERT(0);
    return;
  }

  /* Fetch slot info */
  slot = qcril_uim_instance_id_to_slot(ind_param_ptr->instance_id);
  if(slot >= QMI_UIM_MAX_CARD_COUNT)
  {
    QCRIL_LOG_ERROR("Invalid slot %d, cannot store IND", slot);
    QCRIL_ASSERT(0);
    return;
  }

  apdu_ind_ptr = (qmi_uim_send_apdu_ind_type*)&ind_param_ptr->ind_data.send_apdu_ind;

  QCRIL_LOG_INFO("Send APDU Indication - token: 0x%x, total_len: 0x%x, offset: 0x%x, data_len: 0x%x",
                 apdu_ind_ptr->token,
                 apdu_ind_ptr->total_len,
                 apdu_ind_ptr->offset,
                 apdu_ind_ptr->apdu.data_len);

  if (qcril_uim.long_apdu_info[slot].in_use == TRUE)
  {
    /* If Response already came, we need to check incoming info */
    if ((qcril_uim.long_apdu_info[slot].token          != apdu_ind_ptr->token) ||
        (qcril_uim.long_apdu_info[slot].total_apdu_len != apdu_ind_ptr->total_len))
    {
      /* Error condition - mismatch in data, discrd the response */
      QCRIL_LOG_ERROR("Mismatch with global data, token: 0x%x, total_apdu_len: 0x%x",
                    qcril_uim.long_apdu_info[slot].token,
                    qcril_uim.long_apdu_info[slot].total_apdu_len);
      qcril_uim_cleanup_long_apdu_info(slot);
      return;
    }
  }
  else
  {
    /* Response hasn't come yet, we can still store IND info */
    QCRIL_LOG_INFO("long_apdu_info[%d].in_use is FALSE, storing info", slot);
    qcril_uim.long_apdu_info[slot].in_use         = TRUE;
    qcril_uim.long_apdu_info[slot].token          = apdu_ind_ptr->token;
    qcril_uim.long_apdu_info[slot].total_apdu_len = apdu_ind_ptr->total_len;
  }

  /* If this is the first chunk, allocate the buffer. This buffer will
     only be freed at the end of the receiving all the INDs */
  if (qcril_uim.long_apdu_info[slot].apdu_ptr == NULL)
  {
    qcril_uim.long_apdu_info[slot].rx_len = 0;
    qcril_uim.long_apdu_info[slot].apdu_ptr = qcril_malloc(apdu_ind_ptr->total_len);
    if (qcril_uim.long_apdu_info[slot].apdu_ptr == NULL)
    {
      QCRIL_LOG_ERROR("%s", "Couldnt allocate apdu_ptr pointer !");
      qcril_uim.long_apdu_info[slot].in_use = FALSE;
      return;
    }
  }

  /* Find out the remaining APDU buffer length */
  stored_len    = qcril_uim.long_apdu_info[slot].rx_len;
  remaining_len = qcril_uim.long_apdu_info[slot].total_apdu_len - stored_len;

  /* If this chunk cannot fit in our global buffer, discard the IND */
  if ((apdu_ind_ptr->apdu.data_len > remaining_len) ||
      (apdu_ind_ptr->offset  >= qcril_uim.long_apdu_info[slot].total_apdu_len) ||
      ((apdu_ind_ptr->offset + apdu_ind_ptr->apdu.data_len) > qcril_uim.long_apdu_info[slot].total_apdu_len))
  {
    QCRIL_LOG_ERROR("Mismatch with global data, total_apdu_len: 0x%x stored_len: 0x%x, remaining_len: 0x%x",
                    qcril_uim.long_apdu_info[slot].total_apdu_len,
                    stored_len,
                    remaining_len);
    qcril_uim.long_apdu_info[slot].in_use = FALSE;
    return;
  }

  /* Save the data & update the data length */
  memcpy(qcril_uim.long_apdu_info[slot].apdu_ptr + apdu_ind_ptr->offset,
         apdu_ind_ptr->apdu.data_ptr,
         apdu_ind_ptr->apdu.data_len);
  qcril_uim.long_apdu_info[slot].rx_len += apdu_ind_ptr->apdu.data_len;

  /* If it is the last one, send the response back & clean up global buffer */
  if (qcril_uim.long_apdu_info[slot].total_apdu_len == qcril_uim.long_apdu_info[slot].rx_len)
  {
    if (qcril_uim.long_apdu_info[slot].original_request_ptr != NULL)
    {
      RIL_Token           token = qcril_uim.long_apdu_info[slot].original_request_ptr->token;
      unsigned short      apdu_len = qcril_uim.long_apdu_info[slot].rx_len;
      RIL_SIM_IO_Response ril_response;

      QCRIL_LOG_DEBUG("qcril_uim_process_send_apdu_ind: token=0x%x apdu_len=0x%x",
                       qcril_log_get_token_id(token),
                       apdu_len);

      memset(&ril_response, 0, sizeof(RIL_SIM_IO_Response));
      ril_response.sw1 = qcril_uim.long_apdu_info[slot].apdu_ptr[apdu_len-2];
      ril_response.sw2 = qcril_uim.long_apdu_info[slot].apdu_ptr[apdu_len-1];
      ril_response.simResponse = qcril_uim_alloc_bin_to_hexstring(
                                   qcril_uim.long_apdu_info[slot].apdu_ptr,
                                   apdu_len - 2);

      QCRIL_LOG_DEBUG( "RIL_SIM_IO_Response: sw1=0x%X sw2=0x%X data=%s",
                       ril_response.sw1, ril_response.sw2,
                       ril_response.simResponse != NULL ? ril_response.simResponse : "NULL");

      /* Generate long APDU response */
      qcril_uim_response(qcril_uim.long_apdu_info[slot].original_request_ptr->instance_id,
                         token,
                         RIL_E_SUCCESS,
                         &ril_response,
                         sizeof(RIL_SIM_IO_Response),
                         TRUE,
                         NULL);

      if (ril_response.simResponse)
      {
        qcril_free(ril_response.simResponse);
        ril_response.simResponse = NULL;
      }
      /* Free memory allocated originally in the request and set the pointer to
         NULL so that error response is not sent again in the cleanup routine */
      qcril_free(qcril_uim.long_apdu_info[slot].original_request_ptr);
      qcril_uim.long_apdu_info[slot].original_request_ptr = NULL;
    }
    /* Also clean up the global APDU data */
    qcril_uim_cleanup_long_apdu_info(slot);
  }
} /* qcril_uim_process_send_apdu_ind */
#endif /* FEATURE_QCRIL_UIM_QMI_APDU_ACCESS */


/*===========================================================================

  FUNCTION:  qcril_uim_cleanup_long_apdu_info

===========================================================================*/
/*!
    @brief
    Frees if any memory is allocated in global APDU info structure. It also
    sends an error response in case the original request pointer is still
    pending.

    @return
    None.
*/
/*=========================================================================*/
void qcril_uim_cleanup_long_apdu_info
(
  uint8       slot_index
)
{
  if (slot_index >= QMI_UIM_MAX_CARD_COUNT)
  {
    QCRIL_LOG_ERROR("Invalid slot_index %d", slot_index);
    return;
  }

  /* Free any internal buffers we might have */
  if (qcril_uim.long_apdu_info[slot_index].apdu_ptr != NULL)
  {
    qcril_free(qcril_uim.long_apdu_info[slot_index].apdu_ptr);
    qcril_uim.long_apdu_info[slot_index].apdu_ptr = NULL;
  }
  /* Send error response if request is still pending */
  if (qcril_uim.long_apdu_info[slot_index].original_request_ptr != NULL)
  {
    RIL_SIM_IO_Response ril_response;

    memset(&ril_response, 0, sizeof(RIL_SIM_IO_Response));

    QCRIL_LOG_DEBUG( "RIL_SIM_IO_Response: sw1=0x%X sw2=0x%X data=%s",
                       ril_response.sw1, ril_response.sw2,
                       ril_response.simResponse != NULL ? ril_response.simResponse : "NULL");

    /* Generate response */
    qcril_uim_response(qcril_uim.long_apdu_info[slot_index].original_request_ptr->instance_id,
                       qcril_uim.long_apdu_info[slot_index].original_request_ptr->token,
                       RIL_E_GENERIC_FAILURE,
                       &ril_response,
                       sizeof(RIL_SIM_IO_Response),
                       TRUE,
                       NULL);

    qcril_free(qcril_uim.long_apdu_info[slot_index].original_request_ptr);
    qcril_uim.long_apdu_info[slot_index].original_request_ptr = NULL;
  }

  memset(&qcril_uim.long_apdu_info[slot_index],
         0,
         sizeof(qcril_uim_long_apdu_info_type));
} /* qcril_uim_cleanup_long_apdu_info */


/*=========================================================================

  FUNCTION:  qcril_uim_isim_authenticate_resp

===========================================================================*/
/*!
    @brief
    Process the response for ISIM authenticate command.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_isim_authenticate_resp
(
  const qcril_uim_callback_params_type * const params_ptr
)
{
  RIL_Token                         token         = 0;
  RIL_Errno                         ril_err       = RIL_E_GENERIC_FAILURE;
  char                            * auth_resp_ptr = NULL;
  qcril_uim_original_request_type * original_request_ptr = NULL;

  if(params_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL params_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  /* Retrieve original request */
  original_request_ptr = (qcril_uim_original_request_type*)params_ptr->orig_req_data;
  if(original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL original_request_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  ril_err = (params_ptr->qmi_rsp_data.qmi_err_code == 0) ? RIL_E_SUCCESS : RIL_E_GENERIC_FAILURE;
  token   = (RIL_Token)original_request_ptr->token;

  QCRIL_LOG_DEBUG( "qcril_isim_authenticate_resp: token=%d, qmi_err_code=%d\n",
                    qcril_log_get_token_id(token),
                    params_ptr->qmi_rsp_data.qmi_err_code);

  if (ril_err == RIL_E_SUCCESS)
  {
    auth_resp_ptr = qcril_uim_alloc_bin_to_base64string(
                      params_ptr->qmi_rsp_data.rsp_data.authenticate_rsp.auth_response.data_ptr,
                      params_ptr->qmi_rsp_data.rsp_data.authenticate_rsp.auth_response.data_len);
    ril_err = (auth_resp_ptr == NULL) ? RIL_E_GENERIC_FAILURE : RIL_E_SUCCESS;
  }

  /* No need to call qcril_uim_remove_non_provisioning_session since this
     response happens only for a prov session, continue to generate response */
  qcril_uim_response(original_request_ptr->instance_id,
                     token,
                     ril_err,
                     auth_resp_ptr,
                     (auth_resp_ptr == NULL) ? 0 : strlen((const char *)auth_resp_ptr),
                     TRUE,
                     NULL);

  if (auth_resp_ptr)
  {
    qcril_free(auth_resp_ptr);
    auth_resp_ptr = NULL;
  }

  /* Free memory allocated originally in the request */
  qcril_free(original_request_ptr);
  original_request_ptr = NULL;
} /* qcril_uim_isim_authenticate_resp */


/*===========================================================================

                               QCRIL INTERFACE FUNCTIONS

===========================================================================*/

/*=========================================================================

  FUNCTION:  qcril_uim_request_sim_io

===========================================================================*/
/*!
    @brief
    Handler for RIL_REQUEST_SIM_IO.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_request_sim_io
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type       *const ret_ptr /*!< Output parameter */
)
{
  const RIL_SIM_IO_v6* request_io_ptr = NULL;

  if(params_ptr == NULL || ret_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, cannot process response");
    QCRIL_ASSERT(0);
    return;
  }

  QCRIL_LOG_INFO( "%s\n", __FUNCTION__);

  request_io_ptr = (const RIL_SIM_IO_v6*)params_ptr->data;
  if(request_io_ptr == NULL)
  {
    qcril_uim_response(params_ptr->instance_id,
                       params_ptr->t,
                       RIL_E_GENERIC_FAILURE,
                       NULL,
                       0,
                       TRUE,
                       "NULL request_io_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  QCRIL_LOG_INFO( "qcril_uim_request_sim_io(aid: %s)\n",
                  request_io_ptr->aidPtr != NULL ? request_io_ptr->aidPtr : "NULL");

  QCRIL_LOG_INFO( "qcril_uim_request_sim_io(%d, 0x%X, %s, %d, %d, %d, %s, %s)\n",
                  request_io_ptr->command,
                  request_io_ptr->fileid,
                  request_io_ptr->path != NULL ? request_io_ptr->path : "NULL",
                  request_io_ptr->p1,
                  request_io_ptr->p2,
                  request_io_ptr->p3,
                  request_io_ptr->data != NULL ? request_io_ptr->data : "NULL",
                  request_io_ptr->pin2 != NULL ? request_io_ptr->pin2 : "NULL");

  /* Add entry to ReqList */
  QCRIL_UIM_ADD_ENTRY_TO_REQUEST_LIST(params_ptr);

  switch (request_io_ptr->command)
  {
     case SIM_CMD_READ_BINARY:
      qcril_uim_read_binary(params_ptr->instance_id, params_ptr->t, request_io_ptr);
      break;

    case SIM_CMD_READ_RECORD:
      qcril_uim_read_record(params_ptr->instance_id, params_ptr->t, request_io_ptr);
      break;

    case SIM_CMD_GET_RESPONSE:
      qcril_uim_get_response(params_ptr->instance_id, params_ptr->t, request_io_ptr);
      break;

    case SIM_CMD_UPDATE_BINARY:
      qcril_uim_update_binary(params_ptr->instance_id, params_ptr->t, request_io_ptr);
      break;

    case SIM_CMD_RETRIEVE_DATA:
      /* Not implemented */
      QCRIL_LOG_ERROR( "%s", "NOTIMPL: qcril_uim_request_sim_io SIM_CMD_RETRIEVE_DATA\n");
      qcril_uim_response(params_ptr->instance_id, params_ptr->t,
                         RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0, TRUE, NULL);
      break;

    case SIM_CMD_SET_DATA:
      /* Not implemented */
      QCRIL_LOG_ERROR( "%s", "NOTIMPL: qcril_uim_request_sim_io SIM_CMD_SET_DATA\n");
      qcril_uim_response(params_ptr->instance_id, params_ptr->t,
                         RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0, TRUE, NULL);
      break;

    case SIM_CMD_UPDATE_RECORD:
      qcril_uim_update_record(params_ptr->instance_id, params_ptr->t, request_io_ptr);
      break;

    case SIM_CMD_STATUS:
      qcril_uim_get_status(params_ptr->instance_id, params_ptr->t, request_io_ptr);
      break;

    default:
      QCRIL_LOG_ERROR( "%s", "ILLEGAL: qcril_uim_request_sim_io unknown cmd\n");
      qcril_uim_response(params_ptr->instance_id, params_ptr->t,
                         RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0, TRUE, NULL);
      break;
  }
} /* qcril_uim_request_sim_io() */


/*===========================================================================

  FUNCTION:  qcril_uim_request_get_imsi

===========================================================================*/
/*!
    @brief
    Handler for RIL_REQUEST_GET_IMSI.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_request_get_imsi
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type       *const ret_ptr /*!< Output parameter */
)
{
  qcril_modem_id_e_type                 modem_id = QCRIL_MAX_MODEM_ID - 1;
  uint8                              ** in_ptr    = NULL;
  qmi_uim_read_transparent_params_type  read_params;
  RIL_Errno                             err;
  uint16                                aid_size = 0;
  uint8                                 path[3][4] =
                                        {
                                          {0x3F, 0x00, 0x7F, 0x20}, /* DF GSM        */
                                          {0x3F, 0x00, 0x7F, 0x25}, /* DF CDMA       */
                                          {0x3F, 0x00, 0x7F, 0xFF}  /* ADF USIM/CSIM */
                                        };
  uint16                                first_level_df_path = 0;
  qcril_uim_original_request_type     * original_request_ptr = NULL;
  uint8                                 slot = QCRIL_UIM_INVALID_SLOT_INDEX_VALUE;
#ifndef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  qcril_uim_callback_params_type        callback_params;
#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

  if(params_ptr == NULL || ret_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, cannot process response");
    QCRIL_ASSERT(0);
    return;
  }

  in_ptr = (uint8 **)(params_ptr->data);
  if(in_ptr == NULL)
  {
    qcril_uim_response(params_ptr->instance_id,
                       params_ptr->t,
                       RIL_E_GENERIC_FAILURE,
                       NULL,
                       0,
                       TRUE,
                       "NULL pointer");
    QCRIL_ASSERT(0);
    return;
  }

  memset(&read_params, 0, sizeof(qmi_uim_read_transparent_params_type));

  QCRIL_LOG_INFO( "qcril_uim_request_get_imsi(%s)\n",
                  (in_ptr[0] != NULL) ? (const char *)in_ptr[0] : "NULL" );

  /* Fetch slot info */
  slot = qcril_uim_instance_id_to_slot(params_ptr->instance_id);
  if(slot >= QMI_UIM_MAX_CARD_COUNT)
  {
    QCRIL_LOG_ERROR("invalid slot value 0x%x", slot);
    qcril_uim_response(params_ptr->instance_id,
                       params_ptr->t,
                       RIL_E_GENERIC_FAILURE,
                       NULL,
                       0,
                       TRUE,
                       NULL);
    QCRIL_ASSERT(0);
    return;
  }

  /* Add entry to ReqList */
  QCRIL_UIM_ADD_ENTRY_TO_REQUEST_LIST(params_ptr);

  /* Update the file path based on passed aid pointer */
  aid_size = (in_ptr[0] == NULL) ? 0 : strlen((const char *)in_ptr[0]);
  if (aid_size == 0)
  {
    uint8 index = 0;

    /* If SIM App is present we always fetch IMSI from DF GSM */
    if (qcril_uim_find_app_in_slot(slot, QMI_UIM_APP_SIM))
    {
      read_params.file_id.path.data_ptr = (unsigned char*)path[0];
      first_level_df_path = QCRIL_UIM_FILEID_DF_GSM;
    }
    /* if RUIM app is the only one present (& no SIM App), we fetch IMSI_M from DF CDMA */
    else if (qcril_uim_find_app_in_slot(slot, QMI_UIM_APP_RUIM))
    {
      read_params.file_id.path.data_ptr = (unsigned char*)path[1];
      first_level_df_path = QCRIL_UIM_FILEID_DF_CDMA;
    }
  }
  else
  {
    read_params.file_id.path.data_ptr = (unsigned char*)path[2];
    first_level_df_path = QCRIL_UIM_FILEID_ADF_USIM_CSIM;
  }

  /* Extract session information */
  err = qcril_uim_extract_session_type(slot,
                                       (const char *)in_ptr[0],
                                       first_level_df_path,
                                       &read_params.session_info,
                                       NULL,
                                       0);
  if (err != RIL_E_SUCCESS)
  {
    QCRIL_LOG_ERROR("%s", "error processing input params in RIL_REQUEST_GET_IMSI");
    qcril_uim_response(params_ptr->instance_id, params_ptr->t, err, NULL, 0,
                       TRUE, NULL);
    return;
  }

  QCRIL_LOG_INFO( "session_type for RIL_REQUEST_GET_IMSI: %d\n",
                  read_params.session_info.session_type );

  /* Check session type & update file_id */
  if((read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_PRI_GW_PROV) ||
     (read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_SEC_GW_PROV) ||
     (read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_TER_GW_PROV))
  {
    read_params.file_id.file_id = QCRIL_UIM_FILEID_EF_IMSI;
  }
  else if ((read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_PRI_1X_PROV) ||
           (read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_SEC_1X_PROV) ||
           (read_params.session_info.session_type == QMI_UIM_SESSION_TYPE_TER_1X_PROV))
  {
    read_params.file_id.file_id = QCRIL_UIM_FILEID_EF_IMSI_M;
  }
  else
  {
    QCRIL_LOG_ERROR("%s", "Not proper session type for RIL_REQUEST_GET_IMSI");
    goto send_imsi_error;
  }

  /* Read parameters: length and offset */
  read_params.file_id.path.data_len = QCRIL_UIM_IMSI_PATH_SIZE;
  read_params.offset = 0;
  read_params.length = 0;

  /* Allocate original request, it is freed in qmi_uim_callback */
  original_request_ptr = qcril_uim_allocate_orig_request(params_ptr->instance_id,
                                                         modem_id,
                                                         params_ptr->t,
                                                         params_ptr->event_id,
                                                         read_params.session_info.session_type);
  if (original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "error allocating memory for callback_request_ptr!");
    goto send_imsi_error;
  }

  /* Proceed with read transparent  */
  QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "read transparent" );
#ifdef FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
  if (qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_READ_TRANSPARENT,
                                   qcril_uim.qmi_handle,
                                   &read_params,
                                   qmi_uim_callback,
                                   (void*)original_request_ptr) >= 0)
  {
    return;
  }
#else
  memset(&callback_params, 0, sizeof(qcril_uim_callback_params_type));

  if (qcril_qmi_uim_read_transparent(qcril_uim.qmi_handle,
                                     &read_params,
                                     NULL,
                                     (void*)original_request_ptr,
                                     &callback_params.qmi_rsp_data) >= 0)
  {
    callback_params.orig_req_data = original_request_ptr;
    qcril_uim_get_imsi_resp(&callback_params);

    /* Client needs to free the memory for raw data */
    if(callback_params.qmi_rsp_data.rsp_data.read_transparent_rsp.content.data_ptr)
    {
      qcril_free(callback_params.qmi_rsp_data.rsp_data.read_transparent_rsp.content.data_ptr);
    }
    return;
  }
#endif /* FEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL */

send_imsi_error:
  qcril_uim_response(params_ptr->instance_id, params_ptr->t, RIL_E_GENERIC_FAILURE,
                     NULL, 0, TRUE, "error in qcril_qmi_uim_read_transparent");
  /* Clean up any original request if allocated */
  if (original_request_ptr)
  {
    qcril_free(original_request_ptr);
    original_request_ptr = NULL;
  }

} /* qcril_uim_request_get_imsi */


/*===========================================================================

  FUNCTION:  qcril_uim_request_isim_authenticate

===========================================================================*/
/*!
    @brief
    Handler for RIL_REQUEST_ISIM_AUTHENTICATION.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_request_isim_authenticate
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type       *const ret_ptr /*!< Output parameter */
)
{
  qcril_modem_id_e_type                 modem_id             = QCRIL_MAX_MODEM_ID - 1;
  const char                          * in_ptr               = NULL;
  uint8                                 slot                 = 0;
  uint8                                 isim_index           = 0;
  uint16                                aid_size             = 0;
  uint8                                 aid_buffer[QMI_UIM_MAX_AID_LEN];
  qcril_uim_original_request_type     * original_request_ptr = NULL;
  qmi_uim_authenticate_params_type      auth_params;

  if(params_ptr == NULL || ret_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, cannot process response");
    QCRIL_ASSERT(0);
    return;
  }

  memset(&auth_params, 0, sizeof(qmi_uim_authenticate_params_type));

  /* Add entry to ReqList */
  QCRIL_UIM_ADD_ENTRY_TO_REQUEST_LIST(params_ptr);

  in_ptr = params_ptr->data;

  QCRIL_LOG_INFO( "qcril_uim_request_isim_authenticate(data: %s)\n", in_ptr != NULL ? in_ptr : "NULL");

  /* Return with error if data was not provided */
  if ((in_ptr == NULL) || (strlen(in_ptr) == 0))
  {
    QCRIL_LOG_ERROR( "%s", " Invalid input for data \n");
    goto isim_auth_error;
  }

  /* Current RIL interface doesnt provide the ISIM AID, so this is what we do:
     Based on the RIL instance, we check if ISIM App is present in that slot
     and use a non-prov session type of that slot */
  slot   = qcril_uim_instance_id_to_slot(params_ptr->instance_id);
  switch(slot)
  {
    case 0:
      auth_params.session_info.session_type = QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_1;
      break;

    case 1:
      auth_params.session_info.session_type = QMI_UIM_SESSION_TYPE_NON_PROV_SLOT_2;
      break;

    default:
      QCRIL_LOG_ERROR("Invalid qmi_uim slot id: 0x%X", slot);
      goto isim_auth_error;
  }

  if (qcril_uim_extract_isim_index(&isim_index, slot) != RIL_E_SUCCESS)
  {
    QCRIL_LOG_ERROR( "ISIM app not found for instance_id: 0x%x, slot: 0x%x",
                     params_ptr->instance_id, slot);
    goto isim_auth_error;
  }

  /* Update ISIM AID */
  auth_params.session_info.aid.data_len =
    qcril_uim.card_status.card[slot].application[isim_index].aid_len;
  auth_params.session_info.aid.data_ptr =
    (unsigned char *)qcril_uim.card_status.card[slot].application[isim_index].aid_value;

  /* Update auth parameters */
  auth_params.auth_context       = QMI_UIM_AUTH_CONTEXT_IMS_AKA_SECURITY;
  auth_params.auth_data.data_ptr = qcril_uim_alloc_base64string_to_bin(
                                     in_ptr,
                                     &auth_params.auth_data.data_len);
  if (auth_params.auth_data.data_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Unable to convert input ISIM auth data!");
    goto isim_auth_error;
  }

  /* Allocate original request, it is freed in qmi_uim_callback */
  original_request_ptr = qcril_uim_allocate_orig_request(params_ptr->instance_id,
                                                         modem_id,
                                                         params_ptr->t,
                                                         params_ptr->event_id,
                                                         auth_params.session_info.session_type);
  if (original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "error allocating memory for original_request_ptr!");
    goto isim_auth_error;
  }

  /* Proceed with logical channel request */
  QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "authenticate" );
  if (qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_AUTHENTICATE,
                                   qcril_uim.qmi_handle,
                                   &auth_params,
                                   qmi_uim_callback,
                                   (void*)original_request_ptr) >= 0)
  {
    /* For success case, free AID & data buffer & return */

    qcril_free(auth_params.auth_data.data_ptr);
    auth_params.auth_data.data_ptr = NULL;
    return;
  }

isim_auth_error:
  qcril_uim_response(params_ptr->instance_id, params_ptr->t, RIL_E_GENERIC_FAILURE,
                       NULL, 0, TRUE, "error in qcril_qmi_uim_authentication");

  /* Clean up any original request if allocated */
  if (original_request_ptr)
  {
    qcril_free(original_request_ptr);
    original_request_ptr = NULL;
  }

  /* Free data buffer that was allocated */
  if (auth_params.auth_data.data_ptr)
  {
    qcril_free(auth_params.auth_data.data_ptr);
    auth_params.auth_data.data_ptr = NULL;
  }
} /* qcril_uim_request_isim_authenticate */


#ifdef FEATURE_QCRIL_UIM_QMI_APDU_ACCESS
/*===========================================================================

  FUNCTION:  qcril_uim_request_logical_channel

===========================================================================*/
/*!
    @brief
    Handles RIL_REQUEST_SIM_OPEN_CHANNEL or RIL_REQUEST_SIM_CLOSE_CHANNEL
    request from QCRIL.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_request_logical_channel
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type       *const ret_ptr /*!< Output parameter */
)
{
  qcril_modem_id_e_type                 modem_id             = QCRIL_MAX_MODEM_ID - 1;
  uint16                                aid_size             = 0;
  uint8                                 aid_buffer[QMI_UIM_MAX_AID_LEN];
  qcril_uim_original_request_type     * original_request_ptr = NULL;
  qmi_uim_logical_channel_params_type   logical_channel_params;

  if(params_ptr == NULL || ret_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, cannot process response");
    QCRIL_ASSERT(0);
    return;
  }

  memset(&logical_channel_params, 0, sizeof(qmi_uim_logical_channel_params_type));

  /* Add entry to ReqList */
  QCRIL_UIM_ADD_ENTRY_TO_REQUEST_LIST(params_ptr);

  /* Parse input info based on the request type */
  if (params_ptr->event_id == RIL_REQUEST_SIM_OPEN_CHANNEL)
  {
    uint8        slot   = qcril_uim_instance_id_to_slot(params_ptr->instance_id);
    const char * in_ptr = NULL;
    in_ptr = params_ptr->data;
    QCRIL_LOG_INFO( "qcril_uim_request_logical_channel(aid: %s)\n", in_ptr != NULL ? in_ptr : "NULL");

    /* Return with error if AID was not provided */
    if ((in_ptr == NULL) || (strlen(in_ptr) == 0))
    {
      QCRIL_LOG_ERROR( "%s", " Invalid input for AID \n");
      goto logical_channel_error;
    }

    logical_channel_params.operation_type = QMI_UIM_LOGICAL_CHANNEL_OPEN;

    /* Convert AID string into binary */
    aid_size = qcril_uim_hexstring_to_bin(in_ptr,
                                          aid_buffer,
                                          QMI_UIM_MAX_AID_LEN);
    if (aid_size == 0)
    {
      QCRIL_LOG_ERROR("%s", "Error converting AID string into binary");
      goto logical_channel_error;
    }

    /* Update AID info */
    logical_channel_params.channel_data.aid.data_ptr = (unsigned char*)aid_buffer;
    logical_channel_params.channel_data.aid.data_len = (unsigned short)aid_size;

    /* If it is an ICC card, ask for FCI template when opening the channel */
    if (qcril_uim_find_app_in_slot(slot, QMI_UIM_APP_SIM) ||
        qcril_uim_find_app_in_slot(slot, QMI_UIM_APP_RUIM))
    {
      logical_channel_params.file_control_information.is_valid  = QMI_UIM_TRUE;
      logical_channel_params.file_control_information.fci_value = QMI_UIM_FCI_VALUE_FCP;
    }
  }
  else if (params_ptr->event_id == RIL_REQUEST_SIM_CLOSE_CHANNEL)
  {
    const int * in_ptr = NULL;
    in_ptr = params_ptr->data;

    /* Return with error if input pointer was not provided */
    if (in_ptr == NULL)
    {
      QCRIL_LOG_ERROR( "%s", " Invalid input pointer \n");
      goto logical_channel_error;
    }

    QCRIL_LOG_INFO( "qcril_uim_request_logical_channel(channel id: 0x%X)\n", *in_ptr);

    logical_channel_params.operation_type = QMI_UIM_LOGICAL_CHANNEL_CLOSE;
    logical_channel_params.channel_data.channel_id = *in_ptr;
  }
  else
  {
    QCRIL_LOG_ERROR( " Invalid input event_id: 0x%x\n", params_ptr->event_id);
    goto logical_channel_error;
  }

  /* Find slot info */
  if ( ril_to_uim_is_tsts_enabled() && (params_ptr->instance_id == QCRIL_THIRD_INSTANCE_ID) )
  {
    logical_channel_params.slot = QMI_UIM_SLOT_3;
  }
  else if ( (ril_to_uim_is_tsts_enabled() ||
             ril_to_uim_is_dsds_enabled()) &&
            (params_ptr->instance_id == QCRIL_SECOND_INSTANCE_ID) )
  {
    logical_channel_params.slot = QMI_UIM_SLOT_2;
  }
  else if (params_ptr->instance_id == QCRIL_DEFAULT_INSTANCE_ID)
  {
    logical_channel_params.slot = QMI_UIM_SLOT_1;
  }
  else
  {
    QCRIL_LOG_ERROR( " Invalid instance_id in input: 0x%x\n", params_ptr->instance_id);
    goto logical_channel_error;
  }

  /* Allocate original request, it is freed in qmi_uim_callback */
  original_request_ptr = qcril_uim_allocate_orig_request(params_ptr->instance_id,
                                                         modem_id,
                                                         params_ptr->t,
                                                         params_ptr->event_id,
                                                         0);
  if (original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "error allocating memory for original_request_ptr!");
    goto logical_channel_error;
  }

  /* Proceed with logical channel request */
  QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "logical channel" );
  if (qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_LOGICAL_CHANNEL,
                                   qcril_uim.qmi_handle,
                                   &logical_channel_params,
                                   qmi_uim_callback,
                                   (void*)original_request_ptr) >= 0)
  {
    return;
  }

logical_channel_error:
  qcril_uim_response(params_ptr->instance_id, params_ptr->t, RIL_E_GENERIC_FAILURE,
                       NULL, 0, TRUE, "error in qcril_qmi_uim_logical_channel");

  /* Clean up any original request if allocated */
  if (original_request_ptr)
  {
    qcril_free(original_request_ptr);
    original_request_ptr = NULL;
  }
} /* qcril_uim_request_logical_channel */


/*===========================================================================

  FUNCTION:  qcril_uim_request_send_apdu

===========================================================================*/
/*!
    @brief
    Handles RIL_REQUEST_SIM_APDU or RIL_REQUEST_SIM_TRANSMIT_CHANNEL
    request from QCRIL.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_request_send_apdu
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type       *const ret_ptr /*!< Output parameter */
)
{
  qcril_modem_id_e_type             modem_id             = QCRIL_MAX_MODEM_ID - 1;
  const RIL_SIM_IO_v6             * request_io_ptr       = NULL;
  uint16                            in_apdu_length       = 0;
  qcril_uim_original_request_type * original_request_ptr = NULL;
  qmi_uim_send_apdu_params_type     send_apdu_params;

  if(params_ptr == NULL || ret_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, cannot process response");
    QCRIL_ASSERT(0);
    return;
  }

  request_io_ptr = (const RIL_SIM_IO_v6*)params_ptr->data;
  if(request_io_ptr == NULL)
  {
    qcril_uim_response(params_ptr->instance_id,
                       params_ptr->t,
                       RIL_E_GENERIC_FAILURE,
                       NULL,
                       0,
                       TRUE,
                       "NULL request_io_ptr");
    QCRIL_ASSERT(0);
    return;
  }


  /* Note - An ADPU is constructed from input RIL_SIM_IO_v6 as follows:
     CLA + INS + P1 + P2 + P3 + data
     Mapping for these in input RIL_SIM_IO_v6:
     channel_id: fileid, CLA: cla, INS: command, P1: p1, P2: p2, P3: p3, data: data
     Only RIL_REQUEST_SIM_TRANSMIT_CHANNEL has the valid channel_id */
  QCRIL_LOG_INFO( "qcril_uim_request_sim_io(0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, %s)\n",
                  request_io_ptr->cla,
                  request_io_ptr->command,
                  request_io_ptr->fileid,
                  request_io_ptr->p1,
                  request_io_ptr->p2,
                  request_io_ptr->p3,
                  request_io_ptr->data != NULL ? request_io_ptr->data : "NULL" );

  memset(&send_apdu_params, 0, sizeof(qmi_uim_send_apdu_params_type));

  /* Add entry to ReqList */
  QCRIL_UIM_ADD_ENTRY_TO_REQUEST_LIST(params_ptr);

  /* Sanity check */
  if ((request_io_ptr->p1      < 0)    || (request_io_ptr->p1      > 0xFF) ||
      (request_io_ptr->p2      < 0)    || (request_io_ptr->p2      > 0xFF) ||
      (request_io_ptr->p3      > 0xFF) ||
      (request_io_ptr->cla     < 0)    || (request_io_ptr->cla     > 0xFF) ||
      (request_io_ptr->command < 0)    || (request_io_ptr->command > 0xFF))
  {
    QCRIL_LOG_ERROR( "Unsupported case, P1: 0x%X, P2: 0x%X, P3: 0x%X, cla: 0x%X, command: 0x%X \n",
                     request_io_ptr->p1, request_io_ptr->p2, request_io_ptr->p3,
                     request_io_ptr->cla, request_io_ptr->command);
    qcril_uim_response(params_ptr->instance_id, params_ptr->t, RIL_E_REQUEST_NOT_SUPPORTED,
                       NULL, 0, TRUE, "error in qcril_qmi_uim_send_apdu");
    return;
  }

  /* Find slot info */
  if ( ril_to_uim_is_tsts_enabled() && (params_ptr->instance_id == QCRIL_THIRD_INSTANCE_ID) )
  {
    send_apdu_params.slot = QMI_UIM_SLOT_3;
  }
  else if ( (ril_to_uim_is_tsts_enabled() ||
             ril_to_uim_is_dsds_enabled()) &&
            (params_ptr->instance_id == QCRIL_SECOND_INSTANCE_ID))
  {
    send_apdu_params.slot = QMI_UIM_SLOT_2;
  }
  else if (params_ptr->instance_id == QCRIL_DEFAULT_INSTANCE_ID)
  {
    send_apdu_params.slot = QMI_UIM_SLOT_1;
  }
  else
  {
    QCRIL_LOG_ERROR( " Invalid instance_id in input: 0x%x\n", params_ptr->instance_id);
    goto send_apdu_error;
  }

  /* If request is RIL_REQUEST_SIM_TRANSMIT_CHANNEL, update channel_id also */
  if (params_ptr->event_id == RIL_REQUEST_SIM_TRANSMIT_CHANNEL)
  {
    send_apdu_params.channel_id         = request_io_ptr->fileid;
    send_apdu_params.channel_id_present = QMI_UIM_TRUE;
  }
  else
  {
    send_apdu_params.channel_id_present = QMI_UIM_FALSE;
  }

  /* Calculate total buffer size for APDU data */
  if ((request_io_ptr->data == NULL) || (strlen(request_io_ptr->data) == 0))
  {
    if (request_io_ptr->p3 < 0)
    {
      in_apdu_length = QCRIL_UIM_APDU_MIN_SIZE;
    }
    else
    {
      in_apdu_length = QCRIL_UIM_APDU_MIN_SIZE_PLUS_P3;
    }
  }
  else
  {
    in_apdu_length = QCRIL_UIM_APDU_MIN_SIZE_PLUS_P3 + strlen(request_io_ptr->data);
  }

  /* Allocate memory and compose the raw APDU data */
  send_apdu_params.apdu.data_ptr = (uint8*) qcril_malloc(in_apdu_length);
  if (send_apdu_params.apdu.data_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Unable to allocate buffer for apdu.data_ptr!");
    goto send_apdu_error;
  }

  send_apdu_params.apdu.data_len = in_apdu_length;

  if (qcril_uim_compose_apdu_data(&send_apdu_params.apdu,
                                  request_io_ptr) == FALSE)
  {
    QCRIL_LOG_ERROR("%s", "Error composing APDU data!");
    goto send_apdu_error;
  }

  /* Allocate original request, it is freed in qmi_uim_callback */
  original_request_ptr = qcril_uim_allocate_orig_request(params_ptr->instance_id,
                                                         modem_id,
                                                         params_ptr->t,
                                                         params_ptr->event_id,
                                                         0);
  if (original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "error allocating memory for original_request_ptr!");
    goto send_apdu_error;
  }

  /* Proceed with logical channel request */
  QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "logical channel" );
  if (qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_SEND_APDU,
                                   qcril_uim.qmi_handle,
                                   &send_apdu_params,
                                   qmi_uim_callback,
                                   (void*)original_request_ptr) >= 0)
  {
    qcril_free(send_apdu_params.apdu.data_ptr);
    send_apdu_params.apdu.data_ptr = NULL;
    return;
  }

send_apdu_error:
  qcril_uim_response(params_ptr->instance_id, params_ptr->t, RIL_E_GENERIC_FAILURE,
                       NULL, 0, TRUE, "error in qcril_qmi_uim_send_apdu");

  /* Clean up any original request if allocated */
  if (original_request_ptr)
  {
    qcril_free(original_request_ptr);
    original_request_ptr = NULL;
  }

  /* Clean up any APDU buffer if allocated */
  if(send_apdu_params.apdu.data_ptr)
  {
    qcril_free(send_apdu_params.apdu.data_ptr);
    send_apdu_params.apdu.data_ptr = NULL;
  }
} /* qcril_uim_request_send_apdu */
#endif /* FEATURE_QCRIL_UIM_QMI_APDU_ACCESS */


/*===========================================================================

  FUNCTION:  qcril_uim_process_internal_verify_pin_command_callback

===========================================================================*/
/*!
    @brief
    Handler for QCRIL_EVT_INTERNAL_UIM_VERIFY_PIN_COMMAND_CALLBACK.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_process_internal_verify_pin_command_callback
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type       *const ret_ptr /*!< Output parameter */
)
{
  qcril_modem_id_e_type                   modem_id = QCRIL_MAX_MODEM_ID - 1;
  int                                     res;
  RIL_Errno                               ril_err  = RIL_E_GENERIC_FAILURE;
  qcril_uim_pin2_original_request_type  * original_request_ptr = NULL;
  qcril_uim_original_request_type       * callback_request_ptr = NULL;

  if(params_ptr == NULL || ret_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "Invalid input, cannot process response");
    QCRIL_ASSERT(0);
    return;
  }

  QCRIL_LOG_INFO( "%s\n", __FUNCTION__);

  original_request_ptr = (qcril_uim_pin2_original_request_type*)params_ptr->data;
  if(original_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "NULL original_request_ptr");
    QCRIL_ASSERT(0);
    return;
  }

  /* In case of PIN error, return immediately */
  if (original_request_ptr->pin2_result != 0)
  {
    int32 num_retries = (int32)original_request_ptr->pin2_num_retries;

    /* Change the error code appropriately */
    switch(original_request_ptr->pin2_result)
    {
      case 12: /* QMI_ERR_INCORRECT_PIN */
      case 36: /* QMI_ERR_PIN_PERM_BLOCKED */
        ril_err = RIL_E_PASSWORD_INCORRECT;
        break;

      case 35: /* QMI_ERR_PIN_BLOCKED */
        ril_err = RIL_E_SIM_PUK2;
        break;

      default:
        ril_err = RIL_E_GENERIC_FAILURE;
        break;
    }

    /* Only for set FDN request, we send number of retries in response. For
       all other requests, we send a simple response */
    if (original_request_ptr->cmd == QCRIL_UIM_ORIG_SET_SERVICE_STATUS_FDN)
    {
      qcril_uim_response(params_ptr->instance_id,
                         (RIL_Token)original_request_ptr->token,
                         ril_err,
                         &num_retries,
                         sizeof(int32),
                         TRUE,
                         "error in pin verification");
    }
    else
    {
      qcril_uim_response(params_ptr->instance_id,
                         (RIL_Token)original_request_ptr->token,
                         ril_err,
                         NULL,
                         0,
                         TRUE,
                         "error in pin verification");
    }

    /* Free memory allocated when the initial command was processed */
    qcril_free(original_request_ptr);
    original_request_ptr = NULL;

    return;
  }

  /* Allocate callback_request_ptr, it is freed in qmi_uim_callback
     Note that session type is populated below for each request type */
  callback_request_ptr = qcril_uim_allocate_orig_request(
                                original_request_ptr->instance_id,
                                original_request_ptr->modem_id,
                                (RIL_Token)original_request_ptr->token,
                                (int)original_request_ptr->cmd,
                                0);
  if (callback_request_ptr == NULL)
  {
    QCRIL_LOG_ERROR("%s", "error allocating memory for callback_request_ptr!");
    return;
  }

  /* Executes the original request */
  switch(original_request_ptr->cmd)
  {
    case QCRIL_UIM_ORIG_SIM_IO_READ_BINARY:
      QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "read transparent" );
      callback_request_ptr->session_type = original_request_ptr->data.read_transparent.session_info.session_type;
      res = qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_READ_TRANSPARENT,
                                         qcril_uim.qmi_handle,
                                         &original_request_ptr->data.read_transparent,
                                         qmi_uim_callback,
                                         (void*)callback_request_ptr);
      break;

    case QCRIL_UIM_ORIG_SIM_IO_READ_RECORD:
      QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "read record" );
      callback_request_ptr->session_type = original_request_ptr->data.read_record.session_info.session_type;
      res = qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_READ_RECORD,
                                         qcril_uim.qmi_handle,
                                         &original_request_ptr->data.read_record,
                                         qmi_uim_callback,
                                         (void*)callback_request_ptr);
      break;

    case QCRIL_UIM_ORIG_SIM_IO_UPDATE_BINARY:
      QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "write transparent" );
      callback_request_ptr->session_type = original_request_ptr->data.write_transparent.session_info.session_type;
      res = qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_WRITE_TRANSPARENT,
                                         qcril_uim.qmi_handle,
                                         &original_request_ptr->data.write_transparent,
                                         qmi_uim_callback,
                                         (void*)callback_request_ptr);
      break;

    case QCRIL_UIM_ORIG_SIM_IO_UPDATE_RECORD:
      QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "write record" );
      callback_request_ptr->session_type = original_request_ptr->data.write_record.session_info.session_type;
      res = qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_WRITE_RECORD,
                                         qcril_uim.qmi_handle,
                                         &original_request_ptr->data.write_record,
                                         qmi_uim_callback,
                                         (void*)callback_request_ptr);
      break;

    case QCRIL_UIM_ORIG_SET_SERVICE_STATUS_FDN:
      QCRIL_LOG_QMI( modem_id, "qmi_uim_service", "set service" );
      callback_request_ptr->data.fdn_status = original_request_ptr->data.set_service_status.fdn_status;
      callback_request_ptr->session_type    = original_request_ptr->data.set_service_status.session_info.session_type;
      res = qcril_uim_queue_send_request(QCRIL_UIM_REQUEST_SET_FDN,
                                         qcril_uim.qmi_handle,
                                         &original_request_ptr->data.set_service_status,
                                         qmi_uim_callback,
                                         (void*)callback_request_ptr);
      break;

    default:
      QCRIL_LOG_ERROR( "%s: Invalid cmd for internal pin2 verification - %d\n",
                        __FUNCTION__, original_request_ptr->cmd);
      res = -1;
      qcril_free(callback_request_ptr);
      callback_request_ptr = NULL;
      break;
  }

  /* In case of error, send the response immediately */
  if (res < 0)
  {
    qcril_uim_response(params_ptr->instance_id, original_request_ptr->token,
                       RIL_E_GENERIC_FAILURE, NULL, 0, TRUE, NULL);
  }

  /* Free memory allocated when the initial command was processed */
  qcril_free(original_request_ptr);
  original_request_ptr = NULL;

} /* qcril_uim_process_internal_verify_pin_command_callback */


#endif /* defined (FEATURE_QCRIL_UIM_QMI) */
