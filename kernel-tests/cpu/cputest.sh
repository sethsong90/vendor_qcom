#-----------------------------------------------------------------------------
# Copyright (c) 2011,2013 Qualcomm Technologies, Inc.  All Rights Reserved.
# Qualcomm Technologies Proprietary and Confidential.
#-----------------------------------------------------------------------------

. $TEST_ENV_SETUP #TARGET_TYPE filled in

echo "CPU test starting"
num_cpus=0

#---------------------------------------------------
# $1 - the actual cpu number from proc/cpuinfo
# $2 - the cmdline cpu number
#---------------------------------------------------
compare_cpus_num() {
	if [ "$2" != "0" ]
	then
		num_cpus=$2
	else
	case $TARGET_TYPE in
	8660 | 8960)
		num_cpus=2
		;;
	8064 | 8974 | 8226 | 8x26 | 8926 | 8026 | 8962)
		num_cpus=4
		;;
	8x10 | 8610)
		if [ "$1" == "2" ] || [ "$1" == "4" ]
		then
			num_cpus=$1
		else
			num_cpus=2
		fi
		;;
	*)
		echo "Not able to detect target type"
		num_cpus=2
		;;
	esac
	fi
	if [ "$1" != "$num_cpus" ]
	then
		echo "Unexpected number of CPUs, expected = $num_cpus."
		echo "Test failed."
		exit 1
	fi
}

while [ $# -gt 0 ]; do
	case $1 in
	-c | --cpus)
		num_cpus=$2 ; shift 2
		;;
	-h | --help)
		echo "Usage: $0 [-c <number_of_cpus>]" ;
		exit 1
		;;
	esac
done

echo "Checking number of CPUs"

C=`cat /proc/cpuinfo |grep processor |wc -l`

echo "Detected $C processors."

compare_cpus_num $C $num_cpus

echo "Checking for ARMv7"

cat /proc/cpuinfo | grep -q ARMv7
if [ $? -ne 0 ]
then
	echo "ARMv7 not detected."
	echo "Test failed."
	exit 1
fi

echo "Test passed."

