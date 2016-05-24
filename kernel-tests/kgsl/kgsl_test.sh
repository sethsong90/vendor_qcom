#-----------------------------------------------------------------------------
# Copyright (c) 2011 Qualcomm Technologies, Inc.
# All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
#-----------------------------------------------------------------------------

. $test_env_setup.sh

set -e
if test ! -c /dev/kgsl-3d0;
then
	echo "/dev/kgsl-3d0 missing"
	exit 1
fi

do
	./kgsl_test $@
done
