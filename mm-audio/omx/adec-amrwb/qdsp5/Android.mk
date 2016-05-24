ifeq ($(call is-board-platform-in-list,$(MSM7K_BOARD_PLATFORMS)),true)
ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
# 				Common definitons
# ---------------------------------------------------------------------------------

libOmxAmrwbDec-def := -g -O3
libOmxAmrwbDec-def += -DQC_MODIFIED
libOmxAmrwbDec-def += -D_ANDROID_
libOmxAmrwbDec-def += -D_ENABLE_QC_MSG_LOG_
libOmxAmrwbDec-def += -DVERBOSE
libOmxAmrwbDec-def += -D_DEBUG

ifeq ($(call is-chipset-in-board-platform,msm7630),true)
    AUDIO_V2 := true
endif
ifeq ($(AUDIO_V2), true)
libOmxAmrwbDec-def += -DAUDIOV2
endif
# ---------------------------------------------------------------------------------
# 			Make the Shared library (libOmxAmrwbDec)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

libOmxAmrwbDec-inc      := $(LOCAL_PATH)/inc
libOmxAmrwbDec-inc      += $(TARGET_OUT_HEADERS)/mm-core/omxcore
libOmxAmrwbDec-inc      += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_MODULE            := libOmxAmrwbDec
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxAmrwbDec-def)
LOCAL_C_INCLUDES        := $(libOmxAmrwbDec-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libutils liblog
LOCAL_SRC_FILES         := src/adec_svr.c
LOCAL_SRC_FILES         += src/omx_amrwb_adec.cpp

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------------
# 			Make the apps-test (mm-adec-omxamrwb-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

mm-amrwb-dec-test-inc     := $(LOCAL_PATH)/test
mm-amrwb-dec-test-inc     += $(LOCAL_PATH)/inc
mm-amrwb-dec-test-inc     += $(AUDIO_ROOT)/audio-alsa/inc
mm-amrwb-dec-test-inc     += $(TARGET_OUT_HEADERS)/mm-core/omxcore
mm-amrwb-dec-test-inc     += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_MODULE            := mm-adec-omxamrwb-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxAmrwbDec-def)
LOCAL_C_INCLUDES        := $(mm-amrwb-dec-test-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libmm-omxcore
LOCAL_SHARED_LIBRARIES  += libOmxAmrwbDec
LOCAL_SHARED_LIBRARIES  += libaudioalsa

LOCAL_SRC_FILES         := test/omx_amrwb_dec_test.c

include $(BUILD_EXECUTABLE)

endif #BUILD_TINY_ANDROID
endif #TARGET_BOARD_PLATFORM
# ---------------------------------------------------------------------------------
# 					END
# ---------------------------------------------------------------------------------

