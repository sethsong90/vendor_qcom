# ---------------------------------------------------------------------------------
#                 RA Interface
# ---------------------------------------------------------------------------------
ifeq ($(call is-vendor-board-platform,QCOM),true)
ifneq ($(call is-board-platform,copper),true)

ROOT_DIR := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_PATH := $(ROOT_DIR)

LOCAL_COPY_HEADERS_TO := mm-ra/include
LOCAL_COPY_HEADERS    := inc/IHttpHandler.h
LOCAL_COPY_HEADERS    += inc/IHttpStatusNotificationHandler.h
LOCAL_COPY_HEADERS    += inc/HttpHandlerFactory.h
LOCAL_COPY_HEADERS    += inc/IStreamManager.h
LOCAL_COPY_HEADERS    += inc/QTATypes.h

include $(BUILD_COPY_HEADERS)

endif  #is-board-platform
endif  #is-vendor-board-platform
