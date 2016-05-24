#/******************************************************************************
#*@file Android.mk
#*brief Rules for copiling the source files
#*******************************************************************************/

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS      := optional
LOCAL_SRC_FILES        := $(call all-subdir-java-files)
LOCAL_PACKAGE_NAME     := QSSEPKCS11OtpGen
LOCAL_CERTIFICATE      := platform
LOCAL_SHARED_LIBRARIES := libPkiOtpGenerator
LOCAL_REQUIRED_MODULES := libPkiOtpGenerator

include $(BUILD_PACKAGE)
include $(call all-makefiles-under,$(LOCAL_PATH))
