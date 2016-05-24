#ifndef SNS_SCM_PRIV_H
#define SNS_SCM_PRIV_H

/*============================================================================
  @file sns_scm_priv.h

  Sensors calibration manager header

  This header file contains the private interface of Sensors Calibration Manager

  Copyright (c) 2011-2012 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
============================================================================*/

/*---------------------------------------------------------------------------
* Include Files
* -------------------------------------------------------------------------*/
#include "stdbool.h"
#include "sns_common.h"
#include "sns_debug_api.h"
#include "sns_em.h"
#include "sns_smr.h"

/*---------------------------------------------------------------------------
* Preprocessor Definitions and Constants
* -------------------------------------------------------------------------*/

#if defined(SNS_DSPS_BUILD) || (defined(SNS_PCSIM) && !defined(ENABLE_APPS_PLAYBACK))

#define SNS_SCM_MODULE           SNS_MODULE_DSPS_SCM
#define SNS_SCM_MODULE_PRIORITY  SNS_MODULE_PRI_DSPS_SCM
#define SNS_SCM_DBG_MOD          SNS_DBG_MOD_DSPS_SCM
#define SNS_SCM_MODULE_STK_SIZE  SNS_MODULE_STK_SIZE_DSPS_SCM

#else

#define SNS_SCM_MODULE           SNS_MODULE_APPS_SCM
#define SNS_SCM_MODULE_PRIORITY  SNS_MODULE_PRI_APPS_SCM
#define SNS_SCM_DBG_MOD          SNS_DBG_MOD_APPS_SCM
#define SNS_SCM_MODULE_STK_SIZE  SNS_MODULE_STK_SIZE_APPS_SCM

#endif


#ifndef SNS_PCSIM
#define SNS_SCM_DEBUG
#endif

#ifdef SNS_SCM_DEBUG
   #define SNS_SCM_DEBUG0(level, msg)              \
   SNS_PRINTF_STRING_ID_##level##_0(SNS_SCM_DBG_MOD, msg)
   #define SNS_SCM_DEBUG1(level, msg, p1)          \
   SNS_PRINTF_STRING_ID_##level##_1(SNS_SCM_DBG_MOD, msg, p1)
   #define SNS_SCM_DEBUG2(level, msg, p1, p2)      \
   SNS_PRINTF_STRING_ID_##level##_2(SNS_SCM_DBG_MOD, msg, p1, p2)
   #define SNS_SCM_DEBUG3(level, msg, p1, p2, p3)  \
   SNS_PRINTF_STRING_ID_##level##_3(SNS_SCM_DBG_MOD, msg, p1, p2, p3)
#else
   #define SNS_SCM_DEBUG0(level, msg)
   #define SNS_SCM_DEBUG1(level, msg, p1)
   #define SNS_SCM_DEBUG2(level, msg, p1, p2)
   #define SNS_SCM_DEBUG3(level, msg, p1, p2, p3)

#endif

#define SNS_SCM_MSG_SIG          0x1
#define SNS_SCM_REPORT_TIMER_SIG 0x2
#define SNS_SCM_SIGNAL_MASK      (SNS_SCM_MSG_SIG | SNS_SCM_REPORT_TIMER_SIG)

#define SNS_SCM_MAX_ALGO_INSTS               5
#define SNS_SCM_MAX_DATA_REQS                10
#define SNS_SCM_MAX_ALGO_INSTS_PER_DATA_REQ  5
#define SNS_SCM_MAX_SENSORS_PER_DATA_REQ     3
#define SNS_SCM_MAX_SNS_MON                  5

//All algorithm services must support the following messages
//with specified message ids
#define SNS_SCM_ALGO_ENABLE_REQ  0x0
#define SNS_SCM_ALGO_DISABLE_REQ 0x1

#define SNS_SCM_INVALID_ID       0xFF

#define GRAVITY (9.806)

// AMD configuration as Gradient Motion Detector (GMD)
#define GMD_SAMP_RATE (20.0) // Hz
#define GMD_VAR_THRESH (FX_FLTTOFIX_Q16((30*GRAVITY/1000)*(30*GRAVITY/1000)))
                              // (30 milliG)^2 in Q16 m2/s4
#define GMD_WIN_LEN (FX_FLTTOFIX_Q16(10.0/GMD_SAMP_RATE))// 10 samples in Q16 s

