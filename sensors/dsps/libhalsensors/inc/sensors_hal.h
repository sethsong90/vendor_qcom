#ifndef SENSORS_HAL_H
#define SENSORS_HAL_H

/*============================================================================
  @file sensors_hal.h

  @brief
  Common code across the Sensors HAL implementation.

  Copyright (c) 2010-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
============================================================================*/

/*===========================================================================
                           INCLUDE FILES
===========================================================================*/
/* ADB logcat */
#define LOG_TAG "qcom_sensors_hal"
#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0
#define LOG_NIDEBUG 0
#define LOG_NVDEBUG 0

#include <cutils/log.h>
#include <sensors.h>
#include <stdbool.h>
#include <math.h>
#include "log.h"
#include "sns_smgr_api_v01.h"
#include "sensor1.h"
#include "sensors_hal_sam.h"

/*===========================================================================
                   PREPROCESSOR DEFINTIONS
===========================================================================*/

/* timeout in ms used in mutex wait */
#define TIME_OUT_MS 1000

/* max number of sensors */
#define MAX_NUM_SENSORS 50

/* Unit conversion between sensor1 and android */
#define UNIT_Q16                     (65536.0f)
#define UNIT_CONVERT_Q16             (1.0/65536.0)
#define UNIT_CONVERT_ACCELERATION    UNIT_CONVERT_Q16      /* Android (ms2,
                                                              sensor1 (ms2 in q16) */
#define UNIT_CONVERT_MAGNETIC_FIELD  100*UNIT_CONVERT_Q16  /* Android (uTesla)
                                                              Sensor1 (Gauss in q16) */
#define UNIT_CONVERT_TEMPERATURE     UNIT_CONVERT_Q16      /* Android (deg C),
                                                              Sensor1 (deg C in q16)*/
#define UNIT_CONVERT_LIGHT           UNIT_CONVERT_Q16      /* Android (lux),
                                                              sensor1 (lux in q16) */
#define UNIT_CONVERT_PROXIMITY       100*UNIT_CONVERT_Q16  /* Android (cm),
                                                              sensor1 (meter in q16) */
#define UNIT_CONVERT_GYRO            UNIT_CONVERT_Q16      /* Android(rad/s),
                                                              sensor1(rad/s in q16)*/
#define UNIT_CONVERT_POWER           1/1000                /* android(mA),
                                                              Sensor1(uA in q16)*/
#define UNIT_CONVERT_PRESSURE        UNIT_CONVERT_Q16      /* Android (hPa),
                                                             Sensor1 (hPa in q16)*/
#define UNIT_CONVERT_RELATIVE_HUMIDITY UNIT_CONVERT_Q16    /* Android (percentage humidity),
                                                             Sensor1 (percentage humidity in q16)*/
#define UNIT_CONVERT_COLOR_TEMP      UNIT_CONVERT_Q16      /* Android (kelvin),
                                                              sensor1 (kelvin in q16) */

#define RAD2DEG 180.0f/M_PI
#define MIN_FLT_TO_AVOID_SINGULARITY 0.0001f

#define RAD_Q16_TO_DEG_FLT  (UNIT_CONVERT_Q16 * RAD2DEG)
#define FX_CONV(a,q1,q2)     (((q2)>(q1))?(a)<<((q2)-(q1)):(a)>>((q1)-(q2)))
#define FX_QFACTOR           (16)
#define FX_CONV_Q16(a,q1)    (FX_CONV(a,q1,FX_QFACTOR))

#define FX_FLTTOFIX(f,q)     ((int32_t)( (f)*(1<<(q))+(((f)>(0.0))?(0.5):(-0.5))))
#define FX_FLTTOFIX_Q16(d)   (FX_FLTTOFIX(d,FX_QFACTOR))

#define FX_FIXTOFLT(i,q)     (((double)(i))/((double)(1<<(q))))

#define INVALID_INSTANCE_ID   0xFF

#define OEM_LIB_PATH      "/system/lib/hw/sensors.oem.so"

