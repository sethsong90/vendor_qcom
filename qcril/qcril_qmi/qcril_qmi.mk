# LOCAL_PATH and the include path need to be hard-coded because wmsts is inside
# the qcril directory (bug in the Android makefile system).
# LOCAL_PATH reflects parent directory to ensure common objects are in
# separate pools for each RIL variant.

LOCAL_PATH := $(QCRIL_DIR)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := qcril_qmi/qcril.c
LOCAL_SRC_FILES += qcril_qmi/qcril_arb.c
LOCAL_SRC_FILES += qcril_qmi/qcril_cm_ss.c
LOCAL_SRC_FILES += qcril_qmi/qcril_cm_util.c
LOCAL_SRC_FILES += qcril_qmi/qcril_event.c
LOCAL_SRC_FILES += qcril_qmi/qcril_log.c
LOCAL_SRC_FILES += qcril_qmi/qcril_other.c
LOCAL_SRC_FILES += qcril_qmi/qcril_pbm.c
LOCAL_SRC_FILES += qcril_qmi/qcril_reqlist.c
LOCAL_SRC_FILES += qcril_qmi/qcril_qmi_voice.c
LOCAL_SRC_FILES += qcril_qmi/qcril_qmi_client.c
LOCAL_SRC_FILES += qcril_qmi/qcril_qmi_nas.c
LOCAL_SRC_FILES += qcril_qmi/qcril_qmi_nas2.c
LOCAL_SRC_FILES += qcril_qmi/qcril_qmi_sms.c
LOCAL_SRC_FILES += qcril_qmi/qcril_qmi_sms_errors.c
LOCAL_SRC_FILES += qcril_qmi/qcril_qmi_ims.c
LOCAL_SRC_FILES += qcril_qmi/qcril_qmi_coex.c
LOCAL_SRC_FILES += qcril_qmi/qcril_qmi_imss.c
LOCAL_SRC_FILES += qcril_qmi/qcril_qmi_pdc.c
LOCAL_SRC_FILES += qcril_qmi/qcril_am.cc
LOCAL_SRC_FILES += qcril_qmi/qcril_db.c

ifdef PROTOBUF_SUPPORTED
LOCAL_SRC_FILES += qcril_qmi/ims_socket/imsIF.pb-c.c
endif
LOCAL_SRC_FILES += qcril_qmi/qcril_qmi_imsa.c

LOCAL_SRC_FILES += qcril_qmi/ims_socket/qcril_qmi_ims_socket.cc
LOCAL_SRC_FILES += qcril_qmi/ims_socket/qcril_qmi_ims_packing.c
LOCAL_SRC_FILES += qcril_qmi/ims_socket/qcril_qmi_ims_misc.c
LOCAL_SRC_FILES += qcril_qmi/oem_socket/qcril_qmi_oem_socket.cc
LOCAL_SRC_FILES += qcril_qmi/qcril_qmi_generic_socket.cc
LOCAL_SRC_FILES += qcril_qmi/ims_socket/qcril_qmi_ims_if_pb.c
LOCAL_SRC_FILES += qcril_qmi/qcril_qmi_pil_monitor.cc
LOCAL_SRC_FILES += common/uim/qcril_gstk_qmi.c
LOCAL_SRC_FILES += common/uim/qcril_uim.c
LOCAL_SRC_FILES += common/uim/qcril_uim_card.c
LOCAL_SRC_FILES += common/uim/qcril_uim_file.c
LOCAL_SRC_FILES += common/uim/qcril_uim_security.c
LOCAL_SRC_FILES += common/uim/qcril_uim_util.c
LOCAL_SRC_FILES += common/uim/qcril_uim_queue.c
LOCAL_SRC_FILES += common/uim/qcril_uim_refresh.c
LOCAL_SRC_FILES += common/uim/qcril_uim_restart.c
LOCAL_SRC_FILES += common/uim/qcril_scws.c
LOCAL_SRC_FILES += common/uim/qcril_scws_opt.c
LOCAL_SRC_FILES += common/uim/qcril_uim_qcci.c
LOCAL_SRC_FILES += qcril_qmi/services/qmi_embms_v01.c
LOCAL_SRC_FILES += qcril_qmi/services/qtuner_v01.c

LOCAL_SRC_FILES += qcril_qmi/core/core/core_handler.c
LOCAL_SRC_FILES += qcril_qmi/core/core/core_event_lookup.c
LOCAL_SRC_FILES += qcril_qmi/core/core/core_queue_util.c
LOCAL_SRC_FILES += qcril_qmi/cri/core/cri_core.c
LOCAL_SRC_FILES += qcril_qmi/cri/core/cri_rule_handler.c
LOCAL_SRC_FILES += qcril_qmi/cri/core/cri_event_lookup.c
LOCAL_SRC_FILES += qcril_qmi/cri/csvt/cri_csvt_core.c
LOCAL_SRC_FILES += qcril_qmi/cri/csvt/cri_csvt_rules.c
LOCAL_SRC_FILES += qcril_qmi/cri/csvt/cri_csvt_utils.c
LOCAL_SRC_FILES += qcril_qmi/hlos/core/hlos_core.c
LOCAL_SRC_FILES += qcril_qmi/hlos/core/hlos_event_lookup.c
LOCAL_SRC_FILES += qcril_qmi/hlos/csvt/hlos_csvt_core.c

