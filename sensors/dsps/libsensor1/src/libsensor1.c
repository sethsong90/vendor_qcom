/*============================================================================
  @file libsensor1.c

  @brief
    This implements the remoting for sensor1 APIs using Linux sockets.

  <br><br>

  DEPENDENCIES: Linux

  Copyright (c) 2010-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential
  ============================================================================*/

/*============================================================================

  INCLUDE FILES

  ============================================================================*/
#include "sensor1.h"
#include "libsensor1.h"

#include "sns_common_v01.h"
#include "sns_smgr_api_v01.h"
#include "sns_sam_amd_v01.h"
#include "sns_sam_vmd_v01.h"
#include "sns_sam_rmd_v01.h"
#include "sns_sam_fns_v01.h"
#include "sns_sam_bte_v01.h"
#include "sns_sam_quaternion_v01.h"
#include "sns_sam_gravity_vector_v01.h"
#include "sns_reg_api_v01.h"
#include "sns_reg_api_v02.h"
#include "sns_debug_interface_v01.h"
#include "sns_sam_rotation_vector_v01.h"
#include "sns_sam_filtered_mag_v01.h"
#include "sns_sam_mag_cal_v01.h"
#include "sns_sam_sensor_thresh_v01.h"
#include "sns_time_api_v01.h"
#include "sns_sam_orientation_v01.h"
#include "sns_time_api_v02.h"
#include "sns_sam_sensor_thresh_v01.h"
#include "sns_sam_basic_gestures_v01.h"
#include "sns_sam_tap_v01.h"
#include "sns_sam_facing_v01.h"
#include "sns_sam_integ_angle_v01.h"
#include "sns_sam_gyro_tap2_v01.h"
#include "sns_sam_gyrobuf_v01.h"
#include "sns_sam_gyroint_v01.h"
#include "sns_sam_ped_v01.h"
#include "sns_sam_pam_v01.h"
#include "sns_sam_cmc_v01.h"
#include "sns_sam_distance_bound_v01.h"
#include "sns_sam_smd_v01.h"
#include "sns_sam_game_rotation_vector_v01.h"

#include "qmi_idl_lib.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <sys/system_properties.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <semaphore.h>

#define LOG_TAG "libsensor1"
#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0
#define LOG_NIDEBUG 0
#define LOG_NVDEBUG 0
#include <cutils/log.h>
#include <common_log.h>

/*============================================================================
  Preprocessor Definitions and Constants
  ============================================================================*/

/* Definitions for logging */
#define LIBSENSOR1_DEBUG_PROP_NAME "debug.qualcomm.sns.libsensor1"

#define LOG_VERBOSE(...) ALOGV_IF( (g_log_level <= LOG_LEVEL_VERBOSE), __VA_ARGS__ )
#define LOG_DEBUG(...) ALOGD_IF( (g_log_level <= LOG_LEVEL_DEBUG), __VA_ARGS__ )
#define LOG_INFO(...) ALOGI_IF( (g_log_level <= LOG_LEVEL_INFO), __VA_ARGS__ )
#define LOG_WARN(...) ALOGW_IF( (g_log_level <= LOG_LEVEL_WARN), __VA_ARGS__ )
#define LOG_ERROR(...) ALOGE_IF( (g_log_level <= LOG_LEVEL_ERROR), __VA_ARGS__ )

/* "Magic" name to identify sockets. This name will be used to insure that
 * the file descriptors passed in are indeed sensor1 socket handles.
 * Note that these use Linux "abstract" name, and may not be portable to
 * other OSes. */
#define SENSOR_CTL_MAGIC "\0SNS_CTL_SOCKET"

/* Maximum number of simultaneous clients per process (active and waiting clients) */
#define MAX_CLIENTS 40
/* Maximum number of simultaneous active clients per process */
#define MAX_ACTIVE_CLIENTS 20

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif /* UNIX_MAX_PATH */

#ifdef SNS_LA_SIM
#define strlcpy strncpy
#endif /* SNS_LA_SIM */

/* Android defines PRIxPTR incorrectly. Fix it here */
#ifdef SNS_LA
#  undef PRIxPTR
#  define PRIxPTR "x"
#endif /* SNS_LA */

/* Macro for checking if File Descriptors are valid */
#define FD_IS_VALID( fd ) libsensor_is_ctl_fd_valid( fd, __func__ )

/* Number of retries for sensor1 open before giving up */
#define OPEN_RETRIES 60
#define OPEN_EACCES_RETRIES 500
/* Time between each open retry, in usec */
#define OPEN_RETRY_DELAY 50000 /* 50ms */

/*============================================================================
  Type Declarations
  ============================================================================*/

typedef enum {
  LOG_LEVEL_ALL,
  LOG_LEVEL_VERBOSE,
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARN,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_DISABLED
} log_level_e;

/* Data sent to clients via callback function */
typedef struct libsensor_cb_data
{
  sensor1_msg_header_s      msg_hdr;
  sensor1_msg_type_e        msg_type;
  libsensor_ctl_read_s     *msg;
  struct libsensor_cb_data *next;
} libsensor_cb_data_s;

/* Data used by the receive thread */
typedef struct libsensor_client_data_s
{
  bool                     is_valid;
  int                      ctl_socket;
  sensor1_notify_data_cb_t data_cbf;
  pthread_mutex_t          data_cbf_mutex;
  intptr_t                 cb_data;
  pthread_t                cb_thread;
  libsensor_cb_data_s     *cb_q_head;
  libsensor_cb_data_s     *cb_q_tail;
  pthread_mutex_t          cb_q_mutex;
  pthread_cond_t           cb_q_cond;
} libsensor_client_data_s;

/* Sensor1 handle type -- the underlying file descriptor for the control socket */
struct sensor1_handle_s {
  int socket_id;
};

typedef struct libsensor_svc_accessor_s
{
  qmi_idl_service_object_type (*get_svc)(int32_t, int32_t, int32_t);
  int32_t maj_ver;
  int32_t min_ver;
  int32_t tool_ver;
} libsensor_svc_accessor_s;

/*============================================================================
 * Global Data Definitions
 ============================================================================*/

log_level_e g_log_level = LOG_LEVEL_WARN;

/*============================================================================
  Static Variable Definitions
  ============================================================================*/
