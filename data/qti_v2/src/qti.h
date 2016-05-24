
/******************************************************************************

                        QTI.H

******************************************************************************/

/******************************************************************************

  @file    qti.h
  @brief   Qualcomm Tethering Interface module

  DESCRIPTION
  Header file for Qualcomm Tethering Interface.

  ---------------------------------------------------------------------------
  Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/


/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id:$

when       who        what, where, why
--------   ---        -------------------------------------------------------
11/15/12   sb         Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_addr.h>
#include <linux/rtnetlink.h>
#include <linux/netlink.h>
#include <netinet/in.h>
#include <linux/msm_ipa.h>

#include "comdef.h"
#include "ds_util.h"
#include "qmi_client.h"
#include "qmi_qmux_if.h"
#include "qmi_platform.h"
/*===========================================================================
                              MACRO DEFINITIONS
===========================================================================*/

#define MAX_NUM_OF_FD                       10
#define QTI_NL_MSG_MAX_LEN                  1024
#define RNDIS_INTERFACE                     "rndis0"
#define ECM_INTERFACE                       "ecm0"
#define IF_NAME_LEN                         16
#define QTI_INTERFACES                      2
#define QTI_QMI_SVC_INIT_TIMEOUT_MS         500
#define QTI_SUCCESS                         0
#define QTI_FAILURE                         (-1)
#define QTI_DEFAULT_INTERFACE_ID            (-99999)
#define QCMAP_MSGR_QMI_TIMEOUT_VALUE        90000
#define RMNET_USB_MAX_TRANSFER_SIZE         2048
#define QTI_QMUX_IF_TYPE_QMUX               (0x01)
#define WDA_SET_DATA_FORMAT_RESULT_OFFSET   3
#define QTI_QCMAP_MAX_RETRY                 10
#define QTI_QCMAP_MAX_TIMEOUT_MS            500 /*Timeout value in miliseconds */
#define QTI_MAX_FILE_NAME_SIZE              50

#define TRUE 1
#define FALSE 0

#define IPA_BRIDGE_DRIVER_FILE "/dev/ipa_tethering_bridge"


#define FRMNET_CTRL_IOCTL_MAGIC		'r'
#define FRMNET_CTRL_GET_LINE_STATE		_IOR(FRMNET_CTRL_IOCTL_MAGIC, 2, int)
#define SET_DTR_HIGH 1
#define SET_DTR_LOW  0

#define QMI_TYPE_QOS_DATA_FORMAT              (0x10)
#define QMI_TYPE_LINK_PROTOCOL                (0x11)
#define QMI_TYPE_UL_DATA_AGG_PROTOCOL         (0x12)
#define QMI_TYPE_DL_DATA_AGG_PROTOCOL         (0x13)
#define QMI_TYPE_NDP_SIGNATURE                (0x14)
#define QMI_TYPE_DL_DATA_AGG_MAX_DATAGRAMS    (0x15)
#define QMI_TYPE_DL_DATA_AGG_MAX_SIZE         (0x16)


/*! Minimum size of a QMI_CTL message: header data, plus one message with no
    TLVs */
#define QTI_QMUX_QMI_CTL_MIN_MSG_LEN_BYTES \
  (sizeof(qti_qmux_msg_s) - sizeof(qti_qmux_sdu_s) + sizeof(qti_qmux_qmi_ctl_sdu_s))

/*! Minimum size of a regular QMUX message (non-QMI_CTL): header data, plus one
    message with no TLVs */
#define QTI_QMUX_NON_QMI_CTL_MIN_MSG_LEN_BYTES (sizeof(qti_qmux_msg_s))

/*! Minimum size of a generic QMUX message */
#define QTI_QMUX_MIN_MSG_LEN_BYTES QTI_QMUX_QMI_CTL_MIN_MSG_LEN_BYTES

/*! Size of the QMUX header plus I/F type preamble byte */
#define QTI_QMUX_HDR_LEN_BYTES \
  (sizeof(qti_qmux_if_type_t) + sizeof(qti_qmux_hdr_s))

