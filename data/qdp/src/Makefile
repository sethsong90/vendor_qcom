CC := gcc

INCDIR := ../inc
SRCDIR := ../src
OBJDIR := ../obj
LIBDIR := ../lib
BINDIR := ../bin
TARGET := libqdp.so

# Send logging to stderr
CFLAGS += -MD -DFEATURE_DATA_LOG_STDERR
CFLAGS += -fPIC

LDFLAGS := -shared -Wl,-soname,$(TARGET)
LDFLAGS += -Wl,-rpath,../../../qmi/lib
LDFLAGS += -Wl,-rpath,../../dsutils/lib

LIBS := \
  -ldsutils \
  -lqmi \

LIBDIRS := \
  -L../../dsutils/lib \
  -L../../../qmi/lib

INCPATHS := \
  -I. \
  -I$(INCDIR) \
  -I../../../qmi/inc \
  -I../../../qmi/src \
  -I../../../qmi/platform \
  -I../../dsutils/inc \
  -I../../../common/inc \

SOURCES := \
  qdp.c \
  qdp_platform.c

OBJECTS := $(addprefix $(OBJDIR)/, $(SOURCES:.c=.o))

$(LIBDIR)/$(TARGET): $(OBJECTS)
	@-mkdir -p $(LIBDIR) 2> /dev/null
	$(CC) -o $@ $(LDFLAGS) $(OBJECTS) $(LIBDIRS) $(LIBS)

$(OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.c Makefile
	@-mkdir -p $(OBJDIR) 2> /dev/null
	$(CC) -c $(CFLAGS) $(INCPATHS) -o $@  $<

clean:
	rm -f $(OBJECTS) $(LIBDIR)/$(TARGET)

