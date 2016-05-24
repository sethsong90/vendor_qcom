ifneq ($(call is-platform-sdk-version-at-least,18),true)
QRDPLUS_DIR := $(call my-dir)

include $(call all-subdir-makefiles)

LOCAL_PATH := $(QRDPLUS_DIR)

include $(CLEAR_VARS)
endif

