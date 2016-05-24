ifeq ($(call is-vendor-board-platform,QCOM),true)
ifneq ($(call is-board-platform,copper),true)
ifneq ($(filter msm8974 msm8960 msm8226 msm8610,$(TARGET_BOARD_PLATFORM)),)

ROOT_DIR := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_HTTP_PATH := $(ROOT_DIR)

include $(LOCAL_HTTP_PATH)/IPStream/Common/Network/Android_Network.mk

include $(LOCAL_HTTP_PATH)/IPStream/Common/StreamUtils/Android_StreamUtils.mk

include $(LOCAL_HTTP_PATH)/IPStream/Protocol/HTTP/Android_Protocol.mk

include $(LOCAL_HTTP_PATH)/IPStream/Source/HTTP/Android_Source.mk

include $(LOCAL_HTTP_PATH)/IPStream/MMI/HTTP/Android_MMI.mk

include $(LOCAL_HTTP_PATH)/AAL/Android_AAL.mk

endif  #target-board-platform
endif  #is-board-platform
endif  #is-vendor-board-platform
