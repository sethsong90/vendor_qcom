/*============================================================================
  FILE: sns_scm.c

  This file contains the Sensors Calibration Manager implementation

  Copyright (c) 2011 - 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
 ============================================================================*/

/* $Header: //source/qcom/qct/core/pkg/dsps/dev/kaushiks.accelautocal_v1/core/sensors/scm/framework/src/sns_scm.c#1 $ */
/* $DateTime: 2012/06/19 14:40:08 $ */
/* $Author: coresvc $ */

/*============================================================================
  EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order. 

  when        who  what, where, why
  ----------  ---  -----------------------------------------------------------
  2012-07-10  ks   Refactoring SCM into apps and dsps versions
  2012-03-07  ad   Support gyro autocal sample rate config via registry
  2012-02-02  ad   Consolidate all registry requests during initialization
  2012-01-19  ks   Reenabling Debug Msgs disabled for PCSim
  2012-01-18  ks   Disabling debug msgs for PCSim, reenabling for On Target
  2012-01-06  ad   Cleanup tabs 
  2011-12-19  ks   PCSIM workaround for Calibration Cycle 1 minute timeout
  2011-11-14  jhh  Updated alloc and free function calls to meet new API
  2011-10-10  ad   Do not disable QMD on report from SMGR when in motion  
  2011-08-30  ad   Add registry support for gyro autocal params  
  2011-09-09  sc   Update with registry service V02
  2011-08-12  ad   Activate QMD only for gyro active use case 
  2011-08-09  ad   Added check on gyro absolute value for bias calibration 
  2011-08-04  rb   Added factory calibration initialization 
  2011-08-03  rb   Added bias value storage during shutdown
  2011-07-29  rb   Added checking for allowance of algorithm instantiation
  2011-07-20  ad   Refactor SCM for memory optimization
  2011-07-13  rb   Added support for calibration via registry
  2011-07-11  ad   request filtered data from SMGR for better noise performance
  2011-07-06  ad   handle back-to-back idle/active messages from sensors manager
  2011-07-05  sc   Re-formated with UNIX newline ending
  2011-06-28  ad   incorporate SMGR update to sensor status indication behavior
  2011-06-21  ad   updated gyro bias calibration algorithm
  2011-06-08  ad   changed the gyro sampling rate for offset cal to 25Hz
  2011-04-08  ad   register for QMD only if calibration algorithm is active
  2011-03-17  ad   add debug messages
  2011-03-14  ad   add gyro cal log support
  2011-03-04  ad   integrate SMGR calibration interface
  2011-02-04  ad   initial version

 ============================================================================*/

#define __SNS_MODULE__ SNS_SCM

/*---------------------------------------------------------------------------
 * Include Files
 * -------------------------------------------------------------------------*/
#include "sns_scm_priv.h"
#include "gyro_cal.h"

#include "sns_reg_common.h"
#include "sns_reg_api_v02.h"

#include "sns_init.h"
#include "sns_memmgr.h"
#include "sns_smgr_api_v01.h"
#include "sns_sam_amd_v01.h"
#include "sns_debug_str.h"
#include "fixed_point.h"
#include "sns_log_api.h"
#include "sns_log_types.h"

/*---------------------------------------------------------------------------
 * Type Declarations
 * -------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * Global Data Definitions
 * -------------------------------------------------------------------------*/
#ifdef SNS_PLAYBACK_SKIP_SMGR
static int sns_scm_pb_gyro_cal_algo_svc_id=0;
static uint32_t sns_scm_pb_report_timeout=1966600; // dsps clock ticks corresponding to 1 min timeout of SCM
static uint32_t sns_scm_pb_next_report_time=0; 
static bool sns_scm_pb_update_next_report_time=false;
#endif

/*---------------------------------------------------------------------------
 * Static Variable Definitions
 * -------------------------------------------------------------------------*/
//SCM task stack
static OS_STK sns_scm_task_stk[SNS_SCM_MODULE_STK_SIZE];

//SCM event signal
static OS_FLAG_GRP *sns_scm_sig_event;

//algorithm database
static sns_scm_algo_s* sns_scm_algo_dbase[SNS_SCM_NUM_ALGO_SVCS];

//algorithm instance database
static sns_scm_algo_inst_s* sns_scm_algo_inst_dbase[SNS_SCM_MAX_ALGO_INSTS];
static uint8_t sns_scm_algo_inst_count;

//sensor data request database
static sns_scm_data_req_s* sns_scm_data_req_dbase[SNS_SCM_MAX_DATA_REQS];
static uint8_t sns_scm_data_req_count;

//sensor database
static sns_scm_sensor_s sns_scm_sensor_dbase[SNS_SCM_MAX_SNS_MON];

static uint8_t sns_scm_qmd_inst_id = SNS_SCM_INVALID_ID;
static uint8_t sns_scm_qmd_state = SNS_SAM_MOTION_UNKNOWN_V01;

/*---------------------------------------------------------------------------
 * Function Definitions
 * -------------------------------------------------------------------------*/

/*=========================================================================
  FUNCTION:  sns_scm_timer_cb
  =========================================================================*/
