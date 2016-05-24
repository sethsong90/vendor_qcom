AC_PREREQ(2.61)
AC_INIT([qmi],
        1.0.0)
AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR([src/qmi.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_SUBST([DIAG_CFLAGS])
AC_SUBST([DIAG_LIBS])
AC_PROG_CC
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

# Checks for libraries.
PKG_CHECK_MODULES([CONFIGDB], [configdb])
AC_SUBST([CONFIGDB_CFLAGS])
AC_SUBST([CONFIGDB_LIBS])

PKG_CHECK_MODULES([DIAG], [diag])
AC_SUBST([DIAG_CFLAGS])
AC_SUBST([DIAG_LIBS])

AC_ARG_WITH([stderr],
      AC_HELP_STRING([--with-stderr=@<:@dir@:>@],
         [Enable stderr logging]),
      [],
      with_stderr=no)

if test "x$with_stderr" != "xno"; then
   CPPFLAGS="${CPPFLAGS} -DFEATURE_DATA_LOG_STDERR"
fi

AC_ARG_WITH([qxdm],
      AC_HELP_STRING([--with-qxdm=@<:@dir@:>@],
         [Enable qxdm logging]),
      [],
      with_qxdm=no)

if test "x$with_qxdm" != "xno"; then
   CPPFLAGS="${CPPFLAGS} -DFEATURE_DATA_LOG_QXDM"
fi

AC_ARG_WITH([common_includes],
      AC_HELP_STRING([--with-common-includes=@<:@dir@:>@],
         [Specify the location of the common headers]),
      [common_incdir=$withval],
      with_common_includes=no)

if test "x$with_common_includes" != "xno"; then
   CFLAGS="${CFLAGS} -I${common_incdir}"
fi

AC_SUBST([CPPFLAGS])

AC_CONFIG_FILES([ \
        Makefile \
        core/lib/src/Makefile \
        services/Makefile \
        qmi_client_qmux/Makefile \
        qcci_legacy/Makefile \
        src/Makefile  \
        qmuxd/Makefile \
        tests/Makefile \
        qmi.pc
        ])
AC_OUTPUT
