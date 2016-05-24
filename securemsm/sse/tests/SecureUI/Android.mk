ifeq ($(call is-board-platform-in-list,msm8974),true)

include $(call all-subdir-makefiles)

endif # end filter
