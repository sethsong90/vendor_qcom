# sources and intermediate files are separated
vpath %.c $(SRCDIR)

CFLAGS   += $(QCT_CFLAGS)

CPPFLAGS += $(QCT_CPPFLAGS)
CPPFLAGS += -I$(SRCDIR)/../inc
CPPFLAGS += -I$(SRCDIR)/../inc/oncrpc

STATIC_LIBNAME  := libdsm.a.$(LIBVER)
LIBNAME  := libdsm.so.$(LIBVER)

SRCLIST	 := dsm.c
SRCLIST  += dsmi.c
SRCLIST  += dsm_queue.c
SRCLIST  += dsm_pool.c
SRCLIST  += dsm_init.c
SRCLIST  += dsm_lock.c
SRCLIST  += dsm_rex.c

all: $(LIBNAME) $(STATIC_LIBNAME)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(QCT_CFLAGS_SO)  -Wl, -c  -o $@ $^

$(STATIC_LIBNAME): $(addsuffix .o,$(basename $(SRCLIST)))
	$(AR) -r $@ $^

$(LIBNAME): $(SRCLIST)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(QCT_CFLAGS_SO) $(QCT_LDFLAGS_SO) -Wl,-soname,libdsm.so.$(LIBMAJOR) -o $@ $^ $(LDLIBS)