LOCAL_SRC_FILES += qcril_qmi/utilities/control/control_core.c
LOCAL_SRC_FILES += qcril_qmi/utilities/list/util_list.c
LOCAL_SRC_FILES += qcril_qmi/utilities/memory/util_memory.c
LOCAL_SRC_FILES += qcril_qmi/utilities/bit_field/util_bit_field.c
LOCAL_SRC_FILES += qcril_qmi/utilities/synchronization/util_synchronization.c
LOCAL_SRC_FILES += qcril_qmi/utilities/timer/util_timer.c
LOCAL_SRC_FILES += qcril_qmi/utilities/timer/timer_event_lookup.c

# for asprinf
LOCAL_CFLAGS := -D_GNU_SOURCE

# for embms
LOCAL_CFLAGS += -DFEATURE_DATA_EMBMS

ifeq ($(call is-board-platform-in-list,msm7630_surf msm7630_fusion msm8660 msm8960 msm8974 msm8610 msm8226 msm7627_surf msm7627a),true)
# These targets have route look up available on modem
  LOCAL_SRC_FILES += common/data/qcril_data_netctrl.c
  LOCAL_SRC_FILES += common/data/qcril_data_qos.c
  LOCAL_SRC_FILES += common/data/qcril_data_embms.c
  LOCAL_SRC_FILES += common/data/qcril_data_client.c
  LOCAL_SRC_FILES += common/data/qcril_data_utils.c
  LOCAL_CFLAGS += -DFEATURE_QCRIL_USE_NETCTRL
else
# These targets do not have route look up available on modem
  LOCAL_SRC_FILES += common/data/qcril_data.c
  LOCAL_SRC_FILES += common/data/qcril_datai.c
endif

ifdef PROTOBUF_SUPPORTED
LOCAL_STATIC_LIBRARIES := libprotobuf-c
endif

LOCAL_SHARED_LIBRARIES := libdsutils  # must preceed libcutils in ICS build
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libril
LOCAL_SHARED_LIBRARIES += librilutils

LOCAL_SHARED_LIBRARIES += libril-qcril-hook-oem
LOCAL_SHARED_LIBRARIES += libqmi
LOCAL_SHARED_LIBRARIES += libqcci_legacy
LOCAL_SHARED_LIBRARIES += libdiag
LOCAL_SHARED_LIBRARIES += libhardware_legacy
LOCAL_SHARED_LIBRARIES += libqmiservices
LOCAL_SHARED_LIBRARIES += libidl
LOCAL_SHARED_LIBRARIES += libtime_genoff
LOCAL_SHARED_LIBRARIES += libmedia
LOCAL_SHARED_LIBRARIES += libbinder
LOCAL_SHARED_LIBRARIES += libsqlite


ifeq ($(call is-board-platform-in-list,msm7630_surf msm7630_fusion msm8660 msm8960 msm8974 msm8610 msm8226 msm7627_surf msm7627a),true)
# These targets have route look up available on modem
  LOCAL_SHARED_LIBRARIES += libdsi_netctrl
  LOCAL_SHARED_LIBRARIES += libqdp
else
# These targets do not have route look up available on modem
  LOCAL_SHARED_LIBRARIES += libdss
endif


ifeq ($(call is-board-platform-in-list,msm7630_surf msm7630_fusion msm8660 msm8960 msm8974 msm8610 msm8226 msm7627_surf msm7627a),true)
# define for the case when modem boots up after apps proc
  LOCAL_CFLAGS += -DFEATURE_QCRIL_USE_QDP
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/ims_socket/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/oem_socket/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/services/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/utilities/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/utilities/control
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/utilities/list
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/utilities/log
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/utilities/memory
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/utilities/bit_field
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/utilities/synchronization
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/utilities/timer
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/core/core/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/cri/core/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/cri/csvt/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/hlos/core/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcril_qmi/hlos/csvt/
LOCAL_C_INCLUDES += bionic/libc/include/
LOCAL_C_INCLUDES += external/sqlite/dist/


LOCAL_C_INCLUDES += hardware/ril/include/telephony/

LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/auth/inc/

LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/common/inc/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/common/data/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/common/uim/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/qcrilhook_oem/
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/data/inc/
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/diag/include/
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/qmi/inc/
ifdef PROTOBUF_SUPPORTED
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/protobuf-c/include/
endif
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/time-services/

# For API Definitions and enables
LOCAL_CFLAGS   += $(remote_api_defines)
LOCAL_CFLAGS   += $(remote_api_enables)

