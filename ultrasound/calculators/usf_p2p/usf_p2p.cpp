/*===========================================================================
                           usf_p2p.cpp

DESCRIPTION: Implementation of the P2P daemon.


INITIALIZATION AND SEQUENCING REQUIREMENTS:
  If not started through java app then make sure to have
  correct /data/usf/p2p/usf_p2p.cfg file linked to the wanted cfg file
  placed in /data/usf/p2p/cfg/.

Copyright (c) 2011-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/
#define LOG_TAG "usf_p2p"

/*----------------------------------------------------------------------------
  Include files
----------------------------------------------------------------------------*/
#include "usf_log.h"
#include <ual.h>
#include <cutils/properties.h>
#include <ual_util.h>
#include <P2PExports.h>
#include <stdlib.h>
#include <errno.h>
#include "ual_util_frame_file.h"
#include <usf_unix_domain_socket.h>

/*----------------------------------------------------------------------------
  Defines
----------------------------------------------------------------------------*/
#define LINK_CFG_FILE_LOCATION "/data/usf/p2p/usf_p2p.cfg"
#define USF_DSP_VER_FILE "/data/usf/p2p/usf_dsp_ver.txt"
#define FRAME_FILE_DIR_PATH "/data/usf/p2p/rec/"
#define PATTERN_DIR_PATH "/data/usf/p2p/pattern/"
#define BUFFER_SIZE 500
#define US_MAX_EVENTS 100
#define EPOS_PACKET_SIZE_DWORDS 256
#define STATISTIC_PRINT_INTERVAL 500
#define SOCKET_PROBLEM_MSG_INTERVAL 50
#define EXIT_SIGTERM 3
#define ECHO_P2P_MAX_AZIMUTH_ANGLES 2

enum p2p_event_dest
{
  DEST_UAL = 0x01,
  DEST_SOCKET = 0x02,
};

/*-----------------------------------------------------------------------------
  Typedefs
-----------------------------------------------------------------------------*/
/**
  p2pParams holds information needed for P2P calculation.
*/
typedef struct
{
  signed char       *m_p2p_workspace;
  usf_event_type    m_events[US_MAX_EVENTS];  // Array of struct from sys/stat.h
  bool              m_send_points_to_ual; // For future use
  bool              m_send_points_to_socket;
  int               m_nNextFrameSeqNum;
  int               m_socket_sending_prob;
  uint8_t*          m_pPattern;
  int               m_deviceId;
  int               m_patternSize;
  int               m_iPatternType;
  int               m_stub;
} P2PParams;

/**
  P2PStats holds information about statistics from usf_p2p
  running.
*/
typedef struct
{
  int   m_nPointsCalculated;
  int   m_nTotalFrames;
  int   m_nLostFrames;
  int   m_nOutOfOrderErrors;
} P2PStats;

/*-----------------------------------------------------------------------------
  Static Variable Definitions
-----------------------------------------------------------------------------*/
/**
  Ptr to the cfg file. The ptr is global bacause we want to
  close the file before exit in the function p2p_exit.
  This function is called also from the signal_handler function
  which doesn't know the cfg file.
*/
static FILE *cfgFile = NULL;


/**
  p2pParams will hold all the information needed for P2P
  calculation.
*/
static P2PParams p2pParams;


/**
  P2PStats will hold all the statistics needed.
*/
static P2PStats p2pStats;

/**
  m_p2p_data_socket is pointer to the thread which handles data
  socket communication with the service.
*/
static DataUnSocket *m_p2p_data_socket;


const uint16_t ECHO_FRAME_SEQNUM_OFFSET = 8; // bytes

/**
  P2P calculator name
*/
static const char* CLIENT_NAME =  "p2p";

/**
  P2P calculator version
*/
static const char* CLIENT_VERSION = "1.3.2";

/**
 * The name of the file containg the pid of the daemon
 */
static const char* PID_FILE_NAME = "usf_p2p.pid";

/**
  The daemon running control
*/
static volatile bool sb_run = true;

/*------------------------------------------------------------------------------
  Function definitions
------------------------------------------------------------------------------*/

