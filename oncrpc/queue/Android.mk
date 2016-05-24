LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../oncrpc_defines.mk

common_libqueue_cflags :=  
common_libqueue_cflags += -g
common_libqueue_cflags += -O0
common_libqueue_cflags += -fno-inline
common_libqueue_cflags += -fno-short-enums
common_libqueue_cflags += $(oncrpc_common_defines)	

libqueue_includes := $(LOCAL_PATH)/../inc
libqueue_includes += $(TARGET_OUT_HEADERS)/common/inc

LOCAL_CFLAGS := $(common_libqueue_cflags)

LOCAL_C_INCLUDES := $(libqueue_includes)

LOCAL_SRC_FILES := queue.c

LOCAL_LDLIBS += -lpthread  

LOCAL_MODULE:= libqueue

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)


