/*
 * Copyright (c) 2012, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 * Developed by QRD Engineering team.
 */
#include "utils.h"
#include "debug.h"
#include "agent.h"
#if !DEBUG_ON
static const struct option OPTIONS[] = {
    { "setBuildVersion", no_argument, NULL, 's' },
    { "getBuildVersion", required_argument, NULL, 'g' },
    { "command", required_argument, NULL, 'c' },
    { NULL, 0, NULL, 0 },
};
#endif

int main(int argc, char **argv)
{   
    #if !DEBUG_ON
    int arg;
    while ((arg=getopt_long(argc, argv,"sc:" , OPTIONS, NULL))!=-1)
    {  
        logd(char(arg));
        switch (arg) {
            case 's':
                setBuildVersion(); break;
            case 'c':
                runCommand(optarg); break;
            case '?':
                logd("invalid argument\n");
                continue;
            default:
                logd("nothing to do\n");
                continue;
        }
    }
    #else
    setBuildVersion();
    #endif
    return 0;
}