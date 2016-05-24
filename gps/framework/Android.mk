#/******************************************************************************
#*@file Android.mk
#* brief Rules to create an invisible package which declares the permissions
#* for accessing location api
#*******************************************************************************/
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#this tag means this package will be available for all eng builds
#note if the OEM decides to include this permission and group assignment in the final build, the
#tag should be changed to 'user'
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, java)

LOCAL_SRC_FILES += \
    java/com/qualcomm/location/lci/ILciResponseListener.aidl \
    java/com/qualcomm/location/lci/ILciService.aidl

LOCAL_JAVA_LIBRARIES := telephony-common \
                        com.android.location.provider

LOCAL_PACKAGE_NAME := com.qualcomm.location

#LOCAL_PROGUARD_FLAG_FILES := proguard.flags
LOCAL_PROGUARD_ENABLED := disabled

LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)

######## Copy permissions for com.qualcomm.location
include $(CLEAR_VARS)

LOCAL_MODULE := com.qualcomm.location.xml

# for geofence service
LOCAL_STATIC_JAVA_LIBRARIES := location_mq_client

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := ETC

LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions

ifneq ($(TARGET_NO_RPC),true)
    LOCAL_SRC_FILES := rpc/$(LOCAL_MODULE)
else
    LOCAL_SRC_FILES := $(LOCAL_MODULE)
endif

include $(BUILD_PREBUILT)

include $(addsuffix /Android.mk, $(addprefix $(LOCAL_PATH)/,native))
