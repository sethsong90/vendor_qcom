/*
 * ---------------------------------------------------------------------------
 *  Copyright (c) 2009-2012 Qualcomm Technologies, Inc.  All Rights Reserved.
 *  Qualcomm Technologies Proprietary and Confidential.
 *  --------------------------------------------------------------------------
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/rtc.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <oncrpc.h>
#include <linux/android_alarm.h>
/* For AID_SYSTEM */
#include <private/android_filesystem_config.h>

#include "time_remote_atom.h"
#include "time_remote_atom_rpc.h"
#include "time_genoff_i.h"

static time_genoff_struct_type ats_bases[ATS_MAX];
static pthread_mutex_t genoff_mutex;
static uint8_t time_genoff_set_id;
static uint8_t modem_rpc_initialized = 0;

/* This array lists the bases which have to be sent to MODEM */
static int genoff_update_to_modem[] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0,
					0};
/* This array lists the bases which can receive update from MODEM */
static int genoff_update_from_modem[] = {0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0,
					 0};
/* This array lists the bases, on whose update  the TOD base changes*/
static int genoff_update_tod[] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
/* This array lists the bases on which updates are allowed */
static int genoff_valid_base[] = {0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1};
/* This array lists the bases, which can be updated from APPS */
static int genoff_update_from_apps[] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1,
					0};

static int time_persistent_memory_opr (const char *file_name,
	time_persistant_opr_type rd_wr, int64_t *data_ptr)
{
	char fname[120];
	int fd;

	/* location where offset is to be stored */
	snprintf(fname, 120, "%s/%s", OFFSET_LOCATION, file_name);
	TIME_LOGD("Daemon:Opening File: %s\n", fname);

	switch(rd_wr){
		case TIME_READ_MEMORY:
			TIME_LOGD("Daemon:%s:Genoff Read operation \n", __func__);
			if ((fd = open(fname,O_RDONLY)) < 0) {
				TIME_LOGD("Daemon:Unable to open file for read\n");
				goto fail_operation;
			}
			if (read(fd, (int64_t *)data_ptr, sizeof(int64_t)) < 0) {
				TIME_LOGD("Daemon:%s:Error reading from file\n", __func__);
				close(fd);
				goto fail_operation;
			}
			break;

		case TIME_WRITE_MEMORY:
			TIME_LOGD("Daemon:%s:Genoff write operation \n", __func__);
			if ((fd = open(fname, O_RDWR)) < 0) {
				TIME_LOGD("Daemon:Unable to open file, creating file\n");
				if ((fd = open(fname, O_CREAT | O_RDWR, 0644)) < 0) {
					TIME_LOGD("Daemon:Unable to create file, exiting\n");
					goto fail_operation;
				}
			}
			if (write(fd, (int64_t *)data_ptr, sizeof(int64_t)) < 0) {
				TIME_LOGE("Daemon:%s:Error reading from file\n", __func__);
				close(fd);
				goto fail_operation;
			}
			break;
		default:
			return -EINVAL;
	}
	close(fd);
	return 0;

fail_operation:
	return -EINVAL;
}

static void
genoff_pre_init(time_genoff_ptr time_genoff,time_bases_type base)
{
        time_genoff->initialized = 0;
	time_genoff->reqd_base_genoff = 0;
	time_genoff->generic_offset = base;
	time_genoff->bases_type =  base;
	time_genoff->init_func = NULL;
	time_genoff->per_storage_spec.initialized = 0;
	TIME_LOGD("Daemon:%s::Base = %d\n", __func__, base);
}

static int genoff_post_init(time_genoff_ptr time_genoff)
{
	int rc;

	if (time_genoff->init_func != NULL) {
		rc = time_genoff->init_func();
		if (rc) {
			TIME_LOGE("Daemon:%s:Init func failed\n", __func__);
			return -EINVAL;
		}
	}

	if (time_genoff->per_storage_spec.initialized == 1) {
		/* Read from the generic offset */
		rc = time_persistent_memory_opr(
			time_genoff->per_storage_spec.f_name,
			TIME_READ_MEMORY, &(time_genoff->generic_offset));
		if (rc) {
			TIME_LOGD("Daemon:%s:Error in accessing storage\n",
								__func__);
			time_genoff->generic_offset = 0;
		}
	}

	time_genoff->initialized = 1;

	return 0;
}

