# BUILD dsi_netctrl off target test suite
# pre-req:
# from dsutils/src location
# make <-- This will create libdsutils.so library
# from qdi/src location
# make <-- This will create libqdi.so library
# from dsi_netctrl/src location,
# make <-- This will create libdsi_netctrl.so library
# make teststubs <-- This will create dsi_netctrl test process (uses stub functions)
# make clean; make testnostubs <-- This will create dsi_netctrl test process (calls into actual libraries)
# ../bin/dsi_netctrl_test_pd <-- This will run test engine (uses stub functions)
# ../bin/dsi_netctrl_test_pd_ns <-- This will run test engine (calls into actual libraries)

CC := gcc

INCDIR := ../inc
SRCDIR := ../src
OBJDIR := ../obj
BINDIR := ../bin
LIBDIR := ../lib
TARGET_STUBS := dsi_netctrl_test_pd
TARGET_NO_STUBS := dsi_netctrl_test_pd_ns
LIBTARGET := libdsi_netctrl.so

# Send logging to stderr
CFLAGS += -MD -DFEATURE_DATA_LOG_STDERR
CFLAGS += -g -DDSI_NETCTRL_OFFTARGET
CFLAGS += -fPIC
CFLAGS += -DFEATURE_DSI_MULTIMODEM_ROUTELOOKUP
CFLAGS += -DFEATURE_DSI_TEST
CFLAGS += -Dstrlcpy=strncpy -Dstrlcat=strncat

LDFLAGS := -Wl,-rpath,../../dsutils/lib
LDFLAGS += -Wl,-rpath,../../qdi/lib
LDFLAGS += -Wl,-rpath,../../../qmi/lib
LDFLAGS += -Wl,-rpath,../../netmgr/lib

LIBS := \
  -ldsutils \
  -lpthread \
  -lnetmgr \
  -lqdi

LIBS_STUBS := \
  -ldsutils \
  -lpthread \

LIBDIRS := \
  -L../../dsutils/lib \
  -L../../netmgr/lib \
  -L../../qdi/lib \

LIBDIRS_STUBS := \
  -L../../dsutils/lib \

INCPATHS := \
  -I. \
  -I$(INCDIR) \
  -I../../netmgr/inc \
  -I../../qdi/inc \
  -I../../../qmi/inc \
  -I../../../qmi/src \
  -I../../../qmi/platform \
  -I../../dsutils/inc \
  -I../../../common/inc

TEST_SOURCES := \
  dsi_test_util.c \
  dsi_test_engine.c \
  dsi_test_template.c \
  dsi_test_1.c \
  dsi_test_embms.c

TEST_SOURCES_STUBS := \
  dsi_netctrl_stubs.c

LIB_SOURCES := \
  dsi_netctrl.c \
  dsi_netctrl_init.c \
  dsi_netctrli.c \
  dsi_netctrl_mni_cb.c \
  dsi_netctrl_mni.c \
  dsi_netctrl_multimodem.c \
  dsi_netctrl_netmgr.c \
  dsi_netctrl_platform.c \
  dsi_netctrl_cb_thrd.c \
  dsi_netctrl_qos.c \
  dsi_netctrl_embms.c

SOURCES := \
  $(LIB_SOURCES) \
  $(TEST_SOURCES)

SOURCES_STUBS := \
  $(SOURCES) \
  $(TEST_SOURCES_STUBS)

OBJECTS := $(addprefix $(OBJDIR)/, $(SOURCES:.c=.o))
OBJECTS_STUBS := $(addprefix $(OBJDIR)/, $(SOURCES_STUBS:.c=.o))
LIB_OBJECTS := $(addprefix $(OBJDIR)/, $(LIB_SOURCES:.c=.o))

$(LIBDIR)/$(LIBTARGET): $(LIB_OBJECTS)
	@-mkdir -p $(LIBDIR) 2> /dev/null
	$(CC) -shared -Wl,-soname,$(LIBTARGET) -o $@ $(LDFLAGS) $(LIB_OBJECTS) $(LIBDIRS) $(LIBS)

$(LIB_OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.c Makefile
	@-mkdir -p $(OBJDIR) 2> /dev/null
	$(CC) -c $(CFLAGS) $(INCPATHS) -o $@  $<

$(BINDIR)/$(TARGET_STUBS): $(OBJECTS_STUBS)
	@-mkdir -p $(BINDIR) 2> /dev/null
	$(CC) -o $@ $(LDFLAGS) $(OBJECTS_STUBS) $(LIBDIRS_STUBS) $(LIBS_STUBS)

$(OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.c Makefile
	@-mkdir -p $(OBJDIR) 2> /dev/null
	$(CC) -c $(CFLAGS) $(INCPATHS) -o $@  $<

$(OBJECTS_STUBS): $(OBJDIR)/%.o: $(SRCDIR)/%.c Makefile
	@-mkdir -p $(OBJDIR) 2> /dev/null
	$(CC) -c $(CFLAGS) $(INCPATHS) -o $@  $<

$(BINDIR)/$(TARGET_NO_STUBS): $(OBJECTS)
	@-mkdir -p $(BINDIR) 2> /dev/null
	$(CC) -o $@ $(LDFLAGS) $(OBJECTS) $(LIBDIRS) $(LIBS)

.PHONY: clean teststubs testnostubs library

clean:
	rm -f $(OBJDIR)/* $(BINDIR)/$(TARGET_STUBS) $(BINDIR)/$(TARGET_NO_STUBS) $(LIBDIR)/$(LIBTARGET)

teststubs: CFLAGS += -DDSI_NETCTRL_USE_STUBS
teststubs: $(BINDIR)/$(TARGET_STUBS)
	$(BINDIR)/$(TARGET_STUBS)

testnostubs: $(BINDIR)/$(TARGET_NO_STUBS)
	$(BINDIR)/$(TARGET_NO_STUBS)

library: $(LIBDIR)/$(LIBTARGET)
