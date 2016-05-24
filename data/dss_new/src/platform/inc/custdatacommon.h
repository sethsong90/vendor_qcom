#ifndef CUSTDATACOMMON_H
#define CUSTDATACOMMON_H
/*===========================================================================

DESCRIPTION
  Configuration for DATACOMMON SU

Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.  Notice that changes are listed in reverse chronological order.

$Header: //source/qcom/qct/modem/datacommon/build/cust/main/latest/custdatacommon.h#77 $ $DateTime: 2010/05/20 11:26:51 $ $Author: sukhyung $

when       who     what, where, why
--------   ---     ------------------------------------------------------------
03/09/10   pp      FEATURE_DATA_PS_SYSTEM_HEAP mainlined in 2H09.
02/13/10   pp      Mainlined FEATURE_DATA_PS_ARBITRATION_MGR.
01/26/10   ssh     Define FEATURE_DATA_RMNET_IPV6 for MDM9k
12/29/09   ssh     Define FEATURE_DATA_PS_EAP for 2h09.2 single proc mdm (9k)
11/05/09   mga     Merged eHRPD related features
08/14/09   ar      Updated for 2H09.1 package
06/04/09   mjb     Introduced 1H09 tiers
04/16/09   pp      PS_IN_ALIAS_H for OffTarget, to avoid header inclusion:
                   ps_in_alias.h
04/09/09   pp      Off-Target specific features/customizations.
02/19/09   am      Added DCC Task feature.
11/19/08   mjb     Merge updates from rearch and 08.02
09/22/08   ar      Update for MSM7600 integration
09/09/08   scb     Added PPP/HDLC/EAP related featurization.
08/20/08   mjb     Structured, tooks inputs from Srinivas
07/01/08   mjb     Branched from modem/data/.../custdata.h

===========================================================================*/

/*---------------------------------------------------------------------------
  Features common to all packages
---------------------------------------------------------------------------*/
#define FEATURE_DATA
#define FEATURE_DS
#define FEATURE_DATA_PS
#define FEATURE_DATA_PS_L4
//#define FEATURE_DS_SOCKETS
//#define FEATURE_DATA_PS_DNS
#define FEATURE_DATA_PS_IPV6
#define FEATURE_DATA_PS_QOS
#define FEATURE_DATA_PS_SOCK_REARCH
#define FEATURE_DATA_PS_CMI_REARCH
#define FEATURE_DATA_PS_META_INFO_V2
#define FEATURE_DATA_PS_ARBITRATION_MGR

#ifndef T_WINNT
  #define FEATURE_DATA_PS_IN_ALIASES
#endif


/*---------------------------------------------------------------------------
   MOB SECTION!
   - For OFFTARGET, there is no way we can define externally [like buids file!!]
     Hence, have to define it here!
   - Talk to MOB PoCs if you have any questions!
---------------------------------------------------------------------------*/
#ifdef FEATURE_DATACOMMON_1H09_SINGLE_PROC

  #define FEATURE_DATA_PS_DHCP
  #define FEATURE_DATA_PS_DHCPV6
  #define FEATURE_DATA_PS_ADDR_MGMT
  #define FEATURE_DATA_PS_LLC
  #define FEATURE_DATA_PS_MCAST
  #define FEATURE_DATA_PS_MCAST_V6
  #define FEATURE_DATA_PS_DCC
  #define FEATURE_DATACOMMON_PS_IFACE_IO

  #ifndef FEATURE_DATA_PS_IWLAN
   #define FEATURE_DATA_PS_IWLAN
  #endif

  #define FEATURE_DATA_PS_EAP

  #ifndef FEATURE_SEC_IPSEC
    #define FEATURE_SEC_IPSEC
    #define FEATURE_SEC_IPSEC_CHILD_SA
  #endif

  #ifndef MSG_BT_SSID_DFLT
    #define MSG_BT_SSID_DFLT MSG_SSID_DS
  #endif

  /* Treat OffTarget as Modem image */
  #define FEATURE_DATACOMMON_2H09_1_DUAL_PROC_MODEM
  #define FEATURE_DATA_PS_PING
#endif /* TEST_FRAMEWORK || T_WINNT */

/*---------------------------------------------------------------------------
  Package specific features
---------------------------------------------------------------------------*/