/*!
  @brief Callback registered for timer expiry

  @param[i] argPtr: pointer to argument for callback function

  @return None
*/
/*=======================================================================*/
void sns_scm_timer_cb(
   void *argPtr)
{
   uint8_t err;
   uint32_t algoIndex = (uint32_t)(uintptr_t)argPtr;

   if (algoIndex < SNS_SCM_NUM_ALGO_SVCS &&
       sns_scm_algo_dbase[algoIndex] != NULL)
   {
      sns_scm_algo_dbase[algoIndex]->timeout = true;

      sns_os_sigs_post(sns_scm_sig_event, 
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
}

/*=========================================================================
  FUNCTION:  sns_scm_reg_timer
  =========================================================================*/
/*!
  @brief Register timer for client reports

  @param[i] algoIndex: algorithm instance id

  @return Sensors error code
*/
/*=======================================================================*/
static sns_err_code_e sns_scm_reg_timer(
   uint32_t algoIndex)
{
   sns_err_code_e err;
   sns_scm_algo_s* algoPtr = sns_scm_algo_dbase[algoIndex];

   if(algoPtr->period == 0)
   {
      // timer disabled
      return SNS_SUCCESS;
   }
   
   if (algoPtr->timer == NULL)
   {
      err = sns_em_create_timer_obj(sns_scm_timer_cb, 
                                    (void*)(intptr_t)algoIndex, 
                                    SNS_EM_TIMER_TYPE_PERIODIC,
                                    &(algoPtr->timer));
   }
   else
   {
      err = sns_em_cancel_timer(algoPtr->timer);
   }

#ifndef SNS_PLAYBACK_SKIP_SMGR
   if (err == SNS_SUCCESS)
   {
      err = sns_em_register_timer(algoPtr->timer, 
                                  algoPtr->period);
      if (err == SNS_SUCCESS)
      {

         SNS_SCM_DEBUG2(MEDIUM, DBG_SCM_REG_TIMER_STARTED,
                        algoIndex, algoPtr->period);
      }
      else
      {
         SNS_PRINTF_STRING_ID_ERROR_1(SNS_SCM_DBG_MOD,
                                      DBG_SCM_REG_TIMER_FAILED,
                                      err);
      }
   }
#endif

   return err;
}

/*=========================================================================
  FUNCTION:  sns_scm_dereg_timer
  =========================================================================*/
/*!
  @brief Deregister timer

  @param[i] algoIndex: client request id

  @return Sensors error code
*/
/*=======================================================================*/
static sns_err_code_e sns_scm_dereg_timer(
   uint8_t algoIndex)
{
   sns_err_code_e err;

   if (sns_scm_algo_dbase[algoIndex]->timer == NULL)
   {
      return SNS_SUCCESS;
   }

#ifndef SNS_PLAYBACK_SKIP_SMGR
   err = sns_em_cancel_timer(sns_scm_algo_dbase[algoIndex]->timer);
   if (err != SNS_SUCCESS)
   {
      SNS_PRINTF_STRING_ID_ERROR_1(SNS_SCM_DBG_MOD,
                                   DBG_SCM_DEREG_TIMER_FAILED,
                                   err);
   }
#endif

   err = sns_em_delete_timer_obj(sns_scm_algo_dbase[algoIndex]->timer);
   if (err == SNS_SUCCESS)
   {
      sns_scm_algo_dbase[algoIndex]->timer = NULL;

      SNS_SCM_DEBUG1(MEDIUM, DBG_SCM_DEREG_TIMER_DELETED, 
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
  FUNCTION:  sns_scm_find_data_req
  =========================================================================*/
/*!
  @brief Searches the active sensor data request database for an instance 
  with the same sensor id and report rate

  @param[i] algoInstId: algorithm instance id
  @param[i] reportRate: report generation rate
  @param[i] sensorReqCnt: number of sensors in data request
  @param[i] sensorReq: sensors for which data is requested

  @return  Index to algorithm instance matching the specified configuration
  SNS_SCM_INVALID_ID if match is not found
*/
/*=======================================================================*/
static uint8_t sns_scm_find_data_req(
   uint8_t algoInstId,
   uint32_t reportRate,
   uint8_t sensorReqCnt,
   uint8_t sensorIndex[])
{
   uint8_t dataReqId, dataReqCnt;

   for (dataReqId = 0, dataReqCnt = 0; 
       dataReqCnt < sns_scm_data_req_count && 
       dataReqId < SNS_SCM_MAX_DATA_REQS;
       dataReqId++)
   {
      if (sns_scm_data_req_dbase[dataReqId] != NULL)
      {
         sns_scm_data_req_s *dataReq = sns_scm_data_req_dbase[dataReqId];

         if (dataReq->sensorCount == sensorReqCnt &&
             dataReq->reportRate == reportRate)
         {
            uint8_t i, j;
            for (j = 0; j < sensorReqCnt; j++)
            {
               //ordered match done on requests
               if (dataReq->sensorDbase[j] != sensorIndex[j])
               {
                  break;
               }
            }

            //Found matching request
            if (j >= sensorReqCnt)
            {
               //Avoid duplicate
               for (i = 0; i < dataReq->algoInstCount; i++)
               {
                  if (dataReq->algoInstDbase[i] == algoInstId)
                  {
                     return dataReqId;
                  }
               }

               //Add request
               if (i < SNS_SCM_MAX_ALGO_INSTS_PER_DATA_REQ)
               {
                  dataReq->algoInstDbase[i] = algoInstId;
                  dataReq->algoInstCount++;
                  return dataReqId;
               }
            }
         }

         dataReqCnt++;
      }
   }

   return SNS_SCM_INVALID_ID;
}

/*=========================================================================
  FUNCTION:  sns_scm_send_qmd_stop_req
  =========================================================================*/
/*!
  @brief Send a request to sensors algorithm manager for QMD

  @return Sensors error code
*/
/*=======================================================================*/
static void sns_scm_send_qmd_stop_req(void)
{
   sns_smr_header_s msgHdr;
   sns_sam_qmd_disable_req_msg_v01 *msgPtr;
   uint8_t msgSize = 0;
   sns_err_code_e err;

   msgSize = sizeof(sns_sam_qmd_disable_req_msg_v01);
   msgPtr = 
   (sns_sam_qmd_disable_req_msg_v01 *)sns_smr_msg_alloc(SNS_SCM_DBG_MOD,msgSize);
   SNS_ASSERT(msgPtr != NULL);

   msgHdr.src_module = SNS_SCM_MODULE;
   msgHdr.svc_num = SNS_SAM_AMD_SVC_ID_V01;
   msgHdr.msg_type = SNS_SMR_MSG_TYPE_REQ;
   msgHdr.priority = SNS_SMR_MSG_PRI_LOW;
   msgHdr.msg_id = SNS_SAM_AMD_DISABLE_REQ_V01;
   msgHdr.body_len = msgSize;

   msgHdr.txn_id = 0;
   msgHdr.ext_clnt_id = 0;

   msgPtr->instance_id = sns_scm_qmd_inst_id;

   sns_smr_set_hdr(&msgHdr, msgPtr);
   err = sns_smr_send(msgPtr);
   if (err != SNS_SUCCESS)
   {
      //free the message
      sns_smr_msg_free(msgPtr);
   }
   
   //reset QMD instance id and state
   sns_scm_qmd_inst_id = SNS_SCM_INVALID_ID;
   sns_scm_qmd_state = SNS_SAM_MOTION_UNKNOWN_V01;

   SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_REQ_ALGO_SVC,
                  SNS_SAM_AMD_SVC_ID_V01, 
                  SNS_SAM_AMD_DISABLE_REQ_V01, 
                  err);
}

/*=========================================================================
  FUNCTION:  sns_scm_send_smgr_start_req
  =========================================================================*/
/*!
  @brief Send a request to sensors manager for sensor data

  @param[i] dataReqId: Index of data request in database

  @return Sensors error code
*/
/*=======================================================================*/
static sns_err_code_e sns_scm_send_smgr_start_req(
   uint8_t dataReqId)
{
   sns_smr_header_s msgHdr;
   sns_smgr_periodic_report_req_msg_v01 *msgPtr;
   uint_fast16_t msgSize = 0;
   sns_err_code_e err;
   uint8_t i;

   msgSize = sizeof(sns_smgr_periodic_report_req_msg_v01);
   msgPtr =
   (sns_smgr_periodic_report_req_msg_v01 *)sns_smr_msg_alloc(SNS_SCM_DBG_MOD,msgSize);
   SNS_ASSERT(msgPtr != NULL);

   msgHdr.src_module = SNS_SCM_MODULE;
   msgHdr.svc_num = SNS_SMGR_SVC_ID_V01;
   msgHdr.msg_type = SNS_SMR_MSG_TYPE_REQ;
   msgHdr.priority = SNS_SMR_MSG_PRI_LOW;
   msgHdr.msg_id = SNS_SMGR_REPORT_REQ_V01;
   msgHdr.body_len = msgSize;

   //TODO to be updated (currently not used)
   msgHdr.txn_id = 0;
   msgHdr.ext_clnt_id = 0;

   msgPtr->BufferFactor = 1;

   msgPtr->ReportId = dataReqId;
   msgPtr->Action = SNS_SMGR_REPORT_ACTION_ADD_V01;

   msgPtr->ReportRate = sns_scm_data_req_dbase[dataReqId]->reportRate;

   msgPtr->cal_sel_valid = true;
   msgPtr->cal_sel_len = sns_scm_data_req_dbase[dataReqId]->sensorCount;

   msgPtr->Item_len = sns_scm_data_req_dbase[dataReqId]->sensorCount;
   for (i = 0; i < sns_scm_data_req_dbase[dataReqId]->sensorCount; i++)
   {
      msgPtr->Item[i].SensorId =
      sns_scm_sensor_dbase[sns_scm_data_req_dbase[dataReqId]->sensorDbase[i]].sensorId;
      msgPtr->Item[i].DataType =
      sns_scm_sensor_dbase[sns_scm_data_req_dbase[dataReqId]->sensorDbase[i]].dataType;

      msgPtr->Item[i].Decimation = SNS_SMGR_DECIMATION_FILTER_V01;

      msgPtr->cal_sel[i] = SNS_SMGR_CAL_SEL_FACTORY_CAL_V01;
   }

   // For Accel Auto Cal additionally request for temperature data
   
   if ( (sns_scm_data_req_dbase[dataReqId]->sensorCount == 1) &&
         (sns_scm_sensor_dbase[sns_scm_data_req_dbase[dataReqId]->
         sensorDbase[0]].sensorId == SNS_SMGR_ID_ACCEL_V01) )
   {
      msgPtr->Item_len++;
      msgPtr->Item[msgPtr->Item_len-1].SensorId = SNS_SMGR_ID_GYRO_V01;
      msgPtr->Item[msgPtr->Item_len-1].DataType = SNS_SMGR_DATA_TYPE_SECONDARY_V01;
   }
   

   sns_smr_set_hdr(&msgHdr, msgPtr);
   err = sns_smr_send(msgPtr);
   if (err != SNS_SUCCESS)
   {
      //free the message
      sns_smr_msg_free(msgPtr);
   }

   SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_REQ_SNSR_DATA_INFO,
                  dataReqId, 
                  sns_scm_data_req_dbase[dataReqId]->sensorCount, 
                  sns_scm_sensor_dbase[sns_scm_data_req_dbase[dataReqId]->sensorDbase[0]].sensorId);

   return err;
}

/*=========================================================================
  FUNCTION:  sns_scm_send_smgr_stop_req
  =========================================================================*/
/*!
  @brief Send a request to sensors manager to stop sensor data received by  
  specified algorithm

  @param[i] dataReqId: Index of data request in database

  @return None
*/
/*=======================================================================*/
static void sns_scm_send_smgr_stop_req(
   uint8_t dataReqId)
{
   sns_smgr_periodic_report_req_msg_v01 *msgPtr;
   sns_smr_header_s msgHdr;
   uint16_t msgSize = sizeof(sns_smgr_periodic_report_req_msg_v01);

   //send request to stop sensor data by sensor manager
   msgPtr =
   (sns_smgr_periodic_report_req_msg_v01 *)sns_smr_msg_alloc(SNS_SCM_DBG_MOD,msgSize);
   SNS_ASSERT(msgPtr != NULL);

   msgHdr.src_module = SNS_SCM_MODULE;
   msgHdr.svc_num = SNS_SMGR_SVC_ID_V01;
   msgHdr.msg_type = SNS_SMR_MSG_TYPE_REQ;
   msgHdr.priority = SNS_SMR_MSG_PRI_LOW;
   msgHdr.msg_id = SNS_SMGR_REPORT_REQ_V01;
   msgHdr.body_len = msgSize;

   msgHdr.txn_id = 0;
   msgHdr.ext_clnt_id = 0;

   msgPtr->Item_len = 0;
   msgPtr->BufferFactor = 1;

   msgPtr->ReportId = dataReqId;
   msgPtr->Action = SNS_SMGR_REPORT_ACTION_DELETE_V01;

   sns_smr_set_hdr(&msgHdr, msgPtr);
   if (sns_smr_send(msgPtr) != SNS_SUCCESS)
   {
      //free the message
      sns_smr_msg_free(msgPtr);
   }

   SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_STOP_SNSR_DATA,
                  dataReqId, 
                  sns_scm_data_req_dbase[dataReqId]->sensorCount, 
                  sns_scm_sensor_dbase[sns_scm_data_req_dbase[dataReqId]->sensorDbase[0]].sensorId);
}

/*=========================================================================
  FUNCTION:  sns_scm_start_sensor_data
  =========================================================================*/
/*!
  @brief Request sensor data according to the algorithm needs

  @param[i] algoInstId: algorithm instance id
  @param[i] reportRate: report generation rate
  @param[i] sensorReqCnt: number of sensors in data request
  @param[i] sensorReq: sensors for which data is requested

  @return Sensors error code
*/
/*=======================================================================*/
sns_err_code_e sns_scm_start_sensor_data(
   uint8_t algoInstId,
   uint32_t reportRate,
   uint8_t sensorReqCnt,
   uint8_t sensorIndex[])
{
   sns_err_code_e err = SNS_ERR_FAILED;
   uint8_t dataReqId, i;

   //check if there is an identical existing request
   dataReqId = sns_scm_find_data_req(algoInstId, 
                                     reportRate, 
                                     sensorReqCnt, 
                                     sensorIndex);

   //add request if none exists
   if (dataReqId >= SNS_SCM_MAX_DATA_REQS)
   {
      if (sns_scm_data_req_count >= SNS_SCM_MAX_DATA_REQS)
      {
         return err;
      }

      for (dataReqId = 0; dataReqId < SNS_SCM_MAX_DATA_REQS; dataReqId++)
      {
         if (sns_scm_data_req_dbase[dataReqId] == NULL)
         {
            sns_scm_data_req_dbase[dataReqId] = 
            SNS_OS_MALLOC(SNS_SCM_DBG_MOD, sizeof(sns_scm_data_req_s));
            SNS_ASSERT(sns_scm_data_req_dbase[dataReqId] != NULL);

            for (i = 0; i < SNS_SCM_MAX_ALGO_INSTS_PER_DATA_REQ; i++)
            {
               sns_scm_data_req_dbase[dataReqId]->algoInstDbase[i] = 
               SNS_SCM_INVALID_ID;
            }
            sns_scm_data_req_dbase[dataReqId]->reportRate = reportRate;
            for (i = 0; i < sensorReqCnt; i++)
            {
               sns_scm_data_req_dbase[dataReqId]->sensorDbase[i] = sensorIndex[i];
            }
            sns_scm_data_req_dbase[dataReqId]->sensorCount = sensorReqCnt;
            sns_scm_data_req_dbase[dataReqId]->algoInstDbase[0] = algoInstId;
            sns_scm_data_req_dbase[dataReqId]->algoInstCount = 1;

            //send message to sensors manager requesting required sensor data
            err = sns_scm_send_smgr_start_req(dataReqId);

            sns_scm_data_req_count++;

            break;
         }
      }
   }
   else
   {
      err = SNS_SUCCESS;
   }

   return err;
}

/*=========================================================================
  FUNCTION:  sns_scm_stop_sensor_data
  =========================================================================*/
/*!
  @brief Stop sensor data received for specified algorithm instance

  @param[i] algoInstId: Index of algorithm instance in database
  @param[i] sensorId: Index of algorithm instance in database
  @param[i] reportRate: Index of algorithm instance in database

  @return None
*/
/*=======================================================================*/
static void sns_scm_stop_sensor_data(
   uint8_t algoInstId)
{
   uint8_t dataReqId, dataReqCnt, i;

   for (dataReqId = 0, dataReqCnt = 0; 
       dataReqCnt < sns_scm_data_req_count && 
       dataReqId < SNS_SCM_MAX_DATA_REQS; 
       dataReqId++)
   {
      if (sns_scm_data_req_dbase[dataReqId] != NULL)
      {
         sns_scm_data_req_s *dataReq = sns_scm_data_req_dbase[dataReqId];
         for (i = 0; i < dataReq->algoInstCount; i++)
         {
            if (dataReq->algoInstDbase[i] == algoInstId)
            {
               dataReq->algoInstCount--;
               dataReq->algoInstDbase[i] = 
               dataReq->algoInstDbase[dataReq->algoInstCount];
               dataReq->algoInstDbase[dataReq->algoInstCount] = 
               SNS_SCM_INVALID_ID;

               if (dataReq->algoInstCount == 0)
               {
                  sns_scm_send_smgr_stop_req(dataReqId);
                  SNS_OS_FREE(sns_scm_data_req_dbase[dataReqId]);
                  sns_scm_data_req_dbase[dataReqId] = NULL;

                  sns_scm_data_req_count--;
                  break;
               }
            }
         }
         dataReqCnt++;
      }
   }
}

/*=========================================================================
  FUNCTION:  sns_scm_find_algo_inst
  =========================================================================*/
/*!
  @brief Searches the active algorithm database for an instance 
  of an algorithm with specified configuration

  @param[i] algoSvcId: algorithm service id
  @param[i] algoCfgPtr: pointer to configuration of specified algorithm

  @return  Index to algorithm instance matching the specified configuration
  if successful, SNS_SCM_INVALID_ID otherwise
*/
/*=======================================================================*/
static uint8_t sns_scm_find_algo_inst(
   uint8_t algoSvcId, 
   void *algoCfgPtr)
{
   uint8_t algoInstId, algoInstCnt;

   for (algoInstId = 0, algoInstCnt = 0; 
       algoInstCnt < sns_scm_algo_inst_count && 
       algoInstId < SNS_SCM_MAX_ALGO_INSTS; 
       algoInstId++)
   {
      sns_scm_algo_inst_s *algoInstPtr = sns_scm_algo_inst_dbase[algoInstId];
      if (algoInstPtr == NULL)
      {
         continue;
      }

      if (sns_scm_algo_dbase[algoInstPtr->algoIndex]->serviceId == algoSvcId)
      {
         if (!SNS_OS_MEMCMP(algoCfgPtr, 
                            algoInstPtr->configData.memPtr, 
                            algoInstPtr->configData.memSize))
         {
            return algoInstId;
         }
      }
      algoInstCnt++;
   }
   return SNS_SCM_INVALID_ID;
}

/*=========================================================================
  FUNCTION:  sns_scm_get_algo_inst
  =========================================================================*/
/*!
  @brief
  If an instance of the specified algorithm doesnt exist,
  creates an instance and initializes the reference count.
  If an instance with identical configuration exists, 
  updates the reference count.

  @param[i] algoIndex: index to algorithm in the algorithm database

  @return index of the algorithm instance in database if successful,
          SNS_SCM_INVALID_ID otherwise
*/
/*=======================================================================*/
static uint8_t sns_scm_get_algo_inst(
   uint8_t algoIndex) 
{
   uint8_t algoSvcId = sns_scm_algo_dbase[algoIndex]->serviceId;

   void *algoCfgPtr = sns_scm_algo_dbase[algoIndex]->defConfigData.memPtr;

   uint8_t algoInstId = sns_scm_find_algo_inst(algoSvcId, algoCfgPtr);

   if (algoInstId >= SNS_SCM_MAX_ALGO_INSTS)
   {
      uint8_t instId;

      //check if number of algo instances already reaches max
      if (sns_scm_algo_inst_count >= SNS_SCM_MAX_ALGO_INSTS)
      {
         return SNS_SCM_INVALID_ID;
      }

      for (instId = 0; instId < SNS_SCM_MAX_ALGO_INSTS; instId++)
      {
         if (sns_scm_algo_inst_dbase[instId] == NULL)
         {
            sns_scm_algo_inst_s* algoInstPtr;
            void *statePtr = NULL;
            uint32_t memSize = sizeof(sns_scm_algo_inst_s);

            //create algorithm instance
            sns_scm_algo_inst_dbase[instId] = SNS_OS_MALLOC(SNS_SCM_DBG_MOD, memSize);
            SNS_ASSERT(sns_scm_algo_inst_dbase[instId] != NULL);
            algoInstPtr = sns_scm_algo_inst_dbase[instId];

            //initialize the algorithm instance
            algoInstPtr->configData.memPtr = NULL;
            algoInstPtr->stateData.memPtr = NULL;
            algoInstPtr->inputData.memPtr = NULL;
            algoInstPtr->outputData.memPtr = NULL;

            memSize = sns_scm_algo_dbase[algoIndex]->defConfigData.memSize;
            if (memSize > 0)
            {
               algoInstPtr->configData.memPtr = SNS_OS_MALLOC(SNS_SCM_DBG_MOD, memSize);
               SNS_ASSERT(algoInstPtr->configData.memPtr != NULL);
               SNS_OS_MEMCOPY(algoInstPtr->configData.memPtr, 
                              algoCfgPtr, memSize);
            }
            algoInstPtr->configData.memSize = memSize;

            memSize = sns_scm_algo_dbase[algoIndex]->algoApi.
                      sns_scm_algo_mem_req(algoCfgPtr);

            //Algorithm indicates configuration error with size 0
            if (memSize == 0)
            {
               SNS_OS_FREE(algoInstPtr->configData.memPtr);
               algoInstPtr->configData.memPtr = NULL;
               SNS_OS_FREE(sns_scm_algo_inst_dbase[instId]);
               sns_scm_algo_inst_dbase[instId] = NULL;
               return SNS_SCM_INVALID_ID;
            }

            algoInstPtr->stateData.memPtr = SNS_OS_MALLOC(SNS_SCM_DBG_MOD, memSize);
            SNS_ASSERT(algoInstPtr->stateData.memPtr != NULL);
            SNS_OS_MEMZERO(algoInstPtr->stateData.memPtr, memSize);
            algoInstPtr->stateData.memSize = memSize;

            memSize = sns_scm_algo_dbase[algoIndex]->defInputDataSize;
            if (memSize > 0)
            {
               algoInstPtr->inputData.memPtr = SNS_OS_MALLOC(SNS_SCM_DBG_MOD, memSize);
               SNS_ASSERT(algoInstPtr->inputData.memPtr != NULL);
               SNS_OS_MEMZERO(algoInstPtr->inputData.memPtr, memSize);
            }
            algoInstPtr->inputData.memSize = memSize;

            memSize = sns_scm_algo_dbase[algoIndex]->defOutputDataSize;
            if (memSize > 0)
            {
               algoInstPtr->outputData.memPtr = SNS_OS_MALLOC(SNS_SCM_DBG_MOD, memSize);
               SNS_ASSERT(algoInstPtr->outputData.memPtr != NULL);
               SNS_OS_MEMZERO(algoInstPtr->outputData.memPtr, memSize);
            }
            algoInstPtr->outputData.memSize = memSize;

            statePtr = sns_scm_algo_dbase[algoIndex]->algoApi.
                       sns_scm_algo_reset(algoInstPtr->configData.memPtr, 
                                          algoInstPtr->stateData.memPtr);
            if (statePtr == NULL)
            {
               SNS_PRINTF_STRING_ID_ERROR_0(SNS_SCM_DBG_MOD,
                                            DBG_SCM_ENABLE_ALGO_STATE_NULL);
            }

            algoInstPtr->algoIndex = algoIndex;

            algoInstId = instId;

            sns_scm_algo_inst_count++;

            //request sensor data
            sns_scm_req_sensor_data(algoInstId,
                        sns_scm_algo_dbase[algoIndex]->serviceId,
                        sns_scm_algo_inst_dbase[algoInstId]->configData.memPtr,
                        sns_scm_algo_dbase[algoIndex]->sensorIndex);


            //Log algorithm configuration
            sns_scm_log_algo_config(algoInstId,
                                    algoInstPtr->configData.memPtr,
                                    sns_scm_algo_dbase[algoIndex]->serviceId);

            SNS_SCM_DEBUG3(LOW, DBG_SCM_ALGO_MEM_INFO,
                           sns_scm_algo_dbase[algoInstPtr->algoIndex]->serviceId,
                           sizeof(sns_scm_algo_inst_s),
                           algoInstPtr->configData.memSize);
            SNS_SCM_DEBUG3(LOW, DBG_SCM_ALGO_STATE_MEM_INFO,
                           algoInstPtr->stateData.memSize,
                           algoInstPtr->inputData.memSize,
                           algoInstPtr->outputData.memSize);

            break;
         }
      }
   }

   return algoInstId;
}

/*=========================================================================
  FUNCTION:  sns_scm_delete_algo_inst
  =========================================================================*/
/*!
  @brief Deletes the specified algorithm instance

  @param[i] algoInstId: index to algorithm instance in database

  @return None
*/
/*=======================================================================*/
static void sns_scm_delete_algo_inst(
   uint8_t algoInstId)
{
   sns_scm_algo_inst_s* algoInstPtr = sns_scm_algo_inst_dbase[algoInstId];

   //free memory reserved for this algorithm instance
   SNS_OS_FREE(algoInstPtr->configData.memPtr);
   algoInstPtr->configData.memPtr = NULL;
   algoInstPtr->configData.memSize = 0;

   SNS_OS_FREE(algoInstPtr->stateData.memPtr);
   algoInstPtr->stateData.memPtr = NULL;
   algoInstPtr->stateData.memSize = 0;

   SNS_OS_FREE(algoInstPtr->inputData.memPtr);
   algoInstPtr->inputData.memPtr = NULL;
   algoInstPtr->inputData.memSize = 0;

   SNS_OS_FREE(algoInstPtr->outputData.memPtr);
   algoInstPtr->outputData.memPtr = NULL;
   algoInstPtr->outputData.memSize = 0;

   SNS_OS_FREE(sns_scm_algo_inst_dbase[algoInstId]);
   sns_scm_algo_inst_dbase[algoInstId] = NULL;

   sns_scm_algo_inst_count--;
}

/*=========================================================================
  FUNCTION:  sns_scm_enable_algo
  =========================================================================*/
/*!
  @brief
  enables specified algorithm with the specified configuration

  @param[i] algoIndex: index to algorithm in the algorithm database

  @return index of the algorithm instance in database if successful,
          SNS_SCM_INVALID_ID otherwise
*/
/*=======================================================================*/
static uint8_t sns_scm_enable_algo(
   uint8_t algoIndex) 
{
   uint8_t algoInstId;

   algoInstId = sns_scm_get_algo_inst(algoIndex);
   if (algoInstId >= SNS_SCM_MAX_ALGO_INSTS || 
       sns_scm_algo_inst_dbase[algoInstId] == NULL)
   {
      return SNS_SCM_INVALID_ID;
   }

   if (sns_scm_algo_dbase[algoIndex]->timer == NULL)
   {
      sns_scm_reg_timer(algoIndex);
   }

   sns_scm_motion_state_change_algo_update(sns_scm_algo_dbase[algoIndex]->serviceId,
                                   sns_scm_algo_inst_dbase[algoInstId]->inputData.memPtr,
                                   sns_scm_algo_inst_dbase[algoInstId]->outputData.memPtr,
                                   sns_scm_algo_inst_dbase[algoInstId]->stateData.memPtr,
                                   &sns_scm_algo_dbase[algoIndex]->algoApi,
                                   &sns_scm_qmd_state);

   SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_ENABLE_ALGO,
                  sns_scm_algo_dbase[algoIndex]->serviceId, 
                  sns_scm_sensor_dbase[sns_scm_algo_dbase[algoIndex]->sensorIndex].sensorId,
                  algoInstId);

   return algoInstId;
}

/*=========================================================================
  FUNCTION:  sns_scm_disable_algo
  =========================================================================*/
/*!
  @brief Decrements the reference count of the algorithm instance.
         If count is zero, deletes an instance and frees up its resources

  @param[i] algoIndex: index to algorithm in the algorithm database

  @return Sensors error code
*/
/*=======================================================================*/
static sns_err_code_e sns_scm_disable_algo(
   uint8_t algoIndex)
{
   uint8_t algoInstId;
   sns_scm_algo_inst_s *algoInstPtr;

   algoInstId = sns_scm_find_algo_inst(
                                      sns_scm_algo_dbase[algoIndex]->serviceId, 
                                      sns_scm_algo_dbase[algoIndex]->defConfigData.memPtr);

   if (algoInstId >= SNS_SCM_MAX_ALGO_INSTS ||
       sns_scm_algo_inst_dbase[algoInstId] == NULL)
   {
      SNS_PRINTF_STRING_ID_ERROR_1(SNS_SCM_DBG_MOD,
                                   DBG_SCM_DISABLE_ALGO_INSTANCE_ERR,
                                   algoInstId);
      return SNS_ERR_FAILED;
   }

   algoInstPtr = sns_scm_algo_inst_dbase[algoInstId];

   sns_scm_motion_state_change_algo_update(sns_scm_algo_dbase[algoIndex]->serviceId,
                                      sns_scm_algo_inst_dbase[algoInstId]->inputData.memPtr,
                                      sns_scm_algo_inst_dbase[algoInstId]->outputData.memPtr,
                                      sns_scm_algo_inst_dbase[algoInstId]->stateData.memPtr,
                                      &sns_scm_algo_dbase[algoIndex]->algoApi,
                                      &sns_scm_qmd_state);

   //Deregister timer
   sns_scm_dereg_timer(algoIndex);

   //stop sensor data
   sns_scm_stop_sensor_data(algoInstId);

   //delete the algo instance
   sns_scm_delete_algo_inst(algoInstId);

   SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_DISABLE_ALGO,
                  sns_scm_algo_dbase[algoIndex]->serviceId, 
                  sns_scm_sensor_dbase[sns_scm_algo_dbase[algoIndex]->sensorIndex].sensorId,
                  algoInstId);

   return SNS_SUCCESS;
}

