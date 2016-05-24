
LOCAL_PATH := $(call my-dir)

# gsensor so
include $(CLEAR_VARS)
LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := gsensor.cpp \
                   common.cpp

LOCAL_MODULE := mmi_gsensor
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall
LOCAL_C_INCLUDES := $(MMI_ROOT)/libmmi
LOCAL_C_INCLUDES += external/connectivity/stlport/stlport
LOCAL_C_INCLUDES += bootable/recovery/minui

LOCAL_SHARED_LIBRARIES := libmmi libcutils libhardware libminui

LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

include $(BUILD_SHARED_LIBRARY)

# psensor so
include $(CLEAR_VARS)

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := psensor.cpp \
                   common.cpp

LOCAL_MODULE := mmi_psensor
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall
LOCAL_C_INCLUDES := $(MMI_ROOT)/libmmi
LOCAL_C_INCLUDES += external/connectivity/stlport/stlport
LOCAL_C_INCLUDES += bootable/recovery/minui

LOCAL_SHARED_LIBRARIES := libmmi libcutils libhardware libminui

LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

include $(BUILD_SHARED_LIBRARY)

# msensor so
include $(CLEAR_VARS)

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := msensor.cpp \
                   common.cpp

LOCAL_MODULE := mmi_msensor
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall
LOCAL_C_INCLUDES := $(MMI_ROOT)/libmmi
LOCAL_C_INCLUDES += external/connectivity/stlport/stlport
LOCAL_C_INCLUDES += bootable/recovery/minui

LOCAL_SHARED_LIBRARIES := libmmi libcutils libhardware libminui

LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

include $(BUILD_SHARED_LIBRARY)

# lightsensor so
include $(CLEAR_VARS)

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := lsensor.cpp \
                   common.cpp

LOCAL_MODULE := mmi_lsensor
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall
LOCAL_C_INCLUDES := $(MMI_ROOT)/libmmi
LOCAL_C_INCLUDES += external/connectivity/stlport/stlport
LOCAL_C_INCLUDES += bootable/recovery/minui

LOCAL_SHARED_LIBRARIES := libmmi libcutils libhardware libminui

LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

include $(BUILD_SHARED_LIBRARY)

# gyroscope so
include $(CLEAR_VARS)

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := gyroscope.cpp \
                   common.cpp

LOCAL_MODULE := mmi_gyroscope
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall
LOCAL_C_INCLUDES := $(MMI_ROOT)/libmmi
LOCAL_C_INCLUDES += external/connectivity/stlport/stlport
LOCAL_C_INCLUDES += bootable/recovery/minui

LOCAL_SHARED_LIBRARIES := libmmi libcutils libhardware libminui

LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

include $(BUILD_SHARED_LIBRARY)
