# ---------------------------------------------------------------------------------
#				MM-OMX-ADEC_AAC
# ---------------------------------------------------------------------------------

# Source Path
ADEC_AAC := $(SRCDIR)/omx/adec-aac/qdsp5

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

all: libOmxAacDec.so mm-adec-omxaac-test

# ---------------------------------------------------------------------------------
#				COMPILE LIBRARY
# ---------------------------------------------------------------------------------

SRCS := $(ADEC_AAC)/../../common/qdsp5/src/omx_utils.c
SRCS += $(ADEC_AAC)/../../common/qdsp5/src/omx_base.cpp
SRCS += $(ADEC_AAC)/../../common/qdsp5/src/omx_base_dec.cpp
SRCS += $(ADEC_AAC)/../../common/qdsp5/src/omx_base_trsc.cpp
SRCS += $(ADEC_AAC)/src/omx_dec_aac.cpp
SRCS += $(ADEC_AAC)/src/omx_trsc_aac.cpp

CPPFLAGS += -I$(ADEC_AAC)/inc
CPPFLAGS += -I$(ADEC_AAC)/../../common/qdsp5/inc
CPPFLAGS += -I$(SYSROOTINC_DIR)/mm-core
CPPFLAGS += -I$(KERNEL_DIR)/include
CPPFLAGS += -I$(KERNEL_DIR)/arch/arm/include

CPPFLAGS += -D_ENABLE_QC_MSG_LOG_
CPPFLAGS += -DAUDIOV2

LDLIBS := -lrt
LDLIBS += -lpthread

libOmxAacDec.so:$(SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_SO) $(LDFLAGS_SO) -Wl,-soname,libOmxAacDec.so -o $@ $^ $(LDLIBS)

# ---------------------------------------------------------------------------------
#				COMPILE TEST APP
# ---------------------------------------------------------------------------------

TEST_LDLIBS := -lpthread
TEST_LDLIBS += -ldl
TEST_LDLIBS += -lstdc++
TEST_LDLIBS += -lOmxCore
TEST_LDLIBS += -laudioalsa

CPPFLAGS += -I$(SRCDIR)/audio-alsa/inc

SRCS := $(ADEC_AAC)/test/omx_aac_dec_test.c

mm-adec-omxaac-test: libOmxAacDec.so $(SRCS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(TEST_LDLIBS)

# ---------------------------------------------------------------------------------
#					END
# ---------------------------------------------------------------------------------