#define SNS_GET_SVC_OBJ( svc_name, version )                            \
  {   SNS_##svc_name##_SVC_get_service_object_internal_v##version,      \
      SNS_##svc_name##_SVC_V##version##_IDL_MAJOR_VERS,                 \
      SNS_##svc_name##_SVC_V##version##_IDL_MINOR_VERS,                 \
      SNS_##svc_name##_SVC_V##version##_IDL_TOOL_VERS }


static const libsensor_svc_accessor_s svc_map_g[] = {
  SNS_GET_SVC_OBJ( SMGR, 01),           /* 0 */
  { NULL,0,0,0 },                       /* 1 */
  { NULL,0,0,0 },                       /* 2 */
  SNS_GET_SVC_OBJ( REG, 01),            /* 3 */
  SNS_GET_SVC_OBJ( SAM_AMD, 01),        /* 4 */
  SNS_GET_SVC_OBJ( SAM_RMD, 01),        /* 5 */
  SNS_GET_SVC_OBJ( SAM_VMD, 01),        /* 6 */
  SNS_GET_SVC_OBJ( DEBUG, 01),          /* 7 */
  { NULL,0,0,0 },                       /* 8 */
  SNS_GET_SVC_OBJ( SAM_FNS, 01),        /* 9 */
  SNS_GET_SVC_OBJ( SAM_BTE, 01),        /* 10 */
  { NULL,0,0,0 },                       /* 11 */
  { NULL,0,0,0 },                       /* 12 */
  { NULL,0,0,0 },                       /* 13 */
  { NULL,0,0,0 },                       /* 14 */
  SNS_GET_SVC_OBJ( REG2, 02),           /* 15 */
  SNS_GET_SVC_OBJ( SAM_MAG_CAL, 01),        /* 16 */
  SNS_GET_SVC_OBJ( SAM_FILTERED_MAG, 01),   /* 17 */
  SNS_GET_SVC_OBJ( SAM_ROTATION_VECTOR, 01),/* 18 */
  SNS_GET_SVC_OBJ( SAM_QUATERNION, 01),     /* 19 */
  SNS_GET_SVC_OBJ( SAM_GRAVITY_VECTOR, 01), /* 20 */
  SNS_GET_SVC_OBJ( SAM_SENSOR_THRESH, 01),  /* 21 */
  SNS_GET_SVC_OBJ( TIME, 01),               /* 22 */
  SNS_GET_SVC_OBJ( SAM_ORIENTATION, 01),    /* 23 */
  SNS_GET_SVC_OBJ( TIME2, 02),              /* 24 */
  SNS_GET_SVC_OBJ( SAM_BASIC_GESTURES, 01), /* 25 */
  SNS_GET_SVC_OBJ( SAM_TAP, 01),            /* 26 */
  SNS_GET_SVC_OBJ( SAM_FACING, 01),         /* 27 */
  SNS_GET_SVC_OBJ( SAM_INTEG_ANGLE, 01),     /* 28 */
  { NULL,0,0,0 },                            /* 29 */
  SNS_GET_SVC_OBJ( SAM_GYRO_TAP2, 01),       /* 30 */
  { NULL,0,0,0 },                            /* 31 */
  { NULL,0,0,0 },                            /* 32 */
  { NULL,0,0,0 },                            /* 33 */
  SNS_GET_SVC_OBJ( SAM_GYROBUF, 01),         /* 34 */
  SNS_GET_SVC_OBJ( SAM_GYROINT, 01),         /* 35 */
  { NULL,0,0,0 },                            /* 36 */
  SNS_GET_SVC_OBJ( SAM_PED, 01),             /* 37 */
  SNS_GET_SVC_OBJ( SAM_PAM, 01),             /* 38 */
  { NULL,0,0,0 },                            /* 39 */
  SNS_GET_SVC_OBJ( SAM_SMD, 01),             /* 40 */
  SNS_GET_SVC_OBJ( SAM_CMC, 01),             /* 41 */
  SNS_GET_SVC_OBJ( SAM_DISTANCE_BOUND, 01),  /* 42 */
  SNS_GET_SVC_OBJ( SAM_GAME_ROTATION_VECTOR, 01), /* 43 */
};

/* Number of entries in the service map */
#define SVC_MAP_ENTRY_SZ (sizeof(svc_map_g)/sizeof(libsensor_svc_accessor_s))

/* Thread ID of the listener thread */
static pthread_t listener_thread_id;

/* Database of clients.  */
static libsensor_client_data_s libsensor_cli_data[MAX_CLIENTS];
static pthread_mutex_t         libsensor_cli_data_mutex;

/* Semaphore to manage sensor1_open responses */
static sem_t open_sem;

/* Pipe to wake up listener thread when list of client socket
 * connections change */
static int wakeup_pipe[2];

/* File descriptor for inotify events relating to sensor1 socket */
static int inotify_fd;

/* Whether a SnapDragon Sensors core is present on this device */
static bool ssc_present = false;

/* This macro will send a message to the rx_thread, waking it up
 * from poll. This will allow the RX thread to re-read the list
 * of clients when the client DB changes.
 * See also: libsensor_cli_db_changed */
#define WAKEUP_RX_THREAD()                                              \
  {                                                                     \
    LOG_DEBUG("%s: waking up rx thread %d %d", __func__, wakeup_pipe[0], wakeup_pipe[1]); \
    char wr_data = 1; write( wakeup_pipe[1], &wr_data, 1 );             \
  }

/* Variable to control pthread_once init */
static pthread_once_t init_ctl = PTHREAD_ONCE_INIT;

/*============================================================================
  Static Function Definitions and Documentation
  ============================================================================*/

/*===========================================================================

  FUNCTION:   libsensor_cb_queue_add

  ===========================================================================*/
/**
  @brief Add an entry to the callback queue for a client.  An entry may be a resp/ind
         message, or a BROKEN_PIPE/RETRY_OPEN event.  Signals the processing thread.

  @param [in] clnt_data: Client data struct.  Pointer into libsensor_cli_data.
  @param [in] cb_data: Data entry to be added.
*/
static void
libsensor_cb_queue_add( libsensor_client_data_s *clnt_data, libsensor_cb_data_s *cb_data )
{
  pthread_mutex_lock( &clnt_data->cb_q_mutex );

  if( NULL == clnt_data->cb_q_head ) {
    clnt_data->cb_q_head = cb_data;
  } else {
    clnt_data->cb_q_tail->next = (struct libsensor_cb_data*)cb_data;
  }
  clnt_data->cb_q_tail = cb_data;
  cb_data->next = NULL;

  pthread_cond_signal( &clnt_data->cb_q_cond );
  pthread_mutex_unlock( &clnt_data->cb_q_mutex );
}

/*===========================================================================

  FUNCTION:   libsensor_del_client_data

  ===========================================================================*/
/**
  @brief Delete a queue of client message data from the database.  Cleans-up
         contents of each queue item.

  @param [in] data: Head pointer to a queue
*/
static void
libsensor_del_client_data( libsensor_cb_data_s *data )
{
  if( NULL != data ) {
    LOG_DEBUG( "%s Deleting client data", __func__ );
    libsensor_del_client_data( (libsensor_cb_data_s*) data->next );

    if( data->msg )
      sensor1_free_msg_buf( NULL, data->msg );
    free( data );
  }
}

/*===========================================================================

  FUNCTION:   libsensor_log_read_pkt

  ===========================================================================*/
static void
libsensor_log_read_pkt( libsensor_ctl_read_s *pkt, int fd )
{
  LOG_INFO( "%s: fd %"PRId32"; svc %"PRIu32"; msg %"PRId32
            "; txn %"PRIu8"; type %s; cmd %s", __func__,
            fd, pkt->svc_num, pkt->msg_id, (int)pkt->txn_id,
            ((pkt->msg_type == SENSOR1_MSG_TYPE_REQ) ? "REQ" :
             (pkt->msg_type == SENSOR1_MSG_TYPE_RESP) ? "RESP" :
             (pkt->msg_type == SENSOR1_MSG_TYPE_IND) ? "IND" :
             (pkt->msg_type == SENSOR1_MSG_TYPE_RESP_INT_ERR) ? "RESP ERR" :
             "invalid"),
            ((pkt->socket_cmd == LIBSENSOR_SOCKET_CMD_WRITE_QMI) ? "WRITE_QMI" :
             (pkt->socket_cmd == LIBSENSOR_SOCKET_CMD_WRITE_RAW) ? "WRITE_RAW" :
             (pkt->socket_cmd == LIBSENSOR_SOCKET_CMD_OPEN_BLOCK) ? "OPEN_BLOCK" :
             (pkt->socket_cmd == LIBSENSOR_SOCKET_CMD_OPEN_SUCCESS) ? "OPEN_SUCCESS" :
             "invalid")
  );
}

/*===========================================================================

  FUNCTION:   libsensor_cb_thread

  ===========================================================================*/
/**
  @brief Callback thread for a particular client.  Processes messages on the
         client's queue and sends them to clients via their registered callback
         function.

  @param [in] thread_data: Client data; pointer into libsensor_cli_data array.

  @return Always NULL
*/
static void*
libsensor_cb_thread( void *thread_data )
{
  libsensor_client_data_s *clnt_data = (libsensor_client_data_s*)thread_data;
  sensor1_notify_data_cb_t data_cbf;
  intptr_t cb_data = clnt_data->cb_data;
  int rc = 0;
  libsensor_cb_data_s *data;

  pthread_mutex_lock( &clnt_data->cb_q_mutex );
  for( ;; ) {
    if( NULL == clnt_data->cb_q_head && clnt_data->is_valid ) {
      rc = pthread_cond_wait( &clnt_data->cb_q_cond, &clnt_data->cb_q_mutex );
      if( 0 != rc ) {
        LOG_ERROR( "%s: pthread_cond_wait() rc=%d", __func__, rc );
      }
    }

    while( NULL != (data = clnt_data->cb_q_head) ) {
      clnt_data->cb_q_head = (struct libsensor_cb_data*)data->next;

      // Need to release, otherwise may have deadlock in libsensor_read_socket
      pthread_mutex_unlock( &clnt_data->cb_q_mutex );

      // We do not want to continue using the cbf after the client has called
      // sensor1_close, but we also do not want to accidentally use a NULL cbf.
      pthread_mutex_lock( &clnt_data->data_cbf_mutex );
      data_cbf = clnt_data->data_cbf;
      pthread_mutex_unlock( &clnt_data->data_cbf_mutex );

      if( data_cbf ) {
        data_cbf( cb_data, &data->msg_hdr, data->msg_type, data->msg );
      }

      free( data );
      pthread_mutex_lock( &clnt_data->cb_q_mutex );

      if( !data_cbf ) {
        clnt_data->is_valid = false;
        break;
      }
    }

    if( !clnt_data->is_valid )
      break;
  }
  LOG_DEBUG( "%s: Exiting processing thread for socket %i", __func__, clnt_data->ctl_socket );
  libsensor_del_client_data( clnt_data->cb_q_head );
  clnt_data->cb_q_head = NULL;
  clnt_data->cb_q_tail = NULL;
  pthread_mutex_unlock( &clnt_data->cb_q_mutex );

  pthread_mutex_lock( &libsensor_cli_data_mutex );
  close( clnt_data->ctl_socket );
  clnt_data->ctl_socket = -1;
  pthread_mutex_unlock( &libsensor_cli_data_mutex );

  return NULL;
}

/*===========================================================================

  FUNCTION:   libsensor_add_client

  ===========================================================================*/
/**
  @brief Add a client and update the global client database.  Starts callback
         processing thread.

  @param [in] cli_data: Data associated with the client
  @param [in] is_wait_clnt: False if this is an "active" client,
                            True if this client is waiting [for RETRY_OPEN]

  @return Client index, or MAX_CLIENTS upon failure
*/
static int
libsensor_add_client( libsensor_client_data_s const *cli_data, bool is_wait_clnt )
{
  int i,
      rv = MAX_CLIENTS,
      error,
      min_idx = is_wait_clnt ? MAX_ACTIVE_CLIENTS : 0,
      max_idx = is_wait_clnt ? MAX_CLIENTS : MAX_ACTIVE_CLIENTS;
  pthread_attr_t thread_attr;

  pthread_mutex_lock( &libsensor_cli_data_mutex );
  for( i = min_idx; i < max_idx; i++ ) {
    if( !libsensor_cli_data[i].is_valid && -1 == libsensor_cli_data[i].ctl_socket ) {
      LOG_VERBOSE( "%s Adding client index %i (%i)", __func__, i, cli_data->ctl_socket );

      libsensor_cli_data[i].cb_data = cli_data->cb_data;
      libsensor_cli_data[i].data_cbf = cli_data->data_cbf;
      libsensor_cli_data[i].ctl_socket = cli_data->ctl_socket;
      libsensor_cli_data[i].is_valid = true;
      libsensor_cli_data[i].cb_q_head = NULL;
      libsensor_cli_data[i].cb_q_tail = NULL;

      // Initialize and start callback thread
      if( 0 != ( error = pthread_attr_init( &thread_attr ) ) ) {
        LOG_ERROR( "%s pthread_attr_init failure %i", __func__, error );
      } else {
        if( 0 != ( error = pthread_attr_setdetachstate( &thread_attr, PTHREAD_CREATE_DETACHED ) ) ) {
          LOG_ERROR( "%s pthread_attr_setdetachstate failure %i", __func__, error );
        } else {
          error = pthread_create( &libsensor_cli_data[i].cb_thread, &thread_attr,
                                  libsensor_cb_thread, &libsensor_cli_data[i] );
          if( 0 != error ) {
            LOG_ERROR( "%s error %d initializing thread", __func__, error );
          }
        }
      }
      pthread_attr_destroy( &thread_attr );

      rv = i;
      break;
    }
  }
  pthread_mutex_unlock( &libsensor_cli_data_mutex );
  WAKEUP_RX_THREAD();
  return rv;
}

/*===========================================================================

  FUNCTION:   libsensor_del_client

  ===========================================================================*/
/**
  @brief Delete a client and updates the global client database.  Signals to
         the callback processing thread to exit.  Client slot will not be
         available until cb thread exits.  Note: additional messages may still
         be received after this function returns).

  @param [in]     cli_socket: Socket of the client to delete

  @return true: client deleted
*/
static bool
libsensor_del_client( int cli_socket )
{
  int  i;
  bool rv = false;

  pthread_mutex_lock( &libsensor_cli_data_mutex );
  for( i = 0; i < MAX_CLIENTS; i++ ) {
    if( libsensor_cli_data[i].is_valid == true &&
        libsensor_cli_data[i].ctl_socket == cli_socket ) {
      pthread_mutex_lock( &libsensor_cli_data[i].cb_q_mutex );
      libsensor_cli_data[i].is_valid = false;
      pthread_cond_signal( &libsensor_cli_data[i].cb_q_cond );
      pthread_mutex_unlock( &libsensor_cli_data[i].cb_q_mutex );

      rv = true;
      break;
    }
  }
  pthread_mutex_unlock( &libsensor_cli_data_mutex );
  WAKEUP_RX_THREAD();
  return rv;
}

/*===========================================================================

  FUNCTION:   libsensor_add_waiting_client

  ===========================================================================*/
/**
  @brief Add a client waiting libsensor1 availability to the global database.
         Adds inotify watch if this is the first waiting client.

  @param [in]     cli_data: Data associated with the client

  @return true: client added
*/
static bool
libsensor_add_waiting_client( libsensor_client_data_s const *cli_data )
{
  int  i;
  bool rv = false;

  i = libsensor_add_client( cli_data, true );

  pthread_mutex_lock( &libsensor_cli_data_mutex );
  if( i == MAX_ACTIVE_CLIENTS ) {
    int wd;
    wd = inotify_add_watch( inotify_fd, SENSOR_CTL_SOCKET, IN_ALL_EVENTS );
    if( -1 == wd ) {
      if( ENOENT == errno ) {
        LOG_DEBUG("%s: Socket %s does not exist. Waiting on dir change instead",
             __func__, SENSOR_CTL_SOCKET);
        wd = inotify_add_watch( inotify_fd, SENSOR_CTL_PATH, IN_CREATE );
        if( -1 == wd ) {
        LOG_ERROR("%s: Error adding inotify watch for sensor socket dir: %d: %s",
             __func__, errno, strerror(errno));
        }
      } else {
        LOG_ERROR("%s: Error adding inotify watch for sensor socket file: %d: %s",
             __func__, errno, strerror(errno));
      }
    }
  }

  pthread_mutex_unlock( &libsensor_cli_data_mutex );
  return rv;
}

/*===========================================================================

  FUNCTION:   libsensor_notify_waiting_clients

  ===========================================================================*/
/**
  @brief Notify all clients waiting for sensor1_availability

  Modifies the global client database. Calls all client callbacks to notify
  them that sensor1 may be available, and removes them from the list.
*/
static void
libsensor_notify_waiting_clients( void )
{
  int i;

  pthread_mutex_lock( &libsensor_cli_data_mutex );
  for( i = MAX_ACTIVE_CLIENTS; i < MAX_CLIENTS; i++ ) {
    if( libsensor_cli_data[i].is_valid == true ) {
      libsensor_cb_data_s *data = malloc( sizeof(libsensor_cb_data_s) );

      if( NULL == data ) {
        LOG_ERROR( "%s Malloc failure", __func__ );
        continue;
      }

      libsensor_cli_data[i].is_valid = false;
      data->msg = NULL;
      data->msg_type = SENSOR1_MSG_TYPE_RETRY_OPEN;
      libsensor_cb_queue_add( &libsensor_cli_data[i], data );
    }
  }
  pthread_mutex_unlock( &libsensor_cli_data_mutex );
}

/*===========================================================================

  FUNCTION:   libsensor_get_client_by_fd

  ===========================================================================*/
/**
  @brief Lookup a client in the database.
         Must hold libsensor_cli_data_mutex.

  @param [in]     fd: Socket of the client to find

  @return -1 if not found. >= 0 if found.
*/
static int
libsensor_get_client_by_fd( int fd )
{
  int i;

  for( i = 0; i < MAX_CLIENTS; i++ ) {
    if( libsensor_cli_data[i].is_valid
        && libsensor_cli_data[i].ctl_socket == fd ) {
      return i;
    }
  }
  return -1;
}

/*===========================================================================

  FUNCTION:   libsensor_cli_db_changed

  ===========================================================================*/
/**
  @brief Returns true if the client database has changed since this function
         was previously called.

*/
static bool
libsensor_cli_db_changed( void )
{
  bool changed = false;
  int  num_bytes;
  char buff[100];

  num_bytes = read( wakeup_pipe[0], buff, sizeof(buff) );

  if( num_bytes > 0 ) {
    changed = true;
  }

  return changed;
}

/*===========================================================================

  FUNCTION:   libsensor_cli_db_to_pollfd

  ===========================================================================*/
/**
  @brief Fills a pollfd array with data from the client db.

         Adds two FDs to the end of the list to wakeup poll in the case 1) the
         database has changed or 2) the iNotify FD has been signaled.

  @param [out] pollfd: Array to fill in
  @param [in]  pollfd_size: Number of entries in the array.
  @param [in]  events: Events to listen for

  @return Number of file descriptors to wait on
*/
static int
libsensor_cli_db_to_pollfd( struct pollfd *pollfd,
                            int pollfd_size,
                            short events )
{
  int i;
  int j;

  memset( pollfd, -1, sizeof(struct pollfd) * pollfd_size );

  pthread_mutex_lock( &libsensor_cli_data_mutex );
  for( i = 0, j = 0; j < pollfd_size-2 && i < MAX_ACTIVE_CLIENTS; i++ ) {
    if( libsensor_cli_data[i].is_valid) {
      pollfd[j].fd = libsensor_cli_data[i].ctl_socket;
      pollfd[j].events = events;
      j++;
    }
  }
  pthread_mutex_unlock( &libsensor_cli_data_mutex );

  pollfd[j].fd = wakeup_pipe[0];
  pollfd[j].events = POLLIN;

  pollfd[j+1].fd = inotify_fd;
  pollfd[j+1].events = POLLIN;

  return j+2;
}


/*===========================================================================

  FUNCTION:   libsensor_is_ctl_fd_valid

  ===========================================================================*/
static bool
libsensor_is_ctl_fd_valid( int fd, const char* function_name )
{
  struct sockaddr_un address;
  socklen_t len = UNIX_PATH_MAX;
  int error;

  memset(address.sun_path,0,UNIX_PATH_MAX);
  error = getsockname( fd, (struct sockaddr *)&address, &len );
  if( -1 == error ) {
    LOG_ERROR("%s: Error getting socket name fd: %d: %s", function_name,
         fd, strerror(errno));

    return false;
  }
  if( 0 != memcmp( address.sun_path, SENSOR_CTL_MAGIC,
                   sizeof( SENSOR_CTL_MAGIC ) ) ) {
    return false;
  }
  return true;
}

/*===========================================================================

  FUNCTION:   libsensor_log_ctl_write_pkt

  ===========================================================================*/
static void
libsensor_log_ctl_write_pkt( libsensor_ctl_write_s *pkt, int fd )
{
  LOG_INFO( "%s: fd %"PRId32"; svc %"PRIu32"; msg %"PRId32"; txn %"PRIu8"; cmd %s",
            __func__, fd, pkt->svc_num, pkt->msg_id, (int)pkt->txn_id,
            ((pkt->socket_cmd == LIBSENSOR_SOCKET_CMD_WRITE_QMI) ? "WRITE_QMI" :
             (pkt->socket_cmd == LIBSENSOR_SOCKET_CMD_WRITE_RAW) ? "WRITE_RAW" :
             (pkt->socket_cmd == LIBSENSOR_SOCKET_CMD_DISCON_CTL) ? "DISCON_CTL" :
             (pkt->socket_cmd == LIBSENSOR_SOCKET_CMD_OPEN_BLOCK) ? "OPEN_BLOCK" :
             (pkt->socket_cmd == LIBSENSOR_SOCKET_CMD_OPEN_SUCCESS) ? "OPEN_SUCCESS" :
             "invalid")
          );
}

/*===========================================================================

  FUNCTION:   libsensor_read_socket

  ===========================================================================*/
/**
   @brief Reads a control socket.

   @param[in] fd: File descriptor of socket.

   @dependencies Uses the global client database

   @return: SENSOR1_SUCCESS if successful
 */
static sensor1_error_e
libsensor_read_socket( int fd )
{
  libsensor_ctl_read_s        *rx_msg_p;
  libsensor_ctl_read_s        *decoded_ctype_p;
  uint32_t                     decoded_ctype_sz;
  uint32_t                     err;
  bool                         qmi_enabled;
  sensor1_error_e              rv = SENSOR1_SUCCESS;
  qmi_idl_service_object_type  service;
  int                          cli_idx;

  pthread_mutex_lock( &libsensor_cli_data_mutex );

  if( (cli_idx = libsensor_get_client_by_fd( fd )) < 0 ) {
    pthread_mutex_unlock( &libsensor_cli_data_mutex );
    return SENSOR1_EBUFFER;
  }

  rx_msg_p = malloc( SENSOR_MAX_MSG_SIZE + sizeof(libsensor_ctl_read_s) -1 );

  if( NULL != rx_msg_p ) {
    ssize_t num_bytes;
    sensor1_msg_header_s msg_hdr;

    num_bytes = read( fd,
                      rx_msg_p,
                      sizeof(*rx_msg_p) - 1 + SENSOR_MAX_MSG_SIZE );

    if( num_bytes >= (ssize_t)sizeof(*rx_msg_p)-1 ) {
      if(LIBSENSOR_SOCKET_CMD_OPEN_BLOCK == rx_msg_p->socket_cmd) {
        LOG_INFO("%s: socket received open block response", __func__ );
      } else if(LIBSENSOR_SOCKET_CMD_OPEN_SUCCESS == rx_msg_p->socket_cmd) {
        LOG_DEBUG("%s: socket received open success response", __func__ );
        if( 0 != sem_post( &open_sem ) ) {
          LOG_ERROR("%s: sem_post failed %d", __func__, errno );
        }
      } else if( (rx_msg_p->svc_num >= sizeof(svc_map_g) / sizeof(*svc_map_g) ) ||
          (NULL == svc_map_g[rx_msg_p->svc_num].get_svc) ) {
        LOG_ERROR("%s: rx bad svc id %d", __func__, rx_msg_p->svc_num );
        rv = SENSOR1_EBAD_SVC_ID;
      } else if( NULL != libsensor_cli_data[ cli_idx ].data_cbf ) {

        if( LIBSENSOR_SOCKET_CMD_WRITE_QMI == rx_msg_p->socket_cmd &&
            ( SENSOR1_MSG_TYPE_RESP == rx_msg_p->msg_type ||
              SENSOR1_MSG_TYPE_IND == rx_msg_p->msg_type )) {
          qmi_enabled = true;
        } else {
          qmi_enabled = false;
        }
        if( qmi_enabled ) {
          if (SVC_MAP_ENTRY_SZ > rx_msg_p->svc_num) {
            service =
              svc_map_g[rx_msg_p->svc_num].get_svc( svc_map_g[rx_msg_p->svc_num].maj_ver,
                                                    svc_map_g[rx_msg_p->svc_num].min_ver,
                                                    svc_map_g[rx_msg_p->svc_num].tool_ver );
            err =
              qmi_idl_get_message_c_struct_len( service,
                                                (qmi_idl_type_of_message_type)rx_msg_p->msg_type,
                                                rx_msg_p->msg_id,
                                                &decoded_ctype_sz );
            if( QMI_IDL_LIB_NO_ERR != err ) {
              LOG_ERROR("%s: qmi get ctype len err %d", __func__, err );
              rv = SENSOR1_EUNKNOWN;
              decoded_ctype_sz = 0;
              qmi_enabled = false;
            }
          } else {
            LOG_ERROR("%s: rx unknown svc id %d", __func__, rx_msg_p->svc_num );
            rv = SENSOR1_EUNKNOWN;
            decoded_ctype_sz = 0;
            qmi_enabled = false;
          }
        } else {
          decoded_ctype_sz = num_bytes - sizeof(*rx_msg_p) + 1;
        }

        err = sensor1_alloc_msg_buf( ( (sensor1_handle_s*)
                                       (intptr_t)(fd) ),
                                     decoded_ctype_sz,
                                     (void**)&decoded_ctype_p );

        if( SENSOR1_SUCCESS != err ) {
          LOG_ERROR( "%s sensor1_alloc_msg_buf failure", __func__ );
          rv = SENSOR1_ENOMEM;
        } else {
          if( qmi_enabled ) {
            err = qmi_idl_message_decode( service,
                                          rx_msg_p->msg_type,
                                          rx_msg_p->msg_id,
                                          rx_msg_p->data,
                                          num_bytes + 1 - sizeof(*rx_msg_p),
                                          decoded_ctype_p,
                                          decoded_ctype_sz );
            if( QMI_IDL_LIB_NO_ERR != err ) {
              LOG_ERROR("%s: qmi decode err %d", __func__, err );
              rv = SENSOR1_EUNKNOWN;
            }
          } else {
            memcpy( decoded_ctype_p, rx_msg_p->data, decoded_ctype_sz );
          }
        }

        msg_hdr.service_number = rx_msg_p->svc_num;
        msg_hdr.msg_id = rx_msg_p->msg_id;
        msg_hdr.msg_size = decoded_ctype_sz;
        msg_hdr.txn_id = rx_msg_p->txn_id;
        libsensor_log_read_pkt( rx_msg_p, fd );

        libsensor_cb_data_s *data = malloc( sizeof(libsensor_cb_data_s) );
        if( NULL == data ) {
          LOG_ERROR( "%s Malloc failure, dropped message", __func__ );
          rv = SENSOR1_ENOMEM;
        } else {
          data->msg_type = rx_msg_p->msg_type;
          data->msg_hdr = msg_hdr;
          data->msg = (void*)(decoded_ctype_p);
          libsensor_cb_queue_add( &libsensor_cli_data[ cli_idx ], data );
        }
      }
    } else {
      if( libsensor_cli_data[ cli_idx ].data_cbf ) {
        libsensor_cb_data_s *data;
        LOG_INFO( "%s: socket read error fd %d", __func__, fd );

        data = malloc( sizeof(libsensor_cb_data_s) );
        if( NULL == data ) {
          LOG_ERROR( "%s Malloc failure", __func__ );
        } else {
          data->msg = NULL;
          data->msg_type = SENSOR1_MSG_TYPE_BROKEN_PIPE;
          libsensor_cb_queue_add( &libsensor_cli_data[ cli_idx ], data );
        }
      }
      rv = SENSOR1_EFAILED;
    }
    free( rx_msg_p );
  } else {
    LOG_ERROR("%s: Out of memory! fd %d", __func__, fd );
    rv = SENSOR1_ENOMEM;
  }

  pthread_mutex_unlock( &libsensor_cli_data_mutex );
  return rv;
}

/*===========================================================================

  FUNCTION:   libsensor_rx_thread

  ===========================================================================*/
static void*
libsensor_rx_thread( void *thread_data )
{
  uint32_t err;
  struct pollfd pollfd[MAX_ACTIVE_CLIENTS+2];
  int num_fds = 0;
  int i;

  num_fds = libsensor_cli_db_to_pollfd( pollfd,
                                        MAX_ACTIVE_CLIENTS+2,
                                        POLLIN|POLLPRI );
  while( true )
  {
    if( libsensor_cli_db_changed() ) {
      num_fds = libsensor_cli_db_to_pollfd( pollfd,
                                            MAX_ACTIVE_CLIENTS+2,
                                            POLLIN|POLLPRI );
    }

    /* Note that the wakeup FD and the inotify FD are
     * additional FDs at the end of the list after all of the clients */
#ifdef LOG_NDDEBUG
    for( i = 0; i < num_fds; i++ ) {
      LOG_DEBUG("%s: waiting on fd %d", __func__, pollfd[i].fd);
    }
#endif /* LOG_NDDEBUG */

    poll( pollfd, num_fds, -1 );

#ifdef LOG_NDDEBUG
    if( pollfd[num_fds-2].revents != 0 ) {
      LOG_DEBUG("%s: waking on wakeup pipe %d", __func__, pollfd[num_fds-2].fd);
    }
#endif /* LOG_NDDEBUG */

    for( i = 0; i < num_fds-2; i ++ ) {
      if( pollfd[i].revents & (POLLIN|POLLPRI) ) {
        LOG_DEBUG("%s: waking on fd %d", __func__, pollfd[i].fd);
        err = libsensor_read_socket( pollfd[i].fd );

        if( SENSOR1_SUCCESS != err && SENSOR1_EBUFFER != err ) {
          libsensor_del_client( pollfd[i].fd );
          i = 0;
        }

        pollfd[i].revents &= ~(POLLIN|POLLPRI);
      }
    }

    if( pollfd[num_fds-1].revents != 0 ) {
      char buf[500];
      struct inotify_event *evt = (struct inotify_event *)buf;
      read(pollfd[num_fds-1].fd, evt, 500);
      LOG_DEBUG("%s: inotify: wd %d mask 0x%x name %s", __func__,
           evt->wd, evt->mask, (evt->len > 0) ? evt->name:"<empty>");
      if(evt->mask & IN_IGNORED) {
        /* Previous watch was removed. Nothing to do here */
      } else if(evt->len == 0 ||
         ( (evt->mask & IN_CREATE) &&
           (0 == strncmp( evt->name, SENSOR_CTL_FILENAME, evt->len)))) {
        inotify_rm_watch( inotify_fd, evt->wd );
        libsensor_notify_waiting_clients();
      }
    }
  }

  LOG_INFO( "%s: thread exiting", __func__ );
  return NULL;
}


/*============================================================================
  Externalized Function Definitions
  ============================================================================*/
/*===========================================================================

  FUNCTION:   sensor1_open

  ===========================================================================*/
sensor1_error_e
sensor1_open( sensor1_handle_s **hndl,
              sensor1_notify_data_cb_t data_cbf,
              intptr_t cb_data )
{
  int sockfd;
  int retries, eacces_retries;
  int err;
  socklen_t len;
  struct sockaddr_un address;
  libsensor_client_data_s new_cli;
  struct timespec open_timeout;

  if( NULL == hndl ) {
    return SENSOR1_EBAD_PTR;
  }

  /* Call sensor1_init() here, in case the client failed to do it */
  sensor1_init();

  new_cli.data_cbf  = data_cbf;
  new_cli.cb_data   = cb_data;

  if( !ssc_present )
  {
    LOG_WARN("%s: SSC not present", __func__);
    return SENSOR1_EFAILED;
  }
  if ( (sockfd = socket(AF_UNIX, SOCK_SEQPACKET, 0)) == -1)
  {
    LOG_ERROR("%s: Error in socket() %s", __func__, strerror(errno));
    return SENSOR1_ENOMEM;
  }

  /* name the socket */
  address.sun_family = AF_UNIX;
  strlcpy(address.sun_path, SENSOR_CTL_SOCKET, UNIX_PATH_MAX);
  len = sizeof(address);

  /* connect socket to server */
  if( -1 == connect(sockfd, (struct sockaddr *)&address, len) ) {
    retries = 0;
    eacces_retries = 0;
    while( (errno == ENOENT || errno == ECONNREFUSED || errno == EACCES) &&
           (retries < OPEN_RETRIES && eacces_retries < OPEN_EACCES_RETRIES)    )
    {
      if(EACCES  == errno ) {
        eacces_retries++;
      } else {
        retries++;
      }

      errno = 0;
      LOG_DEBUG("%s: connect failed. retrying in %d msec", __func__, OPEN_RETRY_DELAY/1000);
      usleep( OPEN_RETRY_DELAY );
      connect(sockfd, (struct sockaddr *)&address, len);
    }
    if ( errno != 0 ) {
      int err = errno;
      /* if it still fails no point connecting again */
      LOG_ERROR("%s: Error in connect() errno=%d %s",
           __func__, errno, strerror(errno));
      close( sockfd );
      if (err == ENOENT || err == ECONNREFUSED || err == EACCES) {
        libsensor_client_data_s cli_data;
        cli_data.data_cbf = data_cbf;
        cli_data.cb_data = cb_data;
        cli_data.ctl_socket = -2;
        libsensor_add_waiting_client(&cli_data);
        return SENSOR1_EWOULDBLOCK;
      } else {
        return SENSOR1_EFAILED;
      }
    }
  }

  memset( address.sun_path, 0, UNIX_PATH_MAX );
  memcpy( address.sun_path, SENSOR_CTL_MAGIC, sizeof(SENSOR_CTL_MAGIC) );
  snprintf( address.sun_path+sizeof(SENSOR_CTL_MAGIC),
            UNIX_PATH_MAX-sizeof(SENSOR_CTL_MAGIC),
            "%ld(%ld)",
            (long int)gettid(), random() );


  if( -1 ==  bind( sockfd, (struct sockaddr*)&address,
                   sizeof(address) ) ) {
    LOG_ERROR("%s: Error in bind() errno=%d %s",
         __func__, errno, strerror(errno));

    close( sockfd );
    return SENSOR1_EFAILED;
  }

  new_cli.ctl_socket  = sockfd;

  if( libsensor_add_client( &new_cli, false ) >= MAX_CLIENTS ) {
    LOG_ERROR( "%s: Unable to add new client (%i)", __func__, new_cli.ctl_socket );
    close( sockfd );
    return SENSOR1_ENOMEM;
  }

  if( -1 == ( err = clock_gettime (CLOCK_REALTIME, &open_timeout ) ) ) {
    LOG_ERROR( "%s: Error in clock_gettime(%i) %s", __func__, errno, strerror(errno) );
    libsensor_del_client( sockfd );
    return SENSOR1_EFAILED;
  } else {
    //open_timeout.tv_nsec += 100000000; // Wait 100 milliseconds
    //open_timeout.tv_nsec -= open_timeout.tv_nsec > 1000000000 ? 1000000000 : 0;
    //open_timeout.tv_sec += open_timeout.tv_nsec < 100000000 ? 1 : 0;
    open_timeout.tv_sec += 1;

    err = sem_timedwait( &open_sem, &open_timeout );
  }

  if( 0 != err && ETIMEDOUT == errno ) {
    libsensor_client_data_s cli_data;

    LOG_ERROR( "%s: Sem wait timed-out for socket %i", __func__, sockfd );
    libsensor_del_client( sockfd );

    cli_data.data_cbf = data_cbf;
    cli_data.cb_data = cb_data;
    cli_data.ctl_socket = -1;
    libsensor_add_waiting_client( &cli_data );
    return SENSOR1_EWOULDBLOCK;
  } else if( 0 != err ) {
    LOG_ERROR( "%s: Sem wait failed (err %i) for socket %i",
               __func__, errno, sockfd );
    libsensor_del_client( sockfd );
    return SENSOR1_EFAILED;
  }

  *hndl = (sensor1_handle_s*)((intptr_t)sockfd);
  LOG_INFO( "%s: Adding new client fd %d", __func__, new_cli.ctl_socket );

  return 0;
}

/*===========================================================================

  FUNCTION:   sensor1_close

  ===========================================================================*/
sensor1_error_e
sensor1_close( sensor1_handle_s *hndl )
{
  libsensor_ctl_write_s       write_pkt;
  int                         cli_fd = (int)(intptr_t)hndl;
  int                         cli_idx;
  sensor1_error_e             rv;

  if( !FD_IS_VALID(cli_fd) ) {
    rv = SENSOR1_EINVALID_CLIENT;
  } else {
    LOG_DEBUG("%s: closing client socket fd %d", __func__, cli_fd );

    pthread_mutex_lock( &libsensor_cli_data_mutex );

    /* Clear out the client callback function, so it stops
    * getting notifications */
    cli_idx = libsensor_get_client_by_fd( cli_fd );
    if( cli_idx == -1 ) {
      LOG_ERROR("%s: client socket fd %d seems valid, but not in client table!",
                __func__, cli_fd );
      rv = SENSOR1_EINVALID_CLIENT;
    } else {
      pthread_mutex_lock( &libsensor_cli_data[cli_idx].data_cbf_mutex );
      libsensor_cli_data[cli_idx].data_cbf = NULL;
      pthread_mutex_unlock( &libsensor_cli_data[cli_idx].data_cbf_mutex );

      write_pkt.socket_cmd = LIBSENSOR_SOCKET_CMD_DISCON_CTL;
      write_pkt.reserved = 0;
      libsensor_log_ctl_write_pkt( &write_pkt, cli_fd );
      write( cli_fd, &write_pkt, sizeof(write_pkt) );
      rv = SENSOR1_SUCCESS;
    }

    pthread_mutex_unlock( &libsensor_cli_data_mutex );
  }

  return rv;
}


/*===========================================================================

  FUNCTION:   sensor1_write

  ===========================================================================*/
sensor1_error_e
sensor1_write( sensor1_handle_s     *hndl,
               sensor1_msg_header_s *msg_hdr,
               void                 *msg_ptr )
{
  libsensor_ctl_write_s       *write_pkt;
  void                        *encoded_msg, *msg_ptr_temp;
  uint32_t                     max_encoded_len;
  uint32_t                     encoded_len;
  uint32_t                     ctype_len;
  int32_t                      err;
  qmi_idl_service_object_type  service;
  int                          cli_fd = (int)((intptr_t)hndl);


  if( NULL == msg_hdr ) {
    return SENSOR1_EBAD_PARAM;
  }

  if( false == FD_IS_VALID( cli_fd ) ) {
    return SENSOR1_EINVALID_CLIENT;
  }

  if( (msg_hdr->service_number >= sizeof(svc_map_g) / sizeof(*svc_map_g) ) ||
      (NULL == svc_map_g[msg_hdr->service_number].get_svc) ) {
    return SENSOR1_EBAD_SVC_ID;
  }

  service =
    svc_map_g[msg_hdr->service_number].get_svc( svc_map_g[msg_hdr->service_number].maj_ver,
                                                svc_map_g[msg_hdr->service_number].min_ver,
                                                svc_map_g[msg_hdr->service_number].tool_ver );

  err = qmi_idl_get_message_c_struct_len( service,
                                          QMI_IDL_REQUEST,
                                          msg_hdr->msg_id,
                                          &ctype_len );
  if( QMI_IDL_LIB_NO_ERR != err ) {
    LOG_ERROR("%s: QMI get ctype len error %d", __func__,
         err );
    if( QMI_IDL_LIB_MESSAGE_ID_NOT_FOUND == err ) {
      return SENSOR1_EBAD_MSG_ID;
    } else {
      return SENSOR1_EFAILED;
    }
  }

  err = qmi_idl_get_max_message_len( service,
                                     QMI_IDL_REQUEST,
                                     msg_hdr->msg_id,
                                     &max_encoded_len );

  if( QMI_IDL_LIB_NO_ERR != err ) {
    LOG_ERROR( "%s: QMI get max message len error %d", __func__, err );
    return SENSOR1_EFAILED;
  }

  if( (NULL == msg_ptr) && (0 != msg_hdr->msg_size) ) {
    return SENSOR1_EBAD_PARAM;
  } else {
    err = sensor1_alloc_msg_buf( hndl, max_encoded_len, &encoded_msg );
    if( SENSOR1_SUCCESS != err || NULL == encoded_msg ) {
      return SENSOR1_ENOMEM;
    }
  }

  if( ctype_len != msg_hdr->msg_size ) {
    err = sensor1_alloc_msg_buf( hndl, ctype_len, &msg_ptr_temp );
    if( SENSOR1_SUCCESS != err || NULL == msg_ptr_temp ) {
      return SENSOR1_ENOMEM;
    }

    if( NULL != msg_ptr ) {
      memcpy( msg_ptr_temp, msg_ptr,
              ( ctype_len >= msg_hdr->msg_size ) ?
                msg_hdr->msg_size : ctype_len );

      sensor1_free_msg_buf( hndl, msg_ptr );
    }

    msg_ptr = msg_ptr_temp;
    LOG_DEBUG( "%s: Updating message legth %i -> %i",
               __func__, msg_hdr->msg_size, ctype_len );
    msg_hdr->msg_size = ctype_len;
  }

  if( 0 != msg_hdr->msg_size ) {
    err = qmi_idl_message_encode( service,
                                  QMI_IDL_REQUEST,
                                  msg_hdr->msg_id,
                                  msg_ptr,
                                  msg_hdr->msg_size,
                                  encoded_msg,
                                  max_encoded_len,
                                  &encoded_len );
    if( err != QMI_IDL_LIB_NO_ERR ) {
      LOG_ERROR("%s: QMI encode failed error %d", __func__,
           err );
      sensor1_free_msg_buf( hndl, encoded_msg );
      return SENSOR1_EFAILED;
    }
  } else {
    encoded_len = 0;
  }

  write_pkt = (libsensor_ctl_write_s*)
    (((uint8_t*)encoded_msg)-sizeof(*write_pkt)+1);

  write_pkt->svc_num = msg_hdr->service_number;
  write_pkt->msg_id  = msg_hdr->msg_id;
  write_pkt->txn_id  = msg_hdr->txn_id;
  write_pkt->socket_cmd = LIBSENSOR_SOCKET_CMD_WRITE_QMI;
  write_pkt->reserved = 0;

  libsensor_log_ctl_write_pkt( write_pkt, cli_fd );
  err = write( cli_fd,
               write_pkt,
               sizeof(*write_pkt)-1 + encoded_len );
  if( -1 == err ) {
    LOG_ERROR("%s: Error writing to socket fd: %d: %s", __func__,
         cli_fd, strerror(errno) );

    sensor1_free_msg_buf( hndl, encoded_msg );

    if( EPIPE == errno ||
        EINVAL == errno ||
        ENOTCONN == errno ) {
      return SENSOR1_EINVALID_CLIENT;
    } else if( EAGAIN == errno ||
               EWOULDBLOCK == errno ||
               ENOSPC == errno ) {
      return SENSOR1_EWOULDBLOCK;
    }
    return SENSOR1_EUNKNOWN;
  }

  if( NULL != msg_ptr ) {
    sensor1_free_msg_buf( hndl, msg_ptr );
  }
  sensor1_free_msg_buf( hndl, encoded_msg );

  return SENSOR1_SUCCESS;
}

/*===========================================================================

  FUNCTION:   sensor1_writable

  ===========================================================================*/
sensor1_error_e
sensor1_writable( sensor1_handle_s  *hndl,
                  sensor1_write_cb_t cbf,
                  intptr_t           cb_data,
                  uint32_t           service_number )
{
  // TODO: implement
  cbf( cb_data, service_number );
  return SENSOR1_SUCCESS;
}

/*===========================================================================

  FUNCTION:   sensor1_alloc_msg_buf

  ===========================================================================*/
sensor1_error_e
sensor1_alloc_msg_buf(sensor1_handle_s *hndl,
                      uint16_t          size,
                      void            **buffer )
{
  if( NULL != buffer ) {
    *buffer = calloc(1, size+sizeof(libsensor_ctl_write_s)-1);
    /*
    LOG_INFO("%s: allocated buffer 0x%"PRIxPTR", size %"PRIu16, __func__,
         (uintptr_t)*buffer, size );
    */
    if (NULL == *buffer) {
      LOG_ERROR("%s: Out of memory!", __func__ );
      return SENSOR1_ENOMEM;
    }
    *buffer = ((uint8_t*)*buffer)+sizeof(libsensor_ctl_write_s)-1;
  } else {
    return SENSOR1_EFAILED;
  }
  return SENSOR1_SUCCESS;
}


/*===========================================================================

  FUNCTION:   sensor1_free_msg_buf

  ===========================================================================*/
sensor1_error_e
sensor1_free_msg_buf(sensor1_handle_s *hndl,
                     void             *msg_buf )
{
  void *buffer = ((uint8_t*)msg_buf)-sizeof(libsensor_ctl_write_s)+1;
  /*
  LOG_INFO("%s: freeing buffer 0x%"PRIxPTR, __func__,
       (uintptr_t)buffer );
  */
  free( buffer );
  return SENSOR1_SUCCESS;
}

/*===========================================================================

  FUNCTION:   sensor1_init_once

  ===========================================================================*/
void
sensor1_init_once( void )
{
  struct timespec     ts;
  int                 seed,
                      error,
                      debug_prop_len,
                      i;
  pthread_mutexattr_t mutex_attr;
  pthread_attr_t      thread_attr;
  char                debug_prop[PROP_VALUE_MAX];

  debug_prop_len = __system_property_get( LIBSENSOR1_DEBUG_PROP_NAME, debug_prop );
  if( debug_prop_len == 1 ) {
    switch( debug_prop[0] ) {
      case '0':
        g_log_level = LOG_LEVEL_DISABLED;
        break;
      case '1':
        g_log_level = LOG_LEVEL_ALL;
        break;
      case 'v':
      case 'V':
        g_log_level = LOG_LEVEL_VERBOSE;
        break;
      case 'd':
      case 'D':
        g_log_level = LOG_LEVEL_DEBUG;
        break;
      case 'i':
      case 'I':
        g_log_level = LOG_LEVEL_INFO;
        break;
      case 'w':
      case 'W':
        g_log_level = LOG_LEVEL_WARN;
        break;
      case 'e':
      case 'E':
        g_log_level = LOG_LEVEL_ERROR;
        break;
      default:
        break;
    }
    ALOGI("%s: Setting log level to %d", __FUNCTION__, g_log_level);
  } else if( debug_prop_len > 1 ) {
    LOG_ERROR("%s: invalid value for %s: %s. Enabling all logs", __FUNCTION__,
              LIBSENSOR1_DEBUG_PROP_NAME, debug_prop );
    g_log_level = LOG_LEVEL_ALL;
  }

  LOG_DEBUG("%s", __func__);

#if defined(SNS_LA_SIM)
  ssc_present = true;
#else
  struct stat         stat_buf;
  // Check device driver for A-family
  ssc_present |= (-1 != stat("/dev/msm_dsps",&stat_buf));
  // Check device driver for B-family
  ssc_present |= (-1 != stat("/dev/sensors",&stat_buf));
#endif

  clock_gettime( CLOCK_REALTIME, &ts );
  seed = (int)ts.tv_nsec;
  seed += (int)ts.tv_sec;
  seed += getpid();
  srandom( seed );

  if( 0 != sem_init( &open_sem, 0, 0 ) ) {
    LOG_ERROR("%s error initializing semaphore %i", __func__, errno );
  }

  memset( libsensor_cli_data,
          0,
          sizeof(libsensor_client_data_s) * MAX_CLIENTS );

  error = pthread_mutexattr_init( &mutex_attr );
  error |= pthread_mutexattr_settype( &mutex_attr, PTHREAD_MUTEX_ERRORCHECK );
  if( error != 0 ) {
    LOG_ERROR("%s error initializing mutex attribs %d", __func__, error );
  }

  error = pthread_mutex_init( &libsensor_cli_data_mutex, &mutex_attr );
  if( error != 0 ) {
    LOG_ERROR("%s error %d initializing mutex", __func__, error );
  }

  pthread_mutexattr_destroy(&mutex_attr);

  error = pipe2( wakeup_pipe, O_NONBLOCK );
  if( error != 0 ) {
    LOG_ERROR("%s error %d creating wakeup pipe: %s", __func__, errno, strerror(errno) );
  }

  inotify_fd = inotify_init();
  if( inotify_fd == -1 ) {
    LOG_ERROR("%s error %d creating inotify listener: %s", __func__, errno, strerror(errno) );
  }

  pthread_mutexattr_init( &mutex_attr );
  pthread_mutexattr_settype( &mutex_attr, PTHREAD_MUTEX_RECURSIVE );
  for( i = 0; i < MAX_CLIENTS; i++ )
  {
    pthread_cond_init( &libsensor_cli_data[i].cb_q_cond, NULL );
    pthread_mutex_init( &libsensor_cli_data[i].data_cbf_mutex, &mutex_attr );
    pthread_mutex_init( &libsensor_cli_data[i].cb_q_mutex, NULL );

    libsensor_cli_data[i].is_valid = false;
    libsensor_cli_data[i].ctl_socket = -1;
  }
  pthread_mutexattr_destroy(&mutex_attr);

  if( 0 != ( error = pthread_attr_init( &thread_attr ) ) ) {
    LOG_ERROR( "%s pthread_attr_init failure %i", __func__, error );
  } else {
    if( 0 != ( error = pthread_attr_setdetachstate( &thread_attr, PTHREAD_CREATE_DETACHED ) ) ) {
      LOG_ERROR( "%s pthread_attr_setdetachstate failure %i", __func__, error );
    } else {
      error = pthread_create( &listener_thread_id, &thread_attr,
                              libsensor_rx_thread, NULL );
    }
    if( error != 0 ) {
      LOG_ERROR("%s error %d initializing thread", __func__, error );
    }
    pthread_attr_destroy( &thread_attr );
  }
}


/*===========================================================================

  FUNCTION:   sensor1_init

  ===========================================================================*/
sensor1_error_e
sensor1_init( void )
{
  LOG_DEBUG("%s", __func__);
  pthread_once( &init_ctl, sensor1_init_once );
  return SENSOR1_SUCCESS;
}

