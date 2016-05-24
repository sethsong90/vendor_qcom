# sources and intermediate files are separated
vpath %.c $(SRCDIR)

CPPFLAGS += $(QCT_CPPFLAGS)
CPPFLAGS += -D_GNU_SOURCE 

CPPFLAGS += -g
CPPFLAGS += -DFEATURE_DS_LINUX_NO_RPC

#CPPFLAGS += -DFEATURE_DATA_LOG_ADB
#CPPFLAGS += -DFEATURE_DATA_LOG_SYSLOG
CPPFLAGS += -DFEATURE_DATA_LOG_QXDM

#CPPFLAGS += -D__linux
CPPFLAGS += -I$(SRCDIR)
CPPFLAGS += -I$(SRCDIR)/../inc
CPPFLAGS += -I$(TARGET_OUT_HEADERS)/qmi/inc
CPPFLAGS += -I$(TARGET_OUT_HEADERS)/diag/include/

#CFLAGS   += $(QCT_CFLAGS)
# XXX muzzle warnings. getsockopt prototype differs on android
CFLAGS   += $(patsubst -Werror,,$(QCT_CFLAGS))

DSS_SRCS += ds_fd_pass.c
DSS_SRCS += ds_list.c
DSS_SRCS += ds_socket.c
DSS_SRCS += ds_util.c
DSS_SRCS += dsc_cmd.c
DSS_SRCS += dsc_dcm.c
DSS_SRCS += dsc_kif.c
DSS_SRCS += dsc_main.c
DSS_SRCS += dsc_qmi_wds.c
DSS_SRCS += dsc_test.c
DSS_SRCS += dsc_call.c
DSS_SRCS += dsc_util.c

LDLIBS += $(OBJDIR)/libqmi.so.$(LIBVER)
LDLIBS += -ldiag
LDLIBS += -lstringl

LDFLAGS += $(QCT_LDFLAGS_SO)

all: libdss.so.$(LIBVER)

libdss.so.$(LIBVER): $(DSS_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(QCT_CFLAGS_SO) $(QCT_LDFLAGS_SO) $(LDFLAGS) -Wl,-soname,libdss.so.$(LIBMAJOR) -o $@ $^ -lstringl

