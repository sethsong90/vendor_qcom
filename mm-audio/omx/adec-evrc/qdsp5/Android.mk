ifeq ($(call is-chipset-prefix-in-board-platform,msm7627),true)
ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
#                 Common definitons
# ---------------------------------------------------------------------------------

libOmxEvrcHwDec-def := -g -O3
libOmxEvrcHwDec-def += -DQC_MODIFIED
libOmxEvrcHwDec-def += -D_ANDROID_
libOmxEvrcHwDec-def += -D_ENABLE_QC_MSG_LOG_
libOmxEvrcHwDec-def += -DVERBOSE
libOmxEvrcHwDec-def += -D_DEBUG

ifeq ($(call is-chipset-in-board-platform,msm7630),true)
    AUDIO_V2 := true
endif
ifeq ($(AUDIO_V2), true)
libOmxEvrcHwDec-def += -DAUDIOV2
endif

# ---------------------------------------------------------------------------------
#             Make the Shared library (libOmxEvrcHwDec)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

libOmxEvrcHwDec-inc       := $(LOCAL_PATH)/inc
libOmxEvrcHwDec-inc       += $(TARGET_OUT_HEADERS)/mm-core/omxcore
libOmxEvrcHwDec-inc       += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_MODULE            := libOmxEvrcHwDec
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxEvrcHwDec-def)
LOCAL_C_INCLUDES        := $(libOmxEvrcHwDec-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libutils liblog

LOCAL_SRC_FILES         := src/adec_svr.c
LOCAL_SRC_FILES         += src/omx_evrc_adec.cpp

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------------
#             Make the apps-test (mm-adec-omxevrc-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

mm-evrc-dec-test-hw-inc    := $(LOCAL_PATH)/inc
mm-evrc-dec-test-hw-inc    += $(LOCAL_PATH)/test
mm-evrc-dec-test-hw-inc    += $(AUDIO_ROOT)/audio-alsa/inc
mm-evrc-dec-test-hw-inc    += $(TARGET_OUT_HEADERS)/mm-core/omxcore

LOCAL_MODULE            := mm-adec-omxevrc-hw-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxEvrcHwDec-def)
LOCAL_C_INCLUDES        := $(mm-evrc-dec-test-hw-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libmm-omxcore
LOCAL_SHARED_LIBRARIES  += libOmxEvrcHwDec
LOCAL_SHARED_LIBRARIES  += libaudioalsa

LOCAL_SRC_FILES         := test/omx_evrc_dec_test.c

include $(BUILD_EXECUTABLE)

endif #BUILD_TINY_ANDROID
endif # is-board-platform-in-list
# ---------------------------------------------------------------------------------
#                     END
# ---------------------------------------------------------------------------------

