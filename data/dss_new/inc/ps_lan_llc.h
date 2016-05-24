#ifndef PS_LAN_LLC_H
#define PS_LAN_LLC_H
/*===========================================================================
                               P S _ LAN _ LLC . H

DESCRIPTION
  Header file for the PS 802 LLC protocol suite Interfacing functions.

Copyright (c) 2004-2010 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE
               
Derived from Jeff Dyck's original Ethernet implementation

  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_lan_llc.h#1 $ 
  $DateTime: 2011/01/10 09:44:56 $ 
  $Author: maldinge $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
10/07/10    dm     Adding new APIs for SoftAP throughput optimizations
06/17/10    vs     Extra LLC instances for simultaneous IPV6/IPV4 over QMI
08/20/09    pp     WAPI update.
01/31/09    pp     CMI: arp_flush API moved from ps_arp.h
12/19/08    pp     Common Modem Interface: Public/Private API split.
09/12/08    pp     Metainfo optimizations.
===========================================================================*/


/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#include "comdef.h"
#include "dsm.h"
#include "ps_iface_defs.h"
#include "ps_meta_info.h"


/*===========================================================================

                             MACROS & DATA DECLARATIONS

===========================================================================*/

#define LAN_IEEE_MAC_ADDR_LEN        6

/*---------------------------------------------------------------------------
  Special IEEE MAC address value
---------------------------------------------------------------------------*/
extern const byte LAN_IEEE_MAC_BROADCAST_ADDR [LAN_IEEE_MAC_ADDR_LEN] ;

/*---------------------------------------------------------------------------
  Ethertype defintions
---------------------------------------------------------------------------*/
#define LAN_ETHERTYPE_IPV4   0x0800
#define LAN_ETHERTYPE_IPV6   0x86DD
#define LAN_ETHERTYPE_ARP    0x0806
#define LAN_ETHERTYPE_802_1X          0x888E
#define LAN_ETHERTYPE_802_1X_PREAUTH  0x88C7
#define LAN_ETHERTYPE_802_1Q_TAG      0x8100
#define LAN_ETHERTYPE_WAPI            0x88B4


/*---------------------------------------------------------------------------
    Supported Logical Link Entities (LLEs):
        For now there is a one-to-one mapping between an LLE, a PS_IFACE and
        a supported MAC interface.
 --------------------------------------------------------------------------*/
typedef enum
{
  LAN_LLE_MIN     = -1,
  LAN_LLE_RMNET1  =  0,  /* OSIF WWAN RM MAC                                */
  LAN_LLE_WLAN    =  1,  /* 802.11 MAC                                      */
  LAN_LLE_RMNET2  =  2,
  LAN_LLE_RMNET3  =  3,
  LAN_LLE_RMNET4  =  4,
  LAN_LLE_RMNET5  =  5,
  LAN_LLE_UICC    =  6,
  LAN_LLE_RMNET1_2  =  7,
  LAN_LLE_RMNET2_2  =  8,
  LAN_LLE_RMNET3_2  =  9,
  LAN_LLE_RMNET4_2  =  10,
  LAN_LLE_RMNET5_2  =  11,
  LAN_LLE_MAX
} lan_lle_enum_type;


/*---------------------------------------------------------------------------
    LLC Operation mode enumeration:
        Mode 0: Transparent LLC Mode; no LLC/SNAP header is expected within
                LLC PDU which is concatenation of Ethertype and IP packet.
                LLC PDU frame format:
                    |-----------|-------------------|
                    | Ethertype |   IP Packet Body  |
                    |-----------|-------------------|

        Mode 1: unacknowledged and cnnectionless LLC mode of operation where
                LLC PDU are encapsulated in SNAP header.
                LLC PDU frame format:
                    |---------------|-----------|------------------|
                    | SNAP header   |  Ethertype |  IP Packet Body  |
                    |---------------|-----------|------------------|
---------------------------------------------------------------------------*/
typedef enum
{
  LAN_LLC_MODE_MIN = -1,
  LAN_LLC_MODE_0   =  0,
  LAN_LLC_MODE_1   =  1,
  LAN_LLC_MODE_MAX
} lan_llc_mode_enum_type;


/*---------------------------------------------------------------------------
    RX signal associated with an LLE
---------------------------------------------------------------------------*/
typedef int32 lan_llc_sig;


/*---------------------------------------------------------------------------
    Prototye of RX function that LLC uses to retrieve data arrived at the MAC
    layer.
    Within returned dsm item, the source and destination hardware addresses
    should be pushed ahead of the LLC PDU, the destination address first.
 --------------------------------------------------------------------------*/
typedef dsm_item_type* (*lan_llc_rx_f_ptr_type)(void*, ps_meta_info_type_ex **);


/*---------------------------------------------------------------------------
    Prototye of TX function that LLC uses to request LLC PDU transmission at
    the MAC layer.
    Within the passed dsm item, the source and destination hardware addresses
    are pushed ahead of the LLC PDU, the destination address first.
 --------------------------------------------------------------------------*/
typedef void (*lan_llc_tx_f_ptr_type)(dsm_item_type**, void*);


/*---------------------------------------------------------------------------
    Prototye of function that LLC uses to retrieve qos-related meta_info
    from the PS meta_info and ethertype of the packet
    user_data contains data belonging to the entity registering the callback
 --------------------------------------------------------------------------*/
typedef void* (*lan_llc_get_qos_mi_f_ptr_type)(ps_tx_meta_info_type* ps_mi_ptr,
                                               uint16             ethertype,
                                               void*              user_data);


/*---------------------------------------------------------------------------
    Forward ARP configuration data structure declaration
 --------------------------------------------------------------------------*/
typedef boolean (*arp_query_proxy_f_ptr_type)(lan_lle_enum_type instance,
                                              uint32 src_ip,
                                              uint32 target_ip);

/*---------------------------------------------------------------------------
   Prototype of callback that may registered a client of an LLE instance for
   notification of packets arrived over this LLE instance with a particular
   ethertype
---------------------------------------------------------------------------*/
typedef void (*lan_llc_input_f_ptr_type)(lan_lle_enum_type lle_instance,
                                         dsm_item_type**   pkt,
                                         byte*             src_hw_addr,
                                         byte*             dst_hw_addr);

typedef struct
{
  boolean                      enable_cache_maintenance;
  boolean                      enable_proxy;
  arp_query_proxy_f_ptr_type   arp_proxy_query_f_ptr;
} arp_config_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_ICMP6_ND_CBACK_F_PTR_TYPE

DESCRIPTION
  Type defintion of ICMP6 ND callback prototype ND to notify of any special
  events.
---------------------------------------------------------------------------*/
typedef void (*ps_icmp6_nd_cback_f_ptr_type)
(
  uint8                       event,
  void*                       user_data_ptr
);

/*---------------------------------------------------------------------------
TYPEDEF ICMP6_ND_CONFIG_TYPE

DESCRIPTION
  This is the type of ICMP6 Neighbor Discovery start config items.
---------------------------------------------------------------------------*/
typedef struct
{
  ps_icmp6_nd_cback_f_ptr_type  nd_cback_f_ptr;
  void                         *usr_data_ptr;
} ps_icmp6_nd_config_type;

/*---------------------------------------------------------------------------
    LLE Start Info data structure:
        This is passed to LLC by a Mode Controller willing to start a
        supported LLE instance. It provides necessary information for LLC to
        fully operate on this LLE instance.
 --------------------------------------------------------------------------*/
typedef struct
{
  ps_iface_type*           iface_ptr;
  lan_llc_mode_enum_type   lle_mode;
  lan_llc_sig              lle_rx_sig;
  lan_llc_rx_f_ptr_type    rx_f_ptr;
  void*                    rx_f_user_data;
  lan_llc_tx_f_ptr_type    tx_f_ptr;
  void*                    tx_f_user_data;
  lan_llc_get_qos_mi_f_ptr_type   get_qos_mi_f_ptr;
  void*                           get_qos_mi_f_user_data;
  arp_config_type          arp_config;
  ps_icmp6_nd_config_type  nd_config;
} lan_llc_start_info_type;

/*---------------------------------------------------------------------------
    LLE Configuration buffer:
        This datat structure corresponds to an LLC instance config info.
 --------------------------------------------------------------------------*/
typedef struct
{
  lan_lle_enum_type        lle_instance;
  lan_llc_start_info_type  start_info;
} lan_lle_config_type;




/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION LAN_LLC_START()

DESCRIPTION
  This function posts a START command to the PS task for a particular LLE
  instance. It is called by the specific Mode Controller.

PARAMETERS
  lle_instance: the LLE instance that should be started
  config_ptr: pointer to configuration information for LLE to start.This
  memory belongs to the caller and will not be freed by PS LAN LLC

RETURN VALUE
  -1 in case of error
   0 in the command has been posted successfully.

DEPENDENCIES
  lan_llc_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
int lan_llc_start
(
  lan_lle_enum_type        lle_instance,
  lan_llc_start_info_type* config_ptr
);