#define QTI_QMUX_SDU_MESSAGE_HDR_LEN_BYTES \
  (sizeof(qti_qmux_if_type_t) + sizeof(qti_qmux_hdr_s) + sizeof(qti_qmux_sdu_hdr_s)+ sizeof(qti_qmux_qmi_msg_hdr_s))

/*! Service type indicating a QMI_CTL message */
#define QTI_QMUX_SVC_TYPE_QMI_CTL (0x00)

#define DEFAULT_AGGR_PROTOCOL 0x00
#define DEFAULT_MAX_DATAGRAMS 0x01
#define DEFAULT_MAX_SIZE      0x5DC
#define DEFAULT_NDP_SIGNATURE 0x00

#define DEFAULT_MAX_DATAGRAMS_IPA 16
#define DEFAULT_MAX_SIZE_IPA      (16*1024)

#define WDA_SERVICE_ID 0x1A
#define WDA_SET_DATA_FORMAT_MESSAGE_ID 0x20

#define USB_TETHERED_SMD_CH    "/dev/smdcntl8"
#define MULTI_RMNET_SMD_CH_1   "/dev/smdcntl9"
#define MULTI_RMNET_SMD_CH_2   "/dev/smdcntl10"
#define MULTI_RMNET_SMD_CH_3   "/dev/smdcntl11"


#define RMNET_USB_DEV_FILE_PATH          "/dev/rmnet_ctrl"
#define MULTI_RMNET_USB_DEV_FILE_PATH_1  "/dev/rmnet_ctrl1"
#define MULTI_RMNET_USB_DEV_FILE_PATH_2  "/dev/rmnet_ctrl2"
#define MULTI_RMNET_USB_DEV_FILE_PATH_3  "/dev/rmnet_ctrl3"

#define IPA_LCID_USB_TETHERED      8
#define IPA_LCID_MULTI_RMNET_1     10
#define IPA_LCID_MULTI_RMNET_2     11
#define IPA_LCID_MULTI_RMNET_3     12




#define RMNET_CONFIG_FILE "/etc/rmnet_config.txt"

/*===========================================================================
                              VARIABLE DECLARARTIONS
===========================================================================*/


/*---------------------------------------------------------------------------
   Function pointer registered with the socket listener
   This function is used for reading from a socket on receipt of an incoming
   netlink event
---------------------------------------------------------------------------*/
typedef int (* qti_sock_thrd_fd_read_f) (int fd);

/*--------------------------------------------------------------------------
  QMUX handle used to call QMUXD APIs
---------------------------------------------------------------------------*/
static qmi_qmux_if_hndl_t qti_qmux_qmi_handle;

/*--------------------------------------------------------------------------
  Boolean to validate QMUX handle
---------------------------------------------------------------------------*/
static boolean qti_qmux_qmi_handle_valid = FALSE;

/*--------------------------------------------------------------------------
  Control channel message preamble byte: Interface Type, which indicates the
  protocol used for the message
---------------------------------------------------------------------------*/
typedef uint8 qti_qmux_if_type_t;

/*--------------------------------------------------------------------------
   The different states through which QTI transitions during a connect
   and disconnect of USB cable
---------------------------------------------------------------------------*/
typedef enum
{
  QTI_INIT = 0,
  QTI_LINK_UP_WAIT,
  QTI_LINK_UP,
  QTI_LINK_DOWN_WAIT,
  QTI_LINK_DOWN
} qti_nl_state_e;

/*--------------------------------------------------------------------------
   Interfaces names
---------------------------------------------------------------------------*/
typedef enum
{
  RNDIS_IF = 1,
  ECM_IF
} qti_interface_e;

/*--------------------------------------------------------------------------
   Events that need to propagated to QCMAP from QTI
---------------------------------------------------------------------------*/
typedef enum
{
  QTI_LINK_UP_EVENT =1,
  QTI_LINK_DOWN_EVENT
} qti_qcmap_event_e;

/*--------------------------------------------------------------------------
   States to handle QMUX messages between modem and client
---------------------------------------------------------------------------*/
typedef enum
{
  QTI_RX_INIT = 0,
  QTI_RX_CLIENT_WAIT,
  QTI_RX_MODEM_WAIT
} qti_qmi_mess_state_e;

