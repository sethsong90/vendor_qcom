AM_CFLAGS = -Wundef \
        -Wstrict-prototypes \
        -Wno-trigraphs \
        -g -O0 \
        -fno-inline \
        -fno-short-enums \
        -fpic

AM_CPPFLAGS = -D__packed__= \
        -DIMAGE_APPS_PROC \
        -DFEATURE_DATA_LINUX_LE \
        -DRADISH_OFFLINE

#####################radish##################

radish_sources = radish.c \
        config.c \
        icmpv6.c

radish_h_sources = radish.h

bin_PROGRAMS = radish
radish_CC = @CC@
radish_SOURCES = $(radish_sources) $(radish_h_sources)
if USE_GLIB
radish_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(DIAG_CFLAGS) @GLIB_CFLAGS@
radish_LDFLAGS = -lpthread @GLIB_LIBS@
else
radish_CFLAGS = $(AM_CFLAGS) $(DIAG_CFLAGS) -fPIC
radish_LDFLAGS = -lpthread
endif
radish_CPPFLAGS = $(AM_CPPFLAGS)
radish_LDADD = $(requiredlibs)