/*==============================================================================
  FUNCTION:  p2p_exit
==============================================================================*/
/**
  Perform clean exit of the daemon.
*/
int p2p_exit (int status)
{
  QcUsP2PLibTerminate(p2pParams.m_p2p_workspace);

  int rc = ual_close();
  LOGD("%s: ual_close: rc=%d;",
       __FUNCTION__,
       rc);

  if (NULL != cfgFile)
  {
    fclose(cfgFile);
    cfgFile = NULL;
  }

  if (NULL != p2pParams.m_p2p_workspace)
  {
    free(p2pParams.m_p2p_workspace);
    p2pParams.m_p2p_workspace = NULL;
  }

  if (NULL != p2pParams.m_pPattern)
  {
    free(p2pParams.m_pPattern);
    p2pParams.m_pPattern = NULL;
  }

  if (NULL != m_p2p_data_socket)
  {
    delete m_p2p_data_socket;
    m_p2p_data_socket = NULL;
  }

  LOGI("%s: P2P end. status=%d",
       __FUNCTION__,
       status);

  int ret = ual_util_remove_declare_pid(PID_FILE_NAME);
  if (0 != ret)
  {
    LOGW("%s: Removing pid file failed",
         __FUNCTION__);
  }

  // Must update flag, so that init would not restart the daemon.
  ret = property_set("ctl.stop",
                         "usf_p2p");
  if (0 != ret)
  {
    LOGW("%s: property_set failed",
         __FUNCTION__);
  }


  _exit(status);
}

/*==============================================================================
  FUNCTION:  p2p_params_init
==============================================================================*/
/**
  Init p2pParam struct.
*/
void p2p_params_init (us_all_info *paramsStruct)
{
  uint32_t port;
  char *temp = NULL, *ip = NULL;
  int ret;

  p2pParams.m_p2p_workspace = NULL;
  p2pParams.m_deviceId = paramsStruct->usf_p2p_device_uid;
  p2pParams.m_stub = false;

  p2pParams.m_patternSize = paramsStruct->usf_rx_pattern_size *
                            paramsStruct->usf_rx_sample_width/BYTE_WIDTH;

  p2pParams.m_iPatternType = paramsStruct->usf_p2p_pattern_type;

  p2pParams.m_pPattern = (uint8_t *)malloc(p2pParams.m_patternSize);

  if (NULL == p2pParams.m_pPattern)
  {
    LOGE("%s: Failed to allocate %d bytes",
         __FUNCTION__,
         paramsStruct->usf_rx_pattern_size *
         sizeof(paramsStruct->usf_rx_sample_width));
    p2p_exit(EXIT_FAILURE);
  }

  if (paramsStruct->usf_p2p_event_dest & DEST_UAL)
  {
    p2pParams.m_send_points_to_ual = true;
  }
  else
  {
    p2pParams.m_send_points_to_ual = false;
  }

  if (paramsStruct->usf_p2p_event_dest & DEST_SOCKET)
  {
    p2pParams.m_send_points_to_socket = true;
  }
  else
  {
    p2pParams.m_send_points_to_socket = false;
  }

  p2pParams.m_nNextFrameSeqNum = -1;

  p2pParams.m_socket_sending_prob = SOCKET_PROBLEM_MSG_INTERVAL;

  p2pStats.m_nPointsCalculated = 0;
  p2pStats.m_nTotalFrames = 0;
  p2pStats.m_nLostFrames = 0;
  p2pStats.m_nOutOfOrderErrors = 0;

  if (p2pParams.m_send_points_to_socket)
  {
    m_p2p_data_socket =
      new DataUnSocket("/data/usf/p2p/data_socket");

    if (0 != m_p2p_data_socket->start())
    {
      LOGE("%s: Starting data socket failed.",
           __FUNCTION__);
      p2p_exit(EXIT_FAILURE);
    }
  }

}

