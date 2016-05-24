ifeq ($(call is-board-platform-in-list,$(MSM7K_BOARD_PLATFORMS)),true)
ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
#                 Common definitons
# ---------------------------------------------------------------------------------

libOmxAmrDec-def := -g -O3
libOmxAmrDec-def += -DQC_MODIFIED
libOmxAmrDec-def += -D_ANDROID_
libOmxAmrDec-def += -D_ENABLE_QC_MSG_LOG_
libOmxAmrDec-def += -DVERBOSE 
libOmxAmrDec-def += -D_DEBUG

ifeq ($(call is-chipset-in-board-platform,msm7630),true)
    AUDIO_V2 := true
endif
ifeq ($(AUDIO_V2), true)
libOmxAmrDec-def += -DAUDIOV2
endif

# ---------------------------------------------------------------------------------
#             Make the Shared library (libOmxAmrRtpDec)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

libOmxAmrRtpDec-inc     := $(LOCAL_PATH)/inc
libOmxAmrRtpDec-inc     += $(LOCAL_PATH)/rtp/inc
libOmxAmrRtpDec-inc     += $(TARGET_OUT_HEADERS)/mm-core/omxcore
libOmxAmrRtpDec-inc     += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_MODULE            := libOmxAmrRtpDec
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxAmrDec-def)
LOCAL_C_INCLUDES        := $(libOmxAmrRtpDec-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libutils liblog

LOCAL_SRC_FILES         := rtp/src/bit.c
LOCAL_SRC_FILES         += rtp/src/rtp_amr_profile.c
LOCAL_SRC_FILES         += rtp/src/rtp_packet.c
LOCAL_SRC_FILES         += rtp/src/rtp_api.c

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------------
#             Make the Shared library (libOmxAmrDec)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

libOmxAmrDec-inc        := $(LOCAL_PATH)/inc
libOmxAmrDec-inc        += $(LOCAL_PATH)/rtp/inc
libOmxAmrDec-inc        += $(TARGET_OUT_HEADERS)/mm-core/omxcore
libOmxAmrDec-inc        += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_MODULE            := libOmxAmrDec
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxAmrDec-def)
LOCAL_C_INCLUDES        := $(libOmxAmrDec-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libutils liblog libOmxAmrRtpDec

LOCAL_SRC_FILES         := src/adec_svr.c
LOCAL_SRC_FILES         += src/omx_amr_adec.cpp

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------------
#             Make the apps-test (mm-adec-omxamr-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

mm-amr-dec-test-inc     := $(LOCAL_PATH)/test
mm-amr-dec-test-inc     += $(LOCAL_PATH)/inc
mm-amr-dec-test-inc     += $(TARGET_OUT_HEADERS)/mm-core/omxcore
mm-amr-dec-test-inc     += $(AUDIO_ROOT)/audio-alsa/inc
mm-amr-dec-test-inc     += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_MODULE            := mm-adec-omxamr-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxAmrDec-def)
LOCAL_C_INCLUDES        := $(mm-amr-dec-test-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libmm-omxcore
LOCAL_SHARED_LIBRARIES  += libOmxAmrRtpDec
LOCAL_SHARED_LIBRARIES  += libOmxAmrDec
LOCAL_SHARED_LIBRARIES  += libaudioalsa

LOCAL_SRC_FILES         := test/omx_amr_dec_test.c

include $(BUILD_EXECUTABLE)

endif #BUILD_TINY_ANDROID
endif #TARGET_BOARD_PLATFORM

# ---------------------------------------------------------------------------------
#                     END
# ---------------------------------------------------------------------------------

