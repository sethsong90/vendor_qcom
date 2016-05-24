
AC_PREREQ([2.61])

AC_INIT([audio-ctrl],[1.0.0])

AM_INIT_AUTOMAKE([-Wall -Wno-portability -Werror gnu foreign])

AM_MAINTAINER_MODE

AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_CPP
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_INSTALL

AC_ARG_WITH([sanitized-headers],
	[AS_HELP_STRING([--with-sanitized-headers=DIR],[location of the sanitized Linux headers])],
	[CPPFLAGS="$CPPFLAGS -I$withval"])

AC_CHECK_HEADERS([linux/msm_audio.h],,
	[AC_MSG_ERROR([cannot find linux/msm_audio.h. Please check the --with-sanitized-headers option])])

AC_ARG_ENABLE([audio-v2],
	[AS_HELP_STRING([--enable-audio-v2],[defines AUDIOV2])],
	[AUDIOV2=$enableval],
	[AUDIOV2=no]
)
AM_CONDITIONAL([AUDIOV2], [test "x$AUDIOV2" = "xyes"])

AC_OUTPUT([ \
	Makefile \
])

