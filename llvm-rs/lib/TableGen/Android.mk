LOCAL_PATH:= $(call my-dir)


lib_TableGen_SRC_FILES := \
	Error.cpp  \
	Main.cpp   \
	Record.cpp   \
	TGLexer.cpp   \
	TGParser.cpp   \
	TableGenAction.cpp \
	TableGenBackend.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES :=	\
	$(lib_TableGen_SRC_FILES)

REQUIRES_EH := 1
REQUIRES_RTTI := 1

LOCAL_MODULE:= libRSLLVMTableGen

LOCAL_MODULE_TAGS := optional


include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_STATIC_LIBRARY)
