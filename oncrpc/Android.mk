ifeq ($(call is-vendor-board-platform,QCOM),true)
ifneq ($(TARGET_NO_RPC),true)
include $(call all-subdir-makefiles)
endif
endif
