ifeq ($(strip $(AUDIO_FEATURE_ENABLED_LISTEN)),true)

LOCAL_PATH := $(call my-dir)

#=============================================
#  Listen API lib
#=============================================

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := Listen

include $(BUILD_STATIC_JAVA_LIBRARY)

endif
