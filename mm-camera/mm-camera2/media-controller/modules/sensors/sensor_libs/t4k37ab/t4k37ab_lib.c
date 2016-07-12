/* t4k37ab_lib.c
 *
 * Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <stdio.h>
#include "sensor_lib.h"

#define SENSOR_MODEL_NO_T4K37AB "t4k37ab"
#define T4K37AB_LOAD_CHROMATIX(n) \
  "libchromatix_"SENSOR_MODEL_NO_T4K37AB"_"#n".so"


static sensor_lib_t sensor_lib_ptr;

static sensor_output_t sensor_output = {
  .output_format = SENSOR_BAYER,
  .connection_mode = SENSOR_MIPI_CSI,
  .raw_output = SENSOR_10_BIT_DIRECT,
};

static struct msm_sensor_output_reg_addr_t output_reg_addr = {
  .x_output = 0x034C,
  .y_output = 0x034E,
  .line_length_pclk = 0x0342,
  .frame_length_lines = 0x0340,
};

static struct msm_sensor_exp_gain_info_t exp_gain_info = {
  .coarse_int_time_addr = 0x0202,
  .global_gain_addr = 0x0204,
  .vert_offset = 6,
};

static sensor_aec_data_t aec_info = {
  .max_gain = 8.0,
  .max_linecount = 37920,
};

static sensor_lens_info_t default_lens_info = {
  .focal_length = 4.6,
  .pix_size = 1.12,
  .f_number = 2.2,
  .total_f_dist = 1.97,
  .hor_view_angle = 62.7,
  .ver_view_angle = 48.7,
};

#ifndef VFE_40
static struct csi_lane_params_t csi_lane_params = {
  .csi_lane_assign = 0xE4,
  .csi_lane_mask = 0xf,
  .csi_if = 1,
  .csid_core = {0},
  .csi_phy_sel = 0,
};
#else
static struct csi_lane_params_t csi_lane_params = {
  .csi_lane_assign = 0x4320,
  .csi_lane_mask = 0x1f,
  .csi_if = 1,
  .csid_core = {0},
  .csi_phy_sel = 0,
};
#endif

static struct msm_camera_i2c_reg_array init_reg_array[] = {
  {0x101, 0},
  {0x103, 0},
  {0x104, 0},
  {0x105, 1},
  {0x106, 0},
  {0x110, 0},
  {0x111, 2},
  {0x112, 0xA},
  {0x113, 0xA},
  {0x114, 3},
  {0x115, 0x30},
  {0x117, 0x32},
  {0x130, 0x18},
  {0x131, 0},
  {0x141, 0},
  {0x142, 0},
  {0x143, 0},
  {0x202, 3},
  {0x203, 0xA6},
  {0x204, 0},
  {0x205, 0x3D},
  {0x210, 1},
  {0x211, 0},
  {0x212, 1},
  {0x213, 0},
  {0x214, 1},
  {0x215, 0},
  {0x216, 1},
  {0x217, 0},
  {0x230, 0},
  {0x232, 4},
  {0x234, 0},
  {0x235, 0x19},
  {0x301, 1},
  {0x303, 0xA},
  {0x305, 7},
  {0x306, 0},
  {0x307, 0xD8},
  {0x30B, 1},
  {0x30D, 3},
  {0x30E, 0},
  {0x30F, 0x6C},
  {0x310, 0},
  {0x340, 6},
  {0x341, 0x2C},
  {0x342, 0x15},
  {0x343, 0x20},
  {0x344, 0},
  {0x346, 0},
  {0x347, 0},
  {0x34A, 0xC},
  {0x34B, 0x2F},
  {0x34C, 8},
  {0x34D, 0x38},
  {0x34E, 6},
  {0x34F, 0x18},
  {0x401, 0},
  {0x403, 0},
  {0x404, 0x10},
  {0x408, 0},
  {0x409, 0},
  {0x40A, 0},
  {0x40B, 0},
  {0x40C, 0x10},
  {0x40D, 0x70},
  {0x40E, 0xC},
  {0x40F, 0x30},
  {0x601, 0},
  {0x602, 2},
  {0x603, 0xC0},
  {0x604, 2},
  {0x605, 0xC0},
  {0x606, 2},
  {0x607, 0xC0},
  {0x608, 2},
  {0x609, 0xC0},
  {0x60A, 0},
  {0x60B, 0},
  {0x60C, 0},
  {0x60D, 0},
  {0x60E, 0},
  {0x60F, 0},
  {0x610, 0},
  {0x611, 0},
  {0x800, 0x88},
  {0x801, 0x38},
  {0x802, 0x78},
  {0x803, 0x48},
  {0x804, 0x48},
  {0x805, 0x40},
  {0x806, 0},
  {0x807, 0x48},
  {0x808, 1},
  {0x820, 0xA},
  {0x821, 0x20},
  {0x822, 0},
  {0x823, 0},
  {0x900, 0},
  {0x901, 0},
  {0x902, 0},
  {0xA05, 1},
  {0xA06, 1},
  {0xA07, 0x98},
  {0xA0A, 1},
  {0xA0B, 0x98},
  {0xC00, 0},
  {0xC01, 0},
  {0xC02, 0},
  {0xC03, 0},
  {0xC04, 0},
  {0xC05, 0x32},
  {0xC06, 0},
  {0xC07, 0x10},
  {0xC08, 0},
  {0xC09, 0x49},
  {0xC0A, 1},
  {0xC0B, 0x68},
  {0xC0C, 0},
  {0xC0D, 0x34},
  {0xC0E, 0},
  {0xC0F, 0x40},
  {0xC12, 1},
  {0xC14, 0},
  {0xC15, 1},
  {0xC16, 0},
  {0xC17, 0x20},
  {0xC18, 0},
  {0xC19, 0x40},
  {0xC1A, 0},
  {0xC1B, 0},
  {0xC26, 0},
  {0xC27, 4},
  {0xC28, 0},
  {0xC29, 4},
  {0xC2A, 1},
  {0xC2B, 1},
  {0xC2C, 0},
  {0xC2D, 4},
  {0xC2E, 0},
  {0xC2F, 4},
  {0xF00, 0},
  {0xF01, 1},
  {0xF02, 1},
  {0xF03, 0x40},
  {0xF04, 0},
  {0xF05, 0x40},
  {0xF06, 1},
  {0xF07, 0},
  {0xF08, 1},
  {0xF09, 0},
  {0xF0A, 1},
  {0xF0B, 0},
  {0xF0C, 1},
  {0xF0D, 0},
  {0xF0E, 0},
  {0xF0F, 1},
  {0xF10, 0x50},
  {0xF11, 0},
  {0xF12, 0x50},
  {0xF13, 1},
  {0xF14, 0},
  {0xF15, 1},
  {0xF16, 0},
  {0xF17, 1},
  {0xF18, 0},
  {0xF19, 1},
  {0xF1A, 0},
  {0xF1B, 0},
  {0xF1C, 1},
  {0xF1D, 0x60},
  {0xF1E, 0},
  {0xF1F, 0x60},
  {0xF20, 1},
  {0xF21, 0},
  {0xF22, 1},
  {0xF23, 0},
  {0xF24, 1},
  {0xF25, 0},
  {0xF26, 1},
  {0xF27, 0},
  {0xF28, 0},
  {0x1101, 0},
  {0x1143, 0},
  {0x1202, 0},
  {0x1203, 0x19},
  {0x1204, 0},
  {0x1205, 0x40},
  {0x1210, 1},
  {0x1211, 0},
  {0x1212, 1},
  {0x1213, 0},
  {0x1214, 1},
  {0x1215, 0},
  {0x1216, 1},
  {0x1217, 0},
  {0x1230, 0},
  {0x1232, 4},
  {0x1234, 0},
  {0x1235, 0x19},
  {0x1340, 0xC},
  {0x1341, 0x80},
  {0x1342, 0x15},
  {0x1343, 0xE0},
  {0x1344, 0},
  {0x1346, 0},
  {0x1347, 0},
  {0x134A, 0xC},
  {0x134B, 0x2F},
  {0x134C, 0x10},
  {0x134D, 0x70},
  {0x134E, 0xC},
  {0x134F, 0x30},
  {0x1401, 0},
  {0x1403, 0},
  {0x1404, 0x10},
  {0x1408, 0},
  {0x1409, 0},
  {0x140A, 0},
  {0x140B, 0},
  {0x140C, 0x10},
  {0x140D, 0x70},
  {0x140E, 0xC},
  {0x140F, 0x30},
  {0x1601, 0},
  {0x1602, 2},
  {0x1603, 0xC0},
  {0x1604, 2},
  {0x1605, 0xC0},
  {0x1606, 2},
  {0x1607, 0xC0},
  {0x1608, 2},
  {0x1609, 0xC0},
  {0x160A, 0},
  {0x160B, 0},
  {0x160C, 0},
  {0x160D, 0},
  {0x160E, 0},
  {0x160F, 0},
  {0x1610, 0},
  {0x1611, 0},
  {0x1900, 0},
  {0x1901, 0},
  {0x1902, 0},
  {0x3000, 0x30},
  {0x3001, 0x14},
  {0x3002, 0xE},
  {0x3007, 8},
  {0x3008, 0x80},
  {0x3009, 0xF},
  {0x300A, 0},
  {0x300B, 8},
  {0x300C, 0x20},
  {0x300D, 0},
  {0x300E, 0x1C},
  {0x300F, 5},
  {0x3010, 0},
  {0x3011, 0x46},
  {0x3012, 0},
  {0x3013, 0x18},
  {0x3014, 5},
  {0x3015, 0},
  {0x3016, 0x4E},
  {0x3017, 0},
  {0x3018, 0x4E},
  {0x3019, 0},
  {0x301A, 0x44},
  {0x301B, 0x44},
  {0x301C, 0x88},
  {0x301D, 0},
  {0x301E, 0},
  {0x301F, 0},
  {0x3020, 0},
  {0x3021, 0xA},
  {0x3023, 8},
  {0x3024, 2},
  {0x3025, 4},
  {0x3027, 0},
  {0x3028, 2},
  {0x3029, 0xC},
  {0x302A, 0},
  {0x302B, 6},
  {0x302C, 0x18},
  {0x302D, 0x18},
  {0x302E, 0xC},
  {0x302F, 6},
  {0x3030, 0},
  {0x3031, 0x80},
  {0x3032, 0xFC},
  {0x3033, 0},
  {0x3034, 0},
  {0x3035, 4},
  {0x3036, 0},
  {0x3037, 0xD},
  {0x3038, 0xD},
  {0x3039, 0xD},
  {0x303A, 0xD},
  {0x303B, 0xF},
  {0x303C, 3},
  {0x303D, 3},
  {0x303E, 0},
  {0x303F, 0xB},
  {0x3040, 0x15},
  {0x3042, 0xB},
  {0x3043, 0x15},
  {0x3044, 0},
  {0x3045, 0x14},
  {0x3046, 0},
  {0x3047, 0x28},
  {0x3048, 0xE},
  {0x3049, 0x20},
  {0x304A, 9},
  {0x304B, 0x1F},
  {0x304C, 9},
  {0x304D, 0x21},
  {0x304E, 0},
  {0x304F, 0x29},
  {0x3050, 0x1D},
  {0x3051, 0},
  {0x3052, 0},
  {0x3053, 0xE0},
  {0x3055, 0},
  {0x3056, 0},
  {0x3057, 0},
  {0x3059, 1},
  {0x305A, 0xA},
  {0x305C, 1},
  {0x305D, 0x10},
  {0x305E, 6},
  {0x305F, 4},
  {0x3061, 1},
  {0x3062, 1},
  {0x3063, 0x60},
  {0x3065, 2},
  {0x3066, 1},
  {0x3067, 3},
  {0x3068, 0x24},
  {0x306A, 0x80},
  {0x306B, 8},
  {0x306E, 0x10},
  {0x306F, 2},
  {0x3070, 0xE},
  {0x3071, 0},
  {0x3072, 0},
  {0x3073, 0x26},
  {0x3074, 0x1A},
  {0x3075, 0xF},
  {0x3076, 3},
  {0x3077, 1},
  {0x3078, 1},
  {0x3079, 1},
  {0x307A, 1},
  {0x307B, 4},
  {0x307C, 2},
  {0x307D, 1},
  {0x307E, 2},
  {0x3080, 0},
  {0x3081, 0},
  {0x3082, 0},
  {0x3085, 0},
  {0x3086, 0},
  {0x3087, 0},
  {0x3089, 0xA0},
  {0x308A, 0xA0},
  {0x308B, 0xA0},
  {0x308C, 0xA0},
  {0x308D, 3},
  {0x308E, 0x20},
  {0x3090, 4},
  {0x3091, 4},
  {0x3092, 1},
  {0x3093, 1},
  {0x3094, 0x60},
  {0x3095, 0xE},
  {0x3096, 0x75},
  {0x3097, 0x7E},
  {0x3098, 0x20},
  {0x3099, 1},
  {0x309C, 0},
  {0x309D, 8},
  {0x309E, 0x2C},
  {0x30A0, 0x82},
  {0x30A1, 2},
  {0x30A2, 0},
  {0x30A3, 0xE},
  {0x30A6, 0},
  {0x30A7, 0x20},
  {0x30A8, 4},
  {0x30A9, 0},
  {0x30AA, 0},
  {0x30AB, 0x30},
  {0x30AC, 0x32},
  {0x30AD, 0x10},
  {0x30AE, 0},
  {0x30AF, 0},
  {0x30B0, 0x3E},
  {0x30B1, 0},
  {0x30B2, 0x1F},
  {0x30B3, 0},
  {0x30B4, 0x3E},
  {0x30B5, 0},
  {0x30B6, 0x1F},
  {0x30B7, 0x70},
  {0x30B9, 0x80},
  {0x30BA, 0},
  {0x30BB, 0},
  {0x30BC, 0},
  {0x30BD, 0},
  {0x30BE, 0x80},
  {0x30BF, 0},
  {0x30C0, 0xE6},
  {0x30C1, 0},
  {0x30C2, 0},
  {0x30C4, 0},
  {0x30C5, 0x40},
  {0x30C6, 0},
  {0x30C7, 0},
  {0x30C8, 0x10},
  {0x30C9, 0},
  {0x30CC, 0xC0},
  {0x30CD, 0xF0},
  {0x30CE, 0x65},
  {0x30CF, 0x75},
  {0x30D0, 0x22},
  {0x30D1, 5},
  {0x30D2, 0xB3},
  {0x30D3, 0x80},
  {0x30D5, 9},
  {0x30D6, 0xCC},
  {0x30D7, 0x10},
  {0x30D8, 0},
  {0x30D9, 0x27},
  {0x30DB, 0},
  {0x30DC, 0x21},
  {0x30DD, 7},
  {0x30DE, 0},
  {0x30DF, 0x10},
  {0x30E0, 0x13},
  {0x30E1, 0xC0},
  {0x30E2, 0},
  {0x30E3, 0x40},
  {0x30E4, 0},
  {0x30E5, 0x80},
  {0x30E6, 0},
  {0x30E7, 0},
  {0x30E8, 0x10},
  {0x30E9, 0},
  {0x30EA, 0},
  {0x3100, 7},
  {0x3101, 0xED},
  {0x3104, 0},
  {0x3105, 0},
  {0x3106, 0xA0},
  {0x3107, 6},
  {0x3108, 0x10},
  {0x310A, 0x54},
  {0x310B, 0},
  {0x310C, 0x46},
  {0x310E, 0x79},
  {0x310F, 1},
  {0x3110, 4},
  {0x3111, 0},
  {0x3112, 0},
  {0x3113, 0xC},
  {0x3114, 0x2A},
  {0x3116, 0},
  {0x3118, 0x79},
  {0x311B, 0},
  {0x311C, 0xE},
  {0x311E, 0x30},
  {0x311F, 0x1E},
  {0x3121, 0x64},
  {0x3124, 0xFC},
  {0x3126, 2},
  {0x3127, 0},
  {0x3128, 8},
  {0x312A, 4},
  {0x312C, 0},
  {0x312D, 0x42},
  {0x312E, 0},
  {0x312F, 0},
  {0x3130, 0x42},
  {0x3131, 0x14},
  {0x3132, 8},
  {0x3133, 0},
  {0x3134, 1},
  {0x3135, 4},
  {0x3136, 4},
  {0x3137, 4},
  {0x3138, 4},
  {0x3139, 4},
  {0x313A, 4},
  {0x313B, 4},
  {0x313C, 4},
  {0x313D, 4},
  {0x313E, 4},
  {0x313F, 4},
  {0x3140, 4},
  {0x3141, 2},
  {0x3142, 2},
  {0x3143, 2},
  {0x3144, 2},
  {0x3145, 2},
  {0x3146, 2},
  {0x3147, 2},
  {0x3148, 2},
  {0x3149, 2},
  {0x314A, 2},
  {0x314B, 2},
  {0x314C, 2},
  {0x314D, 0x80},
  {0x314E, 0},
  {0x314F, 0x3D},
  {0x3150, 0x3B},
  {0x3151, 0x36},
  {0x3154, 0},
  {0x3155, 0},
  {0x3156, 0},
  {0x3159, 0},
  {0x315A, 4},
  {0x315B, 0x88},
  {0x315C, 0x88},
  {0x315D, 2},
  {0x315F, 0},
  {0x3160, 0x10},
  {0x3162, 0x7C},
  {0x3163, 0},
  {0x3165, 0x67},
  {0x3166, 0x11},
  {0x3167, 0x11},
  {0x3168, 0xF1},
  {0x3169, 0x77},
  {0x316A, 0x77},
  {0x316B, 0x61},
  {0x316C, 1},
  {0x316D, 0xC},
  {0x316E, 4},
  {0x316F, 4},
  {0x3172, 0x41},
  {0x3173, 0x30},
  {0x3174, 0},
  {0x3175, 0},
  {0x3176, 0x11},
  {0x3177, 0xA},
  {0x3178, 1},
  {0x3180, 2},
  {0x3181, 0x1E},
  {0x3182, 0},
  {0x3183, 0xFA},
  {0x3184, 0},
  {0x3185, 0x88},
  {0x3186, 0},
  {0x3187, 0x9C},
  {0x3188, 0},
  {0x3189, 0xC2},
  {0x318A, 0x10},
  {0x318B, 1},
  {0x318E, 0x88},
  {0x318F, 0x88},
  {0x3190, 0x88},
  {0x3191, 0x88},
  {0x3192, 0x44},
  {0x3193, 0x44},
  {0x3194, 0x44},
  {0x3195, 0x44},
  {0x3197, 0},
  {0x3198, 0},
  {0x3199, 0},
  {0x319A, 0},
  {0x319B, 0},
  {0x319C, 0},
  {0x31A8, 0},
  {0x31A9, 0xFF},
  {0x31AA, 0},
  {0x31AB, 0x10},
  {0x31AC, 0},
  {0x31AD, 0},
  {0x31AE, 0xFF},
  {0x31AF, 0x10},
  {0x31B0, 0x80},
  {0x31B1, 0x70},
  {0x31B5, 3},
  {0x31B6, 0x27},
  {0x31B7, 0},
  {0x31B8, 0x3B},
  {0x31B9, 0x1B},
  {0x31BA, 0x3B},
  {0x31BB, 0x11},
  {0x31BC, 0x3B},
  {0x31BD, 0xC},
  {0x31BE, 0xB},
  {0x31BF, 0x13},
  {0x31C1, 0x27},
  {0x31C2, 0},
  {0x31C3, 0xA3},
  {0x31C4, 0},
  {0x31C5, 0xBB},
  {0x31C6, 0x11},
  {0x31C7, 0},
  {0x31C8, 0x59},
  {0x31C9, 0xE},
  {0x31CA, 0},
  {0x31CB, 0xEE},
  {0x31CC, 0x10},
  {0x31CD, 7},
  {0x31CE, 0},
  {0x31CF, 0x5B},
  {0x31D0, 8},
  {0x31D1, 0},
  {0x31D2, 2},
  {0x31D3, 7},
  {0x31D4, 1},
  {0x31D6, 0},
  {0x31D7, 0},
  {0x31D8, 0},
  {0x31D9, 0},
  {0x31DA, 0x15},
  {0x31DB, 0x15},
  {0x31DC, 0xE0},
  {0x31DD, 0x10},
  {0x31DE, 0xE},
  {0x31DF, 9},
  {0x31E0, 1},
  {0x31E1, 9},
  {0x31E2, 1},
  {0x3200, 0x12},
  {0x3201, 0x12},
  {0x3203, 0},
  {0x3204, 0},
  {0x3205, 0x80},
  {0x3206, 0},
  {0x3207, 0},
  {0x3208, 0},
  {0x3209, 0},
  {0x320A, 0},
  {0x320B, 0},
  {0x320C, 0},
  {0x320D, 0},
  {0x320E, 0},
  {0x320F, 0},
  {0x3210, 0},
  {0x3211, 0},
  {0x3212, 0},
  {0x3213, 0},
  {0x3214, 0},
  {0x3215, 0},
  {0x3216, 0},
  {0x3217, 0},
  {0x3218, 0},
  {0x3219, 0},
  {0x321A, 0},
  {0x321B, 0},
  {0x321C, 0},
  {0x321D, 0},
  {0x321E, 0xC},
  {0x321F, 0x84},
  {0x3220, 0},
  {0x3221, 0},
  {0x3222, 0},
  {0x3223, 3},
  {0x3224, 0x20},
  {0x3225, 0x20},
  {0x3226, 0x20},
  {0x3227, 0x18},
  {0x3230, 0},
  {0x3231, 0},
  {0x3232, 0},
  {0x3233, 0},
  {0x3234, 0},
  {0x3235, 0x80},
  {0x3237, 0x80},
  {0x3238, 0},
  {0x3239, 0x80},
  {0x323A, 0x80},
  {0x323B, 0},
  {0x323C, 0x81},
  {0x323D, 0},
  {0x323E, 2},
  {0x323F, 0},
  {0x3240, 0x10},
  {0x3241, 0x32},
  {0x3242, 0x32},
  {0x3243, 0xD},
  {0x3244, 0x1B},
  {0x3245, 0x20},
  {0x3246, 0x22},
  {0x3247, 0xC},
  {0x3248, 0xE},
  {0x3249, 0x20},
  {0x324A, 0x20},
  {0x324B, 6},
  {0x324C, 0x22},
  {0x324D, 0x1C},
  {0x324E, 0x1C},
  {0x324F, 0xC},
  {0x3250, 0},
  {0x3251, 0x50},
  {0x3252, 0x74},
  {0x3253, 0x72},
  {0x3254, 0x68},
  {0x3255, 0x64},
  {0x3256, 0x6C},
  {0x3257, 0x6C},
  {0x3258, 0x72},
  {0x3259, 0x60},
  {0x325A, 0x82},
  {0x325B, 0x82},
  {0x325C, 0x74},
  {0x325D, 0x71},
  {0x325E, 0x74},
  {0x325F, 0x74},
  {0x3260, 0x70},
  {0x3261, 0x52},
  {0x3262, 4},
  {0x3263, 4},
  {0x3264, 0},
  {0x3265, 0x40},
  {0x3266, 8},
  {0x3267, 8},
  {0x3268, 0},
  {0x3269, 0x16},
  {0x326A, 0},
  {0x326B, 0},
  {0x326C, 0},
  {0x326D, 0x20},
  {0x326E, 0},
  {0x326F, 0},
  {0x3270, 0},
  {0x3271, 0x80},
  {0x3272, 0},
  {0x3273, 0x80},
  {0x3274, 1},
  {0x3275, 0},
  {0x3276, 0},
  {0x3282, 0xC0},
  {0x3284, 6},
  {0x3285, 3},
  {0x3286, 2},
  {0x3287, 0x40},
  {0x3288, 0x80},
  {0x3289, 6},
  {0x328A, 3},
  {0x328B, 2},
  {0x328C, 0x40},
  {0x328D, 0x80},
  {0x328E, 0x13},
  {0x328F, 0x80},
  {0x3290, 0x20},
  {0x3291, 0},
  {0x3294, 0x10},
  {0x3295, 0x20},
  {0x3296, 0x10},
  {0x3297, 0},
  {0x329E, 0},
  {0x329F, 4},
  {0x32A0, 0x3E},
  {0x32A1, 2},
  {0x32A2, 0x1F},
  {0x32A8, 0x84},
  {0x32A9, 2},
  {0x32AB, 0},
  {0x32AC, 0},
  {0x32AD, 0},
  {0x32AF, 0},
  {0x32B0, 8},
  {0x32B1, 0x10},
  {0x32B3, 0x10},
  {0x32B4, 0x1F},
  {0x32B5, 0x1F},
  {0x32B7, 0x3B},
  {0x32B8, 0xFF},
  {0x32BA, 2},
  {0x32BB, 4},
  {0x32BC, 4},
  {0x32BE, 3},
  {0x32BF, 6},
  {0x32C0, 6},
  {0x32C1, 0},
  {0x32C2, 0},
  {0x32C3, 0},
  {0x32C5, 1},
  {0x32C6, 0x50},
  {0x32C8, 0xE},
  {0x32C9, 4},
  {0x32CA, 0},
  {0x32CB, 0xE},
  {0x32CC, 0xE},
  {0x32CD, 0xE},
  {0x32CE, 8},
  {0x32CF, 8},
  {0x32D0, 8},
  {0x32D1, 0xF},
  {0x32D2, 0xF},
  {0x32D3, 0xF},
  {0x32D4, 8},
  {0x32D5, 8},
  {0x32D6, 8},
  {0x32D8, 0},
  {0x32D9, 2},
  {0x32DA, 2},
  {0x32DD, 2},
  {0x32DE, 4},
  {0x32DF, 4},
  {0x32E0, 0x20},
  {0x32E1, 0x20},
  {0x32E2, 0x20},
  {0x32E4, 0},
  {0x32E5, 0},
  {0x32E6, 0},
  {0x32E7, 0},
  {0x32E8, 0},
  {0x32E9, 0},
  {0x32EA, 0},
  {0x32EB, 0},
  {0x32EC, 0},
  {0x32ED, 0},
  {0x32EE, 0},
  {0x32EF, 0},
  {0x32F0, 0},
  {0x32F1, 0},
  {0x32F4, 3},
  {0x32F5, 0x80},
  {0x32F6, 0x80},
  {0x32F7, 0},
  {0x32F8, 3},
  {0x32F9, 3},
  {0x32FA, 1},
  {0x32FD, 0xF0},
  {0x32FE, 0},
  {0x32FF, 0},
  {0x3300, 0xA},
  {0x3301, 5},
  {0x3302, 0},
  {0x3303, 0x70},
  {0x3304, 0},
  {0x3305, 0},
  {0x3307, 0x48},
  {0x3308, 0x23},
  {0x3309, 0xD},
  {0x330A, 1},
  {0x330B, 1},
  {0x330C, 2},
  {0x330D, 0},
  {0x330E, 0},
  {0x3310, 3},
  {0x3311, 5},
  {0x3312, 0x42},
  {0x3313, 0},
  {0x3314, 0},
  {0x3316, 0},
  {0x3318, 0x40},
  {0x3319, 0xA},
  {0x3380, 0},
  {0x3381, 1},
  {0x3383, 8},
  {0x3384, 0x10},
  {0x338B, 0},
  {0x338C, 5},
  {0x338D, 3},
  {0x338E, 0},
  {0x338F, 0x20},
  {0x3390, 0},
  {0x3391, 0},
  {0x339B, 0},
  {0x339C, 0},
  {0x339D, 0},
  {0x33B2, 0},
  {0x33B3, 0},
  {0x33B4, 1},
  {0x33B5, 3},
  {0x33B6, 0},
  {0x33D0, 0},
  {0x33E0, 0},
  {0x33E1, 1},
  {0x33E2, 0},
  {0x33E4, 0},
  {0x33E5, 0},
  {0x33F1, 0},
  {0x33FF, 0},
  {0x3405, 0},
  {0x3420, 0},
  {0x3424, 0},
  {0x3425, 0x78},
  {0x3426, 0xFF},
  {0x3427, 0xC0},
  {0x3428, 0},
  {0x3429, 0x40},
  {0x342A, 0},
  {0x342B, 0},
  {0x342C, 0},
  {0x342D, 0},
  {0x342E, 0},
  {0x342F, 0},
  {0x3430, 0xA7},
  {0x3431, 0x60},
  {0x3432, 0x11},
  {0x3433, 0},
  {0x3434, 0},
  {0x3435, 0},
  {0x3436, 0},
  {0x3437, 0},
  {0x3438, 0},
  {0x3439, 0},
  {0x343A, 0},
  {0x3500, 0},
  {0x3502, 0},
  {0x3504, 0},
  {0x3505, 0},
  {0x3506, 0},
  {0x3507, 0},
  {0x3508, 0},
  {0x3509, 0},
  {0x350A, 0},
  {0x350B, 0},
  {0x350C, 0},
  {0x350D, 0},
  {0x350E, 0},
  {0x350F, 0},
  {0x3510, 0},
  {0x3511, 0},
  {0x3512, 0},
  {0x3513, 0},
  {0x3514, 0},
  {0x3515, 0},
  {0x3516, 0},
  {0x3517, 0},
  {0x3518, 0},
  {0x3519, 0},
  {0x351A, 0},
  {0x351B, 0},
  {0x351C, 0},
  {0x351D, 0},
  {0x351E, 0},
  {0x351F, 0},
  {0x3520, 0},
  {0x3521, 0},
  {0x3522, 0},
  {0x3523, 0},
  {0x3524, 0},
  {0x3525, 0},
  {0x3526, 0},
  {0x3527, 0},
  {0x3528, 0},
  {0x3529, 0},
  {0x352A, 0},
  {0x352B, 0},
  {0x352C, 0},
  {0x352D, 0},
  {0x352E, 0},
  {0x352F, 0},
  {0x3530, 0},
  {0x3531, 0},
  {0x3532, 0},
  {0x3533, 0},
  {0x3534, 0},
  {0x3535, 0},
  {0x3536, 0},
  {0x3537, 0},
  {0x3538, 0},
  {0x3539, 0},
  {0x353A, 0},
  {0x353B, 0},
  {0x353C, 0},
  {0x353D, 0},
  {0x353E, 0},
  {0x353F, 0},
  {0x3540, 0},
  {0x3541, 0},
  {0x3542, 0},
  {0x3543, 0},
  {0x3545, 7},
  {0x3547, 0},
  {0x354E, 0},
  {0x3560, 0},
  {0x3561, 0},
  {0x3562, 0},
  {0x3563, 0},
  {0x3564, 0},
  {0x3565, 0},
  {0x3566, 0},
  {0x3567, 0},
  {0x3568, 0},
  {0x3569, 0},
  {0x356A, 0},
  {0x356B, 0},
  {0x356C, 0},
  {0x356D, 0},
  {0x356E, 0},
  {0x356F, 0},
  {0x3570, 0},
  {0x3571, 0},
  {0x3572, 0},
  {0x3573, 0},
  {0x3574, 0},
  {0x3575, 0},
  {0x3576, 0},
  {0x3577, 0},
  {0x3578, 0},
  {0x3579, 0},
  {0x357A, 0},
  {0x357B, 0},
  {0x357C, 0},
  {0x357D, 0},
  {0x357E, 0},
  {0x357F, 0},
  {0x3580, 0},
  {0x3581, 0},
  {0x3582, 0},
  {0x3583, 0},
  {0x3584, 0},
  {0x3585, 0},
  {0x3586, 0},
  {0x3587, 0},
  {0x3588, 0},
  {0x3589, 0},
  {0x358A, 0},
  {0x358B, 0},
  {0x358C, 0},
  {0x358D, 0},
  {0x358E, 0},
  {0x358F, 0},
  {0x3590, 0},
  {0x3591, 0},
  {0x3592, 0},
  {0x3593, 0},
  {0x3594, 0},
  {0x3595, 0},
  {0x3596, 0},
  {0x3597, 0},
  {0x3598, 0},
  {0x3599, 0},
  {0x359A, 0},
  {0x359B, 0},
  {0x359C, 0},
  {0x359D, 0},
  {0x359E, 0},
  {0x359F, 0},
  {0x100, 1},
};

static struct msm_camera_i2c_reg_setting init_reg_setting[] = {
  {
    .reg_setting = init_reg_array,
    .size = ARRAY_SIZE(init_reg_array),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },
};

static struct sensor_lib_reg_settings_array init_settings_array = {
  .reg_settings = init_reg_setting,
  .size = 1,
};

static struct msm_camera_i2c_reg_array start_reg_array[] = {
  {0x0100, 0x01},
};

static  struct msm_camera_i2c_reg_setting start_settings = {
  .reg_setting = start_reg_array,
  .size = ARRAY_SIZE(start_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_i2c_reg_array stop_reg_array[] = {
  {0x0100, 0x00},
};

static struct msm_camera_i2c_reg_setting stop_settings = {
  .reg_setting = stop_reg_array,
  .size = ARRAY_SIZE(stop_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_i2c_reg_array groupon_reg_array[] = {
  {0x0104, 0x01},
};

static struct msm_camera_i2c_reg_setting groupon_settings = {
  .reg_setting = groupon_reg_array,
  .size = ARRAY_SIZE(groupon_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_i2c_reg_array groupoff_reg_array[] = {
  {0x0104, 0x00},
};

static struct msm_camera_i2c_reg_setting groupoff_settings = {
  .reg_setting = groupoff_reg_array,
  .size = ARRAY_SIZE(groupoff_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_csid_vc_cfg t4k37ab_cid_cfg[] = {
  {0, CSI_RAW10, CSI_DECODE_10BIT},
  {1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params t4k37ab_csi_params = {
  .csid_params = {
    .lane_cnt = 4,
    .lut_params = {
      .num_cid = ARRAY_SIZE(t4k37ab_cid_cfg),
      .vc_cfg = {
         &t4k37ab_cid_cfg[0],
         &t4k37ab_cid_cfg[1],
      },
    },
  },
  .csiphy_params = {
    .lane_cnt = 4,
    .settle_cnt = 0x12,
  },
};

static struct sensor_pix_fmt_info_t t4k37ab_pix_fmt0_fourcc[] = {
  { V4L2_PIX_FMT_SGRBG10 },
};

static struct sensor_pix_fmt_info_t t4k37ab_pix_fmt1_fourcc[] = {
  { MSM_V4L2_PIX_FMT_META },
};

static sensor_stream_info_t t4k37ab_stream_info[] = {
  {1, &t4k37ab_cid_cfg[0], t4k37ab_pix_fmt0_fourcc},
  {1, &t4k37ab_cid_cfg[1], t4k37ab_pix_fmt1_fourcc},
};

static sensor_stream_info_array_t t4k37ab_stream_info_array = {
  .sensor_stream_info = t4k37ab_stream_info,
  .size = ARRAY_SIZE(t4k37ab_stream_info),
};

static struct msm_camera_i2c_reg_array res0_reg_array[] = {
  {0x104, 1},
  {0x340, 0xC},
  {0x341, 0x48},
  {0x342, 0x15},
  {0x343, 0x20},
  {0x346, 0},
  {0x347, 0},
  {0x34A, 0xC},
  {0x34B, 0x2F},
  {0x34C, 0x10},
  {0x34D, 0x20},
  {0x34E, 0xC},
  {0x34F, 0x18},
  {0x401, 0},
  {0x404, 0x10},
  {0x900, 0},
  {0x901, 0},
  {0x902, 0},
  {0x104, 0},
};

static struct msm_camera_i2c_reg_array res1_reg_array[] = {
  {0x104, 1},
  {0x340, 6},
  {0x341, 0x2C},
  {0x342, 0x15},
  {0x343, 0x20},
  {0x346, 0},
  {0x347, 0},
  {0x34A, 0xC},
  {0x34B, 0x2F},
  {0x34C, 8},
  {0x34D, 0x38},
  {0x34E, 6},
  {0x34F, 0x18},
  {0x401, 0},
  {0x404, 0x10},
  {0x900, 1},
  {0x901, 1},
  {0x902, 0},
  {0x104, 0},
};

static struct msm_camera_i2c_reg_setting res_settings[] = {
  {
    .reg_setting = res0_reg_array,
    .size = ARRAY_SIZE(res0_reg_array),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },
  {
    .reg_setting = res1_reg_array,
    .size = ARRAY_SIZE(res1_reg_array),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },
};

static struct sensor_lib_reg_settings_array res_settings_array = {
  .reg_settings = res_settings,
  .size = ARRAY_SIZE(res_settings),
};

static struct msm_camera_csi2_params *csi_params[] = {
  &t4k37ab_csi_params, /* RES 0*/
  &t4k37ab_csi_params, /* RES 1*/
};

