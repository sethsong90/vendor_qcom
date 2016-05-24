/*============================================================================
  FILE: sns_scm_ext.c

  This file contains the Sensors Calibration Manager implementation

  Copyright (c) 2011 - 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
 ============================================================================*/

/* $Header: //source/qcom/qct/core/pkg/dsps/dev/kaushiks.accelautocal_v1/core/sensors/scm/framework/src/sns_scm_ext.c#1 $ */
/* $DateTime: 2012/06/19 14:40:08 $ */
/* $Author: coresvc $ */

/*============================================================================
  EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order. 

  when        who  what, where, why
  ----------  ---  -----------------------------------------------------------
  2012-07-10  ks   Refactoring SCM into apps and dsps versions
  2012-01-31  ad   Increase registry response timeout to account for higher latencies
                   observed on some customer devices
  2012-01-06  ad   Persistent storage for calibration parameters 
  2011-11-14  jhh  Updated alloc and free function calls to meet new API
  2011-08-30  ad   Add registry support for gyro autocal params  
  2011-09-09  sc   Update with registry service V02
  2011-08-24  ad   Added support for gyro autocal config through registry  
  2011-08-10  ad   Bug fixes for factory cal support 
  2011-08-04  rb   Added support for factory calibration
  2011-07-29  rb   Added support for algorithm enabling registry item
  2011-07-20  ad   Refactor SCM for memory optimization

 ============================================================================*/

/*---------------------------------------------------------------------------
 * Include Files
 * -------------------------------------------------------------------------*/
#include "sns_scm_priv.h"
#include "gyro_cal.h"

#include "sns_init.h"
#include "sns_memmgr.h"
#include "sns_smgr_api_v01.h"
#include "sns_sam_amd_v01.h"
#include "sns_debug_str.h"
#include "fixed_point.h"

#include "sns_reg_common.h"
#include "sns_reg_api_v02.h"

/*---------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * -------------------------------------------------------------------------*/
//Initialization timeout period of 10sec 
#define SNS_SCM_INIT_TIMER_PERIOD_USEC (10000000)

/*---------------------------------------------------------------------------
 * Type Declarations
 * -------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * Global Data Definitions
 * -------------------------------------------------------------------------*/
//Sensor database
static sns_scm_sensor_s* sns_scm_sensor_dbase;

/*---------------------------------------------------------------------------
 * Static Variable Definitions
 * -------------------------------------------------------------------------*/
//SCM event signal
static OS_FLAG_GRP *sns_scm_init_sig_event;

//SCM initialization timer
static sns_em_timer_obj_t sns_scm_init_timer;

/*---------------------------------------------------------------------------
 * Function Definitions
 * -------------------------------------------------------------------------*/
/*=========================================================================
  FUNCTION:  sns_scm_init_timer_cb
  =========================================================================*/
/*!
  @brief Callback registered for initialization timer expiry

  @param[i] argPtr: pointer to argument for callback function

  @return None
*/
/*=======================================================================*/
void sns_scm_init_timer_cb(
   void *argPtr)
{
   uint8_t err;

   UNREFERENCED_PARAMETER(argPtr);

   sns_os_sigs_post(sns_scm_init_sig_event, 
                    SNS_SCM_REPORT_TIMER_SIG, 
                    OS_FLAG_SET, 
                    &err);
   if (err != OS_ERR_NONE)
   {
      SNS_PRINTF_STRING_ID_ERROR_1(SNS_SCM_DBG_MOD,
                                   DBG_SCM_TIMER_CB_SIGNALERR,
                                   err);
   }
}

/*=========================================================================
  FUNCTION:  sns_scm_reg_init_timer
  =========================================================================*/
/*!
  @brief Register timer for for SCM initialization

  @param[i] sigEventFlags: Event signal flags
 
  @return Sensors error code
*/
/*=======================================================================*/
sns_err_code_e sns_scm_reg_init_timer(
   OS_FLAG_GRP *sigEventFlags)
{
   sns_err_code_e err;

   sns_scm_init_sig_event = sigEventFlags;

   err = sns_em_create_timer_obj(sns_scm_init_timer_cb, 
                                 NULL, 
                                 SNS_EM_TIMER_TYPE_ONESHOT,
                                 &(sns_scm_init_timer));
   if (err == SNS_SUCCESS)
   {
      err = sns_em_register_timer(sns_scm_init_timer, 
                                  sns_em_convert_usec_to_localtick(
                                     SNS_SCM_INIT_TIMER_PERIOD_USEC));
      if (err == SNS_SUCCESS)
      {

        SNS_SCM_DEBUG2(MEDIUM, DBG_SCM_REG_TIMER_STARTED,
                       0, SNS_SCM_INIT_TIMER_PERIOD_USEC);
      }
   }
   if (err != SNS_SUCCESS)
   {
      SNS_PRINTF_STRING_ID_ERROR_1(SNS_SCM_DBG_MOD,
                                   DBG_SCM_REG_TIMER_FAILED,
                                   err);
   }
   return err;
}


