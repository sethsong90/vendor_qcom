# This file lists all qcom products and defines the QC_PROP flag which
# is used to enable projects inside $(QC_PROP_ROOT) directory.

# Also, This file intended for use by device/product makefiles
# to pick and choose the optional proprietary modules

# Root of Qualcomm Proprietary component tree
QC_PROP_ROOT := vendor/qcom/proprietary

PRODUCT_LIST := msm7625_surf
PRODUCT_LIST += msm7625_ffa
PRODUCT_LIST += msm7627_6x
PRODUCT_LIST += msm7627_ffa
PRODUCT_LIST += msm7627_surf
PRODUCT_LIST += msm7627a
PRODUCT_LIST += msm8625
PRODUCT_LIST += msm7630_surf
PRODUCT_LIST += msm7630_1x
PRODUCT_LIST += msm7630_fusion
PRODUCT_LIST += msm8660_surf
PRODUCT_LIST += qsd8250_surf
PRODUCT_LIST += qsd8250_ffa
PRODUCT_LIST += qsd8650a_st1x
PRODUCT_LIST += msm8660_csfb
PRODUCT_LIST += msm8960
PRODUCT_LIST += msm8974
PRODUCT_LIST += mpq8064
PRODUCT_LIST += msm8610
PRODUCT_LIST += msm8226
PRODUCT_LIST += apq8084
PRODUCT_LIST += mpq8092
PRODUCT_LIST += msm_bronze
PRODUCT_LIST += msm8916

MSM7K_PRODUCT_LIST := msm7625_surf
MSM7K_PRODUCT_LIST += msm7625_ffa
MSM7K_PRODUCT_LIST += msm7627_6x
MSM7K_PRODUCT_LIST += msm7627_ffa
MSM7K_PRODUCT_LIST += msm7627_surf
MSM7K_PRODUCT_LIST += msm7627a
MSM7K_PRODUCT_LIST += msm7630_surf
MSM7K_PRODUCT_LIST += msm7630_1x
MSM7K_PRODUCT_LIST += msm7630_fusion

FOTA_PRODUCT_LIST := msm7627a

ifneq (, $(filter $(PRODUCT_LIST), $(TARGET_PRODUCT)))

  include device/qcom/$(TARGET_PRODUCT)/BoardConfig.mk

  ifeq ($(call is-board-platform,msm8660),true)
    PREBUILT_BOARD_PLATFORM_DIR := msm8660_surf
  else ifeq ($(TARGET_PRODUCT),msm8625)
    PREBUILT_BOARD_PLATFORM_DIR := msm8625
  else ifeq ($(TARGET_PRODUCT),mpq8064)
    PREBUILT_BOARD_PLATFORM_DIR := mpq8064
  else
    PREBUILT_BOARD_PLATFORM_DIR := $(TARGET_BOARD_PLATFORM)
  endif

  $(call inherit-product-if-exists, $(QC_PROP_ROOT)/common-noship/etc/device-vendor-noship.mk)
  $(call inherit-product-if-exists, $(QC_PROP_ROOT)/prebuilt_grease/target/product/$(PREBUILT_BOARD_PLATFORM_DIR)/prebuilt.mk)
  $(call inherit-product-if-exists, $(QC_PROP_ROOT)/prebuilt_HY11/target/product/$(PREBUILT_BOARD_PLATFORM_DIR)/prebuilt.mk)
  $(call inherit-product-if-exists, $(QC_PROP_ROOT)/prebuilt_HY22/target/product/$(PREBUILT_BOARD_PLATFORM_DIR)/prebuilt.mk)

  ifeq ($(wildcard $(QC_PROP_ROOT)/common-noship/etc/device-vendor-noship.mk),)
    ifneq ($(call is-android-codename-in-list,GINGERBREAD HONEYCOMB),true)
      APN_PATH := vendor/qcom/proprietary/common/etc
      PRODUCT_COPY_FILES := $(APN_PATH)/apns-conf.xml:system/etc/apns-conf.xml $(PRODUCT_COPY_FILES)
    endif
  endif

  ifeq ($(BUILD_TINY_ANDROID),true)
    VENDOR_TINY_ANDROID_PACKAGES := $(QC_PROP_ROOT)/diag
    VENDOR_TINY_ANDROID_PACKAGES += $(QC_PROP_ROOT)/kernel-tests
    VENDOR_TINY_ANDROID_PACKAGES += $(QC_PROP_ROOT)/modem-apis
    VENDOR_TINY_ANDROID_PACKAGES += $(QC_PROP_ROOT)/oncrpc
    VENDOR_TINY_ANDROID_PACKAGES += $(QC_PROP_ROOT)/common/build/remote_api_makefiles
    VENDOR_TINY_ANDROID_PACKAGES += $(QC_PROP_ROOT)/common/build/fusion_api_makefiles
    VENDOR_TINY_ANDROID_PACKAGES += $(QC_PROP_ROOT)/common/config

    ifeq ($(call is-board-platform-in-list,$(MSM7K_PRODUCT_LIST)),true)
      VENDOR_TINY_ANDROID_PACKAGES += $(QC_PROP_ROOT)/gps
      VENDOR_TINY_ANDROID_PACKAGES += hardware
      VENDOR_TINY_ANDROID_PACKAGES += external/wpa_supplicant
    endif

  endif # BUILD_TINY_ANDROID
endif

ifeq ($(call is-board-platform-in-list,$(FOTA_PRODUCT_LIST)),true)
  TARGET_FOTA_UPDATE_LIB := libipth libipthlzmadummy
  TARGET_HAS_FOTA        := true
endif

# Include the QRD extensions.
ifeq ($(call is-board-platform-in-list,msm8610 msm8226),true)
$(call inherit-product-if-exists, vendor/qcom/proprietary/qrdplus/Extension/products.mk)
endif

# Include the QRD customer extensions for ChinaMobile.
ifeq ($(call is-board-platform-in-list,msm8610 msm8226),true)
$(call inherit-product-if-exists, vendor/qcom/proprietary/qrdplus/ChinaMobile/products.mk)
endif

# Include the QRD customer extensions for ChinaTelecom.
ifeq ($(call is-board-platform-in-list,msm8610 msm8226),true)
$(call inherit-product-if-exists, vendor/qcom/proprietary/qrdplus/ChinaTelecom/products.mk)
endif

# Include the QRD customer extensions for ChinaUnicom.
ifeq ($(call is-board-platform-in-list,msm8610 msm8226),true)
$(call inherit-product-if-exists, vendor/qcom/proprietary/qrdplus/ChinaUnicom/products.mk)
endif

#Add logkit module to build
TARGET_USES_LOGKIT_LOGGING := true

# Each line here corresponds to an optional LOCAL_MODULE built by
# Android.mk(s) in the proprietary projects. Where project
# corresponds to the vars here in CAPs.

# These modules are tagged with optional as their LOCAL_MODULE_TAGS
# wouldn't be present in your on target images, unless listed here
# explicitly.

#ADC
ADC := qcci_adc_test
ADC += libqcci_adc

#ADSPRPC
ADSPRPC := libadsprpc
ADSPRPC += adsprpcd

#ALLJOYN
ALLJOYN := liballjoyn

#AIVPLAY
AIVPLAY := libaivdrmclient
AIVPLAY += libAivPlay

#Backup Agent
BACKUP_AGENT := QtiBackupAgent

#CameraHawk  App
CAMERAHAWK_APPS := CameraHawk
CAMERAHAWK_APPS += libamui
CAMERAHAWK_APPS += libarcimgfilters
CAMERAHAWK_APPS += libarcimgutils
CAMERAHAWK_APPS += libarcimgutilsbase
CAMERAHAWK_APPS += libarcutils
CAMERAHAWK_APPS += libhdr
CAMERAHAWK_APPS += libphotofix
CAMERAHAWK_APPS += libpluginadaptor
CAMERAHAWK_APPS += libquickview
CAMERAHAWK_APPS += libstory
CAMERAHAWK_APPS += libvideopmk

#Perfect365 App
PERFECT365_APPS := Perfect365
PERFECT365_APPS += libArcSoftFlawlessFace

#Whip Apps
WHIP_APPS := WhipForPhone
WHIP_APPS += libamuiwhip
WHIP_APPS += libarcwhipimgutils
WHIP_APPS += libattextengine
WHIP_APPS += libsns_album
WHIP_APPS += libarcwhipimgutilsbase
WHIP_APPS += libclipmap3

# camerahawk,Whip Common Lib
CAMERAHAWK_WHIP_COMMON_LIB := libarcplatform

#Atmel's mxt cfg file
MXT_CFG := mxt1386e_apq8064_liquid.cfg
MXT_CFG += mxt224_7x27a_ffa.cfg

#BATTERY_CHARGING
BATTERY_CHARGING := battery_charging

#BT
BT := btnvtool
BT += dun-server
BT += hci_qcomm_init
BT += liboi_sbc_decoder
BT += PS_ASIC.pst
BT += RamPatch.txt
BT += sapd
BT += rampatch_tlv.img
BT += nvm_tlv.bin
BT += rampatch_tlv_1.3.tlv
BT += nvm_tlv_1.3.bin
BT += rampatch_tlv_2.1.tlv
BT += nvm_tlv_2.1.bin
BT += wcnss_filter

#BT BLUEZ
BT += bluetoothd
BT += bttest
BT += libbluedroid
BT += libbluetooth
BT += libbluetoothd
BT += stack.conf

#CCID
CCID := ccid_daemon

#CHARGER_MONITOR
CHARGER_MONITOR := charger_monitor

#CNE
CNE := andsfCne.xml
CNE += cnd
CNE += cneapiclient
CNE += cneapiclient.xml
CNE += com.quicinc.cne
CNE += com.quicinc.cneapiclient
CNE += libcneapiclient
CNE += libcneconn
CNE += libcneqmiutils
CNE += libcneutils
CNE += libNimsWrap
CNE += libxml
CNE += NsrmConfiguration.xml
CNE += SwimConfig.xml
CNE += testclient
CNE += testserver

#CRASH_LOGGER
CRASH_LOGGER :=  libramdump

#DATA
DATA := ATFWD-daemon
DATA += bridgemgrd
DATA += CKPD-daemon
DATA += dfa_test_1
DATA += dfa_test_2
DATA += dfa_test_3
DATA += dfa_test_4
DATA += dfa_test_5
DATA += dfa_test_6
DATA += ds_fmc_appd
DATA += dsdnsutil
DATA += dsi_config.xml
DATA += libconfigdb
DATA += libdsnet
DATA += libdsnetutil
DATA += libdsprofile
DATA += libdss
DATA += libdssock
DATA += libdsutils
DATA += libnetmgr
DATA += netmgrd
DATA += netmgr_config.xml
DATA += nl_listener
DATA += port-bridge
DATA += radish
DATA += usbhub
DATA += usbhub_init

#DISPLAY_TESTS
DISPLAY_TESTS := MutexTest

