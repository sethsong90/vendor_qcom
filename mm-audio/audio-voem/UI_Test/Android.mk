
#/******************************************************************************
#*@file Android.mk
#*brief Rules for copiling the source files
#*******************************************************************************/

ifeq ($(call is-board-platform-in-list,msm7630_surf msm7627_surf msm8960 msm8974 msm8610),true)
ifneq ($(BUILD_TINY_ANDROID),true)

include $(call all-subdir-makefiles)

endif #BUILD_TINY_ANDROID
endif #is-board-platform-in-list
