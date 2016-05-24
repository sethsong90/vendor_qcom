/******************************************************************************
 *
 * 	D E C I S I O N . C
 *
 * GENERAL DESCRIPTION
 * 	Multicore Decision Engine

 Copyright (c) 2010-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <time.h>
#include <getopt.h>
#include <pthread.h>
#include <poll.h>
#include <dirent.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <sys/syscall.h>
#include <private/android_filesystem_config.h>
#include <linux/un.h>
#include <cutils/uevent.h>
#include "decision.h"
#include <sched.h>
#ifdef MPCTL_SERVER
extern int  mpctl_server_init(void);
extern void mpctl_server_exit(void);
#endif

#define __NR_sched_setaffinity  (__NR_SYSCALL_BASE+241) /* ARM arch specific */

#define MAX_BUF (30)
#define MAX_PATH (256)
#define MIN_CPUS (1)

#define RQ_DEPTH "/sys/devices/system/cpu/cpu0/rq-stats/run_queue_avg"
#define RQ_POLL_MS "/sys/devices/system/cpu/cpu0/rq-stats/run_queue_poll_ms"
#define DEF_TIMER_MS "/sys/devices/system/cpu/cpu0/rq-stats/def_timer_ms"
#define CPU_LOAD_NORMALIZED "/sys/devices/system/cpu/cpu0/rq-stats/cpu_normalized_load"
#define HOTPLUG_DISABLE "/sys/devices/system/cpu/cpu0/rq-stats/hotplug_disable"
#define CPU_SYSFS "/sys/devices/system/cpu"
#define KTM_NODE "/sys/module/msm_thermal/core_control/cpus_offlined"
#define PM_QOS "/dev/cpu_dma_latency"
#define CPU_BOOST_MS 500
#define MAX_UTIL_WNDW 20
#define ENABLE_PC_LATENCY -1
#define DISABLE_PC_LATENCY 100

#define get_cpu_at_pos(idx) ((idx < available_cpus && idx >= 0)? \
		avail_cpus[idx]: 0)

#define for_each_cpu(i) \
	for (i = 0; i < num_cpus; i++)

/* Order in which cores have to be brought online for every target.
 * Includes cores with thermal condition as well
 */
static int core_pref_order[MAX_CPUS] = {0, 1, 2, 3};
/* Order in which cores have to be brought online for every target.
 * Excludes cores with thermal condition
 */
static int avail_cpus[MAX_CPUS] = {0};
static int available_cpus;
static int rq_depth_fd = 0;
static int cpu_load_normalized_fd = 0;
static int hotplug_disabled_fd = 0;
static int cpu_fd[MAX_CPUS] = {0, 0, 0, 0};
static int pm_qos_fd = 0;
static int mpctl_toggle_on = 0;
static int ktm_node_fd = 0;

/* Online/offline status of cores */
static enum CORE_STATUS core_status[MAX_CPUS];

/* Default average active-threads for wakeup threshold per-core */
static const float DEF_Nw[MAX_CPUS] = {0, 1.9, 2.7, 3.5};
/* Default average active-threads for sleep threshold per-core */
static const float DEF_Ns[MAX_CPUS] = {0, 1.1, 2.1, 3.1};
/* Default average wakeup time threshold per-core (in ms) */
static const int DEF_Tw[MAX_CPUS] = {0, 140, 90, 90};
/* Default average sleep time threshold per-core (in ms) */
static const int DEF_Ts[MAX_CPUS] = {0, 190, 240, 240};
/* Default utilization threshold per-core (%) */
static const int DEF_util_high_and[MAX_CPUS] = {0, 35, 0, 0}; //{0, 50, 115, 210};
/* OR Criterion only for Core 1 */
static const int DEF_util_high_or[MAX_CPUS] = {0, 75, 400, 400};
static const int DEF_util_low_and[MAX_CPUS] = {0, 45, 400, 400};
static const int DEF_util_low_or[MAX_CPUS] = {0, 20, 0, 0};
float total_time_up[MAX_CPUS] = {0};
float total_time_down[MAX_CPUS] = {0};
static const int DEF_max_up_wndw_size = 3;
static const int DEF_max_dw_wndw_size = 5;

static int num_cpus;
static float Nw_cpus[MAX_CPUS];
static float Ns_cpus[MAX_CPUS];
static unsigned int Tw_cpus[MAX_CPUS];
static unsigned int Ts_cpus[MAX_CPUS];
static int util_high_and[MAX_CPUS];
static int util_high_or[MAX_CPUS];
static int util_low_and[MAX_CPUS];
static int util_low_or[MAX_CPUS];
static int max_up_wndw_size;
static int max_dw_wndw_size;
static unsigned int poll_ms = 9;
static unsigned int decision_ms = 50;
static int def_timer_fd = 0;
static unsigned int boost_time = 0;

static int control_sleep_modes = 0;
static int adjust_average = 0;
static int lock_min_cores = 0;
static int lock_max_cores = MAX_CPUS;

static unsigned int cpu_avg_load_window[MAX_UTIL_WNDW]={0};
static int cpu_load_index=0;
static int valid_in_wndw_up = 0;
static int valid_in_wndw_dw = 0;

static pthread_t mp_decision, thermal_monitor, hotplug_monitor;

/* hotplug_mutex protects
   core_status[], stall_decision_external, and thermal_condition */
static pthread_mutex_t hotplug_mutex = PTHREAD_MUTEX_INITIALIZER;

/* bitmask for thermal_condition of hotplug cores, core 0 is ignored
 * ASSUMPTION is made here num_cpus <= 32 */
static uint32_t thermal_condition;

/* macro to check if cpu in thermal condition */
#define THERMAL_COND_CPU(cpu)   ((thermal_condition) & (1<<(cpu)))

