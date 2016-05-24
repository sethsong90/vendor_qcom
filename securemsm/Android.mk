ifeq ($(call is-board-platform-in-list,msm8974 msm8960 msm8660 msm8226 msm8610),true)

include $(call all-subdir-makefiles)

endif # end filter
