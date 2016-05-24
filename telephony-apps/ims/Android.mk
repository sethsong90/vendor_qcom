#/******************************************************************************
#*@file Android.mk
#*brief Rules for compiling the source files
#*******************************************************************************/
src_java := src/org/codeaurora/ims
src_proto := src
src_csvt  := src/com/qualcomm/ims/csvt

LOCAL_PATH:= $(call my-dir)

# Build the auto generated files into a library to be used by both the
# app and the service
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_CLASS := JAVA_LIBRARIES

LOCAL_MODULE := imslibrary
LOCAL_SRC_FILES += src/org/codeaurora/ims/IImsService.aidl
LOCAL_SRC_FILES += src/org/codeaurora/ims/IImsServiceListener.aidl
LOCAL_PROGUARD_ENABLED := disabled

#include $(BUILD_STATIC_JAVA_LIBRARY)
include $(BUILD_JAVA_LIBRARY)
# ==========================================================================
# Build the service
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_PACKAGE_NAME := ims
LOCAL_CERTIFICATE := platform
LOCAL_PROGUARD_ENABLED := disabled

LOCAL_JAVA_LIBRARIES := imslibrary core framework telephony-common

LOCAL_PROTOC_OPTIMIZE_TYPE := micro

LOCAL_SRC_FILES := $(call all-java-files-under, $(src_java)) \
    $(call all-proto-files-under, $(src_proto)) \
    $(call all-java-files-under, $(src_csvt))

include $(BUILD_PACKAGE)

# ==========================================================================

include $(CLEAR_VARS)

LOCAL_MODULE := ims.xml

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := ETC
LOCAL_PROGUARD_ENABLED := disabled

# This will install the file in /system/etc/permissions

LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions

LOCAL_SRC_FILES := $(LOCAL_MODULE)

include $(BUILD_PREBUILT)
