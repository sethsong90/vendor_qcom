##############################################################################
# $Id$
#
# @file  tests/unit/libs/dss/lib/Makefile
# @brief Makefile for building the dsstest library.
#
##############################################################################
vpath %.c $(SRCDIR)

SRCLIST := dss_test.c

CPPFLAGS += -g
CPPFLAGS += -I$(SRCDIR)/../../../dss/inc
CPPFLAGS += -I$(SRCDIR)/../../../dss/src

CPPFLAGS += -I$(SRCDIR)
CPPFLAGS += $(QCT_CPPFLAGS)

CFLAGS += $(QCT_CFLAGS)
CFLAGS += $(QCT_CLFAGS_SO)
LDFLAGS += $(QCT_LDFLAGS_SO)


all: libdsstest.so.$(LIBVER)

libdsstest.so.$(LIBVER): $(SRCLIST)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -Wl,-soname,libdsstest.so.$(LIBMAJOR) -o $@ $^ $(LDLIBS)