#FASTCV
FASTCV := libapps_mem_heap
FASTCV += libdspCV_skel
FASTCV += libfastcvadsp
FASTCV += libfastcvadsp_skel
FASTCV += libfastcvadsp_stub
FASTCV += libfastcvopt

#FASTMMI
FASTMMI := mmi
FASTMMI += mmi.cfg
FASTMMI += fail.png
FASTMMI += pass.png
FASTMMI += libmmi
FASTMMI += mmi_audio
FASTMMI += mmi_battery
FASTMMI += mmi_bt
FASTMMI += mmi_camera
FASTMMI += mmi_flashlight
FASTMMI += mmi_fm
FASTMMI += mmi_gps_garden
FASTMMI += mmi_gps
FASTMMI += mmi_gsensor
FASTMMI += mmi_gyroscope
FASTMMI += mmi_keypadbacklight
FASTMMI += mmi_lcd
FASTMMI += mmi_led
FASTMMI += mmi_lsensor
FASTMMI += mmi_msensor
FASTMMI += mmi_psensor
FASTMMI += mmi_sdcard
FASTMMI += mmi_sim
FASTMMI += mmi_touchpanel
FASTMMI += mmi_touch
FASTMMI += mmi_vibrator
FASTMMI += mmi_volume
FASTMMI += mmi_wifi

#FLASH
FLASH := install_flash_player.apk
FLASH += libflashplayer.so
FLASH += libstagefright_froyo.so
FLASH += libstagefright_honeycomb.so
FLASH += libysshared.so
FLASH += oem_install_flash_player.apk

#FM
FM := fmconfig
FM += fmfactorytest
FM += fmfactorytestserver
FM += fm_qsoc_patches

#FOTA
FOTA := ipth_dua
FOTA += libcrc32.a
FOTA += libdua.a
FOTA += libdme_main
FOTA += libidev_dua.a
FOTA += libipth.a
FOTA += libshared_lib.a
FOTA += libzlib.1.2.1.a
FOTA += MobileUpdateClient.apk

#FTM
FTM := ftmdaemon
FTM += wdsdaemon

#DIAG
DIAG := diag_callback_client
DIAG += diag_dci_sample
DIAG += diag_klog
DIAG += diag_mdlog
DIAG += diag_socket_log
DIAG += diag_qshrink4_daemon
DIAG += diag_uart_log
DIAG += PktRspTest
DIAG += test_diag

#ENTERPRISE_SECURITY
ENTERPRISE_SECURITY := security_boot_check
ENTERPRISE_SECURITY += security-bridge
ENTERPRISE_SECURITY += lib_nqs
ENTERPRISE_SECURITY += nqs
ENTERPRISE_SECURITY += qsb-port

#GPS
GPS := loc_api_app
GPS += test_loc_api_client
GPS += test_bit
GPS += gpsone_daemon
GPS += test_agps_server
GPS += izat.conf
GPS += sap.conf
GPS += com.qualcomm.location.vzw_library.xml
GPS += libloc_ext
GPS += ipepb
GPS += ipead_test
GPS += iwss_test
GPS += xtra_t_app
GPS += libloc_ext_v01
GPS += libloc_ext_v02
GPS += com.qualcomm.location.vzw_library
GPS += com.qualcomm.services.location
GPS += libgeofence
GPS += libxt_v01
GPS += libxt_v02
GPS += libxtadapter
GPS += gsiff_daemon
GPS += com.qualcomm.location.quipslib.xml
GPS += com.qualcomm.location.quipslib
GPS += quipc_main
GPS += quipc_igsn
GPS += libulp
GPS += libulp2
GPS += libquipc_iam
GPS += libquipc_icm
GPS += libquipc_imc
GPS += libquipc_iulp_simulator_api
GPS += libquipc_pf
GPS += libquipc_pe
GPS += libquipc_ipead
GPS += libquips_api_jni
GPS += libquipc_igsd
GPS += libquips_api
GPS += loc_parameter.ini
GPS += pf_test_app
GPS += quipc_os_api_test_1
GPS += quipc_os_api_test_2
GPS += quipc_igsd_tester
GPS += quipc_ipe_test
GPS += quipc_iwmm_test
GPS += quipc.conf
GPS += garden_app
GPS += libalarmservice_jni
GPS += liblocationservice
GPS += libizat_core
GPS += liblbs_core
GPS += com.qualcomm.location
GPS += com.qualcomm.location.xml
GPS += com.qualcomm.criteria.quipsmaptest
GPS += com.qualcomm.quips.criteria.basicapp
GPS += com.qualcomm.msapm
GPS += griffon
GPS += libqmi_client_griffon
GPS += gps-start.sh
GPS += gps-reset.sh
GPS += gnss-soc-rfdev-img.bin
GPS += gnss-fsh.bin
GPS += GFW.img
GPS += libmsapm_jni
GPS += libquipc_os_api
GPS += msap.conf
GPS += libxt_native
GPS += liblowi_client
GPS += libquipc_ulp_adapter
GPS += libwifiscanner
GPS += flp.conf
GPS += flp.$(BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE)

#GPS-XTWiFi
GPS += cacert_location.pem
GPS += libxtwifi_ulp_adaptor
GPS += libxtwifi_zpp_adaptor
GPS += location-mq
GPS += location_osagent
GPS += xtra_root_cert.pem
GPS += xtwifi-client
GPS += xtwifi.conf
GPS += xtwifi-inet-agent
GPS += xtwifi-upload-test
GPS += test-fake-ap
GPS += test-pos-tx
GPS += test-version
GPS += lowi-server
GPS += lowi.conf
GPS += liblowi_client

# GPS-QCA1530
GPS += gnss.qca1530.svcd
GPS += gnss-soc.img
GPS += gnss-soc-data.img
GPS += gnss-soc-rfdev.img
GPS += gnss-soc-es6.img
GPS += gnss-soc-es6-data.img
GPS += gnss-soc-es6-rfdev.img
GPS += gnss-fsh.bin
GPS += gnss.qca1530.start.sh
GPS += gnss.qca1530.detect.sh
GPS += gnss.qca1530.reset.sh
GPS += gnss.qca1530.setenv.sh

#IMS
IMS := exe-ims-regmanagerprocessnative
#IMS += exe-ims-videoshareprocessnative
IMS += lib-imsdpl
IMS += lib-imsfiledemux
IMS += lib-imsfilemux
IMS += lib-imsqimf
IMS += lib-ims-regmanagerbindernative
IMS += lib-ims-regmanagerjninative
IMS += lib-ims-regmanager
#IMS += lib-ims-videosharebindernative
#IMS += lib-ims-videosharejninative
#IMS += lib-ims-videoshare

# IMS Telephony Libs
IMS_TEL := ims.xml
IMS_TEL += imslibrary
IMS_TEL += ims

# BT-Telephony Lib for DSDA
BT_TEL := btmultisim.xml
BT_TEL += btmultisimlibrary
BT_TEL += btmultisim

#IMS_VT
IMS_VT := lib-imsvt
IMS_VT += lib-imscamera

#IMS_SETTINGS
IMS_SETTINGS := lib-imss
IMS_SETTINGS += lib-rcsimssjni

#IMS_RCS
IMS_RCS := lib-imsxml
IMS_RCS += lib-imsrcs
IMS_RCS += lib-rcsjni

#IMS_NEWARCH
IMS_NEWARCH := imsdatadaemon
IMS_NEWARCH += imsqmidaemon
IMS_NEWARCH += ims_rtp_daemon
IMS_NEWARCH += lib-dplmedia
IMS_NEWARCH += lib-imsdpl
IMS_NEWARCH += lib-imsqimf
IMS_NEWARCH += lib-imsSDP
IMS_NEWARCH += lib-rtpcommon
IMS_NEWARCH += lib-rtpcore
IMS_NEWARCH += lib-rtpdaemoninterface
IMS_NEWARCH += lib-rtpsl

#IMS_REGMGR
IMS_REGMGR := RegmanagerApi

#INTERFACE_PERMISSIONS
INTERFACE_PERMISSIONS := InterfacePermissions
INTERFACE_PERMISSIONS += interface_permissions.xml

#IQAGENT
IQAGENT := libiq_service
IQAGENT += libiq_client
IQAGENT += IQAgent
IQAGENT += iqmsd
IQAGENT += iqclient

#MDM_HELPER
MDM_HELPER := mdm_helper

#MM_AUDIO
MM_AUDIO := acdbtest
MM_AUDIO += AudioFilter
MM_AUDIO += libacdbloader
MM_AUDIO += libacdbmapper
MM_AUDIO += libalsautils
MM_AUDIO += libatu
MM_AUDIO += libaudcal
MM_AUDIO += MTP_Bluetooth_cal.acdb
MM_AUDIO += MTP_General_cal.acdb
MM_AUDIO += MTP_Global_cal.acdb
MM_AUDIO += MTP_Handset_cal.acdb
MM_AUDIO += MTP_Hdmi_cal.acdb
MM_AUDIO += MTP_Headset_cal.acdb
MM_AUDIO += MTP_Speaker_cal.acdb
MM_AUDIO += QRD_Bluetooth_cal.acdb
MM_AUDIO += QRD_General_cal.acdb
MM_AUDIO += QRD_Global_cal.acdb
MM_AUDIO += QRD_Handset_cal.acdb
MM_AUDIO += QRD_Hdmi_cal.acdb
MM_AUDIO += QRD_Headset_cal.acdb
MM_AUDIO += QRD_Speaker_cal.acdb
MM_AUDIO += Liquid_Bluetooth_cal.acdb
MM_AUDIO += Liquid_General_cal.acdb
MM_AUDIO += Liquid_Global_cal.acdb
MM_AUDIO += Liquid_Handset_cal.acdb
MM_AUDIO += Liquid_Hdmi_cal.acdb
MM_AUDIO += Liquid_Headset_cal.acdb
MM_AUDIO += Liquid_Speaker_cal.acdb
MM_AUDIO += Fluid_Bluetooth_cal.acdb
MM_AUDIO += Fluid_General_cal.acdb
MM_AUDIO += Fluid_Global_cal.acdb
MM_AUDIO += Fluid_Handset_cal.acdb
MM_AUDIO += Fluid_Hdmi_cal.acdb
MM_AUDIO += Fluid_Headset_cal.acdb
MM_AUDIO += Fluid_Speaker_cal.acdb
MM_AUDIO += libaudioalsa
MM_AUDIO += libaudioeq
MM_AUDIO += libaudioparsers
MM_AUDIO += libcsd-client
MM_AUDIO += lib_iec_60958_61937
MM_AUDIO += libmm-audio-resampler
MM_AUDIO += libOmxAacDec
MM_AUDIO += libOmxAacEnc
MM_AUDIO += libOmxAdpcmDec
MM_AUDIO += libOmxAmrDec
MM_AUDIO += libOmxAmrEnc
MM_AUDIO += libOmxAmrRtpDec
MM_AUDIO += libOmxAmrwbDec
MM_AUDIO += libOmxAmrwbplusDec
MM_AUDIO += libOmxEvrcDec
MM_AUDIO += libOmxEvrcEnc
MM_AUDIO += libOmxEvrcHwDec
MM_AUDIO += libOmxMp3Dec
MM_AUDIO += libOmxQcelp13Dec
MM_AUDIO += libOmxQcelp13Enc
MM_AUDIO += libOmxQcelpHwDec
MM_AUDIO += libOmxVp8Dec
MM_AUDIO += libOmxWmaDec
MM_AUDIO += libvoem-jni
MM_AUDIO += libvoem-test
MM_AUDIO += mm-audio-ctrl-test
MM_AUDIO += mm-audio-voem_if-test
MM_AUDIO += mm-omx-devmgr
MM_AUDIO += QCAudioManager
MM_AUDIO += liblistensoundmodel
MM_AUDIO += liblistenjni
MM_AUDIO += liblisten
MM_AUDIO += liblistenhardware
MM_AUDIO += lns_test
MM_AUDIO += ListenApp
MM_AUDIO += libqcbassboost
MM_AUDIO += libqcvirt
MM_AUDIO += libqcreverb
MM_AUDIO += audio_effects.conf
MM_AUDIO += ftm_test_config

