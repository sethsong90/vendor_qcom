ifneq ($(BUILD_TINY_ANDROID),true)

ROOT_DIR := $(call my-dir)
LOCAL_PATH := $(ROOT_DIR)
include $(CLEAR_VARS)


# ------------------------------------------------------------------------------
#       TSC device configuration file
# ------------------------------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE:= usf_tsc.idc
LOCAL_MODULE_CLASS := DATA
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/system/devices/idc
LOCAL_MODULE_OWNER := qcom
include $(BUILD_PREBUILT)

# ------------------------------------------------------------------------------
#       TSC device configuration file
# ------------------------------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE:= usf_tsc_ext.idc
LOCAL_MODULE_CLASS := DATA
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/system/devices/idc
LOCAL_MODULE_OWNER := qcom
include $(BUILD_PREBUILT)

# ------------------------------------------------------------------------------
#       TSC device configuration file
# ------------------------------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE:= usf_tsc_ptr.idc
LOCAL_MODULE_CLASS := DATA
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/system/devices/idc
LOCAL_MODULE_OWNER := qcom
include $(BUILD_PREBUILT)

# ------------------------------------------------------------------------------
#       Create directories for daemons pattern files
# ------------------------------------------------------------------------------
$(shell mkdir -p --mode=755 $(TARGET_OUT_DATA)/usf/tester/pattern;)

$(shell mkdir -p --mode=755 $(TARGET_OUT_DATA)/usf/epos/pattern;)

$(shell mkdir -p --mode=755 $(TARGET_OUT_DATA)/usf/p2p/pattern;)

$(shell mkdir -p --mode=755 $(TARGET_OUT_DATA)/usf/hovering/pattern;)

$(shell mkdir -p --mode=755 $(TARGET_OUT_DATA)/usf/gesture/pattern;)

$(shell mkdir -p --mode=755 $(TARGET_OUT_DATA)/usf/proximity/pattern;)

# ------------------------------------------------------------------------------
#       Create directories for daemons record files
# ------------------------------------------------------------------------------
$(shell mkdir -p --mode=755 $(TARGET_OUT_DATA)/usf/tester/rec;)

$(shell mkdir -p --mode=755 $(TARGET_OUT_DATA)/usf/epos/rec;)

$(shell mkdir -p --mode=755 $(TARGET_OUT_DATA)/usf/p2p/rec;)

$(shell mkdir -p --mode=755 $(TARGET_OUT_DATA)/usf/hovering/rec;)

$(shell mkdir -p --mode=755 $(TARGET_OUT_DATA)/usf/gesture/rec;)

$(shell mkdir -p --mode=755 $(TARGET_OUT_DATA)/usf/sync_gesture/rec;)

$(shell mkdir -p --mode=755 $(TARGET_OUT_DATA)/usf/proximity/rec;)

$(shell mkdir -p --mode=755 $(TARGET_OUT_DATA)/usf/pairing/rec;)

# ------------------------------------------------------------------------------
#       Create directory for Epos tuning files
# ------------------------------------------------------------------------------
$(shell mkdir -p --mode=755 $(PRODUCT_OUT)/persist/usf/epos;)

# ------------------------------------------------------------------------------
#       Create directory for Mixer profiles directory
# ------------------------------------------------------------------------------
$(shell mkdir -p --mode=755 $(PRODUCT_OUT)/persist/usf/mixer;)

# ------------------------------------------------------------------------------
#       Create directory for Pairing series files
# ------------------------------------------------------------------------------
$(shell mkdir -p --mode=755 $(PRODUCT_OUT)/persist/usf/pen_pairing;)

endif #BUILD_TINY_ANDROID

