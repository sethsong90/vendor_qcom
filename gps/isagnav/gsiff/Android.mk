LOCAL_PATH:=$(my-dir)
include $(CLEAR_VARS)

LOC_API_DIR   = $(TOP)/hardware/qcom/gps/loc_api
GPS_UTILS_DIR = $(TOP)/hardware/qcom/gps/utils

FEATURE_GSIFF_ANDROID_HAL = 1
FEATURE_GSIFF_ANDROID_NDK = 1
ifndef NO_GSIFF_DSPS
FEATURE_GSIFF_DSPS = 1
endif

LOCAL_CFLAGS:= \
    -D_ANDROID_

# Absoulte paths
LOCAL_C_INCLUDES:= \
    $(TARGET_OUT_HEADERS)/qmi/inc \
    $(TARGET_OUT_HEADERS)/qmi-framework/inc \
    $(TARGET_OUT_HEADERS)/diag/include \
    $(TOP) \
    $(LOCAL_PATH) \
    $(TOP)/vendor/qcom/opensource/location/loc_api/loc_api_v02 \
    $(GPS_UTILS_DIR) \
    $(LOCAL_PATH)/../../daemon

# Relative paths
LOCAL_SRC_FILES:= \
    gsiff_daemon_manager.c \
    os_kf.c \
    gsiff_sensor_provider_glue.c \
    gsiff_sensor_provider_common.c \
    gsiff_slim_client_glue.c\
    gsiff_loc_api_glue.c \
    ../../daemon/gpsone_glue_pipe.c \
    ../../daemon/gpsone_glue_msg.c \
    ../../daemon/gpsone_thread_helper.c

LOCAL_SHARED_LIBRARIES:= \
    libutils \
    liblog \
    libloc_api_v02 \
    libgps.utils

#
# Enable the Android NDK sensor provider
#
ifeq ($(FEATURE_GSIFF_ANDROID_NDK),1)
LOCAL_SRC_FILES        += gsiff_sensor_provider_and_ndk.c

LOCAL_C_INCLUDES       += $(TOP)/frameworks/base/native/include

LOCAL_CFLAGS           += -DFEATURE_GSIFF_ANDROID_NDK

LOCAL_SHARED_LIBRARIES += libandroid
endif

#
# Enable the Android HAL sensor provider
#
ifeq ($(FEATURE_GSIFF_ANDROID_HAL),1)
LOCAL_SRC_FILES        += gsiff_sensor_provider_and_hal.c

LOCAL_C_INCLUDES       += $(TOP)/hardware/libhardware/include \
                          $(TOP)/system/core/include

LOCAL_CFLAGS           += -DFEATURE_GSIFF_ANDROID_HAL

LOCAL_SHARED_LIBRARIES += libhardware
endif

#
# Enable the DSPS (Sensor1) sensor provider
#
ifeq ($(FEATURE_GSIFF_DSPS),1)
LOCAL_SRC_FILES        += gsiff_sensor_provider_sensor1.c

LOCAL_C_INCLUDES       += $(TARGET_OUT_HEADERS)/sensors/inc

LOCAL_CFLAGS           += -DFEATURE_GSIFF_DSPS

LOCAL_SHARED_LIBRARIES += libsensor1
endif


LOCAL_MODULE:=gsiff_daemon
LOCAL_MODULE_TAGS:=optional

include $(BUILD_EXECUTABLE)
