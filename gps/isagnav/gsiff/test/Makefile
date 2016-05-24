#
# Makefile template for GSIFF Off-target testing
#

#
# You need to change these next three lines for each assignment
#
TOP            = ../../../../../../..
HARDWARE_DIR   = $(TOP)/hardware
LOC_API_DIR    = $(HARDWARE_DIR)/qcom/gps/loc_api
GPS_UTILS_DIR  = $(HARDWARE_DIR)/qcom/gps/utils
QC_PROP_ROOT   = $(TOP)/vendor/qcom/proprietary

#
# Selective compilation to determine which sensor provider you want.
#
FEATURE_GSIFF_ANDROID_HAL = 1
FEATURE_GSIFF_ANDROID_NDK = 1
FEATURE_GSIFF_DSPS = 1

HEADERS          = sensors \
                   ../ \
                   $(QC_PROP_ROOT)/qmi/inc \
                   $(QC_PROP_ROOT)/qmi/core/lib/inc \
                   $(QC_PROP_ROOT)/qmi/platform \
                   $(QC_PROP_ROOT)/qmi-framework/inc \
                   $(QC_PROP_ROOT)/qmi-framework/qcci/inc \
                   $(QC_PROP_ROOT)/qmi-framework/qcsi/inc \
                   $(QC_PROP_ROOT)/qmi-framework/common/inc \
                   ../../../daemon \
                   $(TOP) \
                   $(LOC_API_DIR)/loc_api_v02 \
                   $(GPS_UTILS_DIR) \
                   $(QC_PROP_ROOT)/gps/internal/test_android_gps

EXT_COMMON_OBJS  = ../../../daemon/gpsone_glue_pipe.o \
                   ../../../daemon/gpsone_glue_msg.o \
                   ../../../daemon/gpsone_thread_helper.o \
                   $(GPS_UTILS_DIR)/msg_q.o \
                   $(GPS_UTILS_DIR)/loc_cfg.o \
                   $(GPS_UTILS_DIR)/loc_log.o \
                   $(GPS_UTILS_DIR)/linked_list.o

INT_OBJS        =  ../gsiff_sensor_provider_glue.o \
                   ../gsiff_sensor_provider_common.o \
                   ../gsiff_loc_api_glue.o \
                   ../os_kf.o \
                   loc_api/loc_api_v02_client.o \
                   $(EXT_COMMON_OBJS)

PREPROCESSOR_FLAGS = -DDEBUG_X86 -DLOG_UTIL_OFF_TARGET
INCLUDE_PATH     = $(patsubst %,-I%,$(HEADERS))

#
# Enable the Android NDK sensor provider
#
ifeq ($(FEATURE_GSIFF_ANDROID_NDK),1)
INT_OBJS += ../gsiff_sensor_provider_and_ndk.o \
            ndk/ndk_stubs.o

HEADERS += $(TOP)/frameworks/base/native/include

PREPROCESSOR_FLAGS += -DFEATURE_GSIFF_ANDROID_NDK
endif

#
# Enable the Android HAL sensor provider
#
ifeq ($(FEATURE_GSIFF_ANDROID_HAL),1)
INT_OBJS += ../gsiff_sensor_provider_and_hal.o

HEADERS += $(TOP)/hardware/libhardware/include \
           $(TOP)/system/core/include

PREPROCESSOR_FLAGS += -DFEATURE_GSIFF_ANDROID_HAL
endif

#
# Enable the DSPS (Sensor1) sensor provider
#
ifeq ($(FEATURE_GSIFF_DSPS),1)
INT_OBJS += ../gsiff_sensor_provider_sensor1.o \
            sensors/sensor1.o

HEADERS += $(QC_PROP_ROOT)/sensors/dsps/api

PREPROCESSOR_FLAGS += -DFEATURE_GSIFF_DSPS
endif

#
# You should not need to change anything below this line
#

#
# Relevant man pages:
#
# man -M /software/common/gnu/man gcc
# man -M /software/common/gnu/man as
# man -M /opt/SUNWspro/man lint
#

