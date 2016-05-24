/*===========================================================================
                           usf_epos_defs.h

DESCRIPTION: This header file contains all shared structs, enums and defines
between all EPOS service files.

Copyright (c) 2012-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/
#ifndef __USF_EPOS_DEFS_
#define __USF_EPOS_DEFS_

#include <usf_unix_domain_socket.h>
#include <usf_types.h>
#include <ual_util.h>
#include "eposexports.h"
#include "usf_geometry.h"

/*-----------------------------------------------------------------------------
  Consts and macros
-----------------------------------------------------------------------------*/

#define CLIENT_NAME "digitalpen"
#define US_MAX_EVENTS 20
#define MM_TO_LIB_UNIT 100

/**
  Events defines, check:
  "ultrasound/sdk/service/src/qcom/digitalpen/util/DigitalPenEvent.java"
  for more information
*/
#define POWER_STATE_CHANGED 0
#define MIC_BLOCKED 2
#define PEN_DRAW_SCREEN_CHANGED 3
#define BATTERY_STATE 4
// EPOS "Type" defines
#define EPOS_TYPE_VALID_BIT   0x01
#define EPOS_TYPE_PEN_DOWN_BIT   0x02
#define MAX_PERSIST_DATA_SIZE 1024
// Tilt Q factor
#define TILT_Q_FORMAT_SHIFT 30
#define TILT_Q_FACTOR       (1 << TILT_Q_FORMAT_SHIFT)

/**
  Number of "0.01 mm" Epos's units in 1 m
*/
static const long CMM_PER_M = 100000;

/*-----------------------------------------------------------------------------
  Typedefs
-----------------------------------------------------------------------------*/

/**
  "Power save" (PS) states.
*/
typedef enum
{
  PS_STATE_ACTIVE,
  PS_STATE_STANDBY,
  PS_STATE_IDLE,
  PS_STATE_OFF,
  PS_STATE_UNDEF,
} ps_state_enum;

/**
  The calibration file is a file used by EPOS library to save/load data specific
  to each platform/device/etc...
*/
struct calib_file {
  // File path
  char     path[FILE_PATH_MAX_LEN];
  // Pointer to memory where data resides
  void     *calib_packet;
  uint32_t calib_packet_len;
  // When mandatory, daemon will exit on loading error
  bool     mandatory;
};

/**
  EPoint wrapper, support the old pen lib API by assigning zero
  to all the non-existing fields
*/
typedef struct
{
  EPSource Source;        // Base station ID and pen ID
  long     X;             // X coordinate in mm*100
  long     Y;             // Y coordinate in mm*100
  long     Z;             // Z coordinate in mm*100
  long     P;             // Pressure
  long     Type;          // Coordinate type
  long     TiltX;         // TiltX +-128
  long     TiltY;         // TiltY +-128
  long     TiltZ;         // TiltZ +-128
  long     WorkingPlane;  // Working plane - off-screen=1, on-screen=0
} usf_extended_epoint_t;

/**
  EposParams holds information needed for EPOS calculation.
*/
typedef struct
{
  void                  *m_epos_workspace;
  EPoint                *m_out_basic_points;  // Struct from EposExports.h
  usf_extended_epoint_t *m_out_points;
  int                   m_nDownscalePointCounter;
  int                   m_nLastEventIndex;
  bool                  m_bLastValidPointUalSent;
  usf_extended_epoint_t m_nLastPoint;
  int                   m_nLastEventSeqNum;
  usf_event_type        m_events[US_MAX_EVENTS];
  double                m_on_screen_transform_matrix[3][3];
  double                m_off_screen_transform_matrix[3][3];
  float                 m_fuzz[3];
  bool                  m_on_screen_send_points_to_ual;
  bool                  m_on_screen_send_points_to_socket;
  bool                  m_off_screen_send_points_to_ual;
  bool                  m_off_screen_send_points_to_socket;
  int                   m_coord_type_on_disp;
  int                   m_coord_type_off_disp;
  int                   m_nNextFrameSeqNum;
  int                   m_socket_sending_prob;
  int                   m_cfg_point_downscale;
  int                   m_group;
  uint8_t               *m_pPattern;
  int                   m_patternSize;
  dynamic_config_t      m_dconfig;
  int                   m_event_type;
  off_screen_mode_t     m_off_screen_mode;
  plane_properties      m_off_screen_plane;
  // Calibration files
  struct calib_file     m_calib_files[EPOS_CALIB_FILE_COUNT];
  bool                  m_smarter_stand_enable;
  double                m_smarter_stand_angle;
  // Eraser
  eraser_button_index_t m_eraser_button_index;
  eraser_button_mode_t  m_eraser_button_mode;
  // Hover icon
  hover_icon_mode_t m_on_screen_hover_icon_mode;
  hover_icon_mode_t m_off_screen_hover_icon_mode;
} EposParams;

/**
  Parameters, used by work functions and other related services (e.g. SoS)
*/
typedef struct
{
  us_all_info  paramsStruct;
  FILE         *frameFile;
  FILE         *coordFile;
  int          num_of_regions;
  int          numberOfFrames;
  uint32_t     packet_size_in_bytes;
  uint32_t     frame_size_in_bytes;
  uint32_t     frame_hdr_size_in_bytes;
  uint32_t     bytesWriteToFile;
  uint32_t     numOfBytes;
  int          numPoints;
  int          packetCounter;
  uint32_t     pointLogFrameCounter;
  uint8_t      *nextPacket;
  // Parameter defines whether the pen is in active zone
  bool         bActZone;
} work_params_type;

/**
  PS transition type
*/
typedef enum
{
  PS_NO_TRANSIT,
  // Start transition, as no US
  PS_NO_US_TRANSIT,
  // Start transition, as US exists
  PS_US_TRANSIT
} ps_transit_enum;

/**
  The command numbers that might be sent back to EPOS lib
 */
enum usf_epos_command_number {
  // This is a special command number, when used, nothing will be sent
  // back
  NO_CMD      = -1,
  // Get active channels
  CMD_GET_ACT_CHANNELS  = 0x400,
  // TODO: use eposexports constant when next library has it.
  // Set sniffing mode
  CMD_SNIFFING_MODE     = 0x660,
  // Speed of sound
  CMD_SPD_SND_TRANSMIT  = 0x700,
};

// Command types
#define CMD_TYPE_SET 0
#define CMD_TYPE_GET 1

// Number of arguments to be sent back
#define NUM_ARGS 2

/**
  This struct contains the needed information to send back to EPOS lib
 */
struct usf_epos_command {
  // Command type (set or get)
  long cmd_type;
  // Number to be sent
  // NOTE: the usage of long instead of enum usf_epos_command_number is because
  //       the epos lib expects this parameter to be long.
  long cmd_num;
  // Args to be sent
  long args[NUM_ARGS];
};

#endif // __USF_EPOS_DEFS_
