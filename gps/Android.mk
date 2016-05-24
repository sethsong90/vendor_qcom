ifneq ($(BUILD_TINY_ANDROID),true)
ifneq ($(BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE),)
ifneq ($(call is-board-platform,copper),true)

LOCAL_PATH := $(call my-dir)

# note the combination of premium_enabled and service_mask controls
# the enable/disable of main components of location services
# 1: enable, otherwise: disable
ADDITIONAL_BUILD_PROPERTIES += ro.qc.sdk.izat.premium_enabled=1
# Bitwise OR the features to enable them as needed
# 0x1: GTP WiFi
# 0x2: GTP WWAN Lite
# 0x4: PIP
ADDITIONAL_BUILD_PROPERTIES += ro.qc.sdk.izat.service_mask=0x5

# This property is used by GPS HAL and Wiper to cooperate with QC NLP
# note: this property must be aligned with persist.loc.nlp_name
# 1: QC Network Location Provider is in use. Note persist.loc.nlp_name must be set.
# 0: otherwise. Note persist.loc.nlp_name must be cleared/commented out.
ADDITIONAL_BUILD_PROPERTIES += persist.gps.qc_nlp_in_use=1

# package name of QC NLP, if so chosen in persist.gps.qc_nlp_in_use
# note: this property must be aligned with persist.gps.qc_nlp_in_use,
#       for LocationManagerService.java is controlled by this property only
# note: the length limit for value is 92 characters
ADDITIONAL_BUILD_PROPERTIES += persist.loc.nlp_name=com.qualcomm.services.location

# This property will decouple the "WiFi & Mobile Network Location" from AGPS database setting(Settings.Global.ASSISTED_GPS_ENABLED)
# 1: AGPS operation is controlled by Settings.Global.ASSISTED_GPS_ENABLED.
#      Recommended for all OEMs who don't use Google SUPL servers.
# 0: AGPS operation is controlled by "WiFi & Mobile Network Location" setting on Android UI
#      Recommended for everybody who use Google SUPL servers.
ADDITIONAL_BUILD_PROPERTIES += ro.gps.agps_provider=1

# Select which RPC lib to link to
LOC_API_USE_LOCAL_RPC:=0
LOC_API_USE_QCOM_AUTO_RPC:=1

# Target-specific makefile
GPS_BUILD_DIR:=$(LOCAL_PATH)/build
GPS_MAKE_INC:=$(TARGET_BOARD_PLATFORM).in
ifeq (, $(wildcard $(GPS_BUILD_DIR)/$(TARGET_BOARD_PLATFORM)*))
   GPS_MAKE_INC=unsupported.in
endif

include $(GPS_BUILD_DIR)/$(GPS_MAKE_INC) $(call all-subdir-makefiles)

endif # is-board-platform
endif # BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE
endif # BUILD_TINY_ANDROID
