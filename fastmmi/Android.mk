ifeq ($(call is-board-platform-in-list,msm8226 msm8610),true)
MMI_ROOT := $(call my-dir)
include $(call all-subdir-makefiles)
endif
