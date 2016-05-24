LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO   := data/inc
LOCAL_COPY_HEADERS      := ../inc/qc_getaddrinfo.h

LOCAL_SRC_FILES += qc_getaddrinfo.c
LOCAL_SRC_FILES += qc_strlcpy.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../../../bionic/libc/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../../../bionic/libc/private
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../../../bionic/libc/netbsd/resolv/
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/common/inc

LOCAL_CFLAGS += -DUSE_BIONIC
LOCAL_CFLAGS += -O0

LOCAL_MODULE := libdsnetutils

LOCAL_MODULE_TAGS := optional debug

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