/* Timing related macros */
#define NSEC_PER_SEC       1000000000LL
#define USEC_PER_SEC       1000000LL
#define USEC_TO_MSEC(usec) ((usec)/1000L)
#define USEC_TO_NSEC(usec) ((usec)*1000L)
#define NSEC_TO_MSEC(nsec) ((nsec)/1000000LL)
#define NSEC_TO_SEC(nsec)  ((nsec)/1000000000LL)
#define HZ_TO_USEC(hz)     (1000000LL/(hz))
#define HZ_TO_NSEC(hz)     (1000000000LL/(hz))
#define MSEC_TO_HZ(ms)     (1000.0/(ms))
#define NSEC_TO_HZ(ns)     (1000000000.0/((float)ns))
#define DSPS_HZ            32768LL

/* If two consecutive sensor samples have non-increasing timestamps:
 * If the second is within X of the first, edit the second. */
#define TS_CORRECT_THRESH 200000

/* If a new sensor sample contains a timestamp that is within X of 0, while
 * the previous sample contained a timestamp within X of UINT32_MAX, the SSC
 * clock may have rolled-over */
#define TS_ROLLOVER_THRESH 983040

/* If a DSPS clock-rollover is detected, ensure that the last rollover event
 * happened more than X ns ago. This prevents false rollover detections from
 * jitter in the incoming SSC timestamps. */
#define MONO_TS_ROLLOVER_THRESH 5000000000 // in ns (5 seconds)

#ifndef LOGV_IF
#define LOGV_IF ALOGV_IF
#endif /* LOG_IF */
#ifndef LOGD_IF
#define LOGD_IF ALOGD_IF
#endif /* LOG_IF */
#ifndef LOGI_IF
#define LOGI_IF ALOGI_IF
#endif /* LOG_IF */
#ifndef LOGW_IF
#define LOGW_IF ALOGW_IF
#endif /* LOG_IF */
#ifndef LOGE_IF
#define LOGE_IF ALOGE_IF
#endif /* LOG_IF */

#define HAL_LOG_VERBOSE(...) LOGV_IF( (*g_hal_log_level_ptr <= HAL_LOG_LEVEL_VERBOSE), __VA_ARGS__ )
#define HAL_LOG_DEBUG(...) LOGD_IF( (*g_hal_log_level_ptr <= HAL_LOG_LEVEL_DEBUG), __VA_ARGS__ )
#define HAL_LOG_INFO(...) LOGI_IF( (*g_hal_log_level_ptr <= HAL_LOG_LEVEL_INFO), __VA_ARGS__ )
#define HAL_LOG_WARN(...) LOGW_IF( (*g_hal_log_level_ptr <= HAL_LOG_LEVEL_WARN), __VA_ARGS__ )
#define HAL_LOG_ERROR(...) LOGE_IF( (*g_hal_log_level_ptr <= HAL_LOG_LEVEL_ERROR), __VA_ARGS__ )

/* Android defines PRI.PTR incorrectly. Fix it here */
#ifdef SNS_LA
#  undef PRIxPTR
#  define PRIxPTR "x"
#  undef PRIuPTR
#  define PRIuPTR "u"
#  undef PRIdPTR
#  define PRIdPTR "d"
#  undef PRIiPTR
#  define PRIiPTR "i"
#endif /* SNS_LA */

/* Sensor data rate in Hz */
#define FREQ_FASTEST_HZ   200.0f
#define FREQ_GAME_HZ      50.0f
#define FREQ_UI_HZ        15.0f
#define FREQ_NORMAL_HZ    5.0f

/* sensor handle */
#define HANDLE_ACCELERATION           (SENSORS_HANDLE_BASE+0)
#define HANDLE_LIGHT                  (SENSORS_HANDLE_BASE+1)
#define HANDLE_GYRO                   (SENSORS_HANDLE_BASE+2)
#define HANDLE_PRESSURE               (SENSORS_HANDLE_BASE+3)
#define HANDLE_SMGR_STEP_DETECTOR     (SENSORS_HANDLE_BASE+4)
#define HANDLE_SMGR_STEP_COUNT        (SENSORS_HANDLE_BASE+5)
#define HANDLE_SMGR_SMD               (SENSORS_HANDLE_BASE+6)
#define HANDLE_SMGR_GAME_RV           (SENSORS_HANDLE_BASE+7)
#define HANDLE_GYRO_UNCALIBRATED      (SENSORS_HANDLE_BASE+8)
#define HANDLE_RELATIVE_HUMIDITY      (SENSORS_HANDLE_BASE+9)
#define HANDLE_MAGNETIC_FIELD              (SENSORS_HANDLE_BASE+10)
#define HANDLE_MAGNETIC_FIELD_UNCALIBRATED (SENSORS_HANDLE_BASE+11)
#define HANDLE_RGB                    (SENSORS_HANDLE_BASE+12)
#define HANDLE_IR_GESTURE             (SENSORS_HANDLE_BASE+13)
#define HANDLE_SAR                    (SENSORS_HANDLE_BASE+14)
#define HANDLE_AMBIENT_TEMPERATURE    (SENSORS_HANDLE_BASE+15)

