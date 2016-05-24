#-----------------------------------------------------------------------------
# Copyright (c) 2008-09 Qualcomm Technologies, Inc.
# All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
#-----------------------------------------------------------------------------

#  Unit test for the bma150 / SPI driver.
#

. $TEST_ENV_SETUP

# Search the proc filesystem for the location of the device associated with the
# bma150 SPI-based accelerometer
export dev_location="/proc/bus/input/devices"
export dev=`cat $dev_location | \
    awk -F= '/N: Name="bma150"/,/H: Handlers=/ { print $2 }' | tail -n 1 | \
    sed 's/[[:space:]]*//g'`

if [ ""$dev == "" ]
then
    echo "device bma150 not found in "$dev_location
    exit 1
fi
# Use /dev/input/eventX if present, else try /dev/eventX. spitest handles the
# case where /dev/eventX is not present.
if [[ -c /dev/input/$dev ]]
then
    export dev=input/$dev
fi
./spitest $dev $1
