# sources and intermediate files are separated
vpath %.c $(SRCDIR)

CFLAGS   += $(QCT_CFLAGS)

CPPFLAGS += $(QCT_CPPFLAGS)
CPPFLAGS += -I$(SRCDIR)/../inc
CPPFLAGS += -I$(SRCDIR)/../inc/oncrpc

STATIC_LIBNAME  := libqueue.a.$(LIBVER)
LIBNAME  := libqueue.so.$(LIBVER)

SRCLIST	 := queue.c

all: $(LIBNAME) $(STATIC_LIBNAME)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(QCT_CFLAGS_SO)  -Wl, -c  -o $@ $^

$(STATIC_LIBNAME): $(addsuffix .o,$(basename $(SRCLIST)))
	$(AR) -r $@ $^

$(LIBNAME): $(SRCLIST)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(QCT_CFLAGS_SO) $(QCT_LDFLAGS_SO) -Wl,-soname,libqueue.so.$(LIBMAJOR) -o $@ $^ $(LDLIBS)

