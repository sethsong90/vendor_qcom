# ---------------------------------------------------------------------------------
#                       Make the pp daemon (mm-pp-daemon)
# ---------------------------------------------------------------------------------

LOCAL_PATH := $(call my-dir)

ifneq ($(BUILD_TINY_ANDROID),true)

include $(CLEAR_VARS)

mm-pp-def := \
        -g -O3 -Dlrintf=_ffix_r \
        -D__align=__alignx \
        -D__alignx\(x\)=__attribute__\(\(__aligned__\(x\)\)\) \
        -D_POSIX_SOURCE -DPOSIX_C_SOURCE=199506L \
        -D_XOPEN_SOURCE=600 -D_XOPEN_SOURCE_EXTENDED=1 \
        -D_BSD_SOURCE=1 -D_SVID_SOURCE=1 \
        -D_GNU_SOURCE -DT_ARM -DQC_MODIFIED \
        -Dinline=__inline -DASSERT=ASSERT_FATAL \
        -Dsvcsm_create=svcrtr_create \
        -DTRACE_ARM_DSP -D_ANDROID_ \
        -DIPL_DEBUG_STANDALONE -DVERBOSE -D_DEBUG \

ifeq ($(call is-board-platform,msm8960),true)
    mm-pp-def += -DHW_MAX_BIN_128
endif

ALS_PRODUCT_LIST := msm8974
ALS_PRODUCT_LIST += msm8226
ALS_PRODUCT_LIST += apq8084

TARGET_WITH_ALS := $(call is-board-platform-in-list,$(ALS_PRODUCT_LIST))

ifeq ($(TARGET_WITH_ALS),true)
    mm-pp-def += -DALS_ENABLE
endif

TARGET_8X10_LIST := msm8610
TARGET_8X10 := $(call is-board-platform-in-list,$(TARGET_8X10_LIST))

ifeq ($(TARGET_8X10),true)
    mm-pp-def += -DNATIVE_ALS_ENABLE
endif

mm-pp-daemon-inc   := $(LOCAL_PATH)/inc
mm-pp-daemon-inc   += $(TARGET_OUT_HEADERS)/pp/inc
mm-pp-daemon-inc   += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
mm-pp-daemon-inc   += $(TOP)/hardware/qcom/display/libqservice
mm-pp-daemon-inc   += external/zlib
mm-pp-daemon-inc   += hardware/qcom/display/libqdutils

ifeq ($(TARGET_WITH_ALS),true)
mm-pp-daemon-inc   += $(TARGET_OUT_HEADERS)/qmi/inc
mm-pp-daemon-inc   += $(TARGET_OUT_HEADERS)/sensors/inc
mm-pp-daemon-inc   += $(TARGET_OUT_HEADERS)/sensors/dsps/api
mm-pp-daemon-inc   += $(TARGET_OUT_HEADERS)/qmi/core/lib/inc
endif

LOCAL_MODULE        := mm-pp-daemon
LOCAL_MODULE_TAGS   := optional
LOCAL_CFLAGS        := $(mm-pp-def)

LOCAL_CFLAGS        += -include bionic/libc/kernel/arch-arm/asm/posix_types.h
ifneq ($(call is-platform-sdk-version-at-least,17),true)
  # Override socket-related kernel headers with Bionic version before JB MR1
  LOCAL_CFLAGS        += -include bionic/libc/kernel/arch-arm/asm/byteorder.h
  LOCAL_CFLAGS        += -include bionic/libc/kernel/common/linux/posix_types.h
  LOCAL_CFLAGS        += -include bionic/libc/kernel/common/linux/types.h
  LOCAL_CFLAGS        += -include bionic/libc/kernel/common/linux/socket.h
  LOCAL_CFLAGS        += -include bionic/libc/kernel/common/linux/in.h
endif

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_C_INCLUDES    := $(mm-pp-daemon-inc)
LOCAL_C_INCLUDES    += $(TARGET_OUT_HEADERS)/common/inc
LOCAL_SRC_FILES     := src/pp_daemon.cpp src/parser.cpp src/DummySensor.cpp src/NativeLightSensor.cpp
LOCAL_SHARED_LIBRARIES  := libdisp-aba libmm-abl libmm-abl-oem liblog libcutils libz
LOCAL_SHARED_LIBRARIES  += libqservice libbinder libutils libqdutils libandroid

ifeq ($(TARGET_WITH_ALS),true)
LOCAL_SRC_FILES    += src/LightSensor.cpp
LOCAL_SHARED_LIBRARIES  += libc libsensor1
endif

ifeq ($(TARGET_ARCH),arm)

DCM_PRODUCT_LIST := msm8974
DCM_PRODUCT_LIST += msm8226

TARGET_WITH_DCM := $(call is-board-platform-in-list,$(DCM_PRODUCT_LIST))

ifneq (,$(strip $(TARGET_WITH_DCM)))

LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/diag/include
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/common/inc

LOCAL_SRC_FILES += src/Calib.cpp
else
LOCAL_SRC_FILES += src/DummyCalib.cpp

endif
endif
LOCAL_MODULE_OWNER := qcom

include $(BUILD_EXECUTABLE)
endif
