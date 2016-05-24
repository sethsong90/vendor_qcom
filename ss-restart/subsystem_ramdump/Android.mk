LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := subsystem_ramdump.c

LOCAL_MODULE := subsystem_ramdump

LOCAL_MODULE_TAGS := optional

ifeq ($(call is-vendor-board-platform,QCOM),true)
  LOCAL_C_INCLUDES := $(TARGET_OUT_HEADERS)/common/inc
endif

LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)