/* 2H09.1 Dual Processor Common Modem Image */
#ifdef FEATURE_DATACOMMON_2H09_1_DUAL_PROC_MODEM

  #define FEATURE_DATA_PS_DHCP
  #define FEATURE_DATA_PS_DHCPV6
  #define FEATURE_DATA_PS_ADDR_MGMT
  #define FEATURE_DATA_PS_LLC
  #define FEATURE_DATA_PS_MCAST
  #define FEATURE_DATA_PS_MCAST_V6
  #define FEATURE_DATA_PS_DCC
  #define FEATURE_DATACOMMON_PS_IFACE_IO

  #define FEATURE_DATA_PS_EAP
  #define FEATURE_DATA_PS_PPP
  #define FEATURE_DATA_PS_PPP_LOGGING
  #define FEATURE_DATA_PS_HDLC
  #define FEATURE_DATA_PS_IPHC
  #define FEATURE_DATA_PS_ROHC
  #define FEATURE_DATA_PS_DATA_LOGGING
  #define FEATURE_DATA_PS_DATA_LOGGING_PARTIAL_PPP
  #define FEATURE_DATA_PS_RSVP
  #define FEATURE_DATA_QMI
  #define FEATURE_DATA_QMI_EAP
  #define FEATURE_DATA_QMI_QOS
  #define FEATURE_DATA_QMI_MCAST
  #define FEATURE_DATA_RM_NET
  #define FEATURE_DATA_RM_NET_USES_SM
  #define FEATURE_DATA_RM_NET_USES_SM_LAPTOP_INST
  #define FEATURE_DATA_RM_NET_MULTI
  #define FEATURE_DATA_RMNET_IPV6
  #define FEATURE_HDLC_HW_ACCEL
  #define FEATURE_DATA_PS_SLIP
  #define FEATURE_DATACOMMON_PACKAGE_MODEM
  #define FEATURE_DATA_QMI_AT
  #ifdef FEATURE_MODEM_HEAP
    #define FEATURE_DATA_PS_SYSTEM_HEAP
  #endif /*FEATURE_MODEM_HEAP*/

  #ifdef FEATURE_CDMA
    #define FEATURE_DATA_PS_MIP
    #define FEATURE_DATA_PS_MIPV6
  #endif

/* 2H09.1 Dual Processor Common Modem Image with EHRPD*/
#elif defined FEATURE_DATACOMMON_2H09_1_DUAL_PROC_MODEM_EHRPD

  #define FEATURE_DATA_PS_DHCP
  #define FEATURE_DATA_PS_DHCPV6
  #define FEATURE_DATA_PS_ADDR_MGMT
  #define FEATURE_DATA_PS_LLC
  #define FEATURE_DATA_PS_MCAST
  #define FEATURE_DATA_PS_MCAST_V6
  #define FEATURE_DATA_PS_DCC
  #define FEATURE_DATACOMMON_PS_IFACE_IO

  #define FEATURE_DATA_PS_EAP
  #define FEATURE_DATA_PS_PPP
  #define FEATURE_DATA_PS_PPP_LOGGING
  #define FEATURE_DATA_PS_HDLC
  #define FEATURE_DATA_PS_IPHC
  #define FEATURE_DATA_PS_ROHC
  #define FEATURE_DATA_PS_DATA_LOGGING
  #define FEATURE_DATA_PS_DATA_LOGGING_PARTIAL_PPP
  #define FEATURE_DATA_PS_RSVP
  #define FEATURE_DATA_QMI
  #define FEATURE_DATA_QMI_EAP
  #define FEATURE_DATA_QMI_QOS
  #define FEATURE_DATA_QMI_MCAST
  #define FEATURE_DATA_RM_NET
  #define FEATURE_DATA_RM_NET_USES_SM
  #define FEATURE_DATA_RM_NET_USES_SM_LAPTOP_INST
  #define FEATURE_DATA_RM_NET_MULTI
  #define FEATURE_DATA_RMNET_IPV6
  #define FEATURE_HDLC_HW_ACCEL
  #define FEATURE_DATA_PS_SLIP
  #define FEATURE_DATACOMMON_PACKAGE_MODEM
  #define FEATURE_DATA_QMI_AT
  #ifdef FEATURE_MODEM_HEAP
    #define FEATURE_DATA_PS_SYSTEM_HEAP
  #endif /*FEATURE_MODEM_HEAP*/

  #ifdef FEATURE_CDMA
    #define FEATURE_DATA_PS_MIP
    #define FEATURE_DATA_PS_MIPV6
  #endif

  #define FEATURE_DATA_PS_EAP_AKA_SW_IMPL
  #define FEATURE_DATA_PS_PPP_EAP
  #define FEATURE_DATA_PS_EHRPD  
  #define FEATURE_DATA_PS_ENFORCE_AUTH 

  #ifdef FEATURE_MDM_MSM_FUSION
    #define FEATURE_DATA_FUSION_MSM
  #endif

