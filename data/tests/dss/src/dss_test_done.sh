#!/bin/ash

# parse arguments

while [ $# -gt 0 ]
do
	if [ $1 = "-h" ] || [ $1 = "--help" ]
	then
        printhelp=1
	elif [ $1 = "-mi" ] || [ $1 = "--master-ip" ]
	then
        mip=$2
        shift
	elif [ $1 = "-mp" ] || [ $1 = "--master-port" ]
	then
        mport=$2
        shift
	fi
	shift
done

# print help if needed

if [ $printhelp ]
then 
	echo "$0 : notify master that we are done"
	echo 'following arguments are supported:'
	echo '-h, --help : print this help'
	echo "-mi, --master-ip <ip address> : master's ip addr"
	echo "-mp, --master-port <port> : master's port"
	exit 0
fi

# send command to master to page user

dss_test_master_client -i $mip -p $mport -C page
