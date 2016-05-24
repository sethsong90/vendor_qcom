################################################################################
# $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/Makefile#1 $
# 
# @file tests/unit/libs/dss/src/Makefile
# @brief Makefile for building dss api tests.
#
################################################################################

vpath %.c $(SRCDIR)

CPPFLAGS += -g
CPPFLAGS += -DFEATURE_DS_LINUX_NO_RPC
CPPFLAGS += -I$(SRCDIR)
CPPFLAGS += -I$(SRCDIR)/../../../dss/inc
CPPFLAGS += -I$(SRCDIR)/../../../dss/src
CPPFLAGS += -I$(SRCDIR)/../../../../stringl
CPPFLAGS += $(QCT_CPPFLAGS)

CFLAGS += $(QCT_CFLAGS)
CFLAGS += $(QCT_CLFAGS_SO)

LDLIBS += $(OBJDIR)/libdss.so.$(LIBVER)
LDLIBS += $(OBJDIR)/libqmi.so.$(LIBVER)
LDLIBS += $(OBJDIR)/libdsstest.so.$(LIBVER)
LDLIBS += -lstringl
LDLIBS += -lpthread

APPS += dss_test_1
APPS += dss_test_2
APPS += dss_test_3
APPS += dss_test_4
APPS += dss_test_5
APPS += dss_test_6
APPS += dss_test_7
APPS += dss_test_8
APPS += dss_test_9
APPS += dss_test_10
APPS += dss_test_20
APPS += dss_test_21
APPS += dss_test_22
APPS += dss_test_30
APPS += dss_test_31
APPS += dss_test_32
APPS += dss_test_40
APPS += dss_test_50
APPS += dss_test_100
APPS += dss_test_101
APPS += dss_test_102
APPS += dss_test_103
APPS += dss_test_104
APPS += dss_test_master_client
APPS += dss_test_netc

SRCLIST-dss_test_1 := dss_test_1.c
SRCLIST-dss_test_2 := dss_test_2.c
SRCLIST-dss_test_3 := dss_test_3.c
SRCLIST-dss_test_4 := dss_test_4.c
SRCLIST-dss_test_5 := dss_test_5.c
SRCLIST-dss_test_6 := dss_test_6.c
SRCLIST-dss_test_7 := dss_test_7.c
SRCLIST-dss_test_8 := dss_test_8.c
SRCLIST-dss_test_9 := dss_test_9.c
SRCLIST-dss_test_10 := dss_test_10.c
SRCLIST-dss_test_20 := dss_test_20.c
SRCLIST-dss_test_21 := dss_test_21.c
SRCLIST-dss_test_22 := dss_test_22.c
SRCLIST-dss_test_30 := dss_test_30.c
SRCLIST-dss_test_31 := dss_test_31.c
SRCLIST-dss_test_32 := dss_test_32.c
SRCLIST-dss_test_40 := dss_test_40.c
SRCLIST-dss_test_50 := dss_test_50.c
SRCLIST-dss_test_100 := dss_test_100.c
SRCLIST-dss_test_101 := dss_test_101.c
SRCLIST-dss_test_102 := dss_test_102.c
SRCLIST-dss_test_103 := dss_test_103.c
SRCLIST-dss_test_104 := dss_test_104.c
SRCLIST-dss_test_master_client := dss_test_master_client.c
SRCLIST-dss_test_netc := dss_test_netc.c

LDLIBS += -ldiag

all: $(APPS) 

.SECONDEXPANSION:
$(APPS) : $$(SRCLIST-$$@) 
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
