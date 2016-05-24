# =============================================================================
#
# Module: Secure Touch Driver Interface
#
# =============================================================================

# function to find all *.cpp files under a directory
define all-cpp-files-under
$(patsubst ./%,%, \
  $(shell cd $(LOCAL_PATH) ; \
          find $(1) -name "*.cpp" -and -not -name ".*") \
)
endef

LOCAL_PATH          := $(call my-dir)
SECUREMSM_PATH      := vendor/qcom/proprietary/securemsm

include $(CLEAR_VARS)
include external/connectivity/stlport/libstlport.mk

LOCAL_MODULE        := libStSideChannels

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES   += \
                    $(LOCAL_PATH)/include \
                    $(LOCAL_PATH)/modules/include \

LOCAL_LDLIBS       += \
                    -Wl,--version-script=$(LOCAL_PATH)/version.script

LOCAL_CFLAGS        += -g -fdiagnostics-show-option -std=c++0x

LOCAL_SRC_FILES := $(call all-c-files-under, .)  $(call all-cpp-files-under, .)

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SHARED_LIBRARIES := liblog

include $(BUILD_SHARED_LIBRARY)

include $(call all-subdir-makefiles)
