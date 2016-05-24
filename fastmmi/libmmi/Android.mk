LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE:= libmmi
LOCAL_SRC_FILES := mmi_window.cpp \
                   mmi_module_manage.cpp \
                   mmi_button.cpp \
                   mmi_text.cpp \
                   mmi_item.cpp \
                   mmi_key.cpp \
                   mmi_config.cpp \
                   mmi_state.cpp \
                   mmi_utils.cpp

LOCAL_C_INCLUDES := bootable/recovery/minui
LOCAL_C_INCLUDES += external/connectivity/stlport/stlport

LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall
LOCAL_SHARED_LIBRARIES := libpixelflinger libcutils libminui libdl

LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

include $(BUILD_SHARED_LIBRARY)

