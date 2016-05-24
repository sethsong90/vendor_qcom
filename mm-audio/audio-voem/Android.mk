
#/******************************************************************************
#*@file Android.mk
#*brief Rules for copiling the source files
#*******************************************************************************/

ifneq ($(MM_AUDIO_VOEM_DISABLED),true)
include $(call all-subdir-makefiles)
endif #ifneq ($(MM_AUDIO_VOEM_DISABLED),true)

