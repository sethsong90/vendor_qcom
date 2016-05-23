ROOT_DIR := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_PATH := $(ROOT_DIR)

# ---------------------------------------------------------------------------------
#            Common definitons
# ---------------------------------------------------------------------------------

libmm-wfd-def := -DQCOM_OMX_VENC_EXT
libmm-wfd-def += -O3
libmm-wfd-def += -D_ANDROID_
libmm-wfd-def += -D_ANDROID_LOG_
libmm-wfd-def += -D_ANDROID_LOG_ERROR
libmm-wfd-def += -D_ANDROID_LOG_PROFILE
libmm-wfd-def += -Du32="unsigned int"
libmm-wfd-def += -DENABLE_WFD_STATS
libmm-wfd-def += -DENABLE_V4L2_DUMP
libmm-wfd-def += -DWFD_DUMP_AUDIO_DATA
libmm-wfd-def += -DAAC_DUMP_ENABLE

ifeq ($(call is-android-codename,ICECREAM_SANDWICH),true)
libmm-wfd-def += -DWFD_ICS
endif

# ---------------------------------------------------------------------------------
#            MM-FRAMEWORK INC
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

mm-wfd-inc := $(LOCAL_PATH)/inc
mm-wfd-inc += $(LOCAL_PATH)/../utils/inc
mm-wfd-inc += $(LOCAL_PATH)/../../interface/inc
mm-wfd-inc += $(LOCAL_PATH)/../../hdcp/common/inc
mm-wfd-inc += $(LOCAL_PATH)/../../../utils/inc
mm-wfd-inc += $(TARGET_OUT_HEADERS)/common/inc
mm-wfd-inc += $(TOP)/external/aac/libAACenc/include
mm-wfd-inc += $(TOP)/external/aac/libSYS/include
mm-wfd-inc += $(TARGET_DISPLAY_HAL_PATH)/libqservice/

ifeq ($(call is-android-codename,ICECREAM_SANDWICH),true)
mm-wfd-inc += $(TOP)/frameworks/base/include/media/stagefright
mm-wfd-inc += $(TOP)/frameworks/base/media/libstagefright/include
mm-wfd-inc += $(TOP)/frameworks/base/include/utils
else
mm-wfd-inc += $(TOP)/frameworks/av/media/libstagefright/include
ifeq ("$(PLATFORM_VERSION)","4.3")
mm-wfd-inc += $(TOP)/frameworks/native/include/utils
else
mm-wfd-inc += $(TOP)/system/core/include/utils
mm-wfd-inc += $(TOP)/system/core/include/system
endif
mm-wfd-inc += $(TOP)/frameworks/native/include/gui
mm-wfd-inc += $(TOP)/frameworks/av/include
mm-wfd-inc += $(TOP)/hardware/libhardware_legacy/include/hardware_legacy
mm-wfd-inc += $(TOP)/external/tinyalsa/include
endif
mm-wfd-inc += $(TARGET_OUT_HEADERS)/mm-core/omxcore
mm-wfd-inc += $(TARGET_OUT_HEADERS)/mm-osal/include
mm-wfd-inc += $(TARGET_OUT_HEADERS)/mm-rtp/encoder
mm-wfd-inc += $(TOP)/external/tinyalsa/include
mm-wfd-inc += $(TARGET_OUT_HEADERS)/mm-rtp/common
mm-wfd-inc += $(TARGET_OUT_HEADERS)/mm-mux
mm-wfd-inc += $(TARGET_OUT_HEADERS)/mm-parser/include
mm-wfd-inc += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
mm-wfd-inc += $(LOCAL_PATH)/../../../uibc/interface/inc
LOCAL_MODULE :=  libwfdmmsrc
LOCAL_CFLAGS := $(libmm-wfd-def)
LOCAL_C_INCLUDES := $(mm-wfd-inc)

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

# ---------------------------------------------------------------------------------
#            MM-FRAMEWORK SHARED LIB
# ---------------------------------------------------------------------------------

LOCAL_SHARED_LIBRARIES := libmm-omxcore
LOCAL_SHARED_LIBRARIES +=  libOmxVenc
LOCAL_SHARED_LIBRARIES += libbinder
LOCAL_SHARED_LIBRARIES += libmmosal
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libmmrtpencoder
LOCAL_SHARED_LIBRARIES += libFileMux
LOCAL_SHARED_LIBRARIES += libwfdmmutils
LOCAL_SHARED_LIBRARIES += libc
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libtinyalsa
LOCAL_SHARED_LIBRARIES += libstagefright_enc_common
LOCAL_SHARED_LIBRARIES += libmedia
LOCAL_SHARED_LIBRARIES += libgui
LOCAL_SHARED_LIBRARIES += libwfdhdcpcp
LOCAL_SHARED_LIBRARIES += libwfdcommonutils
LOCAL_SHARED_LIBRARIES += libaudioparameter
LOCAL_SHARED_LIBRARIES += libstagefright_soft_aacenc
LOCAL_SHARED_LIBRARIES += libqservice

LOCAL_MODULE_TAGS := optional

# ---------------------------------------------------------------------------------
#            MM-FRAMEWORK SRC
# ---------------------------------------------------------------------------------

LOCAL_SRC_FILES := src/WFDMMSource.cpp
LOCAL_SRC_FILES += src/WFDMMSourceAudioSource.cpp
LOCAL_SRC_FILES += src/WFDMMSourceScreenSource.cpp
LOCAL_SRC_FILES += src/WFDMMSourceMux.cpp
LOCAL_SRC_FILES += src/WFDMMIonMemory.cpp

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------------
#      END
# ---------------------------------------------------------------------------------