/*=========================================================================
  FUNCTION:  sns_scm_process_sam_resp
  =========================================================================*/
/*!
  @brief Processes the response received from sensors manager

  @param[i] smgrRespPtr: Pointer to sensors manager response message

  @return Sensors error code
*/
/*=======================================================================*/
static sns_err_code_e sns_scm_process_sam_resp(
   const void *samRespPtr,
   int32_t msgId)
{
   if (msgId == SNS_SAM_AMD_ENABLE_RESP_V01)
   {
      sns_sam_qmd_enable_resp_msg_v01* respPtr = 
      (sns_sam_qmd_enable_resp_msg_v01*)samRespPtr;

      if (respPtr->resp.sns_result_t != SNS_SUCCESS)
      {
         return((sns_err_code_e)(respPtr->resp.sns_err_t));
      }

      SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_SAM_RESP_INFO,
                     msgId, respPtr->instance_id, sns_scm_qmd_inst_id);

      //Update the qmd instance id
      sns_scm_qmd_inst_id = respPtr->instance_id;
   }
   else if (msgId == SNS_SAM_AMD_DISABLE_RESP_V01)
   {
      sns_sam_qmd_disable_resp_msg_v01* respPtr = 
      (sns_sam_qmd_disable_resp_msg_v01*)samRespPtr;

      if (respPtr->resp.sns_result_t != SNS_SUCCESS)
      {
         return((sns_err_code_e)(respPtr->resp.sns_err_t));
      }

      SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_SAM_RESP_INFO,
                     msgId, respPtr->instance_id, sns_scm_qmd_inst_id);
   }
   else
   {
      SNS_SCM_DEBUG1(HIGH, DBG_SCM_SAM_RESP_DROPPED, msgId);
   }

   return SNS_SUCCESS;
}

