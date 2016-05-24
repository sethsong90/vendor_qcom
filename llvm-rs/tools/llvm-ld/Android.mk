LOCAL_PATH:= $(call my-dir)

llvm_ld_SRC_FILES := \
  Optimize.cpp  \
  llvm-ld.cpp

llvm_ld_STATIC_LIBRARIES := \
  libRSLLVMBitWriter \
  libRSLLVMLinker \
  libRSLLVMBitReader \
  libRSLLVMArchive \
  libRSLLVMipo \
  libRSLLVMScalarOpts \
  libRSLLVMInstCombine \
  libRSLLVMTransformUtils \
  libRSLLVMVectorize \
  libRSLLVMipa \
  libRSLLVMAnalysis \
  libRSLLVMTarget \
  libRSLLVMCore \
  libRSLLVMSupport

#===---------------------------------------------------------------===
# llvm-ld command line tool (host)
#===---------------------------------------------------------------===

include $(CLEAR_VARS)

LOCAL_MODULE := RSllvm-ld
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := $(llvm_ld_SRC_FILES)
LOCAL_STATIC_LIBRARIES := $(llvm_ld_STATIC_LIBRARIES)
LOCAL_LDLIBS += -lpthread -lm -ldl

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_EXECUTABLE)
