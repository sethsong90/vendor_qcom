#
#
# Makefile for FastCV Sample app as built from source tree.
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE:= false

LOCAL_MODULE    := libfastcvUtils
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS    := -Werror
LOCAL_SRC_FILES := \
  jni/utils/FPSCounter.cpp \
    jni/utils/CameraRendererRGB565GL2.cpp \
    jni/utils/CameraUtil.cpp \
    jni/utils/FastCVSampleRenderer.cpp \
    jni/utils/FastCVUtil.cpp

LOCAL_SHARED_LIBRARIES := libdl liblog libGLESv2 libcutils libfastcvopt

LOCAL_C_INCLUDES += vendor/qcom/proprietary/fastcv/samples/fastcvdemo/jni/utils
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/fastcv

# Source tree build defaults to using neon, here we need to shut it
# off for cross-device compatibility.
LOCAL_CFLAGS := \
   -O3 -fpic -fno-exceptions -fno-short-enums -fsigned-char \
   -march=armv6 -mtune=arm1136jf-s \
   -mfloat-abi=softfp -mfpu=vfp -fno-math-errno -fno-signed-zeros \
   -fno-tree-vectorize

LOCAL_MODULE_OWNER := qcom
include $(BUILD_STATIC_LIBRARY)

############################################################

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE:= false

LOCAL_MODULE    := libfastcvFeatDetect
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS    := -Werror
LOCAL_SRC_FILES := jni/apis/featureDetection/Corner.cpp

LOCAL_STATIC_LIBRARIES := libfastcvUtils
LOCAL_SHARED_LIBRARIES := libdl liblog libGLESv2 libcutils libfastcvopt

LOCAL_C_INCLUDES += vendor/qcom/proprietary/fastcv/samples/fastcvdemo/jni
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

############################################################

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE:= false

LOCAL_MODULE    := libfastcvImgProc
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS    := -Werror
LOCAL_SRC_FILES := jni/apis/imageProcessing/Filter.cpp \
                   jni/apis/imageProcessing/ImgDiff.cpp \
                   jni/apis/imageProcessing/ImageGradient.cpp \
                   jni/apis/imageProcessing/FilterCorr.cpp

LOCAL_STATIC_LIBRARIES := libfastcvUtils
LOCAL_SHARED_LIBRARIES := libdl liblog libGLESv2 libcutils libfastcvopt

LOCAL_C_INCLUDES += vendor/qcom/proprietary/fastcv/samples/fastcvdemo/jni
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

############################################################

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE:= false

LOCAL_MODULE    := libfastcvImgTransform
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS    := -Werror
LOCAL_SRC_FILES := jni/apis/imageTransformation/Scale.cpp \
                   jni/apis/imageTransformation/Warp.cpp \
                   jni/apis/imageTransformation/Affine.cpp \
                   jni/apis/imageTransformation/Rotate.cpp \
                   jni/apis/imageTransformation/ScaleDownMN.cpp

LOCAL_STATIC_LIBRARIES := libfastcvUtils
LOCAL_SHARED_LIBRARIES := libdl liblog libGLESv2 libcutils libfastcvopt

LOCAL_C_INCLUDES += vendor/qcom/proprietary/fastcv/samples/fastcvdemo/jni
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
LOCAL_JNI_SHARED_LIBRARIES  := libfastcvFeatDetect libfastcvImgProc libfastcvImgTransform
LOCAL_SRC_FILES             := $(call all-subdir-java-files)
LOCAL_PACKAGE_NAME          := FastCVDemo

include $(BUILD_PACKAGE)

