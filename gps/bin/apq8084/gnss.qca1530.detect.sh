#!/system/bin/sh

##
#   Copyright (c) 2013 Qualcomm Atheros, Inc.
#   All Rights Reserved.
#   Qualcomm Atheros Confidential and Proprietary.
##

LOG_TAG=gnss.qca1530.detect
source /system/vendor/bin/gnss.qca1530.setenv.sh

exec /system/vendor/bin/gnss.qca1530.svcd -fsh $WORKING_DIR/gnss-fsh.bin -detect-only