/*--------------------------------------------------------------------------
   Interfaces name and interface index
---------------------------------------------------------------------------*/
typedef struct
{
 char dev_name[IF_NAME_LEN];
 int if_index;
 boolean enabled;
} qti_ifi_dev_name_t;

/*--------------------------------------------------------------------------
   QTI configuration variable which maintains the information needed with
   respect to a QTI call
---------------------------------------------------------------------------*/
typedef struct
{
 qti_nl_state_e          state;
 qti_ifi_dev_name_t      if_dev[QTI_INTERFACES];
 qmi_client_type         qti_qcmap_msgr_handle;
 uint32_t                qti_mobile_ap_handle;
} qti_conf_t;

/*--------------------------------------------------------------------------
   Stores the mapping of a socket descriptor and its associated read
   function
---------------------------------------------------------------------------*/
typedef struct
{
 int sk_fd;
 qti_sock_thrd_fd_read_f read_func;
} qti_nl_sk_fd_map_info_t;

/*--------------------------------------------------------------------------
   Stores the socket information associated with netlink sockets required
   to listen to netlink events
---------------------------------------------------------------------------*/
typedef struct
{
 qti_nl_sk_fd_map_info_t sk_fds[MAX_NUM_OF_FD];
 fd_set fdset;
 int num_fd;
 int max_fd;
} qti_nl_sk_fd_set_info_t;

/*--------------------------------------------------------------------------
   Socket descriptor paramters
---------------------------------------------------------------------------*/
typedef struct
{
 int                 sk_fd;       /* socket descriptor */
 struct sockaddr_nl  sk_addr_loc; /*  stores socket parameters */
} qti_nl_sk_info_t;

/*--------------------------------------------------------------------------
   Stoes the metainfo present in the incoming netlink message
---------------------------------------------------------------------------*/
typedef struct
{
 struct ifinfomsg  metainfo;
} qti_nl_link_info_t;

/*--------------------------------------------------------------------------
   Netlink message: used to decode the incoming netlink message
---------------------------------------------------------------------------*/
typedef struct
{
 unsigned int type;
 boolean link_event;
 qti_nl_link_info_t nl_link_info;
} qti_nl_msg_t;

/*--------------------------------------------------------------------------
  Netlink message structure used to send GET_LINK
---------------------------------------------------------------------------*/
typedef struct
{
  struct nlmsghdr hdr;
  struct rtgenmsg gen;
}nl_req_type;

/*--------------------------------------------------------------------------
  QMUX Header, follows the preamble
---------------------------------------------------------------------------*/
typedef struct __attribute__((packed)) {
  uint16 length;     /*!< Length of the QMUX message including the QMUX header,but not the I/F Type */
  uint8  ctl_flags;  /*!< QMUX Control Flags indicating the sender */
  uint8  svc_type;   /*!< QMI service type of the SDU */
  uint8  client_id;  /*!< Client ID pertaining to this message */
} qti_qmux_hdr_s;

/*--------------------------------------------------------------------------
  QMUX Service Data Unit Transaction Header for a non-QMI_CTL message
---------------------------------------------------------------------------*/
typedef struct __attribute__((packed)) {
  uint8  svc_ctl_flags;  /*!< QMUX SDU Control Flags indicating message type */
  uint16 txn_id;         /*!< Transaction ID (unique among control points) */
} qti_qmux_sdu_hdr_s;

/*--------------------------------------------------------------------------
  QMI_CTL Message Header (replaces QMUX SDU Transaction Header for QMI_CTL
    messages)
---------------------------------------------------------------------------*/
typedef struct __attribute__((packed)) {
  uint8 svc_ctl_flags;  /*!< QMI_CTL Control Flags indicating message type */
  uint8 txn_id;         /*!< Transaction ID (unique for every message) */
} qti_qmux_qmi_ctl_msg_hdr_s;

