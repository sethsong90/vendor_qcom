/*!
  @file
  dsi_netctrli.h

  @brief
  This is an internal header file for dsi_netctrl module. It declares
  data used internally by the dsi_netctrl module.
*/

/*===========================================================================
  Copyright (c) 2008-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
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

$Header:  $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
04/19/10   js      modified original to suit multimodem requirements
===========================================================================*/
#ifdef DSI_NETCTRL_OFFTARGET
#include <sys/socket.h>
#else
#include <linux/socket.h> /* sockaddr_storage */
#include <log.h>
#endif
#include <pthread.h>
#include <assert.h>

#include "dsi_netctrl.h"
#include "qmi_wds_srvc.h"
#include "dsi_netctrl_platform.h"
#include "netmgr.h"
#include "qmi_platform_config.h"
#include "qdi.h"

#ifndef _DSI_NETCTRLI_H
#define _DSI_NETCTRLI_H

/*---------------------------------------------------------------------------
                           UTILITY MACROS
---------------------------------------------------------------------------*/
#define DSI_ASSERT( cond, msg ) \
  if ( !( cond ) ) \
  { \
    DSI_LOG_FATAL( "%s", "*************ASSERTION FAILED (soft)***************" ); \
    DSI_LOG_FATAL( "File: %s, Line: %d, [%s]", __FILE__, __LINE__, msg );         \
    DSI_LOG_FATAL( "%s", "***************************************************" ); \
  }

/* Hard Assert */
#define DSI_ASSERT_H( cond, msg ) \
  if ( !( cond ) ) \
  { \
    DSI_LOG_FATAL( "%s", "*************ASSERTION FAILED (hard)***************" ); \
    DSI_LOG_FATAL( "File: %s, Line: %d, [%s]", __FILE__, __LINE__, msg );         \
    DSI_LOG_FATAL( "%s", "***************************************************" ); \
    assert(0); \
  }

#define DSI_INET4_NTOP(prefix,data)                                                  \
  DSI_LOG_DEBUG(prefix "IPv4 addr [%d.%d.%d.%d]",                                    \
               data[0], data[1], data[2], data[3])
#define DSI_INET6_NTOP(prefix,data)                                                  \
  DSI_LOG_DEBUG(prefix "IPv6 addr [%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x:"             \
                      "%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x]",                        \
               data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],      \
               data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15])

#ifdef DSI_NETCTRL_OFFTARGET
#define DSI_INET_NTOP(prefix,sockaddr)                                         \
        if( AF_INET == (sockaddr)->ss_family ) {                               \
          DSI_INET4_NTOP( prefix, (sockaddr)->__ss_padding );                  \
        } else if( AF_INET6 == (sockaddr)->ss_family ) {                       \
          DSI_INET6_NTOP( prefix, (sockaddr)->__ss_padding );                  \
        } else                                                                 \
          DSI_LOG_DEBUG("Unsupported addr family: 0x%x", (sockaddr)->ss_family)
#else
#define DSI_INET_NTOP(prefix,sockaddr)                                         \
        if( AF_INET == (sockaddr)->ss_family )                                 \
          DSI_INET4_NTOP( prefix, (sockaddr)->__data )                         \
        else if( AF_INET6 == (sockaddr)->ss_family )                           \
          DSI_INET6_NTOP( prefix, (sockaddr)->__data )                         \
        else                                                                   \
          DSI_LOG_DEBUG("Unsupported addr family: 0x%x", (sockaddr)->ss_family)
#endif

#define DSI_IS_ADDR_FAMILY_VALID(addr)                                      \
        ((AF_INET == (addr)->ss_family) || (AF_INET6 == (addr)->ss_family))


#define DSI_LOCK_MUTEX(mutex) \
  { \
    pthread_mutex_lock(mutex); \
    DSI_LOG_VERBOSE("mutex [%p] locked", mutex); \
  }

#define DSI_UNLOCK_MUTEX(mutex) \
  { \
    pthread_mutex_unlock(mutex); \
    DSI_LOG_VERBOSE("mutex [%p] unlocked", mutex); \
  }

#define DSI_MINIMUM(a,b) ((a)<(b)?(a):(b))

