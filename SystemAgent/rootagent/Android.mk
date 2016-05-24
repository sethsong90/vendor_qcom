#some dependency not ready
ifeq ($(strip $(FEATURE_SYSTEMAGENT)),yes)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := utils.cpp agent.cpp debug.cpp main.cpp

LOCAL_C_INCLUDES += external/connectivity/stlport/stlport
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/common/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/oncrpc/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/nv/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/diag/include

LOCAL_SHARED_LIBRARIES := libstlport libstdc++ libcutils
LOCAL_SHARED_LIBRARIES += libnv liboncrpc

LOCAL_MODULE := rootagent
LOCAL_MODULE_TAGS := optional eng

include $(BUILD_EXECUTABLE)
endif
