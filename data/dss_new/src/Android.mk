LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Include DSS common definitions
include $(LOCAL_PATH)/Android.min
LOCAL_ADDITIONAL_DEPENDENCIES += $(LOCAL_PATH)/Android.min

LOCAL_SRC_FILES += platform/src/dcc_task_linux.c
LOCAL_SRC_FILES += platform/src/dsnet_lib.c
LOCAL_SRC_FILES += platform/src/ds_sig_task_linux.c
LOCAL_SRC_FILES += platform/src/pstimer.c
LOCAL_SRC_FILES += platform/src/ps_platform_timer.c
LOCAL_SRC_FILES += platform/src/ps_platform_crit_sect.c
LOCAL_SRC_FILES += platform/src/ps_system_heap_linux.c
LOCAL_SRC_FILES += platform/src/qw.c
LOCAL_SRC_FILES += platform/src/time_svc_platform.c
LOCAL_SRC_FILES += platform/src/dss_linux_stubs.c
LOCAL_SRC_FILES += platform/src/ps_utils.c

LOCAL_SRC_FILES += utils/src/ds_Utils_Atomic.cpp
LOCAL_SRC_FILES += utils/src/ds_Utils_Conversion.cpp
LOCAL_SRC_FILES += utils/src/ds_Utils_CreateInstance.cpp
LOCAL_SRC_FILES += utils/src/ds_Utils_CritSect.cpp
LOCAL_SRC_FILES += utils/src/ds_Utils_Factory.cpp
LOCAL_SRC_FILES += utils/src/ds_Utils_List.cpp
LOCAL_SRC_FILES += utils/src/ds_Utils_MemManager.cpp
LOCAL_SRC_FILES += utils/src/ds_Utils_Signal.cpp
LOCAL_SRC_FILES += utils/src/ds_Utils_SignalBus.cpp
LOCAL_SRC_FILES += utils/src/ds_Utils_SignalCtl.cpp
LOCAL_SRC_FILES += utils/src/ds_Utils_SignalFactory.cpp
LOCAL_SRC_FILES += utils/src/ds_Utils_SignalHandler.cpp
LOCAL_SRC_FILES += utils/src/ds_Utils_SignalHandlerBase.cpp
LOCAL_SRC_FILES += utils/src/ds_Utils_FullPrivSet.cpp
LOCAL_SRC_FILES += utils/src/ps_handle_mgr.c
LOCAL_SRC_FILES += utils/src/ps_mem.c
LOCAL_SRC_FILES += utils/src/ps_stat_common.c
LOCAL_SRC_FILES += utils/src/ps_stat_logging.c
LOCAL_SRC_FILES += utils/src/ps_stat_mem.c
LOCAL_SRC_FILES += utils/src/ps_utils_init.cpp

LOCAL_SRC_FILES += netiface/src/netiface_stubs.c
LOCAL_SRC_FILES += netiface/src/ps_acl.c
LOCAL_SRC_FILES += netiface/src/ps_aclrules.c
LOCAL_SRC_FILES += netiface/src/ps_arbitration_mgr.c
LOCAL_SRC_FILES += netiface/src/ps_flow.c
LOCAL_SRC_FILES += netiface/src/ps_flow_ioctl.c
LOCAL_SRC_FILES += netiface/src/ps_flowi.c
LOCAL_SRC_FILES += netiface/src/ps_flowi_event.c
LOCAL_SRC_FILES += netiface/src/ps_iface.c
LOCAL_SRC_FILES += netiface/src/ps_iface_addr_v6.c
LOCAL_SRC_FILES += netiface/src/ps_iface_flow.c
LOCAL_SRC_FILES += netiface/src/ps_iface_ioctl.c
LOCAL_SRC_FILES += netiface/src/ps_iface_ipfltr.c
LOCAL_SRC_FILES += netiface/src/ps_iface_logging.c
LOCAL_SRC_FILES += netiface/src/ps_iface_rx_qos_fltr.c
LOCAL_SRC_FILES += netiface/src/ps_ifacei_event.c
LOCAL_SRC_FILES += netiface/src/ps_inbound_acl.c
LOCAL_SRC_FILES += netiface/src/ps_phys_link.c
LOCAL_SRC_FILES += netiface/src/ps_stat_iface.c
LOCAL_SRC_FILES += netiface/src/ps_qos_spec_logging.c
LOCAL_SRC_FILES += netiface/src/ps_iface_logical_flowi.c
LOCAL_SRC_FILES += netiface/src/ps_iface_dns_cache.c
LOCAL_SRC_FILES += netiface/src/ps_dns_cache_config.c

# required by ds_qmh_hdlr.c
LOCAL_SRC_FILES += netiface/src/ps_phys_link_io.c
LOCAL_SRC_FILES += netiface/src/ps_phys_link_ioctl.c
LOCAL_SRC_FILES += netiface/src/ps_phys_linki_event.c
LOCAL_SRC_FILES += netiface/src/ps_policy_mgr.c
LOCAL_SRC_FILES += netiface/src/ps_route.c
LOCAL_SRC_FILES += netiface/src/ps_netiface_init.c
LOCAL_SRC_FILES += netiface/src/ps_routei.c
LOCAL_SRC_FILES += netiface/src/NetPlatformLib.cpp

