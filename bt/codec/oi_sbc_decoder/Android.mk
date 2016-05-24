LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO   := codecs/decoder/inc

LOCAL_COPY_HEADERS      += ./include/oi_codec_sbc.h
LOCAL_COPY_HEADERS      += ./include/oi_status.h
LOCAL_COPY_HEADERS      += ./include/oi_stddefs.h
LOCAL_COPY_HEADERS      += ./include/oi_assert.h
LOCAL_COPY_HEADERS      += ./include/oi_bitstream.h
LOCAL_COPY_HEADERS      += ./include/oi_bt_spec.h
LOCAL_COPY_HEADERS      += ./include/oi_codec_sbc_private.h
LOCAL_COPY_HEADERS      += ./include/oi_common.h
LOCAL_COPY_HEADERS      += ./include/oi_cpu_dep.h
LOCAL_COPY_HEADERS      += ./include/oi_modules.h
LOCAL_COPY_HEADERS      += ./include/oi_osinterface.h
LOCAL_COPY_HEADERS      += ./include/oi_status.h
LOCAL_COPY_HEADERS      += ./include/oi_string.h
LOCAL_COPY_HEADERS      += ./include/oi_time.h
LOCAL_COPY_HEADERS      += ./include/oi_utils.h

# sbc decoder
LOCAL_SRC_FILES+= \
        ./srce/alloc.c \
        ./srce/bitalloc.c \
        ./srce/bitalloc-sbc.c \
        ./srce/bitstream-decode.c \
        ./srce/decoder-oina.c \
        ./srce/decoder-private.c \
        ./srce/decoder-sbc.c \
        ./srce/dequant.c \
        ./srce/framing.c \
        ./srce/framing-sbc.c \
        ./srce/oi_codec_version.c \
        ./srce/synthesis-sbc.c \
        ./srce/synthesis-dct8.c \
        ./srce/synthesis-8-generated.c \

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/srce

LOCAL_MODULE:= liboi_sbc_decoder
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := qcom
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)

include $(BUILD_SHARED_LIBRARY)
