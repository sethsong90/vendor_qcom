LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

commonSources  :=
commonIncludes := $(TARGET_OUT_HEADERS)/common/inc

ifeq ($(call is-board-platform-in-list,msm8974 msm8226 apq8084 msm8962),true)
LOCAL_SHARED_LIBRARIES += libqmi_encdec
commonIncludes += $(TARGET_OUT_HEADERS)/qmi-framework/inc

LOCAL_CFLAGS += -DSNS_QMI_ENABLE
else
LOCAL_SHARED_LIBRARIES += libidl
commonIncludes += $(TARGET_OUT_HEADERS)/qmi/inc
endif

LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../api

LOCAL_MODULE:=libsensor1
#LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)

LOCAL_SHARED_LIBRARIES += liblog

LOCAL_SRC_FILES += src/libsensor1.c

#include IDL files
IDL_H_FILES := $(shell find $(LOCAL_PATH)/../api -type f -name '*_v[0-9][0-9].h' -print )
IDL_FILES := $(notdir $(IDL_H_FILES))
IDL_SRC_FILES := $(IDL_FILES:.h=.c)

LOCAL_SRC_FILES += $(addprefix ../sensordaemon/common/idl/src/,$(IDL_SRC_FILES))

LOCAL_CFLAGS += -D_GNU_SOURCE -Wno-missing-field-initializers -Werror -DSNS_LA

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)


