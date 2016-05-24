# ---------------------------------------------------------------------------------
#				MM-OMX-ADEC_QCELP13
# ---------------------------------------------------------------------------------

# Source Path
ADEC_QCELP13 := $(SRCDIR)/omx/adec-qcelp13/qdsp5

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

all: libOmxQcelpDec.so mm-adec-omxQcelp13-test mm-adec-omxvam-test

# ---------------------------------------------------------------------------------
#				COMPILE LIBRARY
# ---------------------------------------------------------------------------------

SRCS := $(ADEC_QCELP13)/src/adec_svr.c
SRCS += $(ADEC_QCELP13)/src/omx_Qcelp13_adec.cpp

CPPFLAGS += -I$(ADEC_QCELP13)/inc
CPPFLAGS += -I$(SYSROOTINC_DIR)/mm-core
CPPFLAGS += -I$(KERNEL_DIR)/include
CPPFLAGS += -I$(KERNEL_DIR)/arch/arm/include

CPPFLAGS += -D_ENABLE_QC_MSG_LOG_
CPPFLAGS += -DAUDIOV2

LDLIBS := -lrt
LDLIBS += -lpthread

libOmxQcelpDec.so:$(SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_SO) $(LDFLAGS_SO) -Wl,-soname,libOmxQcelpDec.so -o $@ $^ $(LDLIBS)

# ---------------------------------------------------------------------------------
#				COMPILE TEST APP
# ---------------------------------------------------------------------------------

TEST_LDLIBS := -lpthread
TEST_LDLIBS += -ldl
TEST_LDLIBS += -lstdc++
TEST_LDLIBS += -lOmxCore
TEST_LDLIBS += -laudioalsa

CPPFLAGS += -I$(SRCDIR)/audio-alsa/inc

SRCS := $(ADEC_QCELP13)/test/omx_Qcelp13_dec_test.c

mm-adec-omxQcelp13-test: libOmxQcelpDec.so $(SRCS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(TEST_LDLIBS)

# ---------------------------------------------------------------------------------
#					END
# ---------------------------------------------------------------------------------
 
# ---------------------------------------------------------------------------------
# Test Application for Answering Machine
# ---------------------------------------------------------------------------------
 
TEST_LDLIBS := -lpthread
TEST_LDLIBS += -ldl
TEST_LDLIBS += -lstdc++
TEST_LDLIBS += -lOmxCore
TEST_LDLIBS += -laudioalsa

CPPFLAGS += -I$(SRCDIR)/audio-alsa/inc

SRCS := $(ADEC_QCELP13)/test/omx_vam_dec_test.c

mm-adec-omxvam-test: libOmxQcelpDec.so $(SRCS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(TEST_LDLIBS)

