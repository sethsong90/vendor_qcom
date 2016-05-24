# DS Common library makefile
# pre-req:
# from dsutils/src location
# mkdir ../obj, ../lib
# make <-- This will create dsutils archives
# from netmgr/src location
# mkdir ../obj, ../bin
# make
# make test

CC := gcc
#HIDE := @

INCDIR := ../inc
SRCDIR := ../src
OBJDIR := ../obj
BINDIR := ../bin
LIBDIR := ../lib
TARGET := netmgrd
TESTTARGET := netmgrd_test
LIBTARGET := libnetmgr.so

DHCP_CONFIG_SH := udhcpc.sh
KERNEL_CONFIG_SH := qcom_netmgr_kif_test.sh

# Send logging to stderr
CFLAGS += -MD -DFEATURE_DATA_LOG_STDERR -DNETMGR_QOS_ENABLED -DFEATURE_DS_LINUX_NO_RPC
CFLAGS += -g
CFLAGS += -DNETMGR_OFFTARGET
CFLAGS += -fPIC

# Linux libc does not support strl* functions.
CFLAGS += -Dstrlcpy=strncpy -Dstrlcat=strncat

LDFLAGS := -Wl,-rpath,../../dsutils/lib
LDFLAGS += -Wl,-rpath,../../../qmi/lib
LDFLAGS += -Wl,-rpath,../lib

LIBS := \
  -ldsutils \
  -lqmi \
  -lnetmgr \
  -lpthread

LIBDIRS := \
  -L../../dsutils/lib \
  -L../../../qmi/lib \
  -L../lib

INCPATHS := \
  -I. \
  -I$(INCDIR) \
  -I../../../qmi/inc \
  -I../../../qmi/src \
  -I../../../qmi/platform \
  -I../../dsutils/inc \
  -I../../../common/inc \

SOURCES := \
  netmgr.c \
  netmgr_exec.c \
  netmgr_kif.c \
  netmgr_main.c \
  netmgr_netlink.c \
  netmgr_platform.c \
  netmgr_qmi.c \
  netmgr_sm.c \
  netmgr_sm_int.c \
  netmgr_tc.c \
  netmgr_test.c

TEST_SOURCES := \
  $(SOURCES) \
  netmgr_stubs.c

CLIENT_SOURCES :=  \
  netmgr_client.c  \
  netmgr_netlink.c \
  netmgr_util.c

OBJECTS := $(addprefix $(OBJDIR)/, $(SOURCES:.c=.o))
TEST_OBJECTS := $(addprefix $(OBJDIR)/, $(TEST_SOURCES:.c=.o))
CLIENT_OBJECTS := $(addprefix $(OBJDIR)/, $(CLIENT_SOURCES:.c=.o))

.PHONY: all clean test

all: $(LIBDIR)/$(LIBTARGET) $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJECTS)
	@-mkdir -p $(BINDIR) 2> /dev/null
	$(HIDE) $(CC) -o $@ $(LDFLAGS) $(OBJECTS) $(LIBDIRS) $(LIBS)

$(OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.c Makefile
	@-mkdir -p $(OBJDIR) 2> /dev/null
	$(HIDE) $(CC) -c $(CFLAGS) $(INCPATHS) -o $@  $<

$(LIBDIR)/$(LIBTARGET): $(CLIENT_OBJECTS)
	@-mkdir -p $(LIBDIR) 2> /dev/null
	$(HIDE) $(CC) -shared -Wl,-soname,$(LIBTARGET) -o $@ $(LDFLAGS) $(CLIENT_OBJECTS)

$(CLIENT_OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.c Makefile
	@-mkdir -p $(OBJDIR) 2> /dev/null
	$(HIDE) $(CC) -c $(CFLAGS) $(INCPATHS) -o $@  $<

test: $(BINDIR)/$(TESTTARGET)
	$(BINDIR)/$(TESTTARGET) -D -T -l 0 -n 3 -i rmnet

$(BINDIR)/$(TESTTARGET): CFLAGS += -DNETMGR_TEST
$(BINDIR)/$(TESTTARGET): $(LIBDIR)/$(LIBTARGET) $(TEST_OBJECTS)
	@-mkdir -p $(BINDIR) 2> /dev/null
	$(HIDE) $(CC) -o $@ $(LDFLAGS) $(TEST_OBJECTS) $(LIBDIRS) $(LIBS)

$(TEST_OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.c Makefile
	@-mkdir -p $(OBJDIR) 2> /dev/null
	$(HIDE) $(CC) -c $(CFLAGS) $(INCPATHS) -o $@  $<

clean:
	rm -f $(OBJECTS) $(BINDIR)/$(TARGET) $(LIBDIR)/$(LIBTARGET) $(BINDIR)/$(TESTTARGET)