#define SAM_HANDLE_BASE               (SENSORS_HANDLE_BASE+16)
#define HANDLE_GRAVITY                (SAM_HANDLE_BASE)
#define HANDLE_LINEAR_ACCEL           (SAM_HANDLE_BASE+1)
#define HANDLE_GESTURE_FACE_N_SHAKE   (SAM_HANDLE_BASE+2)
#define HANDLE_GESTURE_BRING_TO_EAR   (SAM_HANDLE_BASE+3)
#define HANDLE_MOTION_ABSOLUTE        (SAM_HANDLE_BASE+4)
#define HANDLE_MOTION_RELATIVE        (SAM_HANDLE_BASE+5)
#define HANDLE_MOTION_VEHICLE         (SAM_HANDLE_BASE+6)
#define HANDLE_ROTATION_VECTOR        (SAM_HANDLE_BASE+7)

#define HANDLE_GESTURE_BASIC_GESTURES (SAM_HANDLE_BASE+8)
#define HANDLE_GESTURE_TAP            (SAM_HANDLE_BASE+9)
#define HANDLE_GESTURE_FACING         (SAM_HANDLE_BASE+10)
#define HANDLE_GESTURE_TILT           (SAM_HANDLE_BASE+11)
#define HANDLE_GESTURE_GYRO_TAP       (SAM_HANDLE_BASE+12)
#define HANDLE_PEDOMETER              (SAM_HANDLE_BASE+13)
#define HANDLE_SAM_STEP_DETECTOR      (SAM_HANDLE_BASE+14)
#define HANDLE_SAM_STEP_COUNTER       (SAM_HANDLE_BASE+15)
#define HANDLE_PAM                    (SAM_HANDLE_BASE+16)
#define HANDLE_MOTION_ACCEL           (SAM_HANDLE_BASE+17)
#define HANDLE_SIGNIFICANT_MOTION     (SAM_HANDLE_BASE+18)
#define HANDLE_PROXIMITY              (SAM_HANDLE_BASE+19)
#define HANDLE_ORIENTATION            (SAM_HANDLE_BASE+20)
#define HANDLE_GAME_ROTATION_VECTOR   (SAM_HANDLE_BASE+21)
#define HANDLE_MAGNETIC_FIELD_SAM              (SAM_HANDLE_BASE+22)
#define HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM (SAM_HANDLE_BASE+23)
#define HANDLE_GEOMAGNETIC_ROTATION_VECTOR     (SAM_HANDLE_BASE+24)

#define HANDLE_CMC                    (SAM_HANDLE_BASE+25)

#define OEM_HANDLE_BASE               (SAM_HANDLE_BASE+26)

#define HANDLE_OEM_LIGHT              (OEM_HANDLE_BASE)
#define HANDLE_OEM_PROXIMITY          (OEM_HANDLE_BASE+1)

#define TXN_ID_BASE                   (OEM_HANDLE_BASE+2)
// Transaction ID used when a response should not be signalled
#define TXN_ID_NO_RESP_SIGNALLED      (TXN_ID_BASE)

/* All QC defined sensor types must start from QC_SENSOR_TYPE_BASE
   This is to ensure no collission with the Android sensor types
   defined in sensors.h */
#define QC_SENSOR_TYPE_BASE                 33171000

