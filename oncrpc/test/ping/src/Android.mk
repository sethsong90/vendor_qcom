LOCAL_PATH := $(my-dir)

include $(QC_PROP_ROOT)/common/build/remote_api_makefiles/target_api_enables.mk

commonSources := ping_mdm_client_common.c ping_client_common_stats.c
commonIncludes := $(TARGET_OUT_HEADERS)/common/inc
commonIncludes += $(TARGET_OUT_HEADERS)/diag/include
commonIncludes += $(TARGET_OUT_HEADERS)/ping_mdm/inc
commonIncludes += $(LOCAL_PATH)/../../../inc
commonSharedLibraries := libping_mdm liboncrpc

ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := ping_mdm_clnt_test_0000
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) ping_mdm_client_test_0000.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := ping_mdm_clnt_test_0001
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) ping_mdm_client_test_0001.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := ping_mdm_clnt_test_0002
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) ping_mdm_client_test_0002.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := ping_mdm_clnt_test_1000
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) ping_mdm_client_test_1000.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := ping_mdm_clnt_test_1001
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) ping_mdm_client_test_1001.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := ping_mdm_clnt_test_1002
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) ping_mdm_client_test_1002.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := ping_mdm_lte_test_0000
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) ping_mdm_lte_test_0000.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := ping_mdm_lte_test_0001
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) ping_mdm_lte_test_0001.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := ping_mdm_lte_test_1001
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) ping_mdm_lte_test_1001.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := ping_mdm_lte_test_1000
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) ping_mdm_lte_test_1000.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif
