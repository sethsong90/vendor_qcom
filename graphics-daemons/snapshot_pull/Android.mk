ifeq ($(call is-vendor-board-platform,QCOM),true)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)


LOCAL_C_INCLUDES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_SRC_FILES := snapshot_pull.c

LOCAL_SHARED_LIBRARIES := \
		libcutils

LOCAL_MODULE := gpu_snapshotd
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -DLOG_TAG='"gpu_snapshot"'
LOCAL_MODULE_OWNER := qcom
include $(BUILD_EXECUTABLE)
endif
