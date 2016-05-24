ifeq ($(call is-vendor-board-platform,QCOM),true)
ifeq ($(findstring true, $(BOARD_HAVE_QCOM_FM) $(BOARD_HAVE_BLUETOOTH)),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(TARGET_OUT_HEADERS)/common/inc

LOCAL_CFLAGS := -DANDROID

LOCAL_SRC_FILES := wds_main.c
LOCAL_SRC_FILES += wds_hci_pfal_linux.c

LOCAL_LDLIBS := -lpthread
LOCAL_MODULE := wdsdaemon
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libdiag
LOCAL_SHARED_LIBRARIES += libcutils

include $(BUILD_EXECUTABLE)
endif # filter
endif # is-vendor-board-platform
