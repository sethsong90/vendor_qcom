#!/bin/ash

NETCAPP="dss_test_netc"
SIGUSR1="USR1"
SIGUSR2="USR2"
SIGTERM="TERM"

# run through all arguments and see what the user wants

while [ $# -gt 0 ]
do
	if [ $1 = "-c" ] || [ $1 = "--command" ]
	then
		command=$2
        shift
	elif [ $1 = "-h" ] || [ $1 = "--help" ]
	then
		printhelp=1
	elif [ $1 = "-n" ] || [ $1 = "--pretend" ]
	then
		pretend=1
    elif [ $1 = "-o" ] || [ $1 = "--options" ]
    then
        prgoptions=$2
        shift
	fi
	shift
done

# print help if needed

if [ $printhelp ]
then 
	echo "$0 : issue command to dss test netc app"
	echo "Usage: $0 -c <command> -o \"<options>\""
	echo "Following commands are supported:"
	echo "run: start netc app"
	echo "netup: bring up network"
	echo "netdown: bring down network"
    echo "exit: shut down netc app"
	exit 0
fi

if [ ! $command ]
then
    echo "no command specified!"
    exit 0
fi

npid=`ps | grep "$NETCAPP" | grep -v "grep" | sed 's/^[ ]*//' | sed 's/[ ].*$//'`

if [ $pretend ]
then 
    EXEC=echo
    if [ ! $npid ]
    then
        npid="<pid>"
    fi
fi

if [ $command != "run" ]
then
    if [ ! $npid ]
    then
        echo "$NETCAPP not running"
        exit 0
    fi
else
    if [ $npid ]
    then
        echo "$NETCAPP already running"
        exit 0
    fi
fi

case $command in 
    "run")
        $EXEC ./$NETCAPP $prgoptions &
        ;;
    "netup")
        $EXEC kill -$SIGUSR1 $npid
        ;;
    "netdown")
        $EXEC kill -$SIGUSR2 $npid
        ;;
    "exit")
        $EXEC kill -$SIGTERM $npid
        ;;
    *)
        echo "ignoring unknown command $command"
        ;;
esac

exit 0