/* bitmask for all hotplug cores, initialized in all_cores_thermal_condition */
static uint32_t hotplug_core_mask;

static int get_cpu_status(int cpu);
static int __act_on_decision(int cpu, unsigned int status);
static inline int __get_num_online_cpus();
static int  set_pm_qos_latency();

/**************************
 * EXTERNALIZED FUNCTIONS *
 **************************/

/* mpdecision hook for disabling power collapse externally */
int mpdecision_toggle_power_collapse(int on)
{
	int ret;
	pthread_mutex_lock(&hotplug_mutex);
	mpctl_toggle_on = on? 0: 1;
	ret = set_pm_qos_latency();
	pthread_mutex_unlock(&hotplug_mutex);
	return ret;
}

/* mpdecision hook for locking a maximum required cores for power */
int mpdecision_lock_max_cores(int max)
{
	int cpu;
	int ret = 0;
	int i;

	if (max <= 0)
		return -EINVAL;

	if (max > num_cpus)
		max = num_cpus;

	pthread_mutex_lock(&hotplug_mutex);
	lock_max_cores = max;

	for (i = available_cpus - 1; i > 0; i--) {
		if (__get_num_online_cpus() <= max)
			break;
		cpu = get_cpu_at_pos(i);
		if (core_status[cpu] == CORE_UP)
			ret = __act_on_decision(cpu, CORE_DOWN);
	}

	if (__get_num_online_cpus() > lock_max_cores) {
		info("Error locking maximum cores\n");
		ret = -EBUSY;
	}

	pthread_mutex_unlock(&hotplug_mutex);
	return ret;
}

/* mpdecision hook for locking a minimum required cores for performance */
int mpdecision_lock_min_cores(int min)
{
	int cpu;
	int ret = 0;
	int i;

	if (min < 0)
		return -EINVAL;

	if (min > num_cpus)
		min = num_cpus;

	pthread_mutex_lock(&hotplug_mutex);
	lock_min_cores = min;

	for (i = 1; i < available_cpus; i++) {
		if (__get_num_online_cpus() >= lock_min_cores)
			break;
		cpu = get_cpu_at_pos(i);
		if (core_status[cpu] == CORE_DOWN)
			ret = __act_on_decision(cpu, CORE_UP);
	}

	if (__get_num_online_cpus() < lock_min_cores) {
		info("Error locking minimum cores\n");
		ret = -EBUSY;
	}

	pthread_mutex_unlock(&hotplug_mutex);
	return ret;
}

/*******************
 * LOCAL FUNCTIONS *
 *******************/

/*Update pm_qos latency to allow/disallow power collapse modes*/
static int update_pm_qos_latency(int latency)
{
	int ret = 0;
	static int prev_latency = -1;

	if (prev_latency == latency) {
		dbgmsg("Pm_qos latency already set to %d\n", prev_latency);
		return 0;
	}

	if (pm_qos_fd < 0) {
		msg("Error pm-qos file descriptor invalid\n");
		return -EINVAL;
	}

	if (write(pm_qos_fd, &latency, sizeof(latency)) == -1) {
		dbgmsg("Unable to set pm_qos latency to %d\n", latency);
		return errno;
	}
	else {
		dbgmsg("Set pm_qos latency to %d\n", latency);
		prev_latency = latency;
	}
	return 0;
}

/* Set appropriate latency based on the number of cores
 * and performance settings in mpctl*/
static int set_pm_qos_latency()
{
	int latency = ENABLE_PC_LATENCY;

	if (__get_num_online_cpus() > 1 &&
			(control_sleep_modes || mpctl_toggle_on))
			latency = DISABLE_PC_LATENCY;

	return update_pm_qos_latency(latency);
}

/* Check all secondary cores are in thermal_condition */
static int all_cores_thermal_condition(void)
{
	static int hotplug_mask_inited;
	int cpu;

	if (!hotplug_mask_inited) {
		for (cpu = 1; cpu < num_cpus; cpu++)
			hotplug_core_mask |= (1 << cpu);
		hotplug_mask_inited = 1;
	}
	return ((thermal_condition & hotplug_core_mask) == hotplug_core_mask);
}

/* Read float value from file
   * file - file name
   * val - output float pointer
   Return value - 0 if successful, -errno if failed */
static int read_rq_depth(char *file, float *val)
{
	int ret = -ENODEV;
	char buf[MAX_BUF] = {0};
	static pthread_mutex_t rq_depth_mutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&rq_depth_mutex);

	if (rq_depth_fd >= 0) {
		/* ensure NULL terminated, sizeof(buf) - 1 */
		if (read(rq_depth_fd, buf, sizeof(buf) - 1) == -1)
			ret = -EINVAL;
		else {
			lseek(rq_depth_fd, 0, SEEK_SET);
			*val = strtof(buf, NULL);
			ret = 0;
		}
	}
	pthread_mutex_unlock(&rq_depth_mutex);

	return ret;
}

/* Read int value from file
 * file - file name
 * val - output int pointer
 * Return value - 0 if successful, -errno if failed */
static int read_hotplug(char *file, unsigned int *val)
{
	int ret = -ENODEV;
	char buf[MAX_BUF] = {0};
	static pthread_mutex_t hotplug_disabled_mutex
				= PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&hotplug_disabled_mutex);
	if (hotplug_disabled_fd >= 0) {
	/* ensure NULL terminated, sizeof(buf) - 1 */
		if (read(hotplug_disabled_fd, buf, sizeof(buf) - 1) == -1)
			ret = -EINVAL;
		else {
			lseek(hotplug_disabled_fd, 0, SEEK_SET);
			*val = atoi(&buf[0]);
			ret = 0;
		}
	}
	pthread_mutex_unlock(&hotplug_disabled_mutex);
	return ret;
}

