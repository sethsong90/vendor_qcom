qcedevdir           = $(prefix)/qcedev

qcedev_PROGRAMS     = qcedev_test

qcedev_test_SOURCES = qcedev_test.c

dist_qcedev_SCRIPTS = qcedev_test.sh run.sh ../test_env_setup.sh

LDFLAGS	           += -lrt -lpthread
dist_qcedev_DATA    = README.txt
