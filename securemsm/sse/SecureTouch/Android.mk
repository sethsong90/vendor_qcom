ifeq ($(call is-board-platform-in-list, msm8974 apq8084),true)
  include $(call all-subdir-makefiles)
endif

