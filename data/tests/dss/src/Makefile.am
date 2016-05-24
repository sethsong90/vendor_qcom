AM_CFLAGS = -Wundef \
        -Wno-trigraphs \
        -g -O0 \
        -fno-inline \
        -fno-short-enums \
        -fpic

#refer to Androik.mk for staging header files
AM_CPPFLAGS = -D__packed__= \
        -DIMAGE_APPS_PROC \
        -DFEATURE_Q_NO_SELF_QPTR \
        -DAEEINTERFACE_CPLUSPLUS \
        -DFEATURE_SMD \
        -DFEATURE_DIAG_MP_MASTER_APPS \
        -DFEATURE_DSS_LINUX \
        -DFEATURE_DATA_LINUX_LE \
        -DFEATURE_DSS_NO_DSSOCK \
        -DFEATURE_MULTIPROCESSOR \
        -DFEATURE_DSUTILS_OFFTARGET \
        -DFEATURE_NO_REX \
        -I$(top_srcdir)/dss_new/src/platform/inc \
        -I../inc \
        -I$(top_srcdir)/netmgr/inc \
        -I$(top_srcdir)/dss_new/src/api \
        -I$(top_srcdir)/dss_new/src/api/dss \
        -I$(top_srcdir)/dss_new/src/api/netiface \
        -I$(top_srcdir)/dss_new/src/api/utils \
        -I$(top_srcdir)/dss_new/src/dss/inc \
        -I$(top_srcdir)/dss_new/src/dsnet/inc \
        -I$(top_srcdir)/dss_new/src/dssock/inc \
        -I$(top_srcdir)/dss_new/src/dssock/src \
        -I$(top_srcdir)/dss_new/src/dssvc/inc \
        -I$(top_srcdir)/dss_new/src/inet/inc \
        -I$(top_srcdir)/dss_new/src/utils/src \
        -I$(top_srcdir)/dss_new/src/utils/inc \
        -I$(top_srcdir)/dss_new/src/tasks/inc \
        -I$(top_srcdir)/dss_new/src/netiface/src \
        -I$(top_srcdir)/dss_new/src/netiface/inc \
        -I$(top_srcdir)/dss_new/inc \
        $(QMI_CFLAGS) \
        -I$(top_srcdir)/dss_new/src/qmiifacectls/src \
        -I$(top_srcdir)/dss_new/src/qmiifacectls/inc \
        $(DIAG_CFLAGS) \
        $(DSUTILS_CFLAGS) \
        -I$(WORKSPACE)/common/inc

libdsstest_c_sources = dss_test.c

requiredlibs = -lpthread \
        ../../../netmgr/src/libnetmgr.la \
        ../../../dss_new/src/libdsnet.la \
        ../../../dss_new/src/libdssnew.la

library_includedir = $(pkgincludedir)

lib_LTLIBRARIES = libdsstest.la
libdsstest_la_SOURCES = $(libdsstest_c_sources)
if USE_GLIB
libdsstest_la_CFLAGS  = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
libdsstest_la_LDFLAGS  = $(DIAG_LIBS) $(QMI_LIBS) $(DSUTILS_LIBS) -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libdsstest_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libdsstest_la_CFLAGS = $(AM_CFLAGS) $(DIAG_CFLAGS) -fPIC
libdsstest_la_LDFLAGS = -lpthread $(DSUTILS_LIBS) $(DIAG_LIBS) $(QMI_LIBS) -shared -version-info 1:0:0
libdsstest_la_CPPFLAGS = $(AM_CPPFLAG)
endif
libdsstest_la_LIBADD = $(requiredlibs) $(DSUTILS_LIBS)

######test executables##########

testsdir = /tmp/tests

dss_test_netc_CC = @CC@
dss_test_netc_SOURCES = dss_test_netc.c
if USE_GLIB
dss_test_netc_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
dss_test_netc_LDFLAGS = $(DIAG_LIBS) @GLIB_LIBS@
else
dss_test_netc_CFLAGS = $(AM_CFLAGS) $(DIAG_CFLAGS) -fPIC
dss_test_netc_LDFLAGS = $(DIAG_LIBS)
endif
dss_test_netc_CPPFLAGS = $(AM_CPPFLAGS)
dss_test_netc_LDADD = $(requiredlibs) libdsstest.la $(DSUTILS_LIBS) $(DIAG_LIBS)

tests_PROGRAMS = dss_test_netc
tests_DATA = dssnetc.sh
