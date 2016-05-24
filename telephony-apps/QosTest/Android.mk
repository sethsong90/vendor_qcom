#/******************************************************************************
#*@file Android.mk
#*brief Rules for copiling the source files
#*******************************************************************************/
ifeq ($(call is-vendor-board-platform,QCOM),true)
ifneq ($(call is-platform-sdk-version-at-least,17),true)
ifeq ($(call is-android-codename-in-list,ICECREAM_SANDWICH JELLY_BEAN),true)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-subdir-java-files)


LOCAL_PACKAGE_NAME := QosTest
LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)

# Copy the LDSTestApp.xml file to /data/
include $(CLEAR_VARS)
LOCAL_MODULE:= LDSTestApp.xml
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := assets/$(LOCAL_MODULE)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)
include $(BUILD_PREBUILT)

# Copy the QosTestConfig.xml file to /data/
include $(CLEAR_VARS)
LOCAL_MODULE:= QosTestConfig.xml
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := assets/$(LOCAL_MODULE)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)
include $(BUILD_PREBUILT)

endif
endif
endif