#MM_CAMERA
MM_CAMERA := cpp_firmware_v1_1_1.fw
MM_CAMERA += cpp_firmware_v1_1_6.fw
MM_CAMERA += cpp_firmware_v1_2_0.fw
MM_CAMERA += cpp_firmware_v1_2_A.fw
MM_CAMERA += cpp_firmware_v1_6_0.fw
MM_CAMERA += libactuator_dw9714_camcorder
MM_CAMERA += libactuator_dw9714_camera
MM_CAMERA += libactuator_dw9716_camcorder
MM_CAMERA += libactuator_dw9716_camera
MM_CAMERA += libactuator_iu074_camcorder
MM_CAMERA += libactuator_iu074_camera
MM_CAMERA += libactuator_ov12830_camcorder
MM_CAMERA += libactuator_ov12830_camera
MM_CAMERA += libactuator_ov8825_camcorder
MM_CAMERA += libactuator_ov8825_camera
MM_CAMERA += libactuator_ROHM_BU64243GWZ_camcorder
MM_CAMERA += libactuator_ROHM_BU64243GWZ_camera
MM_CAMERA += libadsp_denoise_skel
MM_CAMERA += libactuator_dw9714_camera
MM_CAMERA += libactuator_dw9714_camcorder
MM_CAMERA += libactuator_dw9716_camera
MM_CAMERA += libactuator_dw9716_camcorder
MM_CAMERA += libactuator_ov8825_camera
MM_CAMERA += libactuator_ov8825_camcorder
MM_CAMERA += libactuator_ROHM_BU64243GWZ_camera
MM_CAMERA += libactuator_ROHM_BU64243GWZ_camcorder
MM_CAMERA += libactuator_ov12830_camera
MM_CAMERA += libactuator_ov12830_camcorder
MM_CAMERA += libactuator_iu074_camera
MM_CAMERA += libactuator_iu074_camcorder
MM_CAMERA += libchromatix_imx074_default_video
MM_CAMERA += libchromatix_imx074_preview
MM_CAMERA += libchromatix_imx074_video_hd
MM_CAMERA += libchromatix_imx074_zsl
MM_CAMERA += libchromatix_imx091_default_video
MM_CAMERA += libchromatix_imx091_preview
MM_CAMERA += libchromatix_imx091_video_hd
MM_CAMERA += libchromatix_imx132_common
MM_CAMERA += libchromatix_imx132_default_video
MM_CAMERA += libchromatix_imx132_liveshot
MM_CAMERA += libchromatix_imx132_preview
MM_CAMERA += libchromatix_imx132_snapshot
MM_CAMERA += libchromatix_imx134_common
MM_CAMERA += libchromatix_imx134_default_video
MM_CAMERA += libchromatix_imx134_hfr_120
MM_CAMERA += libchromatix_imx134_hfr_60
MM_CAMERA += libchromatix_imx134_preview
MM_CAMERA += libchromatix_imx134_snapshot
MM_CAMERA += libchromatix_imx135_common
MM_CAMERA += libchromatix_imx135_default_video
MM_CAMERA += libchromatix_imx135_hfr_60
MM_CAMERA += libchromatix_imx135_hfr_90
MM_CAMERA += libchromatix_imx135_hfr_120
MM_CAMERA += libchromatix_imx135_liveshot
MM_CAMERA += libchromatix_imx135_preview
MM_CAMERA += libchromatix_imx135_snapshot
MM_CAMERA += libchromatix_imx135_video_hd
MM_CAMERA += libchromatix_imx135_zsl
MM_CAMERA += libchromatix_mt9e013_ar
MM_CAMERA += libchromatix_mt9e013_default_video
MM_CAMERA += libchromatix_mt9e013_preview
MM_CAMERA += libchromatix_mt9e013_video_hfr
MM_CAMERA += libchromatix_ov2720_common
MM_CAMERA += libchromatix_ov2720_default_video
MM_CAMERA += libchromatix_ov2720_hfr
MM_CAMERA += libchromatix_ov2720_liveshot
MM_CAMERA += libchromatix_ov2720_preview
MM_CAMERA += libchromatix_ov2720_zsl
MM_CAMERA += libchromatix_ov5647_ar
MM_CAMERA += libchromatix_ov5647_default_video
MM_CAMERA += libchromatix_ov5647_preview
MM_CAMERA += libchromatix_ov5648_oty5f03_common
MM_CAMERA += libchromatix_ov5648_oty5f03_default_video
MM_CAMERA += libchromatix_ov5648_oty5f03_preview
MM_CAMERA += libchromatix_ov5648_oty5f03_snapshot
MM_CAMERA += libchromatix_ov5648_oty5f03_video_hd
MM_CAMERA += libchromatix_ov5648_oty5f03_zsl
MM_CAMERA += libchromatix_ov7692_ar
MM_CAMERA += libchromatix_ov7692_default_video
MM_CAMERA += libchromatix_ov7692_preview
MM_CAMERA += libchromatix_ov8825_7853f_common
MM_CAMERA += libchromatix_ov8825_7853f_default_video
MM_CAMERA += libchromatix_ov8825_7853f_hfr_120fps
MM_CAMERA += libchromatix_ov8825_7853f_hfr_60fps
MM_CAMERA += libchromatix_ov8825_7853f_hfr_90fps
MM_CAMERA += libchromatix_ov8825_7853f_liveshot
MM_CAMERA += libchromatix_ov8825_7853f_preview
MM_CAMERA += libchromatix_ov8825_7853f_snapshot
MM_CAMERA += libchromatix_ov8825_7853f_zsl
MM_CAMERA += libchromatix_ov8825_ar
MM_CAMERA += libchromatix_ov8825_common
MM_CAMERA += libchromatix_ov8825_default_video
MM_CAMERA += libchromatix_ov8825_hfr
MM_CAMERA += libchromatix_ov8825_hfr_120fps
MM_CAMERA += libchromatix_ov8825_hfr_60fps
MM_CAMERA += libchromatix_ov8825_hfr_90fps
MM_CAMERA += libchromatix_ov8825_liveshot
MM_CAMERA += libchromatix_ov8825_liveshot_hd
MM_CAMERA += libchromatix_ov8825_preview
MM_CAMERA += libchromatix_ov8825_preview_hd
MM_CAMERA += libchromatix_ov8825_snapshot
MM_CAMERA += libchromatix_ov8825_snapshot_hd
MM_CAMERA += libchromatix_ov8825_video_hd
MM_CAMERA += libchromatix_ov8825_zsl
MM_CAMERA += libchromatix_ov8865_q8v18a_common
MM_CAMERA += libchromatix_ov8865_q8v18a_default_video
MM_CAMERA += libchromatix_ov8865_q8v18a_hfr_120fps
MM_CAMERA += libchromatix_ov8865_q8v18a_hfr_60fps
MM_CAMERA += libchromatix_ov8865_q8v18a_hfr_90fps
MM_CAMERA += libchromatix_ov8865_q8v18a_liveshot
MM_CAMERA += libchromatix_ov8865_q8v18a_preview
MM_CAMERA += libchromatix_ov8865_q8v18a_snapshot
MM_CAMERA += libchromatix_ov8865_q8v18a_video_hd
MM_CAMERA += libchromatix_ov8865_q8v18a_zsl
MM_CAMERA += libchromatix_ov9724_common
MM_CAMERA += libchromatix_ov9724_default_video
MM_CAMERA += libchromatix_ov9724_hfr
MM_CAMERA += libchromatix_ov9724_liveshot
MM_CAMERA += libchromatix_ov9724_preview
MM_CAMERA += libchromatix_ov9724_zsl
MM_CAMERA += libchromatix_ov9726_ar
MM_CAMERA += libchromatix_ov9726_default_video
MM_CAMERA += libchromatix_ov9726_preview
MM_CAMERA += libchromatix_s5k3l1yx_common
MM_CAMERA += libchromatix_s5k3l1yx_default_video
MM_CAMERA += libchromatix_s5k3l1yx_hfr_120fps
MM_CAMERA += libchromatix_s5k3l1yx_hfr_60fps
MM_CAMERA += libchromatix_s5k3l1yx_hfr_90fps
MM_CAMERA += libchromatix_s5k3l1yx_liveshot
MM_CAMERA += libchromatix_s5k3l1yx_preview
MM_CAMERA += libchromatix_s5k3l1yx_snapshot
MM_CAMERA += libchromatix_s5k3l1yx_video_hd
MM_CAMERA += libchromatix_s5k3l1yx_zsl
MM_CAMERA += libchromatix_s5k4e1_ar
MM_CAMERA += libchromatix_s5k4e1_default_video
MM_CAMERA += libchromatix_s5k4e1_preview
MM_CAMERA += libchromatix_SKUAA_ST_gc0339_common
MM_CAMERA += libchromatix_SKUAA_ST_gc0339_default_video
MM_CAMERA += libchromatix_SKUAA_ST_gc0339_preview
MM_CAMERA += libchromatix_skuab_shinetech_gc0339_common
MM_CAMERA += libchromatix_skuab_shinetech_gc0339_default_video
MM_CAMERA += libchromatix_skuab_shinetech_gc0339_liveshot
MM_CAMERA += libchromatix_skuab_shinetech_gc0339_preview
MM_CAMERA += libchromatix_skuab_shinetech_gc0339_snapshot
MM_CAMERA += libchromatix_skuab_shinetech_gc0339_zsl
MM_CAMERA += libchromatix_SKUAB_ST_s5k4e1_common
MM_CAMERA += libchromatix_SKUAB_ST_s5k4e1_default_video
MM_CAMERA += libchromatix_SKUAB_ST_s5k4e1_hfr_120fps
MM_CAMERA += libchromatix_SKUAB_ST_s5k4e1_hfr_60fps
MM_CAMERA += libchromatix_SKUAB_ST_s5k4e1_hfr_90fps
MM_CAMERA += libchromatix_SKUAB_ST_s5k4e1_liveshot
MM_CAMERA += libchromatix_SKUAB_ST_s5k4e1_preview
MM_CAMERA += libchromatix_SKUAB_ST_s5k4e1_snapshot
MM_CAMERA += libchromatix_SKUAB_ST_s5k4e1_video_hd
MM_CAMERA += libchromatix_SKUAB_ST_s5k4e1_zsl
MM_CAMERA += libchromatix_skuf_ov12830_p12v01c_common
MM_CAMERA += libchromatix_skuf_ov12830_p12v01c_default_video
MM_CAMERA += libchromatix_skuf_ov12830_p12v01c_hfr_120fps
MM_CAMERA += libchromatix_skuf_ov12830_p12v01c_hfr_60fps
MM_CAMERA += libchromatix_skuf_ov12830_p12v01c_hfr_90fps
MM_CAMERA += libchromatix_skuf_ov12830_p12v01c_preview
MM_CAMERA += libchromatix_skuf_ov12830_p12v01c_snapshot
MM_CAMERA += libchromatix_skuf_ov12830_p12v01c_video_hd
MM_CAMERA += libchromatix_skuf_ov12830_p12v01c_zsl
MM_CAMERA += libchromatix_skuf_ov5648_p5v23c_common
MM_CAMERA += libchromatix_skuf_ov5648_p5v23c_default_video
MM_CAMERA += libchromatix_skuf_ov5648_p5v23c_preview
MM_CAMERA += libchromatix_skuf_ov5648_p5v23c_snapshot
MM_CAMERA += libmmcamera
MM_CAMERA += libmmcamera_cac_lib
MM_CAMERA += libmmcamera_faceproc
MM_CAMERA += libmmcamera_frameproc
MM_CAMERA += libmmcamera_hdr_gb_lib
MM_CAMERA += libmmcamera_hdr_lib
MM_CAMERA += libmmcamera_hi256
MM_CAMERA += libmmcamera_imglib
MM_CAMERA += libmmcamera_imx091
MM_CAMERA += libmmcamera_imx132
MM_CAMERA += libmmcamera_imx134
MM_CAMERA += libmmcamera_imx135
MM_CAMERA += libmmcamera_isp_bpc44
MM_CAMERA += libmmcamera_isp_bcc44
MM_CAMERA += libmmcamera_isp_bf_stats44
MM_CAMERA += libmmcamera_isp_bhist_stats44
MM_CAMERA += libmmcamera_isp_bg_stats44
MM_CAMERA += libmmcamera_isp_cs_stats44
MM_CAMERA += libmmcamera_isp_chroma_enhan40
MM_CAMERA += libmmcamera_isp_chroma_suppress40
MM_CAMERA += libmmcamera_isp_clamp_encoder40
MM_CAMERA += libmmcamera_isp_clamp_viewfinder40
MM_CAMERA += libmmcamera_isp_clf44
MM_CAMERA += libmmcamera_isp_color_correct40
MM_CAMERA += libmmcamera_isp_demosaic40
MM_CAMERA += libmmcamera_isp_demux40
MM_CAMERA += libmmcamera_isp_fovcrop_encoder40
MM_CAMERA += libmmcamera_isp_fovcrop_viewfinder40
MM_CAMERA += libmmcamera_isp_gamma40
MM_CAMERA += libmmcamera_isp_gamma44
MM_CAMERA += libmmcamera_isp_ihist_stats44
MM_CAMERA += libmmcamera_isp_linearization40
MM_CAMERA += libmmcamera_isp_luma_adaptation40
MM_CAMERA += libmmcamera_isp_mce40
MM_CAMERA += libmmcamera_isp_mesh_rolloff44
MM_CAMERA += libmmcamera_isp_rs_stats44
MM_CAMERA += libmmcamera_isp_scaler_encoder44
MM_CAMERA += libmmcamera_isp_scaler_viewfinder44
MM_CAMERA += libmmcamera_isp_sub_module
MM_CAMERA += libmmcamera_isp_wb40
MM_CAMERA += libmmcamera_mt9m114
MM_CAMERA += libmmcamera_ofilm_oty5f03_eeprom
MM_CAMERA += libmmcamera_ov2720
MM_CAMERA += libmmcamera_ov5648_oty5f03
MM_CAMERA += libmmcamera_ov8825
MM_CAMERA += libmmcamera_ov8865_q8v18a
MM_CAMERA += libmmcamera_ov9724
MM_CAMERA += libmmcamera_plugin
MM_CAMERA += libmmcamera_s5k3l1yx
MM_CAMERA += libmmcamera_SKUAA_ST_gc0339
MM_CAMERA += libmmcamera_skuab_shinetech_gc0339
MM_CAMERA += libmmcamera_SKUAB_ST_s5k4e1
MM_CAMERA += libmmcamera_skuf_ov12830_p12v01c
MM_CAMERA += libmmcamera_skuf_ov5648_p5v23c
MM_CAMERA += libmmcamera_sp1628
MM_CAMERA += libmmcamera_statsproc31
MM_CAMERA += libmmcamera_sunny_p12v01m_eeprom
MM_CAMERA += libmmcamera_sunny_p5v23c_eeprom
MM_CAMERA += libmmcamera_sunny_q8v18a_eeprom
MM_CAMERA += libmmcamera_target
MM_CAMERA += libmmcamera_truly_cm7700_eeprom
MM_CAMERA += libmmcamera_tuning
MM_CAMERA += libmmcamera_wavelet_lib
MM_CAMERA += libmmcamera2_frame_algorithm
MM_CAMERA += libmmcamera2_stats_algorithm
MM_CAMERA += libmmcamera2_stats_release
MM_CAMERA += libmmcamera-core
MM_CAMERA += libmm-qcamera
MM_CAMERA += liboemcamera
MM_CAMERA += libqcamera
MM_CAMERA += mm-qcamera-app
MM_CAMERA += mm-qcamera-daemon
MM_CAMERA += mm-qcamera-test
MM_CAMERA += mm-qcamera-testsuite-client
MM_CAMERA += v4l2-qcamera-app
MM_CAMERA += libmmcamera_tintless_algo
MM_CAMERA += libmmcamera_tintless_bg_pca_algo