/*==============================================================================
  FUNCTION:  p2p_init
==============================================================================*/
/**
  Init P2P resources.
*/
void p2p_init (us_all_info *paramsStruct)
{
  // Allocate memory for P2P algorithm.
  int p2p_workspace_size = 0;
  int mic, spkr, dim;
  float mics_info[US_FORM_FACTOR_CONFIG_MAX_MICS][COORDINATES_DIM] = {{0}};
  float spkrs_info[US_FORM_FACTOR_CONFIG_MAX_SPEAKERS][COORDINATES_DIM] = {{0}};
  float mics_coords[COORDINATES_DIM][US_FORM_FACTOR_CONFIG_MAX_MICS] = {{0}};
  float speaks_coord[COORDINATES_DIM][US_FORM_FACTOR_CONFIG_MAX_SPEAKERS] = {{0}};

  QcUsP2PLibGetSizes(&p2p_workspace_size);
  p2pParams.m_p2p_workspace =
  (signed char *)malloc(p2p_workspace_size * sizeof(signed char));
  if (NULL == p2pParams.m_p2p_workspace)
  {
    LOGE("%s: Failed to allocate %d bytes.",
         __FUNCTION__,
         p2p_workspace_size);
    p2p_exit(EXIT_FAILURE);
  }

  int mics_num = paramsStruct->usf_tx_port_count;
  for (mic = 0; mic < mics_num; mic++)
  {
    if (-1 == ual_util_get_mic_config (mic, mics_info[mic]))
    {
      LOGE("%s: get_mic_config for mic %d failed.",
           __FUNCTION__,
           mic);
      p2p_exit(EXIT_FAILURE);
    }
  }

  int spkr_num = paramsStruct->usf_rx_port_count;
  for (spkr = 0; spkr < spkr_num; spkr++)
  {
    if (-1 == ual_util_get_speaker_config (spkr, spkrs_info[spkr]))
    {
      LOGE("%s: ual_util_get_speaker_config for speaker %d failed.",
           __FUNCTION__,
           spkr);
      p2p_exit(EXIT_FAILURE);
    }
  }

  // Fiting mics and speakers info to the P2P lib API
  for (dim = 0; dim < COORDINATES_DIM; dim++)
  {
    for (int mic = 0; mic < mics_num; mic++)
    {
      mics_coords[dim][mic] = mics_info[mic][dim];
    }
    for (int spkr = 0; spkr < spkr_num; spkr++)
    {
      speaks_coord[dim][spkr] = spkrs_info[spkr][dim];
    }
  }

  LOGD("mics_num=%d; Mic 1 = (%.5f, %.5f), Mic 2 = (%.5f, %.5f), Mic 3 = (%.5f, %.5f), "
       "spkr_num=%d; Spkr = (%.5f, %.5f), DeviceID = %d",
       mics_num,
       mics_coords[X_IND][0],
       mics_coords[Y_IND][0],
       mics_coords[X_IND][1],
       mics_coords[Y_IND][1],
       mics_coords[X_IND][2],
       mics_coords[Y_IND][2],
       spkr_num,
       speaks_coord[X_IND][0],
       speaks_coord[Y_IND][0],
       p2pParams.m_deviceId);

  int rc = QcUsP2PLibInit(p2pParams.m_p2p_workspace,
                          p2pParams.m_deviceId,
                          p2pParams.m_iPatternType,
                          mics_num,
                          mics_coords[0],
                          mics_coords[1],
                          mics_coords[2],
                          spkr_num,
                          speaks_coord[0],
                          speaks_coord[1],
                          speaks_coord[2],
                          paramsStruct->usf_tx_port_data_size,
                          paramsStruct->usf_rx_port_data_size);

  if (rc)
  {
    LOGE("%s: AlgorithmInit failed.",
         __FUNCTION__);
    p2p_exit(EXIT_FAILURE);
  }

  LOGD("%s: P2P lib init completed.",
       __FUNCTION__);
}

