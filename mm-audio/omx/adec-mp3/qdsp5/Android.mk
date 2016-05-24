ifeq ($(call is-board-platform-in-list,$(MSM7K_BOARD_PLATFORMS)),true)
ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
#                 Common definitons
# ---------------------------------------------------------------------------------

libOmxMp3Dec-def := -g -O3
libOmxMp3Dec-def += -DQC_MODIFIED
libOmxMp3Dec-def += -D_ANDROID_
libOmxMp3Dec-def += -D_ENABLE_QC_MSG_LOG_
libOmxMp3Dec-def += -DVERBOSE
libOmxMp3Dec-def += -D_DEBUG

ifeq ($(call is-chipset-in-board-platform,msm7630),true)
    AUDIO_V2 := true
endif
ifeq ($(AUDIO_V2), true)
libOmxMp3Dec-def += -DAUDIOV2
endif
ifeq ($(call is-board-platform,msm8610),true)
libOmxMp3Dec-def += -DQCOM_AUDIO_USE_SYSTEM_HEAP_ID
endif
# ---------------------------------------------------------------------------------
#             Make the Shared library (libOmxMp3Dec)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

libOmxMp3Dec-inc        := $(LOCAL_PATH)/inc
libOmxMp3Dec-inc        += $(TARGET_OUT_HEADERS)/mm-core/omxcore
libOmxMp3Dec-inc        += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_MODULE            := libOmxMp3Dec
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxMp3Dec-def)
LOCAL_C_INCLUDES        := $(libOmxMp3Dec-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libutils liblog

LOCAL_SRC_FILES         := src/adec_svr.c
LOCAL_SRC_FILES         += src/omx_mp3_adec.cpp

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------------
#             Make the apps-test (mm-adec-omxmp3-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

mm-mp3-dec-test-inc     := $(LOCAL_PATH)/inc
mm-mp3-dec-test-inc     += $(LOCAL_PATH)/test
mm-mp3-dec-test-inc     += $(AUDIO_ROOT)/audio-alsa/inc
mm-mp3-dec-test-inc     += $(TARGET_OUT_HEADERS)/mm-core/omxcore
mm-mp3-dec-test-inc     += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_MODULE            := mm-adec-omxmp3-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxMp3Dec-def)
LOCAL_C_INCLUDES        := $(mm-mp3-dec-test-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libmm-omxcore
LOCAL_SHARED_LIBRARIES  += libOmxMp3Dec
LOCAL_SHARED_LIBRARIES  += libaudioalsa

LOCAL_SRC_FILES         := test/omx_mp3_dec_test.c

include $(BUILD_EXECUTABLE)

endif #BUILD_TINY_ANDROID
endif #TARGET_BOARD_PLATFORM

# ---------------------------------------------------------------------------------
#                     END
# ---------------------------------------------------------------------------------

