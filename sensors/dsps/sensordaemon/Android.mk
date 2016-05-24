LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

commonIncludes := $(TARGET_OUT_HEADERS)/common/inc
commonIncludes += $(TARGET_OUT_HEADERS)/diag/include
commonIncludes += $(TARGET_OUT_HEADERS)/qmi-framework/inc
commonIncludes += $(TARGET_OUT_HEADERS)/qmi/inc

ifeq ($(call is-board-platform-in-list,msm8974 msm8226 apq8084 msm8962),true)
LOCAL_CFLAGS += -DSNS_QMI_ENABLE
endif

LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_C_INCLUDES += $(shell find $(LOCAL_PATH) -type d -name 'inc' -print )
LOCAL_C_INCLUDES += $(shell find $(LOCAL_PATH) -type d -name 'src' -print )
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../api
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libsensor1/inc

# Figure out if this build system is a QCOM build. If so, include
# the path to the sanitized headers in the target "out" dir.
ifeq ($(call is-vendor-board-platform,QCOM),true)
LOCAL_CFLAGS     += -include bionic/libc/kernel/arch-arm/asm/posix_types.h
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
endif

LOCAL_MODULE:=sensors.qcom
LOCAL_MODULE_OWNER := qcom

LOCAL_SHARED_LIBRARIES += libdiag
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libdl
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libqmi_common_so
LOCAL_SHARED_LIBRARIES += libqmi_cci
LOCAL_SHARED_LIBRARIES += libqmi_encdec
LOCAL_SHARED_LIBRARIES += libqmi_csi

LOCAL_STATIC_LIBRARIES += libsensors_lib

LOCAL_SRC_FILES += \
	$(shell find $(LOCAL_PATH)/* -name '*.c' | grep  'src/.*\.c' | sed s:^$(LOCAL_PATH)/::g )

LOCAL_CFLAGS += -D_GNU_SOURCE -Wall -Wno-missing-field-initializers -Werror -DSNS_LA -std=c99
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