/*==============================================================================
  FUNCTION:  p2p_get_points
==============================================================================*/
/**
  Call QcUsP2PLibEngine() from P2P lib.
  Returns 0 for number of points goes to UAL.
*/
int p2p_get_points (short *pPacket)
{
  int     rc = 0;
  float   fAzimuthAngle[ECHO_P2P_MAX_AZIMUTH_ANGLES] = {0};
  float   fInclinationAngle[ECHO_P2P_MAX_AZIMUTH_ANGLES];
  float   fDistance[ECHO_P2P_MAX_AZIMUTH_ANGLES];
  char    cOutputValid[ECHO_P2P_MAX_AZIMUTH_ANGLES]= {0};
  int     iPatternUpdate = 0;

  QcUsP2PLibEngine(pPacket,
                   (short int *) p2pParams.m_pPattern,
                   fAzimuthAngle,
                   fInclinationAngle,
                   fDistance,
                   cOutputValid,
                   0,
                   &iPatternUpdate);

  if (p2pParams.m_stub)
  {
    return 0;
  }

  // If pPacket is NULL then we try to update pattern from P2P lib
  // for the first time (and not from pattern file received from cfg file).
  if ((NULL == pPacket) && (1 != iPatternUpdate))
  {
    LOGE("%s: QcUsP2PLibEngine failed.",
         __FUNCTION__);
    p2p_exit(EXIT_FAILURE);
  }

  // Update pattern on runtime by P2P lib.
  if (1 == iPatternUpdate)
  {
    if ((NULL == p2pParams.m_pPattern))
    {
      LOGE("%s: QcUsP2PLibEngine failed.",
           __FUNCTION__);
      p2p_exit(EXIT_FAILURE);
    }
    else
    {
      LOGD("%s: Update pattern from P2P lib.",
           __FUNCTION__);

      // Pattern is transmitted only once. DSP transmits pattern in loop.
      rc = ual_write(p2pParams.m_pPattern,
                     p2pParams.m_patternSize);
      if (1 != rc)
      {
        LOGE("%s: ual_write failed.",
             __FUNCTION__);
        p2p_exit(EXIT_FAILURE);
      }
    }
  }

  if (p2pParams.m_send_points_to_socket)
  {
    for (int i=0; i<ECHO_P2P_MAX_AZIMUTH_ANGLES; i++)
    {
      if (cOutputValid[i])
      {
        rc = m_p2p_data_socket->send_p2p_event(p2pParams.m_deviceId,
                                               i,
                                               fAzimuthAngle[i]);
        if (0 > rc)
        {
          // If we got here there is some problem in sending azimuth to socket.
          // The m_socket_sending_prob starts from SOCKET_PROBLEM_MSG_INTERVAL
          // and only when it gets to 0 a warning msg is shown to the user and
          // m_socket_sending_prob set to SOCKET_PROBLEM_MSG_INTERVAL again.
          p2pParams.m_socket_sending_prob--;
          if (0 == p2pParams.m_socket_sending_prob)
          {
            LOGW("%s: SendAzimuth() failed.",
                 __FUNCTION__);
            p2pParams.m_socket_sending_prob = SOCKET_PROBLEM_MSG_INTERVAL;
          }
        }
        else
        {
          p2pParams.m_socket_sending_prob = SOCKET_PROBLEM_MSG_INTERVAL;
        }

        p2pStats.m_nPointsCalculated++;
      }
    }
  }

  if (p2pParams.m_send_points_to_ual)
  {
    LOGW("%s: Send to UAL not implemented.",
         __FUNCTION__);
  }

  return 0;
}

/*==============================================================================
  FUNCTION:  print_DSP_ver
==============================================================================*/
/**
  Print DSP version to file.
*/
void print_DSP_ver()
{
  char szVersion [256];
  uint32_t szVersionSize = sizeof(szVersion) - 1;
  FILE* fp = fopen (USF_DSP_VER_FILE,
                    "wt");
  if (fp == NULL)
  {
    LOGE("%s: Could not open %s - %s",
         __FUNCTION__,
         USF_DSP_VER_FILE,
         strerror(errno));
    p2p_exit(EXIT_FAILURE);
  }

  QcUsP2PLibGetVersion(szVersion,
                       (int*)&szVersionSize);
  if (szVersionSize > sizeof(szVersion) - 1)
  {
    LOGW("%s: Wrong version size (%d)",
         __FUNCTION__,
         szVersionSize);
  }
  else
  {
    szVersion[szVersionSize] = 0;
    fprintf (fp, "%s\n",
             szVersion);
  }

  if (strncmp(szVersion, STUB_VERSION, strlen(STUB_VERSION)) == 0)
  {
    p2pParams.m_stub = true;
  }

  fclose (fp);
}

/*==============================================================================
  FUNCTION:  signal_handler
==============================================================================*/
/**
  Perform clean exit after receive signal.
*/
void signal_handler (int sig)
{
  LOGD("%s: Received signal %d; sb_run=%d",
         __FUNCTION__, sig, sb_run);
  // All supportd signals cause the daemon exit
  sb_run = false;
}

