#!/bin/ash

. $TEST_ENV_SETUP

# list of tests to skip
# dss_test_104 and dss_test_40 removed until Google SMD patch in incorporated
SKIP=" \
		dss_test_2 \
		dss_test_50 \
	"

# utility functions 

loud_echo() {
	argstr=

	for j in $* 
	do 
		argstr="$argstr $j"
	done 

	echo $argstr

	if [ $file ]
	then 
		eval echo $argstr >> $file
	fi
}

backup_file() {
	if test -f $1
	then 
		tmpfile="$1.bak"
		loud_echo "backing up $1 to $tmpfile"
		cp $1 $tmpfile
		rm $1
	fi
}

skip() {
    if [ $noskip ]
    then 
        return 0
    fi 

	for k in $SKIP
	do
		if [ $k = $1 ] 
		then
            if [ $invert ] 
            then
                return 0
            fi

			return 1
		fi
	done

    if [ $invert ]
    then
        return 1
    fi

	return 0
}

wait() {
    waitt=$1

    if [ $waitt -gt 0 ]
    then
        echo -n "Waiting"
    else
        return 0
    fi

    while [ $waitt -gt 0 ]
    do
        echo -n '.'
	    if [ $runmode -eq $REAL ]
        then
            sleep 1
        fi
        waitt=`expr $waitt - 1`
    done

    echo
    return 0
}

# internal variables used 

REAL=0
PRETEND=1

# do all the initial setup stuff

runmode=$REAL
printhelp=0
waitsec=0

# run through all arguments and see what the user wants

while [ $# -gt 0 ]
do
	if [ $1 = "-n" ] || [ $1 = "--pretend" ]
	then
		runmode=$PRETEND
	elif [ $1 = "-h" ] || [ $1 = "--help" ]
	then
		printhelp=1
	elif [ $1 = "-ns" ] || [ $1 = "--no-skip" ]
	then 
		noskip=1
	elif [ $1 = "-pg" ] || [ $1 = "--page" ]
	then 
		page=1
	elif [ $1 = "-w" ] || [ $1 = "--wait" ]
	then 
		waitsec=$2
		shift
	elif [ $1 = "-l" ] || [ $1 = "--logfile" ]
	then 
		logfile=$2
		shift
	elif [ $1 = "-f" ] || [ $1 = "--file" ]
	then 
		file=$2
		shift
	elif [ $1 = "-p" ] || [ $1 = "--pass" ]
	then 
		passarg_exe=$2
		shift
	elif [ $1 = "-ps" ] || [ $1 = "--pass-script" ]
	then 
		passarg_scr=$2
		shift
	elif [ $1 = "-t" ] || [ $1 = "--tech" ]
	then 
		tech=$2
		shift
	elif [ $1 = "-i" ] || [ $1 = "--invert" ]
	then 
		invert=1
	fi
	shift
done

# print help if needed

if [ $printhelp -eq 1 ]
then 
	echo "$0 : run dss smoke test"
	echo 'following arguments are supported:'
	echo "-n, --pretend : only pretend, don't really do anything"
	echo '-h, --help : print this help'
	echo "-ns, --no-skip : don't skip any tests"
	echo '-pg, --page : page me when done'
	echo '-w, --wait <time> : time to wait in secs between tests'
	echo '-l, --logfile <file> : redirect test output to this file'
	echo '-f, --file <file> : also log to file'
    echo '-p, --pass <arg string> : pass arguments to tests'
    echo '-ps, --pass-script <arg string> : pass arguments to test scripts'
    echo '-t, --tech <technology> : technology under test (umts/cdma)'
    echo '-i, --invert : use skip list in the inverted sense'
	exit 0
fi

# construct the test command trailer and back up log files

if [ $logfile ]
then 
	trailer="$trailer -l $logfile"
	backup_file $logfile
fi

if [ $tech ]
then
    trailer="$trailer --tech $tech"
fi

trailer_exe="$trailer $passarg_exe"
trailer_scr="$trailer $passarg_scr" 

if [ $file ]
then
	backup_file $file
fi

# ok, run all the tests

loud_echo "running smoke test suite.."

if [ $ANDROID_ROOT ]
then 
	chmod 0777 * 
	echo "Disabling Android Always-ON Data Call" 
	echo -n '1' >/data/dun
	echo "sleeping" 
	sleep 5
fi

for i in dss_test_*[0-9]
do
	skip $i

	if [ $? -eq 0 ]
	then
        SKIP_CURR=0
	else 
        SKIP_CURR=1
	fi

    if [ -x $i.sh ] 
    then 
        i=$i.sh
        trailer=$trailer_scr
    else
        trailer=$trailer_exe
    fi

    if [ $SKIP_CURR -eq 0 ] 
    then 
		loud_echo -n "running test $i.."
    else
		loud_echo "skipping test $i.."
		continue
    fi

	if [ $runmode -eq $REAL ]
	then
		# i=dss_try.sh
		i="$i $trailer"
		./$i
		status=$?
	else
		status=0
	fi

	if [ $status -eq 0 ]
	then
		loud_echo "passed."
	else 
		if [ $status -eq 2 ]
		then 
			loud_echo "failed! Aborting.."
			break
        elif [ $status -eq 3 ]
        then
            loud_echo "skipped."
		else
			loud_echo "failed! Continuing.."
		fi
	fi

    wait $waitsec
done

# done with running tests
# generate report and send it 

# page user 

wrap_up=dss_test_done.sh

if [ $page ]
then
    if [ -x dss_test_done.sh ] 
    then 
        loud_echo "running $wrap_up"

        if [ $runmode -eq $REAL ]
        then
            $wrap_up $trailer_scr
        fi
    else 
        loud_echo "can't find $wrap_up"
    fi
fi

if [ $ANDROID_ROOT ]
then 
	echo "Enabling Android Always-ON Data Call" 
	echo -n '2' >/data/dun
fi
