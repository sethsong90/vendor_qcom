LOCAL_PATH := $(call my-dir)

mips_codegen_TBLGEN_TABLES := \
  MipsGenRegisterInfo.inc \
  MipsGenInstrInfo.inc \
  MipsGenCodeEmitter.inc \
  MipsGenMCCodeEmitter.inc \
  MipsGenAsmWriter.inc \
  MipsGenDAGISel.inc \
  MipsGenCallingConv.inc \
  MipsGenSubtargetInfo.inc

mips_codegen_SRC_FILES := \
  MipsAnalyzeImmediate.cpp \
  MipsAsmPrinter.cpp \
  MipsCodeEmitter.cpp \
  MipsDelaySlotFiller.cpp \
  MipsEmitGPRestore.cpp \
  MipsExpandPseudo.cpp \
  MipsJITInfo.cpp \
  MipsInstrInfo.cpp \
  MipsISelDAGToDAG.cpp \
  MipsISelLowering.cpp \
  MipsFrameLowering.cpp \
  MipsMachineFunction.cpp \
  MipsMCInstLower.cpp \
  MipsRegisterInfo.cpp \
  MipsSubtarget.cpp \
  MipsTargetMachine.cpp \
  MipsTargetObjectFile.cpp \
  MipsSelectionDAGInfo.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

LOCAL_MODULE:= lib${LLVM_VER}LLVMMipsCodeGen
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(mips_codegen_SRC_FILES)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/MCTargetDesc

TBLGEN_TABLES := $(mips_codegen_TBLGEN_TABLES)

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_TBLGEN_RULES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device only
# =====================================================
ifeq ($(TARGET_ARCH),mips)
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

LOCAL_MODULE:= lib${LLVM_VER}LLVMMipsCodeGen
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(mips_codegen_SRC_FILES)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/MCTargetDesc

TBLGEN_TABLES := $(mips_codegen_TBLGEN_TABLES)

include $(LLVM_DEVICE_BUILD_MK)
include $(LLVM_TBLGEN_RULES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_STATIC_LIBRARY)
endif