static struct sensor_lib_csi_params_array csi_params_array = {
  .csi2_params = &csi_params[0],
  .size = ARRAY_SIZE(csi_params),
};

static struct sensor_crop_parms_t crop_params[] = {
  {0, 0, 0, 0}, /* RES 0 */
  {0, 0, 0, 0}, /* RES 1 */
};

static struct sensor_lib_crop_params_array crop_params_array = {
  .crop_params = crop_params,
  .size = ARRAY_SIZE(crop_params),
};

static struct sensor_lib_out_info_t sensor_out_info[] = {
  {
    .x_output = 0x1020, /* 4128 */
    .y_output = 0xC18, /* 3096 */
    .line_length_pclk = 0x1520, /* 5408 */
    .frame_length_lines = 0xC48, /* 3144 */
    .vt_pixel_clk = 259200000,
    .op_pixel_clk = 324000000,
    .binning_factor = 1,
    .max_fps = 15.24,
    .min_fps = 7.5,
    .mode = SENSOR_DEFAULT_MODE,
  },
  {
    .x_output = 0x838, /* 2104 */
    .y_output = 0x618, /* 1560 */
    .line_length_pclk = 0x1520, /* 5408 */
    .frame_length_lines = 0x62C, /* 1580 */
    .vt_pixel_clk = 259200000,
    .op_pixel_clk = 324000000,
    .binning_factor = 1,
    .max_fps = 30.33,
    .min_fps = 7.5,
    .mode = SENSOR_DEFAULT_MODE,
  },
};

