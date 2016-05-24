#!/system/bin/sh

#
# Copyright (c) 2013 QUALCOMM Atheros, Inc.
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary
#

#make a dir for service NV and EFS to live
export WORKING_DIR=/data/misc/location/qca1530

if [ -$LOG_TAG = - ]
then
  LOG_TAG=gnss.qca1530
fi

function log_error
{
  log -p e -t $LOG_TAG $*
}

function log_debug
{
  log -p d -t $LOG_TAG $*
}

function die
{
  log_error $*
  exit 1
}

if [ -d $WORKING_DIR ]
then
  log_debug "QCA1530: NV directory exists"
else
  log_debug "QCA1530: Creating NV directory"
  mkdir -p $WORKING_DIR || die "QCA1530: Failed to create NV directory"
  chown gps:gps $WORKING_DIR
  chmod 750 $WORKING_DIR
fi

if [ -d $WORKING_DIR/efs ]
then
  log_debug "QCA1530: EFS directory exists"
else
  log_debug "QCA1530: Creating EFS directory"
  mkdir -p $WORKING_DIR/efs || die "QCA1530: Failed to create EFS directory"
  chown gps:gps $WORKING_DIR/efs
  chmod 750 $WORKING_DIR/efs
fi

#make available the NV file for service to start
if [ -f $WORKING_DIR/gnss-fsh.bin ]
then
  log_debug "QCA1530: FSH File exists"
else
  log_debug "QCA1530: FSH File doesn't exist. Copying file."
  cat /system/vendor/etc/gnss/gnss-fsh.bin > $WORKING_DIR/gnss-fsh.bin \
    || die "QCA1530: Failed to install FSH file"
  chown gps:gps $WORKING_DIR/gnss-fsh.bin
  chmod 600 $WORKING_DIR/gnss-fsh.bin
fi
