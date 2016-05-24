OMX_ADEC_EVRC := $(call my-dir)
ifneq ($(MM_AUDIO_OMX_ADEC_EVRC_DISABLED),true)
include $(call all-subdir-makefiles)
endif #ifneq ($(MM_AUDIO_OMX_ADEC_EVRC_DISABLED),true)