static struct sensor_lib_out_info_array out_info_array = {
  .out_info = sensor_out_info,
  .size = ARRAY_SIZE(sensor_out_info),
};

static sensor_res_cfg_type_t t4k37ab_res_cfg[] = {
  SENSOR_SET_STOP_STREAM,
  SENSOR_SET_NEW_RESOLUTION, /* set stream config */
  SENSOR_SET_CSIPHY_CFG,
  SENSOR_SET_CSID_CFG,
  SENSOR_LOAD_CHROMATIX, /* set chromatix prt */
  SENSOR_SEND_EVENT, /* send event */
  SENSOR_SET_START_STREAM,
};

static struct sensor_res_cfg_table_t t4k37ab_res_table = {
  .res_cfg_type = t4k37ab_res_cfg,
  .size = ARRAY_SIZE(t4k37ab_res_cfg),
};

static struct sensor_lib_chromatix_t t4k37ab_chromatix[] = {
  {
    .common_chromatix = T4K37AB_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = T4K37AB_LOAD_CHROMATIX(snapshot), /* RES0 */
    .camera_snapshot_chromatix = T4K37AB_LOAD_CHROMATIX(snapshot), /* RES0 */
    .camcorder_chromatix = T4K37AB_LOAD_CHROMATIX(default_video), /* RES0 */
    .liveshot_chromatix = NULL, /* RES0 */
  },
  {
    .common_chromatix = T4K37AB_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = T4K37AB_LOAD_CHROMATIX(preview), /* RES1 */
    .camera_snapshot_chromatix = T4K37AB_LOAD_CHROMATIX(preview), /* RES1 */
    .camcorder_chromatix = T4K37AB_LOAD_CHROMATIX(default_video), /* RES1 */
    .liveshot_chromatix = NULL, /* RES1 */
  },
};

