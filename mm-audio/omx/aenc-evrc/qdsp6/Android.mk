ifeq ($(call is-board-platform-in-list,$(QSD8K_BOARD_PLATFORMS)),true)
ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
#                 Common definitons
# ---------------------------------------------------------------------------------

libOmxEvrcEnc-def := -g -O3
libOmxEvrcEnc-def += -DQC_MODIFIED
libOmxEvrcEnc-def += -D_ANDROID_
libOmxEvrcEnc-def += -D_ENABLE_QC_MSG_LOG_
libOmxEvrcEnc-def += -DVERBOSE
libOmxEvrcEnc-def += -D_DEBUG

# ---------------------------------------------------------------------------------
#             Make the Shared library (libOmxEvrcEnc)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

libOmxEvrcEnc-inc        := $(LOCAL_PATH)/inc
libOmxEvrcEnc-inc        += $(TARGET_OUT_HEADERS)/mm-core/omxcore

LOCAL_MODULE            := libOmxEvrcEnc
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxEvrcEnc-def)
LOCAL_C_INCLUDES        := $(libOmxEvrcEnc-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libutils liblog

LOCAL_SRC_FILES         := src/aenc_svr.c
LOCAL_SRC_FILES         += src/omx_evrc_aenc.cpp

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------------
#             Make the apps-test (mm-aenc-omxevrc-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

mm-evrc-enc-test-inc     := $(LOCAL_PATH)/inc
mm-evrc-enc-test-inc     += $(LOCAL_PATH)/test
mm-evrc-enc-test-inc     += $(TARGET_OUT_HEADERS)/mm-core/omxcore

LOCAL_MODULE            := mm-aenc-omxevrc-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libOmxEvrcEnc-def)
LOCAL_C_INCLUDES        := $(mm-evrc-enc-test-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libmm-omxcore
LOCAL_SHARED_LIBRARIES  += libOmxEvrcEnc

LOCAL_SRC_FILES         := test/omx_evrc_enc_test.c

include $(BUILD_EXECUTABLE)

endif #BUILD_TINY_ANDROID
endif # is-board-platform-in-list
# ---------------------------------------------------------------------------------
#                     END
# ---------------------------------------------------------------------------------

