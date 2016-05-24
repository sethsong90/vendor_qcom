# DS Common library makefile
# pre-req:
# from dsutils/src location
# mkdir ../obj, ../lib
# make <-- This will create dsutils archives

CC := gcc
HIDE := @

INCDIR := ../inc
SRCDIR := ../src
OBJDIR := ../obj
LIBDIR := ../lib
STATICTARGET := libdsutils.a
DYNAMICTARGET := libdsutils.so

LDFLAGS := -shared -Wl,-soname,$(DYNAMICTARGET)

# Send logging to stderr
CFLAGS += -MD -DFEATURE_DATA_LOG_STDERR
CFLAGS += -DFEATURE_DSUTILS_OFFTARGET
CFLAGS += -g
CFLAGS += -Dstrlcat=strncat
CFLAGS += -fPIC

INCPATHS := \
  -I$(INCDIR) \
  -I../../../diag/include \
  -I../../../common/inc \
  -I../../../../../../system/core/include \
  -I/usr/include

SOURCES := \
	ds_cmdq.c \
	ds_list.c \
	ds_util.c \
	ds_sl_list.c \
	stm2.c

OBJECTS := $(addprefix $(OBJDIR)/, $(SOURCES:.c=.o))

$(LIBDIR)/$(STATICTARGET): $(OBJECTS)
	@-mkdir -p $(LIBDIR) 2> /dev/null
	$(HIDE) $(AR) $(ARFLAGS) $@ $(OBJECTS)
	$(HIDE) $(CC) $(LDFLAGS) -o $(LIBDIR)/$(DYNAMICTARGET) $(OBJECTS)

$(OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.c Makefile
	@-mkdir -p $(OBJDIR) 2> /dev/null
	$(HIDE) $(CC) -c $(CFLAGS) $(INCPATHS) -o $@  $<

clean:
	rm -f $(OBJECTS) $(LIBDIR)/$(STATICTARGET) $(LIBDIR)/$(DYNAMICTARGET)