//QMD instance id when request is pending
#define SNS_SCM_QMD_PEND_ID             (0xFE)

/*---------------------------------------------------------------------------
* Type Declarations
* -------------------------------------------------------------------------*/
/*=======================================================================*/
//generic structure for algorithm memory management
//(config, state, input, output)
typedef struct {
   uint32_t memSize;
   void *memPtr;
} sns_scm_algo_mem_s;

//Algorithm API
typedef struct {
   //Algorithm state memory requirement query API
   /*=========================================================================
     FUNCTION:  sns_scm_algo_mem_req
     =======================================================================*/
   /*!
       @brief Query interface to get memory requirement of algorithm state
       based on specified configuration

       @param[i] configPtr: Pointer to algorithm configuration

       @return Size of memory required for algorithm state
   */
   /*=======================================================================*/
   int32_t (*sns_scm_algo_mem_req)(void *configPtr);

   /*=========================================================================
     FUNCTION:  sns_scm_algo_reset
     =======================================================================*/
   /*!
       @brief Reset/Initialize the state of the algorithm instance

       @param[i] configPtr: Pointer to algorithm configuration
       @param[i] statePtr: Pointer to algorithm state

       @return Pointer to algorithm state if successful
       NULL if error
   */
   /*=======================================================================*/
   void* (*sns_scm_algo_reset)(
      void *configPtr,
      void *statePtr);

   /*=========================================================================
     FUNCTION:  sns_scm_algo_update
     =======================================================================*/
   /*!
       @brief Execute the algorithm to generate output using specified input

       @param[i] statePtr: Pointer to algorithm state
       @param[i] inputPtr: Pointer to input data
       @param[o] outputPtr: Pointer to output data

       @return Error code
   */
   /*=======================================================================*/
   void (*sns_scm_algo_update)(
      void *statePtr,
      void *inputPtr,
      void *outputPtr);
} sns_scm_algo_api_s;

//registry item type
typedef enum
{
  SNS_SCM_REG_ITEM_TYPE_NONE,
  SNS_SCM_REG_ITEM_TYPE_SINGLE,
  SNS_SCM_REG_ITEM_TYPE_GROUP
} sns_scm_reg_item_type_e;

//algorithm specific information
typedef struct {
   sns_scm_algo_api_s algoApi;         /*algorithm API*/
   sns_scm_algo_mem_s defConfigData;   /*default algorithm configuration*/
   sns_scm_reg_item_type_e regItemType;/*registry item type*/
   sns_em_timer_obj_t timer;           /*timer for periodic execution*/
   uint32_t defInputDataSize;          /*default algorithm input size*/
   uint32_t defOutputDataSize;         /*default algorithm output size*/
   uint32_t regItemId;                 /*registry item id */
   uint32_t period;                    /*duration of periodic execution*/
   uint8_t serviceId;                  /*algorithm service id*/
   uint8_t sensorIndex;                /*sensor under calibration*/
   bool enableAlgo;                    /*toggle algorithm usage*/
   bool timeout;                       /*timer timeout indication*/
} sns_scm_algo_s;

//algorithm instance specific information
typedef struct {
   sns_scm_algo_mem_s configData; /*pointer to algorithm config*/
   sns_scm_algo_mem_s stateData;  /*pointer to algorithm state*/
   sns_scm_algo_mem_s inputData;  /*pointer to algorithm input data*/
   sns_scm_algo_mem_s outputData; /*pointer to algorithm output data*/
   uint8_t algoIndex;             /*index to algorithm in database*/
} sns_scm_algo_inst_s;

//sensor information
typedef struct {
   uint8_t sensorId;                /*sensor id*/
   uint8_t dataType;                /*sensor data type*/
   uint8_t status;                  /*sensor status*/
   uint32_t fac_cal_reg_id;         /*factory cal registry id*/
   int32_t fac_cal_bias_params[3];  /*factory cal bias parameters*/
   int32_t fac_cal_scale_params[3]; /*factory cal scale parameters*/
   uint32_t auto_cal_reg_id;        /*auto cal registry id*/
   int32_t auto_cal_bias_params[3]; /*auto cal bias parameters*/
   int32_t auto_cal_scale_params[3];/*auto cal scale parameters*/
} sns_scm_sensor_s;

