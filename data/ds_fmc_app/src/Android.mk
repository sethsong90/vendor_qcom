LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO   := data/inc
LOCAL_COPY_HEADERS      := ../inc/ds_fmc_app.h

# Logging Features. Enable only one at any time  
#LOCAL_CFLAGS += -DFEATURE_DATA_LOG_STDERR
#LOCAL_CFLAGS += -DFEATURE_DATA_LOG_SYSLOG
#LOCAL_CFLAGS += -DFEATURE_DATA_LOG_ADB
LOCAL_CFLAGS += -DFEATURE_DATA_LOG_QXDM

LOCAL_CFLAGS += -DFEATURE_DS_LINUX_NO_RPC
LOCAL_CFLAGS += -DFEATURE_DS_LINUX_ANDROID
#LOCAL_CFLAGS += -DFEATURE_DS_LINUX_FUSION
#LOCAL_CFLAGS += -DDS_FMC_APP_TEST

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../dsutils/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/diag/include
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/qmi/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/common/inc
LOCAL_C_INCLUDES += system/core/libnetutils/
 
LOCAL_SRC_FILES += ds_fmc_app.c
LOCAL_SRC_FILES += ds_fmc_app_main.c
LOCAL_SRC_FILES += ds_fmc_app_call_mgr.c
LOCAL_SRC_FILES += ds_fmc_app_tunnel_mgr.c
LOCAL_SRC_FILES += ds_fmc_app_qmi.c
LOCAL_SRC_FILES += ds_fmc_app_exec.c
LOCAL_SRC_FILES += ds_fmc_app_sm.c
LOCAL_SRC_FILES += ds_fmc_app_sm_int.c
LOCAL_SRC_FILES += ds_fmc_app_util.c
LOCAL_SRC_FILES += ds_fmc_app_data_ext_if.c
LOCAL_SRC_FILES += ds_fmc_app_data_mdm_if.c

LOCAL_LDLIBS += -lpthread

LOCAL_MODULE := ds_fmc_appd
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := \
	libdsutils  \
	libqmi      \
	libdiag     \
  libcutils

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)
