ifeq ($(call is-vendor-board-platform,QCOM),true)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := qcom-system-daemon
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
		    $(TARGET_OUT_HEADERS)/common/inc/ \
		    $(TARGET_OUT_HEADERS)/diag/src \
		    $(TARGET_OUT_HEADERS)/diag/include \
		    $(TARGET_OUT_HEADERS)/subsystem_control/inc
LOCAL_ADDITIONAL_DEPENDENCIES += ${TARGET_OUT_INTERMEDIATES}/KERNEL_OBJ/usr
LOCAL_SRC_FILES := qcom_system_daemon.c
LOCAL_SHARED_LIBRARIES := libc libcutils libutils libdiag libsubsystem_control
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -Wall
include $(BUILD_EXECUTABLE)
endif
