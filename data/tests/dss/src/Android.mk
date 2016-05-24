################################################################################
# $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/Makefile#1 $
#
# @file tests/unit/libs/dss/src/Makefile
# @brief Makefile for building dss api tests.
################################################################################

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_1.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_1
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_3##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_3.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_3
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_4##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_4.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_4
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_5##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_5.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_5
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_6##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_6.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_6
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_7##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_7.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_7
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_8##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_8.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_8
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_9##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_9.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_9
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_10##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_10.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_10
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_20##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_20.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_20
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_21##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_21.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_21
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_22##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_22.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_22
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_30##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_30.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_30
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_31##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_31.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_31
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_32##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_32.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_32
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_40##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_40.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_40
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_50##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_50.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_50
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_100##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_100.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_100
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_101##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_101.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_101
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_102##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_102.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE:= dss_test_102
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_103##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_103.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE := dss_test_103
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_104##########

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	dss_test.c \
	dss_test_104.c

LOCAL_SHARED_LIBRARIES := \
	libqmi \
	libdss \
	libdsutils

LOCAL_CFLAGS := \
	-DFEATURE_DSS_LINUX_ANDROID \
        -DFEATURE_DATA_LOG_STDERR

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../dss/src  \
	$(TARGET_OUT_HEADERS)/data/inc/ \
	$(TARGET_OUT_HEADERS)/common/inc/

LOCAL_MODULE := dss_test_104
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)

#######dss_test_104##########
include $(CLEAR_VARS)
LOCAL_MODULE:= dss_smoke_test.sh
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := dss_smoke_test.sh
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/data_test
LOCAL_MODULE_OWNER := qcom
include $(BUILD_PREBUILT)

