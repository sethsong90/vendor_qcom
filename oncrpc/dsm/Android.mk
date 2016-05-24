LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../oncrpc_defines.mk

LOCAL_COPY_HEADERS_TO := oncrpc/inc/oncrpc
LOCAL_COPY_HEADERS    := ../inc/oncrpc/err.h
LOCAL_COPY_HEADERS    += ../inc/oncrpc/oncrpc_cb.h
LOCAL_COPY_HEADERS    += ../inc/oncrpc/oncrpc_clnt.h
LOCAL_COPY_HEADERS    += ../inc/oncrpc/oncrpc_init.h
LOCAL_COPY_HEADERS    += ../inc/oncrpc/oncrpc_lookup.h
LOCAL_COPY_HEADERS    += ../inc/oncrpc/oncrpc_mem.h
LOCAL_COPY_HEADERS    += ../inc/oncrpc/oncrpc_svc_err.h
LOCAL_COPY_HEADERS    += ../inc/oncrpc/oncrpc_svc.h
LOCAL_COPY_HEADERS    += ../inc/oncrpc/oncrpc_target.h
LOCAL_COPY_HEADERS    += ../inc/oncrpc/oncrpc_xdr.h
LOCAL_COPY_HEADERS    += ../inc/oncrpc/oncrpc_xdr_types.h

common_libdsm_cflags := -g
common_libdsm_cflags +=	-O0
common_libdsm_cflags +=	-fno-inline
common_libdsm_cflags +=	-fno-short-enums
common_libdsm_cflags +=	$(oncrpc_common_defines)
common_libdsm_cflags +=	-DFEATURE_DSM_NATIVE_LINUX

libdsm_includes := $(LOCAL_PATH)/../inc
libdsm_includes += $(LOCAL_PATH)/../inc/oncrpc
libdsm_includes += $(TARGET_OUT_HEADERS)/common/inc
libdsm_includes += $(TARGET_OUT_HEADERS)/diag/include

LOCAL_CFLAGS := $(common_libdsm_cflags)

LOCAL_C_INCLUDES := $(libdsm_includes)

LOCAL_CFLAGS += -include $(LOCAL_PATH)/../inc/oncrpc/err.h

LOCAL_SRC_FILES := dsm.c
LOCAL_SRC_FILES += dsmi.c
LOCAL_SRC_FILES += dsm_queue.c
LOCAL_SRC_FILES += dsm_pool.c
LOCAL_SRC_FILES += dsm_init.c
LOCAL_SRC_FILES += dsm_lock.c
LOCAL_SRC_FILES += dsm_rex.c

LOCAL_SHARED_LIBRARIES := libqueue
LOCAL_SHARED_LIBRARIES += libdiag

LOCAL_LDLIBS += -lpthread  

LOCAL_MODULE:= libdsm

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
