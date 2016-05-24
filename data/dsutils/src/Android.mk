# sources and intermediate files are separated

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO   := data/inc
LOCAL_COPY_HEADERS      := ../inc/comdef.h
LOCAL_COPY_HEADERS      += ../inc/ds_cmdq.h
LOCAL_COPY_HEADERS      += ../inc/ds_list.h
LOCAL_COPY_HEADERS      += ../inc/ds_string.h
LOCAL_COPY_HEADERS      += ../inc/ds_util.h
LOCAL_COPY_HEADERS      += ../inc/ds_sl_list.h
LOCAL_COPY_HEADERS      += ../inc/queue.h
LOCAL_COPY_HEADERS      += ../inc/stm2.h
LOCAL_COPY_HEADERS      += ../inc/stm2_os.h

# Logging Features. Enable only one at any time  
#LOCAL_CFLAGS += -DFEATURE_DATA_LOG_SYSLOG
#LOCAL_CFLAGS += -DFEATURE_DATA_LOG_ADB
LOCAL_CFLAGS += -DFEATURE_DATA_LOG_QXDM
LOCAL_CFLAGS += -DFEATURE_DS_LINUX_NO_RPC
LOCAL_CFLAGS += -DFEATURE_DATA_LINUX
LOCAL_CFLAGS += -DFEATURE_DS_LINUX_ANDROID
LOCAL_CFLAGS += -DFEATURE_Q_NO_SELF_QPTR

LOCAL_C_INCLUDES := system/core/libnetutils/
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../dss_new/src/platform/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/common/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/diag/include

LOCAL_SRC_FILES += ds_cmdq.c
LOCAL_SRC_FILES += ds_list.c
LOCAL_SRC_FILES += ds_util.c
LOCAL_SRC_FILES += ds_sl_list.c
LOCAL_SRC_FILES += queue.c
LOCAL_SRC_FILES += stm2.c

LOCAL_LDLIBS += -lpthread

LOCAL_MODULE := libdsutils
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := liblog libdiag libcutils

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
