ifeq ($(call is-board-platform-in-list,$(MSM7K_BOARD_PLATFORMS)),true)
ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
#                 Common definitons
# ---------------------------------------------------------------------------------

libOmxAacDec-def := -g -O3
libOmxAacDec-def += -DQC_MODIFIED
libOmxAacDec-def += -D_ANDROID_
libOmxAacDec-def += -D_ENABLE_QC_MSG_LOG_
libOmxAacDec-def += -DVERBOSE
libOmxAacDec-def += -D_DEBUG

ifeq ($(call is-chipset-in-board-platform,msm7630),true)
    AUDIO_V2 := true
endif
ifeq ($(AUDIO_V2), true)
libOmxAacDec-def += -DAUDIOV2
endif
# ---------------------------------------------------------------------------------
#             Make the Shared library (libOmxAacDec)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

libOmxAacDec-inc        := $(LOCAL_PATH)/inc
libOmxAacDec-inc        += $(AUDIO_OMX)/common/qdsp5/inc
libOmxAacDec-inc        += $(TARGET_OUT_HEADERS)/mm-core/omxcore
libOmxAacDec-inc        += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_MODULE            := libOmxAacDec
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxAacDec-def)
LOCAL_C_INCLUDES        := $(libOmxAacDec-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libutils liblog

LOCAL_SRC_FILES         := ../../common/qdsp5/src/omx_utils.c
LOCAL_SRC_FILES         += ../../common/qdsp5/src/omx_base.cpp
LOCAL_SRC_FILES         += ../../common/qdsp5/src/omx_base_dec.cpp
LOCAL_SRC_FILES         += ../../common/qdsp5/src/omx_base_trsc.cpp
LOCAL_SRC_FILES         += src/omx_dec_aac.cpp
LOCAL_SRC_FILES         += src/omx_trsc_aac.cpp

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------------
#             Make the apps-test (mm-adec-omxaac-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

mm-aac-dec-test-inc     += $(LOCAL_PATH)/inc
mm-aac-dec-test-inc     += $(LOCAL_PATH)/test
mm-aac-dec-test-inc     += $(AUDIO_ROOT)/audio-alsa/inc
mm-aac-dec-test-inc     += $(TARGET_OUT_HEADERS)/mm-core/omxcore
mm-aac-dec-test-inc     += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_MODULE            := mm-adec-omxaac-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxAacDec-def)
LOCAL_C_INCLUDES        := $(mm-aac-dec-test-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libmm-omxcore
LOCAL_SHARED_LIBRARIES  += libOmxAacDec
LOCAL_SHARED_LIBRARIES  += libaudioalsa

LOCAL_SRC_FILES         := test/omx_aac_dec_test.c

include $(BUILD_EXECUTABLE)

endif #BUILD_TINY_ANDROID
endif #TARGET_BOARD_PLATFORM

# ---------------------------------------------------------------------------------
#                     END
# ---------------------------------------------------------------------------------

