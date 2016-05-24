LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../oncrpc_defines.mk

LOCAL_COPY_HEADERS_TO := oncrpc/inc
LOCAL_COPY_HEADERS    := oncrpc_dsm.h
LOCAL_COPY_HEADERS    += oncrpci.h

LOCAL_COPY_HEADERS    += ../inc/oncrpc.h
LOCAL_COPY_HEADERS    += ../inc/queue.h

LOCAL_COPY_HEADERS    += ../dsm/assert.h
LOCAL_COPY_HEADERS    += ../dsm/dsm.h
LOCAL_COPY_HEADERS    += ../dsm/dsm_init.h
LOCAL_COPY_HEADERS    += ../dsm/dsm_item.h
LOCAL_COPY_HEADERS    += ../dsm/dsm_kind.h
LOCAL_COPY_HEADERS    += ../dsm/dsm_lock.h
LOCAL_COPY_HEADERS    += ../dsm/dsm_pool.h
LOCAL_COPY_HEADERS    += ../dsm/dsm_queue.h
LOCAL_COPY_HEADERS    += ../dsm/dsm_tracer.h
LOCAL_COPY_HEADERS    += ../dsm/memory.h

common_liboncrpc_cflags := -g
common_liboncrpc_cflags += -O0
common_liboncrpc_cflags += -fno-inline
common_liboncrpc_cflags += -fno-short-enums
common_liboncrpc_cflags += -fpic

LOCAL_CFLAGS := $(common_liboncrpc_cflags)
LOCAL_CFLAGS += $(oncrpc_common_defines)
LOCAL_CFLAGS += $(oncrpc_defines)

LOCAL_CFLAGS += -include $(LOCAL_PATH)/../inc/oncrpc/err.h
LOCAL_CFLAGS += -include $(LOCAL_PATH)/../dsm/assert.h

LOCAL_C_INCLUDES := $(TARGET_OUT_HEADERS)/common/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc/oncrpc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../dsm
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../queue
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../smem_log
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/diag/include

LOCAL_SRC_FILES  := oncrpc_cb_table.gas.s
LOCAL_SRC_FILES  += oncrpc_cb.c
LOCAL_SRC_FILES  += oncrpc_clnt.c
LOCAL_SRC_FILES  += oncrpc_init.c
LOCAL_SRC_FILES  += oncrpc_linux.c
LOCAL_SRC_FILES  += oncrpc_lookup.c
LOCAL_SRC_FILES  += oncrpc_main_linux.c
LOCAL_SRC_FILES  += oncrpc_main.c
LOCAL_SRC_FILES  += oncrpc_mem.c
LOCAL_SRC_FILES  += oncrpc_pacmark.c
LOCAL_SRC_FILES  += oncrpc_plugger_server.c
LOCAL_SRC_FILES  += oncrpc_proxy.c
LOCAL_SRC_FILES  += oncrpc_svc_auth.c
LOCAL_SRC_FILES  += oncrpc_svc_err.c
LOCAL_SRC_FILES  += oncrpc_svc.c
LOCAL_SRC_FILES  += oncrpc_xdr_array.c
LOCAL_SRC_FILES  += oncrpc_xdr.c
LOCAL_SRC_FILES  += oncrpc_xdr_ref.c
LOCAL_SRC_FILES  += oncrpc_xdr_std.c
LOCAL_SRC_FILES  += oncrpc_rtr_linux.c
LOCAL_SRC_FILES  += oncrpc_rtr.c
LOCAL_SRC_FILES  += ../smem_log/smem_log.c

LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libdsm
LOCAL_SHARED_LIBRARIES += libqueue
LOCAL_SHARED_LIBRARIES += libdiag
LOCAL_SHARED_LIBRARIES += liblog

LOCAL_LDLIBS := -lpthread

LOCAL_MODULE := liboncrpc

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_OWNER := qcom
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