/* 2H09.1 Dual Processor BMP Apps Image */
#elif defined FEATURE_DATACOMMON_2H09_1_DUAL_PROC_BMP_APPS

  #define FEATURE_DATA_PS_DHCP
  #define FEATURE_DATA_PS_DHCPV6
  #define FEATURE_DATA_PS_ADDR_MGMT
  #define FEATURE_DATA_PS_LLC
  #define FEATURE_DATA_PS_MCAST
  #define FEATURE_DATA_PS_MCAST_V6
  #define FEATURE_DATA_PS_DCC
  #define FEATURE_DATACOMMON_PS_IFACE_IO

  #define FEATURE_DATA_PS_PING
  #define FEATURE_DATA_PS_EAP
  #define FEATURE_QMI_CLIENT  /* Enable QMI mode handler */
  #define FEATURE_DATA_RM_NET_USES_SM  /* Used by tmc_apps.c for SMD bridge */
  #define FEATURE_DATACOMMON_PACKAGE_BMP

/* 2H09.1 Single Processor Image */
#elif defined FEATURE_DATACOMMON_2H09_1_SINGLE_PROC

  #define FEATURE_DATA_PS_DHCP
  #define FEATURE_DATA_PS_DHCPV6
  #define FEATURE_DATA_PS_ADDR_MGMT
  #define FEATURE_DATA_PS_LLC
  #define FEATURE_DATA_PS_MCAST
  #define FEATURE_DATA_PS_MCAST_V6
  #define FEATURE_DATA_PS_DCC
  #define FEATURE_DATACOMMON_PS_IFACE_IO

  #define FEATURE_DATA_PS_EAP
  #define FEATURE_DATA_PS_PING
  #define FEATURE_DATA_PS_DATA_LOGGING
  #define FEATURE_DATA_PS_DATA_LOGGING_PARTIAL_PPP
  #define FEATURE_DATA_PS_PPP
  #define FEATURE_DATA_PS_PPP_LOGGING
  #define FEATURE_DATA_PS_RSVP
  #define FEATURE_DATA_PS_HDLC
  #define FEATURE_DATA_PS_ROHC
  #define FEATURE_DATA_PS_IPHC
  #define FEATURE_DATA_RM_NET
  #define FEATURE_DATA_QMI
  #define FEATURE_DATA_QMI_EAP
  #define FEATURE_DATA_QMI_QOS
  #define FEATURE_DATA_QMI_MCAST
  #define FEATURE_HDLC_HW_ACCEL
  #define FEATURE_DATA_PS_SLIP
  #define FEATURE_DATACOMMON_PACKAGE_MODEM
  #ifdef FEATURE_MODEM_HEAP
    #define FEATURE_DATA_PS_SYSTEM_HEAP
  #endif /*FEATURE_MODEM_HEAP*/

  #ifdef FEATURE_CDMA
    #define FEATURE_DATA_PS_MIP
    #define FEATURE_DATA_PS_MIPV6
  #endif

/* 2H09.2 Single Processor Image */
#elif defined FEATURE_DATACOMMON_2H09_2_SINGLE_PROC

  #define FEATURE_DATA_PS_DHCP
  #define FEATURE_DATA_PS_DHCPV6
  #define FEATURE_DATA_PS_ADDR_MGMT
  #define FEATURE_DATA_PS_LLC
  #define FEATURE_DATA_PS_MCAST
  #define FEATURE_DATA_PS_MCAST_V6
  #define FEATURE_DATA_PS_DCC
  #define FEATURE_DATACOMMON_PS_IFACE_IO

  #define FEATURE_DATA_PS_EAP
  #define FEATURE_DATA_PS_PING
  #define FEATURE_DATA_PS_DATA_LOGGING
  #define FEATURE_DATA_PS_DATA_LOGGING_PARTIAL_PPP
  #define FEATURE_DATA_PS_PPP
  #define FEATURE_DATA_PS_PPP_LOGGING
  #define FEATURE_DATA_PS_RSVP
  #define FEATURE_DATA_PS_HDLC
  #define FEATURE_DATA_PS_ROHC
  #define FEATURE_DATA_PS_IPHC
  #define FEATURE_DATA_RM_NET
  #define FEATURE_DATA_QMI
  #define FEATURE_DATA_QMI_EAP
  #define FEATURE_DATA_QMI_QOS
  #define FEATURE_DATA_QMI_MCAST
  #define FEATURE_HDLC_HW_ACCEL
  #define FEATURE_DATA_PS_SLIP
  #define FEATURE_DATACOMMON_PACKAGE_MODEM
  #ifdef FEATURE_MODEM_HEAP
    #define FEATURE_DATA_PS_SYSTEM_HEAP
  #endif /*FEATURE_MODEM_HEAP*/

  #ifdef FEATURE_CDMA
    #define FEATURE_DATA_PS_MIP
    #define FEATURE_DATA_PS_MIPV6
  #endif