/* Write uint32 value from file
   * file - file name
   * val - uint32 value to write
   Return value - 0 if successful, -errno if failed */
static int write_file_uint32(char *file, unsigned int val)
{
	int ret = -ENODEV;
	int fd = 0;
	char buf[MAX_BUF] = {0};

	snprintf(buf, MAX_BUF, "%u\n", val);

	fd = open(file, O_WRONLY);
	if (fd > 0) {
		if (write(fd, buf, sizeof(buf)) == -1)
			ret = -EINVAL;
		else
			ret = 0;
		close(fd);
	}

	return ret;
}

/* Read CPU stats from sysfs nodes and compute utilization
   * hotplug_avg_load_up - computed integer value
   * hotplug_avg_load_dw - computed integer value
   * num_online - current number of online CPUs
   Return value - 0 if successful, -errno if failed */
static int get_cpu_utils(int *hotplug_avg_load_up,
				   int *hotplug_avg_load_dw,
				   int num_online)
{
	int ret = -ENODEV;
	int fd = 0;
	char buf[MAX_BUF] = {0};
	int i,j;
	int count = 0;
	int max_wndw;
	int val[MAX_CPUS] = {0};

	*hotplug_avg_load_up = 0;
	*hotplug_avg_load_dw = 0;

	if (valid_in_wndw_up < max_up_wndw_size) {
		valid_in_wndw_up++;
	}
	if (valid_in_wndw_dw < max_dw_wndw_size) {
		valid_in_wndw_dw++;
	}

	cpu_avg_load_window[cpu_load_index] = 0;

	/* Get the total normalized cpu_util */
	if (cpu_load_normalized_fd >= 0) {
		/* ensure NULL terminated, sizeof(buf) - 1 */
		if (read(cpu_load_normalized_fd, buf, sizeof(buf) - 1) == -1)
			ret = -EINVAL;
		else {
			lseek(cpu_load_normalized_fd, 0, SEEK_SET);
			cpu_avg_load_window[cpu_load_index] = strtol(buf, NULL, 0);
			ret = 0;
		}
	}

	/* compute average load across 'up' sampling periods */
	for (i = 0, j = cpu_load_index; i < valid_in_wndw_up; i++, j--) {
		*hotplug_avg_load_up += cpu_avg_load_window[j];
		if (j == 0)
			j = MAX_UTIL_WNDW;
	}
	/* compute avg loads across the whole 'up' window */
	*hotplug_avg_load_up  = *hotplug_avg_load_up/valid_in_wndw_up;

	/* compute average load across 'dw' sampling periods */
	for (i = 0, j = cpu_load_index; i < valid_in_wndw_dw; i++, j--) {
		*hotplug_avg_load_dw += cpu_avg_load_window[j];
		if (j == 0)
			j = MAX_UTIL_WNDW;
	}
	/* compute avg loads across the whole 'dw' window */
	*hotplug_avg_load_dw  = *hotplug_avg_load_dw/valid_in_wndw_dw;

	/* return to first element if we're at the circular buffer's end */
	if (++cpu_load_index == MAX_UTIL_WNDW)
		cpu_load_index = 0;

	return ret;
}

/* Update CPU hotplug 'online' sysfs node
   * cpu - core id
   * on - 0/1 for offline/online
   Return value: 0 if successful, -errno if error */
static int __cpu_up(int cpu, int on)
{
	int ret = 0;
	int fd;
	char str[MAX_BUF * 3] = {0};
	char val[MAX_BUF] = {0};
	unsigned int hotplug_disabled;

	snprintf(str, MAX_BUF * 3, "/sys/devices/system/cpu/cpu%d/online", cpu);
	snprintf(val, MAX_BUF, "%d\n", (on ? 1 : 0));

	fd = open(str, O_RDWR);
	if (fd > 0) {
		if (write(fd, val, strnlen(val, MAX_BUF)) == -1)
			ret = -EINVAL;
		else
			core_status[cpu] = on;

		close(fd);
	}
	else
		ret = -ENODEV;

	if (ret) {
		switch (errno) {
		case EBUSY:
			if (!read_hotplug(HOTPLUG_DISABLE,
					&hotplug_disabled)) {
				if (hotplug_disabled == 1) {
					msg("Hotplug is disabled, action"
						"could not be completed\n");
					ret = 0;
				}
				/* If this core is the last secondary core to
				* come offline and it is already offlined
				* msm_thermal,kernel return EBUSY error. Set
				* the core status to DOWN and return without
				* any error.
				*/
				else {
					if(THERMAL_COND_CPU(cpu)) {
						core_status[cpu] = CORE_DOWN;
						ret = 0;
					}
				}
			} else {
				msg("Error reading hotplug value");
			}
			break;
		case EINVAL:
			core_status[cpu] = on;
			ret = 0;
			dbgmsg("%s: No update required for cpu%d\n",__func__, cpu);
			break;
		default:
			/* If the thermal has already offlined the core, set the
			 * core status to CORE_DOWN
			 */
			if (THERMAL_COND_CPU(cpu)) {
				core_status[cpu] = CORE_DOWN;
				ret = 0;
			}
			else {
				msg("Error %d setting online status to %d for"
						"cpu%d\n", errno, on, cpu);
			}
			break;
		}
	}

	if (!ret)
		set_pm_qos_latency();

	return ret;
}

/* Utility function to read cpu
   * cpu - core id
   * on - 0/1 for offline/online
   Return value: 0/1 core status, -errno if error */
