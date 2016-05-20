LOCAL_PATH := $(call my-dir)

ifneq ($(QCPATH),)
include $(call all-subdir-makefiles,$(LOCAL_PATH))
endif
