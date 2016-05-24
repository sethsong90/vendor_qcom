ifeq ($(call is-board-platform-in-list,msm8974 msm8226 msm8610),true)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE		:= rfs_access
LOCAL_MODULE_TAGS	:= optional

LOCAL_C_INCLUDES	:= $(TARGET_OUT_HEADERS)/qmi-framework/inc \
				$(TARGET_OUT_HEADERS)/common/inc \
				$(LOCAL_PATH)/src \
				$(LOCAL_PATH)/src/qmi \
				$(LOCAL_PATH)/src/util

LOCAL_SRC_FILES		+= src/rfsa_server.c \
				src/rfsa_vtl_server.c \
				src/qmi/rfsa_qmi_server.c \
				src/qmi/rfsa_v01.c \
				src/util/rfsa_event.c \
				src/util/rfsa_list.c \
				src/util/rfsa_lock.c \
				src/util/rfsa_thread.c

LOCAL_C_FLAGS		:= -Wall -DLOG_NIDEBUG=0

LOCAL_PRELINK_MODULE	:= false
LOCAL_SHARED_LIBRARIES	:= libqmi_csi libqmi_common_so libcutils

LOCAL_MODULE_OWNER := qcom
include $(BUILD_EXECUTABLE)
endif