static int get_cpu_status(int cpu)
{
	int status = CORE_DOWN;
	char buf[MAX_BUF] = {0};

	if (cpu_fd[cpu] >= 0) {
		/* ensure NULL terminated, sizeof(buf) - 1 */
		if (read(cpu_fd[cpu], buf, sizeof(buf) - 1) == -1)
			buf[0] = '0';
		lseek(cpu_fd[cpu], 0, SEEK_SET);
		buf[1] = '\0';
		status = atoi(buf) ? CORE_UP: CORE_DOWN;
	} else {
		msg("Error %d getting cpu %d online status", errno, cpu);
		status = -ENODEV;
	}

	return status;
}

/* Get current number of online CPUs
   Return value: number of online CPUs */
static inline int __get_num_online_cpus()
{
	int online = 0;
	int cpu = 0;

	for (cpu = 0; cpu < num_cpus; cpu++)
		if (core_status[cpu] == CORE_UP)
			online++;

	return online;
}

/* Updates the CPU online status, with side-effect of controlling sleep modes
   (if the option is enabled). In leaving one-core-up system state (to deal with
   system load), power modes should be saved and idle power collapse modes
   disabled. On returning to one-core-up state, power modes should be restored.
   Must called with hotplug_mutex held.
   * cpu - core id
   * status - 0/1 for offline/online
   Return value - 0 if successful, -errno if error */
static int __cpu_action_control_sleep(int cpu, unsigned int status)
{
	int ret = 0;
	unsigned int hotplug_disabled;

	if (status == core_status[cpu]) {
		dbgmsg("No action required to change core status(%d).", status);
		return 0;
	}

	if (!read_hotplug(HOTPLUG_DISABLE, &hotplug_disabled)) {
		if(hotplug_disabled == 1) {
			dbgmsg("Cannot change cpu%d status as"
					"hotplug is disabled", cpu);
			return 0;
		}
	}

	dbgmsg("Action to bring core %d %s\n",
			cpu, (status == CORE_UP) ? "online" : "offline");

	switch (status) {
	case CORE_UP:
		/* Power up the core */
		ret = __cpu_up(cpu, CORE_UP);
		break;
	case CORE_DOWN:
		/* Power down the core */
		ret = __cpu_up(cpu, CORE_DOWN);
		break;
	default:
		break;
	}

	if (ret) {
		msg("Error(%d) changing core %d status to %s\n", ret, cpu,
				status ? "online" : "offline");
	} else {
		dbgmsg("Core %d is now %s", cpu,
		       (status == CORE_UP ? "online" : "offline"));
	}

	return ret;
}

/* Acts on MPdecision algorithm to update core status.
   Decision may be ignored if externally stalled or
   system in thermal condition.
   * cpu - core id
   * status - 0/1 for offline/online
   Return value - 0 if successful, -errno if error */
static int __act_on_decision(int cpu, unsigned int status)
{
       if (THERMAL_COND_CPU(cpu)) {
		msg("CPU%d in thermal condition, request to set core status "
		    "to %d ignored\n", cpu, status);
		return -ENODEV;
       }

       return __cpu_action_control_sleep(cpu, status);
}

/* Do deferrable sleep (allowing for power collapse)
   * sleep_ms - number of millisecs to sleep
   Return value - length of time slept */
static int deferrable_sleep(int sleep_ms)
{
	int err = 0;
	int time_ms = 0;
	struct pollfd fds;
	char buf[20] = {0};

	write_file_uint32(DEF_TIMER_MS, sleep_ms);

	while (1) {
		fds.fd = def_timer_fd;
		fds.events = POLLERR|POLLPRI;
		fds.revents = 0;

		err = poll(&fds, 1, -1);
		if (err == -1) {
			msg("Error waiting for timer\n");
			close(fds.fd);
			return 0;
		}

		/* ensure NULL terminated, sizeof(buf) - 1 */
		if (read(def_timer_fd, buf, sizeof(buf) - 1) != -1) {
			lseek(def_timer_fd, 0, SEEK_SET);

			time_ms = atoi(buf);
		}
		if (time_ms)
			break;
	}

	return time_ms;
}

static void thermally_restricted_cores()
{
	int i, j;
	char cores[MAX_BUF * 3] = {0};
	int len = 0;

	pthread_mutex_lock(&hotplug_mutex);

	available_cpus = 0;
	memset(avail_cpus, 0, sizeof(avail_cpus));
	for (i = 0, j = 0; i < num_cpus; i++)
		if (!THERMAL_COND_CPU(core_pref_order[i])) {
			avail_cpus[j] = core_pref_order[i];
			available_cpus++;
			j++;
		}

	pthread_mutex_unlock(&hotplug_mutex);

	for (i = 0; i < available_cpus; i++) {
		len += snprintf(cores + len, MAX_BUF, " %d", avail_cpus[i]);
	}
	info("Available core list/order for available_cpus - %d: %s\n",
			available_cpus, cores);
}

