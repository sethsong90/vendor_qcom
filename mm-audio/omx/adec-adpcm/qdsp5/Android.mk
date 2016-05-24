ifeq ($(call is-board-platform-in-list,$(MSM7K_BOARD_PLATFORMS)),true)
ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
#                 Common definitons
# ---------------------------------------------------------------------------------

libOmxAdpcmDec-def := -g -O3
libOmxAdpcmDec-def += -DQC_MODIFIED
libOmxAdpcmDec-def += -D_ANDROID_
libOmxAdpcmDec-def += -D_ENABLE_QC_MSG_LOG_
libOmxAdpcmDec-def += -DVERBOSE -D_DEBUG
libOmxAdpcmDec-def += -D_DEBUG

ifeq ($(call is-chipset-in-board-platform,msm7630),true)
    AUDIO_V2 := true
endif
ifeq ($(AUDIO_V2), true)
libOmxAdpcmDec-def += -DAUDIOV2
endif #AUDIO_V2

# ---------------------------------------------------------------------------------
#             Make the Shared library (libOmxAdpcmDec)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

libOmxAdpcmDec-inc      := $(LOCAL_PATH)/inc
libOmxAdpcmDec-inc      += $(TARGET_OUT_HEADERS)/mm-core/omxcore
libOmxAdpcmDec-inc      += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_MODULE            := libOmxAdpcmDec
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxAdpcmDec-def)
LOCAL_C_INCLUDES        := $(libOmxAdpcmDec-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libutils liblog

LOCAL_SRC_FILES         := src/adec_svr.c
LOCAL_SRC_FILES         += src/omx_adpcm_adec.cpp

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------------
#             Make the apps-test (mm-adec-omxadpcm-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

mm-adpcm-dec-test-inc   := $(LOCAL_PATH)/inc
mm-adpcm-dec-test-inc   += $(LOCAL_PATH)/test
mm-adpcm-dec-test-inc   += $(AUDIO_ROOT)/audio-alsa/inc
mm-adpcm-dec-test-inc   += $(TARGET_OUT_HEADERS)/mm-core/omxcore
mm-adpcm-dec-test-inc   += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_MODULE            := mm-adec-omxadpcm-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxAdpcmDec-def)
LOCAL_C_INCLUDES        := $(mm-adpcm-dec-test-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libmm-omxcore
LOCAL_SHARED_LIBRARIES  += libOmxAdpcmDec
LOCAL_SHARED_LIBRARIES  += libaudioalsa

LOCAL_SRC_FILES         := test/omx_adpcm_dec_test.c

include $(BUILD_EXECUTABLE)

endif #BUILD_TINY_ANDROID
endif #TARGET_BOARD_PLATFORM
# ---------------------------------------------------------------------------------
#                     END
# ---------------------------------------------------------------------------------

