ifeq ($(call is-chipset-in-board-platform,msm7627a),true)
ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
#                 Common definitons
# ---------------------------------------------------------------------------------

libOmxAc3HwDec-def := -g -O3
libOmxAc3HwDec-def += -DQC_MODIFIED
libOmxAc3HwDec-def += -D_ANDROID_
libOmxAc3HwDec-def += -D_ENABLE_QC_MSG_LOG_
libOmxAc3HwDec-def += -DVERBOSE
libOmxAc3HwDec-def += -D_DEBUG

# ---------------------------------------------------------------------------------
#             Make the Shared library (libOmxAc3HwDec)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

libOmxAc3HwDec-inc       := $(LOCAL_PATH)/inc
libOmxAc3HwDec-inc        += $(AUDIO_OMX)/common/qdsp5/inc
libOmxAc3HwDec-inc       += $(TARGET_OUT_HEADERS)/mm-core/omxcore
libOmxAc3HwDec-inc       += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_MODULE            := libOmxAc3HwDec
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxAc3HwDec-def)
LOCAL_C_INCLUDES        := $(libOmxAc3HwDec-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libutils liblog

LOCAL_SRC_FILES         := ../../common/qdsp5/src/omx_utils.c
LOCAL_SRC_FILES         += ../../common/qdsp5/src/omx_base.cpp
LOCAL_SRC_FILES         += ../../common/qdsp5/src/omx_base_dec.cpp
LOCAL_SRC_FILES         += src/omx_ac3_adec.cpp

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------------
#             Make the apps-test (mm-adec-omxac3-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

mm-ac3-dec-test-hw-inc    := $(LOCAL_PATH)/inc
mm-ac3-dec-test-hw-inc    += $(LOCAL_PATH)/test
mm-ac3-dec-test-hw-inc    += $(AUDIO_ROOT)/audio-alsa/inc
mm-ac3-dec-test-hw-inc    += $(TARGET_OUT_HEADERS)/mm-core/omxcore

LOCAL_MODULE            := mm-adec-omxac3-hw-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxAc3HwDec-def)
LOCAL_C_INCLUDES        := $(mm-ac3-dec-test-hw-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libmm-omxcore
LOCAL_SHARED_LIBRARIES  += libOmxAc3HwDec
LOCAL_SHARED_LIBRARIES  += libaudioalsa

LOCAL_SRC_FILES         := test/omx_ac3_dec_test.c

include $(BUILD_EXECUTABLE)

endif #BUILD_TINY_ANDROID
endif # is-board-platform-in-list
# ---------------------------------------------------------------------------------
#                     END
# ---------------------------------------------------------------------------------