/*Thermal monitoring thread*/
static void *do_thermal_monitoring(void *data)
{
	int err = 0;
	int ret;
	struct pollfd fds;
	uint32_t prev_thermal_cond;
	int cpu, cpu_condition, prev_cpu_condition;

	if (ktm_node_fd < 0) {
		msg("KTM node is not available."
			"Exiting thermal_monitor thread\n");
		return 0;
	}

	while (1) {
		char buf[20] = {0};
		fds.fd = ktm_node_fd;
		fds.events = POLLERR|POLLPRI;
		fds.revents = 0;
		prev_thermal_cond = thermal_condition;

		err = poll(&fds, 1, -1);
		if (err == -1) {
			msg("Error waiting for thermal_condition\n");
			close(fds.fd);
			return 0;
		}

		/* ensure NULL terminated, sizeof(buf) - 1 */
		if (read(ktm_node_fd, buf, sizeof(buf) - 1) != -1) {
			lseek(ktm_node_fd, 0, SEEK_SET);
			thermal_condition = atoi(buf);
			dbgmsg("Received thermal notification%d\n",
					thermal_condition);
			if (prev_thermal_cond == thermal_condition)
				continue;

			thermally_restricted_cores();

			for (cpu = 1; cpu < num_cpus; cpu++) {
				cpu_condition = THERMAL_COND_CPU(cpu);
				prev_cpu_condition = prev_thermal_cond &
							(1<<cpu);

				if ((prev_cpu_condition != cpu_condition)
					&& (cpu_condition == CORE_DOWN)) {
					dbgmsg("Thermal condition removed,"
					"bringing core%d online\n", cpu);

					pthread_mutex_lock(&hotplug_mutex);

						ret = __act_on_decision(cpu,
								CORE_UP);

					pthread_mutex_unlock(&hotplug_mutex);

				} else if (!prev_cpu_condition &&
						cpu_condition) {
					core_status[cpu] = CORE_DOWN;
				}
			}
		}
	}
}

static void down_cpus(int cpus_reqd)
{
	int cpu;
	int core_idx;
	int ret;
	int num_online = __get_num_online_cpus();

	dbgmsg("cpus_reqd = %d, num_online = %d\n", cpus_reqd, num_online);

	for (core_idx = available_cpus - 1; core_idx > 0; core_idx--){
		cpu = get_cpu_at_pos(core_idx);
		ret = __act_on_decision(cpu, CORE_DOWN);
		if (!ret)
			total_time_down[core_idx] = 0;
		num_online = __get_num_online_cpus();
		if (cpus_reqd == num_online)
			break;
	}
}

static void up_cpus(int cpus_reqd)
{
	int cpu;
	int core_idx;
	int ret;
	int num_online = __get_num_online_cpus();

	dbgmsg("cpus_reqd = %d, num_online = %d\n", cpus_reqd, num_online);

	for (core_idx = 1; core_idx < available_cpus; core_idx++){
		cpu = get_cpu_at_pos(core_idx);
		if (core_status[cpu] == CORE_UP)
			continue;
		ret = __act_on_decision(cpu, CORE_UP);
		if (!ret)
			total_time_up[core_idx] = 0;
		num_online = __get_num_online_cpus();
		if (cpus_reqd == num_online)
			break;
	}
}

static int get_cpus_required(float rq_depth, float time)
{
	float Nw = 0.0;
	float Ns = 0.0;
	int Tw = 0;
	int Ts = 0;
	int cpus_reqd;
	int cpus_up = 0;
	int cpus_down = 0;
	int core_idx = 1;
	int condition1 = 0;
	int condition2 = 0;
	int cond1_util_and = 0;
	int cond2_util_or = 0;
	int hotplug_avg_load_up;
	int hotplug_avg_load_dw;
	int num_online = __get_num_online_cpus();

	/*get cpu utilization */
	if(get_cpu_utils(&hotplug_avg_load_up, &hotplug_avg_load_dw, num_online) < 0) {
		msg("Error reading cpu load.\n");
		return -1;
	}

	dbgmsg("Runqueue depth :%f load_up :%d load_dw :%d time :%f online :%d\n",
		rq_depth, hotplug_avg_load_up, hotplug_avg_load_dw, time, num_online);

	cpus_reqd = num_online;

	for (core_idx = 1; core_idx < num_online; core_idx++) {
		Ns = Ns_cpus[core_idx];
		Ts = Ts_cpus[core_idx];
		/* 'cpu' is online */
		if (rq_depth <= Ns)
			total_time_down[core_idx] += time;
		else
			total_time_down[core_idx] = 0;
		total_time_up[core_idx] = 0;

		if (!cpus_down) {
			condition1 = ((total_time_down[core_idx] >= Ts) &&
				(hotplug_avg_load_dw < util_low_and[core_idx]));
			condition2 =
				(hotplug_avg_load_dw < util_low_or[core_idx]);

			if (condition1 || condition2)
				cpus_down++;
		}
	}

	for (core_idx = num_online; core_idx < available_cpus; core_idx++) {
		Nw = Nw_cpus[core_idx];
		Tw = Tw_cpus[core_idx];
		/* 'cpu' is offline */
		if (rq_depth >= Nw)
			total_time_up[core_idx] += time;
		else
			total_time_up[core_idx] = 0;
		total_time_down[core_idx] = 0;
		cond1_util_and =  util_high_and[num_online]; /* NEW */
		if (core_idx == num_online) {
			/* if it is considering to bring online just
			 * one additional core */
			cond2_util_or = util_high_or[core_idx];
		} else {
			/*
			 * if it is adding multiple cores at once, then
			 * - the system must be 100% busy
			 * - cannot add multiple core by using
			 *   the util_or condition only
			 */
			cond2_util_or = INT_MAX;
		}

		/* Verify the conditions to bring the core online */
		condition1 = ((total_time_up[core_idx] >= Tw) &&
			(hotplug_avg_load_up >= cond1_util_and));
		condition2 = (hotplug_avg_load_up >= cond2_util_or);

		if (condition1 || condition2)
			cpus_up++;
	}
	dbgmsg("cpus_up: %d cpus_down: %d thermal_mask: 0x%x num_online %d\n",
		       cpus_up, cpus_down, thermal_condition, num_online);

	if (cpus_up) {
		cpus_reqd = num_online + cpus_up;
		if (cpus_reqd > num_cpus) {
			cpus_reqd = num_cpus;
			msg("Required number of cpus > num of cpus available");
		}
	} else if (cpus_down) {
		if (num_online >= cpus_down)
			cpus_reqd = num_online - cpus_down;
		else {
			cpus_reqd = MIN_CPUS;
			msg("Required number of cpus < num online");
		}
	}

	return cpus_reqd;
}

