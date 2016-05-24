#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dss_test.h"

const char Test_id[]   = "INVALID";
const char Test_name[] = "INVALID";
const char Test_desc[] = "INVALID";

int 
main (int argc, char * argv[])
{
    dss_test_init(argc, argv);
    dss_test_master();

    return 0;
}
