LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := RSllvm-as

LOCAL_SRC_FILES := llvm-as.cpp

REQUIRES_EH := 1
REQUIRES_RTTI := 1

LOCAL_STATIC_LIBRARIES := \
    libRSLLVMAsmParser \
    libRSLLVMBitWriter \
    libRSLLVMCore \
    libRSLLVMSupport

LOCAL_LDLIBS += -lpthread -lm -ldl

LOCAL_MODULE_TAGS := optional

include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_EXECUTABLE)