/*===========================================================================
FUNCTION LAN_LLC_RECONFIG()

DESCRIPTION
  Change the configuration of a lan_llc instance after it was started.

PARAMETERS
  lle_instance:  Which LLC instance to reconfigure
  config_ptr:    The new configuration for this LLE instance

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int lan_llc_reconfig
(
  lan_lle_enum_type        lle_instance,
  lan_llc_start_info_type* config_ptr
);

/*===========================================================================
FUNCTION LAN_LLC_STOP()

DESCRIPTION
  This function posts a STOP command to the PS task for a particular LLE. It
  is called by the specific Mode controller (not running in PS task context).

PARAMETERS
  lle_instance: the LLE instance to stop.

RETURN VALUE
  -1 in case of error
   0 in case of success

DEPENDENCIES
  lan_llc_init should habe been called.

SIDE EFFECTS
  None
===========================================================================*/
int lan_llc_stop
(
  lan_lle_enum_type  lle_instance
);


/*===========================================================================
FUNCTION LAN_LLC_SET_INPUT_HANDLER()

DESCRIPTION
  This function registers with a particular LLE instance the handler for
  packets of a particular ethertype. Note that LLC support natively handler
  for IP and ARP packets and those handlers does not have to be registered
  explicitly.

PARAMETERS
  lle_instance: the LLE instance to stop.

RETURN VALUE
  -1 in case of error
   0 in case of success

DEPENDENCIES
  lan_llc_init should habe been called.

SIDE EFFECTS
  None
===========================================================================*/
int lan_llc_set_input_handler
(
  lan_lle_enum_type        lle_instance,
  uint16                   ethertype,
  lan_llc_input_f_ptr_type lle_input_f_ptr
);


/*===========================================================================
FUNCTION LAN_LLC_IS_STARTED()

DESCRIPTION
  This function queries whether a paticular LLC instance has been started.

PARAMETERS
  lle_instance: the LLE instance

RETURN VALUE
  TRUE:  The LLC has been started
  FALSE: The instance is stopped

DEPENDENCIES
  lan_llc_init should habe been called.

SIDE EFFECTS
  None
===========================================================================*/
boolean lan_llc_is_started
(
  lan_lle_enum_type        lle_instance
);

/*===========================================================================
FUNCTION LAN_LLC_SEND_PKT()

DESCRIPTION
  This function is called to send an IP Packet without address resolution in
  (case of broadcast IP packets) or by the ARP module to send either queued
  IP or ARP packets. As such, this function performs the necessary LLC/SNAP
  encapulation based on the LLE mode of operation. All pointers supplied as
  function arguments belong to caller and are not freed by this function.

PARAMETERS
  lle_instance:  The LLE instance transmitting this packet
  pkt_chain_ptr: pointer to chain of dms items that hold the payload of the
                 packet that is to be transmitted.
  ethertype:     Packet Ethertype.
  dst_hw_addr:   pointer to destination hardware address.

RETURN VALUE
  Abide by the prototype of a PS IFACE Tx CMD:
     0 on success
    -1 on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int lan_llc_send_pkt
(
  lan_lle_enum_type lle_instance,
  dsm_item_type**   pkt_chain_ptr,
  uint16            ethertype,
  byte*             dst_hw_addr
);

/*===========================================================================
FUNCTION ARP_FLUSH()

DESCRIPTION
  This function is called to flush all ARP entries realted to a particular
  LLE. The caller may be the ARP module itself or Mode Controller because it
  has determined the need to flush the ARP cache.

PARAMETERS
  lle_instance: the LLE instance to stop.

RETURN VALUE
  -1 in case of error
   0 if successful

DEPENDENCIES
  arp_init should habe been called.

SIDE EFFECTS
  None
===========================================================================*/
int arp_flush
(
  lan_lle_enum_type lle_instance
);

/*===========================================================================
FUNCTION LAN_LLC_SOFTAP_START()

DESCRIPTION
  This function synchronizes START command for Soft AP in protocol task for 
  a particular LLE instance. It is called by the specific Mode Controller.

PARAMETERS
  lle_instance: the LLE instance that should be started
  config_ptr: pointer to configuration information for LLE to start.This
  memory belongs to the caller and will not be freed by PS LAN LLC

RETURN VALUE
   -1 in case of error
   0 in the command has been posted successfully.

DEPENDENCIES
  lan_llc_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
int lan_llc_softap_start
(
  lan_lle_enum_type        lle_instance,
  lan_llc_start_info_type* config_ptr
);

/*===========================================================================
FUNCTION LAN_LLC_SET_SIGNAL()

DESCRIPTION
  This function is called to set the appropiate protocol task signal for
  the corresponding LLE instance.

PARAMETERS
  lle_instance: the LLE instance to stop.

RETURN VALUE
  -1 in case of error
   0 if successful

DEPENDENCIES
  arp_init should habe been called.

SIDE EFFECTS
  None
===========================================================================*/
int lan_llc_set_signal
(
  lan_lle_enum_type instance
);

#endif /* PS_LAN_LLC_H */