/*=========================================================================
  FUNCTION:  sns_scm_dereg_init_timer
  =========================================================================*/
/*!
  @brief Deregister timer for SAM initialization

  @return Sensors error code
*/
/*=======================================================================*/
sns_err_code_e sns_scm_dereg_init_timer(void)
{
   sns_err_code_e err;

   err = sns_em_cancel_timer(sns_scm_init_timer);
   if (err != SNS_SUCCESS)
   {
      SNS_PRINTF_STRING_ID_ERROR_1(SNS_SCM_DBG_MOD,
                                   DBG_SCM_DEREG_TIMER_FAILED,
                                   err);
   }

   err = sns_em_delete_timer_obj(sns_scm_init_timer);
   if (err == SNS_SUCCESS)
   {
      sns_scm_init_timer = NULL;

      SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_DEREG_TIMER_DELETED, 
                     0, 
                     0,
                     sns_em_get_timestamp());
   }
   else
   {
     SNS_PRINTF_STRING_ID_ERROR_1(SNS_SCM_DBG_MOD,
                                  DBG_SCM_DEREG_TIMER_FAILED,
                                  err);
   }

   return err;
}

/*=========================================================================
  FUNCTION:  sns_scm_send_cal_msg
  =========================================================================*/
/*!
  @brief Process a factory calibration message and forward data to the SMGR

  @param[i] snsInfoPtr: pointer to sensor info in sensor database
  
  @return None
*/
/*=======================================================================*/
static void sns_scm_send_cal_msg(
   const sns_scm_sensor_s* snsInfoPtr,
   uint8_t calType)
{
   const int32_t *biasCalPtr = NULL;
   const int32_t *scaleCalPtr = NULL;
   sns_err_code_e err;
   sns_smr_header_s msgHdr;
   sns_smgr_sensor_cal_req_msg_v01 *msgPtr;
   uint8_t msgSize = sizeof(sns_smgr_sensor_cal_req_msg_v01);

   msgPtr = (sns_smgr_sensor_cal_req_msg_v01 *)sns_smr_msg_alloc(SNS_SCM_DBG_MOD, msgSize);
   SNS_ASSERT(msgPtr != NULL);

   msgHdr.src_module = SNS_SCM_MODULE;
   msgHdr.svc_num = SNS_SMGR_SVC_ID_V01;
   msgHdr.msg_type = SNS_SMR_MSG_TYPE_REQ;
   msgHdr.priority = SNS_SMR_MSG_PRI_LOW;
   msgHdr.msg_id = SNS_SMGR_CAL_REQ_V01;
   msgHdr.body_len = msgSize;

   msgHdr.txn_id = 0;
   msgHdr.ext_clnt_id = 0;

   msgPtr->usage = calType;
   msgPtr->SensorId = snsInfoPtr->sensorId;
   msgPtr->DataType = SNS_SMGR_DATA_TYPE_PRIMARY_V01;

   //Send bias correction values to SMGR
   msgPtr->ZeroBias_len = 3;
   msgPtr->ScaleFactor_len = 3;

   if (calType == SNS_SMGR_CAL_FACTORY_V01)
   {
      biasCalPtr = snsInfoPtr->fac_cal_bias_params;
      scaleCalPtr = snsInfoPtr->fac_cal_scale_params;
   }
   else if (calType == SNS_SMGR_CAL_DYNAMIC_V01)
   {
      biasCalPtr = snsInfoPtr->auto_cal_bias_params;
      scaleCalPtr = snsInfoPtr->auto_cal_scale_params;
   }

   if (biasCalPtr != NULL)
   {
      msgPtr->ZeroBias[0] = biasCalPtr[0];
      msgPtr->ZeroBias[1] = biasCalPtr[1];
      msgPtr->ZeroBias[2] = biasCalPtr[2];
   }
   if (scaleCalPtr != NULL)
   {
      msgPtr->ScaleFactor[0] = scaleCalPtr[0];
      msgPtr->ScaleFactor[1] = scaleCalPtr[1];
      msgPtr->ScaleFactor[2] = scaleCalPtr[2];
   }

   sns_smr_set_hdr(&msgHdr, msgPtr);
   err = sns_smr_send(msgPtr);
   if (err != SNS_SUCCESS)
   {
      //free the message
      sns_smr_msg_free(msgPtr);
   }

   if (biasCalPtr != NULL)
   {
      SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_GYRO_CAL_REQ_INFO,
                     biasCalPtr[0], biasCalPtr[1], biasCalPtr[2]);
   }

   if (scaleCalPtr != NULL)
   {
      SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_GYRO_CAL_REQ_INFO,
                     scaleCalPtr[0], scaleCalPtr[1], scaleCalPtr[2]);
   }
}

