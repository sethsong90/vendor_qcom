####################################################################
# ONCRPC
####################################################################
# sources and intermediate files are separated
vpath %.c $(SRCDIR)
vpath %.s $(SRCDIR)

# Define default transport. Can be overridden on the comamnd line
FEATURE_ONCRPC_TRANSPORT = FEATURE_ONCRPC_ROUTER

CFLAGS   += $(subst -Werror,,$(QCT_CFLAGS))
CFLAGS   += -g

CPPFLAGS += $(subst -Werror,,$(QCT_CPPFLAGS))

CPPFLAGS += -DONCRPC_REPLY_SIG=0x00800000
CPPFLAGS += -Doncrpc_printf=printf
CPPFLAGS += -DONCRPC_ERR_FATAL=ERR_FATAL
CPPFLAGS += -DFEATURE_ONCRPC_SM_IS_ROUTER
CPPFLAGS += -D$(FEATURE_ONCRPC_TRANSPORT)
CPPFLAGS += -DFEATURE_ONCRPC
CPPFLAGS += -DFEATURE_USE_LINUX_POLL_H
CPPFLAGS += -D__linux
CPPFLAGS += -DRPC_ROUTER_LOCAL_PROCESSOR_ID=1
CPPFLAGS += -DRPC_ROUTER_REMOTE_DEFAULT_PROCESSOR_ID=0

ifeq ("fsm9xxx_surf","$(TARGET)")
CPPFLAGS += -DONCRPC_64K_RPC
endif

ifeq ("fsm9xxx_serval","$(TARGET)")
CPPFLAGS += -DONCRPC_64K_RPC
endif


CPPFLAGS += -I$(SRCDIR)/../inc
CPPFLAGS += -I$(SRCDIR)/../inc/oncrpc
CPPFLAGS += -I$(SRCDIR)/../dsm
CPPFLAGS += -I$(SRCDIR)/../queue
  
LDLIBS   += -lrt 
LDLIBS   += $(OBJDIR)/libdsm.so.$(LIBVER)
LDLIBS   += $(OBJDIR)/libqueue.so.$(LIBVER)

STATIC_LIBNAME  := liboncrpc.a.$(LIBVER)
LIBNAME  := liboncrpc.so.$(LIBVER)

SRCLIST  := oncrpc_cb_table.gas.s
SRCLIST  += oncrpc_cb.c
SRCLIST  += oncrpc_clnt.c
SRCLIST  += oncrpc_init.c
SRCLIST  += oncrpc_linux.c
SRCLIST  += oncrpc_lookup.c
SRCLIST  += oncrpc_main_linux.c
SRCLIST  += oncrpc_main.c
SRCLIST  += oncrpc_mem.c
SRCLIST  += oncrpc_pacmark.c
SRCLIST  += oncrpc_plugger_server.c
SRCLIST  += oncrpc_proxy.c
SRCLIST  += oncrpc_svc_auth.c
SRCLIST  += oncrpc_svc_err.c
SRCLIST  += oncrpc_svc.c
SRCLIST  += oncrpc_xdr_array.c
SRCLIST  += oncrpc_xdr.c
SRCLIST  += oncrpc_xdr_ref.c
SRCLIST  += oncrpc_xdr_std.c
SRCLIST  += oncrpc_rtr_linux.c
SRCLIST  += oncrpc_rtr.c

all: $(LIBNAME) $(STATIC_LIBNAME)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(QCT_CFLAGS_SO)  -Wl, -c  -o $@ $^

$(STATIC_LIBNAME): $(addsuffix .o,$(basename $(SRCLIST)))
	$(AR) -r $@ $^

$(LIBNAME): $(SRCLIST)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(QCT_CFLAGS_SO) $(QCT_LDFLAGS_SO) -Wl,-soname,liboncrpc.so.$(LIBMAJOR) -o $@ $^ $(LDLIBS)

