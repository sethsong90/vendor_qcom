/******************************************************************************
  @file:  ndk_stubs.c

  DESCRIPTION
    Test framework implementation for ndk to enable off-target testing.

  INITIALIZATION AND SEQUENCING REQUIREMENTS

  -----------------------------------------------------------------------------
  Copyright (c) 2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

******************************************************************************/

/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
05/01/11   jb       Initial version
08/01/11   jb       1. Misc typedef changes

======================================================================*/

#include "gsiff_sensor_provider_and_ndk.h"
#include "gsiff_sensor_provider_common.h"
#include <log_util.h>

#include "gpsone_glue_msg.h"
#include "gsiff_msg.h"
#include "gpsone_thread_helper.h"

#include <android/sensor.h>

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>


ALooper* ALooper_prepare(int opts)
{
   return NULL;
}

int ALooper_pollOnce(int timeoutMillis, int* outFd, int* outEvents, void** outData)
{
   return 0;
}


void ALooper_wake(ALooper* looper)
{
}

ASensorManager* ASensorManager_getInstance()
{
   return NULL;
}

/*
 * Returns the list of available sensors.
 */
int ASensorManager_getSensorList(ASensorManager* manager, ASensorList* list)
{
   return 0;
}

/*
 * Returns the default sensor for the given type, or NULL if no sensor
 * of that type exist.
 */
ASensor const* ASensorManager_getDefaultSensor(ASensorManager* manager, int type)
{
   return NULL;
}

/*
 * Creates a new sensor event queue and associate it with a looper.
 */
ASensorEventQueue* ASensorManager_createEventQueue(ASensorManager* manager,
        ALooper* looper, int ident, ALooper_callbackFunc callback, void* data)
{
   return NULL;
}

/*
 * Destroys the event queue and free all resources associated to it.
 */
int ASensorManager_destroyEventQueue(ASensorManager* manager, ASensorEventQueue* queue)
{
   return 0;
}


/*****************************************************************************/

/*
 * Enable the selected sensor. Returns a negative error code on failure.
 */
int ASensorEventQueue_enableSensor(ASensorEventQueue* queue, ASensor const* sensor)
{
   return 0;
}

/*
 * Disable the selected sensor. Returns a negative error code on failure.
 */
int ASensorEventQueue_disableSensor(ASensorEventQueue* queue, ASensor const* sensor)
{
   return 0;
}

/*
 * Sets the delivery rate of events in microseconds for the given sensor.
 * Note that this is a hint only, generally event will arrive at a higher
 * rate. It is an error to set a rate inferior to the value returned by
 * ASensor_getMinDelay().
 * Returns a negative error code on failure.
 */
int ASensorEventQueue_setEventRate(ASensorEventQueue* queue, ASensor const* sensor, int32_t usec)
{
   return 0;
}

ssize_t ASensorEventQueue_getEvents(ASensorEventQueue* queue,
                ASensorEvent* events, size_t count)
{
   return 0;
}

/*
 * Returns this sensor's name (non localized)
 */
const char* ASensor_getName(ASensor const* sensor)
{
   return "Google Super Sensor";
}

/*
 * Returns this sensor's vendor's name (non localized)
 */
const char* ASensor_getVendor(ASensor const* sensor)
{
   return "Google Super Vendor";
}

/*
 * Return this sensor's type
 */
int ASensor_getType(ASensor const* sensor)
{
   return 0xFF;
}

/*
 * Returns this sensors's resolution
 */
float ASensor_getResolution(ASensor const* sensor)
{
   return 2.5;
}

/*
 * Returns the minimum delay allowed between events in microseconds.
 * A value of zero means that this sensor doesn't report events at a
 * constant rate, but rather only when a new data is available.
 */
int ASensor_getMinDelay(ASensor const* sensor)
{
   return -1;
}