MM_CAMERA += libmmcamera_ov13850
MM_CAMERA += libmmcamera_sunny_q13v03a_eeprom
MM_CAMERA += libchromatix_ov13850_common
MM_CAMERA += libchromatix_ov13850_preview
MM_CAMERA += libchromatix_ov13850_default_video
MM_CAMERA += libchromatix_ov13850_hfr
MM_CAMERA += libchromatix_ov13850_liveshot
MM_CAMERA += libchromatix_ov13850_hfr_60fps
MM_CAMERA += libchromatix_ov13850_hfr_90fps
MM_CAMERA += libchromatix_ov13850_hfr_120fps
MM_CAMERA += libchromatix_ov13850_snapshot
MM_CAMERA += libchromatix_ov13850_zsl
MM_CAMERA += libchromatix_ov13850_video_hd

MM_CAMERA += libmmcamera_t4k37ab
MM_CAMERA += libmmcamera_f4k37ab_qtech_t4k37_eeprom
MM_CAMERA += libchromatix_t4k37ab_common
MM_CAMERA += libchromatix_t4k37ab_preview
MM_CAMERA += libchromatix_t4k37ab_default_video
MM_CAMERA += libchromatix_t4k37ab_snapshot

MM_CAMERA += libmmcamera_imx214
MM_CAMERA += libchromatix_imx214_common
MM_CAMERA += libchromatix_imx214_preview
MM_CAMERA += libchromatix_imx214_default_video
MM_CAMERA += libchromatix_imx214_liveshot
MM_CAMERA += libchromatix_imx214_hfr_60fps
MM_CAMERA += libchromatix_imx214_hfr_120fps
MM_CAMERA += libchromatix_imx214_snapshot
MM_CAMERA += libchromatix_imx214_snapshot_hdr
MM_CAMERA += libchromatix_imx214_video_hdr


#MM_CORE
MM_CORE := CABLService
MM_CORE += libdisp-aba
MM_CORE += libmm-abl
MM_CORE += libmm-abl-oem
MM_CORE += libscale
MM_CORE += mm-pp-daemon
MM_CORE += mm-omx-cts-test-1.0
MM_CORE += mm-omx-cts-test-1.1
MM_CORE += SVIService
MM_CORE += libmm-hdcpmgr
MM_CORE += libvpu
MM_CORE += libvfmclientutils

#MM_COLOR_CONVERSION
MM_COLOR_CONVERSION := libtile2linear

#MM_COLOR_CONVERTOR
MM_COLOR_CONVERTOR := libmm-color-convertor
MM_COLOR_CONVERTOR += libI420colorconvert

#MM_GESTURES
MM_GESTURES := gesture_mouse.idc
MM_GESTURES += GestureOverlayService
MM_GESTURES += GestureTouchInjectionConfig
MM_GESTURES += GestureTouchInjectionService
MM_GESTURES += libgesture-core
MM_GESTURES += libmmgesture-activity-trigger
MM_GESTURES += libmmgesture-bus2
MM_GESTURES += libmmgesture-camera
MM_GESTURES += libmmgesture-camera-factory
MM_GESTURES += libmmgesture-client
MM_GESTURES += libmmgesture-jni
MM_GESTURES += libmmgesture-linux
MM_GESTURES += libmmgesture-service
MM_GESTURES += mm-gesture-daemon

#MM_GRAPHICS
MM_GRAPHICS := a225_pfp.fw
MM_GRAPHICS += a225_pm4.fw
MM_GRAPHICS += a225p5_pm4.fw
MM_GRAPHICS += a300_pfp.fw
MM_GRAPHICS += a300_pm4.fw
MM_GRAPHICS += a330_pfp.fw
MM_GRAPHICS += a330_pm4.fw
MM_GRAPHICS += a420_pfp.fw
MM_GRAPHICS += a420_pm4.fw
MM_GRAPHICS += eglsubAndroid
MM_GRAPHICS += gpu_dcvsd
MM_GRAPHICS += leia_pfp_470.fw
MM_GRAPHICS += leia_pm4_470.fw
MM_GRAPHICS += libadreno_utils
MM_GRAPHICS += libC2D2
MM_GRAPHICS += libCB
MM_GRAPHICS += libc2d2_z180
MM_GRAPHICS += libc2d2_a3xx
MM_GRAPHICS += libEGL_adreno
MM_GRAPHICS += libc2d30-a3xx
MM_GRAPHICS += libc2d30-a4xx
MM_GRAPHICS += libc2d30
MM_GRAPHICS += libgsl
MM_GRAPHICS += libGLESv2_adreno
MM_GRAPHICS += libGLESv2S3D_adreno
MM_GRAPHICS += libGLESv1_CM_adreno
MM_GRAPHICS += libllvm-a3xx
MM_GRAPHICS += libllvm-arm
MM_GRAPHICS += libllvm-qcom
MM_GRAPHICS += libOpenVG
MM_GRAPHICS += libOpenCL
MM_GRAPHICS += libplayback_adreno
MM_GRAPHICS += libq3dtools_adreno
MM_GRAPHICS += libsc-a2xx
MM_GRAPHICS += libsc-a3xx
MM_GRAPHICS += libsc-adreno.a
MM_GRAPHICS += ProfilerPlaybackTools
MM_GRAPHICS += yamato_pfp.fw
MM_GRAPHICS += yamato_pm4.fw
MM_GRAPHICS += librs_adreno
MM_GRAPHICS += libRSDriver_adreno
MM_GRAPHICS += librs_adreno_sha1

