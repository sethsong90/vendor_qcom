ifeq ($(GPS_LOC_API_V02_ENABLED),true)
ifneq ($(FEATURE_GSIFF),false)
include $(call all-subdir-makefiles)
endif
endif