static int SENSOR_TYPE_BASIC_GESTURES       = QC_SENSOR_TYPE_BASE;
static int SENSOR_TYPE_TAP                  = QC_SENSOR_TYPE_BASE + 1;
static int SENSOR_TYPE_FACING               = QC_SENSOR_TYPE_BASE + 2;
static int SENSOR_TYPE_TILT                 = QC_SENSOR_TYPE_BASE + 3;
#ifdef FEATURE_SNS_HAL_SAM_INT
static int SENSOR_TYPE_GESTURE_FACE_N_SHAKE = QC_SENSOR_TYPE_BASE + 4;
static int SENSOR_TYPE_GESTURE_BRING_TO_EAR = QC_SENSOR_TYPE_BASE + 5;
#endif /* FEATURE_SNS_HAL_SAM_INT */
static int SENSOR_TYPE_MOTION_ABSOLUTE      = QC_SENSOR_TYPE_BASE + 6;
static int SENSOR_TYPE_MOTION_RELATIVE      = QC_SENSOR_TYPE_BASE + 7;
static int SENSOR_TYPE_MOTION_VEHICLE       = QC_SENSOR_TYPE_BASE + 8;
static int SENSOR_TYPE_PEDOMETER            = QC_SENSOR_TYPE_BASE + 9;
static int SENSOR_TYPE_PAM                  = QC_SENSOR_TYPE_BASE + 10;
static int SENSOR_TYPE_SCREEN_ORIENTATION   = QC_SENSOR_TYPE_BASE + 11;
static int SENSOR_TYPE_CMC                  = QC_SENSOR_TYPE_BASE + 12;
static int SENSOR_TYPE_RGB                  = QC_SENSOR_TYPE_BASE + 13;
static int SENSOR_TYPE_IR_GESTURE           = QC_SENSOR_TYPE_BASE + 14;
static int SENSOR_TYPE_SAR                  = QC_SENSOR_TYPE_BASE + 15;

/*===========================================================================
                   DATA TYPES
===========================================================================*/

typedef struct sensors_poll_device_1 poll_dev_t;
typedef struct sensors_poll_device_1 sensors_poll_device_1;

typedef enum {
  HAL_LTCY_MEASURE_ACCEL,
  HAL_LTCY_MEASURE_GYRO,
  HAL_LTCY_MEASURE_MAG,
  HAL_LTCY_MEASURE_PRESSURE,
  HAL_LTCY_MEASURE_PROX_LIGHT,
  HAL_LTCY_MEASURE_HUMIDITY,
  HAL_LTCY_MEASURE_RGB,
  HAL_LTCY_MEASURE_IR_GESTURES,
  HAL_LTCY_MEASURE_SAR,
  HAL_LTCY_NUM_TYPES
} hal_ltcy_measure_t;

/* Definitions for logging */
typedef enum {
  HAL_LOG_LEVEL_ALL,
  HAL_LOG_LEVEL_VERBOSE,
  HAL_LOG_LEVEL_DEBUG,
  HAL_LOG_LEVEL_INFO,
  HAL_LOG_LEVEL_WARN,
  HAL_LOG_LEVEL_ERROR,
  HAL_LOG_LEVEL_DISABLED
} hal_log_level_e;

typedef struct hal_sam_service_info {
  uint32_t      freq;
  uint8_t       instance_id;   /* Used to store instance IDs for SAM req/resp */
  uint8_t       ref_count;
} hal_sam_service_info_t;

typedef struct hal_oem_sensor_info_t {
  struct sensors_module_t* OEMModule; /* oem sensors module  */
  poll_dev_t             * OEMDevice; /* oem device */
  pthread_t                OEM_poll_thread; /* to call oem poll function */
  int                      threadCounter; /* to ensure thread is created only once */
} hal_oem_sensor_info_t;

typedef struct hal_sensor_info_t {
  char            name[SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01];   /* sensor name */
  char            vendor[SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01]; /* vendor name */
  int             version;                    /* version */
  int             handle;                     /* sensor handle */
  int             type;                       /* sensor type */
  float           max_range;                  /* maximum range */
  float           resolution;                 /* resolution */
  float           power;                      /* power in mA */
  float           max_freq;                   /* maximum frequency */
  float           min_freq;                   /* minimum frequency */
  bool            is_attrib_ok;               /* true if sensor attributes can be obtained, false otherwise */
  int             oem_handle;                 /* oem's sensor handle */
  int             max_buffered_samples;       /* Number of samples this sensor can buffer */
} hal_sensor_info_t;