#MM_HTTP
MM_HTTP := libmmipstreamaal
MM_HTTP += libmmipstreamnetwork
MM_HTTP += libmmipstreamutils
MM_HTTP += libmmiipstreammmihttp
MM_HTTP += libmmhttpstack
MM_HTTP += libmmipstreamsourcehttp
MM_HTTP += libmmqcmediaplayer

#MM_DRMPLAY
MM_DRMPLAY := drmclientlib
MM_DRMPLAY += libDrmPlay

#MM_MUX
MM_MUX := libFileMux

#MM_OSAL
MM_OSAL := libmmosal

#MM_PARSER
MM_PARSER := libASFParserLib
MM_PARSER += libDivxDrm
MM_PARSER += libExtendedExtractor
MM_PARSER += libmmparser
MM_PARSER += libmmparser_lite
MM_PARSER += libSHIMDivxDrm

#MM_QSM
MM_QSM := libmmQSM

#MM_RTP
MM_RTP := libmmrtpdecoder
MM_RTP += libmmrtpencoder

#MM_STEREOLIB
MM_STEREOLIB := libmmstereo

#MM_STILL
MM_STILL := libadsp_jpege_skel
MM_STILL += libgemini
MM_STILL += libimage-jpeg-dec-omx-comp
MM_STILL += libimage-jpeg-enc-omx-comp
MM_STILL += libimage-omx-common
MM_STILL += libjpegdhw
MM_STILL += libjpegehw
MM_STILL += libmmipl
MM_STILL += libmmjpeg
MM_STILL += libmmjps
MM_STILL += libmmmpo
MM_STILL += libmmmpod
MM_STILL += libmmqjpeg_codec
MM_STILL += libmmstillomx
MM_STILL += libqomx_jpegenc
MM_STILL += libqomx_jpegdec
MM_STILL += mm-jpeg-dec-test
MM_STILL += mm-jpeg-dec-test-client
MM_STILL += mm-jpeg-enc-test
MM_STILL += mm-jpeg-enc-test-client
MM_STILL += mm-jps-enc-test
MM_STILL += mm-mpo-enc-test
MM_STILL += mm-mpo-dec-test
MM_STILL += mm-qjpeg-dec-test
MM_STILL += mm-qjpeg-enc-test
MM_STILL += mm-qomx-ienc-test
MM_STILL += mm-qomx-idec-test
MM_STILL += test_gemini

#MM_VIDEO
MM_VIDEO := ast-mm-vdec-omx-test7k
MM_VIDEO += iv_h264_dec_lib
MM_VIDEO += iv_mpeg4_dec_lib
MM_VIDEO += libdivxdrmdecrypt
MM_VIDEO += libh264decoder
MM_VIDEO += libHevcSwDecoder
MM_VIDEO += liblasic
MM_VIDEO += libmm-adspsvc
MM_VIDEO += libmp4decoder
MM_VIDEO += libmp4decodervlddsp
MM_VIDEO += libOmxH264Dec
MM_VIDEO += libOmxIttiamVdec
MM_VIDEO += libOmxIttiamVenc
MM_VIDEO += libOmxMpeg4Dec
MM_VIDEO += libOmxOn2Dec
MM_VIDEO += libOmxrv9Dec
MM_VIDEO += libOmxVidEnc
MM_VIDEO += libOmxWmvDec
MM_VIDEO += libon2decoder
MM_VIDEO += librv9decoder
MM_VIDEO += libVenusMbiConv
MM_VIDEO += mm-vdec-omx-test
MM_VIDEO += mm-venc-omx-test
MM_VIDEO += msm-vidc-test
MM_VIDEO += venc-device-android
MM_VIDEO += venus.b00
MM_VIDEO += venus.b01
MM_VIDEO += venus.b02
MM_VIDEO += venus.b03
MM_VIDEO += venus.b04
MM_VIDEO += venus.mbn
MM_VIDEO += venus.mdt
MM_VIDEO += vidc_1080p.fw
MM_VIDEO += vidc.b00
MM_VIDEO += vidc.b01
MM_VIDEO += vidc.b02
MM_VIDEO += vidc.b03
MM_VIDEO += vidcfw.elf
MM_VIDEO += vidc.mdt
MM_VIDEO += vidc_720p_command_control.fw
MM_VIDEO += vidc_720p_h263_dec_mc.fw
MM_VIDEO += vidc_720p_h264_dec_mc.fw
MM_VIDEO += vidc_720p_h264_enc_mc.fw
MM_VIDEO += vidc_720p_mp4_dec_mc.fw
MM_VIDEO += vidc_720p_mp4_enc_mc.fw
MM_VIDEO += vidc_720p_vc1_dec_mc.fw
MM_VIDEO += vt-sim-test

#MM_VPU
MM_VPU := vpu.b00
MM_VPU += vpu.b01
MM_VPU += vpu.b02
MM_VPU += vpu.b03
MM_VPU += vpu.b04
MM_VPU += vpu.b05
MM_VPU += vpu.b06
MM_VPU += vpu.b07
MM_VPU += vpu.b08
MM_VPU += vpu.b09
MM_VPU += vpu.b10
MM_VPU += vpu.b11
MM_VPU += vpu.b12
MM_VPU += vpu.mbn
MM_VPU += vpu.mdt


#MODEM_APIS
MODEM_APIS := libadc
MODEM_APIS += libauth
MODEM_APIS += libcm
MODEM_APIS += libdsucsd
MODEM_APIS += libfm_wan_api
MODEM_APIS += libgsdi_exp
MODEM_APIS += libgstk_exp
MODEM_APIS += libisense
MODEM_APIS += libloc_api
MODEM_APIS += libmmgsdilib
MODEM_APIS += libmmgsdisessionlib
MODEM_APIS += libmvs
MODEM_APIS += libnv
MODEM_APIS += liboem_rapi
MODEM_APIS += libpbmlib
MODEM_APIS += libpdapi
MODEM_APIS += libpdsm_atl
MODEM_APIS += libping_mdm
MODEM_APIS += libplayready
MODEM_APIS += librfm_sar
MODEM_APIS += libsnd
MODEM_APIS += libtime_remote_atom
MODEM_APIS += libuim
MODEM_APIS += libvoem_if
MODEM_APIS += libwidevine
MODEM_APIS += libwms
MODEM_APIS += libcommondefs
MODEM_APIS += libcm_fusion
MODEM_APIS += libcm_mm_fusion
MODEM_APIS += libdsucsdappif_apis_fusion
MODEM_APIS += liboem_rapi_fusion
MODEM_APIS += libpbmlib_fusion
MODEM_APIS += libping_lte_rpc
MODEM_APIS += libwms_fusion

#MODEM_API_TEST
MODEM_API_TEST := librstf

#MPDCVS
MPDCVS := dcvsd
MPDCVS += mpdecision

#MPQ_COMMAND_IF
MPQ_COMMAND_IF := libmpq_ci
MPQ_COMMAND_IF += libmpqci_cimax_spi

#MPQ_PLATFORM
MPQ_PLATFORM := AvSyncTest
MPQ_PLATFORM += libavinput
MPQ_PLATFORM += libavinputhaladi
MPQ_PLATFORM += libfrc
MPQ_PLATFORM += libmpqaudiocomponent
MPQ_PLATFORM += libmpqaudiosettings
MPQ_PLATFORM += libmpqavs
MPQ_PLATFORM += libmpqavstreammanager
MPQ_PLATFORM += libmpqcore
MPQ_PLATFORM += libmpqplatform
MPQ_PLATFORM += libmpqplayer
MPQ_PLATFORM += libmpqplayerclient
MPQ_PLATFORM += libmpqsource
MPQ_PLATFORM += libmpqstobinder
MPQ_PLATFORM += libmpqstreambuffersource
MPQ_PLATFORM += libmpqtsdm
MPQ_PLATFORM += libmpqutils
MPQ_PLATFORM += libmpqvcapsource
MPQ_PLATFORM += libmpqvcxo
MPQ_PLATFORM += libmpqvideodecoder
MPQ_PLATFORM += libmpqvideorenderer
MPQ_PLATFORM += libmpqvideosettings
MPQ_PLATFORM += librffrontend
MPQ_PLATFORM += libmpqtestpvr
MPQ_PLATFORM += MPQDvbCTestApp
MPQ_PLATFORM += MPQPlayerApp
MPQ_PLATFORM += MPQStrMgrTest
MPQ_PLATFORM += MPQUnitTest
MPQ_PLATFORM += MPQVideoRendererTestApp
MPQ_PLATFORM += mpqavinputtest
MPQ_PLATFORM += mpqi2ctest
MPQ_PLATFORM += mpqvcaptest
MPQ_PLATFORM += mpqdvbtestapps
MPQ_PLATFORM += mpqdvbservice

#N_SMUX
N_SMUX := n_smux

#NFC
NFC += Signedrompatch_v20.bin
NFC += Signedrompatch_v21.bin
NFC += Signedrompatch_v24.bin
NFC += nfc_test.bin
NFC += nfcee_access.xml
NFC += nfc-nci.conf

#OEM_SERVICES - system monitor shutdown modules
OEM_SERVICES := libSubSystemShutdown
OEM_SERVICES += libsubsystem_control
OEM_SERVICES += oem-services

#ONCRPC
ONCRPC := libdsm
ONCRPC += liboncrpc
ONCRPC += libping_apps
ONCRPC += libqueue
ONCRPC += ping_apps_client_test_0000

#PERF
PERF := libqc-opt

#PLAYREADY
PLAYREADY := drmdiagapp
PLAYREADY += drmtest
PLAYREADY += libdrmdiag
PLAYREADY += libdrmfs
PLAYREADY += libdrmtime
PLAYREADY += libtzplayready

#PROFILER
PROFILER := profiler_tester
PROFILER += profiler_daemon
PROFILER += libprofiler_msmadc

#TREPN
TREPN := Trepn

#QCHAT
QCHAT := QComQMIPermissions