/*=========================================================================
  FUNCTION:  sns_scm_init_sensor_cal
  =========================================================================*/
/*!
  @brief Initialize sensor calibration parameters
  This implementation works for any sensor with the same format 
  of 3 int32_t values each for bias & scale params.
  any other format requires a different implementation

  @param[i] sensorId: sensor id
  @param[i] regDataPtr: pointer to the data in the registry message
  @param[i] calType: calibration parameter type
  
  @return None
*/
/*=======================================================================*/
bool sns_scm_init_sensor_cal(
   uint8_t sensorId,
   const int32_t* regDataPtr,
   uint8_t calType)
{
   uint8_t i = 0;
   bool status = false;
   
   if(NULL != sns_scm_sensor_dbase)
   {
      for(i = 0; i < SNS_SCM_MAX_SNS_MON; i++)
      {
         if (sns_scm_sensor_dbase[i].sensorId == sensorId)
         {
            int32_t *biasCalPtr = NULL;
            int32_t *scaleCalPtr = NULL;
            switch (calType)
            {
               case SNS_SMGR_CAL_FACTORY_V01:
                  biasCalPtr = sns_scm_sensor_dbase[i].fac_cal_bias_params;
                  scaleCalPtr = sns_scm_sensor_dbase[i].fac_cal_scale_params;
                  break;
               case SNS_SMGR_CAL_DYNAMIC_V01:
                  biasCalPtr = sns_scm_sensor_dbase[i].auto_cal_bias_params;
                  scaleCalPtr = sns_scm_sensor_dbase[i].auto_cal_scale_params;
                  break;
               default:
                  break;
            }

            if (biasCalPtr != NULL)
            {
               biasCalPtr[0] = regDataPtr[0];
               biasCalPtr[1] = regDataPtr[1];
               biasCalPtr[2] = regDataPtr[2];
            }
            if (scaleCalPtr != NULL)
            {
               scaleCalPtr[0] = regDataPtr[3];
               scaleCalPtr[1] = regDataPtr[4];
               scaleCalPtr[2] = regDataPtr[5];
            }

            sns_scm_send_cal_msg(&sns_scm_sensor_dbase[i], calType);

            status = true;
            break;
         }
      }
   }

   return status;
}

/*=========================================================================
  FUNCTION:  sns_scm_process_reg_single_read_resp
  =========================================================================*/
/*!
  @brief Process registry response to single item read request  

  @param[i] regRespPtr: Pointer to registry response message

  @return None
*/
/*=======================================================================*/
static void sns_scm_process_reg_single_read_resp(
   const void *regRespPtr)
{
   sns_reg_single_read_resp_msg_v02* msgPtr
      = (sns_reg_single_read_resp_msg_v02*)regRespPtr;

   if (msgPtr->resp.sns_result_t == SNS_SUCCESS)
   {
      sns_scm_process_reg_data(SNS_SCM_REG_ITEM_TYPE_SINGLE, 
                               msgPtr->item_id, 
                               msgPtr->data_len, 
                               msgPtr->data,
                               sns_scm_sensor_dbase);
   }
   else
   {
      SNS_SCM_DEBUG3(HIGH, DBG_SCM_REG_RESP_ERR,
                     msgPtr->item_id, 
                     msgPtr->resp.sns_result_t, 
                     msgPtr->resp.sns_err_t);
   }
}

/*=========================================================================
  FUNCTION:  sns_scm_process_reg_group_read_resp
  =========================================================================*/
/*!
  @brief Process registry response to group item read request  

  @param[i] regRespPtr: Pointer to registry response message

  @return None
*/
/*=======================================================================*/
static void sns_scm_process_reg_group_read_resp(
   const void *regRespPtr)
{
   sns_reg_group_read_resp_msg_v02* msgPtr
      = (sns_reg_group_read_resp_msg_v02*)regRespPtr;

   if (msgPtr->resp.sns_result_t == SNS_SUCCESS)
   {
      sns_scm_process_reg_data(SNS_SCM_REG_ITEM_TYPE_GROUP, 
                               msgPtr->group_id,
                               msgPtr->data_len, 
                               msgPtr->data,
                               sns_scm_sensor_dbase);
   }
   else
   {
      SNS_SCM_DEBUG3(HIGH, DBG_SCM_REG_RESP_ERR,
                     msgPtr->group_id, 
                     msgPtr->resp.sns_result_t, 
                     msgPtr->resp.sns_err_t);
   }
}

