CC := gcc

INCDIR := ../inc
SRCDIR := ../src
PLATDIR := ../platform
OBJDIR := ../obj
BINDIR := ../bin
TARGET := qmuxd

LDFLAGS := -Wl,-rpath,../lib/
LDFLAGS += -Wl,-rpath,../../data/dsutils/lib
CFLAGS += -DFEATURE_WAIT_FOR_MODEM_HACK
CFLAGS += -DFEATURE_DATA_LOG_STDERR

LIBS := -lpthread
LIBS += -lqmi
LIBS += -lqmi_client_qmux
LIBS += -ldsutils

LIBDIRS := -L../lib
LIBDIRS += -L../../data/dsutils/lib

INCPATHS := \
	-I. \
	-I$(INCDIR) \
	-I$(PLATDIR) \
	-I$(SRCDIR) \
	-I../../common/inc \
	-I../core/lib/inc \

SRCDIR_SOURCES := \
	qmi_qmux.c \
	qmi_util.c

PLATDIR_SOURCES := \
	linux_qmi_qmux_if_server.c \
	qmi_platform_qmux_io.c \
	qmi_platform.c

SOURCES := PLATDIR_SOURCES \
           SRCDIR_SOURCES

PLATDIR_OBJS := $(addprefix $(OBJDIR)/, $(PLATDIR_SOURCES:.c=.o))
SRCDIR_OBJS := $(addprefix $(OBJDIR)/, $(SRCDIR_SOURCES:.c=.o))
OBJECTS := $(PLATDIR_OBJS) \
           $(SRCDIR_OBJS)

$(BINDIR)/$(TARGET): $(PLATDIR_OBJS) $(SRCDIR_OBJS) Makefile
	@-mkdir -p $(BINDIR) 2> /dev/null
	$(CC) -o $@ $(LDFLAGS) $(OBJECTS) $(LIBDIRS) $(LIBS)

$(PLATDIR_OBJS): $(OBJDIR)/%.o: $(PLATDIR)/%.c
	@-mkdir -p $(OBJDIR) 2> /dev/null
	$(CC) -c $(CFLAGS) $(INCPATHS) -o $@ $<

$(SRCDIR_OBJS): $(OBJDIR)/%.o: $(SRCDIR)/%.c
	@-mkdir -p $(OBJDIR) 2> /dev/null
	$(CC) -c $(CFLAGS) $(INCPATHS) -o $@ $<

clean:
	rm -f $(OBJECTS) $(BINDIR)/$(TARGET)

all: $(BINDIR)/$(TARGET)
