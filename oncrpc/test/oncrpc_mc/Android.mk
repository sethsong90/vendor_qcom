ifeq ($(call is-board-platform,msm8660),true)
include $(call all-subdir-makefiles)
endif
