#!/bin/bash

NULL=0
RUN=1
TERMINATE=2
PAGE=3
PIDFILE=dss_test.pid

while [ $# -gt 0 ]
do
	if [ $1 = "-c" ] || [ $1 = "--command" ]
	then
        COMMAND=1
        command=$2
        shift
	fi
	shift
done

if ! [ $COMMAND ] 
then
    echo 'no command specified. exiting..'
    exit 0
else 
    echo "executing command $command"
fi

TYPE=$NULL

for i in $command
do
    case $i in 
    "run")
        TYPE=$RUN
        ;;
    "terminate")
        TYPE=$TERMINATE
        ;;
    "page")
        TYPE=$PAGE
        ;;
    *)
        realcmd="$realcmd $i"
        ;;
    esac
done

if [ $TYPE -eq $RUN ]
then 
    $realcmd &
    echo "$!" > $PIDFILE
elif [ $TYPE -eq $TERMINATE ]
then
    PID=`cat $PIDFILE`
    kill -s SIGTERM $PID
elif [ $TYPE -eq $PAGE ]
then
    echo "don't know how to page yet"
fi