/* hotplug thread - listens for kernel hotplug events */
static void *do_hotplug_monitoring(void *data)
{
	int err = 0;
	int cpu = 1;
	struct pollfd fds;
	char buf[1024] = {0};
	char cpu_online[MAX_BUF * 2] = {0};
	char cpu_offline[MAX_BUF * 2] = {0};
	unsigned int online;
	int recv_bytes = 0;

	/* Looking for online@/devices/system/cpu/cpuX or
	* offline@/devices/system/cpu/cpuX */
	snprintf(cpu_online, MAX_BUF * 2, "online@/devices/system/cpu/cpu" );
	snprintf(cpu_offline, MAX_BUF * 2, "offline@/devices/system/cpu/cpu");

	fds.events = POLLIN;
	fds.fd = uevent_open_socket(64*1024, true);
	if (fds.fd == -1) {
		msg("Error opening socket for hotplug uevent.\n");
		return NULL;
	}

	while (1) {

		err = poll(&fds, 1, -1);
		if (err == -1) {
			msg("Error in hotplug CPU[%d] poll.\n", cpu);
			break;
		}

		recv_bytes = uevent_kernel_multicast_recv(fds.fd, buf,
						sizeof(buf));
		if (recv_bytes == -1)
			continue;
		if (recv_bytes >= 1024)
			buf[1023] = '\0';
		else
			buf[recv_bytes] = '\0';

		dbgmsg("Hotplug read: %s\n", buf);

		if (strstr(buf, cpu_online) != NULL)
			online = 1;
		else if (strstr(buf, cpu_offline) != NULL)
			online = 0;
		else
			continue;

		if (online)
			err = sscanf(buf, "online@/devices/system/cpu/cpu%d",
					&cpu);
		else
			err = sscanf(buf, "offline@/devices/system/cpu/cpu%d",
					&cpu);

		pthread_mutex_lock(&hotplug_mutex);

		if (core_status[cpu] != online) {
			core_status[cpu] = get_cpu_status(cpu);
			dbgmsg("Core%d status changed to %d\n", cpu,
					core_status[cpu]);
		}

		pthread_mutex_unlock(&hotplug_mutex);
	}

	return NULL;
}

/* Main mpdecision algorithm thread */
static void *do_mp_decision(void *data)
{
	float rq_depth = 0;
	float time = 0;
	int cpu = 0;
	int cpus_reqd;
	enum CORE_STATUS curr_online_status = CORE_DOWN;
	unsigned int hotplug_disabled;
	int i;
	int num_online = 1;

	while (1) {
		if (num_online == 1 || num_online <= lock_min_cores)
			time = deferrable_sleep(decision_ms);
		else {
			time = decision_ms;
			usleep(decision_ms*1000);
		}

		if (lock_min_cores == num_cpus) {
			dbgmsg("Ignore sample. As all cores are locked for perf");
			continue;
		}

		if (all_cores_thermal_condition()) {
			memset(total_time_up, 0, sizeof(float) * MAX_CPUS);
			memset(total_time_down, 0, sizeof(float) * MAX_CPUS);
			dbgmsg("MP-Decision is disabled in thermal condition. Will reset and continue.\n");
			continue;
		}

		if (read_rq_depth(RQ_DEPTH, &rq_depth) < 0) {
			msg("Error reading run queue depth.\n");
			continue;
		}


		if (!read_hotplug(HOTPLUG_DISABLE, &hotplug_disabled)) {
			if(hotplug_disabled == 1) {
				dbgmsg("Hotplug is disabled. Cannot change the"
						"core status\n");
				continue;
			}
		}

		if (rq_depth < 0) {
			msg("Invalid run queue depth :%f\n", rq_depth);
			continue;
		}
		pthread_mutex_lock(&hotplug_mutex);

		/*
		 * If the timer has been deferred, set runQ to zero
		 */
		if (adjust_average == 1 && time > decision_ms) {
			dbgmsg("Runqueue depth :%f time :%f (deferred)\n", rq_depth, time);
			rq_depth = 0;
		}

		cpus_reqd = get_cpus_required(rq_depth, time);

		if (cpus_reqd < 1)
			goto unlock;

		if (cpus_reqd < lock_min_cores)
			cpus_reqd = lock_min_cores;

		if (cpus_reqd > lock_max_cores && lock_max_cores > 0)
			cpus_reqd = lock_max_cores;

		if (boost_time > 0) {
			if (boost_time < decision_ms)
				boost_time = 0;
			else
				boost_time -= decision_ms;
		}

		num_online = __get_num_online_cpus();
		/* Change core state if there are available cpus */
		if (cpus_reqd != num_online) {
			dbgmsg("num online cores: %d reqd : %d available : %d\n",
			       num_online, cpus_reqd, available_cpus);
			if (cpus_reqd < num_online)
				down_cpus(cpus_reqd);
			else if (num_cpus > num_online)
				up_cpus(cpus_reqd);

			/*reset the number of valid samples in the cpu
			  utilization window upon a cpu state change*/
			valid_in_wndw_up = 0;
			valid_in_wndw_dw = 0;
		}

unlock:
		pthread_mutex_unlock(&hotplug_mutex);
	}

	return NULL;
}

static void init_core_thermal_condition()
{
	int fd = 0;
	char buf[MAX_BUF] = {0};
	fd = open(KTM_NODE, O_RDONLY);
	if (fd >= 0) {
		if (read(fd, buf, sizeof(buf) - 1) == -1)
			msg("Error reading KTMNODE file descriptor");
		else
                        thermal_condition = atoi(&buf[0]);
		dbgmsg("Thermal condition on startup %d", thermal_condition);
		close(fd);
	}
	else
		msg("Bad KTM file descriptor");
}