LOCAL_SRC_FILES += qmiifacectls/src/ds_qmh_config.c
LOCAL_SRC_FILES += qmiifacectls/src/ds_qmh_acl.c
LOCAL_SRC_FILES += qmiifacectls/src/ds_qmh.c
LOCAL_SRC_FILES += qmiifacectls/src/ds_qmh_hdlr.c
LOCAL_SRC_FILES += qmiifacectls/src/ds_qmh_ioctl.c
LOCAL_SRC_FILES += qmiifacectls/src/ds_qmh_llif.c
LOCAL_SRC_FILES += qmiifacectls/src/ds_qmh_netplat.c
LOCAL_SRC_FILES += qmiifacectls/src/ds_qmh_sm.c
LOCAL_SRC_FILES += qmiifacectls/src/ds_qmh_sm_int.c

LOCAL_SRC_FILES += dsnet/src/ds_Net_BearerTech.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_CreateInstance.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_Init.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_NetworkActive.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_NetworkMonitored.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_MemManager.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_EventManager.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_Handle.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_MCastSession.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_MCastSessionsInput.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_MCastSessionsOutput.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_MCastSessionPriv.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_MCastMBMSCtrl.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_Network.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_Network1X.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_NetworkFactory.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_NetworkIPv6.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_NetworkIPv6Address.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_NetworkUMTS.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_PhysLink.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_Policy.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_NatSession.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_MBMSSpec.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_IPFilterReg.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_MTPDReg.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_TechUMTSFactory.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_MCastManager.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_Conversion.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_MCastManagerPriv.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_NatManager.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_ClassIDInstantiator.cpp
LOCAL_SRC_FILES += dsnet/src/ds_Net_NetworkFactoryClient.cpp


LOCAL_MODULE := libdsnet
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := \
   libdiag     \
   libqmi      \
   libdsutils  \
   libcutils  \
   libnetmgr

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

##################################################


include $(CLEAR_VARS)

# Include DSS common definitions
include $(LOCAL_PATH)/Android.min
LOCAL_ADDITIONAL_DEPENDENCIES += $(LOCAL_PATH)/Android.min

LOCAL_SRC_FILES += dss/src/DSS_BearerTechHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_Conversion.cpp
LOCAL_SRC_FILES += dss/src/DSS_EventHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_EventHandlerNetApp.cpp
LOCAL_SRC_FILES += dss/src/DSS_EventHandlerPrivIPV6Addr.cpp
LOCAL_SRC_FILES += dss/src/DSS_ExtendedIPConfigHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_GenScope.cpp
LOCAL_SRC_FILES += dss/src/DSS_Globals.cpp
LOCAL_SRC_FILES += dss/src/DSS_HDRRev0RateInteriaHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_IPv6PrefixChangedStateHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_MTPDRequestHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_MemoryManagement.cpp
LOCAL_SRC_FILES += dss/src/DSS_NetworkIPHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_NetApp.cpp
LOCAL_SRC_FILES += dss/src/DSS_NetDSAPI.cpp
LOCAL_SRC_FILES += dss/src/DSS_NetIOCTLDSAPI.cpp
LOCAL_SRC_FILES += dss/src/DSS_RFConditionsHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_NetworkStateHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_OutageHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_PhysLinkStateHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_PrimaryNetApp.cpp
LOCAL_SRC_FILES += dss/src/DSS_PrivIpv6Addr.cpp
LOCAL_SRC_FILES += dss/src/DSS_PrivIpv6AddrHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_SecondaryNetApp.cpp
LOCAL_SRC_FILES += dss/src/DSS_SlottedResultHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_SlottedSessionChangedHandler.cpp

#QOS
LOCAL_SRC_FILES += dss/src/DSS_EventHandlerQoS.cpp
LOCAL_SRC_FILES += dss/src/DSS_NetQoSDefault.cpp
LOCAL_SRC_FILES += dss/src/DSS_NetQoSSecondary.cpp
LOCAL_SRC_FILES += dss/src/DSS_PrimaryQoSModifyHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_PrimaryQoSModifyStatusHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_QoSAwareUnAwareHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_QoSHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_QoSInfoCodeUpdatedHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_QoSModifyHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_QoSProfileChangedHandler.cpp
LOCAL_SRC_FILES += dss/src/DSS_QoSRequestExScope.cpp

LOCAL_SRC_FILES += dss/src/DSS_Socket.cpp
LOCAL_SRC_FILES += dss/src/DSS_dsapi.cpp
LOCAL_SRC_FILES += dss/src/dss_init.c
LOCAL_SRC_FILES += dss/src/dss_net_mgr.c
LOCAL_SRC_FILES += dss/src/dss_ping.c
LOCAL_SRC_FILES += dss/src/dss_ping_comm_mgr.c
LOCAL_SRC_FILES += dss/src/ps_stat_sock.c
LOCAL_SRC_FILES += dss/src/ps_sock_mem_pool.cpp

LOCAL_MODULE := libdssock
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := \
   libdiag     \
   libqmi      \
   libdsutils  \
   libnetmgr   \
   libdsnet

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