#QCNVITEM
QCNVITEM := qcnvitems
QCNVITEM += qcnvitems.xml

#QCRIL
QCRIL := libril-qc-1
QCRIL += libril-qc-qmi-1
QCRIL += libril-qcril-hook-oem
QCRIL += qcrilhook
QCRIL += qcrilmsgtunnel
QCRIL += qcrilhook.xml
QCRIL += libwmsts

#QCOMSYSDAEMON
QCOMSYSDAEMON := qcom-system-daemon

#QMI
QMI := irsc_util
QMI += libidl
QMI += libqmi
QMI += libqmi_cci
QMI += libqmi_common_so
QMI += libqmi_csi
QMI += libqmi_encdec
QMI += libqmiservices
QMI += qmiproxy
QMI += qmi_config.xml
QMI += qmuxd

#QOSMGR
QOSMGR := qosmgr
QOSMGR += qosmgr_rules.xml

#QUICKCHARGE
QUICKCHARGE := hvdcp

#REMOTEFS
REMOTEFS := rmt_storage

#RFS_ACCESS
RFS_ACCESS := rfs_access

#RNGD
RNGD := rngd

#SCVE
SCVE := libscve
SCVE += libscve_stub
SCVE += libscveT2T_skel

#SECUREMSM
SECUREMSM := com.qualcomm.qti.services.secureui
SECUREMSM += dbAccess
SECUREMSM += drmdiagapp
SECUREMSM += drm_generic_prov_test
SECUREMSM += isdbtmmtest
SECUREMSM += InstallKeybox
SECUREMSM += libprdrmdecrypt
SECUREMSM += libdrmprplugin
SECUREMSM += libdrmdiag
SECUREMSM += libdrmfs
SECUREMSM += libdrmtime
SECUREMSM += libgoogletest
SECUREMSM += libtzplayready
SECUREMSM += libtzdrmgenprov
SECUREMSM += liboemcrypto
SECUREMSM += liboemcrypto.a
SECUREMSM += libQSEEComAPI
SECUREMSM += libMcClient
SECUREMSM += libMcCommon
SECUREMSM += libMcDriverDevice
SECUREMSM += libMcRegistry
SECUREMSM += libPaApi
SECUREMSM += libP11EncryptorDecryptor
SECUREMSM += libP11Notify
SECUREMSM += libPkiOtpGenerator
SECUREMSM += librmp
SECUREMSM += libpvr
SECUREMSM += librpmb
SECUREMSM += lib-sec-disp
SECUREMSM += libsecureui
SECUREMSM += libsecureui_svcsock
SECUREMSM += libSecureUILib
SECUREMSM += libsecureuisvc_jni
SECUREMSM += libssd
SECUREMSM += libqsappsver
SECUREMSM += libSSEPKCS11
SECUREMSM += libStDrvInt
SECUREMSM += mcDriverDaemon
SECUREMSM += mcDriverDaemonQC
SECUREMSM += mcStarter
SECUREMSM += oemwvtest
SECUREMSM += qseecom_sample_client
SECUREMSM += qseecom_security_test
SECUREMSM += qseecomd
SECUREMSM += QSSEP11EncryptorDecryptor
SECUREMSM += QSSEPKCS11OtpGen
SECUREMSM += secure_ui_sample_client
SECUREMSM += StoreKeybox
SECUREMSM += tlcP11
SECUREMSM += TlcWrapper
SECUREMSM += tlcWrapperApp
SECUREMSM += widevinetest
SECUREMSM += widevinetest_rpc
SECUREMSM += keystore.qcom
SECUREMSM += keymaster_test


#SENSORS
SENSORS := sensor_def_qcomdev.conf
SENSORS += sensors.apq8084
SENSORS += sensors.msm8226
SENSORS += sensors.msm8610
SENSORS += sensors.msm8930
SENSORS += sensors.msm8960
SENSORS += sensors.msm8974
SENSORS += sensors.qcom

#SS_RESTART
SS_RESTART := ssr_diag
SS_RESTART += subsystem_ramdump

#SVGT
SVGT := libsvgecmascriptBindings
SVGT += libsvgutils
SVGT += libsvgabstraction
SVGT += libsvgscriptEngBindings
SVGT += libsvgnativeandroid
SVGT += libsvgt
SVGT += libsvgcore

#SWDEC2DTO3D
SW2DTO3D := libswdec_2dto3d

#TELEPHONY_APPS
TELEPHONY_APPS := shutdownlistener
TELEPHONY_APPS += fastdormancy
TELEPHONY_APPS += GsmTuneAway
TELEPHONY_APPS += NetworkSetting

#THERMAL
THERMAL := thermald
THERMAL += thermald.conf
THERMAL += thermald-8960.conf
THERMAL += thermald-8064.conf
THERMAL += thermald-8930.conf
THERMAL += thermald-8960ab.conf
THERMAL += thermald-8064ab.conf
THERMAL += thermald-8930ab.conf
THERMAL += thermald-8974.conf
THERMAL += thermald-8x25-msm1-pmic_therm.conf
THERMAL += thermald-8x25-msm2-pmic_therm.conf
THERMAL += thermald-8x25-msm2-msm_therm.conf

#THERMAL-ENGINE
THERMAL-ENGINE := thermal-engine
THERMAL-ENGINE += libthermalclient
THERMAL-ENGINE += thermal-engine-8960.conf
THERMAL-ENGINE += thermal-engine-8064.conf
THERMAL-ENGINE += thermal-engine-8064ab.conf
THERMAL-ENGINE += thermal-engine-8930.conf
THERMAL-ENGINE += thermal-engine-8974.conf
THERMAL-ENGINE += thermal-engine-8226.conf
THERMAL-ENGINE += thermal-engine-8610.conf

#TIME_SERVICES
TIME_SERVICES := time_daemon TimeService libTimeService

#TINY xml
TINYXML := libtinyxml

#TINYXML2
TINYXML2 := libtinyxml2

#TS_TOOLS
TS_TOOLS := evt-sniff.cfg

#TTSP firmware
TTSP_FW := cyttsp_7630_fluid.hex
TTSP_FW += cyttsp_8064_mtp.hex
TTSP_FW += cyttsp_8660_fluid_p2.hex
TTSP_FW += cyttsp_8660_fluid_p3.hex
TTSP_FW += cyttsp_8660_ffa.hex
TTSP_FW += cyttsp_8960_cdp.hex

#TV_TUNER
TV_TUNER := atv_fe_test
TV_TUNER += dtv_fe_test
TV_TUNER += lib_atv_rf_fe
TV_TUNER += lib_dtv_rf_fe
TV_TUNER += lib_MPQ_RFFE
TV_TUNER += libmpq_bsp8092_cdp_h1
TV_TUNER += libmpq_bsp8092_cdp_h5
TV_TUNER += libmpq_bsp8092_rd_h1
TV_TUNER += lib_tdsn_c231d
TV_TUNER += lib_tdsq_g631d
TV_TUNER += lib_tdss_g682d
TV_TUNER += libmpq_rf_utils
TV_TUNER += lib_sif_demod_stub
TV_TUNER += lib_tv_bsp_mpq8064_dvb
TV_TUNER += lib_tv_receiver_stub
TV_TUNER += libtv_tuners_io
TV_TUNER += tv_driver_test
TV_TUNER += tv_fe_test
TV_TUNER += libUCCP330
TV_TUNER += libForza

#ULTRASOUND
ULTRASOUND := DigitalPenSettings
ULTRASOUND += DigitalPenService
ULTRASOUND += UltrasoundSettings
ULTRASOUND += usf_epos_liquid_ps_disabled.cfg
ULTRASOUND += service_settings_liquid.xml
ULTRASOUND += service_settings_liquid_folio.xml
ULTRASOUND += service_settings_liquid_1_to_1.xml
ULTRASOUND += service_settings_fluid.xml
ULTRASOUND += service_settings_dragon.xml
ULTRASOUND += usf_sync_gesture_lpass_rec_mtp.cfg
ULTRASOUND += usf_epos_liquid_6_channels.cfg
ULTRASOUND += usf_epos_mtp_ps_disabled.cfg
ULTRASOUND += service_settings_mtp.xml
ULTRASOUND += digitalpenservice.xml
ULTRASOUND += PenPairingApp
ULTRASOUND += readme.txt
ULTRASOUND += form_factor_fluid.cfg
ULTRASOUND += ps_tuning1_fluid.bin
ULTRASOUND += usf_tester_echo_fluid.cfg
ULTRASOUND += usf_pairing_fluid.cfg
ULTRASOUND += usf_epos_fluid.cfg
ULTRASOUND += usf_tester_epos_fluid.cfg
ULTRASOUND += usf_sync_gesture_fluid.cfg
ULTRASOUND += usf_sync_gesture_apps_fluid.cfg
ULTRASOUND += usf_sync_gesture_fluid_tx_transparent_data.bin
ULTRASOUND += form_factor_liquid.cfg
ULTRASOUND += ps_tuning1_standby_liquid.bin
ULTRASOUND += ps_tuning1_idle_liquid.bin
ULTRASOUND += usf_epos_liquid.cfg
ULTRASOUND += usf_tester_epos_liquid.cfg
ULTRASOUND += usf_pairing_liquid.cfg
ULTRASOUND += usf_sync_gesture_liquid.cfg
ULTRASOUND += usf_sync_gesture_liquid_tx_transparent_data.bin
ULTRASOUND += form_factor_mtp.cfg
ULTRASOUND += ps_tuning1_mtp.bin
ULTRASOUND += usf_epos_mtp.cfg
ULTRASOUND += usf_tester_echo_mtp.cfg
ULTRASOUND += usf_tester_epos_mtp.cfg
ULTRASOUND += libgessockadapter
ULTRASOUND += libgessyncsockadapter
ULTRASOUND += libgesadapter
ULTRASOUND += libsyncgesadapter
ULTRASOUND += libqcgesture
ULTRASOUND += libual
ULTRASOUND += libualutil
ULTRASOUND += mixer_paths_dragon.xml
ULTRASOUND += mixer_paths_fluid.xml
ULTRASOUND += mixer_paths_liquid.xml
ULTRASOUND += mixer_paths_mtp.xml
ULTRASOUND += usf_epos
ULTRASOUND += usf_tester
ULTRASOUND += usf_pairing
ULTRASOUND += usf_sync_gesture
ULTRASOUND += usf_tsc.idc
ULTRASOUND += usf_tsc_ptr.idc
ULTRASOUND += usf_tsc_ext.idc
ULTRASOUND += libppl
ULTRASOUND += usf_post_boot.sh
ULTRASOUND += usf_settings.sh
ULTRASOUND += usf_pairing_mtp.cfg
ULTRASOUND += libepdsp
ULTRASOUND += libusutils
ULTRASOUND += usf_sync_gesture_mtp.cfg
ULTRASOUND += usf_sync_gesture_apps_mtp.cfg
ULTRASOUND += usf_sync_gesture_mtp_tx_transparent_data.bin
ULTRASOUND += libqcsyncgesture
ULTRASOUND += libmmgesture-bus2
ULTRASOUND += libusndroute
ULTRASOUND += product_calib_dragon.dat
ULTRASOUND += unit_calib_dragon.dat
ULTRASOUND += product_calib_liquid.dat
ULTRASOUND += unit_calib_liquid.dat
ULTRASOUND += product_calib_fluid.dat
ULTRASOUND += unit_calib_fluid.dat
ULTRASOUND += product_calib_mtp.dat
ULTRASOUND += unit_calib_mtp.dat
SERIES_FILES_NUM = 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42
define add_series_packets
  ULTRASOUND += $1
