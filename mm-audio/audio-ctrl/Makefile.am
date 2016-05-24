
ACLOCAL_AMFLAGS = -I m4


include_HEADERS := inc/acdb_params.h
include_HEADERS += inc/audioeq.h


common_cppflags := -DQC_MODIFIED
common_cppflags += -DVERBOSE
common_cppflags += -D_DEBUG
if AUDIOV2
common_cppflags += -DAUDIOV2
endif

common_cflags := -g
common_cflags += -O3


# libaudioeq.so
lib_LTLIBRARIES := libaudioeq.la

libaudioeq_la_CPPFLAGS := $(common_cppflags)

libaudioeq_la_CFLAGS := $(common_cflags)

libaudioeq_la_SOURCES := src/audioeq.c

libaudioeq_la_LDFLAGS := -shared
libaudioeq_la_LDFLAGS += -version-info 1:0:0


# mm-audio-ctrl-test
bin_PROGRAMS := mm-audio-ctrl-test

mm_audio_ctrl_test_CPPFLAGS := $(common_cppflags)
mm_audio_ctrl_test_CPPFLAGS += -I inc

mm_audio_ctrl_test_CFLAGS := $(common_cflags)

mm_audio_ctrl_test_SOURCES := test/audio-ctrl.c

mm_audio_ctrl_test_LDADD := libaudioeq.la

