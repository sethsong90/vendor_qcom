ifeq ($(strip $(AUDIO_FEATURE_ENABLED_LISTEN)),true)
AUDCAL_ROOT := $(call my-dir)
include $(call all-subdir-makefiles)
endif
