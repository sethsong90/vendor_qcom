# definitions for oncrpc used by many files
oncrpc_common_defines := -D__packed__=
oncrpc_common_defines += -DIMAGE_APPS_PROC
oncrpc_common_defines += -DFEATURE_Q_SINGLE_LINK
oncrpc_common_defines += -DFEATURE_Q_NO_SELF_QPTR
oncrpc_common_defines += -DFEATURE_NATIVELINUX
oncrpc_common_defines += -DFEATURE_DSM_DUP_ITEMS
oncrpc_common_defines += -DFEATURE_ANDROID
oncrpc_common_defines += -DASSERT=ASSERT_FATAL
oncrpc_common_defines += -DFEATURE_SMEM_LOG
oncrpc_common_defines += -DONCRPC_SMEM_DEBUG
oncrpc_common_defines += -DONCRPC_SYSLOG_DEBUG
ifeq ($(call is-board-platform-in-list,msm7627_surf msm7627a msm7630_surf msm8660 msm7627_6x),true)
oncrpc_common_defines += -DONCRPC_64K_RPC
endif

ifeq ($(call is-board-platform,msm8660),true)
oncrpc_common_defines += -DWAIT_FOR_SERVER
oncrpc_common_defines += -DFEATURE_MODEM_LOAD
endif

# definitions for oncrpc (from the Makefile)
oncrpc_defines := -DONCRPC_REPLY_SIG=0x00800000
oncrpc_defines += -Doncrpc_printf=printf
oncrpc_defines += -DONCRPC_ERR_FATAL=ERR_FATAL
oncrpc_defines += -DFEATURE_ONCRPC_SM_IS_ROUTER
oncrpc_defines += -DFEATURE_ONCRPC_ROUTER
oncrpc_defines += -D$(FEATURE_ONCRPC_TRANSPORT)=FEATURE_ONCRPC_ROUTER
oncrpc_defines += -DFEATURE_ONCRPC
oncrpc_defines += -D__linux
oncrpc_defines += -DRPC_ROUTER_LOCAL_PROCESSOR_ID=1
oncrpc_defines += -DRPC_ROUTER_REMOTE_DEFAULT_PROCESSOR_ID=0
oncrpc_defines += -DFEATURE_ONCRPC_SERVER_CLEANUP_SUPPORT
