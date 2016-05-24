LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES :=	\
	llvm-link.cpp

LOCAL_STATIC_LIBRARIES := \
    libRSLLVMLinker \
    libRSLLVMBitReader \
    libRSLLVMBitWriter \
    libRSLLVMCore \
    libRSLLVMTransformUtils \
    libRSLLVMAnalysis \
    libRSLLVMSupport \
    libRSLLVMTarget \
		libRSLLVMAsmParser \
		libRSLLVMCore


LOCAL_MODULE := RSllvm-link

LOCAL_MODULE_TAGS := optional

LOCAL_LDLIBS += -lpthread -lm -ldl

include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_EXECUTABLE)
