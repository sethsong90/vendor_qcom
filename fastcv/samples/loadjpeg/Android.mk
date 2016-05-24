#
#
# Makefile for FastCV Sample app as built from source tree.
#
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE:= false

LOCAL_MODULE    := libloadjpeg
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS    := -Werror
LOCAL_SRC_FILES := \
    jni/loadjpeg.cpp \

LOCAL_SHARED_LIBRARIES := libdl liblog libGLESv2 libfastcvopt

LOCAL_C_INCLUDES += vendor/qcom/proprietary/fastcv/samples/loadjpeg/jni
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/fastcv/inc

# Source tree build defaults to using neon, here we need to shut it
# off for cross-device compatibility.
LOCAL_CFLAGS := \
   -O3 -fpic -fno-exceptions -fno-short-enums -fsigned-char \
   -march=armv6 -mtune=arm1136jf-s \
   -mfloat-abi=softfp -mfpu=vfp -fno-math-errno -fno-signed-zeros \
   -fno-tree-vectorize

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

#
# Build sample application package
#
include $(CLEAR_VARS)

LOCAL_STATIC_JAVA_LIBRARIES    :=
LOCAL_JNI_SHARED_LIBRARIES  := libloadjpeg
LOCAL_SRC_FILES             := $(call all-subdir-java-files)
LOCAL_PACKAGE_NAME          := LoadJpeg

include $(BUILD_PACKAGE)
