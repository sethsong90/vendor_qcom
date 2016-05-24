ifeq ($(strip $(AUDIO_FEATURE_ENABLED_LISTEN)),true)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -DAUDIO_LISTEN_ENABLED \
                -DLOG_NDEBUG

LOCAL_SRC_FILES:= \
    src/com_qualcomm_listen_ListenReceiver.cpp \
    src/com_qualcomm_listen_ListenMasterControl.cpp \
    src/com_qualcomm_listen_ListenVoiceWakeupSession.cpp \
    src/com_qualcomm_listen_ListenSoundModel.cpp

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libnativehelper \
    libutils \
    libbinder \
    libskia \
    libui \
    libcutils \
    libgui \
    liblisten

LOCAL_SHARED_LIBRARIES += liblistensoundmodel

LOCAL_MODULE:= liblistenjni
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/inc \
    $(TOP)/vendor/qcom/proprietary/mm-audio/audio-listen/framework/service/inc \
    $(TOP)/hardware/libhardware/include \
    $(TOP)/hardware/libhardware_legacy/include \
    $(TARGET_OUT_HEADERS)/mm-audio/audio-listen

include $(BUILD_SHARED_LIBRARY)

endif
