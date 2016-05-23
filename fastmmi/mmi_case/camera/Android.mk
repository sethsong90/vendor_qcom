ifdef ENABLE__
LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES:= camera.cpp


LOCAL_C_INCLUDES := $(MMI_ROOT)/libmmi
LOCAL_C_INCLUDES += external/connectivity/stlport/stlport
LOCAL_C_INCLUDES += bootable/recovery/minui
LOCAL_C_INCLUDES += vendor/qcom/proprietary/mm-camera/apps/appslib \
                    vendor/qcom/proprietary/mm-still/ipl/inc \
                    vendor/qcom/proprietary/mm-still/jpeg/inc


LOCAL_C_INCLUDES += hardware/qcom/camera/QCamera2/stack/common \
                    hardware/qcom/camera/QCamera2/stack/mm-camera-test/inc \
                    hardware/qcom/camera/QCamera2/stack/mm-camera-interface/inc \
                    hardware/qcom/camera/mm-image-codec/qexif \
                    hardware/qcom/camera/mm-image-codec/qomx_core \
                    frameworks/native/include/media/openmax


LOCAL_C_INCLUDES+= $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include/media
LOCAL_C_INCLUDES+= $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_CFLAGS += -DCAMERA_ION_HEAP_ID=ION_CAMERA_HEAP_ID
LOCAL_CFLAGS += -Wall
ifeq ($(strip $(TARGET_USES_ION)),true)
LOCAL_CFLAGS += -DUSE_ION
endif

LOCAL_SHARED_LIBRARIES:= liboemcamera libcutils libdl libmmi libmmjpeg_interface libminui libmm-qcamera

LOCAL_MODULE:= mmi_camera

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
endif
