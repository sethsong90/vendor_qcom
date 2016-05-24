#ifndef SNS_SMR_H
#define SNS_SMR_H

/*============================================================================

  @file sns_smr.h

  @brief
  This file contains definition for SMR(Sensor Message Router)

  Copyright (c) 2010-2012 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/

/*===========================================================================

                   EXTERNAL DEFINITIONS AND TYPES

===========================================================================*/
#include  "sns_queue.h"
#include  "sns_common.h"
#include  "sensor1.h"
#include  "qmi_idl_lib.h"
#include  "sns_smr_util.h"
#include  "sns_debug_api.h"

/*============================================================================

                Preprocessor Definitions and Constants

============================================================================*/
#define SMR_MAX_MSG_LEN         4096        /* 4KB */
#define SMR_RSVD_HEADER_LEN     64
#define SMR_MAX_BODY_LEN        (SMR_MAX_MSG_LEN - SMR_RSVD_HEADER_LEN) /* The max body length
                                                     allowed for sns_smr_msg_alloc() */

/*============================================================================

                Macro definition

============================================================================*/
#define SNS_SMR_IS_INTERNAL_MODULE(module) ((((module)&SNS_MODULE_GRP_MASK)==SNS_THIS_MODULE_GRP)?true:false)

/*============================================================================

                        External Variable Declarations

============================================================================*/
/*
 * Each module shall sem_post to this semaphore after module initialization
 * is complete.
 */
extern OS_EVENT *sns_init_sem_ptr;

/*===========================================================================

  FUNCTION:   sns_smr_register

===========================================================================*/
/*!
  @brief This function registers an event handle which will be used to signal to the module
         when a message is arrived.

  @param[i] module_id: The sensor module id which is unique within the sensor framework.
  @param[i] sig_grp_ptr: A pointer to the signal group to which sig_flag is associated
  @param[i] sig_flag: The signal flag that triggers the event

  @return
   - SNS_SUCCESS if the message queues was registered successfully.
   - All other values indicate an error has occurred.

*/
/*=========================================================================*/
sns_err_code_e sns_smr_register(uint8_t module_id, OS_FLAG_GRP *sig_grp_ptr, OS_FLAGS sig_flag);

/*===========================================================================

  FUNCTION:   sns_smr_send

===========================================================================*/
/*!
  @brief This function transfers the message to an appropriate module within the Sensors framework

  @param[i] body_ptr: A pointer to the message body allocated by sns_smr_msg_alloc

  @return
   - SNS_SUCCESS if the message header was gotten successfully.
   - SNS_ERR_WOULDBLOCK if some resource is tentatively unavailable.
   - All other values indicate an error has occurred.

*/
/*=========================================================================*/
sns_err_code_e sns_smr_send(void* body_ptr);

/*===========================================================================

  FUNCTION:   sns_smr_rcv

===========================================================================*/
/*!
  @brief returns a message body from one of the module’s queues

  @param[i] module_id: The module ID that calls this function.

  @return
   NULL if there was no more message in the queues, or a pointer to the message body

*/
/*=========================================================================*/
void* sns_smr_rcv(uint8_t module_id);

/*===========================================================================

  FUNCTION:   sns_smr_get_qmi_max_encode_msg_len

===========================================================================*/
/*!
  @brief This function returns the maximum encoded message length for all services
         which are used in this framework.

  @return
   the maximum encoded message length

*/
/*=========================================================================*/
uint16_t  sns_smr_get_qmi_max_encode_msg_len (void);

/*===========================================================================

  FUNCTION:   sns_smr_close

===========================================================================*/
/*!
  @brief  Close the SMD port

  @param[i] None

  @detail

  @return
   - SNS_SUCCESS if close system call was success.
   - SNS_ERR_FAILED if an error occurred.

*/
/*=========================================================================*/
sns_err_code_e sns_smr_close (void);

#endif /* SNS_SMR_H */