endef
$(foreach item,$(SERIES_FILES_NUM),$(eval $(call add_series_packets,series_calib$(item).dat)))

#USB
USB := ice40.bin

#USB_UICC_CLIENT
USB_UICC_CLIENT := usb_uicc_client

#WEBKIT
WEBKIT := browsermanagement
WEBKIT += libwebkitaccel
WEBKIT += PrivInit
WEBKIT += libdnshostprio
WEBKIT += libnetmonitor
WEBKIT += qnet-plugin
WEBKIT += pp_proc_plugin
WEBKIT += tcp-connections
WEBKIT += libtcpfinaggr
WEBKIT += modemwarmup
WEBKIT += libgetzip
WEBKIT += spl_proc_plugin
WEBKIT += libsocketpoolextend
WEBKIT += libvideo_cor

#WFD
WFD := capability.xml
WFD += libmmwfdinterface
WFD += libmmwfdsinkinterface
WFD += libmmwfdsrcinterface
WFD += libwfduibcinterface
WFD += libwfduibcsrcinterface
WFD += libwfduibcsrc
WFD += libOmxMux
WFD += libwfdcommonutils
WFD += libwfdhdcpcp
WFD += libwfdmmsrc
WFD += libwfdmmutils
WFD += libwfdnative
WFD += libwfdsm
WFD += libwfdrtsp
WFD += libextendedremotedisplay
WFD += WfdCommon
WFD += WfdService
WFD += WfdClient
WFD += wfdconfig.xml
WFD += wfdconfigsink.xml
WFD += WfdP2pCommon
WFD += WfdP2pService
WFD += com.qualcomm.wfd.permissions.xml
WFD += wfd-test


#WLAN
WLAN := libAniAsf
WLAN += cfg.dat
WLAN += msm7627a_qcom_wlan_nv.bin
WLAN += msm7630_qcom_wlan_nv.bin
WLAN += msm8660_qcom_wlan_nv.bin
WLAN += ptt_socket_app
WLAN += qcom_cfg_default.ini
WLAN += qcom_cfg.ini
WLAN += qcom_fw.bin
WLAN += qcom_wapi_fw.bin
WLAN += qcom_wlan_nv.bin
WLAN += WCN1314_cfg.dat
WLAN += WCN1314_qcom_cfg.ini
WLAN += WCN1314_qcom_fw.bin
WLAN += WCN1314_qcom_wlan_nv.bin
WLAN += WCNSS_cfg.dat
WLAN += WCNSS_qcom_cfg.ini
WLAN += WCNSS_qcom_wlan_nv.bin
WLAN += pktlogconf
WLAN += athdiag
WLAN += cld-fwlog-record
WLAN += cld-fwlog-netlink
WLAN += cld-fwlog-parser

PRODUCT_PACKAGES += $(ADC)
PRODUCT_PACKAGES += $(ADSPRPC)
PRODUCT_PACKAGES += $(ALLJOYN)
PRODUCT_PACKAGES += $(AIVPLAY)
PRODUCT_PACKAGES += $(BACKUP_AGENT)
PRODUCT_PACKAGES += $(BATTERY_CHARGING)
PRODUCT_PACKAGES += $(BT)
PRODUCT_PACKAGES += $(CAMERAHAWK_APPS)
PRODUCT_PACKAGES += $(CAMERAHAWK_WHIP_COMMON_LIB)
PRODUCT_PACKAGES += $(CCID)
PRODUCT_PACKAGES += $(CHARGER_MONITOR)
PRODUCT_PACKAGES += $(CNE)
PRODUCT_PACKAGES += $(CRASH_LOGGER)
PRODUCT_PACKAGES += $(DATA)
PRODUCT_PACKAGES += $(DIAG)
PRODUCT_PACKAGES += $(DISPLAY_TESTS)
PRODUCT_PACKAGES += $(ENTERPRISE_SECURITY)
PRODUCT_PACKAGES += $(FASTCV)
PRODUCT_PACKAGES += $(FASTMMI)
PRODUCT_PACKAGES += $(FLASH)
PRODUCT_PACKAGES += $(FM)
PRODUCT_PACKAGES += $(FOTA)
PRODUCT_PACKAGES += $(FTM)
PRODUCT_PACKAGES += $(GPS)
#Ims modules are intentionally set to optional
#so that they are not packaged as part of system image.
#Hence removing them from PRODUCT_PACKAGES list
#PRODUCT_PACKAGES += $(IMS)
PRODUCT_PACKAGES += $(IMS_VT)
PRODUCT_PACKAGES += $(IMS_TEL)
PRODUCT_PACKAGES += $(IMS_SETTINGS)
PRODUCT_PACKAGES += $(IMS_RCS)
PRODUCT_PACKAGES += $(IMS_NEWARCH)
PRODUCT_PACKAGES += $(IMS_REGMGR)
PRODUCT_PACKAGES += $(INTERFACE_PERMISSIONS)
PRODUCT_PACKAGES += $(IQAGENT)
PRODUCT_PACKAGES += $(MDM_HELPER)
PRODUCT_PACKAGES += $(MM_AUDIO)
PRODUCT_PACKAGES += $(MM_CAMERA)
PRODUCT_PACKAGES += $(MM_CORE)
PRODUCT_PACKAGES += $(MM_COLOR_CONVERSION)
PRODUCT_PACKAGES += $(MM_COLOR_CONVERTOR)
PRODUCT_PACKAGES += $(MM_DRMPLAY)
PRODUCT_PACKAGES += $(MM_GESTURES)
PRODUCT_PACKAGES += $(MM_GRAPHICS)
PRODUCT_PACKAGES += $(MM_HTTP)
PRODUCT_PACKAGES += $(MM_MUX)
PRODUCT_PACKAGES += $(MM_OSAL)
PRODUCT_PACKAGES += $(MM_PARSER)
PRODUCT_PACKAGES += $(MM_QSM)
PRODUCT_PACKAGES += $(MM_RTP)
PRODUCT_PACKAGES += $(MM_STEREOLIB)
PRODUCT_PACKAGES += $(MM_STILL)
PRODUCT_PACKAGES += $(MM_VIDEO)
PRODUCT_PACKAGES += $(MM_VPU)
PRODUCT_PACKAGES += $(MODEM_APIS)
PRODUCT_PACKAGES += $(MODEM_API_TEST)
PRODUCT_PACKAGES += $(MPDCVS)
PRODUCT_PACKAGES += $(MPQ_COMMAND_IF)
PRODUCT_PACKAGES += $(MPQ_PLATFORM)
PRODUCT_PACKAGES += $(MXT_CFG)
PRODUCT_PACKAGES += $(N_SMUX)
PRODUCT_PACKAGES += $(NFC)
PRODUCT_PACKAGES += $(OEM_SERVICES)
PRODUCT_PACKAGES += $(ONCRPC)
PRODUCT_PACKAGES += $(PERF)
PRODUCT_PACKAGES += $(PERFECT365_APPS)
PRODUCT_PACKAGES += $(PLAYREADY)
PRODUCT_PACKAGES += $(PROFILER)
PRODUCT_PACKAGES += $(QCHAT)
PRODUCT_PACKAGES += $(QCRIL)
PRODUCT_PACKAGES += $(QCNVITEM)
PRODUCT_PACKAGES += $(QCOMSYSDAEMON)
PRODUCT_PACKAGES += $(QMI)
PRODUCT_PACKAGES += $(QOSMGR)
PRODUCT_PACKAGES += $(QUICKCHARGE)
PRODUCT_PACKAGES += $(REMOTEFS)
PRODUCT_PACKAGES += $(RFS_ACCESS)
PRODUCT_PACKAGES += $(RNGD)
PRODUCT_PACKAGES += $(SCVE)
PRODUCT_PACKAGES += $(SECUREMSM)
PRODUCT_PACKAGES += $(SENSORS)
PRODUCT_PACKAGES += $(SS_RESTART)
PRODUCT_PACKAGES += $(SVGT)
PRODUCT_PACKAGES += $(SW2DTO3D)
PRODUCT_PACKAGES += $(TELEPHONY_APPS)
PRODUCT_PACKAGES += $(THERMAL)
PRODUCT_PACKAGES += $(THERMAL-ENGINE)
PRODUCT_PACKAGES += $(TIME_SERVICES)
PRODUCT_PACKAGES += $(TINYXML)
PRODUCT_PACKAGES += $(TINYXML2)
PRODUCT_PACKAGES += $(TREPN)
PRODUCT_PACKAGES += $(TS_TOOLS)
PRODUCT_PACKAGES += $(TTSP_FW)
PRODUCT_PACKAGES += $(TV_TUNER)
PRODUCT_PACKAGES += $(ULTRASOUND)
PRODUCT_PACKAGES += $(USB)
PRODUCT_PACKAGES += $(USB_UICC_CLIENT)
PRODUCT_PACKAGES += $(WEBKIT)
PRODUCT_PACKAGES += $(WFD)
PRODUCT_PACKAGES += $(WHIP_APPS)
PRODUCT_PACKAGES += $(WLAN)
PRODUCT_PACKAGES += $(BT_TEL)

# Each line here corresponds to a debug LOCAL_MODULE built by
# Android.mk(s) in the proprietary projects. Where project
# corresponds to the vars here in CAPs.

# These modules are tagged with LOCAL_MODULE_TAGS := optional.
# They would not be installed into the image unless
# they are listed here explicitly.

#BT_DBG
BT_DBG := AR3002_PS_ASIC
BT_DBG += AR3002_RamPatch

#CNE_DBG
CNE_DBG := AndsfParser
CNE_DBG += cnelogger
CNE_DBG += swimcudpclient
CNE_DBG += swimstcpclient
CNE_DBG += swimtcpclient
CNE_DBG += swimudpclient
CNE_DBG += test2client

#CNESETTINGS_DBG
CNESETTINGS_DBG := CNESettings

#COMMON_DBG
COMMON_DBG := remote_apis_verify

#GPS_DBG
GPS_DBG := QVTester
GPS_DBG += lowi_test
GPS_DBG += gps-test.sh
GPS_DBG += libposlog
GPS_DBG += IZatSample
GPS_DBG += QCLocSvcTests
GPS_DBG += com.qualcomm.qlogcat
GPS_DBG += libloc2jnibridge
GPS_DBG += libdiagbridge
GPS_DBG += ODLT
GPS_DBG += com.qualcomm.qmapbridge.xml
GPS_DBG += sftc

