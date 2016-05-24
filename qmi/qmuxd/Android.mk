LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS += -DFEATURE_QMI_ANDROID
LOCAL_CFLAGS += -DFEATURE_QMI_IWLAN

# Logging Features. Turn any one ON at any time  

#LOCAL_CFLAGS += -DFEATURE_DATA_LOG_STDERR
#LOCAL_CFLAGS += -DFEATURE_DATA_LOG_ADB
LOCAL_CFLAGS += -DFEATURE_DATA_LOG_QXDM

ifeq ($(call is-board-platform,msm7630_fusion),true)
LOCAL_CFLAGS += -DFEATURE_QMI_FUSION
LOCAL_CFLAGS += -DFEATURE_WAIT_FOR_MODEM_HACK=0
endif

ifeq ($(call is-board-platform-in-list,msm8660 msm8960),true)
LOCAL_CFLAGS += -DFEATURE_WAIT_FOR_MODEM_HACK=0
endif

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../src
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../platform
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../core/lib/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/common/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/diag/include
LOCAL_C_INCLUDES += system/core/include/cutils




LOCAL_SRC_FILES:=../platform/linux_qmi_qmux_if_server.c
LOCAL_SRC_FILES += ../platform/qmi_platform_qmux_io.c
LOCAL_SRC_FILES += ../platform/qmi_platform.c
LOCAL_SRC_FILES += ../src/qmi_qmux.c
LOCAL_SRC_FILES += ../src/qmi_util.c

LOCAL_MODULE:= qmuxd

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := liblog libdiag
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libqmi

LDLIBS += -lpthread

include $(BUILD_EXECUTABLE)
