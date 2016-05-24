AM_CFLAGS = -Wundef \
        -Wstrict-prototypes \
        -Wno-trigraphs \
        -g -O0 \
        -fno-inline \
        -fno-short-enums \
        -fpic


AM_CPPFLAGS = -D__packed__= \
        -DIMAGE_APPS_PROC \
        -DFEATURE_Q_NO_SELF_QPTR \
        -DAEEINTERFACE_CPLUSPLUS \
        -DFEATURE_SMD \
        -DFEATURE_DIAG_MP_MASTER_APPS \
        -DFEATURE_DS_LINUX_ANDROID \
        -DFEATURE_DSS_LINUX \
        -DFEATURE_LINUX \
        -DFEATURE_MULTIPROCESSOR \
        -DFEATURE_DSUTILS_OFFTARGET \
        -DFEATURE_NO_REX \
        -DFEATURE_XMLLIB \
        -I$(WORKSPACE)/qmi/services \
        -I$(top_srcdir)/dss_new/src/platform/inc \
        -I$(top_srcdir)/dsutils/inc \
        -I$(top_srcdir)/netmgr/inc \
        -I$(top_srcdir)/dss_new/src/utils/inc \
        -I$(WORKSPACE)/qmi-framework/inc \
        -I$(WORKSPACE)/qmi-framework/qcci/inc \
        -I$(WORKSPACE)/kernel/include \
        $(QMI_CFLAGS) \
        $(DIAG_CFLAGS) \
        $(DSUTILS_CFLAGS) \
        $(XMLLIB_CFLAGS) \
        -I$(WORKSPACE)/common/inc 

requiredlibs = -lpthread -lrt \
        ../../netmgr/src/libnetmgr.la

library_includedir = $(pkgincludedir)

qti_sources = qti_main.c \
        qti_netlink.c \
        qti_qcmap.c \
        qti_cmdq.c \
        qti_rmnet_usb.c \
        qti_rmnet_qmux.c \
        qti_rmnet_xml.c \
        $(WORKSPACE)/qmi/services/qualcomm_mobile_access_point_msgr_v01.c \
        $(WORKSPACE)/qmi/services/wireless_data_administrative_service_v01.c

qti_h_sources = qti.h \
        qti_cmdq.h \
        $(WORKSPACE)/qmi/services/qualcomm_mobile_access_point_msgr_v01.h \
        $(WORKSPACE)/qmi/services/wireless_data_administrative_service_v01.h

etcdir = ${sysconfdir}
etc_SCRIPTS = rmnet_config.txt

init_ddir = ${sysconfdir}/init.d
init_d_SCRIPTS = start_qti_le

bin_PROGRAMS = qti
qti_CC = @CC@
qti_SOURCES = $(qti_sources) $(qti_h_sources)
if USE_GLIB
qti_CFLAGS  = -Dstrlcpy=strncpy -Dstrlcat=strncat -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
qti_LDFLAGS  = $(DIAG_LIBS) $(XMLLIB_LIBS) -lpthread -lqmi_cci @GLIB_LIBS@ -shared -version-info 1:0:0
qti_CPPFLAGS = -Dstrlcpy=strncpy -Dstrlcat=strncat -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
qti_CFLAGS = -Dstrlcpy=strncpy -Dstrlcat=strncat $(AM_CFLAGS) $(DIAG_CFLAGS) -fPIC
qti_LDFLAGS = -lpthread $(DIAG_LIBS) $(XMLLIB_LIBS) -lqmi_cci -shared -version-info 1:0:0
qti_CPPFLAGS = -Dstrlcpy=strncpy -Dstrlcat=strncat $(AM_CPPFLAGS)
endif
qti_CPPFLAGS = $(AM_CPPFLAGS)
qti_LDADD = $(requiredlibs)

