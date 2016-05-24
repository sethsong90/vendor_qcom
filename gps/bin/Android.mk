ifneq ($(BUILD_TINY_ANDROID),true)
ifneq ($(QCA_1530_ANDROID_CONFIG),)

LOCAL_PATH := $(call my-dir)

ATLAS_BOARD_PLATFORM_LIST := msm7627a
ifeq ($(call is-board-platform-in-list,$(ATLAS_BOARD_PLATFORM_LIST)),true)

endif #is-board-platform-in-list

include $(LOCAL_PATH)/common/Android.mk $(LOCAL_PATH)/$(QCA_1530_ANDROID_CONFIG)/Android.mk

endif
endif # not BUILD_TINY_ANDROID

