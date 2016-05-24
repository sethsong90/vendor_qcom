LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
   dun_service.c \
   port_bridge.c \
   dun_kevents.c

LOCAL_MODULE:= port-bridge
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libutils libcutils libCommandSvc

LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/common/inc/
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/command-svc/


LOCAL_LDLIBS += -lpthread

LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)
