FASTCV_BOARD_PLATFORM_LIST := msm8960
FASTCV_BOARD_PLATFORM_LIST += msm7627a
FASTCV_BOARD_PLATFORM_LIST += msm8974
FASTCV_BOARD_PLATFORM_LIST += msm8226

ifeq ($(call is-board-platform-in-list,$(FASTCV_BOARD_PLATFORM_LIST)),true)

include $(call all-subdir-makefiles)
endif
