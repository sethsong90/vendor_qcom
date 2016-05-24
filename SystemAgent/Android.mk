#
# Copyright (c) 2012, Qualcomm Technologies, Inc.
# All Rights Reserved.
# Qualcomm Technologies Proprietary and Confidential.
# Developed by QRD Engineering team.
#

LOCAL_PATH:= $(call my-dir)

ifeq ($(TARGET_USES_LOGKIT_LOGGING),true)
include $(CLEAR_VARS)

LOCAL_JAVA_LIBRARIES := telephony-common telephony-msim

LOCAL_MODULE_TAGS := optional eng

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_PACKAGE_NAME := SystemAgent
LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
endif
