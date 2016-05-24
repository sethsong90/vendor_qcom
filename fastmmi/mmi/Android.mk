
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := mmi.cpp \
                   draw.cpp \
                   input.cpp \
                   exec.cpp \
                   diagext.cpp

LOCAL_MODULE := mmi
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall
LOCAL_C_INCLUDES := bootable/recovery/minui \
                    external/connectivity/stlport/stlport \
                    vendor/qcom/proprietary/diag/include \
                    vendor/qcom/proprietary/diag/src/ \
                    $(TARGET_OUT_HEADERS)/common/inc \
                    $(MMI_ROOT)/libmmi \

LOCAL_SHARED_LIBRARIES := libmmi libdl libcutils libdiag libminui libpng

LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

ifeq ($(call is-board-platform,msm8226),true)
LOCAL_SRC_FILES := mmi-8x26.cfg
else
  ifeq ($(call is-board-platform,msm8610),true)
      LOCAL_SRC_FILES := mmi-8x10.cfg
  else
      LOCAL_SRC_FILES := mmi.cfg
  endif
endif

LOCAL_CFLAGS := -Wall
LOCAL_MODULE := mmi.cfg
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := MMI_PCBA.cfg
LOCAL_CFLAGS := -Wall
LOCAL_MODULE := MMI_PCBA.cfg
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional

include $(BUILD_PREBUILT)
