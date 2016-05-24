ifeq ($(call is-vendor-board-platform,QCOM),true)

# Build only if board has BT/FM/WLAN
ifeq ($(findstring true, $(BOARD_HAVE_QCOM_FM) $(BOARD_HAVE_BLUETOOTH) $(BOARD_HAS_ATH_WLAN_AR6320)),true)

LOCAL_PATH:= $(call my-dir)

BDROID_DIR:= external/bluetooth/bluedroid

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(TARGET_OUT_HEADERS)/diag/include \
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/common/inc \
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/bt/hci_qcomm_init \
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
LOCAL_C_INCLUDES += $(BDROID_DIR)/hci/include \

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_CFLAGS:= \
              -DANDROID

LOCAL_CFLAGS += -include bionic/libc/kernel/arch-arm/asm/posix_types.h
LOCAL_CFLAGS += -include bionic/libc/kernel/arch-arm/asm/byteorder.h
LOCAL_CFLAGS += -include bionic/libc/kernel/common/linux/posix_types.h
LOCAL_CFLAGS += -include bionic/libc/kernel/common/linux/types.h
LOCAL_CFLAGS += -include bionic/libc/include/sys/socket.h
LOCAL_CFLAGS += -include bionic/libc/include/netinet/in.h

LOCAL_SRC_FILES:= \
	ftm_main.c \
	ftm_bt.c \
	ftm_bt_power_pfal_linux.c \
	ftm_fm.c \
	ftm_fm_pfal_linux.c \
	ftm_bt_hci_pfal_linux.c \
	ftm_bt_persist.cpp \
	ftm_ant.c \
	ftm_nfc.c

ifeq ($(findstring true, $(BOARD_HAS_ATH_WLAN) $(BOARD_HAS_ATH_WLAN_AR6320)),true)
LOCAL_CFLAGS += -DATH_WLAN
LOCAL_STATIC_LIBRARIES += libtcmd
LOCAL_SHARED_LIBRARIES += libnl_2
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/libtcmd
LOCAL_SRC_FILES  += ftm_wlan.c
endif

LOCAL_SHARED_LIBRARIES += libdl
LOCAL_LDLIBS += -lpthread
LOCAL_MODULE:= ftmdaemon
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES  += libdiag
LOCAL_SHARED_LIBRARIES  += libbtnv libcutils libhardware

# By default NV persist gets used
LOCAL_CFLAGS += -DBT_NV_SUPPORT

LDFLAGS += -ldl

include $(BUILD_EXECUTABLE)
include $(call all-makefiles-under,$(LOCAL_PATH))
endif # filter
endif # is-vendor-board-platform