GCC     = /usr/bin/g++
G++     = /usr/bin/g++
ASM     = $(GCC)

GCC_FLAGS   = -c -g -Wall -D__EXTENSIONS__ $(PREPROCESSOR_FLAGS) $(INCLUDE_PATH)
G++_FLAGS   = -c -g -Wall -D__EXTENSIONS__ $(PREPROCESSOR_FLAGS) $(INCLUDE_PATH)
ASM_FLAGS   = -c -g
LD_FLAGS    = -g -Wall -lpthread -lrt

#
# Standard rules
#

%.o: %.c
	$(GCC) $(GCC_FLAGS) -o $@ $<

%.o: %.cpp
	$(GCC) $(G++_FLAGS) -o $@ $<

#
# Define different targets
#
all: gsiff_daemon 

gsiff_daemon:	$(INT_OBJS) ../gsiff_daemon_manager.o
	@echo "Linking all object modules ..."
	$(GCC) -o $@ $(LD_FLAGS) $(INT_OBJS) ../gsiff_daemon_manager.o
	@echo ""
	@echo "Done."

ss_hack:	$(INT_OBJS) ../../ss_hack/ss_hack.o
	@echo "Linking all object modules ..."
	$(GCC) -o $@ $(LD_FLAGS) $(INT_OBJS) ../../ss_hack/ss_hack.o
	@echo ""
	@echo "Done."

sns_intercept:	$(INT_OBJS) ../../sns_intercept/sns_intercept.o
	@echo "Linking all object modules ..."
	$(GCC) -o $@ $(LD_FLAGS) $(INT_OBJS) ../../sns_intercept/sns_intercept.o
	@echo ""
	@echo "Done."

gpsone_glue_msg_test: $(INT_OBJS) gpsone_glue_msg_test.o
	$(GCC) -o gpsone_glue_msg_test $(LD_FLAGS) gpsone_glue_msg_test.o $(INT_OBJS)
	@echo ""
	@echo "Done."

gsiff_sensor_provider_sensor1_test: $(INT_OBJS) gsiff_sensor_provider_sensor1_test.o
	$(GCC) -o gsiff_sensor_provider_sensor1_test $(LD_FLAGS) gsiff_sensor_provider_sensor1_test.o $(INT_OBJS)
	@echo ""
	@echo "Done."

gsiff_sensor_provider_android_test: $(INT_OBJS) gsiff_sensor_provider_android_test.o
	$(GCC) -o gsiff_sensor_provider_android_test $(LD_FLAGS) gsiff_sensor_provider_android_test.o $(INT_OBJS)
	@echo ""
	@echo "Done."

gsiff_sensor_provider_common_test: $(INT_OBJS) gsiff_sensor_provider_common_test.o
	$(GCC) -o gsiff_sensor_provider_common_test $(LD_FLAGS) gsiff_sensor_provider_common_test.o $(INT_OBJS)
	@echo ""
	@echo "Done."

msg_glue_test: $(EXT_COMMON_OBJS) msg_glue_test.o
	$(GCC) -o msg_glue_test $(LD_FLAGS) msg_glue_test.o $(EXT_COMMON_OBJS)
	@echo ""
	@echo "Done."

list_glue_test: $(EXT_COMMON_OBJS) list_glue_test.o
	$(GCC) -o list_glue_test $(LD_FLAGS) list_glue_test.o $(EXT_COMMON_OBJS)
	@echo ""
	@echo "Done."

os_kf_test: $(INT_OBJS) os_kf_test.o
	$(GCC) -o os_kf_test $(LD_FLAGS) os_kf_test.o $(INT_OBJS)
	@echo ""
	@echo "Done."

.PHONY: clean

clean:
	@echo "Cleaning up project directory ..."
	rm -f $(INT_OBJS) *.o ../*.o a.out *.ln core gsiff_daemon list_glue_test msg_glue_test gsiff_sensor_provider_sensor1_test gsiff_sensor_provider_common_test gsiff_sensor_provider_android_test
	@echo ""
	@echo "Clean."

new:
	make clean
	make

