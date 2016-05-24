/******************************************************************************
 *
 *      D E C I S I O N - C O R E S . C
 *
 * GENERAL DESCRIPTION
 *      Contains target related core selection algorithm
 Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
 ******************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "decision.h"

#define SYSFS_PLATFORMID   "/sys/devices/soc0/soc_id"
#define MAX_SOC_INFO_NAME_LEN (15)
#define NUM_OF_MAPS 1
#define ARRAY_SIZE(x) (int)(sizeof(x)/sizeof(x[0]))

enum map_types {FLOOR_MAP = 0,
		IDDQ_MAP = 1,
		CORE_PERF_MAP = 2};

struct core_map_info {
	int cpu;
	float confidence_lvl; //in percentage
};

enum msm_id_types {
	NOT_DEFINED = 0,
	MSM_8974 = 1};

struct msm_soc_type {
	enum msm_id_types msm_id;
	int soc_id;
};

static struct msm_soc_type msm_soc_table[] = {
	{MSM_8974, 126},
	{MSM_8974, 184},
	{MSM_8974, 185},
	{MSM_8974, 186},
	{MSM_8974, 208},
	{MSM_8974, 211},
	{MSM_8974, 214},
	{MSM_8974, 217},
	{MSM_8974, 209},
	{MSM_8974, 212},
	{MSM_8974, 215},
	{MSM_8974, 218},
	{MSM_8974, 194},
	{MSM_8974, 210},
	{MSM_8974, 213},
	{MSM_8974, 216}
};

static struct core_map_info msm8974_map[NUM_OF_MAPS][MAX_CPUS] = {
        {{0, .25}, {3, .25}, {2, .25}, {1, .25}}}; //Floor map
static struct core_map_info default_map[NUM_OF_MAPS][MAX_CPUS] = {
        {{0, .25}, {1, .25}, {2, .25}, {3, .25}}}; //Floor map
static struct core_map_info map_order[NUM_OF_MAPS][MAX_CPUS];
static int map_weight[3] = {1, 2, 3};
float coremap[MAX_CPUS][MAX_CPUS] = {{0}};
static int core_mapped_flag[MAX_CPUS] = {0};

enum msm_id_types get_msm_id(void)
{
	static enum msm_id_types msm_id = NOT_DEFINED;
	static uint8_t msm_id_init;
	int fd;
	int idx;
	int soc_id;
	char buf[MAX_SOC_INFO_NAME_LEN];

	if (msm_id != NOT_DEFINED)
		return msm_id;

	fd = open(SYSFS_PLATFORMID, O_RDONLY);
	if (fd < 0) {
		dbgmsg("Error opening soc platform node\n");
		goto failed;
	}

	if (read(fd, buf, MAX_SOC_INFO_NAME_LEN) < 0) {
		dbgmsg("Error reading soc platform node\n");
		goto failed;
	}
	close(fd);

	soc_id = atoi(buf);

	for (idx = 0; idx < ARRAY_SIZE(msm_soc_table) ; idx++) {
		if (soc_id == msm_soc_table[idx].soc_id) {
			msm_id = msm_soc_table[idx].msm_id;
			break;
		}
	}

	if (!msm_id)
		dbgmsg("Unknown target identified with soc id %d\n", soc_id);

failed:
	msm_id_init = 1;
	return msm_id;
}

static void get_target_map(void)
{
	struct core_map_info *ptr = NULL;
	enum msm_id_types msm_id = get_msm_id();

	switch (msm_id) {
	case MSM_8974:
		ptr = &msm8974_map[0][0];
		break;
	default:
		ptr = &default_map[0][0];
		break;
	}

	memcpy(map_order, ptr, NUM_OF_MAPS * MAX_CPUS *
			sizeof(struct core_map_info));
}

static void restrict_core0_hotplug(int *aggregated_order)
{
	int i, j, idx;

	for (i = 0; i < MAX_CPUS; i++) {
		if (aggregated_order[i] == 0) {
			for (j = i; j > 0 ; j--)
				aggregated_order[j] = aggregated_order[j-1];
			aggregated_order[0] = 0;
			break;
		}
	}
}

static void clear_row(int row)
{
	int i;

	for (i = 0; i < MAX_CPUS; i++)
		 coremap[row][i] = 0;
}

static void clear_column(int column)
{
	int i;

	for (i = 0; i < MAX_CPUS; i++)
		coremap[i][column] = 0;
}

static void print_confidence_matrix()
{
	int i, j;

	for (i = 0; i < MAX_CPUS; i++) {
		dbgmsg("Core%d: ", i);
		for (j = 0; j < MAX_CPUS; j++)
			dbgmsg(" %f", coremap[i][j]);
		dbgmsg("\n");
	}
}

static void map_core_and_position(int *aggregated_order)
{
	float max = 0;
	int i, j;
	int core = -1;
	int pos = -1;

	for (i = 0; i < MAX_CPUS; i++)
		for (j = 0; j < MAX_CPUS; j++)
			if (max < coremap[i][j]) {
				max = coremap[i][j];
				core = i;
				pos = j;
			}

	if (core != -1 && pos != -1)
		goto core_selected;

	for (i = 0; i < MAX_CPUS; i++) {
		/* Check for unmapped cores */
		if (core_mapped_flag[i])
			continue;

		core = i;
		for (j = 0 ; j < MAX_CPUS; j++) {
			/* *
			 * Find available position for these
			 * unmapped cores. The core is going
			 * to fill the first available
			 * position.
			 * */
			if(aggregated_order[j] == -1) {
				pos = j;
				break;
			}
		}
	}

	/* If all the cores and positions are filled, exit the function */
	if (core == -1 || pos == -1)
		goto exit;

core_selected:
	dbgmsg("Core %d selected at pos %d\n", core, pos);
	/* Update the physical cores logical position */
	aggregated_order[pos] = core;
	/* Mark the core as mapped */
	core_mapped_flag[core] = 1;
	clear_row(core);
	clear_column(pos);
	print_confidence_matrix();
	map_core_and_position(aggregated_order);

exit:
	return;
}

void mpdecision_get_core_mapping(int *aggregated_order)
{
	int i, j;
	float sortedmap[MAX_CPUS] = {0};
	int unused[MAX_CPUS] = {0};

	get_target_map();
	for (i = 0; i < NUM_OF_MAPS; i++) {
		for (j = 0; j < MAX_CPUS; j++)
			coremap[map_order[i][j].cpu][j] += (float)map_weight[i]
					* map_order[i][j].confidence_lvl;
	}

	print_confidence_matrix();

	memset(core_mapped_flag, 0, sizeof(int) * MAX_CPUS);
	for (j = 0; j < MAX_CPUS; j++)
		aggregated_order[j] = -1;

	map_core_and_position(aggregated_order);
	restrict_core0_hotplug(aggregated_order);
}
