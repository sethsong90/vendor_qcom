LOCAL_PATH := $(my-dir)

include $(QC_PROP_ROOT)/common/build/remote_api_makefiles/target_api_enables.mk

commonIncludes := $(TARGET_OUT_HEADERS)/common/inc
commonIncludes += $(TARGET_OUT_HEADERS)/diag/include
commonIncludes += $(TARGET_OUT_HEADERS)/oem_rapi/inc
commonIncludes += $(LOCAL_PATH)/../../../inc
commonSharedLibraries := liboem_rapi liboncrpc

ifeq ($(OEM_RAPI_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oem_rapi_test
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := oem_rapi_test.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif
