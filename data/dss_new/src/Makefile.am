AM_CFLAGS = -Wundef \
        -Wno-trigraphs \
        -g -O0 \
        -fno-inline \
        -fno-short-enums \
        -fpic

AM_CPPFLAGS = -D__packed__= \
        -DIMAGE_APPS_PROC \
        -DFEATURE_Q_NO_SELF_QPTR \
        -DAEEINTERFACE_CPLUSPLUS \
        -DFEATURE_SMD \
        -DFEATURE_DIAG_MP_MASTER_APPS \
        -DFEATURE_DSS_LINUX \
        -DFEATURE_DATA_LINUX_LE \
        -DFEATURE_MULTIPROCESSOR \
        -DFEATURE_DSUTILS_OFFTARGET \
        -DFEATURE_NO_REX \
        -I$(top_srcdir)/dss_new/src/platform/inc \
        -I../inc \
        -I$(top_srcdir)/netmgr/inc \
        -I$(top_srcdir)/dss_new/src/api \
        -I$(top_srcdir)/dss_new/src/api/dss \
        -I$(top_srcdir)/dss_new/src/api/netiface \
        -I$(top_srcdir)/dss_new/src/api/utils \
        -I$(top_srcdir)/dss_new/src/dss/inc \
        -I$(top_srcdir)/dss_new/src/dsnet/inc \
        -I$(top_srcdir)/dss_new/src/dssock/inc \
        -I$(top_srcdir)/dss_new/src/dssock/src \
        -I$(top_srcdir)/dss_new/src/dssvc/inc \
        -I$(top_srcdir)/dss_new/src/inet/inc \
        -I$(top_srcdir)/dss_new/src/utils/src \
        -I$(top_srcdir)/dss_new/src/utils/inc \
        -I$(top_srcdir)/dss_new/src/tasks/inc \
        -I$(top_srcdir)/dss_new/src/netiface/src \
        -I$(top_srcdir)/dss_new/src/netiface/inc \
        $(QMI_CFLAGS) \
        -I$(top_srcdir)/dss_new/src/qmiifacectls/src \
        -I$(top_srcdir)/dss_new/src/qmiifacectls/inc \
        $(DIAG_CFLAGS) \
        $(DSUTILS_CFLAGS) \
        -I$(WORKSPACE)/common/inc

