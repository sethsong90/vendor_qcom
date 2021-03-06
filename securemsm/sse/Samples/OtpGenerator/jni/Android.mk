LOCAL_PATH           := $(call my-dir)
PKCS11_PATH          := $(LOCAL_PATH)/../../../pkcs11/include

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE         := libPkiOtpGenerator
LOCAL_CFLAGS         := -g -fdiagnostics-show-option -Wno-format \
                       -Wno-missing-braces -Wno-missing-field-initializers \
                       -std=gnu++0x -fpermissive

LOCAL_PRELINK_MODULE := false
LOCAL_CPP_EXTENSION  := .cpp

# includes
LOCAL_C_INCLUDES     += $(LOCAL_PATH)/include \
                        $(EXTERNAL_INCLUDES) \
                        $(PKCS11_PATH)

# Add all source file names to be included in lib separated by a whitespace
LOCAL_SRC_FILES        := pki.cpp pkiInterface.cpp
LOCAL_SHARED_LIBRARIES := liblog libutils libcutils libandroid
LOCAL_SHARED_LIBRARIES += libdl
LOCAL_STATIC_LIBRARIES :=

LOCAL_MODULE_OWNER := qcom
include $(BUILD_SHARED_LIBRARY)

