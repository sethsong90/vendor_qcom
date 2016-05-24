ifneq ($(MM_AUDIO_MVS_DISABLED),true)
ifeq ($(call is-board-platform-in-list,$(MSM7K_BOARD_PLATFORMS)),true)
ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(AUDIO_ROOT)/../common/build/remote_api_makefiles/target_api_enables.mk
include $(AUDIO_ROOT)/../common/build/remote_api_makefiles/remote_api_defines.mk

ifeq ($(MVS_ENABLE),1)

ifeq ($(call is-chipset-in-board-platform,msm7630),true)
  AUDIO_V2 := true
  USE_MVS_DRIVER := true
endif

ifeq ($(call is-board-platform,msm7627a),true)
  USE_MVS_DRIVER := true
endif


# ---------------------------------------------------------------------------------
#                               Common definitons
# ---------------------------------------------------------------------------------

mm-audio-mvs-def := $(remote_api_enables)
mm-audio-mvs-def += $(remote_api_defines)
mm-audio-mvs-def += -g -O3
mm-audio-mvs-def += -DQC_MODIFIED
mm-audio-mvs-def += -D_ANDROID_
mm-audio-mvs-def += -DQCT_CFLAGS
mm-audio-mvs-def += -DQCT_CPPFLAGS
mm-audio-mvs-def += -DFEATURE_AUDIO_AGC
mm-audio-mvs-def += -DVERBOSE
mm-audio-mvs-def += -D_DEBUG
ifeq ($(AUDIO_V2), true)
mm-audio-mvs-def += -DAUDIOV2
endif
ifeq ($(USE_MVS_DRIVER), true)
mm-audio-mvs-def += -DUSE_MVS_DRIVER
endif


mm-audio-mvs-inc := $(TARGET_OUT_HEADERS)/common/inc
ifeq ($(SND_ENABLE),1)
mm-audio-mvs-inc += $(TARGET_OUT_HEADERS)/snd/inc
endif
mm-audio-mvs-inc += $(TARGET_OUT_HEADERS)/mvs/inc
mm-audio-mvs-inc += $(TARGET_OUT_HEADERS)/oncrpc/inc
mm-audio-mvs-inc += $(TARGET_OUT_HEADERS)/diag/include
ifeq ($(USE_MVS_DRIVER), true)
mm-audio-mvs-inc += $(AUDIO_ROOT)/audio-alsa/inc
mm-audio-mvs-inc += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
endif

# ---------------------------------------------------------------------------------
#                       Make the apps-test (mm-audio-mvs-test)
# ---------------------------------------------------------------------------------

mm-mvs-native-inc       := $(LOCAL_PATH)
LOCAL_MODULE            := mm-audio-mvs-test
LOCAL_MODULE_TAGS       := optional
LOCAL_CFLAGS            := $(mm-audio-mvs-def)
LOCAL_C_INCLUDES        := $(mm-audio-mvs-inc)
LOCAL_PRELINK_MODULE    := false
LOCAL_SRC_FILES         := test/mvs_loopback_test.c
ifeq ($(SND_ENABLE),1)
LOCAL_SHARED_LIBRARIES  := libdsm libqueue liboncrpc libmvs libsnd
endif
ifeq ($(AUDIO_V2), true)
LOCAL_SHARED_LIBRARIES  := libaudioalsa
endif

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

include $(BUILD_EXECUTABLE)

endif

endif
endif
# ---------------------------------------------------------------------------------
#                                       END
# ---------------------------------------------------------------------------------
endif