/*-------------------------------------------------------------------------
  Message ID and message length
--------------------------------------------------------------------------*/
typedef struct __attribute__((packed)) {
  uint16 msg_id;     /*!< QMI Service-specific message ID */
  uint16 msg_length; /*!< Length of TLV data to follow */
} qti_qmux_qmi_msg_hdr_s;

/*-------------------------------------------------------------------------
  SDU structure specifically for QMI_CTL messages
--------------------------------------------------------------------------*/
typedef struct __attribute__((packed)) {
  qti_qmux_qmi_ctl_msg_hdr_s hdr;
  qti_qmux_qmi_msg_hdr_s     msg;
} qti_qmux_qmi_ctl_sdu_s;

/*-------------------------------------------------------------------------
  SDU structure for non-QMI_CTL messages
--------------------------------------------------------------------------*/
typedef struct __attribute__((packed)) {
  qti_qmux_sdu_hdr_s     hdr;
  qti_qmux_qmi_msg_hdr_s msg;
} qti_qmux_sdu_s;

/*-------------------------------------------------------------------------
  QMUX Message including everything for a single QMI message in the
  transaction except for the TLV data
  @note Because of the union for QMI_CTL/regular SDUs, the size of this struct
  is not accurate to the size of the QMI_CTL header data. Use the constant
  QTI_QMUX_QMI_CTL_MIN_MSG_LEN_BYTES to get an offset to the start of TLV data
  for a QMI_CTL message
--------------------------------------------------------------------------*/
typedef struct __attribute__((packed)) {
  qti_qmux_if_type_t if_type;
  qti_qmux_hdr_s     qmux_hdr;
  union __attribute__((packed)) {
    qti_qmux_qmi_ctl_sdu_s qmi_ctl;
    qti_qmux_sdu_s         qmux;
  } sdu;

  /*note Followed by msg_length bytes of TLV data, and potentially additional
    msg_id/msg_length/tlv_data sets of data */
} qti_qmux_msg_s;

/*-------------------------------------------------------------------------
  Set Data Format Parameters
--------------------------------------------------------------------------*/
typedef struct{
  boolean qos_enable;
	uint32 link_protocol;
	uint32 ul_aggr_prot;
	uint32 dl_aggr_prot;
	uint32 ndp_signature;
	uint32 dl_max_datagrams;
	uint32 dl_max_size;
}set_data_format_param_s;

/*-------------------------------------------------------------------------
  Set data format status with IPA
--------------------------------------------------------------------------*/
typedef struct{
  boolean link_prot_ipa_status;
  boolean aggr_param_status;
}set_data_format_status;

/*-------------------------------------------------------------------------
  QMI message handling status
--------------------------------------------------------------------------*/
typedef struct{
  int                    usb_fd;
  char                   usb_device_file[QTI_MAX_FILE_NAME_SIZE];
  qmi_connection_id_type qmux_conn_id;
  char                   smd_device_file[QTI_MAX_FILE_NAME_SIZE];
  int                    ipa_lcid;
  boolean                multi_rmnet_enabled;
  boolean                dtr_enabled;
  qti_qmi_mess_state_e   qti_qmi_mess_state;
}qti_rmnet_state_config;

/*-------------------------------------------------------------------------
  Structure to store rmnet_config to support autoconnect
--------------------------------------------------------------------------*/
typedef struct{
  boolean auto_connect;
  enum teth_link_protocol_type link_prot;
  enum teth_aggr_protocol_type ul_aggr_prot;
  enum teth_aggr_protocol_type dl_aggr_prot;
}qti_rmnet_autoconnect_config;


/*-------------------------------------------------------------------------
  Temporary buffer structure used to hold QMUX packet
--------------------------------------------------------------------------*/
typedef struct {
  uint32 size;  /*!< Size of data buffer in bytes */
  void  *data;  /*!< Pointer to location of buffer */
} qti_qmux_buf_s;




/*===========================================================================
                       FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================

FUNCTION QTI_NETLINK_INIT()

DESCRIPTION

  This function initializes QTI:
  - obtains the interface index and name mapping
  - initializes the QTI configuration varaible

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

==========================================================================*/
int qti_netlink_init(qti_conf_t * qti_conf);