libdsnet_c_sources = platform/src/dcc_task_linux.c \
        platform/src/ds_sig_task_linux.c \
        platform/src/pstimer.c \
        platform/src/ps_platform_timer.c \
        platform/src/ps_platform_crit_sect.c \
        platform/src/ps_system_heap_linux.c \
        platform/src/qw.c \
        platform/src/time_svc_platform.c \
        platform/src/dss_linux_stubs.c \
        platform/src/ps_utils.c \
        platform/src/queue.c \
        platform/src/dsnet_lib.c \
        utils/src/ds_Utils_Atomic.cpp \
        utils/src/ds_Utils_Conversion.cpp \
        utils/src/ds_Utils_CreateInstance.cpp \
        utils/src/ds_Utils_CritSect.cpp \
        utils/src/ds_Utils_Factory.cpp \
        utils/src/ds_Utils_List.cpp \
        utils/src/ds_Utils_MemManager.cpp \
        utils/src/ds_Utils_Signal.cpp \
        utils/src/ds_Utils_SignalBus.cpp \
        utils/src/ds_Utils_SignalCtl.cpp \
        utils/src/ds_Utils_SignalFactory.cpp \
        utils/src/ds_Utils_SignalHandler.cpp \
        utils/src/ds_Utils_SignalHandlerBase.cpp \
        utils/src/ds_Utils_FullPrivSet.cpp \
        utils/src/ps_handle_mgr.c \
        utils/src/ps_mem.c \
        utils/src/ps_stat_common.c \
        utils/src/ps_stat_logging.c \
        utils/src/ps_stat_mem.c \
        utils/src/ps_utils_init.cpp \
        netiface/src/netiface_stubs.c \
        netiface/src/ps_acl.c \
        netiface/src/ps_aclrules.c\
        netiface/src/ps_arbitration_mgr.c \
        netiface/src/ps_flow.c \
        netiface/src/ps_flow_ioctl.c \
        netiface/src/ps_flowi.c \
        netiface/src/ps_flowi_event.c \
        netiface/src/ps_iface.c \
        netiface/src/ps_iface_addr_v6.c \
        netiface/src/ps_iface_flow.c \
        netiface/src/ps_iface_ioctl.c \
        netiface/src/ps_iface_ipfltr.c \
        netiface/src/ps_iface_logging.c \
        netiface/src/ps_iface_rx_qos_fltr.c \
        netiface/src/ps_ifacei_event.c \
        netiface/src/ps_inbound_acl.c \
        netiface/src/ps_phys_link.c \
        netiface/src/ps_stat_iface.c \
        netiface/src/ps_qos_spec_logging.c \
        netiface/src/ps_iface_logical_flowi.c \
        netiface/src/ps_iface_dns_cache.c \
        netiface/src/ps_dns_cache_config.c \
        netiface/src/ps_phys_link_io.c \
        netiface/src/ps_phys_link_ioctl.c \
        netiface/src/ps_phys_linki_event.c \
        netiface/src/ps_policy_mgr.c \
        netiface/src/ps_route.c \
        netiface/src/ps_netiface_init.c \
        netiface/src/ps_routei.c \
        netiface/src/NetPlatformLib.cpp \
        qmiifacectls/src/ds_qmh_config.c \
        qmiifacectls/src/ds_qmh_acl.c \
        qmiifacectls/src/ds_qmh.c \
        qmiifacectls/src/ds_qmh_hdlr.c \
        qmiifacectls/src/ds_qmh_ioctl.c \
        qmiifacectls/src/ds_qmh_llif.c \
        qmiifacectls/src/ds_qmh_netplat.c \
        qmiifacectls/src/ds_qmh_sm.c \
        qmiifacectls/src/ds_qmh_sm_int.c \
        dsnet/src/ds_Net_BearerTech.cpp \
        dsnet/src/ds_Net_CreateInstance.cpp \
        dsnet/src/ds_Net_Init.cpp \
        dsnet/src/ds_Net_NetworkActive.cpp \
        dsnet/src/ds_Net_NetworkMonitored.cpp \
        dsnet/src/ds_Net_MemManager.cpp \
        dsnet/src/ds_Net_EventManager.cpp \
        dsnet/src/ds_Net_Handle.cpp \
        dsnet/src/ds_Net_MCastSession.cpp \
        dsnet/src/ds_Net_MCastSessionsInput.cpp \
        dsnet/src/ds_Net_MCastSessionsOutput.cpp \
        dsnet/src/ds_Net_MCastSessionPriv.cpp \
        dsnet/src/ds_Net_MCastMBMSCtrl.cpp \
        dsnet/src/ds_Net_Network.cpp \
        dsnet/src/ds_Net_Network1X.cpp \
        dsnet/src/ds_Net_NetworkFactory.cpp \
        dsnet/src/ds_Net_NetworkIPv6.cpp \
        dsnet/src/ds_Net_NetworkIPv6Address.cpp \
        dsnet/src/ds_Net_NetworkUMTS.cpp \
        dsnet/src/ds_Net_PhysLink.cpp \
        dsnet/src/ds_Net_Policy.cpp \
        dsnet/src/ds_Net_NatSession.cpp \
        dsnet/src/ds_Net_MBMSSpec.cpp \
        dsnet/src/ds_Net_IPFilterReg.cpp \
        dsnet/src/ds_Net_MTPDReg.cpp \
        dsnet/src/ds_Net_TechUMTSFactory.cpp \
        dsnet/src/ds_Net_MCastManager.cpp \
        dsnet/src/ds_Net_Conversion.cpp \
        dsnet/src/ds_Net_MCastManagerPriv.cpp \
        dsnet/src/ds_Net_NatManager.cpp \
        dsnet/src/ds_Net_ClassIDInstantiator.cpp \
        dsnet/src/ds_Net_NetworkFactoryClient.cpp

requiredlibs = -lpthread \
	../../netmgr/src/libnetmgr.la

library_includedir = $(pkgincludedir)

#libdsnet_la_CC = @CC@
libdsnet_la_SOURCES = $(libdsnet_c_sources)
if USE_GLIB
libdsnet_la_CFLAGS  = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
libdsnet_la_LDFLAGS  = -lstdc++ $(DIAG_LIBS) $(DSUTILS_LIBS) -lqmi  -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libdsnet_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libdsnet_la_CFLAGS = $(AM_CFLAGS) -fPIC
libdsnet_la_LDFLAGS = -lpthread $(DIAG_LIBS) $(DSUTILS_LIBS) -lqmi  -shared -version-info 1:0:0
libdsnet_la_CPPFLAGS = $(AM_CPPFLAG)
endif
libdsnet_la_LIBADD = $(requiredlibs) -lstdc++ $(DSUTILS_LIBS)

##############dss_new#############

libdssnew_c_sources = dss/src/DSS_BearerTechHandler.cpp \
        dss/src/DSS_Conversion.cpp \
        dss/src/DSS_EventHandler.cpp \
        dss/src/DSS_EventHandlerNetApp.cpp \
        dss/src/DSS_EventHandlerPrivIPV6Addr.cpp \
        dss/src/DSS_ExtendedIPConfigHandler.cpp \
        dss/src/DSS_GenScope.cpp \
        dss/src/DSS_Globals.cpp \
        dss/src/DSS_HDRRev0RateInteriaHandler.cpp \
        dss/src/DSS_IPv6PrefixChangedStateHandler.cpp \
        dss/src/DSS_MTPDRequestHandler.cpp \
        dss/src/DSS_MemoryManagement.cpp \
        dss/src/DSS_NetworkIPHandler.cpp \
        dss/src/DSS_NetApp.cpp \
        dss/src/DSS_NetDSAPI.cpp  \
        dss/src/DSS_NetIOCTLDSAPI.cpp \
        dss/src/DSS_RFConditionsHandler.cpp \
        dss/src/DSS_NetworkStateHandler.cpp \
        dss/src/DSS_OutageHandler.cpp \
        dss/src/DSS_PhysLinkStateHandler.cpp \
        dss/src/DSS_PrimaryNetApp.cpp \
        dss/src/DSS_PrivIpv6Addr.cpp \
        dss/src/DSS_PrivIpv6AddrHandler.cpp \
        dss/src/DSS_SecondaryNetApp.cpp \
        dss/src/DSS_SlottedResultHandler.cpp \
        dss/src/DSS_SlottedSessionChangedHandler.cpp \
        dss/src/DSS_EventHandlerQoS.cpp \
        dss/src/DSS_NetQoSDefault.cpp \
        dss/src/DSS_NetQoSSecondary.cpp	\
        dss/src/DSS_PrimaryQoSModifyHandler.cpp \
        dss/src/DSS_PrimaryQoSModifyStatusHandler.cpp \
        dss/src/DSS_QoSAwareUnAwareHandler.cpp \
        dss/src/DSS_QoSHandler.cpp \
        dss/src/DSS_QoSInfoCodeUpdatedHandler.cpp \
        dss/src/DSS_QoSModifyHandler.cpp \
        dss/src/DSS_QoSProfileChangedHandler.cpp \
        dss/src/DSS_QoSRequestExScope.cpp \
        dss/src/DSS_Socket.cpp \
        dss/src/DSS_dsapi.cpp \
        dss/src/dss_init.c \
        dss/src/dss_net_mgr.c \
        dss/src/dss_ping.c  \
        dss/src/dss_ping_comm_mgr.c \
        dss/src/ps_stat_sock.c \
        dss/src/ps_sock_mem_pool.cpp


#lib_LTLIBRARIES = libdssnew.la
libdssnew_la_SOURCES = $(libdssnew_c_sources)
if USE_GLIB
libdssnew_la_CFLAGS  = -DUSE_GLIB $(AM_CFLAGS) $(DIAG_CFLAGS) @GLIB_CFLAGS@
libdssnew_la_LDFLAGS  = -lstdc++ $(DIAG_LIBS) $(DSUTILS_LIBS) -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libdssnew_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libdssnew_la_CFLAGS = $(AM_CFLAGS) $(DIAG_CFLAGS) -fPIC
libdssnew_la_LDFLAGS = -lpthread $(DIAG_LIBS) $(DSUTILS_LIBS) -lqmi -shared -version-info 1:0:0
libdssnew_la_CPPFLAGS = $(AM_CPPFLAGS)
endif
libdssnew_la_LIBADD = $(requiredlibs) libdsnet.la -lstdc++

#Create and Install libraries
lib_LTLIBRARIES = libdsnet.la libdssnew.la