/* Parse commandline arguments
   * argc - number of arguments
   * argv - argument array */
static void parse_args(int argc, char *argv[])
{
	float Nw, Ns;
	int Tw, Ts;
	int cpu;
	int util;

	while (1) {
		int idx = 0;
		int c = 0;
		static struct option long_options[] = {
			{"Nw", required_argument, NULL, 1},
			{"Tw", required_argument, NULL, 2},
			{"Ns", required_argument, NULL, 3},
			{"Ts", required_argument, NULL, 4},
			{"poll_ms", required_argument, NULL, 5},
			{"decision_ms", required_argument, NULL, 6},
			{"no_sleep", no_argument, NULL, 7},
			{"avg_comp", no_argument, NULL, 8},
			{"debug", no_argument, NULL, 9},
			{"util_h_and", required_argument, NULL, 10},
			{"util_h_or", required_argument, NULL, 11},
			{"util_l_and", required_argument, NULL, 12},
			{"util_l_or", required_argument, NULL, 13},
			{NULL, 0, 0, 0},
		};

		c = getopt_long_only(argc, argv, "", long_options, &idx);
		if (c == -1)
			break;

		switch (c) {
		case 1:
			sscanf(optarg, "%d:%f", &cpu, &Nw);
			if ((cpu > 0 && cpu < num_cpus) && (Nw > 0 && Nw < 4))
				Nw_cpus[cpu] = Nw;
			break;
		case 2:
			sscanf(optarg, "%d:%d", &cpu, &Tw);
			if ((cpu > 0 && cpu < num_cpus) && (Tw > 0 && Tw < 1000000))
				Tw_cpus[cpu] = Tw;
			break;
		case 3:
			sscanf(optarg, "%d:%f", &cpu, &Ns);
			if ((cpu > 0 && cpu < num_cpus) && (Ns > 0 && Ns < 4))
				Ns_cpus[cpu] = Ns;
			break;
		case 4:
			sscanf(optarg, "%d:%d", &cpu, &Ts);
			if ((cpu > 0 && cpu < num_cpus) && (Ts > 0 && Ts < 1000000))
				Ts_cpus[cpu] = Ts;
			break;
		case 5:
			poll_ms = atoi(optarg);
			break;
		case 6:
			decision_ms = atoi(optarg);
			break;
		case 7:
			control_sleep_modes = 1;
			info("OPTION ENABLED: Control sleep modes");
			break;
		case 8:
			adjust_average = 1;
			info("OPTION ENABLED: Adjusting average");
			break;
		case 9:
			debug_output = 1;
			info("OPTION ENABLED: Debug output");
			break;
		case 10:
			sscanf(optarg, "%d:%d", &cpu, &util);
			if ((cpu > 0 && cpu < num_cpus) && (util > 0 && util < (num_cpus*100)))
				util_high_and[cpu] = util;
			break;
		case 11:
			sscanf(optarg, "%d:%d", &cpu, &util);
			if ((cpu > 0 && cpu < num_cpus) && (util > 0 && util < (num_cpus*100)))
				util_high_or[cpu] = util;
			break;
		case 12:
			sscanf(optarg, "%d:%d", &cpu, &util);
			if ((cpu > 0 && cpu < num_cpus) && (util > 0 && util < (num_cpus*100)))
				util_low_and[cpu] = util;
			break;
		case 13:
			sscanf(optarg, "%d:%d", &cpu, &util);
			if ((cpu > 0 && cpu < num_cpus) && (util > 0 && util < (num_cpus*100)))
				util_low_or[cpu] = util;
			break;
		default:
		case '?':
			info("\n MP Decision Options:\n");
			info("--poll_ms --decision_ms --no_sleep --avg_comp --debug\n");
			info("--Nw=[1..3]:[value] --Tw=[1..3]:[value] --Ns=[1..3]:[value] --Ts=[1..3]:[value]\n");
			info("--util_h_and=[1..3]:[value] --util_h_or=[1..3]:[value]\n");
			info("--util_l_and=[1..3]:[value] --util_l_or=[1..3]:[value]\n");
			exit(-1);
			break;
		}
	}

	for_each_cpu(cpu) {
		info("Decision parameters CPU %d: Nw=%f, Tw=%d, Ns=%f, Ts=%d\n",
			cpu, Nw_cpus[cpu], Tw_cpus[cpu], Ns_cpus[cpu], Ts_cpus[cpu]);
		info("util_h_and=%d, util_h_or=%d, util_low=%d\n",
			util_high_and[cpu], util_high_or[cpu], util_low_or[cpu]);
	}
	info("Decision parameters: poll_ms: %d decision_ms: %d\n", poll_ms, decision_ms);
}

/* Reads from CPU_SYSFS folder to system CPUs count
   Return value - number of cpus, -1 if error */
static unsigned int get_num_cpus()
{
	DIR *tdir = NULL;
	struct dirent *tdirent = NULL;
	int ncpus = 0;
	char name[MAX_PATH] = {0};
	char cwd[MAX_PATH] = {0};
	int cpu = 0;
	char buf[50];
	struct dirent *cpu_dirent;

	if (!getcwd(cwd, sizeof(cwd)))
		return -1;

	chdir(CPU_SYSFS); /* Change dir to read the entries. Doesnt work
				 otherwise */
	tdir = opendir(CPU_SYSFS);
	if (!tdir) {
		msg("Unable to open %s\n", CPU_SYSFS);
		return -1;
	}

	while ((cpu_dirent = readdir(tdir))) {
		if (strstr(cpu_dirent->d_name, "cpu") == NULL)
			continue;
		if (!sscanf(cpu_dirent->d_name, "cpu%d", &cpu))
			continue;
		ncpus++;
	}

	closedir(tdir);
	chdir(cwd); /* Restore current working dir */

	dbgmsg("Number of cores %d\n", ncpus);

	return ncpus;
}

