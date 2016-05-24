ifeq ($(NFC_D), true)
LOCAL_PATH:= $(call my-dir)

########################################
# com.android.nfc.helper - library
########################################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := com.android.nfc.helper
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
 # Install to system/frameworks/lib
LOCAL_MODULE_PATH := $(TARGET_OUT_JAVA_LIBRARIES)
LOCAL_NO_STANDARD_LIBRARIES := true
LOCAL_JAVA_LIBRARIES := framework bouncycastle core ext
# Resources
res_source_path := APPS/framework-res_intermediates/src
LOCAL_INTERMEDIATE_SOURCES := \
                        $(res_source_path)/android/R.java \
                        $(res_source_path)/android/Manifest.java \
                        $(res_source_path)/com/android/internal/R.java

LOCAL_SRC_FILES += \
        android/nfc/IAppCallback.aidl \
        android/nfc/INfcAdapter.aidl \
        android/nfc/INfcAdapterExtras.aidl \
        android/nfc/INfcTag.aidl \
        android/nfc/INfcCardEmulation.aidl \
        $(call all-java-files-under, android)

LOCAL_CERTIFICATE := platform
LOCAL_REQUIRED_MODULES := com.android.nfc.helper.patch.init.rc
LOCAL_DX_FLAGS := --core-library

include $(BUILD_JAVA_LIBRARY)
# ==== init.rc ============================
include $(CLEAR_VARS)

INITRC := $(TARGET_ROOT_OUT)/init.environ.rc
LOCAL_MODULE := com.android.nfc.helper.patch.init.rc
# temporary file to be deleted
LOCAL_SRC_FILES := placeholder/init.rc.placeholder
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
LOCAL_MODULE_CLASS := ETC
LOCAL_POST_INSTALL_CMD := echo Modify BOOTCLASSPATH in init.environ.rc ; \
        sed -i -s 's?\/system\/framework\/ext.jar:\/system\/framework\/framework.jar?\/system\/framework\/ext.jar:\/system\/framework\/com.android.nfc.helper.jar:\/system\/framework\/framework.jar?g' $(INITRC) ; \
	rm $(TARGET_ROOT_OUT)/$(LOCAL_MODULE)

include $(BUILD_PREBUILT)
$(LOCAL_BUILT_MODULE) : $(INSTALLED_RAMDISK_TARGET)
# ====  permissions ========================
include $(CLEAR_VARS)

LOCAL_MODULE := com.android.nfc.helper.xml

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := ETC

# Install to /system/etc/permissions
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions

LOCAL_SRC_FILES := android/$(LOCAL_MODULE)

include $(BUILD_PREBUILT)

# the documentation
# ============================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-subdir-java-files) $(call all-subdir-html-files)

LOCAL_MODULE:= com.android.nfc.helper.doc
LOCAL_DROIDDOC_OPTIONS := com.android.nfc.helper
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_DROIDDOC_USE_STANDARD_DOCLET := true

include $(BUILD_DROIDDOC)

########################################
# NCI Configuration
########################################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES += \
        $(call all-java-files-under, src)

LOCAL_SRC_FILES += \
        $(call all-java-files-under, nci)

LOCAL_PACKAGE_NAME := NfcDNci
LOCAL_OVERRIDES_PACKAGES := Nfc
LOCAL_CERTIFICATE := platform

LOCAL_STATIC_JAVA_LIBRARIES := NfcDLogTags
LOCAL_JAVA_LIBRARIES := com.android.nfc.helper

LOCAL_REQUIRED_MODULES  := libnfcD_nci_jni com.android.nfc.helper

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_PACKAGE)

#####
# static lib for the log tags
#####
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := src/com/android/nfc/EventLogTags.logtags

LOCAL_MODULE:= NfcDLogTags

include $(BUILD_STATIC_JAVA_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
endif