static struct sensor_lib_chromatix_array t4k37ab_lib_chromatix_array = {
  .sensor_lib_chromatix = t4k37ab_chromatix,
  .size = ARRAY_SIZE(t4k37ab_chromatix),
};

/*===========================================================================
 * FUNCTION    - t4k37ab_real_to_register_gain -
 *
 * DESCRIPTION:
 *==========================================================================*/
static uint16_t t4k37ab_real_to_register_gain(float gain)
{
  uint16_t reg_gain;

  if(gain < 1.0) {
      gain = 1.0;
  } else if (gain > 8.0) {
      gain = 8.0;
  }
  gain = (gain) * 72.0;
  reg_gain = (uint16_t) gain;

  return reg_gain;
}

/*===========================================================================
 * FUNCTION    - t4k37ab_register_to_real_gain -
 *
 * DESCRIPTION:
 *==========================================================================*/
static float t4k37ab_register_to_real_gain(uint16_t reg_gain)
{
  float real_gain;

  if (reg_gain > 0x240) {
      reg_gain = 0x240;
  }
  real_gain = (float) reg_gain / 72.0;

  return real_gain;
}

/*===========================================================================
 * FUNCTION    - t4k37ab_calculate_exposure -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int32_t t4k37ab_calculate_exposure(float real_gain,
  uint16_t line_count, sensor_exposure_info_t *exp_info)
{
  if (!exp_info) {
    return -1;
  }
  exp_info->reg_gain = t4k37ab_real_to_register_gain(real_gain);
  float sensor_real_gain = t4k37ab_register_to_real_gain(exp_info->reg_gain);
  exp_info->digital_gain = real_gain / sensor_real_gain;
  exp_info->line_count = line_count;
  return 0;
}

/*===========================================================================
 * FUNCTION    - t4k37ab_fill_exposure_array -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int32_t t4k37ab_fill_exposure_array(uint16_t gain, uint32_t line,
  uint32_t fl_lines, int32_t luma_avg, uint32_t fgain,
  struct msm_camera_i2c_reg_setting* reg_setting)
{
  int32_t rc = 0;
  uint16_t reg_count = 0;
  uint16_t i = 0;

  if (!reg_setting) {
    return -1;
  }

  for (i = 0; i < sensor_lib_ptr.groupon_settings->size; i++) {
    reg_setting->reg_setting[reg_count].reg_addr =
      sensor_lib_ptr.groupon_settings->reg_setting[i].reg_addr;
    reg_setting->reg_setting[reg_count].reg_data =
      sensor_lib_ptr.groupon_settings->reg_setting[i].reg_data;
    reg_count = reg_count + 1;
  }

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.output_reg_addr->frame_length_lines;
  reg_setting->reg_setting[reg_count].reg_data = (fl_lines & 0xFF00) >> 8;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.output_reg_addr->frame_length_lines + 1;
  reg_setting->reg_setting[reg_count].reg_data = (fl_lines & 0xFF);
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->coarse_int_time_addr - 1;
  reg_setting->reg_setting[reg_count].reg_data = line >> 12;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->coarse_int_time_addr;
  reg_setting->reg_setting[reg_count].reg_data = line >> 4;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->coarse_int_time_addr + 1;
  reg_setting->reg_setting[reg_count].reg_data = line << 4;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->global_gain_addr;
  reg_setting->reg_setting[reg_count].reg_data = (gain & 0xFF00) >> 8;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->global_gain_addr + 1;
  reg_setting->reg_setting[reg_count].reg_data = (gain & 0xFF);
  reg_count++;

  for (i = 0; i < sensor_lib_ptr.groupoff_settings->size; i++) {
    reg_setting->reg_setting[reg_count].reg_addr =
      sensor_lib_ptr.groupoff_settings->reg_setting[i].reg_addr;
    reg_setting->reg_setting[reg_count].reg_data =
      sensor_lib_ptr.groupoff_settings->reg_setting[i].reg_data;
    reg_count = reg_count + 1;
  }

  reg_setting->size = reg_count;
  reg_setting->addr_type = MSM_CAMERA_I2C_WORD_ADDR;
  reg_setting->data_type = MSM_CAMERA_I2C_BYTE_DATA;
  reg_setting->delay = 0;

  return rc;
}

static sensor_exposure_table_t t4k37ab_expsoure_tbl = {
  .sensor_calculate_exposure = t4k37ab_calculate_exposure,
  .sensor_fill_exposure_array = t4k37ab_fill_exposure_array,
};

static sensor_lib_t sensor_lib_ptr = {
  /* sensor output settings */
  .sensor_output = &sensor_output,
  /* sensor output register address */
  .output_reg_addr = &output_reg_addr,
  /* sensor exposure gain register address */
  .exp_gain_info = &exp_gain_info,
  /* sensor aec info */
  .aec_info = &aec_info,
  /* sensor snapshot exposure wait frames info */
  .snapshot_exp_wait_frames = 1,
  /* number of frames to skip after start stream */
  .sensor_num_frame_skip = 4,
  /* number of frames to skip after start HDR stream */
  .sensor_num_HDR_frame_skip = 2,
  /* sensor exposure table size */
  .exposure_table_size = 8,
  /* sensor lens info */
  .default_lens_info = &default_lens_info,
  /* csi lane params */
  .csi_lane_params = &csi_lane_params,
  /* csi cid params */
  .csi_cid_params = t4k37ab_cid_cfg,
  /* csi csid params array size */
  .csi_cid_params_size = ARRAY_SIZE(t4k37ab_cid_cfg),
  /* init settings */
  .init_settings_array = &init_settings_array,
  /* start settings */
  .start_settings = &start_settings,
  /* stop settings */
  .stop_settings = &stop_settings,
  /* group on settings */
  .groupon_settings = &groupon_settings,
  /* group off settings */
  .groupoff_settings = &groupoff_settings,
  /* resolution cfg table */
  .sensor_res_cfg_table = &t4k37ab_res_table,
  /* res settings */
  .res_settings_array = &res_settings_array,
  /* out info array */
  .out_info_array = &out_info_array,
  /* crop params array */
  .crop_params_array = &crop_params_array,
  /* csi params array */
  .csi_params_array = &csi_params_array,
  /* sensor port info array */
  .sensor_stream_info_array = &t4k37ab_stream_info_array,
  /* exposure funtion table */
  .exposure_func_table = &t4k37ab_expsoure_tbl,
  /* chromatix array */
  .chromatix_array = &t4k37ab_lib_chromatix_array,
};

/*===========================================================================
 * FUNCTION    - t4k37ab_open_lib -
 *
 * DESCRIPTION:
 *==========================================================================*/
void *t4k37ab_open_lib(void)
{
  return &sensor_lib_ptr;
}
