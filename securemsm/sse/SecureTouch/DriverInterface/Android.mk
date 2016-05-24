# =============================================================================
#
# Module: Secure Touch Driver Interface
#
# =============================================================================

LOCAL_PATH          := $(call my-dir)
SECUREMSM_PATH      := vendor/qcom/proprietary/securemsm

include $(CLEAR_VARS)

LOCAL_MODULE        := libStDrvInt

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES   := \
                    $(LOCAL_PATH)/include \

LOCAL_LDLIBS       := \
                    -Wl,--version-script=$(LOCAL_PATH)/version.script

LOCAL_CFLAGS        := -g -fdiagnostics-show-option

LOCAL_SRC_FILES     := \
                    src/StDrvInt.c \

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