static void close_file_descriptors()
{
	int cpu;
	close(cpu_load_normalized_fd);
	close(rq_depth_fd);
	close(def_timer_fd);
	close(hotplug_disabled_fd);
	close(pm_qos_fd);
	for_each_cpu(cpu) {
		close(cpu_fd[cpu]);
	}
	close(ktm_node_fd);
}

static int open_file_descriptors()
{
	char str[MAX_BUF * 3] = {0};
	int cpu;
	rq_depth_fd = open(RQ_DEPTH, O_RDONLY);
	if (rq_depth_fd < 0) {
		msg("Unable to open rq_depth file\n");
		goto failure;
	}

	cpu_load_normalized_fd = open(CPU_LOAD_NORMALIZED, O_RDONLY);
	if (cpu_load_normalized_fd < 0) {
		msg("Unable to open cpu load normalized file\n");
		goto failure;
	}

	def_timer_fd = open(DEF_TIMER_MS, O_RDONLY);
	if (def_timer_fd < 0) {
		msg("Unable to open deferrable timer file\n");
		goto failure;
	}

	hotplug_disabled_fd = open(HOTPLUG_DISABLE, O_RDONLY);
	if (hotplug_disabled_fd < 0) {
		msg("Unable to open deferrable timer file\n");
		goto failure;
	}


	for_each_cpu(cpu) {
		snprintf(str, MAX_BUF * 3, "/sys/devices/system/cpu/cpu%d/online", cpu);
		cpu_fd[cpu] = open(str, O_RDONLY);
		if (cpu_fd[cpu] < 0) {
			msg("Unable to open core%d file\n", cpu);
			goto failure;
		}
		str[0] = '\0';
	}

	pm_qos_fd = open(PM_QOS, O_WRONLY);
	if (pm_qos_fd < 0) {
		msg("Unable to open pm_qos file");
		goto failure;
	}

	ktm_node_fd = open(KTM_NODE, O_RDONLY);
	if (ktm_node_fd < 0) {
		msg("Unable to open ktm_node file\n");
	}

	return 0;

failure: close_file_descriptors();
	return -1;
}

/* Main entry to mpdecision
   * argc - number of command-line arguments
   * argv - command-line argument array
   Return value - 0 if successful, negative otherwise */
int main(int argc, char *argv[])
{
	int ret = 0;
	int cpu = 1;
	unsigned int cpu_set = 1; /* Run on core0 */
	unsigned int hotplug_disabled = 0;

#ifdef MPCTL_SERVER
	mpctl_server_init();
#endif

	ret = syscall(__NR_sched_setaffinity, 0, sizeof(cpu_set), &cpu_set);
	if (ret < 0) {
		msg("Cannot set cpu affinity: %s\n", strerror(-ret));
		return ret;
	}

	num_cpus = get_num_cpus();
	if (num_cpus < 2 || num_cpus > MAX_CPUS) {
		msg("CPUs detected: %d, not supported. Exiting.\n", num_cpus);
		return -1;
	}

	/* Priority */
	setpriority(PRIO_PROCESS, getpid(), -7);

	/* Init MP-Decision parameters */
	for_each_cpu(cpu) {
		core_status[cpu] = CORE_UP;
		Nw_cpus[cpu] = DEF_Nw[cpu];
		Tw_cpus[cpu] = DEF_Tw[cpu];
		Ns_cpus[cpu] = DEF_Ns[cpu];
		Ts_cpus[cpu] = DEF_Ts[cpu];
		util_high_and[cpu] = DEF_util_high_and[cpu];
		util_high_or[cpu] = DEF_util_high_or[cpu];
		util_low_and[cpu] = DEF_util_low_and[cpu];
		util_low_or[cpu] = DEF_util_low_or[cpu];
		max_up_wndw_size = DEF_max_up_wndw_size;
		max_dw_wndw_size = DEF_max_dw_wndw_size;
	}

	/* Command line overrides */
	parse_args(argc, argv);

	/* Enable kernel calculation of rq depth */
	write_file_uint32(RQ_POLL_MS, poll_ms);

	ret = open_file_descriptors();
	if (ret < 0)
		return -1;

	if (read_hotplug(HOTPLUG_DISABLE, &hotplug_disabled))
		hotplug_disabled == 0;

	init_core_thermal_condition();
	mpdecision_get_core_mapping(core_pref_order);
	thermally_restricted_cores();
	/* Bring all the secondary cores down */
	for (cpu = 1; cpu < num_cpus; cpu++) {
		pthread_mutex_lock(&hotplug_mutex);
		if (CORE_UP == get_cpu_status(cpu)) {
			if (!hotplug_disabled) {
				ret = __cpu_up(cpu, CORE_DOWN);
				if (ret)
					msg("Unable to bring cpu%d"
						"down on start\n", cpu);
			}
		} else {
			core_status[cpu] = CORE_DOWN;
		}
		pthread_mutex_unlock(&hotplug_mutex);
	}

	pthread_create(&mp_decision, NULL, do_mp_decision, NULL);
	pthread_create(&thermal_monitor, NULL, do_thermal_monitoring, NULL);
	pthread_create(&hotplug_monitor, NULL, do_hotplug_monitoring, NULL);

	pthread_join(mp_decision, NULL);
	pthread_join(thermal_monitor, NULL);
	pthread_join(hotplug_monitor, NULL);

	close_file_descriptors();

#ifdef MPCTL_SERVER
	mpctl_server_exit();
#endif
	return ret;
}
