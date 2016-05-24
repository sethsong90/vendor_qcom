ifeq ($(TARGET_ARCH),arm)
ifeq ($(call is-board-platform-in-list,msm8960),true)
MDM_HELPER_ROOT := $(call my-dir)
include $(MDM_HELPER_ROOT)/mdmfiletransfer/Android.mk
LOCAL_PATH := $(MDM_HELPER_ROOT)
include $(CLEAR_VARS)
LOCAL_MODULE := mdm_helper
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
		    $(LOCAL_PATH)/mdmfiletransfer \
		    $(TARGET_OUT_HEADERS)/common/inc/
LOCAL_ADDITIONAL_DEPENDENCIES += ${TARGET_OUT_INTERMEDIATES}/KERNEL_OBJ/usr
LOCAL_SRC_FILES := mdm_helper.c qsc_helper.c 9x15_helper.c
LOCAL_SHARED_LIBRARIES := libc libcutils libutils
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES := libmdmdload
LOCAL_CFLAGS += -Wall -pthread
ifneq ($(call is-platform-sdk-version-at-least,17),true)
LOCAL_CFLAGS += -include bionic/libc/kernel/arch-arm/asm/posix_types.h
LOCAL_CFLAGS += -include bionic/libc/kernel/arch-arm/asm/byteorder.h
LOCAL_CFLAGS += -include bionic/libc/kernel/common/linux/posix_types.h
LOCAL_CFLAGS += -include bionic/libc/kernel/common/linux/types.h
LOCAL_CFLAGS += -include bionic/libc/kernel/common/linux/socket.h
LOCAL_CFLAGS += -include bionic/libc/kernel/common/linux/in.h
LOCAL_CFLAGS += -include bionic/libc/kernel/common/linux/netlink.h
LOCAL_CFLAGS += -include bionic/libc/kernel/common/linux/un.h
endif
include $(BUILD_EXECUTABLE)
endif
endif
