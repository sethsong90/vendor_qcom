/*===========================================================================

Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.

===========================================================================*/

#include "thermal.h"
#include "thermal_config.h"
#include "ss_algorithm.h"

static struct setting_info ss_cfgs_8974[] =
{
	{
		.desc = "SS-CPU0",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "cpu0",
			.device = "cpu",
			.sampling_period_ms = 65,
			.set_point = 90000,
			.set_point_clr = 55000,
		},
	},
	{
		.desc = "SS-CPU1",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "cpu1",
			.device = "cpu",
			.sampling_period_ms = 65,
			.set_point = 90000,
			.set_point_clr = 55000,
		},
	},
	{
		.desc = "SS-CPU2",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "cpu2",
			.device = "cpu",
			.sampling_period_ms = 65,
			.set_point = 90000,
			.set_point_clr = 55000,
		},
	},
	{
		.desc = "SS-CPU3",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "cpu3",
			.device = "cpu",
			.sampling_period_ms = 65,
			.set_point = 90000,
			.set_point_clr = 55000,
		},
	},
	{
		.desc = "SS-POPMEM",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "pop_mem",
			.device = "cpu",
			.sampling_period_ms = 65,
			.set_point = 80000,
			.set_point_clr = 55000,
			.time_constant = 16,
		},
	},
};

static struct setting_info ss_cfgs_8974_pro_ac[] =
{
	{
		.desc = "SS-CPU0",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "cpu0",
			.device = "cpu",
			.sampling_period_ms = 65,
			.set_point = 90000,
			.set_point_clr = 55000,
		},
	},
	{
		.desc = "SS-CPU1",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "cpu1",
			.device = "cpu",
			.sampling_period_ms = 65,
			.set_point = 90000,
			.set_point_clr = 55000,
		},
	},
	{
		.desc = "SS-CPU2",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "cpu2",
			.device = "cpu",
			.sampling_period_ms = 65,
			.set_point = 90000,
			.set_point_clr = 55000,
		},
	},
	{
		.desc = "SS-CPU3",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "cpu3",
			.device = "cpu",
			.sampling_period_ms = 65,
			.set_point = 90000,
			.set_point_clr = 55000,
		},
	},
	{
		.desc = "SS-POPMEM",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "pop_mem",
			.device = "cpu",
			.sampling_period_ms = 65,
			.set_point = 80000,
			.set_point_clr = 55000,
			.time_constant = 16,
		},
	},
	{
		.desc = "SS-GPU",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "tsens_tz_sensor10",
			.device = "gpu",
			.sampling_period_ms = 250,
			.set_point = 100000,
			.set_point_clr = 65000,
		},
	},
};

static struct setting_info ss_cfgs_8226_v1[] =
{
	{
		.desc = "SS-CPU2-3",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "cpu2-3",
			.device = "cpu",
			.sampling_period_ms = 1000,
			.set_point = 90000,
			.set_point_clr = 55000,
		},
	},
	{
		.desc = "SS-POPMEM",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "pop_mem",
			.device = "cpu",
			.sampling_period_ms = 1000,
			.set_point = 50000,
			.set_point_clr = 45000,
			.time_constant = 5,
		},
	},
};

static struct setting_info ss_cfgs_8226[] =
{
	{
		.desc = "SS-CPU0-1",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "cpu0-1",
			.device = "cpu",
			.sampling_period_ms = 250,
			.set_point = 90000,
			.set_point_clr = 55000,
		},
	},
	{
		.desc = "SS-CPU2-3",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "cpu2-3",
			.device = "cpu",
			.sampling_period_ms = 250,
			.set_point = 90000,
			.set_point_clr = 55000,
		},
	},
	{
		.desc = "SS-POPMEM",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "pop_mem",
			.device = "cpu",
			.sampling_period_ms = 1000,
			.set_point = 60000,
			.set_point_clr = 45000,
			.time_constant = 2,
		},
	},
};

static struct setting_info ss_cfgs_8610[] =
{
	{
		.desc = "SS-CPU0-1-2-3",
		.algo_type = SS_ALGO_TYPE,
		.data.ss =
		{
			.sensor = "cpu0-1-2-3",
			.device = "cpu",
			.sampling_period_ms = 250,
			.set_point = 80000,
			.set_point_clr = 65000,
		},
	},
};

void ss_init_data(struct thermal_setting_t *setting)
{
	int i, arr_size;
	struct setting_info *cfg;

	switch (therm_get_msm_id()) {
		case THERM_MSM_8974:
		case THERM_MSM_8x62:
		case THERM_MSM_8974PRO_AA:
		case THERM_MSM_8974PRO_AB:
			cfg = ss_cfgs_8974;
			arr_size = ARRAY_SIZE(ss_cfgs_8974);
			break;
		case THERM_MSM_8974PRO_AC:
		case THERM_MSM_8084:
			cfg = ss_cfgs_8974_pro_ac;
			arr_size = ARRAY_SIZE(ss_cfgs_8974_pro_ac);
			break;
		case THERM_MSM_8226:
			switch (therm_get_msm_version()) {
				case THERM_VERSION_V1:
					cfg = ss_cfgs_8226_v1;
					arr_size = ARRAY_SIZE(ss_cfgs_8226_v1);
					break;
				default:
					cfg = ss_cfgs_8226;
					arr_size = ARRAY_SIZE(ss_cfgs_8226);
					break;
			}
			break;
		case THERM_MSM_8926:
			cfg = ss_cfgs_8226;
			arr_size = ARRAY_SIZE(ss_cfgs_8226);
			break;
		case THERM_MSM_8610:
			cfg = ss_cfgs_8610;
			arr_size = ARRAY_SIZE(ss_cfgs_8610);
			break;
		default:
			msg("%s: ERROR Uknown device\n", __func__);
			/* Better to have something in place even if it's wrong. */
			cfg = ss_cfgs_8974;
			arr_size = ARRAY_SIZE(ss_cfgs_8974);
			break;
	}

	update_config_with_default_temp_limit(cfg, arr_size, SS_ALGO_TYPE);

	for (i = 0; i < arr_size; i++)
		add_setting(setting, &cfg[i]);
}
