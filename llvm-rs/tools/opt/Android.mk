LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := RSopt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES

LOCAL_SRC_FILES := \
  AnalysisWrappers.cpp \
  GraphPrinters.cpp \
  PrintSCC.cpp \
  opt.cpp

LOCAL_STATIC_LIBRARIES := \
  libRSLLVMipo \
  libRSLLVMScalarOpts \
  libRSLLVMInstCombine \
  libRSLLVMInstrumentation \
  libRSLLVMTransformUtils \
  libRSLLVMipa \
  libRSLLVMAnalysis \
  libRSLLVMTarget \
  libRSLLVMMC \
  libRSLLVMAsmParser \
  libRSLLVMBitWriter \
  libRSLLVMBitReader \
  libRSLLVMCore \
  libRSLLVMSupport

LOCAL_LDLIBS += -lpthread -lm -ldl

LOCAL_C_INCLUDES += external/llvm/include

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_EXECUTABLE)
