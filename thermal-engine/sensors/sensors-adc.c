/*===========================================================================

  sensor-adc.c

  DESCRIPTION
  ADC sensor access functions.

  INITIALIZATION AND SEQUENCING REQUIREMENTS
  setup() function should be called before get_temperature().
  shutdown() function should be called to clean up resources.

  Copyright (c) 2012 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

===========================================================================*/

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <poll.h>

#include "sensors-adc.h"
#include "sensors_manager_internal.h"
#include "thermal.h"

#define MSM_ADC_NODE  "/sys/devices/platform/msm_ssbi.0/pm8921-core/pm8xxx-adc/%s"
#define LVL_BUF_MAX (12)

int adc_sensors_setup(struct sensor_info *sensor)
{
	int fd = -1;
	int sensor_count = 0;
	char name[MAX_PATH] = {0};

	snprintf(name, MAX_PATH, MSM_ADC_NODE, sensor->name);
	fd = open(name, O_RDONLY);
	if (fd < 0) {
		msg("%s: Error opening %s\n", __func__, name);
		return sensor_count;
	}

	sensor_count++;
	sensor->fd = fd;

	return sensor_count;
}

void adc_sensors_shutdown(struct sensor_info *sensor)
{
	if (NULL == sensor) {
		msg("%s: unexpected NULL", __func__);
		return;
	}
	if (sensor->fd > 0)
		close(sensor->fd);
}

int adc_sensor_get_temperature(struct sensor_info *sensor)
{
	char buf[3*LVL_BUF_MAX] = {0};
	int temp = 0;

	if (NULL == sensor) {
		msg("%s: unexpected NULL", __func__);
		return 0;
	}

	if (read(sensor->fd, buf, sizeof(buf) - 1) != -1) {
		sscanf(buf, "Result:%d Raw:%*d\n", &temp);
	}

	lseek(sensor->fd, 0, SEEK_SET);
	return CONV(temp);
}
