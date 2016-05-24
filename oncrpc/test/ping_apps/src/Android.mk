LOCAL_PATH := $(my-dir)

include $(QC_PROP_ROOT)/common/build/remote_api_makefiles/target_api_enables.mk

commonIncludes := $(TARGET_OUT_HEADERS)/common/inc
commonIncludes += $(TARGET_OUT_HEADERS)/diag/include
commonIncludes += $(LOCAL_PATH)/../../../inc
commonSharedLibraries := libping_apps liboncrpc



include $(CLEAR_VARS)
include $(QC_PROP_ROOT)/common/build/remote_api_makefiles/remote_api_defines.mk
include $(LOCAL_PATH)/../../../oncrpc_defines.mk
LOCAL_CFLAGS := $(oncrpc_defines)
LOCAL_CFLAGS += $(oncrpc_common_defines)
LOCAL_CFLAGS += $(defines_api_features)
LOCAL_C_INCLUDES += $(ONCRPC_PATH)/oncrpc/oncrpc
LOCAL_C_INCLUDES += $(ONCRPC_PATH)/oncrpc/inc
LOCAL_C_INCLUDES += $(ONCRPC_PATH)/../common/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/diag/include
LOCAL_C_INCLUDES += $(commonIncludes)
LOCAL_SRC_FILES := ping_apps_clnt.c
LOCAL_SRC_FILES += ping_apps_xdr.c
LOCAL_SHARED_LIBRARIES := liboncrpc
LOCAL_SHARED_LIBRARIES += libdiag
LOCAL_COPY_HEADERS_TO := ping_apps/inc
LOCAL_COPY_HEADERS := ping_apps.h
LOCAL_COPY_HEADERS += ping_apps_rpc.h
LOCAL_MODULE := libping_apps
LOCAL_MODULE_TAGS := optional
LOCAL_LDLIBS += -lpthread
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)



include $(CLEAR_VARS)
LOCAL_MODULE := ping_apps_client_test_0000
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := ping_apps_client_test_0000.c
LOCAL_SRC_FILES += ping_apps_client_common.c
LOCAL_SRC_FILES += ping_client_common_stats.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
