#!/bin/ash

# ---------------------------------------------------------------------------
# Copyright (c) 2007 Qualcomm Technologies, Inc.
# All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
# ---------------------------------------------------------------------------

DSC_DAEMON=qcomdscd
DSC_DCHP_CONFIG_SH=udhcpc.sh
DSC_KERNEL_CONFIG_SH=qcomdsc-kif.sh

modprobe smd_cntl

if test -d /var/log
then
	echo 'syslogd already running'
else
	mkdir /var/log
	echo 'starting syslogd..'
	/sbin/syslogd
fi

cdir=`pwd`

if ! [ -x ${DSC_DAEMON} ]
then 
    echo 'executable file ${DSC_DAEMON} does not exist in curr directory! exiting..'
    exit 1
fi

if ! [ -f ${DSC_DHCP_CONFIG_SH} ]
then 
    echo 'file ${DSC_DHCP_CONFIG_SH} does not exist in curr directory! exiting..'
    exit 1
fi

if ! [ -f ${DSC_KERNEL_CONFIG_SH} ]
then 
    echo 'file ${DSC_KERNEL_CONFIG_SH} does not exist in curr directory! exiting..'
    exit 1
fi

# run through all the arguments 

while [ $# -gt 0 ]
do
    if [ $1 = "-t" ] || [ $1 = "--tech" ]
    then
        tech=$2
        shift
    elif [ $1 = "-h" ] || [ $1 = "--help" ]
    then
        printhelp=1
    fi
    shift
done

if [ $printhelp ]
then
    echo "$0 -t [tech] : run dsc program for tech"
    echo "tech: one of \"cdma\" or \"umts\" "
    exit 0
fi

if [ ! $tech ]
then
    echo 'tech not specified! exiting..'
    exit 1
fi

${cdir}/${DSC_DAEMON} -s -l 0 -n 3 -i qdid -u ${cdir}/${DSC_DHCP_CONFIG_SH} -m ${cdir}/${DSC_KERNEL_CONFIG_SH} -t $tech
