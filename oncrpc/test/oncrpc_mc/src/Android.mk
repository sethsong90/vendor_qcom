LOCAL_PATH := $(my-dir)

include $(QC_PROP_ROOT)/common/build/remote_api_makefiles/target_api_enables.mk
include $(QC_PROP_ROOT)/common/build/remote_api_makefiles/remote_api_defines.mk

commonSources := oncrpc_mc_common.c
commonIncludes := $(TARGET_OUT_HEADERS)/common/inc
commonIncludes += $(TARGET_OUT_HEADERS)/diag/include
commonIncludes += $(TARGET_OUT_HEADERS)/ping_mdm/inc
commonIncludes += $(TARGET_OUT_HEADERS)/oem_rapi/inc
commonIncludes += $(LOCAL_PATH)/../../../inc
commonCflags   := $(remote_api_defines)
commonCflags   += $(remote_api_enables)
commonSharedLibraries := libping_mdm liboem_rapi liboncrpc


ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_mc_test_0001
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) oncrpc_mc_test_0001.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif


ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_mc_test_0002
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) oncrpc_mc_test_0002.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif


ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_mc_test_0004
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) oncrpc_mc_test_0004.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif


ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_mc_test_0005
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) oncrpc_mc_test_0005.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif


ifeq ($(PING_MDM_RPC_ENABLE),1)
ifeq ($(OEM_RAPI_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_mc_test_0006
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) oncrpc_mc_test_0006.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif
endif


ifeq ($(PING_MDM_RPC_ENABLE),1)
ifeq ($(OEM_RAPI_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_mc_test_0007
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) oncrpc_mc_test_0007.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif
endif


ifeq ($(PING_MDM_RPC_ENABLE),1)
ifeq ($(OEM_RAPI_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_mc_test_0008
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) oncrpc_mc_test_0008.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif
endif


ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_mc_test_1001
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) oncrpc_mc_test_1001.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif


ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_mc_test_1002
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) oncrpc_mc_test_1002.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif
