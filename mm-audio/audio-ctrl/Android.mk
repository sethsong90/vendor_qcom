ifeq ($(call is-board-platform-in-list,$(MSM7K_BOARD_PLATFORMS)),true)

ifneq ($(BUILD_TINY_ANDROID),true)

ROOT_DIR := $(call my-dir)
LOCAL_PATH:= $(ROOT_DIR)
include $(CLEAR_VARS)

COMPILE_EQ_SRC := false
COMPILE_EQ_SRC := $(shell if [ -d $(ROOT_DIR)/src ] ; then echo true; fi)

# ---------------------------------------------------------------------------------
#                               Common definitons
# ---------------------------------------------------------------------------------

mm-audio-ctrl-def := -g -O3
mm-audio-ctrl-def += -DQC_MODIFIED
mm-audio-ctrl-def += -D_ANDROID_
mm-audio-ctrl-def += -DVERBOSE
mm-audio-ctrl-def += -D_DEBUG
ifeq ($(AUDIO_V2), true)
mm-audio-ctrl-def += -DAUDIOV2
endif

LIBPATH := $(ROOT_DIR)/lib

ifeq ($(strip $(COMPILE_EQ_SRC)),true)

# ---------------------------------------------------------------------------------
#                       Make the Shared library (libaudioeq)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

LOCAL_MODULE            := libaudioeq
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(mm-audio-ctrl-def)
LOCAL_PRELINK_MODULE    := false
LOCAL_SRC_FILES         := src/audioeq.c
LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

#$(shell cp -f $(TARGET_OUT_INTERMEDIATE_LIBRARIES)/$(LOCAL_MODULE).so $(LIBPATH))

else # COMPILE_EQ_SRC

# ---------------------------------------------------------------------------------
#                       Deploy the pre-built library
# ---------------------------------------------------------------------------------

LOCAL_PATH:= $(LIBPATH)
include $(CLEAR_VARS)
LOCAL_MODULE := libaudioeq
LOCAL_SRC_FILES := libaudioeq.so
LOCAL_MODULE_TAGS  := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_PREBUILT)

endif # end COMPILE_EQ_SRC

# ---------------------------------------------------------------------------------
#                       Make the apps-test (mm-audio-ctrl-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_PATH:= $(ROOT_DIR)

LOCAL_MODULE            := mm-audio-ctrl-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(mm-audio-ctrl-def)

LOCAL_C_INCLUDES        := $(LOCAL_PATH)/inc
LOCAL_C_INCLUDES        += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libaudioeq
LOCAL_SRC_FILES         := test/audio-ctrl.c


LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

include $(BUILD_EXECUTABLE)

ifeq ($(call is-chipset-prefix-in-board-platform,msm7627),true)
include $(CLEAR_VARS)
LOCAL_SRC_FILES    := AudioFilter.csv
LOCAL_BUILT_MODULE_STEM := AudioFilter.csv
LOCAL_MODULE_SUFFIX := $(suffix AudioFilter.csv)
LOCAL_MODULE := $(basename AudioFilter.csv)
LOCAL_MODULE_TAGS  := optional
LOCAL_MODULE_CLASS := ETC
include $(BUILD_PREBUILT)
endif

endif
endif
# ---------------------------------------------------------------------------------
#                                       END
# ---------------------------------------------------------------------------------

