ifeq ($(TARGET_ARCH), arm)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := kgsl_test
LOCAL_SRC_FILES += $(commonSources) kgsl_test.c kgsl_helper.c kgsl_test_adv.c \
		kgsl_test_small.c kgsl_test_stress.c kgsl_test_nominal.c \
		kgsl_test_repeat.c
LOCAL_CFLAGS = -Wall
LOCAL_C_INCLUDES  = $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include/
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_SHARED_LIBRARIES := \
		    libc \
		    libcutils \
		    libutils \

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/kernel-tests
LOCAL_MODULE_OWNER := qcom
include $(BUILD_EXECUTABLE)

endif
