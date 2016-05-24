/*
 * Copyright(c) 2013 Qualcomm Technologies, Inc.All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

/*
 * NOTE: This file is for demonstration purposes only.
 * Please change it according to the device specific configuration.
 * */

#include <module.h>
#include <stdint.h>

/* The following are for demonstration only. Please change this file according
 * to the device specific configuration */
extern struct SideChannelModule cameraModule;
extern struct SideChannelModule audioModule;
extern struct SideChannelModule sensorsModule;

/* The main library expect this array to be initialized statically here */
struct SideChannelModule modules[] = {
  cameraModule,
  audioModule,
  sensorsModule
};
/* Number of modules in the array. Evaluated at library load-time */
size_t modulesLen;

/* Modules constructor */
__attribute__((constructor)) static void modulesInit()
{
  modulesLen = sizeof(modules) / sizeof(struct SideChannelModule);
}
