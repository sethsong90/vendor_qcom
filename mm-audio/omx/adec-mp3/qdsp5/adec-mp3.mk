# ---------------------------------------------------------------------------------
#				MM-OMX-ADEC_MP3
# ---------------------------------------------------------------------------------

# Source Path
ADEC_MP3 := $(SRCDIR)/omx/adec-mp3/qdsp5

# cross-compiler flags
CFLAGS := -Wall 
CFLAGS += -Wundef 
CFLAGS += -Wstrict-prototypes 
CFLAGS += -Wno-trigraphs 

# cross-compile flags specific to shared objects
CFLAGS_SO := $(QCT_CFLAGS_SO)

# Preproc flags
CPPFLAGS := $(QCT_CPPFLAGS)
CPPFLAGS += -D_ENABLE_QC_MSG_LOG_
CPPFLAGS += -DAUDIOV2

# linker flags for shared objects
LDFLAGS_SO += -shared

# linker flags
LDFLAGS := -L$(SYSROOTLIB_DIR)

# hard coding target for 7630
TARGET := 7630

# ---------------------------------------------------------------------------------
#					BUILD
# ---------------------------------------------------------------------------------

all: libOmxMp3Dec.so mm-adec-omxmp3-test

# ---------------------------------------------------------------------------------
#				COMPILE LIBRARY
# ---------------------------------------------------------------------------------

SRCS := $(ADEC_MP3)/src/adec_svr.c
SRCS += $(ADEC_MP3)/src/omx_mp3_adec.cpp

CPPFLAGS += -I$(ADEC_MP3)/inc
CPPFLAGS += -I$(SYSROOTINC_DIR)/mm-core
CPPFLAGS += -I$(KERNEL_DIR)/include
CPPFLAGS += -I$(KERNEL_DIR)/arch/arm/include

CPPFLAGS += -D_ENABLE_QC_MSG_LOG_
CPPFLAGS += -DAUDIOV2

LDLIBS := -lrt
LDLIBS += -lpthread

libOmxMp3Dec.so:$(SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_SO) $(LDFLAGS_SO) -Wl,-soname,libOmxMp3Dec.so -o $@ $^ $(LDLIBS)

# ---------------------------------------------------------------------------------
#				COMPILE TEST APP
# ---------------------------------------------------------------------------------

TEST_LDLIBS := -lpthread
TEST_LDLIBS += -ldl
TEST_LDLIBS += -lstdc++
TEST_LDLIBS += -lOmxCore
TEST_LDLIBS += -laudioalsa

CPPFLAGS += -I$(SRCDIR)/audio-alsa/inc

SRCS := $(ADEC_MP3)/test/omx_mp3_dec_test.c

mm-adec-omxmp3-test: libOmxMp3Dec.so $(SRCS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(TEST_LDLIBS)

# ---------------------------------------------------------------------------------
#					END
# ---------------------------------------------------------------------------------