/* 2H09.2 Single Processor Image with EHRPD*/
#elif defined FEATURE_DATACOMMON_2H09_2_SINGLE_PROC_EHRPD

  #define FEATURE_DATA_PS_DHCP
  #define FEATURE_DATA_PS_DHCPV6
  #define FEATURE_DATA_PS_ADDR_MGMT
  #define FEATURE_DATA_PS_LLC
  #define FEATURE_DATA_PS_MCAST
  #define FEATURE_DATA_PS_MCAST_V6
  #define FEATURE_DATA_PS_DCC
  #define FEATURE_DATACOMMON_PS_IFACE_IO

  #define FEATURE_DATA_PS_EAP
  #define FEATURE_DATA_PS_PING
  #define FEATURE_DATA_PS_DATA_LOGGING
  #define FEATURE_DATA_PS_DATA_LOGGING_PARTIAL_PPP
  #define FEATURE_DATA_PS_PPP
  #define FEATURE_DATA_PS_PPP_LOGGING
  #define FEATURE_DATA_PS_RSVP
  #define FEATURE_DATA_PS_HDLC
  #define FEATURE_DATA_PS_ROHC
  #define FEATURE_DATA_PS_IPHC
  #define FEATURE_DATA_RM_NET
  #define FEATURE_DATA_RMNET_IPV6
  #define FEATURE_DATA_QMI
  #define FEATURE_DATA_QMI_EAP
  #define FEATURE_DATA_QMI_QOS
  #define FEATURE_DATA_QMI_MCAST
  #define FEATURE_HDLC_HW_ACCEL
  #define FEATURE_DATA_PS_SLIP
  #define FEATURE_DATACOMMON_PACKAGE_MODEM
  #ifdef FEATURE_MODEM_HEAP
    #define FEATURE_DATA_PS_SYSTEM_HEAP
  #endif /*FEATURE_MODEM_HEAP*/

  #ifdef FEATURE_CDMA
    #define FEATURE_DATA_PS_MIP
    #define FEATURE_DATA_PS_MIPV6
  #endif

  #define FEATURE_DATA_PS_EAP_AKA_SW_IMPL
  #define FEATURE_DATA_PS_PPP_EAP
  #define FEATURE_DATA_PS_EHRPD 
  #define FEATURE_DATA_PS_ENFORCE_AUTH 

/* 2H09.2 Single Processor Image for MDM9K tier */
#elif defined FEATURE_DATACOMMON_2H09_2_SINGLE_PROC_MDM

  #define FEATURE_DATA_PS_DHCP
  #define FEATURE_DATA_PS_DHCPV6
  #define FEATURE_DATA_PS_ADDR_MGMT
  #define FEATURE_DATA_PS_LLC
  #define FEATURE_DATA_PS_MCAST
  #define FEATURE_DATA_PS_MCAST_V6
  #define FEATURE_DATA_PS_DCC
  #define FEATURE_DATACOMMON_PS_IFACE_IO

  #define FEATURE_DATA_PS_EAP
  #define FEATURE_DATA_PS_PING
  #define FEATURE_DATA_PS_DATA_LOGGING
  #define FEATURE_DATA_PS_DATA_LOGGING_PARTIAL_PPP
  #define FEATURE_DATA_PS_PPP
  #define FEATURE_DATA_PS_PPP_LOGGING
  #define FEATURE_DATA_PS_RSVP
  #define FEATURE_DATA_PS_HDLC
  #define FEATURE_DATA_PS_ROHC
  #define FEATURE_DATA_PS_IPHC
  #define FEATURE_DATA_RM_NET
  #define FEATURE_DATA_RMNET_IPV6
  #define FEATURE_DATA_QMI
  #define FEATURE_DATA_QMI_QOS
  #define FEATURE_DATA_QMI_MCAST
  #define FEATURE_DATACOMMON_PACKAGE_MODEM
  #define FEATURE_DATA_A2
  #define FEATURE_DATA_RM_NET_MULTI
  #ifdef FEATURE_MODEM_HEAP
    #define FEATURE_DATA_PS_SYSTEM_HEAP
  #endif /*FEATURE_MODEM_HEAP*/

  #ifdef FEATURE_CDMA
    #define FEATURE_DATA_PS_MIP
  #endif

