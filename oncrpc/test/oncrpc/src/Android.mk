LOCAL_PATH := $(my-dir)

include $(QC_PROP_ROOT)/common/build/remote_api_makefiles/target_api_enables.mk
include $(QC_PROP_ROOT)/common/build/remote_api_makefiles/remote_api_defines.mk

commonSources :=
commonIncludes := $(TARGET_OUT_HEADERS)/common/inc
commonIncludes += $(TARGET_OUT_HEADERS)/diag/include
commonIncludes += $(TARGET_OUT_HEADERS)/ping_mdm/inc
commonIncludes += $(LOCAL_PATH)/../../../inc
commonCflags   := $(remote_api_defines)
commonCflags   += $(remote_api_enables)
commonSharedLibraries := libping_mdm liboncrpc


ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_test_stopstart
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) oncrpc_test_stopstart.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_test
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) oncrpc_test.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_init_test
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) oncrpc_init_test.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)

ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_test_threads
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) oncrpc_test_threads.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_test_threads_multi
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) oncrpc_test_threads_multi.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(PING_MDM_RPC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_test_threads_mid
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_SRC_FILES := $(commonSources) oncrpc_test_threads_mid.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_timer_test
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../oncrpc
LOCAL_SRC_FILES := $(commonSources) oncrpc_timer_test.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := $(commonSharedLibraries)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)

ifneq ($(call is-board-platform,msm7627a),true)
ifeq ($(SND_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_test_snd_0001
LOCAL_CFLAGS := $(commonCflags)
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/snd/inc
LOCAL_SRC_FILES := $(commonSources) oncrpc_test_snd_0001.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libsnd liboncrpc
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif
endif

ifeq ($(SND_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_test_snd_0002
LOCAL_CFLAGS := $(commonCflags)
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/snd/inc
LOCAL_SRC_FILES := $(commonSources) oncrpc_test_snd_0002.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libsnd liboncrpc
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(CM_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_test_cm_0001
LOCAL_CFLAGS := $(commonCflags)
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/cm/inc
LOCAL_SRC_FILES := $(commonSources) oncrpc_test_cm_0001.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcm liboncrpc
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(CM_FUSION_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_test_cm_fusion
LOCAL_CFLAGS := $(commonCflags)
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/cm/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/cm_fusion/inc
LOCAL_SRC_FILES := $(commonSources) oncrpc_test_cm_fusion.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcm libcm_fusion liboncrpc
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(WMS_FUSION_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_test_wms_fusion
LOCAL_CFLAGS := $(commonCflags)
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/wms_fusion/inc
LOCAL_SRC_FILES := $(commonSources) oncrpc_test_wms_fusion.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libwms_fusion liboncrpc
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif

ifeq ($(ADC_ENABLE),1)
include $(CLEAR_VARS)
LOCAL_MODULE := oncrpc_adc_test
LOCAL_CFLAGS := $(commonCflags)
LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/adc/inc
LOCAL_SRC_FILES := $(commonSources) oncrpc_adc_test.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libadc liboncrpc
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/rpc-tests
include $(BUILD_EXECUTABLE)
endif