/*==============================================================================
  FUNCTION:  main
==============================================================================*/
/**
  Main function of the P2P daemon. Handle all the P2P operations.
*/
int main (void)
{
  int ret, ind = 0, numPoints = 0, packetCounter = 0;
  int iPatternUpdate = 0;
  FILE *frameFile = NULL;
  static us_all_info paramsStruct;
  bool rc = false, frame_file_created = false;
  ual_data_type data;
  uint32_t frame_hdr_size_in_bytes;
  uint32_t packet_size_in_bytes, packets_in_frame, frame_size_in_bytes;

  LOGI("%s: P2P start",
       __FUNCTION__);

  // Setup signal handling
  signal(SIGHUP,
         signal_handler);
  signal(SIGTERM,
         signal_handler);
  signal(SIGINT,
         signal_handler);
  signal(SIGQUIT,
         signal_handler);

  if (false == ual_util_is_supported((char *)CLIENT_NAME))
  {
    LOGE("%s: Daemon is not supported",
         __FUNCTION__);
    p2p_exit(EXIT_FAILURE);
  }

  if (ual_util_declare_pid(getpid(),
                           PID_FILE_NAME))
  {
    LOGE("%s: Declare_pid failed",
         __FUNCTION__);
  }

  if (ual_util_daemon_init(&paramsStruct,
                           (char *)LINK_CFG_FILE_LOCATION,
                           cfgFile,
                           (char *)CLIENT_NAME))
  {
    LOGE("%s: ual_util init failed",
         __FUNCTION__);
    p2p_exit(EXIT_FAILURE);
  }

  ual_cfg_type cfg;
  cfg.usf_dev_id = 1;
  cfg.ual_mode = static_cast<ual_work_mode_type>(paramsStruct.ual_work_mode);
  rc = ual_open(&cfg);

  if (1 != rc)
  {
    LOGE("%s: ual_open failed.",
         __FUNCTION__);
    p2p_exit(EXIT_FAILURE);
  }

  ual_util_print_US_version(CLIENT_NAME,
                            CLIENT_VERSION);

  // Build tx_transparent_data manually. In the future (hopefully),
  // this will be removed.
  ual_util_set_echo_tx_transparent_data(&paramsStruct);

  ual_util_set_tx_buf_size(&paramsStruct);

  if (ual_util_tx_config(&paramsStruct,
                         (char *)CLIENT_NAME))
  {
    LOGE("%s: ual_util_tx_config failed.",
         __FUNCTION__);
    p2p_exit(EXIT_FAILURE);
  }

  // Build rx_transparent_data manually. In the future (hopefully),
  // this will be removed.
  ual_util_set_echo_rx_transparent_data(&paramsStruct);

  ual_util_set_rx_buf_size(&paramsStruct);

  if (ual_util_rx_config(&paramsStruct,
                         (char* )CLIENT_NAME))
  {
    LOGE("%s: ual_util_rx_config failed.",
         __FUNCTION__);
    p2p_exit(EXIT_FAILURE);
  }

  p2p_params_init(&paramsStruct);

  p2p_init(&paramsStruct);

  print_DSP_ver();

  // Send pattern to UAL for the first time

  // Pattern is taken from file named in the cfg file
  if (0 != paramsStruct.usf_rx_pattern[0])
  {
    rc = !(ual_util_read_pattern(p2pParams.m_pPattern,
                                 &paramsStruct,
                                 (char *)PATTERN_DIR_PATH));
    if (1 != rc)
    {
      LOGE("%s: ual_util_read_pattern failed.",
           __FUNCTION__);
      p2p_exit(EXIT_FAILURE);
    }

    // Pattern is transmitted only once. DSP transmits pattern in loop.
    rc = ual_write(p2pParams.m_pPattern,
                   p2pParams.m_patternSize);
    if (1 != rc)
    {
      LOGE("%s: ual_write failed.",
           __FUNCTION__);
      p2p_exit(EXIT_FAILURE);
    }

  }
  // Pattern is taken from P2P lib
  else
  {
    p2p_get_points(NULL);
  }

  if (0 >= paramsStruct.usf_frame_count)
  {
    LOGD("%s: usf_frame_count is %d. No record has made.",
         __FUNCTION__,
         paramsStruct.usf_frame_count);
  }

  frame_hdr_size_in_bytes =
    paramsStruct.usf_tx_frame_hdr_size;

  packet_size_in_bytes =
    paramsStruct.usf_tx_port_data_size *
    sizeof(paramsStruct.usf_tx_sample_width);

  packets_in_frame =
    paramsStruct.usf_tx_port_count;

  frame_size_in_bytes =
    packet_size_in_bytes * packets_in_frame +
    frame_hdr_size_in_bytes;

  uint32_t numOfBytes = 0;

  uint32_t bytesWriteToFile = paramsStruct.usf_frame_count *
                              frame_size_in_bytes;

  int num_of_regions = sizeof(data.region) / sizeof(ual_data_region_type);

  while (sb_run)
  {
    uint8_t* nextFrame = NULL;

    rc = ual_read(&data,
                  p2pParams.m_events,
                  numPoints);
    if (rc != 1)
    {
      LOGE("%s: ual_read failed.",
           __FUNCTION__);
      if (NULL != frameFile)
      {
        fclose(frameFile);
        frameFile = NULL;
      }
      p2p_exit(EXIT_FAILURE);
    }

    if (0 < paramsStruct.usf_frame_count &&
        false == frame_file_created)
    {
      frame_file_created = true;
      // Open frame file from cfg file
      frameFile = ual_util_get_frame_file (&paramsStruct,
                                           (char *)FRAME_FILE_DIR_PATH);
      if (NULL == frameFile)
      {
        LOGE("%s: ual_util_get_frame_file failed",
             __FUNCTION__);
        p2p_exit(EXIT_FAILURE);
      }
    }

    if (0 == data.region[0].data_buf_size)
    {
      continue;
    }

    // Underlay layer provides US data frames in buffers.
    // Each buffer includes one group of the frames.
    // A number of frames is defined by configurable group factor.
    int numberOfFrames = paramsStruct.usf_tx_buf_size / frame_size_in_bytes;
    int group_data_size = numberOfFrames * frame_size_in_bytes;

    for (int r = 0; r < num_of_regions; r++)
    {
      int num_of_groups = data.region[r].data_buf_size /
                          paramsStruct.usf_tx_buf_size;
      uint8_t *pGroupData = data.region[r].data_buf;
      for (int g = 0; g < num_of_groups; g++)
      {
        nextFrame =  pGroupData;

       // Recording
       if (numOfBytes < bytesWriteToFile)
       {
          uint32_t bytestFromGroup =
            (numOfBytes + group_data_size <= bytesWriteToFile) ?
            group_data_size :
            bytesWriteToFile - numOfBytes;
          ual_util_frame_file_write(pGroupData,
                                  sizeof(uint8_t),
                                  bytestFromGroup,
                                  &paramsStruct,
                                  frameFile);
          numOfBytes += bytestFromGroup;

          if (numOfBytes >= bytesWriteToFile)
          {
            if (NULL != frameFile)
            {
              fclose(frameFile);
              frameFile = NULL;
            }
          }
        } // recording

        for (int f = 0; f < numberOfFrames ; f++)
        {
          // Statistics

          int seqNum = *(nextFrame + ECHO_FRAME_SEQNUM_OFFSET /
                       sizeof(short));

          // If this is the first iteration then the frames
          // counter is -1 and we need to update the frames counter.
          if (p2pParams.m_nNextFrameSeqNum == -1)
          {
            p2pParams.m_nNextFrameSeqNum = seqNum;
          }
          // This is not the first iteration.
          else
          {
            if (p2pParams.m_nNextFrameSeqNum != seqNum)
            {
              // We lost some frames so we add the number of lost frames
              // to the statistics.
              if (p2pParams.m_nNextFrameSeqNum < seqNum)
              {
                p2pStats.m_nLostFrames += (seqNum -
                                           p2pParams.m_nNextFrameSeqNum)/
                                           paramsStruct.usf_tx_skip;
              }
              // We got out of order frames so we add the number of
              // out of order frames to the statistics.
              else
              {
                p2pStats.m_nOutOfOrderErrors += (p2pParams.m_nNextFrameSeqNum-
                                                 seqNum)/
                                                 paramsStruct.usf_tx_skip;
              }

              // Update the frames counter to the correct count.
              p2pParams.m_nNextFrameSeqNum = seqNum;
            }
          }
          p2pStats.m_nTotalFrames++;
          // Update the frames counter to the expected count in the next
          // iteration.
          p2pParams.m_nNextFrameSeqNum += paramsStruct.usf_tx_skip;

          packetCounter++;
          if (STATISTIC_PRINT_INTERVAL == packetCounter)
          {
            LOGI("%s: Statistics (printed every %d frames):",
                 __FUNCTION__,
                 STATISTIC_PRINT_INTERVAL);
            LOGI("Points calculated: %d, total frames: %d, lost frames: %d, "
                 "out of order: %d",
                 p2pStats.m_nPointsCalculated,
                 p2pStats.m_nTotalFrames,
                 p2pStats.m_nLostFrames,
                 p2pStats.m_nOutOfOrderErrors);
            packetCounter = 0;
          }

          // Calculation
          numPoints = p2p_get_points ((short *)nextFrame);
          if (numPoints < 0)
          {
            if (NULL != frameFile)
            {
              fclose(frameFile);
              frameFile = NULL;
            }
            p2p_exit(EXIT_FAILURE);
          }

          nextFrame += frame_size_in_bytes;

        } // End for f (frames)
        pGroupData += paramsStruct.usf_tx_buf_size;
      } // g (groups) loop
    } // r (regions) loop
  } // main loop

  p2p_exit(EXIT_SUCCESS);

}
