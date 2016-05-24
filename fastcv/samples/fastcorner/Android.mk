#
#
# Makefile for FastCV Sample app as built from source tree.
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE:= false

LOCAL_MODULE    := libfastcvsample
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS    := -Werror
LOCAL_SRC_FILES := \
        jni/FastCVSample.cpp \
        jni/FastCVSampleRenderer.cpp \
        jni/FPSCounter.cpp \
        jni/CameraRendererRGB565GL2.cpp \
        jni/CameraUtil.cpp

LOCAL_SHARED_LIBRARIES := libdl liblog libGLESv2 libcutils libfastcvopt

LOCAL_C_INCLUDES += vendor/qcom/proprietary/fastcv/samples/fastcorner/jni
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/fastcv

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
LOCAL_JNI_SHARED_LIBRARIES  := libfastcvsample
LOCAL_SRC_FILES             := $(call all-subdir-java-files)
LOCAL_PACKAGE_NAME          := FastCVSample

include $(BUILD_PACKAGE)

