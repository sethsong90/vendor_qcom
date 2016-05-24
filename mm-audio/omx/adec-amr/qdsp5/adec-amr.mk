# ---------------------------------------------------------------------------------
#				MM-OMX-ADEC_AMR
# ---------------------------------------------------------------------------------

# Source Path
ADEC_AMR := $(SRCDIR)/omx/adec-amr/qdsp5

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

all: libOmxAmrRtpDec.so libOmxAmrDec.so mm-adec-omxamr-test

# ---------------------------------------------------------------------------------
#				COMPILE LIBRARY
# ---------------------------------------------------------------------------------

SRCS := $(ADEC_AMR)/rtp/src/bit.c
SRCS += $(ADEC_AMR)/rtp/src/rtp_amr_profile.c
SRCS += $(ADEC_AMR)/rtp/src/rtp_packet.c
SRCS += $(ADEC_AMR)/rtp/src/rtp_api.c

CPPFLAGS += -I$(ADEC_AMR)/inc
CPPFLAGS += -I$(ADEC_AMR)/rtp/inc
CPPFLAGS += -I$(SYSROOTINC_DIR)/mm-core
CPPFLAGS += -I$(KERNEL_DIR)/include
CPPFLAGS += -I$(KERNEL_DIR)/arch/arm/include

CPPFLAGS += -D_ENABLE_QC_MSG_LOG_
CPPFLAGS += -DAUDIOV2

LDLIBS := -lrt
LDLIBS += -lpthread

libOmxAmrRtpDec.so:$(SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_SO) $(LDFLAGS_SO) -Wl,-soname,libOmxAmrRtpDec.so -o $@ $^ $(LDLIBS)

# ---------------------------------------------------------------------------------
#				COMPILE LIBRARY
# ---------------------------------------------------------------------------------

SRCS := $(ADEC_AMR)/src/adec_svr.c
SRCS += $(ADEC_AMR)/src/omx_amr_adec.cpp

LDLIBS := -lrt
LDLIBS += -lpthread

libOmxAmrDec.so:$(SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_SO) $(LDFLAGS_SO) -Wl,-soname,libOmxAmrDec.so -o $@ $^ $(LDLIBS)

# ---------------------------------------------------------------------------------
#				COMPILE TEST APP
# ---------------------------------------------------------------------------------

TEST_LDLIBS := -lpthread
TEST_LDLIBS += -ldl
TEST_LDLIBS += -lstdc++
TEST_LDLIBS += -lOmxCore
TEST_LDLIBS += -laudioalsa

CPPFLAGS += -I$(SRCDIR)/audio-alsa/inc

SRCS := $(ADEC_AMR)/test/omx_amr_dec_test.c

mm-adec-omxamr-test: libOmxAmrRtpDec.so libOmxAmrDec.so $(SRCS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(TEST_LDLIBS)

# ---------------------------------------------------------------------------------
#					END
# ---------------------------------------------------------------------------------