//sensor data request
typedef struct {
   uint32_t reportRate;          /*sensor data report rate*/
   uint8_t sensorCount;          /*sensor count*/
   uint8_t algoInstCount;        /*algorithm instance count*/
   uint8_t sensorDbase[SNS_SCM_MAX_SENSORS_PER_DATA_REQ];
                                 /*sensors whose data is requested*/
   uint8_t algoInstDbase[SNS_SCM_MAX_ALGO_INSTS_PER_DATA_REQ];
                                 /*algorithm instances requesting data*/
} sns_scm_data_req_s;

//calibration algorithms
typedef enum
{
   SNS_SCM_GYRO_CAL_SVC = 0,
   SNS_SCM_ACCEL_CAL_SVC = 1,
   SNS_SCM_NUM_ALGO_SVCS
} sns_scm_algo_svc_e;

/*---------------------------------------------------------------------------
* Function Declarations
* -------------------------------------------------------------------------*/
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
   OS_FLAG_GRP *sigEventFlags);

/*=========================================================================
  FUNCTION:  sns_scm_dereg_init_timer
  =========================================================================*/
/*!
  @brief Deregister timer for SAM initialization

  @return Sensors error code
*/
/*=======================================================================*/
sns_err_code_e sns_scm_dereg_init_timer(void);

/*=========================================================================
  FUNCTION:  sns_scm_process_init_events
  =========================================================================*/
/*!
  @brief Process initialization events

  @param[i] numRegQuery: Number of queries made to registry
  @param[i] snsDbaseHandle: Handle to the SCM internal sensor database

  @return None
*/
/*=======================================================================*/
void sns_scm_process_init_events(
   uint8_t numRegQuery,
   sns_scm_sensor_s* snsDbaseHandle);

/*=========================================================================
  FUNCTION:  sns_scm_reg_algo
  =========================================================================*/
/*!
  @brief Register specified algorithm

  @param[i] algoPtr: pointer to algorithm

  @return None
*/
/*=======================================================================*/
sns_err_code_e sns_scm_reg_algo(
   sns_scm_algo_s* algoPtr);

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
   uint8_t sensorId);

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
   uint8_t algoSvcId);

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
   uint8_t sensorId);

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
   uint32_t regItemId);

/*=========================================================================
  FUNCTION:  sns_scm_store_auto_cal_params
  =========================================================================*/
/*!
  @brief Store gyro dynamic calibration parameters

  @param[i] gyroCalOutput: pointer to gyro calibration algo output

  @return Sensors error code
*/
/*=======================================================================*/
sns_err_code_e sns_scm_store_gyro_auto_cal_params(
   void* gyroCalOutput);

/*=========================================================================
  FUNCTION:  sns_scm_motion_state_change_algo_update
  =========================================================================*/
/*!
  @brief Notifies algorithm if required about the occurrence of end of rest

  @param[i] serviceId: Index of the algorithm instance database
  @param[i] algo_input_p: pointer to input struct of algo instance
  @param[i] algo_output_p: pointer to output struct of algo instance
  @param[i] algo_state_p: pointer to state struct of algo instance
  @param[i] algo_api_p: pointer to algo api

  @return None
*/
/*=======================================================================*/
void sns_scm_motion_state_change_algo_update(uint8_t serviceId,
                                           void *algo_input_p,
                                           void *algo_output_p,
                                           void *algo_state_p,
                                           sns_scm_algo_api_s* algo_api_p,
                                           uint8_t* sns_scm_qmd_state_p);

/*=========================================================================
  FUNCTION:  sns_scm_log_algo_result
  =========================================================================*/
/*!
  @brief Log algorithm result

  @param[i] algoInstId: algorithm instance id
  @param[i] timestamp: timestamp on the algorithm input data
  @param[i] algo_input_p: pointer to input struct of algo instance
  @param[i] algo_output_p: pointer to output struct of algo instance

  @return None
*/
/*=======================================================================*/
void sns_scm_log_algo_result(
   uint8_t algoInstId,
   uint32_t timestamp,
   uint8_t serviceId,
   void* algo_input_p,
   void* algo_output_p);

/*=========================================================================
  FUNCTION:  sns_scm_process_algo_result
  =========================================================================*/
