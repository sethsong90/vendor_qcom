# ---------------------------------------------------------------------------------
#				MM-OMX-ADEC_AC3
# ---------------------------------------------------------------------------------

# Source Path
ADEC_AC3 := $(SRCDIR)/omx/adec-ac3/qdsp5

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

# hard coding target for 7627a
TARGET := 7627a

# ---------------------------------------------------------------------------------
#					BUILD
# ---------------------------------------------------------------------------------

all: libOmxAc3Dec.so mm-adec-omxac3-test

# ---------------------------------------------------------------------------------
#				COMPILE LIBRARY
# ---------------------------------------------------------------------------------

SRCS := $(ADEC_AC3)/../../common/qdsp5/src/omx_utils.c
SRCS += $(ADEC_AC3)/../../common/qdsp5/src/omx_base.cpp
SRCS += $(ADEC_AC3)/../../common/qdsp5/src/omx_base_dec.cpp
SRCS += $(ADEC_AC3)/../../common/qdsp5/src/omx_base_trsc.cpp
SRCS += $(ADEC_AC3)/src/omx_dec_ac3.cpp
SRCS += $(ADEC_AC3)/src/omx_trsc_ac3.cpp

CPPFLAGS += -I$(ADEC_AC3)/inc
CPPFLAGS += -I$(ADEC_AC3)/../../common/qdsp5/inc
CPPFLAGS += -I$(SYSROOTINC_DIR)/mm-core
CPPFLAGS += -I$(KERNEL_DIR)/include
CPPFLAGS += -I$(KERNEL_DIR)/arch/arm/include

CPPFLAGS += -D_ENABLE_QC_MSG_LOG_
CPPFLAGS += -DAUDIOV2

LDLIBS := -lrt
LDLIBS += -lpthread

libOmxAc3Dec.so:$(SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_SO) $(LDFLAGS_SO) -Wl,-soname,libOmxAc3Dec.so -o $@ $^ $(LDLIBS)

# ---------------------------------------------------------------------------------
#				COMPILE TEST APP
# ---------------------------------------------------------------------------------

TEST_LDLIBS := -lpthread
TEST_LDLIBS += -ldl
TEST_LDLIBS += -lstdc++
TEST_LDLIBS += -lOmxCore
TEST_LDLIBS += -laudioalsa

CPPFLAGS += -I$(SRCDIR)/audio-alsa/inc

SRCS := $(ADEC_AC3)/test/omx_ac3_dec_test.c

mm-adec-omxac3-test: libOmxAc3Dec.so $(SRCS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(TEST_LDLIBS)

# ---------------------------------------------------------------------------------
#					END
# ---------------------------------------------------------------------------------
