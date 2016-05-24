ifeq ($(call is-board-platform-in-list,msm7627_surf msm7627a msm7630_surf msm7630_fusion),true)

ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
include $(AUDIO_ROOT)/../common/build/remote_api_makefiles/target_api_enables.mk
include $(AUDIO_ROOT)/../common/build/remote_api_makefiles/remote_api_defines.mk

ifeq ($(VOEM_IF_ENABLE),1)

# ---------------------------------------------------------------------------------
#                               Common definitons
# ---------------------------------------------------------------------------------

mm-audio-voem_if-def := $(remote_api_enables)
mm-audio-voem_if-def += $(remote_api_defines)
mm-audio-voem_if-def += -g -O3
mm-audio-voem_if-def += -DQC_MODIFIED
mm-audio-voem_if-def += -D_ANDROID_
mm-audio-voem_if-def += -DQCT_CFLAGS
mm-audio-voem_if-def += -DQCT_CPPFLAGS
mm-audio-voem_if-def += -DVERBOSE
mm-audio-voem_if-def += -D_DEBUG

ifeq ($(call is-chipset-in-board-platform,msm7627),true)
mm-audio-voem_if-def += -DAUDIO7x27
endif
ifeq ($(call is-board-platform,msm7627a),true)
mm-audio-voem_if-def += -DAUDIO7x27A
endif

mm-audio-voem_if-inc := $(TARGET_OUT_HEADERS)/common/inc
mm-audio-voem_if-inc += $(TARGET_OUT_HEADERS)/voem_if/inc
mm-audio-voem_if-inc += $(TARGET_OUT_HEADERS)/oncrpc/inc
mm-audio-voem_if-inc += $(TARGET_OUT_HEADERS)/diag/include
# ---------------------------------------------------------------------------------
#                       Make the apps-test (mm-audio-voem-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

mm-voem_if-native-inc   := $(LOCAL_PATH)
LOCAL_MODULE            := mm-audio-voem_if-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(mm-audio-voem_if-def)
LOCAL_C_INCLUDES        := $(mm-audio-voem_if-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SRC_FILES         := voem_if_test.c
LOCAL_SHARED_LIBRARIES  := libdsm libqueue liboncrpc libvoem_if

include $(BUILD_EXECUTABLE)

endif
endif
endif

# ---------------------------------------------------------------------------------
#                                       END
# ---------------------------------------------------------------------------------

