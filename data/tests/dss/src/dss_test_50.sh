#!/bin/ash

# internal variables used 

REAL=0
PRETEND=1

# do all the initial setup stuff

runmode=$REAL

# parse arguments

while [ $# -gt 0 ]
do
	if [ $1 = "-n" ] || [ $1 = "--pretend" ]
	then
		runmode=$PRETEND
	elif [ $1 = "-h" ] || [ $1 = "--help" ]
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
	elif [ $1 = "-pp" ] || [ $1 = "--peer-port" ]
	then
        pport=$2
        shift
	elif [ $1 = "-l" ] || [ $1 = "--logfile" ]
	then
        logfile=$2
        shift
	elif [ $1 = "-f" ] || [ $1 = "--datafile" ]
	then
        datafile=$2
        shift
	elif [ $1 = "-fo" ] || [ $1 = "--datafileout" ]
	then
        datafileout=$2
        shift
	elif [ $1 = "-r" ] || [ $1 = "--tech" ]
	then
        tech=$2
        shift
	fi
	shift
done

# print help if needed

if [ $printhelp ]
then 
	echo "$0 : prepare and run dss_test_50"
	echo 'following arguments are supported:'
	echo "-n, --pretend : only pretend, don't really do anything"
	echo '-h, --help : print this help'
	echo '-l, --logfile <file> : redirect test output to this file'
	echo '-f, --datafile <file> : data file to send'
	echo '-r, --tech <tech> : technology (umts/cdma) to test on'
	exit 0
fi

if [ ! $datafile ] 
then 
    echo 'no data file specified. exiting..'
    exit 0
elif [ ! -f $datafile ]
then 
    echo "data file $datafile does not exist. exiting.."
    exit 0
fi

# build command to send to run dss_test_peer_in tcp server mode

command="run dss_test_peer -t -i $mip -p $pport"
cleanup="terminate"

if [ $datafileout ]
then
    command="$command -f $datafileout"
fi

# send command to master to run dss_test_peer in tcp server mode

dss_test_master_client -i $mip -p $mport -C "$command"

sleep 1

# ok, now run the test 

command="dss_test_50 -i $mip -p $pport -f $datafile -w 5000"

if [ $logfile ] 
then
    command="$command -l $logfile"
fi

if [ $tech ]
then
    command="$command --tech $tech"
fi

$command

# clean up 

dss_test_master_client -i $mip -p $mport -C $cleanup
