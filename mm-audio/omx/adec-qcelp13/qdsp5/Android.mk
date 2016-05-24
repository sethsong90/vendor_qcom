ifeq ($(call is-board-platform-in-list,$(MSM7K_BOARD_PLATFORMS)),true)
ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
#                 Common definitons
# ---------------------------------------------------------------------------------

libOmxQcelpHwDec-def := -g -O3
libOmxQcelpHwDec-def += -DQC_MODIFIED
libOmxQcelpHwDec-def += -D_ANDROID_
libOmxQcelpHwDec-def += -D_ENABLE_QC_MSG_LOG_
libOmxQcelpHwDec-def += -DVERBOSE -D_DEBUG

ifeq ($(call is-chipset-in-board-platform,msm7630),true)
    AUDIO_V2 := true
endif
ifeq ($(AUDIO_V2), true)
libOmxQcelpHwDec-def += -DAUDIOV2
endif

# ---------------------------------------------------------------------------------
#             Make the Shared library (libOmxQcelpHwDec)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

libOmxQcelpHwDec-inc      := $(LOCAL_PATH)/inc
libOmxQcelpHwDec-inc      += $(TARGET_OUT_HEADERS)/mm-core/omxcore
libOmxQcelpHwDec-inc      += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_MODULE            := libOmxQcelpHwDec
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxQcelpHwDec-def)
LOCAL_C_INCLUDES        := $(libOmxQcelpHwDec-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libutils liblog

LOCAL_SRC_FILES         := src/adec_svr.c
LOCAL_SRC_FILES         += src/omx_Qcelp13_adec.cpp

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------------
#             Make the apps-test (mm-adec-omxQcelp13-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

mm-qcelp-dec-test-hw-inc   := $(LOCAL_PATH)/inc
mm-qcelp-dec-test-hw-inc   += $(LOCAL_PATH)/test
mm-qcelp-dec-test-hw-inc   += $(AUDIO_ROOT)/audio-alsa/inc
mm-qcelp-dec-test-hw-inc   += $(TARGET_OUT_HEADERS)/mm-core/omxcore
mm-qcelp-dec-test-hw-inc   += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_MODULE            := mm-adec-omxQcelpHw-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxQcelpHwDec-def)
LOCAL_C_INCLUDES        := $(mm-qcelp-dec-test-hw-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libmm-omxcore
LOCAL_SHARED_LIBRARIES  += libOmxQcelpHwDec
LOCAL_SHARED_LIBRARIES  += libaudioalsa

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_SRC_FILES         := test/omx_Qcelp13_dec_test.c

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

mm-qcelp-dec-vam-test-inc   := $(LOCAL_PATH)/inc
mm-qcelp-dec-vam-test-inc   += $(LOCAL_PATH)/test
mm-qcelp-dec-vam-test-inc   += $(AUDIO_ROOT)/audio-alsa/inc
mm-qcelp-dec-vam-test-inc   += $(TARGET_OUT_HEADERS)/mm-core/omxcore
mm-qcelp-dec-vam-test-inc   += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_MODULE            := mm-adec-omxvam-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxQcelpHwDec-def)
LOCAL_C_INCLUDES        := $(mm-qcelp-dec-vam-test-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libmm-omxcore
LOCAL_SHARED_LIBRARIES  += libOmxQcelpHwDec
LOCAL_SHARED_LIBRARIES  += libaudioalsa

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_SRC_FILES         := test/omx_vam_test.c

include $(BUILD_EXECUTABLE)

endif #BUILD_TINY_ANDROID
endif # is-board-platform-in-list
# ---------------------------------------------------------------------------------
#                     END
# ---------------------------------------------------------------------------------