/* 2H09.2 Single Processor Image for MDM9K tier with EHRPD*/
#elif defined FEATURE_DATACOMMON_2H09_2_SINGLE_PROC_MDM_EHRPD

  #define FEATURE_DATA_PS_DHCP
  #define FEATURE_DATA_PS_DHCPV6
  #define FEATURE_DATA_PS_ADDR_MGMT
  #define FEATURE_DATA_PS_LLC
  #define FEATURE_DATA_PS_MCAST
  #define FEATURE_DATA_PS_MCAST_V6
  #define FEATURE_DATA_PS_DCC
  #define FEATURE_DATACOMMON_PS_IFACE_IO

  #define FEATURE_DATA_PS_EAP
  #define FEATURE_DATA_PS_PING
  #define FEATURE_DATA_PS_DATA_LOGGING
  #define FEATURE_DATA_PS_DATA_LOGGING_PARTIAL_PPP
  #define FEATURE_DATA_PS_PPP
  #define FEATURE_DATA_PS_PPP_LOGGING
  #define FEATURE_DATA_PS_RSVP
  #define FEATURE_DATA_PS_HDLC
  #define FEATURE_DATA_PS_ROHC
  #define FEATURE_DATA_PS_IPHC
  #define FEATURE_DATA_RM_NET
  #define FEATURE_DATA_RMNET_IPV6
  #define FEATURE_DATA_QMI
  #define FEATURE_DATA_QMI_QOS
  #define FEATURE_DATA_QMI_MCAST
  #define FEATURE_DATA_QMI_WMS
  #define FEATURE_DATACOMMON_PACKAGE_MODEM
  #define FEATURE_DATA_A2
  #define FEATURE_DATA_RM_NET_MULTI
  #ifdef FEATURE_MODEM_HEAP
    #define FEATURE_DATA_PS_SYSTEM_HEAP
  #endif /*FEATURE_MODEM_HEAP*/

  #define FEATURE_DATA_PS_EAP_AKA_SW_IMPL
  #define FEATURE_DATA_PS_PPP_EAP
  #define FEATURE_DATA_PS_EHRPD  
  #define FEATURE_DATA_PS_ENFORCE_AUTH

  #ifdef FEATURE_CDMA
    #define FEATURE_DATA_PS_MIP
  #endif
  #ifdef FEATURE_MDM_MSM_FUSION
    #define FEATURE_DATA_FUSION_MDM
  #endif

#elif defined FEATURE_DATACOMMON_2H09_1_THIRD_PARTY_APPS
  #define FEATURE_DATACOMMON_THIRD_PARTY_APPS
  /* No additional features required currently. */

#else
  /* No action needed: Cust file read can happen during Build tools
     generation and no package gets defined at that time!! */

#endif /* FEATURE_DATACOMMON_2H09_1_SINGLE_PROC       */
       /* FEATURE_DATACOMMON_2H09_1_DUAL_PROC_BP_APPS */
       /* FEATURE_DATACOMMON_2H09_1_DUAL_PROC_MODEM   */

/*-------------------------------------------------------------------------
  Enable features based on non-PS features
-------------------------------------------------------------------------*/
#ifdef FEATURE_HDR_QOS
  #define FEATURE_DATA_PS_DO_QOS
#endif /* FEATURE_HDR_QOS */

#ifdef FEATURE_HDLC_HW_ACCEL
  #define FEATURE_HDLC_SW_WORKAROUND
  #define FEATURE_PPP_HW_ACCEL
#endif

#if defined(FEATURE_WCDMA) && defined(FEATURE_DSM_DYNAMIC_POOL_SELECTION)
  #define FEATURE_DATA_PS_HDLC_DSM_OPTIMIZED
#endif

#if defined(FEATURE_WCDMA)
  #define FEATURE_DATA_WCDMA_PS
#endif

#if defined(FEATURE_GSM_GPRS) || defined(FEATURE_WCDMA)
  #undef FEATURE_DATA_WCDMA_PS_PDP_IP
  #define FEATURE_STA_QOS
  #define FEATURE_PS_DORMANT_PWR_SAVE
#endif /* (GSM || WCDMA) */

/* --------------------------------------------------------------------------
** Handoff Support
** ------------------------------------------------------------------------*/
#if defined(FEATURE_LTE) && defined(FEATURE_EHRPD)
#define FEATURE_EPC_HANDOFF
#endif /* FEATURE_LTE && FEATURE_EHRPD*/

#ifdef TEST_FRAMEWORK
#define FEATURE_DATA_OFFTARGET_TEST
#endif /* TEST_FRAMEWORK */

#endif /* CUSTDATACOMMON_H */