static void genoff_set_generic_offset(time_genoff_ptr time_genoff,
						int64_t offset)
{
	int64_t delta_ms;

	delta_ms = offset - time_genoff->generic_offset;
	time_genoff->generic_offset = offset;
}

static time_genoff_ptr genoff_get_pointer(time_bases_type base)
{
	if (base >= ATS_MAX)
		return NULL;

	return &(ats_bases[base]) ;
}

static void
genoff_updates_per_storage(time_genoff_ptr time_genoff, char *name,
							int64_t thold)
{
	time_genoff->per_storage_spec.initialized = 1;
	time_genoff->per_storage_spec.threshold   = thold;
	strlcpy(time_genoff->per_storage_spec.f_name, name, FILE_NAME_MAX);

	TIME_LOGD("Daemon: Storage Name: %s \n", time_genoff->per_storage_spec.f_name);
} /* time_genoff_updates_per_storage */


void genoff_persistent_update(time_genoff_ptr ptime_genoff, int64_t delta_ms)
{
	int rc;

	if (ptime_genoff->per_storage_spec.initialized == 1) {
		TIME_LOGD("Daemon:%s: Writing genoff = %llu to memory\n", __func__,
					ptime_genoff->generic_offset);
		rc = time_persistent_memory_opr(
			ptime_genoff->per_storage_spec.f_name,
			TIME_WRITE_MEMORY, &ptime_genoff->generic_offset);
			if (rc) {
				TIME_LOGD("Daemon:%s:or in accessing storage\n",
								__func__);
			}
	}
}

static int rtc_get(int64_t *msecs)
{
	int rc, fd;
	time_t secs = 0;
	struct tm rtc_tm;

	fd = open("/dev/rtc0", O_RDONLY);
	if(fd < 0) {
		TIME_LOGE("Daemon:%s: Unable to open RTC device\n", __func__);
		return -EINVAL;
	}

	rc = ioctl(fd,RTC_RD_TIME,&rtc_tm);
	if(rc < 0) {
		TIME_LOGE("Daemon:%s: Unable to read from RTC device\n",
								__func__);
		goto fail_rtc;
	}

	TIME_LOGD("Daemon:%s: Time read from RTC -- MM/DD/YY HH:MM:SS"
			"%d/%d/%d %d:%d:%d\n", __func__, rtc_tm.tm_mon,
			rtc_tm.tm_mday, rtc_tm.tm_year, rtc_tm.tm_hour,
			rtc_tm.tm_min, rtc_tm.tm_sec);

	/* Convert the time to UTC and then to milliseconds and store   it */
	secs = mktime(&rtc_tm);
	secs += rtc_tm.tm_gmtoff;
	if(secs < 0) {
		TIME_LOGE("Daemon:Invalid RTC seconds = %ld\n", secs);
		goto fail_rtc;
	}

	*msecs = SEC_TO_MSEC(secs);
	TIME_LOGD("Daemon:Value read from RTC seconds = %lld\n", *msecs);

	close(fd);
	return 0;

fail_rtc:
	close(fd);
	return -EINVAL;
}



static int genoff_set(time_genoff_info_type *pargs)
{
	int64_t rtc_msecs;
	int64_t delta_ms;

	time_genoff_info_type temp_genoff_args;
	time_genoff_ptr ptime_genoff, ptime_genoff_tod;

	ptime_genoff = genoff_get_pointer(pargs->base);
	if (ptime_genoff == NULL) {
		TIME_LOGE("Daemon:%s:Genoff memory not allocated\n", __func__);
		return -EINVAL;
	}
	if (ptime_genoff->initialized == 0) {
		TIME_LOGE("Daemon:%s:Genoff not initialized\n", __func__);
		return -EINVAL;
	}

	/* Get RTC time */
	if (rtc_get(&rtc_msecs))
		return -EINVAL;
	
	/* delta_ms = new time - rtc time */
	delta_ms = *(uint64_t *)pargs->ts_val - rtc_msecs;

	TIME_LOGD("Daemon: new time %lld \n",
			*(uint64_t *)pargs->ts_val);

	ptime_genoff->generic_offset = delta_ms;
	TIME_LOGD("Daemon: delta %lld genoff %lld \n", delta_ms,
			ptime_genoff->generic_offset);

	genoff_persistent_update(ptime_genoff, delta_ms);

	if (genoff_update_tod[ptime_genoff->bases_type] &&
			ptime_genoff->bases_type != ATS_TOD) {
		/* Update the TOD offset */
		TIME_LOGD("Updating the TOD offset\n");
		ptime_genoff_tod = genoff_get_pointer(ATS_TOD);
		ptime_genoff_tod->generic_offset = ptime_genoff->generic_offset;
		genoff_persistent_update(ptime_genoff_tod, delta_ms);
	}

	return 0;
}

