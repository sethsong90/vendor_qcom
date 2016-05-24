LOCAL_PATH:= $(call my-dir)

support_SRC_FILES :=   \
  APFloat.cpp \
  APInt.cpp \
  APSInt.cpp \
  Allocator.cpp \
  BlockFrequency.cpp \
  BranchProbability.cpp \
  circular_raw_ostream.cpp \
  CommandLine.cpp \
  ConstantRange.cpp \
  CrashRecoveryContext.cpp \
  DataExtractor.cpp \
  DataStream.cpp \
  Debug.cpp \
  DeltaAlgorithm.cpp \
  DAGDeltaAlgorithm.cpp \
  Dwarf.cpp \
  ErrorHandling.cpp \
  FileUtilities.cpp \
  FoldingSet.cpp \
  FormattedStream.cpp \
  GraphWriter.cpp \
  Hashing.cpp \
  IntEqClasses.cpp \
  IntervalMap.cpp \
  IntrusiveRefCntPtr.cpp \
  IsInf.cpp \
  IsNAN.cpp \
  Locale.cpp \
  LockFileManager.cpp \
  ManagedStatic.cpp \
  MemoryBuffer.cpp \
  MemoryObject.cpp \
  PluginLoader.cpp \
  PrettyStackTrace.cpp \
  Regex.cpp \
  SmallPtrSet.cpp \
  SmallVector.cpp \
  SourceMgr.cpp \
  Statistic.cpp \
  StreamableMemoryObject.cpp \
  StringExtras.cpp \
  StringMap.cpp \
  StringPool.cpp \
  StringRef.cpp \
  SystemUtils.cpp \
  Timer.cpp \
  ToolOutputFile.cpp \
  Triple.cpp \
  Twine.cpp \
  YAMLParser.cpp \
  raw_os_ostream.cpp \
  raw_ostream.cpp \
  regcomp.c \
  regerror.c \
  regexec.c \
  regfree.c \
  regstrlcpy.c \
  Atomic.cpp \
  Disassembler.cpp \
  DynamicLibrary.cpp \
  Errno.cpp \
  Host.cpp \
  IncludeFile.cpp \
  Memory.cpp \
  Mutex.cpp \
  Path.cpp \
  PathV2.cpp \
  Process.cpp \
  Program.cpp \
  RWMutex.cpp \
  SearchForAddressOfSpecialSymbol.cpp \
  Signals.cpp \
  system_error.cpp \
  TargetRegistry.cpp \
  ThreadLocal.cpp \
  Threading.cpp \
  TimeValue.cpp \
  Valgrind.cpp \

# For the host
# =====================================================
include $(CLEAR_VARS)

# FIXME: This only requires RTTI because tblgen uses it.  Fix that.
REQUIRES_RTTI := 1

LOCAL_SRC_FILES := $(support_SRC_FILES)

LOCAL_MODULE:= libRSLLVMSupport

LOCAL_CFLAGS := -D__android__

LOCAL_MODULE_TAGS := optional

include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(support_SRC_FILES)

LOCAL_MODULE:= libRSLLVMSupport

LOCAL_CFLAGS := -D__android__

LOCAL_MODULE_TAGS := optional

include $(LLVM_DEVICE_BUILD_MK)
include $(BUILD_STATIC_LIBRARY)