/*=========================================================================
  FUNCTION:  sns_scm_process_sam_ind
  =========================================================================*/
/*!
  @brief Processes sensor status indication received from sensors manager

  @param[i] smgrIndPtr: Pointer to sensors manager indication message

  @return Sensors error code
*/
/*=======================================================================*/
static sns_err_code_e sns_scm_process_sam_ind(
   const void *samIndPtr)
{
   sns_sam_qmd_report_ind_msg_v01* indPtr;
   uint8_t i;

   indPtr = (sns_sam_qmd_report_ind_msg_v01*)samIndPtr;

   if (indPtr->instance_id == sns_scm_qmd_inst_id)
   {
      sns_scm_qmd_state = indPtr->state;
   }

   if (sns_scm_qmd_state == SNS_SAM_MOTION_REST_V01)
   {
#ifdef SNS_PLAYBACK_SKIP_SMGR
      sns_scm_pb_next_report_time=indPtr->timestamp + sns_scm_pb_report_timeout;
      sns_scm_pb_update_next_report_time=true;
#endif

	  //enable algos
      for (i = 0;
           (i < SNS_SCM_NUM_ALGO_SVCS) && (sns_scm_algo_dbase[i] != NULL);
           i++)
      {
         if (sns_scm_algo_dbase[i]->enableAlgo)
         {
            if (sns_scm_sensor_dbase[sns_scm_algo_dbase[i]->sensorIndex].status ==
                SNS_SMGR_SENSOR_STATUS_ACTIVE_V01 ||
                sns_scm_sensor_dbase[sns_scm_algo_dbase[i]->sensorIndex].status ==
                SNS_SMGR_SENSOR_STATUS_ONE_CLIENT_V01)
            {
               sns_scm_enable_algo(i);
            }

         }
      }
   }
   else if (sns_scm_qmd_state == SNS_SAM_MOTION_MOVE_V01)
   {
      uint8_t algoInstCnt = sns_scm_algo_inst_count;

#ifdef SNS_PLAYBACK_SKIP_SMGR
      sns_scm_pb_update_next_report_time=false;
#endif

	  for (i=0; i < SNS_SCM_MAX_ALGO_INSTS && algoInstCnt > 0; i++)
      {
         if (sns_scm_algo_inst_dbase[i] != NULL)
         {
            sns_scm_disable_algo(sns_scm_algo_inst_dbase[i]->algoIndex);
            algoInstCnt --;
         }
      }
   }

   SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_SAM_IND_INFO,
                  sns_scm_qmd_inst_id, indPtr->instance_id, indPtr->state);

   return SNS_SUCCESS;
}