static int genoff_get(time_genoff_info_type *pargs)
{
	time_genoff_ptr ptime_genoff;
	int64_t rtc_msecs = 0;

	TIME_LOGD("Daemon: genoff get for %d\n", pargs->base);

	ptime_genoff = genoff_get_pointer(pargs->base);
	if (ptime_genoff == NULL) {
		TIME_LOGE("Daemon:%s:Genoff memory not allocated\n", __func__);
		return -EINVAL;
	}

	if (ptime_genoff->initialized == 0) {
		TIME_LOGE("Daemon:%s:Bases not initialized\n", __func__);
		goto fail_time_get;
	}


	if (rtc_get(&rtc_msecs))
		goto fail_time_get;

	TIME_LOGD("Daemon:Value read from RTC seconds = %lld\n", rtc_msecs);
	TIME_LOGD("Daemon:Value read from generif offset = %lld\n",
					ptime_genoff->generic_offset);

	/* Add RTC time to the offset */
	*(uint64_t *)pargs->ts_val = ptime_genoff->generic_offset + rtc_msecs;

	TIME_LOGD("Daemon:Final Time = %llu\n", *(uint64_t *)pargs->ts_val);

	return 0;

fail_time_get:
	*(uint64_t *)pargs->ts_val = 0;
	return -EINVAL;
}

static int genoff_opr(time_genoff_info_type *pargs)
{
	time_genoff_info_type temp_genoff_args;
	uint64_t ts_val_secs;
	int rc = 0;

	TIME_LOGD("Daemon:%s: Base = %d, val = %llu, operation = %d", __func__,
		pargs->base, *(uint64_t *)(pargs->ts_val), pargs->operation);

	if (pargs->operation == T_GET) {
		switch(pargs->unit) {
		case TIME_MSEC:
			rc = genoff_get(pargs);
			break;
		case TIME_SECS:
			rc = genoff_get(pargs);
			/* Convert the time to secs */
			*(uint64_t *)pargs->ts_val = (uint64_t)MSEC_TO_SEC(*(uint64_t *)pargs->ts_val);
			break;
		case TIME_JULIAN:
			temp_genoff_args.base        = pargs->base;
		        temp_genoff_args.ts_val      = &ts_val_secs;
		        temp_genoff_args.unit        = TIME_SECS;
		        temp_genoff_args.operation   = T_GET;
			rc = genoff_get(&temp_genoff_args);
			/* convert the time to julian */
			pargs->ts_val = gmtime((time_t *)&ts_val_secs);
			break;
		default:
		        TIME_LOGE("Daemon:%s:Invalid time unit %d", __func__, pargs->unit);
			return -EINVAL;
		        break;
		}
	}
	else if (pargs->operation == T_SET) {
		switch(pargs->unit) {
		case TIME_MSEC:
			rc = genoff_set(pargs);
			break;
		case TIME_SECS:
			/* Convert the time to msecs */
			ts_val_secs =  SEC_TO_MSEC(*(uint64_t *)pargs->ts_val);
			pargs->ts_val = &ts_val_secs;
		        rc = genoff_set(pargs);
			break;
		case TIME_JULIAN:
			/* Convert input time to UTC */
			ts_val_secs = mktime((struct tm *)pargs->ts_val);
		        temp_genoff_args.base        = pargs->base;
		        temp_genoff_args.ts_val      = &ts_val_secs;
		        temp_genoff_args.unit        = TIME_SECS;
		        temp_genoff_args.operation   = T_SET;
		        rc = genoff_opr(&temp_genoff_args);
		        break;
		default:
		        TIME_LOGE("Daemon:%s:Invalid time unit %d", __func__, pargs->unit);
			return -EINVAL;
		        break;
		}
	}

	return rc;
}