/*!
  @brief Process specified algorithm result

  @param[i] algoInstId: index to algorithm instance in the database
  @param[i] flagStopSensorData_p: flag to determine if sensor data should be stopped
  @param[i] serviceId: algo service ID
  @param[i] algo_output_p: pointer to output struct of algo instance
  @param[i] algo_state_p: pointer to state struct of algo instance

  @return Sensors error code
*/
/*=======================================================================*/
void sns_scm_process_algo_result(uint8_t algoInstId,
                                        bool* flagStopSensorData_p,
                                        uint8_t serviceId,
                                        void* algo_output_p,
                                        void* algo_state_p);

/*=========================================================================
  FUNCTION:  sns_scm_update_algo_specific_input
  =========================================================================*/
/*!
  @brief Update algorithm input structure

  @param[i] algoSvcId: algorithm service Id
  @param[i] indPtr: pointer to SMGR report indication message
  @param[io] algoInpPtr: pointer to algorithm input structure
  @param[io] skipCal: pointer to calibration skip flag

  @return Sensors error code
*/
/*=======================================================================*/
sns_err_code_e sns_scm_update_algo_specific_input(
   uint8 algoSvcId,
   const void* indPtr,
   void *algoInpPtr,
   bool *skipCal);

/*=========================================================================
  FUNCTION:  sns_scm_log_algo_config
  =========================================================================*/
/*!
  @brief Log algorithm configuration

  @param[i] algoInstId: algorithm instance id
  @param[i] algoCfgPtr: ptr to config struct for algo instance
  @param[i] serviceId: algorithm service ID

  @return None
*/
/*=======================================================================*/
void sns_scm_log_algo_config(uint8_t algoInstId,
                             void* algoCfgPtr,
                             uint8_t serviceId);

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
   uint8_t sensorIndex[]);

/*=========================================================================
  FUNCTION:  sns_scm_req_sensor_data
  =========================================================================*/
/*!
  @brief Request sensor data for algorithm

  @param[i] algoInstId: algorithm instance id
  @param[i] serviceId: algorithm service id
  @param[i] algo_config_p: pointer to algo instance config struct
  @param[i] sensorIndex: index of sensor in sensor database

  @return None
*/
/*=======================================================================*/
void sns_scm_req_sensor_data(uint8_t algoInstId,
                             uint8_t serviceId,
                             void* algo_config_p,
                             uint8_t sensorIndex);

/*=========================================================================
  FUNCTION:  sns_scm_send_qmd_start_req
  =========================================================================*/
/*!
  @brief Send a request to sensors algorithm manager for QMD

  @param[i] sns_scm_qmd_inst_id_p: pointer to qmd instance ID

  @return Sensors error code
*/
/*=======================================================================*/
sns_err_code_e sns_scm_send_qmd_start_req(uint8_t* sns_scm_qmd_inst_id_p);

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
   uint8_t calType);

/*=========================================================================
  FUNCTION:  sns_scm_process_reg_data
  =========================================================================*/
/*!
  @brief Process data received from registry

  @param[i] regItemType - registry item type
  @param[i] regItemId - registry item identifier
  @param[i] regDataLen - registry data length
  @param[i] regDataPtr - registry data pointer
  @param[i] sns_scm_sensor_dbase - database of sensor scm sensor information

  @return None
*/
/*=======================================================================*/
 void sns_scm_process_reg_data(
   sns_scm_reg_item_type_e regItemType,
   uint16_t regItemId,
   uint32_t regDataLen,
   const uint8_t* regDataPtr,
   sns_scm_sensor_s* sns_scm_sensor_dbase);

 /*=========================================================================
  FUNCTION:  sns_scm_init_cal_algos
  =========================================================================*/
/*!
  @brief Initialize calibration algorithms

  @param[i] sns_scm_sensor_dbase - database of sensor scm sensor information
  @param[i] sns_scm_algo_dbase - database of scm algo information
  @param[i] sns_scm_sig_event - SCM event signal

  @return none
*/
/*=======================================================================*/
void sns_scm_init_cal_algos(sns_scm_sensor_s* sns_scm_sensor_dbase,
                            sns_scm_algo_s** sns_scm_algo_dbase,
                            OS_FLAG_GRP* sns_scm_sig_event);

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
   uint8_t algoSvcId, uint8_t sensorId, uint8_t dataType);

/*=========================================================================
  FUNCTION:  sns_scm_reg_algos
  =========================================================================*/
/*!
  @brief Register all algorithms

  @return None
*/
/*=======================================================================*/
void sns_scm_reg_algos(void);

#endif /*#ifndef SNS_SCM_PRIV_H*/