/*=========================================================================
  FUNCTION:  sns_scm_process_smgr_resp
  =========================================================================*/
/*!
  @brief Processes the response received from sensors manager

  @param[i] smgrRespPtr: Pointer to sensors manager response message

  @return Sensors error code
*/
/*=======================================================================*/
static sns_err_code_e sns_scm_process_smgr_resp(
   const void *smgrRespPtr,
   int32_t msgId)
{
   uint8_t dataReqId = SNS_SCM_INVALID_ID;
   uint32_t i;

   if (msgId == SNS_SMGR_REPORT_RESP_V01)
   {
      sns_smgr_periodic_report_resp_msg_v01* respPtr = 
      (sns_smgr_periodic_report_resp_msg_v01*)smgrRespPtr;

      if (respPtr->Resp.sns_result_t != SNS_SUCCESS)
      {
         return((sns_err_code_e)(respPtr->Resp.sns_err_t));
      }

      dataReqId = respPtr->ReportId;

      if (respPtr->AckNak == SNS_SMGR_RESPONSE_ACK_SUCCESS_V01 ||
          respPtr->AckNak == SNS_SMGR_RESPONSE_ACK_MODIFIED_V01)
      {
         SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_SMGR_RESP_SUCCESS, 
                        dataReqId, respPtr->AckNak, respPtr->ReasonPair_len);
         return SNS_SUCCESS;
      }

      if (dataReqId < SNS_SCM_MAX_DATA_REQS && 
          sns_scm_data_req_dbase[dataReqId] != NULL)
      {
         SNS_SCM_DEBUG3(HIGH, DBG_SCM_SMGR_RESP_ACK_VAL,
                        dataReqId, respPtr->AckNak, respPtr->ReasonPair_len);
      }
      else
      {
         SNS_SCM_DEBUG3(HIGH, DBG_SCM_SMGR_RESP_DATA_DROP,
                        dataReqId, respPtr->AckNak, respPtr->ReasonPair_len);
      }

      if (respPtr->ReasonPair_len < SNS_SMGR_MAX_NUM_REASONS_V01)
      {
         for (i = 0; i < respPtr->ReasonPair_len; i++)
         {
            SNS_SCM_DEBUG2(MEDIUM, DBG_SCM_SMGR_RESP_INFO,
                           respPtr->ReasonPair[i].ItemNum, 
                           respPtr->ReasonPair[i].Reason);
         }
      }
   }
   else if (msgId == SNS_SMGR_SENSOR_STATUS_RESP_V01)
   {
      sns_smgr_sensor_status_resp_msg_v01* respPtr =
      (sns_smgr_sensor_status_resp_msg_v01 *)smgrRespPtr;

      if (respPtr->Resp.sns_result_t != SNS_SUCCESS)
      {
         return((sns_err_code_e)(respPtr->Resp.sns_err_t));
      }

      for (i=0; i < SNS_SCM_MAX_SNS_MON &&
          sns_scm_sensor_dbase[i].sensorId != SNS_SCM_INVALID_ID; i++)
      {
         if (sns_scm_sensor_dbase[i].sensorId == respPtr->SensorID
             && sns_scm_sensor_dbase[i].status == SNS_SCM_INVALID_ID)
         {
            sns_scm_sensor_dbase[i].status = SNS_SMGR_SENSOR_STATUS_UNKNOWN_V01;
            break;
         }
      }
   }
   else if (msgId == SNS_SMGR_CAL_RESP_V01)
   {
      sns_smgr_sensor_cal_resp_msg_v01* respPtr =
      (sns_smgr_sensor_cal_resp_msg_v01*)smgrRespPtr;

      if (respPtr->Resp.sns_result_t != SNS_SUCCESS)
      {
         return((sns_err_code_e)(respPtr->Resp.sns_err_t));
      }
   }
   else
   {
      SNS_SCM_DEBUG1(HIGH, DBG_SCM_SMGR_RESP_DROPPED, msgId);
   }

   return SNS_SUCCESS;
}

/*=========================================================================
  FUNCTION:  sns_scm_process_smgr_ind
  =========================================================================*/
/*!
  @brief Processes the indication received from sensors manager

  @param[i] smgrIndPtr: Pointer to sensors manager indication message

  @return Sensors error code
*/
/*=======================================================================*/
static sns_err_code_e sns_scm_process_smgr_ind(const void *smgrIndPtr)
{
   sns_smgr_periodic_report_ind_msg_v01* indPtr;
   uint8_t dataReqId = SNS_SCM_INVALID_ID, i;
   uint32_t timestamp;
   bool flagStopSensorData = false;

   indPtr = (sns_smgr_periodic_report_ind_msg_v01*)smgrIndPtr;

   if (indPtr->status != SNS_SMGR_REPORT_OK_V01)
   {
      SNS_SCM_DEBUG2(MEDIUM, DBG_SCM_SMGR_IND_INFO,
                     dataReqId, indPtr->status);
      return SNS_ERR_FAILED;
   }

#ifdef SNS_PLAYBACK_SKIP_SMGR
   if(sns_scm_pb_update_next_report_time==true)
   {
      if(sns_scm_pb_next_report_time < indPtr->Item[0].TimeStamp)
      {
         //Simulate timeout
         sns_scm_timer_cb((void*)sns_scm_pb_gyro_cal_algo_svc_id);
         sns_scm_pb_next_report_time += sns_scm_pb_report_timeout;
      }
   }
#endif

   dataReqId = indPtr->ReportId;
   if (dataReqId >= SNS_SCM_MAX_DATA_REQS || 
       sns_scm_data_req_dbase[dataReqId] == NULL)
   {
      SNS_SCM_DEBUG1(MEDIUM, DBG_SCM_SMGR_IND_DROPPED, dataReqId);
      return SNS_ERR_FAILED;
   }

/*   if (indPtr->Item_len != sns_scm_data_req_dbase[dataReqId]->sensorCount)
   {
      SNS_SCM_DEBUG1(MEDIUM, DBG_SCM_SMGR_IND_DROPPED, dataReqId);
      return SNS_ERR_FAILED;
   }
*/ 

   if (indPtr->CurrentRate != sns_scm_data_req_dbase[dataReqId]->reportRate)
   {
      SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_SMGR_IND_RATE_INFO, 
                     dataReqId, indPtr->CurrentRate, 
                     sns_scm_data_req_dbase[dataReqId]->reportRate);
   }

   //Validate sensor data
   for (i = 0; i < indPtr->Item_len; i++)
   {
/*      if (indPtr->Item[i].SensorId != 
          sns_scm_sensor_dbase[sns_scm_data_req_dbase[dataReqId]->sensorDbase[i]].sensorId ||
          indPtr->Item[i].DataType !=
          sns_scm_sensor_dbase[sns_scm_data_req_dbase[dataReqId]->sensorDbase[i]].dataType)
      {
         SNS_SCM_DEBUG1(MEDIUM, DBG_SCM_SMGR_IND_DROPPED, dataReqId);
         return SNS_ERR_FAILED;
      }
*/
      if (indPtr->Item[i].ItemFlags == SNS_SMGR_ITEM_FLAG_INVALID_V01 ||
          indPtr->Item[i].ItemQuality == 
          SNS_SMGR_ITEM_QUALITY_INVALID_FAILED_SENSOR_V01 ||
          indPtr->Item[i].ItemQuality == 
          SNS_SMGR_ITEM_QUALITY_INVALID_NOT_READY_V01 ||
          indPtr->Item[i].ItemQuality == 
          SNS_SMGR_ITEM_QUALITY_INVALID_SUSPENDED_V01)
      {
         SNS_SCM_DEBUG1(MEDIUM, DBG_SCM_SMGR_IND_DROPPED, dataReqId);
         return SNS_ERR_FAILED;
      }
   }

   //use the timestamp of the first item i.e. the sensor data being calibrated
   timestamp = indPtr->Item[0].TimeStamp;

   //execute algorithms waiting for this sensor data
   for (i = 0; i < sns_scm_data_req_dbase[dataReqId]->algoInstCount; i++)
   {
      uint8_t algoInpSize, *algoInpPtr;
      uint8_t algoInstId = sns_scm_data_req_dbase[dataReqId]->algoInstDbase[i];
      sns_scm_algo_inst_s *algoInstPtr = sns_scm_algo_inst_dbase[algoInstId];
      uint8_t j;
      sns_err_code_e err;
      bool skipCal = false;


      //Update the algorithm specific input
      err = sns_scm_update_algo_specific_input(sns_scm_algo_dbase[algoInstPtr->algoIndex]->serviceId,
                                               indPtr,
                                               algoInstPtr->inputData.memPtr,
                                               &skipCal);
      if (err != SNS_SUCCESS)
      {   
         return err;
      }


      // skip calibration if flag is set
      if (skipCal)
      {
         sns_scm_stop_sensor_data(algoInstId);
         if (sns_scm_data_req_dbase[dataReqId] == NULL)
         {
            break;
         }
      }

      //copy sensor data to algorithm input
      for (j = 0, 
           algoInpPtr = algoInstPtr->inputData.memPtr,
           algoInpSize = algoInstPtr->inputData.memSize; 
          j < indPtr->Item_len; 
          j++)
      {

         uint8_t itemSize;

         if(indPtr->Item[j].DataType == SNS_SMGR_DATA_TYPE_SECONDARY_V01)
         {
            itemSize = sizeof(indPtr->Item[j].ItemData[0]);
         }
         else
         {
            itemSize = sizeof(indPtr->Item[j].ItemData);
         }

         if (itemSize > algoInpSize)
         {
            SNS_PRINTF_STRING_ID_ERROR_2(SNS_SCM_DBG_MOD,
                                         DBG_SCM_SMGR_IND_INVALID,
                                         algoInstPtr->inputData.memSize,
                                         indPtr->Item_len);
            return SNS_ERR_FAILED;
         }
         SNS_OS_MEMCOPY(algoInpPtr, indPtr->Item[j].ItemData, itemSize);

         algoInpPtr += itemSize;
         algoInpSize -= itemSize;
      }

      //execute the algorithm
      sns_scm_algo_dbase[algoInstPtr->algoIndex]->algoApi.
      sns_scm_algo_update(algoInstPtr->stateData.memPtr,
                          algoInstPtr->inputData.memPtr,
                          algoInstPtr->outputData.memPtr);

      sns_scm_process_algo_result(algoInstId,
         &flagStopSensorData,
         sns_scm_algo_dbase[sns_scm_algo_inst_dbase[algoInstId]->algoIndex]->serviceId,
         sns_scm_algo_inst_dbase[algoInstId]->outputData.memPtr,
         sns_scm_algo_inst_dbase[algoInstId]->stateData.memPtr);

      sns_scm_log_algo_result(
         algoInstId,
         timestamp,
         sns_scm_algo_dbase[sns_scm_algo_inst_dbase[algoInstId]->algoIndex]->serviceId,
         sns_scm_algo_inst_dbase[algoInstId]->inputData.memPtr,
         sns_scm_algo_inst_dbase[algoInstId]->outputData.memPtr);

      if(flagStopSensorData)
      {
         //stop sensor data
         sns_scm_stop_sensor_data(algoInstId);
         if (sns_scm_data_req_dbase[dataReqId] == NULL)
         {
            break;
         }
      }
   }

   return SNS_SUCCESS;
}