# defines necessary for QCRIL code
LOCAL_CFLAGS += -DRIL_SHLIB
LOCAL_CFLAGS += -DFEATURE_MMGSDI_GSM
LOCAL_CFLAGS += -DFEATURE_AUTH
LOCAL_CFLAGS += -DPACKED=

# Define for UIM files using QMI
LOCAL_CFLAGS += -DFEATURE_QCRIL_UIM_QMI
LOCAL_CFLAGS += -DFEATURE_QCRIL_UIM_QMI_SIMIO_ASYNC_CALL
LOCAL_CFLAGS += -DFEATURE_QCRIL_QMI_CAT

ifdef FEATURE_QCRIL_TOOLKIT_SKIP_SETUP_EVT_LIST_FILTER
LOCAL_CFLAGS += -DFEATURE_QCRIL_TOOLKIT_SKIP_SETUP_EVT_LIST_FILTER
endif

ifdef PROTOBUF_SUPPORTED
LOCAL_CFLAGS += -DQCRIL_PROTOBUF_BUILD_ENABLED
endif

ifeq ($(call is-board-platform,msm8960),true)
  LOCAL_CFLAGS += -DFEATURE_QCRIL_8960
endif

ifeq ($(call is-board-platform,msm7627a),true)
  LOCAL_CFLAGS += -DFEATURE_QCRIL_7627A
endif

ifeq ($(call is-board-platform,msm8974),true)
  LOCAL_CFLAGS += -DFEATURE_QCRIL_8974
endif

ifeq ($(call is-board-platform,msm8610),true)
  LOCAL_CFLAGS += -DFEATURE_QCRIL_8610
endif

ifeq ($(call is-board-platform,msm8226),true)
  LOCAL_CFLAGS += -DFEATURE_QCRIL_8226
endif

ifeq ($(call is-chipset-in-board-platform,msm7630),true)
  LOCAL_CFLAGS += -DFEATURE_QCRIL_LTE
  LOCAL_CFLAGS += -DFEATURE_QCRIL_IMS
endif

ifeq ($(call is-board-platform-in-list,msm7627a msm7627_surf msm7630_surf msm7630_fusion msm8660 msm8960 msm8974 msm8610 msm8226),true)
  LOCAL_CFLAGS += -DFEATURE_QCRIL_HDR_RELB
endif

ifeq ($(call is-board-platform-in-list,msm7630_surf msm7630_fusion msm8660 msm8960 msm8974 msm8610 msm8226 msm7627_6x),true)
  LOCAL_CFLAGS += -DFEATURE_QCRIL_NCELL
endif

ifeq ($(call is-board-platform-in-list,msm7627a msm7627_surf msm7630_surf msm7630_fusion msm7627_6x),true)
  LOCAL_CFLAGS += -DFEATURE_QCRIL_PRL_INIT
endif

ifeq ($(call is-board-platform,msm7630_fusion),true)
  LOCAL_CFLAGS += -DFEATURE_QCRIL_ADB_LOG_ENABLE
endif

ifeq ($(call is-board-platform,msm7627_6x),true)
  LOCAL_CFLAGS += -DFEATURE_QCRIL_SUBS_CTRL
endif



# defines common to all shared libraries
LOCAL_CFLAGS += \
  -DLOG_NDDEBUG=0 \
  -D__packed__= \
  -DIMAGE_APPS_PROC \
  -DFEATURE_Q_SINGLE_LINK \
  -DFEATURE_Q_NO_SELF_QPTR \
  -DFEATURE_NATIVELINUX \
  -DFEATURE_DSM_DUP_ITEMS \

# compiler options
LOCAL_CFLAGS += -g
LOCAL_CFLAGS += -O0
LOCAL_CFLAGS += -fno-inline
LOCAL_CFLAGS += -fno-short-enums

# Google defines
ifeq ($(TARGET_PRODUCT),sooner)
  LOCAL_CFLAGS += -DOMAP_CSMI_POWER_CONTROL -DUSE_TI_COMMANDS
endif

ifeq ($(call is-board-platform-in-list,msm7627_surf msm7627a msm7627_6x),true)
  LOCAL_CFLAGS += -DPOLL_CALL_STATE -DUSE_QMI
endif

ifeq ($(TARGET_PRODUCT),dream)
  LOCAL_CFLAGS += -DPOLL_CALL_STATE -DUSE_QMI
endif

LOCAL_LDLIBS += -lpthread
LOCAL_LDLIBS += -lrt

ifndef QCRIL_DSDA_INSTANCE
   LOCAL_MODULE:= libril-qc-qmi-1
else
   LOCAL_CFLAGS += -DFEATURE_DSDA_RIL_INSTANCE=$(QCRIL_DSDA_INSTANCE)
   LOCAL_MODULE:= libril-qc-qmi-$(QCRIL_DSDA_INSTANCE)
endif

LOCAL_MODULE_TAGS := debug

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

