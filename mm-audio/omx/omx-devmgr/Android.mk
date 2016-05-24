ifeq ($(call is-board-platform-in-list,msm7630_surf msm7630_fusion msm8660),true)
ifneq ($(BUILD_TINY_ANDROID),true)

ROOT_DIR := $(call my-dir)
LOCAL_PATH := $(ROOT_DIR)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
# 				Common definitons
# ---------------------------------------------------------------------------------

mm-omx-devmgr-def := -g -O3
mm-omx-devmgr-def += -D_ANDROID_

# ---------------------------------------------------------------------------------
# 			Make the apps-test (omx-devmgr)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)
mm-omx-devmgr-inc     += $(LOCAL_PATH)/../../audio-alsa/inc

LOCAL_MODULE		:= mm-omx-devmgr
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS	  	:= $(mm-omx-devmgr-def)
LOCAL_C_INCLUDES	:= $(mm-omx-devmgr-inc)
LOCAL_PRELINK_MODULE	:= false
LOCAL_SHARED_LIBRARIES	:= libaudioalsa
LOCAL_SRC_FILES		:= omx_dev_mgr.c

include $(BUILD_EXECUTABLE)

# ---------------------------------------------------------------------------------
# 					END
# ---------------------------------------------------------------------------------

endif #BUILD_TINY_ANDROID
endif # is-board-platform-in-list
