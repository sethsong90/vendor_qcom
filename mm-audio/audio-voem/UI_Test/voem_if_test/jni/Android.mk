ifeq ($(call is-board-platform-in-list,msm7630_surf msm7627_surf),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
	voem_if_test_jni.cpp \
   	onload.cpp

LOCAL_C_INCLUDES += \
	$(JNI_H_INCLUDE)\
        $(TARGET_OUT_HEADERS)/common/inc

LOCAL_SHARED_LIBRARIES := \
	libvoem-test \
	libcutils \
        libutils \
	libnativehelper \
        libhardware \
        libhardware_legacy \
        libui

LOCAL_PRELINK_MODULE    := false
LOCAL_MODULE:= libvoem-jni
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
endif #is-board-platform-in-list