#define _DSI_UPDATE_CALL_STATE(_st, _call_state, _call_state_str) \
  (_st)->priv.call_state = (_call_state); \
  DSI_LOG_DEBUG("DSI call state logging: Handle %d, call state %s", (_st)->priv.dsi_iface_id, _call_state_str)

#define DSI_UPDATE_CALL_STATE(_st, _call_state) \
  _DSI_UPDATE_CALL_STATE(_st, _call_state, #_call_state)

/* ================= ps_iface_defs ============================*/
/* unfortunately, they decided to use definitions from
   ps_iface_defs.h to be used as value for technology
   that is returned by qmi_wds_route_look_up

   NOTE: this could have some defines that are not part of
   qmi_wds_iface_name_type defined in qmi_wds_srvc.h
   some unpublished iface (from QMI_WDS spec)could be returned*/
#define DSI_PS_CDMA_SN_IFACE 0x8001
#define DSI_PS_CDMA_AN_IFACE 0x8002
#define DSI_PS_UMTS_IFACE 0x8004
#define DSI_PS_EPC_IFACE 0x8880 /* LTE, HRPD, TDSCDMA */
#define DSI_PS_FMC_IFACE 0x8840

/* ================= MODEMS  ============================*/
/* used as index in table. Always start with 0 and update MAX */
#define DSI_RADIO_4GMODEM 0
#define DSI_RADIO_3GMODEM 1
#define DSI_MAX_MODEMS 2

#define DSI_DEFAULT_MODEM 0
#define DSI_INVALID_MODEM -1
typedef struct dsi_modem_s
{
  unsigned int technology; /* identifies tech(s) supported by this modem */
  int default_dsi_iface; /* default QMI port for this modem */
  /* dsi_iface_start..dsi_iface_end identify the interfaces in
     dsi_iface_tbl that are attached to this modem */
  int dsi_iface_start; /* start of index in dsi_iface_tbl */
  int dsi_iface_end; /* end of index in dsi_iface_tbl */
  int qmi_inst_offset; /* QMI instance offset for route lookup */
} dsi_modem_t;

extern dsi_modem_t dsi_modem_tbl[DSI_MAX_MODEMS];

#define DSI_MODEM_GET_DEFAULT_IFACE(m) \
  (dsi_modem_tbl[m].default_dsi_iface)

#define DSI_MODEM_SET_DEFAULT_IFACE(m, i) \
  (dsi_modem_tbl[m].default_dsi_iface = i)

#define DSI_MODEM_GET_TECH(m) \
  (dsi_modem_tbl[m].technology)

#define DSI_MODEM_SET_TECH(m, t) \
  (dsi_modem_tbl[m].technology |= t)

#define DSI_MODEM_GET_IFACE_START(m) \
  (dsi_modem_tbl[m].dsi_iface_start)

#define DSI_MODEM_SET_IFACE_START(m, i) \
  (dsi_modem_tbl[m].dsi_iface_start = i)

#define DSI_MODEM_GET_IFACE_END(m) \
  (dsi_modem_tbl[m].dsi_iface_end)

#define DSI_MODEM_SET_IFACE_END(m, i) \
  (dsi_modem_tbl[m].dsi_iface_end = i)

#define DSI_MODEM_GET_QMI_OFFSET(m) \
  (dsi_modem_tbl[m].qmi_inst_offset)

#define DSI_MODEM_SET_QMI_OFFSET(m, o) \
  (dsi_modem_tbl[m].qmi_inst_offset = o)

/* ================= DSI INTERFACES ============================*/

#define DSI_MAX_IFACES 16

/* The iface start and end values are dependent on DSI_MAX_IFACES */
#define DSI_LOCAL_MODEM_IFACE_START  (0)
#define DSI_LOCAL_MODEM_IFACE_END    (7)
#define DSI_REMOTE_MODEM_IFACE_START (8)
#define DSI_REMOTE_MODEM_IFACE_END   (15)

#define DSI_NUM_ENTRIES(arr) (sizeof(arr)/sizeof(arr[0]))

/* I could've used boolean TRUE/FALSE, but
   having ENUM is more descriptive than a boolean */
typedef enum dsi_iface_state_enum_e
{
  DSI_IFACE_STATE_NOT_IN_USE = 0,
  DSI_IFACE_STATE_IN_USE
} dsi_iface_state_t;

/* dsi interface type */
/* there is one interface per qmi port */
typedef struct dsi_iface_s
{
  /* used to arbitrate the use of this interface among
     various clients
     lifetime: starting from when start_nw_if() is issued
     to the point where the interface is disconnected
  */
  dsi_iface_state_t state;
  int qos_hndl;       /* QMI QOS service handle */
  int wds_hndl;       /* QMI WDS service handle */
  int wds_txn_hndl;   /* pending QMI txn */
  int qdi_hndl;       /* QDI handle */
  char * wds_str;     /* QMI port identifying string */
  char * dev_str;     /* Device name string */
  /* incremented only when start_nw_if succeeds for a client
     decremetned only when client wanted to stop call this interface
     lifetime: starting from when call is CONNECTED to
     the point where client issues stop_data_call
  */
  unsigned short ref_count; /* ref count for a wds client */
  pthread_mutex_t mutex; /* protect this interface */
  int modem; /* each iface belongs to a unique modem */
  int ipv6_profile_index; /* temporary: save ipv6 profile
                          * so can be delelated when iface
                          * is brought down */
  dsi_addr_info_t addr_info_s[DSI_NUM_IP_FAMILIES]; /* address info structure */
} dsi_iface_t;

#define DSI_DEFAULT_IFACE 0
#define DSI_INVALID_IFACE -1
#define DSI_INVALID_WDS_TXN -1
#define DSI_INVALID_WDS_HNDL -1
#define DSI_INVALID_QOS_HNDL -1
#define DSI_INVALID_PROFILE_INDEX -1

extern dsi_iface_t dsi_iface_tbl[DSI_MAX_IFACES];

#define DSI_QMI_PORT_MAX_LEN 12
#define DSI_DEV_STR_MAX_LEN  16

#define DSI_QMI_PORT_RMNET_INVALID ""

/* QMI Control port names */
/* smd ports */
#define DSI_QMI_PORT_RMNET_0 QMI_PORT_RMNET_0
#define DSI_QMI_PORT_RMNET_1 QMI_PORT_RMNET_1
#define DSI_QMI_PORT_RMNET_2 QMI_PORT_RMNET_2
#define DSI_QMI_PORT_RMNET_3 QMI_PORT_RMNET_3
#define DSI_QMI_PORT_RMNET_4 QMI_PORT_RMNET_4
#define DSI_QMI_PORT_RMNET_5 QMI_PORT_RMNET_5
#define DSI_QMI_PORT_RMNET_6 QMI_PORT_RMNET_6
#define DSI_QMI_PORT_RMNET_7 QMI_PORT_RMNET_7
/* sdio ports */
#define DSI_QMI_PORT_RMNET_SDIO_0 QMI_PORT_RMNET_SDIO_0
#define DSI_QMI_PORT_RMNET_SDIO_1 QMI_PORT_RMNET_SDIO_1
#define DSI_QMI_PORT_RMNET_SDIO_2 QMI_PORT_RMNET_SDIO_2
#define DSI_QMI_PORT_RMNET_SDIO_3 QMI_PORT_RMNET_SDIO_3
#define DSI_QMI_PORT_RMNET_SDIO_4 QMI_PORT_RMNET_SDIO_4
#define DSI_QMI_PORT_RMNET_SDIO_5 QMI_PORT_RMNET_SDIO_5
#define DSI_QMI_PORT_RMNET_SDIO_6 QMI_PORT_RMNET_SDIO_6
#define DSI_QMI_PORT_RMNET_SDIO_7 QMI_PORT_RMNET_SDIO_7
/* usb ports */
#define DSI_QMI_PORT_RMNET_USB_0 QMI_PORT_RMNET_USB_0
#define DSI_QMI_PORT_RMNET_USB_1 QMI_PORT_RMNET_USB_1
#define DSI_QMI_PORT_RMNET_USB_2 QMI_PORT_RMNET_USB_2
#define DSI_QMI_PORT_RMNET_USB_3 QMI_PORT_RMNET_USB_3
#define DSI_QMI_PORT_RMNET_USB_4 QMI_PORT_RMNET_USB_4
#define DSI_QMI_PORT_RMNET_USB_5 QMI_PORT_RMNET_USB_5
#define DSI_QMI_PORT_RMNET_USB_6 QMI_PORT_RMNET_USB_6
#define DSI_QMI_PORT_RMNET_USB_7 QMI_PORT_RMNET_USB_7
/*smux ports*/
#define DSI_QMI_PORT_RMNET_SMUX_0 QMI_PORT_RMNET_SMUX_0
/* usb ports (2nd modem) */
#define DSI_QMI_PORT_RMNET2_USB_0 QMI_PORT_RMNET2_USB_0
#define DSI_QMI_PORT_RMNET2_USB_1 QMI_PORT_RMNET2_USB_1
#define DSI_QMI_PORT_RMNET2_USB_2 QMI_PORT_RMNET2_USB_2
#define DSI_QMI_PORT_RMNET2_USB_3 QMI_PORT_RMNET2_USB_3
#define DSI_QMI_PORT_RMNET2_USB_4 QMI_PORT_RMNET2_USB_4
#define DSI_QMI_PORT_RMNET2_USB_5 QMI_PORT_RMNET2_USB_5
#define DSI_QMI_PORT_RMNET2_USB_6 QMI_PORT_RMNET2_USB_6
#define DSI_QMI_PORT_RMNET2_USB_7 QMI_PORT_RMNET2_USB_7

/* Device name prefixes */
/* smd prefix */
#define DSI_DEV_RMNET_SMD_PREFIX  "rmnet"
/* sdio prefix */
#define DSI_DEV_RMNET_SDIO_PREFIX "rmnet_sdio"
/* usb prefix */
#define DSI_DEV_RMNET_USB_PREFIX  "rmnet_usb"
/*smux prefix*/
#define DSI_DEV_RMNET_SMUX_PREFIX "rmnet_smux"
/* usb prefix (2nd modem) */
#define DSI_DEV_RMNET2_USB_PREFIX "rmnet2_usb"
/*rmnet data prefix*/
#define DSI_DEV_RMNET_DATA_PREFIX "rmnet_data"
/* invalid prefix */
#define DSI_DEV_RMNET_INVALID_PREFIX  (NULL)

extern char * dsi_qmi_port_names[DSI_MAX_IFACES];

extern char * dsi_device_names[DSI_MAX_IFACES];

#define DSI_GET_DEFAULT_WDS_HNDL \
  ( dsi_iface_tbl[DSI_DEFAULT_IFACE].wds_hndl )

#define DSI_SET_WDS_HNDL(i, hndl) \
  ( dsi_iface_tbl[i].wds_hndl = hndl )

#define DSI_GET_QDI_HNDL(i) \
  ( dsi_iface_tbl[i].qdi_hndl )

#define DSI_SET_QDI_HNDL(i, hndl) \
  ( dsi_iface_tbl[i].qdi_hndl = hndl )

#define DSI_GET_WDS_HNDL(i) \
  ( dsi_iface_tbl[i].wds_hndl )

#define DSI_IS_WDS_HNDL_VALID(i) \
  ((DSI_GET_WDS_HNDL(i) != DSI_INVALID_WDS_HNDL) && \
   (DSI_GET_WDS_HNDL(i) > 0))

#define DSI_SET_WDS_STR(i,str,ret) \
  do \
  { \
    if (NULL != str && \
        strlen(str) > DSI_QMI_PORT_MAX_LEN) \
    { \
      DSI_LOG_ERROR( "string %s length too long for qmi conn", \
                      str ); \
      ret = DSI_ERROR; \
    } \
    else \
    { \
      dsi_iface_tbl[i].wds_str = str; \
      DSI_LOG_DEBUG("dsi_iface_tbl[%d].wds_str is set to [%s]", \
                    i, str); \
      ret = DSI_SUCCESS; \
    } \
  } while (0)

#define DSI_SET_DEV_STR(i,str,ret) \
  do \
  { \
    if (NULL != str && \
        strlen(str) > DSI_DEV_STR_MAX_LEN) \
    { \
      DSI_LOG_ERROR( "string %s length too long for device", \
                      str ); \
      ret = DSI_ERROR; \
    } \
    else \
    { \
      dsi_iface_tbl[i].dev_str = (NULL != str)? str : "unknown"; \
      DSI_LOG_DEBUG("dsi_iface_tbl[%d].dev_str is set to [%s]", \
                    i, dsi_iface_tbl[i].dev_str); \
      ret = DSI_SUCCESS; \
    } \
  } while (0)

#define DSI_GET_WDS_STR(i) \
  ( dsi_iface_tbl[i].wds_str )

#define DSI_GET_DEV_STR(i) \
  ( dsi_iface_tbl[i].dev_str )

#define DSI_SET_WDS_TXN(i, txn) \
  ( dsi_iface_tbl[i].wds_txn_hndl = txn )

#define DSI_GET_WDS_TXN(i) \
  ( dsi_iface_tbl[i].wds_txn_hndl )

#define DSI_SET_QOS_HNDL(i, hndl) \
  ( dsi_iface_tbl[i].qos_hndl = hndl )

#define DSI_GET_QOS_HNDL(i) \
  ( dsi_iface_tbl[i].qos_hndl )

#define DSI_IS_QOS_HNDL_VALID(i) \
  ((DSI_GET_QOS_HNDL(i) != DSI_INVALID_QOS_HNDL) && \
   (DSI_GET_QOS_HNDL(i) > 0))

#define DSI_GET_STATE(i) \
  ( dsi_iface_tbl[i].state )

#define DSI_SET_STATE(i,s) \
  ( dsi_iface_tbl[i].state = s )

#define DSI_RESERVED(i) \
  ( dsi_iface_tbl[i].state != \
    DSI_IFACE_STATE_NOT_IN_USE )

#define DSI_INCR_COUNT(i) \
  do { \
    dsi_iface_tbl[i].ref_count++; \
    DSI_LOG_DEBUG("iface [%d] ref count incr [%d]", i, dsi_iface_tbl[i].ref_count); \
  } while (0)

#define DSI_DECR_COUNT(i,client) \
  do \
  { \
    if (client->priv.dsi_iface_id != i) \
    { \
      DSI_LOG_ERROR("client [%x] attempted to decr ref count " \
                    "on dsi iface [%d] that it does not refer to", \
                    (unsigned int)client, i); \
    } \
    else \
    { \
      if(dsi_iface_tbl[i].ref_count <= 0) \
      { \
        DSI_LOG_ERROR("client [%x] attempted to decr ref count " \
                      "that is not positive [%d]", \
                      (unsigned int)client, dsi_iface_tbl[i].ref_count); \
      } \
      else \
      { \
        dsi_iface_tbl[i].ref_count--; \
        DSI_LOG_DEBUG("iface [%d] ref count decr [%d]", i, dsi_iface_tbl[i].ref_count); \
      } \
    } \
  } while (0)

#define DSI_SET_COUNT(i,rc) \
  ( dsi_iface_tbl[i].ref_count = rc )

#define DSI_GET_COUNT(i) \
  ( dsi_iface_tbl[i].ref_count )

#define DSI_GET_MUTEX(i) \
  ( &(dsi_iface_tbl[i].mutex) )

#define DSI_GET_MODEM(i) \
  ( dsi_iface_tbl[i].modem )

#define DSI_SET_MODEM(i,m) \
  ( dsi_iface_tbl[i].modem = m )

#define DSI_GET_IPV6_PROFILE(i) \
  ( dsi_iface_tbl[i].ipv6_profile_index )

#define DSI_SET_IPV6_PROFILE(i, p) \
  ( dsi_iface_tbl[i].ipv6_profile_index = p )

#define DSI_IS_ADDR_VALID(i,ipf,address) \
  ( TRUE == dsi_iface_tbl[i].addr_info_s[ipf].address##_s.valid_addr )

#define DSI_SET_INVALID_ADDR(i,ipf,address) \
  ( dsi_iface_tbl[i].addr_info_s[ipf].address##_s.valid_addr = FALSE )

#define DSI_SET_VALID_ADDR(i,ipf,address) \
  ( dsi_iface_tbl[i].addr_info_s[ipf].address##_s.valid_addr = TRUE )

#define DSI_GET_ADDR(i,ipf,address) \
  ( dsi_iface_tbl[i].addr_info_s[ipf].address##_s.addr )

#define DSI_GET_ADDRINFO(i,ipf) \
  ( dsi_iface_tbl[i].addr_info_s[ipf] )

typedef struct dsi_qmi_to_dsi_tech_s
{
  unsigned int qmi_tech;
  unsigned int dsi_tech;
} dsi_qmi_to_dsi_tech_t;

extern dsi_qmi_to_dsi_tech_t
dsi_qmi_tech_mapping_tbl[DSI_RADIO_TECH_MAX];

/* ================= DSI CALLS ============================*/
#define DSI_MAX_NUM_OF_TMGI_LIST 64 /* max number of TMGI_List
                                       same value as NUMBER_MAX_V01 defined
                                       qmi_embms_v01.h */
#define DSI_SIZE_OF_TMGI          6 /* TMGI is 6 byte array
                                       same value as NUMBER_MAX_V01 defined
                                       qmi_embms_v01.h */
#define DSI_SIZE_OF_EARFCNLIST    32/* max number of earfcnlist size
                                       same value as EARFCNLIST_MAX_V01 defined
                                       in qmi_embms_v01.h */

#define DSI_EMBMS_SAI_PER_FREQ_MAX_SIZE  QMI_WDS_EMBMS_SAI_PER_FREQ_MAX_SIZE

typedef enum
{
  DSI_STATE_EVT_NET_IS_CONN_UNKNOWN,
  DSI_STATE_EVT_NET_IS_CONN_PENDING,
  DSI_STATE_EVT_NET_IS_CONN_SENT
} dsi_state_evt_net_is_conn_t;

typedef enum
{
  DSI_STATE_CALL_IDLE,
  DSI_STATE_CALL_CONNECTING,
  DSI_STATE_CALL_CONNECTED,
  DSI_STATE_CALL_DISCONNECTING,
  DSI_STATE_CALL_MAX
} dsi_state_call_state_t;

/* status for activate/deactivate indication has only 1 TMGI */
#define DSI_EMBMS_TMGI_STATUS_IND_TMGI_LIST_LEN QMI_WDS_EMBMS_TMGI_STATUS_IND_LIST_SIZE
typedef struct
{
  /* Private data for DSI module */
  int dsi_iface_id; /* index into dsi_iface_tbl */

  qmi_wds_start_nw_if_params_type start_nw_params;

  /* call end reason structure */
  dsi_ce_reason_t ce_reason[DSI_NUM_IP_FAMILIES];

  /* wds async rsp cb may change state of this
     structure. protect it with mutex when needed */
  pthread_mutex_t mutex;
  int rl_qmi_inst;  /* Modem route lookup result */

  /* Modem call state */
  dsi_state_call_state_t call_state;

  /* IP version of the call */
  int ip_version;

  /*Technology of the call*/
  qmi_wds_iface_name_type call_tech;

  dsi_embms_tmgi_info_type   embms_tmgi_list_info;

  /* Flag indicating whether partial retry is being attempted */
  boolean  partial_retry;

  /* QDI call handle */
  qdi_call_handle_t  qdi_call_hndl;

  /* Call connect ref count associated with the handle */
  int ref_count;

  /* The type of app */
  int app_type;

} dsi_priv_t;

#define DSI_INVALID_QMI_INST (-1)

typedef struct dsi_store_s
{
  dsi_net_ev_cb net_ev_cb; /* caller/owner callback function */
  void * user_data; /* caller/owner cookie */
  dsi_evt_payload_t *ev_payload;  /* Cached event payload */
  dsi_priv_t priv; /* private data */
  int dsi_tech; /* if user provided specific technology
                * other than CDMA/UMTS, use this field
                * to store it */
  void * self; /* used to validate dsi_stor_t ptr */
} dsi_store_t;

/*!
  @brief:
  dsi_store_ptr: Pointer to the dsi store.

  This ptr is given to the higher layers (i.e. dsi_netctrl)
  as a call handle. Each valid dsi_stor_tbl_t entry should
  contain a valid dsi_iface_id in it's priv structure.
*/
typedef struct
{
  unsigned char is_valid;
  void * dsi_store_ptr;
}dsi_store_tbl_t;

extern dsi_store_tbl_t dsi_store_table[DSI_MAX_DATA_CALLS];

/* constant represents that we don't know the call end reason  */
#define DSI_CE_REASON_UNKNOWN QMI_WDS_VERBOSE_CE_INVALID

/*===========================================================================
                    VALIDATION MACROS
===========================================================================*/
#define DSI_IS_HNDL_VALID( hndl ) \
  ( (dsi_inited == DSI_TRUE) && ( hndl != NULL) && ( hndl->self == hndl ) )
#define DSI_IS_IDENT_VALID( ident ) \
  ( (DSI_CALL_INFO_MIN < ident) && (ident < DSI_CALL_INFO_MAX) )
/* iface id (index into dsi_iface_tbl starts from 0
   so it can be anything from 0 to DSI_MAX_IFACES */
#define DSI_IS_ID_VALID(id) \
  ( (dsi_inited == DSI_TRUE) && (id < DSI_MAX_IFACES) && (id >= 0) )
#define DSI_IS_IP_FAMILY_VALID(ipf) \
  (((int)(ipf) >= (int)DSI_IP_FAMILY_V4) && ((int)(ipf) < (int)DSI_NUM_IP_FAMILIES))

/*===========================================================================
                      INTERNAL CONFIGURATION VTABLES
===========================================================================*/
/* client can provide their own function
   pointers to adapt to their needs */
typedef int (*dsi_mni_init_fptr)();
typedef int (*dsi_mni_start_fptr)(int i, dsi_store_t * st);
typedef int (*dsi_mni_stop_ftpr)(int i, dsi_store_t * st);
typedef int (*dsi_mni_abort_start_ftpr)(int i, dsi_store_t * st);
typedef int (*dsi_mni_look_up_ftpr)(dsi_store_t * st, int * pi);
typedef int (*dsi_mni_reg_unreg_handoff_ind_fptr)(int i, dsi_store_t * st, boolean reg_unreg);
typedef int (*dsi_mni_register_embms_ind_ftpr)(int i, dsi_store_t * st);
typedef int (*dsi_mni_embms_tmgi_activate_ftpr)(int i, dsi_store_t *st);
typedef int (*dsi_mni_embms_tmgi_deactivate_ftpr)(int i, dsi_store_t *st);
typedef int (*dsi_mni_embms_tmgi_act_deactivate_ftpr)(int i, dsi_store_t *st);
typedef int (*dsi_mni_embms_tmgi_list_query_ftpr)(int i, dsi_store_t *st);

typedef struct dsi_mni_vtable_s
{
  dsi_mni_init_fptr mni_init_f;
  dsi_mni_start_fptr mni_start_f;
  dsi_mni_stop_ftpr mni_stop_f;
  dsi_mni_abort_start_ftpr mni_abort_start_f;
  dsi_mni_look_up_ftpr mni_look_up_f;
  dsi_mni_reg_unreg_handoff_ind_fptr mni_reg_unreg_handoff_ind_f;
  dsi_mni_register_embms_ind_ftpr mni_register_embms_ind_f;
  dsi_mni_embms_tmgi_activate_ftpr mni_embms_tmgi_activate_f;
  dsi_mni_embms_tmgi_deactivate_ftpr mni_embms_tmgi_deactivate_f;
  dsi_mni_embms_tmgi_act_deactivate_ftpr mni_embms_tmgi_act_deactivate_f;
  dsi_mni_embms_tmgi_list_query_ftpr mni_embms_tmgi_list_query_f;
} dsi_mni_vtable_t;

extern dsi_mni_vtable_t dsi_mni_vtbl;

extern boolean dsi_inited;
typedef struct dsi_init_cb_info_s
{
  void (*cb_func)( void * );
  void *cb_data;
} dsi_init_cb_info_t;

extern dsi_init_cb_info_t dsi_init_cb_info;

extern boolean netmgr_ready_queried;

/* for testing/demo - set this iface id where we can issue nas query */
#define DSI_NAS_IFACE_ID 0

extern int nas_clnt_id;

extern pthread_mutex_t dsi_global_mutex;

#define DSI_GLOBAL_LOCK \
  DSI_LOCK_MUTEX(&dsi_global_mutex)

#define DSI_GLOBAL_UNLOCK \
  DSI_UNLOCK_MUTEX(&dsi_global_mutex)

extern netmgr_client_hdl_t netmgr_hndl;

extern int dsi_qos_enable;

/*===========================================================================
                    FUNCTIONS EXPORTED INTERNALLY
Note: to avoid duplicate description for the functions, you'll
only find the description commented in the C file.
===========================================================================*/
extern void dsi_cleanup_store_tbl(short int store_index);

extern void dsi_notify_users(int dsi_iface_index, int event);

extern int dsi_fill_addr(const struct sockaddr_storage * src_addr_ptr,
                         struct sockaddr_storage       * dst_addr_ptr);

extern void dsi_fill_addr_info(int dsi_id, dsi_ip_family_t ipf, const netmgr_nl_event_info_t * info_ptr);

extern void dsi_fill_qos_info(int dsi_id, const netmgr_nl_event_info_t * info_ptr);

extern void dsi_release_dsi_iface(int dsi_iface_index, dsi_ce_reason_t * ce_reason);

extern int dsi_find_any_valid_iface_on_modem(int modem_id);

extern int dsi_find_available_iface_on_modem(int modem_id, int app_type);

extern int dsi_find_dsi_id_by_tech(int tech, boolean unused);

extern int dsi_find_dsi_id_by_qmi(unsigned char qmi_inst,
                                  int dsi_iface_index);

extern unsigned int dsi_map_qmi_to_dsi_tech(unsigned int qmi_tech);

extern unsigned int dsi_map_dsi_to_qmi_tech(unsigned int qmi_tech);

extern dsi_ce_reason_type_t
dsi_map_qmi_to_dsi_ce_reason_type
(qmi_wds_verbose_ce_reason_type qmi_ce_reason_type);

extern dsi_embms_tmgi_deactivate_reason_type_t
dsi_map_qmi_to_dsi_tmgi_deactivated_reason_type(int reason);

extern DSI_INLINE void dsi_attach_dsi_iface(int dsi_iface_index,
                                            dsi_store_t * st);

extern DSI_INLINE void dsi_detach_dsi_iface(dsi_store_t * st);

extern int dsi_prepare_route_lookup_params
(
  dsi_store_t * dsi_hndl,
  qmi_wds_route_look_up_params_type * pparams
);

extern void dsi_delete_ipv6_profile(int ipv6_iface);

extern int dsi_init_internal(dsi_mni_vtable_t * mni_vtbl);

extern int dsi_is_qmiport_incall( int iface, int *inuse_ptr );

extern void dsi_modem_out_of_service_hdlr(int conn_id);

extern void dsi_modem_in_service_hdlr(int conn_id);
extern void dsi_update_store_table
(
  int iface,
  qmi_wds_verbose_ce_reason_type type,
  int verbose_code,
  dsi_ip_family_t ipf
);

dsi_data_bearer_tech_t
dsi_translate_qmi_to_dsi_bearer_tech
(
  qmi_wds_data_bearer_tech_type  *qmi_bearer_tech
);

dsi_data_bearer_tech_t
dsi_translate_qmi_to_dsi_bearer_tech_ex
(
  qmi_wds_data_bearer_tech_type_ex  *qmi_bearer_tech_ex
);

extern void dsi_update_call_tech
(
  int iface,
  qmi_wds_iface_name_type  call_tech
);

extern int dsi_netctrl_copy_tmgi_list
(
  qmi_wds_embms_tmgi_type **tmgi_list_ptr_to,
  qmi_wds_embms_tmgi_type *tmgi_list_ptr_from,
  unsigned char            len
);

extern int dsi_set_ril_instance(int instance);

extern int dsi_get_ril_instance(void);


#endif
