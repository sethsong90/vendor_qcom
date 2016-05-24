LOCAL_PATH:= $(call my-dir)

transforms_scalar_SRC_FILES := \
  ADCE.cpp \
  BasicBlockPlacement.cpp \
  CodeGenPrepare.cpp \
  ConstantProp.cpp \
  CorrelatedValuePropagation.cpp \
  DCE.cpp \
  DeadStoreElimination.cpp \
  EarlyCSE.cpp \
  EnableBaseOffset.cpp \
  FactorizeExpr.cpp \
  GVN.cpp \
  GlobalMerge.cpp \
  IndVarSimplify.cpp \
  JumpThreading.cpp \
  LICM.cpp \
  LoopDeletion.cpp \
  LoopIdiomRecognize.cpp \
  LoopIndexOffset.cpp \
  LoopInstSimplify.cpp \
  LoopRotation.cpp \
  LoopStrengthReduce.cpp \
  LoopUnrollPass.cpp \
  LoopUnswitch.cpp \
  LowerAtomic.cpp \
  MemCpyOptimizer.cpp \
  ObjCARC.cpp \
  PartialLICM.cpp \
  PostIncrement.cpp \
  PrecomputeLoop.cpp \
  Reassociate.cpp \
  Reg2Mem.cpp \
  SCCP.cpp \
  Scalar.cpp \
  ScalarReplAggregates.cpp \
  SelectConversion.cpp \
  SimplifyCFGPass.cpp \
  SimplifyLibCalls.cpp \
  Sink.cpp \
  TailDuplication.cpp \
  TailRecursionElimination.cpp \
  TypePromotion.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES :=	\
	$(transforms_scalar_SRC_FILES)

LOCAL_MODULE:= libRSLLVMScalarOpts

LOCAL_MODULE_TAGS := optional

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(transforms_scalar_SRC_FILES)
LOCAL_MODULE:= libRSLLVMScalarOpts

LOCAL_MODULE_TAGS := optional

include $(LLVM_DEVICE_BUILD_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_STATIC_LIBRARY)
