ifneq (true,$(TARGET_BUILD_PDK))
  ifeq ($(call is-board-platform-in-list,msm8960 msm8974 apq8084),true)
    include $(call all-subdir-makefiles)
  endif
endif
