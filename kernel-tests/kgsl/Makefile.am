kgsldir = $(prefix)/kgsl

AM_CPPFLAGS = -D__packed__= \
		-DIMAGE_APPS_PROC \
		-DFEATURE_Q_SINGLE_LINK \
		-DFEATURE_Q_NO_SELF_QPTR \
		-DFEATURE_LINUX \
		-DFEATURE_NATIVELINUX \
		-DFEATURE_DSM_DUP_ITEMS

kgsl_PROGRAMS = kgsl_test
kgsl_test_SOURCES = kgsl_test.c
kgsl_test_SOURCES += kgsl_test_adv.c
kgsl_test_SOURCES += kgsl_helper.c
kgsl_test_SOURCES += kgsl_test_nominal.c
kgsl_test_SOURCES += kgsl_test_repeat.c
kgsl_test_SOURCES += kgsl_test_small.c
kgsl_test_SOURCES += kgsl_test_stress.c
kgsl_test_LDFLAGS= -lpthread
dist_kgsl_SCRIPTS = run.sh
dist_kgsl_SCRIPTS += ../test_env_setup.sh
dist_kgsl_DATA    = README
