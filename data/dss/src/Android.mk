# sources and intermediate files are separated

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO   := data/inc
LOCAL_COPY_HEADERS      := dsc_call.h
LOCAL_COPY_HEADERS      += dsc_cmd.h
LOCAL_COPY_HEADERS      += dsc_dcmi.h
LOCAL_COPY_HEADERS      += dsc_kif.h
LOCAL_COPY_HEADERS      += dsc_main.h
LOCAL_COPY_HEADERS      += dsc_qmi_nasi.h
LOCAL_COPY_HEADERS      += dsc_qmi_wds.h
LOCAL_COPY_HEADERS      += dsc_test.h
LOCAL_COPY_HEADERS      += dsc_util.h

LOCAL_COPY_HEADERS      += ../inc/dsc_dcm_api.h
LOCAL_COPY_HEADERS      += ../inc/dsc_dcm.h
LOCAL_COPY_HEADERS      += ../inc/dsci.h
LOCAL_COPY_HEADERS      += ../inc/dserrno.h
LOCAL_COPY_HEADERS      += ../inc/ds_fd_pass.h
LOCAL_COPY_HEADERS      += ../inc/ds_linux.h
LOCAL_COPY_HEADERS      += ../inc/dss_netpolicy.h
LOCAL_COPY_HEADERS      += ../inc/dssocket_common.h
LOCAL_COPY_HEADERS      += ../inc/dssocket_defs_linux.h
LOCAL_COPY_HEADERS      += ../inc/dssocket.h

LOCAL_CFLAGS += -DFEATURE_DS_LINUX_NO_RPC
LOCAL_CFLAGS += -DFEATURE_DSS_LINUX_ANDROID

# Logging Features. Turn any one ON at any time  
#LOCAL_CFLAGS += -DFEATURE_DATA_LOG_SYSLOG
#LOCAL_CFLAGS += -DFEATURE_DATA_LOG_ADB
LOCAL_CFLAGS += -DFEATURE_DATA_LOG_QXDM

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc/
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/data/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/common/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/qmi/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/diag/include

ifeq ($(call is-android-codename-in-list,GINGERBREAD HONEYCOMB),true)
LOCAL_C_INCLUDES += system/core/libnetutils
LOCAL_CFLAGS += -DFEATURE_GB_NET_UTILS
else
LOCAL_C_INCLUDES += system/core/include/netutils
endif

LOCAL_SRC_FILES := ds_fd_pass.c
LOCAL_SRC_FILES += ds_socket.c
LOCAL_SRC_FILES += dsc_cmd.c
LOCAL_SRC_FILES += dsc_dcm.c
LOCAL_SRC_FILES += dsc_kif.c
LOCAL_SRC_FILES += dsc_main.c
LOCAL_SRC_FILES += dsc_qmi_wds.c
LOCAL_SRC_FILES += dsc_call.c
LOCAL_SRC_FILES += dsc_util.c

LOCAL_LDLIBS += -lpthread

LOCAL_MODULE := libdss
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libqmi liblog libdiag libnetutils libdsutils libcutils

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