#IMS_RCS_DBG
IMS_RCS_DBG := RCSRealApp
IMS_RCS_DBG += RCSBootstraputil
IMS_RCS_DBG += rcsservice.xml
IMS_RCS_DBG += rcsservice

#KERNEL_DBG
KERNEL_TEST_DBG := cpuhotplug_test.sh
KERNEL_TEST_DBG += cputest.sh
KERNEL_TEST_DBG += msm_uart_test
KERNEL_TEST_DBG += probe_test.sh
KERNEL_TEST_DBG += msm_uart_test.sh
KERNEL_TEST_DBG += uarttest.sh
KERNEL_TEST_DBG += clocksourcetest.sh
KERNEL_TEST_DBG += socinfotest.sh
KERNEL_TEST_DBG += timertest.sh
KERNEL_TEST_DBG += vfp.sh
KERNEL_TEST_DBG += vfptest
KERNEL_TEST_DBG += pctest
KERNEL_TEST_DBG += modem_test
KERNEL_TEST_DBG += pc-compound-test.sh
KERNEL_TEST_DBG += msm_sps_test
KERNEL_TEST_DBG += cacheflush
KERNEL_TEST_DBG += cacheflush.sh
KERNEL_TEST_DBG += _cacheflush.sh
KERNEL_TEST_DBG += loop.sh
KERNEL_TEST_DBG += clk_test.sh
KERNEL_TEST_DBG += cpufreq_test.sh
KERNEL_TEST_DBG += fbtest
KERNEL_TEST_DBG += fbtest.sh
KERNEL_TEST_DBG += geoinfo_flash
KERNEL_TEST_DBG += gpio_lib.conf
KERNEL_TEST_DBG += gpio_lib.sh
KERNEL_TEST_DBG += gpio_tlmm.sh
KERNEL_TEST_DBG += gpio_tlmm.conf
KERNEL_TEST_DBG += i2c-msm-test
KERNEL_TEST_DBG += i2c-msm-test.sh
KERNEL_TEST_DBG += irq_test.sh
KERNEL_TEST_DBG += kgsl_test
KERNEL_TEST_DBG += mpp_test.sh
KERNEL_TEST_DBG += msm_adc_test
KERNEL_TEST_DBG += msm_dma
KERNEL_TEST_DBG += msm_dma.sh
KERNEL_TEST_DBG += mtd_driver_test.sh
KERNEL_TEST_DBG += mtd_test.sh
KERNEL_TEST_DBG += mtd_yaffs2_test.sh
KERNEL_TEST_DBG += AR_LUT_1_0_B0
KERNEL_TEST_DBG += AR_LUT_1_0_B
KERNEL_TEST_DBG += AR_LUT_1_0_G0
KERNEL_TEST_DBG += AR_LUT_1_0_G
KERNEL_TEST_DBG += AR_LUT_1_0_R0
KERNEL_TEST_DBG += r_only_igc
KERNEL_TEST_DBG += SanityCfg.cfg
KERNEL_TEST_DBG += qcedev_test
KERNEL_TEST_DBG += qcedev_test.sh
KERNEL_TEST_DBG += yv12_qcif.yuv
KERNEL_TEST_DBG += rotator
KERNEL_TEST_DBG += rtc_test
KERNEL_TEST_DBG += sd_test.sh
KERNEL_TEST_DBG += smd_pkt_loopback_test
KERNEL_TEST_DBG += smem_log_test
KERNEL_TEST_DBG += smd_tty_loopback_test
KERNEL_TEST_DBG += spidevtest
KERNEL_TEST_DBG += spidevtest.sh
KERNEL_TEST_DBG += spitest
KERNEL_TEST_DBG += spitest.sh
KERNEL_TEST_DBG += spiethernettest.sh
KERNEL_TEST_DBG += test_env_setup.sh
KERNEL_TEST_DBG += vreg_test.sh

#MM_AUDIO_DBG
MM_AUDIO_DBG := libds_native
MM_AUDIO_DBG += libds_jni
MM_AUDIO_DBG += libstagefright_soft_ddpdec
MM_AUDIO_DBG += libsurround_proc
MM_AUDIO_DBG += surround_sound_headers
MM_AUDIO_DBG += filter1i.pcm
MM_AUDIO_DBG += filter1r.pcm
MM_AUDIO_DBG += filter2i.pcm
MM_AUDIO_DBG += filter2r.pcm
MM_AUDIO_DBG += filter3i.pcm
MM_AUDIO_DBG += filter3r.pcm
MM_AUDIO_DBG += filter4i.pcm
MM_AUDIO_DBG += filter4r.pcm
MM_AUDIO_DBG += mm-audio-ftm
MM_AUDIO_DBG += libOmxAc3HwDec
MM_AUDIO_DBG += mm-adec-omxac3-hw-test
MM_AUDIO_DBG += mm-adec-omxQcelpHw-test
MM_AUDIO_DBG += mm-adec-omxvam-test
MM_AUDIO_DBG += mm-adec-omxevrc-hw-test
MM_AUDIO_DBG += mm-adec-omxamrwb-test
MM_AUDIO_DBG += mm-adec-omxamr-test
MM_AUDIO_DBG += mm-adec-omxadpcm-test
MM_AUDIO_DBG += mm-adec-omxwma-test
MM_AUDIO_DBG += mm-audio-alsa-test
MM_AUDIO_DBG += mm-adec-omxaac-test
#MM_AUDIO_DBG += avs_test_ker.ko
MM_AUDIO_DBG += mm-adec-omxQcelp13-test
MM_AUDIO_DBG += mm-adec-omxevrc-test
MM_AUDIO_DBG += libsrsprocessing_libs
MM_AUDIO_DBG += libsrsprocessing

#MM_CAMERA_DBG
MM_CAMERA_DBG := libmmcamera_wavelet_debug

#MM_CORE_DBG
MM_CORE_DBG := libmm-abl
MM_CORE_DBG += PPPreference
MM_CORE_DBG += ADService

#MM_VIDEO_DBG
MM_VIDEO_DBG := mm-adspsvc-test

#SECUREMSM_DBG
SECUREMSM_DBG := oemprtest
SECUREMSM_DBG += oemwvgtest
SECUREMSM_DBG += PlayreadySamplePlayer
SECUREMSM_DBG += playreadygtest
SECUREMSM_DBG += playreadygtest_error
SECUREMSM_DBG += playreadygtest_cryptoplugin
SECUREMSM_DBG += PlayreadyDrmTesting
SECUREMSM_DBG += widevinegtest

#SENSORS_DEBUG
SENSORS_DBG := QSensorTest
SENSORS_DBG += libsensor_reg
SENSORS_DBG += libsensor_test
SENSORS_DBG += libsensor_thresh
SENSORS_DBG += libsensor_user_cal
SENSORS_DBG += sns_cm_conc_test
SENSORS_DBG += sns_cm_test
SENSORS_DBG += sns_dsps_tc0001
SENSORS_DBG += sns_file_test
SENSORS_DBG += sns_hal_batch
SENSORS_DBG += sns_regedit_ssi
SENSORS_DBG += sns_smr_loopback_test

#TELEPHONY_APPS_DBG
TELEPHONY_APPS_DBG := Presence
TELEPHONY_APPS_DBG += SimContacts
TELEPHONY_APPS_DBG += NetworkSetting
TELEPHONY_APPS_DBG += QosTest
TELEPHONY_APPS_DBG += LDSTestApp.xml
TELEPHONY_APPS_DBG += QosTestConfig.xml
TELEPHONY_APPS_DBG += FTMMode
TELEPHONY_APPS_DBG += RoamingSettings
TELEPHONY_APPS_DBG += UniversalDownload

#WEBKIT_DBG
WEBKIT_DBG := pageload_proc_plugin
WEBKIT_DBG += modemwarmup.xml

#WFD_DBG
WFD_DBG := rtspclient
WFD_DBG += rtspserver

#WLAN_DBG

WLAN_DBG := abtfilt
WLAN_DBG += artagent
WLAN_DBG += ar6004_wlan.conf
WLAN_DBG += ar6004_fw_12
WLAN_DBG += ar6004_bdata_12
WLAN_DBG += ar6004_usb_fw_13
WLAN_DBG += ar6004_usb_bdata_13
WLAN_DBG += ar6004_sdio_fw_13
WLAN_DBG += ar6004_sdio_bdata_13
WLAN_DBG += ar6004_usb_fw_ext_13
WLAN_DBG += ar6004_fw_30
WLAN_DBG += ar6004_usb_bdata_30
WLAN_DBG += ar6004_sdio_bdata_30
WLAN_DBG += proprietary_pronto_wlan.ko

#QUICKBOOT
QUICKBOOT := QuickBoot

#LOG_SYSTEM
LOG_SYSTEM := Logkit
LOG_SYSTEM += SystemAgent
LOG_SYSTEM += qlogd
LOG_SYSTEM += qlog-conf.xml
LOG_SYSTEM += ftrace.cfg
LOG_SYSTEM += default_diag_mask.cfg
LOG_SYSTEM += rootagent
LOG_SYSTEM += init.qcom.rootagent.sh
LOG_SYSTEM += dynamic_debug_mask.cfg

PRODUCT_PACKAGES_DEBUG += $(BT_DBG)
PRODUCT_PACKAGES_DEBUG += $(CNE_DBG)
PRODUCT_PACKAGES_DEBUG += $(CNESETTINGS_DBG)
PRODUCT_PACKAGES_DEBUG += $(COMMON_DBG)
PRODUCT_PACKAGES_DEBUG += $(GPS_DBG)
PRODUCT_PACKAGES_DEBUG += $(IMS_RCS_DBG)
PRODUCT_PACKAGES_DEBUG += $(KERNEL_TEST_DBG)
PRODUCT_PACKAGES_DEBUG += $(MM_AUDIO_DBG)
PRODUCT_PACKAGES_DEBUG += $(MM_CAMERA_DBG)
PRODUCT_PACKAGES_DEBUG += $(MM_CORE_DBG)
PRODUCT_PACKAGES_DEBUG += $(MM_VIDEO_DBG)
PRODUCT_PACKAGES_DEBUG += $(SECUREMSM_DBG)
PRODUCT_PACKAGES_DEBUG += $(SENSORS_DBG)
PRODUCT_PACKAGES_DEBUG += $(TELEPHONY_APPS_DBG)
PRODUCT_PACKAGES_DEBUG += $(WEBKIT_DBG)
PRODUCT_PACKAGES_DEBUG += $(WFD_DBG)
PRODUCT_PACKAGES_DEBUG += $(WLAN_DBG)
PRODUCT_PACKAGES_DEBUG += $(QUICKBOOT)
PRODUCT_PACKAGES_DEBUG += $(LOG_SYSTEM)