/*===========================================================================

FUNCTION QTI_QCMAP_INIT()

DESCRIPTION

  This function initializes QTI:
  - initializes a QCMAP MSGR client

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

==========================================================================*/

int qti_qcmap_init(qti_conf_t * qti_conf);

/*===========================================================================
FUNCTION QTI_NL_LISTENER_INIT()

DESCRIPTION

  This function initializes netlink sockets and also performs a query to find
  any netlink events that could happened before netlink socket
  initialization.

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None
=========================================================================*/
int qti_nl_listener_init
(
  unsigned int nl_type,
  unsigned int nl_groups,
  qti_nl_sk_fd_set_info_t * sk_fdset,
  qti_sock_thrd_fd_read_f read_f
);


/*===========================================================================
FUNCTION QTI_NL_RECV_MSG()

DESCRIPTION

  Function to receive incoming messages over the NETLINK routing socket.

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None
==========================================================================*/
int qti_nl_recv_msg(int fd);

/*===========================================================================

FUNCTION QTI_QCMAP_CMD_EXEC()

DESCRIPTION

  This function performs the execution of commands present in command queue.
  It mainly is involved in sending required QCMAP messages to QCMAP daemon to
  perform QCMAP specific operations

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/

int qti_qcmap_cmd_exec(qti_qcmap_event_e event, qti_interface_e interface);


/*===========================================================================

FUNCTION QTI_QMUX_TX_TO_MODEM()

DESCRIPTION

  This function
  - delivers all the QMI packets from USB interface to QMUXD interface.
  - processes the WDA set data format message and informs bridge driver
  about link protocol and aggregation protocol.

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/



int qti_qmux_tx_to_modem
(
  qti_qmux_msg_s *qmux_msg,
  uint32          qmux_msg_len
);

/*===========================================================================

FUNCTION QTI_QMUX_RX_CB()

DESCRIPTION

  This function
  - delivers all the QMI response packets from QMUXD interface to USB interface.
  - processes the WDA set data format response message, aggregates responses
    from modem and IPA and sends back the response to the client

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/

void qti_qmux_rx_cb
(
  qmi_connection_id_type conn_id,
  qmi_service_id_type    service_id,
  qmi_client_id_type     client_id,
  unsigned char          control_flags,
  unsigned char         *rx_msg,
  int                    rx_msg_len
);

/*===========================================================================

FUNCTION QTI_RMNET_USB_RECV_MSG()

DESCRIPTION

  This function
  - receives QMI message from USB interface.

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/
int qti_rmnet_usb_recv_msg
(
   int rmnet_usb_fd
);

/*===========================================================================

FUNCTION QTI_RMNET_USB_RECV_MSG()

DESCRIPTION

  This function
  - sets up QTI to start listening for QMI packets coming on USB interface.

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/
int qti_rmnet_listener_init
(
  qti_rmnet_state_config  * rmnet_state,
  qti_nl_sk_fd_set_info_t * fd_set,
  qti_sock_thrd_fd_read_f read_f
);

/*===========================================================================

FUNCTION QTI_RMNET_QMUX_OPEN()

DESCRIPTION

  This function
  - sets up the interface between QTI and QMUX.

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/
int qti_rmnet_qmux_open
(
  qmi_qmux_if_rx_msg_hdlr_type qti_qmux_rx_cb,
  qti_rmnet_state_config       * rmnet_state
);

/*===========================================================================

FUNCTION PRINT_BUFFER()

DESCRIPTION

  This function
  - prints the QMI packet.

DEPENDENCIES
  None.

RETURN VALUE


SIDE EFFECTS
  None

/*=========================================================================*/
void print_buffer
(
  char *buf,
  int size
);
/*===========================================================================

FUNCTION PROCESS_USB_RESET()

DESCRIPTION

  This function
  - if auto-connect is enabled sets the bridge driver to the mode present in
    rmnet_config.txt.
  - sends DTR high/low to modem

DEPENDENCIES
  None.

RETURN VALUE


SIDE EFFECTS
  None

/*=========================================================================*/
int process_usb_reset();