/*=========================================================================
  FUNCTION:  sns_scm_process_sensor_status_ind
  =========================================================================*/
/*!
  @brief Processes sensor status indication received from sensors manager

  @param[i] smgrIndPtr: Pointer to sensor status indication message

  @return Sensors error code
*/
/*=======================================================================*/
static sns_err_code_e sns_scm_process_sensor_status_ind(
   const void *smgrIndPtr)
{
   uint8_t i;
   bool found = false;
   sns_smgr_sensor_status_ind_msg_v01* indPtr 
   = (sns_smgr_sensor_status_ind_msg_v01*)smgrIndPtr;

   /*Update sensor state*/
   for (i=0; i<SNS_SCM_MAX_SNS_MON &&
       sns_scm_sensor_dbase[i].sensorId != SNS_SCM_INVALID_ID; i++)
   {
      if (sns_scm_sensor_dbase[i].sensorId == indPtr->SensorID)
      {
		 sns_scm_sensor_dbase[i].status = indPtr->SensorState;
         found = true;
         break;
      }
   }

   if (!found)
   {
      return SNS_ERR_FAILED;
   }

   switch (indPtr->SensorState)
   {
      case SNS_SMGR_SENSOR_STATUS_ACTIVE_V01:
         if (sns_scm_qmd_inst_id == SNS_SCM_INVALID_ID)
         {
            for (i = 0; (i < SNS_SCM_NUM_ALGO_SVCS) && (sns_scm_algo_dbase[i] != NULL); i++)
            {
               if(sns_scm_algo_dbase[i]->enableAlgo)
               {
                  sns_scm_send_qmd_start_req(&sns_scm_qmd_inst_id);
                  break;
               }
            }
         }
         else if (sns_scm_qmd_state == SNS_SAM_MOTION_REST_V01)
         {
            for (i = 0; 
                 (i < SNS_SCM_NUM_ALGO_SVCS) && (sns_scm_algo_dbase[i] != NULL); 
                 i++)
            {
               if (sns_scm_algo_dbase[i]->enableAlgo && 
                   indPtr->SensorID == 
                   sns_scm_sensor_dbase[sns_scm_algo_dbase[i]->sensorIndex].sensorId)
               {
                  sns_scm_enable_algo(i);
               }
            }
         }
         break;

      case SNS_SMGR_SENSOR_STATUS_IDLE_V01:

         if (sns_scm_qmd_inst_id != SNS_SCM_INVALID_ID)
         {
            uint8_t algoInstCnt = sns_scm_algo_inst_count;
            for (i=0; i < SNS_SCM_MAX_ALGO_INSTS && algoInstCnt > 0; i++)
            {
               if (sns_scm_algo_inst_dbase[i] != NULL)
               {
                  uint8_t algoIndex = sns_scm_algo_inst_dbase[i]->algoIndex;

                  if (indPtr->SensorID ==
                      sns_scm_sensor_dbase[sns_scm_algo_dbase[algoIndex]->sensorIndex].sensorId)
                  {
                     sns_scm_disable_algo(algoIndex);
                     break;
                  }
                  algoInstCnt--;
               }
            }

            if (sns_scm_algo_inst_count == 0 &&
                sns_scm_qmd_inst_id < SNS_SCM_QMD_PEND_ID)
            {
               sns_scm_send_qmd_stop_req();
            }
         }
         break;

      case SNS_SMGR_SENSOR_STATUS_ONE_CLIENT_V01:

         if ((sns_scm_qmd_inst_id != SNS_SCM_INVALID_ID) &&
             (indPtr->SensorID == SNS_SMGR_ID_ACCEL_V01) &&
             (sns_scm_qmd_state != SNS_SAM_MOTION_MOVE_V01))
         {
            uint8_t algoInstCnt = sns_scm_algo_inst_count;
            for (i=0; i < SNS_SCM_MAX_ALGO_INSTS && algoInstCnt > 0; i++)
            {
               if (sns_scm_algo_inst_dbase[i] != NULL)
               {
                  uint8_t algoIndex = sns_scm_algo_inst_dbase[i]->algoIndex;

                  if (indPtr->SensorID ==
                      sns_scm_sensor_dbase[sns_scm_algo_dbase[algoIndex]->sensorIndex].sensorId)
                  {
                     sns_scm_disable_algo(algoIndex);
                     break;
                  }
                  algoInstCnt--;
               }
            }
         }

         if (sns_scm_algo_inst_count == 0 &&
             sns_scm_qmd_inst_id < SNS_SCM_QMD_PEND_ID &&
             sns_scm_qmd_state != SNS_SAM_MOTION_MOVE_V01)
         {
            sns_scm_send_qmd_stop_req();
         }
         break;

      case SNS_SMGR_SENSOR_STATUS_UNKNOWN_V01:
      default:
         break;

   }

   return SNS_SUCCESS;
}


/*=========================================================================
  FUNCTION:  sns_scm_get_algo_handle
  =========================================================================*/
/*!
  @brief Get the handle to the specified algorithm in the algorithm database 

  @param[i] algoSvcId: algorithm service id

  @return handle to the specified algorithm if found,
          NULL otherwise
*/
/*=======================================================================*/
sns_scm_algo_s* sns_scm_get_algo_handle(
   uint8_t algoSvcId)
{
   uint8_t i;

   for (i = 0; i < SNS_SCM_NUM_ALGO_SVCS && sns_scm_algo_dbase[i] != NULL; i++)
   {
      if (sns_scm_algo_dbase[i]->serviceId == algoSvcId)
      {
         return sns_scm_algo_dbase[i];
      }
   }
   SNS_PRINTF_STRING_ID_ERROR_1(SNS_SCM_DBG_MOD,
                                DBG_SCM_GET_ALGO_INDX_ERR,
                                algoSvcId);

   return NULL;
}


/*=========================================================================
  FUNCTION:  sns_scm_process_msg
  =========================================================================*/