static void
genoff_add_base_subsys(time_genoff_ptr time_genoff,
				time_bases_type base)
{
	time_genoff->subsys_base = base;
	time_genoff->reqd_base_genoff = 1;
}

static int ats_bases_init(time_bases_type time_base, time_bases_type subsys_base,
				char *f_name, time_genoff_ptr ptime_genoff)
{
	int rc;

	genoff_pre_init(ptime_genoff, time_base);
	/* subsys_base = "parent" base */
	genoff_add_base_subsys(ptime_genoff, subsys_base);
	genoff_updates_per_storage(ptime_genoff, f_name,
				  TIME_GENOFF_UPDATE_THRESHOLD_MS);

	rc = genoff_post_init(ptime_genoff);
	if (rc) {
		TIME_LOGE("Daemon:%s: Post init failed for base = %d\n", __func__, time_base);
		return -EINVAL;
	}

	return 0;
}

static int ats_rtc_init(time_genoff_ptr ptime_genoff)
{
	int fd, rc;
	struct tm rtc_tm;
	time_t secs = 0;
	int64_t msecs =0;

	memset(&rtc_tm, 0, sizeof(struct tm));
	genoff_pre_init(ptime_genoff, ATS_RTC);

	fd = open("/dev/rtc0", O_RDONLY);
	if(fd < 0) {
		TIME_LOGE("Daemon:%s: Unable to open RTC device\n", __func__);
		return -EINVAL;
	}

	rc = ioctl(fd,RTC_RD_TIME,&rtc_tm);
	if(rc < 0) {
		TIME_LOGE("Daemon:%s: Unable to read from RTC device\n", __func__);
		close(fd);
		return -EINVAL;
	}

	TIME_LOGD("Daemon:%s: Time read from RTC -- year = %d, month = %d, day = %d\n",
		__func__, rtc_tm.tm_year, rtc_tm.tm_mon, rtc_tm.tm_mday);

	close(fd);

	/* Convert the time to UTC and then to milliseconds and store it */
	secs = mktime(&rtc_tm);
	secs += rtc_tm.tm_gmtoff;
	if(secs < 0) {
		TIME_LOGE("Daemon:Invalid RTC seconds = %ld\n", secs);
		return -EINVAL;
	}

	msecs = SEC_TO_MSEC(secs);
	TIME_LOGD("Daemon:Value read from RTC seconds = %lld\n", msecs);
	genoff_set_generic_offset(ptime_genoff, msecs);

	rc = genoff_post_init(ptime_genoff);
	if (rc) {
		TIME_LOGE("Daemon:%s: Genoff post_init operation failed\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int genoff_init_config(void)
{
	int i, rc;
	char f_name[FILE_NAME_MAX];

	/* Initialize RTC values */
	rc = ats_rtc_init(&ats_bases[0]);
	if (rc) {
		TIME_LOGE("Daemon:%s: RTC initilization failed\n", __func__);
		return -EINVAL;
	}

	TIME_LOGD("Daemon:%s: ATS_RTC initialized\n", __func__);

	/* Initialize the other offsets */
	for(i = 1; i < ATS_MAX; i++) {
		snprintf(f_name, FILE_NAME_MAX, "ats_%d", i);
		rc = ats_bases_init(i, ATS_RTC, f_name, &ats_bases[i]);
		if (rc) {
			TIME_LOGE("Daemon:%s: Init failed for base = %d\n", __func__, i);
			return -EINVAL;
		}
	}

	TIME_LOGD("Daemon:%s: Other bases initilized, exiting genoff_init\n", __func__);

	return 0;
}

static int genoff_boot_tod_init()
{
	uint64_t tod_value;
	int fd, res;
	struct timespec ts;

	time_genoff_info_type tod_genoff;

	TIME_LOGD("Daemon:%s:TOD value at boot = %llu\n",
				__func__, tod_value);

	tod_genoff.base = ATS_TOD;
        tod_genoff.unit = TIME_MSEC;
        tod_genoff.operation  = T_GET;
        tod_genoff.ts_val = &tod_value;
	res = genoff_opr(&tod_genoff);
	if (res < 0)
		return -EINVAL;

	ts.tv_sec = (time_t) MSEC_TO_SEC(tod_value);
	ts.tv_nsec = (long) ((tod_value % 1000LL) * 1000000LL);

	/*
	 * Update System time by writting to /dev/alarm this will susequently
	 * update the alarm deltas for maintaile elapsed realtime and uptime.
	 */

	fd = open("/dev/alarm", O_RDWR);
	if(fd < 0) {
		struct timeval tv;

		tv.tv_sec = ts.tv_sec;
		tv.tv_usec = ts.tv_nsec / 1000LL;

		TIME_LOGE("Daemon:%s: Unable to open alarm device\n",
				__func__);

		TIME_LOGE("Daemon:%s: Updating system time to sec=%ld, usec"
				"=%ld\n", __func__, tv.tv_sec, tv.tv_usec);
		/*
		 * Update system time (May be system is not using alarm
		 * framework).
		 */
		if (settimeofday(&tv, NULL) != 0) {
			TIME_LOGE("Daemon:%s: Unable to set clock to sec=%ld"
					"usec=%ld\n", __func__, tv.tv_sec,
					tv.tv_usec);
			return -EINVAL;
		}

		return 0;
	}

	TIME_LOGD("Daemon:%s: Setting system time to sec =%ld, nsec =%ld\n",
			__func__, ts.tv_sec, ts.tv_nsec);

	res = ioctl(fd, ANDROID_ALARM_SET_RTC, &ts);
	if(res < 0) {
		TIME_LOGE("Daemon:%s: Unable to set TOD at boot up\n",
				__func__);
		close(fd);
		return -EINVAL;
	}

	close(fd);
	return 0;
}

static int
genoff_send_modem(time_genoff_info_type *genoff_args)
{
	uint64_t time_value;
	TIME_LOGD("Daemon:%s: Sending data to MODEM !\n", __func__);
	if (time_remote_atom_null() == 0) {
		TIME_LOGE("Daemon:%s: Unable to find time_remote_atom on MODEM\n",
								 __func__);
		return -EINVAL;
	}

	/* Do a genoff opr call */
	genoff_args->unit = TIME_MSEC;
	time_value =  *(uint64_t *)genoff_args->ts_val;
	time_value -= (uint64_t)SEC_TO_MSEC(MODEM_EPOCH_DIFFERENCE);

	TIME_LOGD("Daemon: Base = %d, Value being sent to MODEM = %llu\n",
					genoff_args->base, time_value);

	time_remote_genoff_opr(genoff_args->base, genoff_args->unit,
		genoff_args->operation, TIME_SCLK, (time_remote_opr_type *)&time_value);

	return 0;
}

static void genoff_handler(void *recv_arg)
{
	int rc;
	int recv_id = (int) recv_arg;
	struct send_recv_struct to_recv ;
	time_genoff_info_type genoff_args;
	time_genoff_ptr ptime_genoff;

	/*
	 * Initialize base to 0 (Invalid )
	 */
	to_recv.base = 0;

	/*
	 * Receive data from the library
	 * Format: base, unit, operation, value (for set operation)
	 */
	if (recv(recv_id, (void *)&to_recv, sizeof(to_recv), 0) < 0) {
		TIME_LOGE("Daemon:Unable to recv data from client\n");
		goto error_invalid_input;
	}

	if (genoff_update_from_apps[to_recv.base] == 0 &&
			to_recv.operation == T_SET) {
		TIME_LOGE("Daemon:%s: Update is not allowed for "
				" offset[%d]\n", __func__, to_recv.base);
		to_recv.result = -EPERM;
		to_recv.base = to_recv.unit = to_recv.value = 0;
		if (send(recv_id, &to_recv, sizeof(to_recv), 0) < 0)
			TIME_LOGE("Daemon:Send to client failed %d\n",
					errno);

		goto error_invalid_input;
	}
	TIME_LOGD("Daemon:Received base = %d, unit = %d, operation = %d, value = %llu\n",
			to_recv.base, to_recv.unit, to_recv.operation, to_recv.value);

	genoff_args.base = to_recv.base;
	genoff_args.unit = to_recv.unit;
	genoff_args.operation = to_recv.operation;
	genoff_args.ts_val = (uint64_t *)&to_recv.value;

	/* Check if a valid base update is received */
	if (!genoff_valid_base[to_recv.base]) {
		TIME_LOGE("Daemon: Operation on this base is not supported\n");
		to_recv.result = -EINVAL;
		to_recv.base = to_recv.unit = to_recv.value = 0;
		if (send(recv_id, &to_recv, sizeof(to_recv), 0) < 0)
			TIME_LOGE("Daemon:Send to client failed %d\n",
					errno);
		goto error_invalid_input;
	}

	pthread_mutex_lock(&genoff_mutex);
	to_recv.result = genoff_opr(&genoff_args);
	pthread_mutex_unlock(&genoff_mutex);

	switch (genoff_args.operation) {
		case T_GET:
			if (send(recv_id, &to_recv, sizeof(to_recv), 0) < 0)
				TIME_LOGE("Daemon:Send to client failed %d\n",
						errno);
			break;
		case T_SET:
			/* Send data, result back to library */
			if (send(recv_id, &to_recv, sizeof(to_recv), 0) < 0)
				TIME_LOGE("Daemon:Send to client failed %d\n",
						errno);
			/* Send the data to MODEM, if to_modem bit is set */
			ptime_genoff = genoff_get_pointer(to_recv.base);
			if (ptime_genoff == NULL) {
				TIME_LOGE("Daemon:%s:Genoff memory not allocated\n", __func__);
				break;
			}
			if (genoff_update_to_modem[ptime_genoff->bases_type] &&
							modem_rpc_initialized) {
				TIME_LOGE("Daemon:Update to modem bit set\n");
				if ((rc = genoff_send_modem(&genoff_args)) < 0)
					TIME_LOGE("Daemon:Unable to send data to MODEM\n");
			}
			break;
		default:
			TIME_LOGE("Daemon:%s: Invalid option\n", __func__);
			break;
	}

error_invalid_input:
	close(recv_id);
	pthread_detach(pthread_self());
	pthread_exit(NULL);
}

static void
time_remote_genoff_cb(ats_cfg_event_e_type event, ats_cfg_event_info_s_type *info_ptr)
{
	int rc;
	uint64_t time_value;
	time_genoff_info_type genoff_update;
	time_genoff_ptr ptime_genoff;

	if (event != ATS_CFG_EVENT_GENOFF_CHANGE)
		return;

	ptime_genoff = genoff_get_pointer(info_ptr->ats_genoff_update.base_type);
	if (ptime_genoff == NULL) {
		TIME_LOGE("Daemon: ptime_genoff is NULL \n");
		return;
	}

	if (genoff_update_from_modem[ptime_genoff->bases_type] == 0) {
		TIME_LOGE("Daemon: Ignore time update from MODEM, for base = %d\n",
							ptime_genoff->bases_type);
		return;
	}

	TIME_LOGD("Daemon: %s: Callback from MODEM for base = %d\n",
					__func__, ptime_genoff->bases_type);

	pthread_mutex_lock(&genoff_mutex);
	/* Get the time for the base which has changed */
	time_remote_genoff_opr(info_ptr->ats_genoff_update.base_type,
		TIME_MSEC, T_GET, TIME_SCLK, (time_remote_opr_type *)&time_value);

	TIME_LOGD("Time received from MODEM = %llu\n", time_value);
	/* Add the MODEM EPOCH difference */
	time_value += (uint64_t)(MODEM_EPOCH_DIFFERENCE * 1000ULL);

	TIME_LOGD("Time after adding MODEM EPOCH diff = %llu\n", time_value);

	/* update the time locally to the required base */
	genoff_update.base = info_ptr->ats_genoff_update.base_type;
        genoff_update.unit = TIME_MSEC;
        genoff_update.operation  = T_SET;
        genoff_update.ts_val = &time_value;
	rc = genoff_opr(&genoff_update);
	pthread_mutex_unlock(&genoff_mutex);

	TIME_LOGD("Daemon:%s: Local Genoff update for base = %d , rc = %d\n",
		__func__, genoff_update.base, rc);
}

static int genoff_modem_rpc_init()
{
	uint32_t enable_bases = 0;
	int i = 0, rc;
	/* Intialize ONCRPC, this will fail if modem does not exist */
	oncrpc_init();
	oncrpc_task_start();

	/* Check if the remote server on MODEM exists */
	if (time_remote_atom_null() == 0) {
		TIME_LOGD("Daemon: time_remote_atom RPC init failed\n");
		modem_rpc_initialized = 0;
		return -EINVAL;
	}

	TIME_LOGE("Daemon: time_remote_atom RPC passed\n");

	/* Initialize RPC call backs */
	time_remote_atomcb_app_init();

	/* Disable remote updates for based not to be updated from MODEM */
	for (i = 0 ; i < ATS_MAX ; i++) {
		if (genoff_update_from_modem[i])
			enable_bases |= (1 << ats_bases[i].bases_type);
	}
	TIME_LOGD("Daemon: Bases allowed for modem updates = %x\n", enable_bases);
	time_remote_genoff_set_allow_remote_updates(enable_bases);

	/* Intialisation done changing UID/GID */
	rc = setgid(AID_SYSTEM);
	if (rc < 0) {
		TIME_LOGE("Error changing gid :%d \n", rc);
		TIME_LOGE("Time-services exiting\n");
		exit(-1);
	}

	rc = setuid(AID_SYSTEM);
	if (rc < 0) {
		TIME_LOGE("Error changing uid :%d \n", rc);
		TIME_LOGE("Time-services exiting\n");
		exit(-1);
	}

	/*
	 * Setup a callback with MODEM, this is required as there are time
	 * updates on MODEM which need to be communicated to APPS. APPS then
	 * updates its TOD / offset.
	 */
	ats_client_init(ATS_CLIENT_TYPE_GENOFF_UPDATE, &time_genoff_set_id, ATS_CLIENT_PROCESSOR_APP1);
	ats_client_reg_cfg_cb(time_genoff_set_id, (ats_cfg_event_cb_type) &time_remote_genoff_cb);

	modem_rpc_initialized = 1;

	return 0;
}

int main(void)
{
	int rc;
	int sock_id, recv_id, length, recv_val;
	struct sockaddr_un time_socket, time_recv;
	pthread_t time_thread;

	/* We expect socket write failure so ignore SIGPIPE */
	signal(SIGPIPE, SIG_IGN);

	/* Initialization of bases */
	rc = genoff_init_config();
	if (rc) {
		TIME_LOGE("Daemon: Unable to initialize bases, exiting\n");
		return -EINVAL;
	}

	if (pthread_mutex_init(&genoff_mutex,NULL)) {
		TIME_LOGE("Daemon: Pthread mutex init failed\n");
		goto error_return;
	}

	/* Initialize TOD if stored previously */
	if (genoff_boot_tod_init() < 0)
		TIME_LOGE("Daemon: Unable to set TOD at boot up\n");

	rc = genoff_modem_rpc_init();
	if (rc)
		TIME_LOGE("Daemon: RPC init failed, RPC calls inactive\n");

	/* Start a server to accept connections from the shared library */
	sock_id = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock_id < 0) {
		TIME_LOGE("Daemon: Unable to create socket:time_genoff\n");
		return -EINVAL;
	}
	time_socket.sun_family = AF_UNIX;
	strlcpy(time_socket.sun_path, GENOFF_SOCKET_NAME, UNIX_PATH_MAX);
	/* abstract domain socket */
	time_socket.sun_path[0] = 0;
	length = strlen(GENOFF_SOCKET_NAME) + sizeof(time_socket.sun_family);
	/* Remove any existing socket with the same name */
	unlink(time_socket.sun_path);
	if (bind(sock_id, (struct sockaddr *)&time_socket, length) < 0) {
		TIME_LOGE("Daemon: Unable to bind socket:time_genoff\n");
		goto err_close_socket;
	}

	if (listen(sock_id, GENOFF_MAX_CONCURRENT_CONN) < 0) {
		TIME_LOGE("Daemon: Unable to listen on socket:time_genoff\n");
		goto err_close_socket;
	}

	TIME_LOGE("Daemon: Time-services: All initializations done\n");

	while (1) {
		/* Loop to accept connections from the shared library */
		TIME_LOGE("Daemon: Time-services: Waiting to accept connections\n");
		recv_id = accept(sock_id, (struct sockaddr *)&time_recv,&recv_val);
		if (recv_id < 0) {
			TIME_LOGE("Daemon: Unable to accept connection:time_genoff\n");
			continue;
		}

		TIME_LOGD("Daemon: Connection accepted:time_genoff\n");
		/* Thread to handle this request */
		rc = pthread_create(&time_thread, NULL, (void *)&genoff_handler,
							(void *)recv_id);
		if (rc < 0)
			TIME_LOGE("Daemon: Cannot create pthread:time_genof\n");
	}

	return 0;

err_close_socket:
	close(sock_id);
error_return:
	return -EINVAL;
}
