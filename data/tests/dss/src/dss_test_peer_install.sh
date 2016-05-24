#!/bin/bash 

bld_root=./../../../../..

dsslib_dir=${bld_root}/common/libs/dss/
dsc_dir=${bld_root}/common/apps/dsc/
stringl_dir=${bld_root}/common/libs/stringl/
dsstest_dir=./

while [ $# -gt 0 ]
do
	if [ $1 = "-n" ] || [ $1 = "--pretend" ]
	then
		pretend=1
	elif [ $1 = "-h" ] || [ $1 = "--help" ]
	then
		printhelp=1
	elif [ $1 = "-i" ] || [ $1 = "--install-dir" ]
	then 
        target_root=$2
        shift
	fi
	shift
done

if [ $printhelp ]
then
	echo "$0 : install dss test peer program package"
	echo 'following arguments are supported:'
	echo "-n, --pretend : only pretend, don't really do anything"
	echo '-h, --help : print this help'
	echo '-i, --install-dir <dir> : installation directory'
    exit 0
fi

if [ ! $target_root ]
then
    echo 'no target root dir specified. exiting..'
    exit 0
fi

if [ $pretend ]
then 
    ECHO=echo
fi

if [ ! -d $target_root ]
then
    $ECHO mkdir -v $target_root
fi

target_dsc=${target_root}/dsc/
target_dss=${target_root}/dss/
target_stringl=${target_root}/stringl/

if [ ! -d $target_dsc ]
then
    $ECHO mkdir -v $target_dsc
fi

$ECHO cp -v ${dsc_dir}/inc/dsc_dcm.h ${target_dsc}/.
$ECHO cp -v ${dsc_dir}/inc/dsci.h ${target_dsc}/.

$ECHO cp -v -r $dsslib_dir $target_dss
$ECHO cp -v -r $stringl_dir $target_stringl

$ECHO cp -v ${dsstest_dir}/dss_test.c ${target_root}/.
$ECHO cp -v ${dsstest_dir}/dss_test.h ${target_root}/.
$ECHO cp -v ${dsstest_dir}/dss_test_master.c ${target_root}/.
$ECHO cp -v ${dsstest_dir}/dss_test_peer.c ${target_root}/.
$ECHO cp -v ${dsstest_dir}/dss_test_peer_Makefile ${target_root}/Makefile
$ECHO cp -v ${dsstest_dir}/dss_test_dispatch.sh ${target_root}/.
