ifeq ($(call is-board-platform-in-list,$(QSD8K_BOARD_PLATFORMS)),true)

ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
#                               Common definitons
# ---------------------------------------------------------------------------------

libatu-def := -g -O3
libatu-def += -DQC_MODIFIED
libatu-def += -DVERBOSE
libatu-def += -D_DEBUG

# ---------------------------------------------------------------------------------
#             Deploy the headers that can be exposed
# ---------------------------------------------------------------------------------

LOCAL_COPY_HEADERS_TO   := mm-audio/atu
LOCAL_COPY_HEADERS      := inc/msm8k_atu.h

# ---------------------------------------------------------------------------------
#                       Make the Shared library (libatu)
# ---------------------------------------------------------------------------------

libatu-inc              := $(LOCAL_PATH)/inc

LOCAL_MODULE            := libatu
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(libatu-def)
LOCAL_C_INCLUDES        := $(libatu-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libutils liblog

LOCAL_SRC_FILES         := src/msm8k_atu.c
LOCAL_SRC_FILES         += src/msm8k_atu_sound_db.c
LOCAL_SRC_FILES         += src/msm8k_atu_tone_db.c

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

endif
endif
# ---------------------------------------------------------------------------------
#                                       END
# ---------------------------------------------------------------------------------