/*!
  @brief Process the messages from SCM input message queue  

  @return Sensors error code
*/
/*=======================================================================*/
static sns_err_code_e sns_scm_process_msg(void)
{
   void *msgPtr;
   sns_smr_header_s msgHdr;
   sns_err_code_e err;

   while ((msgPtr = sns_smr_rcv(SNS_SCM_MODULE)) != NULL)
   {
      err = sns_smr_get_hdr(&msgHdr, msgPtr);
      if (err == SNS_SUCCESS)
      {
         SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_PROCESS_MSG_RCVD,
                        msgHdr.msg_id, msgHdr.src_module, msgHdr.svc_num);

         if (msgHdr.src_module == SNS_MODULE_DSPS_SMGR)
         {
            if (msgHdr.msg_type == SNS_SMR_MSG_TYPE_RESP)
            {
               err = sns_scm_process_smgr_resp(msgPtr, msgHdr.msg_id);
            }
            else if (msgHdr.msg_type == SNS_SMR_MSG_TYPE_IND)
            {
               if (msgHdr.msg_id == SNS_SMGR_REPORT_IND_V01)
               {
                  err = sns_scm_process_smgr_ind(msgPtr);
               }
               else if (msgHdr.msg_id == SNS_SMGR_SENSOR_STATUS_IND_V01)
               {
                  err = sns_scm_process_sensor_status_ind(msgPtr);
               }
               else
               {
                  SNS_SCM_DEBUG1(HIGH, DBG_SCM_SMGR_RESP_DROPPED, msgHdr.msg_id);
               }
            }
            else
            {
               SNS_SCM_DEBUG1(HIGH, DBG_SCM_SMGR_RESP_TYPE_DROP, msgHdr.msg_type);
            }
         }
         else if (msgHdr.src_module == SNS_MODULE_DSPS_SAM)
         {
            if (msgHdr.msg_type == SNS_SMR_MSG_TYPE_RESP)
            {
               err = sns_scm_process_sam_resp(msgPtr, msgHdr.msg_id);
            }
            else if (msgHdr.msg_type == SNS_SMR_MSG_TYPE_IND)
            {
               err = sns_scm_process_sam_ind(msgPtr);
            }
            else
            {
               SNS_SCM_DEBUG1(HIGH, DBG_SCM_SAM_RESP_TYPE_DROP, msgHdr.msg_type);
            }
         }

         SNS_SCM_DEBUG2(MEDIUM, DBG_SCM_PROCESS_MSG_STATUS,
                        msgHdr.msg_type, err);
      }

      sns_smr_msg_free(msgPtr);
   }

   return SNS_SUCCESS;
}

/*=========================================================================
  FUNCTION:  sns_scm_reg_algo_svc
  =========================================================================*/
/*!
  @brief Register the algorithm with SCM. This is expected to be done
         at SCM initialization for all algorithms to be handled by SCM
  
  @param[i] algoSvcId: Algorithm service id
  @param[i] sensorId: sensor id
  @param[i] dataType: sensor datatype 

  @return Sensors error code
*/
/*=======================================================================*/
sns_err_code_e sns_scm_reg_algo_svc(
   uint8_t algoSvcId, uint8_t sensorId, uint8_t dataType)
{
   uint8_t algoIndex,sensorIndex;

   for (sensorIndex = 0; sensorIndex < SNS_SCM_MAX_SNS_MON; sensorIndex++)
   {
      if(sns_scm_sensor_dbase[sensorIndex].sensorId == SNS_SCM_INVALID_ID)
      {
         sns_scm_sensor_dbase[sensorIndex].sensorId = sensorId;
         sns_scm_sensor_dbase[sensorIndex].dataType = dataType;
         break;
      }
   }
   if(sensorIndex >= SNS_SCM_MAX_SNS_MON)
   {
      // maximum number of sensors that can be monitored already reached
      // cannot add additional sensor
      return SNS_ERR_FAILED;
   }

   for (algoIndex = 0; 
       algoIndex < SNS_SCM_NUM_ALGO_SVCS && sns_scm_algo_dbase[algoIndex] != NULL;
       algoIndex++)
   {
      if (sns_scm_algo_dbase[algoIndex]->serviceId == algoSvcId)
      {
         SNS_PRINTF_STRING_ID_ERROR_1(SNS_SCM_DBG_MOD,
                                      DBG_SCM_REG_ALGO_ERR,
                                      algoSvcId);
         return SNS_SUCCESS;
      }
   }

   if (algoIndex >= SNS_SCM_NUM_ALGO_SVCS)
   {
      return SNS_ERR_FAILED;
   }

   sns_scm_algo_dbase[algoIndex] = SNS_OS_MALLOC(SNS_SCM_DBG_MOD, 
                                                 sizeof(sns_scm_algo_s));
   SNS_ASSERT(sns_scm_algo_dbase[algoIndex] != NULL);

   sns_scm_algo_dbase[algoIndex]->serviceId = algoSvcId;
   sns_scm_algo_dbase[algoIndex]->timer = NULL;
   sns_scm_algo_dbase[algoIndex]->timeout = false;

   return(sns_scm_reg_algo(sns_scm_algo_dbase[algoIndex]));
}

/*=========================================================================
  FUNCTION:  sns_scm_reg_smgr
  =========================================================================*/
/*!
  @brief Register all algorithms

  @return None
*/
/*=======================================================================*/
static void sns_scm_reg_smgr(void)
{
   uint8_t algoIndex;

   for ( algoIndex = 0;
         algoIndex < SNS_SCM_NUM_ALGO_SVCS &&
         sns_scm_algo_dbase[algoIndex] != NULL &&
         sns_scm_algo_dbase[algoIndex]->enableAlgo;
         algoIndex++)
   {
      uint8_t sensorId = 
         sns_scm_sensor_dbase[sns_scm_algo_dbase[algoIndex]->sensorIndex].sensorId;
      if (sensorId != SNS_SCM_INVALID_ID)
      {
         sns_scm_reg_sensor_status(sensorId);
      }
   }
}

/*=========================================================================
  FUNCTION:  sns_scm_handle_report_timeout
  =========================================================================*/
/*!
  @brief Handle the timeout indicating periodic report to client is due

  @return None
*/
/*=======================================================================*/
static void sns_scm_handle_report_timeout(void)
{
   uint8_t algoIndex;
   sns_scm_algo_s* algoPtr;

   for (algoIndex = 0; algoIndex < SNS_SCM_NUM_ALGO_SVCS; algoIndex++)
   {
      algoPtr = sns_scm_algo_dbase[algoIndex];

      if (algoPtr != NULL && algoPtr->timer != NULL)
      {
         if (algoPtr->timeout == true)
         {
            algoPtr->timeout = false;

            if (sns_scm_sensor_dbase[algoPtr->sensorIndex].status == 
                SNS_SMGR_SENSOR_STATUS_ACTIVE_V01  ||
                sns_scm_sensor_dbase[algoPtr->sensorIndex].status ==
                SNS_SMGR_SENSOR_STATUS_ONE_CLIENT_V01)
            {
               uint8_t algoInstId = sns_scm_find_algo_inst(
                  algoPtr->serviceId, 
                  algoPtr->defConfigData.memPtr);

               if (algoInstId < SNS_SCM_MAX_ALGO_INSTS)
               {
                  //reset algorithm state
                  algoPtr->algoApi.sns_scm_algo_reset(
                     sns_scm_algo_inst_dbase[algoInstId]->configData.memPtr, 
                     sns_scm_algo_inst_dbase[algoInstId]->stateData.memPtr);

                  //start sensor data
                  sns_scm_req_sensor_data(algoInstId,
                        sns_scm_algo_dbase[algoIndex]->serviceId,
                        sns_scm_algo_inst_dbase[algoInstId]->configData.memPtr,
                        sns_scm_algo_dbase[algoIndex]->sensorIndex);

               }
            }
         }
      }
   }
}

/*=========================================================================
  FUNCTION:  sns_scm_process_events
  =========================================================================*/
/*!
  @brief Wait for events and process signaled events.

  @return None
*/
/*=======================================================================*/
static void sns_scm_process_events(void)
{
   uint8_t err;
   OS_FLAGS sigFlags;

   //wait for event
   sigFlags = sns_os_sigs_pend(sns_scm_sig_event, 
                               SNS_SCM_SIGNAL_MASK, 
                               OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 
                               0, 
                               &err);
   SNS_ASSERT(err == 0);

   //message event check
   if (sigFlags & SNS_SCM_MSG_SIG)
   {
      sns_scm_process_msg();
      sigFlags &= (~SNS_SCM_MSG_SIG);
   }
   //timer event check 
   if (sigFlags & SNS_SCM_REPORT_TIMER_SIG)
   {
      sns_scm_handle_report_timeout();
      sigFlags &= (~SNS_SCM_REPORT_TIMER_SIG);
   }
   if (sigFlags != 0)
   {
      SNS_SCM_DEBUG1(MEDIUM, DBG_SCM_PROCESS_EVT_UNKWN_EVT, sigFlags);
   }
}

/*=========================================================================
  FUNCTION:  sns_scm_task
  =========================================================================*/
/*!
  @brief Sensors calibration manager task

  @detail
  All algorithms are executed in this task context.
  Waits on events primarily from sensors manager or client.

  @param[i] argPtr: pointer to task argument

  @return None
*/
/*=======================================================================*/
static void sns_scm_task(
   void *argPtr)
{
   int8_t i;

   UNREFERENCED_PARAMETER(argPtr);

   //initialize algorithm database
   for (i = SNS_SCM_NUM_ALGO_SVCS-1; i >= 0; i--)
   {
      sns_scm_algo_dbase[i] = NULL;
   }

   //initialize algorithm instance database
   for (i = SNS_SCM_MAX_ALGO_INSTS-1; i >= 0; i--)
   {
      sns_scm_algo_inst_dbase[i] = NULL;
   }
   sns_scm_algo_inst_count = 0;

   //initialize sensor data request database
   for (i = SNS_SCM_MAX_DATA_REQS-1; i >= 0; i--)
   {
      sns_scm_data_req_dbase[i] = NULL;
   }
   sns_scm_data_req_count = 0;

   for (i = SNS_SCM_MAX_SNS_MON-1; i >= 0; i--)
   {
      sns_scm_sensor_dbase[i].sensorId = SNS_SCM_INVALID_ID;
      sns_scm_sensor_dbase[i].dataType = SNS_SCM_INVALID_ID;
      sns_scm_sensor_dbase[i].status = SNS_SCM_INVALID_ID;
      sns_scm_sensor_dbase[i].fac_cal_reg_id = SNS_SCM_INVALID_ID;
      sns_scm_sensor_dbase[i].auto_cal_reg_id = SNS_SCM_INVALID_ID;
   }

   //register algorithms
   sns_scm_reg_algos();

   //register SCM to SMR so as to receive messages
   sns_smr_register(SNS_SCM_MODULE, sns_scm_sig_event, SNS_SCM_MSG_SIG);
   
   //initialize calibration algorithms
   sns_scm_init_cal_algos(sns_scm_sensor_dbase,sns_scm_algo_dbase,sns_scm_sig_event);

   //register SCM to SMGR for sensor status reports
   sns_scm_reg_smgr();

   SNS_SCM_DEBUG0(MEDIUM, DBG_SCM_TASK_STARTED);

   sns_init_done();

   while (1)
   {
      //wait for events and process received events
      sns_scm_process_events();
   }
}