/*=========================================================================
  FUNCTION:  sns_scm_process_init_msg
  =========================================================================*/
/*!
  @brief Process messages from SCM's input queue during initialization 

  @return Number of messages processed
*/
/*=======================================================================*/
static uint8_t sns_scm_process_init_msg(void)
{
   void *msgPtr;
   sns_smr_header_s msgHdr;
   sns_err_code_e err;
   uint8_t numMsgProcessed = 0;

   while ((msgPtr = sns_smr_rcv(SNS_SCM_MODULE)) != NULL)
   {
      err = sns_smr_get_hdr(&msgHdr, msgPtr);
      if (err != SNS_SUCCESS)
      {
         SNS_PRINTF_STRING_ID_ERROR_1(SNS_SCM_DBG_MOD,
                                      DBG_SCM_PROC_MSG_HDR_ERR,
                                      err);

         sns_smr_msg_free(msgPtr);
         continue;
      }

      err = SNS_ERR_FAILED;

      if (msgHdr.src_module == SNS_MODULE_APPS_REG)
      {
         if (msgHdr.msg_type == SNS_SMR_MSG_TYPE_RESP)
         {
            if (msgHdr.msg_id == SNS_REG_GROUP_READ_RESP_V02)
            {
               sns_scm_process_reg_group_read_resp(msgPtr);
               err = SNS_SUCCESS;
               numMsgProcessed++;
            }             
            else if (msgHdr.msg_id == SNS_REG_SINGLE_READ_RESP_V02)
            {
               sns_scm_process_reg_single_read_resp(msgPtr);
               err = SNS_SUCCESS;
               numMsgProcessed++;
            }
         }
      }
      else if (msgHdr.src_module == SNS_MODULE_DSPS_SMGR)
      {
         if (msgHdr.msg_type == SNS_SMR_MSG_TYPE_RESP)
         {
            if (msgHdr.msg_id == SNS_SMGR_CAL_RESP_V01)
            {
               err = SNS_SUCCESS;
            }
         }
      }

      if (err != SNS_SUCCESS)
      {
         SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_MSG_DROP,
                        msgHdr.msg_id, msgHdr.src_module, msgHdr.svc_num);
      }
   
      sns_smr_msg_free(msgPtr);
   }

   return numMsgProcessed;
}

/*=========================================================================
  FUNCTION:  sns_scm_process_init_events
  =========================================================================*/
/*!
  @brief Process initialization events
  
  @param[i] numRegQuery: Number of queries made to registry
  @param[i] snsDbaseHandle: Handle to the SCM internal sensor DB

  @return None
*/
/*=======================================================================*/
void sns_scm_process_init_events(
   uint8_t numRegQuery,
   sns_scm_sensor_s* snsDbaseHandle)
{
   uint8_t err;
   OS_FLAGS sigFlags;

   SNS_SCM_DEBUG1(HIGH, DBG_SCM_REG_REQ_COUNT, numRegQuery);
   sns_scm_sensor_dbase = snsDbaseHandle;

   while (numRegQuery)
   {
      //wait for event
      sigFlags = sns_os_sigs_pend(sns_scm_init_sig_event, 
                                  SNS_SCM_SIGNAL_MASK, 
                                  OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 
                                  0, 
                                  &err);
      SNS_ASSERT(err == 0);

      //message event check
      if (sigFlags & SNS_SCM_MSG_SIG)
      {
         uint8_t numRegQueryProcessed = sns_scm_process_init_msg();
         SNS_ASSERT(numRegQuery >= numRegQueryProcessed);
         numRegQuery -= numRegQueryProcessed;
         sigFlags &= (~SNS_SCM_MSG_SIG);
      }
      //client report timer event check 
      if (sigFlags & SNS_SCM_REPORT_TIMER_SIG)
      {
         SNS_SCM_DEBUG1(HIGH, DBG_SCM_REG_REQ_FAIL, numRegQuery);
         sigFlags &= (~SNS_SCM_REPORT_TIMER_SIG);
         return;
      }
      if (sigFlags != 0)
      {
         SNS_SCM_DEBUG1(MEDIUM, DBG_SCM_PROCESS_EVT_UNKWN_EVT, sigFlags);
      }
   }

   SNS_SCM_DEBUG0(HIGH, DBG_SCM_REG_REQ_SUCCESS);
}

