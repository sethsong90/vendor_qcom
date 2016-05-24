LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO   := data/inc
LOCAL_COPY_HEADERS      := ../inc/dsi_netctrl.h
LOCAL_COPY_HEADERS      += ../inc/dsi_netctrl_qos.h

LOCAL_SRC_FILES += dsi_netctrl.c
LOCAL_SRC_FILES += dsi_netctrl_init.c
LOCAL_SRC_FILES += dsi_netctrli.c
LOCAL_SRC_FILES += dsi_netctrl_mni_cb.c
LOCAL_SRC_FILES += dsi_netctrl_mni.c
LOCAL_SRC_FILES += dsi_netctrl_multimodem.c
LOCAL_SRC_FILES += dsi_netctrl_netmgr.c
LOCAL_SRC_FILES += dsi_netctrl_platform.c
LOCAL_SRC_FILES += dsi_netctrl_cb_thrd.c
LOCAL_SRC_FILES += dsi_netctrl_qos.c
LOCAL_SRC_FILES += dsi_netctrl_embms.c

#LOCAL_CFLAGS := -DFEATURE_DSI_TEST

LOCAL_SHARED_LIBRARIES += libqmi
LOCAL_SHARED_LIBRARIES += libqdi
LOCAL_SHARED_LIBRARIES += libdiag
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libnetmgr
LOCAL_SHARED_LIBRARIES += libdiag
LOCAL_SHARED_LIBRARIES += libdsutils
LOCAL_SHARED_LIBRARIES += libdsnetutils

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../dsutils/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/diag/include
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/qmi/inc
#LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/oncrpc/inc/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../netmgr/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../qdi/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/diag/include
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/common/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../ds_netutils/inc

ifeq ($(call is-board-platform,msm7630_fusion),true)
LOCAL_CFLAGS += -DFEATURE_DSI_FUSION
endif
LOCAL_CFLAGS += -DFEATURE_QCRIL_USE_QDP
LOCAL_CFLAGS += -DFEATURE_DSI_MULTIMODEM_ROUTELOOKUP
LOCAL_CFLAGS += -DFEATURE_DATA_LOG_QXDM
LOCAL_CFLAGS += -DFEATURE_DNS_RESOLVER

LOCAL_MODULE := libdsi_netctrl

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
