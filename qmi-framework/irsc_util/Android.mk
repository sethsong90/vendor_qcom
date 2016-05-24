LOCAL_PATH := $(call my-dir)

commonIncludes := $(LOCAL_PATH)/../inc

include $(CLEAR_VARS)
LOCAL_MODULE := irsc_util
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES  := irsc_util.c
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