typedef struct hal_sensor_dataq_t {
  sensors_event_t             data;
  struct hal_sensor_dataq_t*  next;
} hal_sensor_dataq_t;

typedef struct hal_sensor_mag_cal_sample_t {
  uncalibrated_event_t sample;
  uint32_t             smgr_ts;
  uint32_t             sam_ts;
} hal_sensor_mag_cal_sample_s;

typedef enum {
 HAL_MAG_CAL_SRC_UNKNOWN,
 HAL_MAG_CAL_SRC_SAM,
 HAL_MAG_CAL_SRC_SMGR,
 HAL_MAG_CAL_SRC_NONE
}hal_mag_cal_src_e;

typedef struct hal_sensor_control_t {
  /* Fields only modified during initialization */
  poll_dev_t                       device;
  sensor1_handle_s*                hndl;
  int                              report_ids[MAX_NUM_SENSORS];    /* Number of reports */
  uint32_t                         num_smgr_sensors;               /* number of SMGR sensors */
  hal_sensor_info_t                sensor_list[MAX_NUM_SENSORS];   /* list of sensors */
  uint64_t                         available_sensors;              /* Bit mask of available sensors */

  /* Thread control mutex and cond variables */
  pthread_mutex_t                  cb_mutex;           /* mutex lock for sensor1 callback */
  pthread_cond_t                   cb_arrived_cond;    /* cond variable to signal callback has arrived */
  bool                             is_resp_arrived;    /* flag to indicate callback has arrived */
  bool                             error;

  pthread_mutex_t                  data_mutex;         /* mutex lock for data */
  bool                             is_ind_arrived;     /* flag to indicate callback has arrived */
  pthread_cond_t                   data_arrived_cond;  /* cond variable to signal data has arrived */

  pthread_mutex_t                  acquire_resources_mutex; /* Used to serialize hal_acquire_resources */
  timer_t                          acquire_resources_timer; /* Acquire resources is called when the timer expires */

  /* Protected by data_mutex */
  sensors_event_t                  last_event[MAX_NUM_SENSORS];    /* Last event sent for sensor */
  hal_sensor_dataq_t*              q_head_ptr;                     /* data queue head */
  hal_sensor_dataq_t*              q_tail_ptr;
  bool                             flush_requested[MAX_NUM_SENSORS]; /* true if a flush has been requested for this sensor */

  /* Protected by cb_mutex */
  hal_sam_service_info_t           sam_service[MAX_SAM_SERVICES];
  hal_sensor_mag_cal_sample_s      mag_cal_cur_sample;
  hal_mag_cal_src_e                mag_cal_src;                     /* The source for calibrated mag data */
  uint64_t                         active_sensors;                 /* bit mask of active sensors */
  uint64_t                         changed_sensors;                /* Bit mask of sensors who's "current" values are out of date */
  uint32_t                         current_freq[MAX_NUM_SENSORS];  /* current freq set to this sensor */
  uint32_t                         current_rpt_rate[MAX_NUM_SENSORS];  /* current freq set to this sensor */
  bool                             current_batching[MAX_NUM_SENSORS];  /* true if batched reports are enabled */
  bool                             current_WuFF[MAX_NUM_SENSORS];  /* true if wake on fifo full flag is set */

  /* Latecny measurement enable control */
  bool                             is_ltcy_measure_enabled;   /* True if latency measurement enabled for any sensor */
  bool                             ltcy_en_table[HAL_LTCY_NUM_TYPES];  /* True if latency measure enabled for this sensor */
  uint32_t                         ltcy_measure_dsps_tick;    /* When latency measurement enabled, records DSPS tick for sensor data */

  /* keep track of SAM step counter */
  uint64_t                         step_counter_running_total;
  uint64_t                         step_counter_running_instance;
  uint64_t                         step_counter_current_instance;
  int64_t                          stepc_last_ts; // Last timestamp received for step counter sensor
  bool                             stepc_activated;
} hal_sensor_control_t;

