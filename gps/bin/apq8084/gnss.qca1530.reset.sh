#!/system/bin/sh
#
# Copyright (C) 2013 Qualcomm Atheros.
# All Rights Reserved
#
log_tag="gps-reset"

gpio1_path=/sys/class/gpio/gpio133
gpio2_path=/sys/class/gpio/gpio128

function log_error
{
    log -p e -t $log_tag $*
}

function log_debug
{
    log -p d -t $log_tag $*
}

if [ -w $gpio1_path/value ]
then
    log_debug "QCA1530 reset control is detected on GPIO133"
    path=$gpio1_path
elif [ -w $gpio2_path/value ]
then
    log_debug "QCA1530 reset control is detected on GPIO128"
    path=$gpio2_path
else
    log_error "QCA1530 reset control GPIO is not detected"
    exit 1
fi

case $1 in
    0)
        log_debug "Disabling QCA1530"
        echo 0 > ${path}/value
        ;;
    1)
        log_debug "Enabling QCA1530"
        echo 1 > ${path}/value
        ;;
    *)
        log_error "Bad argument"
        exit 1
        ;;
esac

exit 0

