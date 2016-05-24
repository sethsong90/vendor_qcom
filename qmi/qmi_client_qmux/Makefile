CC := gcc

HIDE := @

INCDIR := ../inc
SRCDIR := ../src
PLATDIR := ../platform
OBJDIR := ../obj
BINDIR := ../bin
LIBDIR := ../lib
TARGET := libqmi_client_qmux.so

CFLAGS := -DFEATURE_DATA_LOG_STDERR
CFLAGS += -fPIC

LDFLAGS := -shared -Wl,-soname,$(TARGET)

LIBS := -lpthread

INCPATHS := \
	-I. \
	-I$(INCDIR) \
	-I$(SRCDIR) \
	-I$(PLATDIR) \
        -I../core/lib/inc \
        -I../proxy \
	-I../../common/inc

PLATDIR_SOURCES := linux_qmi_qmux_if_client.c \
                   qmi_platform.c
SRCDIR_SOURCES  := qmi_qmux_if.c \
                   qmi_util.c

PLATDIR_OBJS := $(addprefix $(OBJDIR)/, $(PLATDIR_SOURCES:.c=.o))
SRCDIR_OBJS := $(addprefix $(OBJDIR)/, $(SRCDIR_SOURCES:.c=.o))
OBJECTS := $(PLATDIR_OBJS) \
           $(SRCDIR_OBJS)

$(LIBDIR)/$(TARGET): $(PLATDIR_OBJS) $(SRCDIR_OBJS) Makefile
	@-mkdir -p $(LIBDIR) 2> /dev/null
	$(HIDE) $(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

$(PLATDIR_OBJS): $(OBJDIR)/%.o: $(PLATDIR)/%.c
	@-mkdir -p $(OBJDIR) 2> /dev/null
	$(HIDE) $(CC) -c $(CFLAGS) $(INCPATHS) -o $@ $<

$(SRCDIR_OBJS): $(OBJDIR)/%.o: $(SRCDIR)/%.c
	@-mkdir -p $(OBJDIR) 2> /dev/null
	$(HIDE) $(CC) -c $(CFLAGS) $(INCPATHS) -o $@ $<

clean:
	rm -f $(OBJECTS) $(LIBDIR)/$(TARGET)

all: $(LIBDIR)/$(TARGET)