/*===========================================================================
                    GLOBAL VARIABLES
===========================================================================*/
extern hal_log_level_e *g_hal_log_level_ptr;

/*===========================================================================
                    FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
  FUNCTION:  hal_time_init
===========================================================================*/
/*!
 * @brief
 * Must be called during HAL initialization.
 *
 * @return 0 Upon success, error otherwise
*/
int hal_time_init();

/*===========================================================================
  FUNCTION:  hal_time_start
===========================================================================*/
/*!
 * @brief
 * Initializes local state, and registers with the Time Sync service.
 *
 * @return 0 Upon success, error otherwise
*/
int hal_time_start();

/*===========================================================================
  FUNCTION:  hal_time_stop
===========================================================================*/
/*!
 * @brief
 * Close connections and reset local state.
 *
 * @return 0 Upon success, error otherwise
*/
int hal_time_stop();

/*===========================================================================
  FUNCTION:  hal_timestamp_calc
===========================================================================*/
/*!
  @brief
  Converts the DSPS clock ticks from a sensor sample to a LA timestamp (ns
  since epoch).  Adjusts return value based on dsps timestamp rollover
  and makes minor adjustments to ensure sensor samples are sent with
  ascending timestamps.

  @param[i] dsps_timestamp Timestamp as received from the DSPS
  @param[i] sensor_handle Integer representation of the sensor in question.
              Used to ensure timestamp ordering.

  @return The determined APPS-processor timestamp.
*/
int64_t hal_timestamp_calc( uint64_t dsps_timestamp, int sensor_handle );

/*===========================================================================
  FUNCTION:  hal_is_gyro_available
===========================================================================*/
/*!
  @brief
  Lets the caller know if a gyroscope sensor is present on the phone

  @return True, if a gyro device is supported, false otherwise
*/
bool hal_is_gyro_available( void );

/*===========================================================================
  FUNCTION:  hal_insert_queue
===========================================================================*/
/*!
  @brief
  Inserts data to the tail of the queue
  Helper function

  @param[i] data: sensor data to insert

  @return true if data inserted succesfully

  @dependencies Caller needs to lock the g_sensor_control->data_mutex before
                calling this function
*/
bool hal_insert_queue( sensors_event_t const * data_ptr );

/*===========================================================================
  FUNCTION:  hal_signal_ind()
===========================================================================*/
void hal_signal_ind( pthread_cond_t* cond_ptr );

/*===========================================================================
  FUNCTION:  hal_wait_for_response
===========================================================================*/
/*!
  @brief
  Blocks waiting for sensor response, either sensor1 callback to arrvive or timeout

  Helper function

  @param [i] timeout timeout in ms
  @param [i] cb_mutex_ptr pointer to locked mutex
  @param [i] cond_ptr pointer to condition variable
  @param [i/o] cond_var boolean predicate.

  @return true if callback arrived, false if timeout

  @dependencies Caller needs to lock cb_mutex_ptr before
                calling this function. Another thread must set cond_var
                to true before signalling the condition variable.
*/
bool hal_wait_for_response( int timeout, pthread_mutex_t* cb_mutex_ptr,
                            pthread_cond_t*  cond_ptr, bool *cond_var );

/*===========================================================================
  FUNCTION:  hal_ma_init
===========================================================================*/
/*!
 * @brief
 * Must be called during HAL initialization.
 *
 * @param[i] reset Whether to clear-out all ma state before [re-]initializing
 *                 data structures.
 *
 * @return 0 Upon success, error otherwise
*/
int hal_ma_init( bool reset );

/*===========================================================================
  FUNCTION:  hal_ma_destroy
===========================================================================*/
/*!
 * @brief
 * Frees and cleans-up all memory associated with hal_ma.
 *
 * @return 0 Upon success, error otherwise
*/
int hal_ma_destroy();


/*===========================================================================
  FUNCTION:  hal_ma_activate
===========================================================================*/
/*!
 * @brief
 * Enables or disables motion accel based on the current global state
 *
 * @param[i] enabled Whether we intend to enable or disable Motion Accel
 *
 * @return 0 Upon success, error otherwise
*/
int hal_ma_activate( bool enabled );

#endif /* SENSORS_HAL_H */
