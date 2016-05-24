/*===========================================================================

Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.

===========================================================================*/

#include "thermal.h"
#include "thermal_config.h"
#include "pid_algorithm.h"

#define KHZ_PER_C (5000)
#define SET_POINT (95000)
#define SET_POINT_CLR (75000)
#define POP_MEM_SET_POINT (80000)
#define POP_MEM_SET_POINT_CLR (65000)
#define K_P (1.25)
#define K_I (0.8)
#define K_D (0.5)
#define PID_SAMPLE_PERIOD (1000)
#define I_SAMPLES (10)
#define ERR_WEIGHT (1.0)

#define PID_HYBRID_PERIOD (1000)

static struct setting_info pid_cfgs_8974[] =
{
	{
		.desc = "PID-CPU0",
		.algo_type = PID_ALGO_TYPE,
		.disable = 1,
		.data.pid =
		{
			.sensor = "cpu0",
			.device = "cpu0",
			.sampling_period_ms = PID_SAMPLE_PERIOD,
			.i_samples = I_SAMPLES,
			.set_point = SET_POINT,
			.set_point_clr = SET_POINT_CLR,
			.units_per_C = KHZ_PER_C,
			.p_const = K_P,
			.i_const = K_I,
			.d_const = K_D,
			.err_weight = ERR_WEIGHT,
		},
	},
	{
		.desc = "PID-CPU1",
		.algo_type = PID_ALGO_TYPE,
		.disable = 1,
		.data.pid =
		{
			.sensor = "cpu1",
			.device = "cpu1",
			.sampling_period_ms = PID_SAMPLE_PERIOD,
			.i_samples = I_SAMPLES,
			.set_point = SET_POINT,
			.set_point_clr = SET_POINT_CLR,
			.units_per_C = KHZ_PER_C,
			.p_const = K_P,
			.i_const = K_I,
			.d_const = K_D,
			.err_weight = ERR_WEIGHT,
		},
	},
	{
		.desc = "PID-CPU2",
		.algo_type = PID_ALGO_TYPE,
		.disable = 1,
		.data.pid =
		{
			.sensor = "cpu2",
			.device = "cpu2",
			.sampling_period_ms = PID_SAMPLE_PERIOD,
			.i_samples = I_SAMPLES,
			.set_point = SET_POINT,
			.set_point_clr = SET_POINT_CLR,
			.units_per_C = KHZ_PER_C,
			.p_const = K_P,
			.i_const = K_I,
			.d_const = K_D,
			.err_weight = ERR_WEIGHT,
		},
	},
	{
		.desc = "PID-CPU3",
		.algo_type = PID_ALGO_TYPE,
		.disable = 1,
		.data.pid =
		{
			.sensor = "cpu3",
			.device = "cpu3",
			.sampling_period_ms = PID_SAMPLE_PERIOD,
			.i_samples = I_SAMPLES,
			.set_point = SET_POINT,
			.set_point_clr = SET_POINT_CLR,
			.units_per_C = KHZ_PER_C,
			.p_const = K_P,
			.i_const = K_I,
			.d_const = K_D,
			.err_weight = ERR_WEIGHT,
		},
	},
	{
		.desc = "PID-POPMEM",
		.algo_type = PID_ALGO_TYPE,
		.disable = 1,
		.data.pid =
		{
			.sensor = "pop_mem",
			.device = "cpu",
			.sampling_period_ms = PID_SAMPLE_PERIOD,
			.i_samples = I_SAMPLES,
			.set_point = POP_MEM_SET_POINT,
			.set_point_clr = POP_MEM_SET_POINT_CLR,
			.units_per_C = KHZ_PER_C,
			.p_const = K_P,
			.i_const = K_I,
			.d_const = K_D,
			.err_weight = ERR_WEIGHT,
		},
	},
};

static struct setting_info pid_cfgs_8226[] =
{
	{
		.desc = "PID-POPMEM",
		.algo_type = PID_ALGO_TYPE,
		.disable = 1,
		.data.pid =
		{
			.sensor = "pop_mem",
			.device = "cpu",
			.sampling_period_ms = PID_SAMPLE_PERIOD,
			.i_samples = I_SAMPLES,
			.set_point = POP_MEM_SET_POINT,
			.set_point_clr = POP_MEM_SET_POINT_CLR,
			.units_per_C = KHZ_PER_C,
			.p_const = K_P,
			.i_const = K_I,
			.d_const = K_D,
			.err_weight = ERR_WEIGHT,
		},
	},
};

static struct setting_info pid_cfgs_8610[] =
{
	{
		.desc = "PID-POPMEM",
		.algo_type = PID_ALGO_TYPE,
		.disable = 1,
		.data.pid =
		{
			.sensor = "pop_mem",
			.device = "cpu",
			.sampling_period_ms = PID_SAMPLE_PERIOD,
			.i_samples = I_SAMPLES,
			.set_point = POP_MEM_SET_POINT,
			.set_point_clr = POP_MEM_SET_POINT_CLR,
			.units_per_C = KHZ_PER_C,
			.p_const = K_P,
			.i_const = K_I,
			.d_const = K_D,
			.err_weight = ERR_WEIGHT,
		},
	},
};

void pid_init_data(struct thermal_setting_t *setting)
{
	int i, arr_size;
	struct setting_info *cfg;

	switch (therm_get_msm_id()) {
		case THERM_MSM_8974:
		case THERM_MSM_8084:
		case THERM_MSM_8x62:
		case THERM_MSM_8974PRO_AA:
		case THERM_MSM_8974PRO_AB:
		case THERM_MSM_8974PRO_AC:
			cfg = pid_cfgs_8974;
			arr_size = ARRAY_SIZE(pid_cfgs_8974);
			break;
		case THERM_MSM_8226:
		case THERM_MSM_8926:
			cfg = pid_cfgs_8226;
			arr_size = ARRAY_SIZE(pid_cfgs_8226);
			break;
		case THERM_MSM_8610:
			cfg = pid_cfgs_8610;
			arr_size = ARRAY_SIZE(pid_cfgs_8610);
			break;
		default:
			msg("%s: ERROR Uknown device\n", __func__);
			/* Better to have something in place even if it's wrong. */
			cfg = pid_cfgs_8974;
			arr_size = ARRAY_SIZE(pid_cfgs_8974);
			break;
	}
	update_config_with_default_temp_limit(cfg, arr_size, PID_ALGO_TYPE);

	for (i = 0; i < arr_size; i++)
		add_setting(setting, &cfg[i]);
}