/*=========================================================================
  FUNCTION:  sns_scm_req_reg_data
  =========================================================================*/
/*!
  @brief Request registry data

  @param[i] regItemType: Registry item type (single / group)
  @param[i] regItemId: Registry item identifier

  @return Sensors error code
*/
/*=======================================================================*/
sns_err_code_e sns_scm_req_reg_data( 
   sns_scm_reg_item_type_e regItemType,
   uint32_t regItemId)
{
   void*            reqMsgPtr = NULL;
   sns_smr_header_s reqMsgHdr;
   sns_err_code_e   err;

   reqMsgHdr.src_module = SNS_SCM_MODULE;
   reqMsgHdr.priority = SNS_SMR_MSG_PRI_HIGH;
   reqMsgHdr.msg_type = SNS_SMR_MSG_TYPE_REQ;
   reqMsgHdr.svc_num = SNS_REG2_SVC_ID_V01;

   reqMsgHdr.txn_id = 0;
   reqMsgHdr.ext_clnt_id = 0;

   if (regItemType == SNS_SCM_REG_ITEM_TYPE_GROUP)
   {
     reqMsgHdr.msg_id = SNS_REG_GROUP_READ_REQ_V02;
     reqMsgHdr.body_len = sizeof(sns_reg_group_read_req_msg_v02);
   }
   else if (regItemType == SNS_SCM_REG_ITEM_TYPE_SINGLE)
   {
     reqMsgHdr.msg_id = SNS_REG_SINGLE_READ_REQ_V02;    
     reqMsgHdr.body_len = sizeof(sns_reg_single_read_req_msg_v02);
   }
   else
   {
     return SNS_ERR_FAILED;
   }

   reqMsgPtr = sns_smr_msg_alloc(SNS_SCM_DBG_MOD,reqMsgHdr.body_len);

   if(reqMsgPtr == NULL)
   {
     return SNS_ERR_NOMEM;
   }

   if (regItemType == SNS_SCM_REG_ITEM_TYPE_GROUP)
   {
     ((sns_reg_group_read_req_msg_v02*)reqMsgPtr)->group_id = regItemId;
   }
   else
   {
     ((sns_reg_single_read_req_msg_v02*)reqMsgPtr)->item_id = regItemId;
   }

   sns_smr_set_hdr(&reqMsgHdr, reqMsgPtr);

   err = sns_smr_send(reqMsgPtr);

   if ( err != SNS_SUCCESS)
   {
      //free the message
      sns_smr_msg_free(reqMsgPtr);
   }

   return err;
}

/*=========================================================================
  FUNCTION:  sns_scm_reg_sensor_status
  =========================================================================*/
/*!
  @brief Send a request to sensors manager for sensor status report

  @param[i] sensorId: sensor id

  @return Sensors error code
*/
/*=======================================================================*/
sns_err_code_e sns_scm_reg_sensor_status(
   uint8_t sensorId)
{
   sns_smr_header_s msgHdr;
   sns_err_code_e err;

   uint8_t msgSize = sizeof(sns_smgr_sensor_status_req_msg_v01);
   sns_smgr_sensor_status_req_msg_v01* msgPtr = 
      (sns_smgr_sensor_status_req_msg_v01 *)sns_smr_msg_alloc(SNS_SCM_DBG_MOD,msgSize);
   SNS_ASSERT(msgPtr != NULL);

   msgHdr.src_module = SNS_SCM_MODULE;
   msgHdr.svc_num = SNS_SMGR_SVC_ID_V01;
   msgHdr.msg_type = SNS_SMR_MSG_TYPE_REQ;
   msgHdr.priority = SNS_SMR_MSG_PRI_LOW;
   msgHdr.msg_id = SNS_SMGR_SENSOR_STATUS_REQ_V01;
   msgHdr.body_len = msgSize;

   msgHdr.txn_id = 0;
   msgHdr.ext_clnt_id = 0;

   msgPtr->SensorID = sensorId;
   msgPtr->Action = SNS_SMGR_SENSOR_STATUS_ADD_V01;
   msgPtr->ReqDataTypeNum = 1;

   sns_smr_set_hdr(&msgHdr, msgPtr);
   err = sns_smr_send(msgPtr);
   if (err != SNS_SUCCESS)
   {
      //free the message
      sns_smr_msg_free(msgPtr);
   }

   SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_REQ_SNSR_STATUS_INFO,
                  SNS_SMGR_SENSOR_STATUS_ADD_V01, 
                  sensorId, 
                  1);

   return err;
}




