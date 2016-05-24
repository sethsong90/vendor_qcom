ifeq ($(call is-board-platform-in-list,msm7630_surf msm7627_surf),true)

LOCAL_PATH:= $(call my-dir)
include $(AUDIO_ROOT)/../common/build/remote_api_makefiles/target_api_enables.mk
include $(AUDIO_ROOT)/../common/build/remote_api_makefiles/remote_api_defines.mk
include $(CLEAR_VARS)
# ---------------------------------------------------------------------------------
#                               Common definitons
# ---------------------------------------------------------------------------------

ui-mm-audio-voem_if-def := $(remote_api_enables)
ui-mm-audio-voem_if-def += $(remote_api_defines)
ui-mm-audio-voem_if-def += -g -O3
ui-mm-audio-voem_if-def += -DQC_MODIFIED
ui-mm-audio-voem_if-def += -D_ANDROID_
ui-mm-audio-voem_if-def += -DQCT_CFLAGS
ui-mm-audio-voem_if-def += -DQCT_CPPFLAGS
ui-mm-audio-voem_if-def += -DVERBOSE
ui-mm-audio-voem_if-def += -D_DEBUG
ifeq ($(call is-board-platform,msm7627_surf),true)
ui-mm-audio-voem_if-def += -DAUDIO7x27
endif

ui-mm-audio-voem_if-inc := $(TARGET_OUT_HEADERS)/common/inc
ui-mm-audio-voem_if-inc += $(TARGET_OUT_HEADERS)/voem_if/inc
ui-mm-audio-voem_if-inc += $(TARGET_OUT_HEADERS)/oncrpc/inc
ui-mm-audio-voem_if-inc += $(TARGET_OUT_HEADERS)/diag/include

# ---------------------------------------------------------------------------------
#                       Make the library (libvoem-test)
# ---------------------------------------------------------------------------------

LOCAL_CFLAGS            := $(ui-mm-audio-voem_if-def)
LOCAL_C_INCLUDES        := $(ui-mm-audio-voem_if-inc) \
			  
LOCAL_PRELINK_MODULE    := false
LOCAL_SRC_FILES         := voem_if_test.c \

LOCAL_SHARED_LIBRARIES  :=  \
	libui libdsm libqueue liboncrpc libvoem_if libutils

ifeq ($(TARGET_SIMULATOR),true)
ifeq ($(TARGET_OS),linux)
ifeq ($(TARGET_ARCH),x86)
LOCAL_LDLIBS += -lpthread -ldl -lrt -llog
endif
endif
endif

ifeq ($(WITH_MALLOC_LEAK_CHECK),true)
	LOCAL_CFLAGS += -DMALLOC_LEAK_CHECK
endif

ifeq ($(TARGET_HAVE_TSLIB),true)
	LOCAL_CFLAGS += -DHAVE_TSLIB
	LOCAL_C_INCLUDES += external/tslib/src
endif

LOCAL_MODULE:= libvoem-test

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
endif #is-board-platform-in-list
# ---------------------------------------------------------------------------------
#                                       END
# ---------------------------------------------------------------------------------

