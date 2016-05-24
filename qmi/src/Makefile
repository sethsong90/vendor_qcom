CC := gcc
HIDE := @

INCDIR := ../inc
SRCDIR := ../src
PLATDIR := ../platform
OBJDIR := ../obj
BINDIR := ../bin
LIBDIR := ../lib
TARGET := libqmi.so

LDFLAGS := -shared -Wl,-soname,$(TARGET)
LDFLAGS += -Wl,-rpath,../lib
LDFLAGS += -Wl,-rpath,../../data/dsutils/lib

CFLAGS += -DFEATURE_WAIT_FOR_MODEM_HACK
CFLAGS += -DFEATURE_DATA_LOG_STDERR
CFLAGS += -fPIC

LIBS := -lpthread
LIBS += -ldsutils
LIBS += -lqmi_client_qmux

LIBDIRS := -L../../data/dsutils/lib
LIBDIRS += -L../lib

INCPATHS := \
	-I. \
	-I$(INCDIR) \
	-I$(SRCDIR) \
	-I$(PLATDIR) \
	-I../core/lib/inc \
	-I../../common/inc \
	-I../../data/dsutils/inc \
	-I../proxy

SOURCES := \
	qmi_service.c \
	qmi.c \
	qmi_wds_srvc.c \
	qmi_qos_srvc.c \
	qmi_nas_srvc.c \
	qmi_eap_srvc.c \
	qmi_atcop_srvc.c \
	qmi_wds_utils.c \
	qmi_uim_srvc.c \
	qmi_cat_srvc.c


OBJECTS := $(addprefix $(OBJDIR)/, $(SOURCES:.c=.o))

$(LIBDIR)/$(TARGET): $(OBJECTS)
	@-mkdir -p $(LIBDIR) 2> /dev/null
	$(HIDE) $(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBDIRS) $(LIBS)

$(OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.c Makefile
	@-mkdir -p $(OBJDIR) 2> /dev/null
	$(HIDE) $(CC) -c $(CFLAGS) $(INCPATHS) -o $@ $<

clean:
	rm -f $(OBJECTS) $(LIBDIR)/$(TARGET)

all: $(LIBDIR)/$(TARGET)