/*---------------------------------------------------------------------------
 * Externalized Function Definitions
 * -------------------------------------------------------------------------*/
/*=========================================================================
  FUNCTION:  sns_scm_init
  =========================================================================*/
/*!
  @brief Sensors calibration manager initialization.
         Creates the SCM task and internal databases.

  @return Sensors error code
*/
/*=======================================================================*/
sns_err_code_e sns_scm_init(void)
{
   uint8_t err;

   //initialize events
   sns_scm_sig_event = sns_os_sigs_create((OS_FLAGS)0x0, &err);
   SNS_ASSERT(sns_scm_sig_event != NULL);

   //create the SCM task
   err = sns_os_task_create(sns_scm_task,
                            NULL,
                            &sns_scm_task_stk[SNS_SCM_MODULE_STK_SIZE-1],
                            SNS_SCM_MODULE_PRIORITY);
   SNS_ASSERT(err == 0);

   return SNS_SUCCESS;
}

/*=========================================================================
  FUNCTION:  sns_scm_get_sensor_index
  =========================================================================*/
/*!
  @brief Get the index into the sensor status database for specified sensor

  @param[i] sensorId: algorithm service id

  @return index for the specified sensor in database if found,
          SNS_SCM_INVALID_ID otherwise
*/
/*=======================================================================*/
uint8_t sns_scm_get_sensor_index(
   uint8_t sensorId)
{
   uint8_t i;

   for (i=0; i < SNS_SCM_MAX_SNS_MON &&
       sns_scm_sensor_dbase[i].sensorId != SNS_SCM_INVALID_ID; i++)
   {
      if (sns_scm_sensor_dbase[i].sensorId == sensorId)
      {
         return i;
      }
   }

   return SNS_SCM_INVALID_ID;
}

//retained for future use
#if 0
 
/*=========================================================================
  FUNCTION:  sns_scm_get_algo_index
  =========================================================================*/
/*!
  @brief Get the index into the algorithm database for the specified algorithm

  @param[i] algoSvcId: algorithm service id

  @return algorithm index for the specified algorithm if found,
          SNS_SCM_INVALID_ID otherwise
*/
/*=======================================================================*/
static uint8_t sns_scm_get_algo_index(
   uint8_t algoSvcId)
{
   uint8_t i;

   for (i = 0; i < SNS_SCM_NUM_ALGO_SVCS && sns_scm_algo_dbase[i] != NULL; i++)
   {
      if (sns_scm_algo_dbase[i]->serviceId == algoSvcId)
      {
         return i;
      }
   }
   SNS_PRINTF_STRING_ID_ERROR_1(SNS_SCM_DBG_MOD,
                                DBG_SCM_GET_ALGO_INDX_ERR,
                                algoSvcId);

   return SNS_SCM_INVALID_ID;
}

/*=========================================================================
  FUNCTION:  sns_scm_dereg_sensor_status
  =========================================================================*/
/*!
  @brief Send a request to sensors manager to stop sensor data received by  
  specified algorithm

  @param[i] sensorId: sensor id

  @return None
*/
/*=======================================================================*/
static void sns_scm_dereg_sensor_status(
   uint8_t sensorId)
{
   sns_smr_header_s msgHdr;

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
   msgPtr->Action = SNS_SMGR_SENSOR_STATUS_DEL_V01;
   msgPtr->ReqDataTypeNum = 1;

   sns_smr_set_hdr(&msgHdr, msgPtr);
   if (sns_smr_send(msgPtr) != SNS_SUCCESS)
   {
      //free the message
      sns_smr_msg_free(msgPtr);
   }

   SNS_SCM_DEBUG3(MEDIUM, DBG_SCM_REQ_SNSR_STATUS_INFO,
                  SNS_SMGR_SENSOR_STATUS_DEL_V01, 
                  sensorId, 
                  1);
}

/*=========================================================================
  FUNCTION:  sns_scm_send_resp_msg
  =========================================================================*/
/*!
  @brief Sends response message for the specified request

  @param[i] reqMsgPtr: Pointer to request message for which
            response needs to be sent
  @param[i] respMsgPtr: Pointer to response message body,
            to be sent with header
  @param[i] respMsgBodyLen: Response message body length
            (excluding the header part)

  @return None
*/
/*=======================================================================*/
static void sns_scm_send_resp_msg(
   const uint8_t *reqMsgPtr,
   void *respMsgPtr,
   uint16_t respMsgBodyLen)
{
   sns_smr_header_s reqMsgHdr, respMsgHdr;

   sns_smr_get_hdr(&reqMsgHdr, reqMsgPtr);
   if (reqMsgHdr.msg_type == SNS_SMR_MSG_TYPE_REQ)
   {
      respMsgHdr.dst_module = reqMsgHdr.src_module;
      respMsgHdr.src_module = reqMsgHdr.dst_module;
      respMsgHdr.svc_num = reqMsgHdr.svc_num;
      respMsgHdr.msg_type = SNS_SMR_MSG_TYPE_RESP;
      respMsgHdr.priority = SNS_SMR_MSG_PRI_HIGH;
      respMsgHdr.body_len = respMsgBodyLen;

      respMsgHdr.txn_id = reqMsgHdr.txn_id;
      respMsgHdr.ext_clnt_id = reqMsgHdr.ext_clnt_id;

      //request and response messages are assumed to have same message ids
      respMsgHdr.msg_id = reqMsgHdr.msg_id;

      sns_smr_set_hdr(&respMsgHdr, respMsgPtr);
      if (sns_smr_send(respMsgPtr) != SNS_SUCCESS)
      {
         //free the message
         sns_smr_msg_free(respMsgPtr);
      }

      SNS_SCM_DEBUG3(HIGH, DBG_SCM_SEND_RSP_INFOMSG,
                     reqMsgHdr.msg_id, reqMsgHdr.src_module, reqMsgHdr.svc_num);
   }
}
/*=========================================================================
  FUNCTION:  sns_scm_process_shutdown_msg
  =========================================================================*/
/*!
  @brief Process a shutdown message from SCM input message queue  
  
  @return none
*/
/*=======================================================================*/
void sns_scm_process_shutdown_msg(void)  
{
   uint8_t i;
   sns_smr_header_s msgHdr;
   sns_reg_group_write_req_msg_v02 *msgPtr;
   uint8_t msgSize = 0;
   sns_err_code_e err;
   int32_t * dataPtr;
   /*for all instantiated algorithms*/
   for ( i = 0; i < SNS_SCM_MAX_ALGO_INSTS && sns_scm_algo_inst_dbase[i] != NULL; i++ )
   {
      sns_scm_algo_inst_s * algoInstPtr = sns_scm_algo_inst_dbase[i];
      /*If the algorithm is gyro_cal */
      if ( SNS_SCM_GYRO_CAL_SVC == sns_scm_algo_dbase[algoInstPtr->algoIndex]->serviceId )
      {
         gyro_cal_output_s * gyroCalOutput = algoInstPtr->outputData.memPtr;
         msgSize = sizeof(sns_smgr_sensor_cal_req_msg_v01);
         msgPtr = (sns_reg_group_write_req_msg_v02 *) sns_smr_msg_alloc(SNS_SCM_DBG_MOD,msgSize);

         SNS_ASSERT(msgPtr != NULL);

         msgHdr.src_module = SNS_SCM_MODULE;
         msgHdr.svc_num = SNS_REG2_SVC_ID_V01;
         msgHdr.msg_type = SNS_SMR_MSG_TYPE_REQ;
         msgHdr.priority = SNS_SMR_MSG_PRI_HIGH;
         msgHdr.msg_id = SNS_REG_GROUP_WRITE_REQ_V02;
         msgHdr.body_len = msgSize;

         /*Important Note: If not all 6 bias&scale values are set by a single algorithm, then they must
         all be sent as single write messages, not a group write*/

         msgPtr->group_id = SNS_REG_SCM_GROUP_GYRO_DYN_CAL_PARAMS_V02;
         msgPtr->data_len = 6 * sizeof(int32_t);
         dataPtr = (int32_t*)msgPtr->data;

         dataPtr[0] = gyroCalOutput->bias[0];    
         dataPtr[1] = gyroCalOutput->bias[1];
         dataPtr[2] = gyroCalOutput->bias[2];
         dataPtr[3] = 65536;                 
         dataPtr[4] = 65536;                 
         dataPtr[5] = 65536;                 

         sns_smr_set_hdr(&msgHdr, msgPtr);
         err = sns_smr_send(msgPtr);
         if (err != SNS_SUCCESS)
         {
            //free the message
            sns_smr_msg_free(msgPtr);
         }
      }
   }
}

#endif

