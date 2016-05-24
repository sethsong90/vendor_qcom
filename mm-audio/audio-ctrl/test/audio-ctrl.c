#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/msm_audio.h>
#include <linux/android_pmem.h>
#include <pthread.h>
#include <getopt.h>
#include "audioeq.h"

#define AUDPP_MAX_MBADRC_BANDS 5
#define AUDPP_MBADRC_EXTERNAL_BUF_SIZE 196

#define AUDPP_DATA_EQUALIZER_MAX_BAND 12

#define AUDPP_DATA_QCONCERT_DEFAULT_MODE  (2)
#define AUDPP_DATA_QCONCERT_HP_MODE  (-1)
#define AUDPP_DATA_QCONCERT_SPK_MODE 0
#define AUDPP_DATA_QCONCERT_SPKDESK_MODE 1

struct adrc_cfg {
	uint16_t subband_enable;
	uint16_t adrc_sub_mute;
	uint16_t rms_time;
	uint16_t compression_th;
	uint16_t compression_slope;
	uint16_t attack_const_lsw;
	uint16_t attack_const_msw;
	uint16_t release_const_lsw;
	uint16_t release_const_msw;
	uint16_t makeup_gain;
};

struct adrc_ext_buf {
	int16_t buff[AUDPP_MBADRC_EXTERNAL_BUF_SIZE];
};

struct adrc_filter {
	uint16_t compression_th;
	uint16_t compression_slope;
	uint16_t rms_time;
	uint16_t attack_const_lsw;
	uint16_t attack_const_msw;
	uint16_t release_const_lsw;
	uint16_t release_const_msw;
	uint16_t adrc_delay;
};

struct mbadrc_filter {
	uint16_t num_bands;
	uint16_t down_samp_level;
	uint16_t adrc_delay;
	uint16_t ext_buf_size;
	uint16_t ext_partition;
	uint16_t ext_buf_msw;
	uint16_t ext_buf_lsw;
	struct adrc_cfg adrc_band[AUDPP_MAX_MBADRC_BANDS];
	struct adrc_ext_buf  ext_buf;
};

struct eq_filter {
	int16_t gain;
	uint16_t freq; 
	uint16_t type;
	uint16_t qf;
};

struct eq_filter eq[17][12] = {
	{
		{0x0,0x3c,0x1,0x16a},
		{0x0,0xaa,0x1,0x16a},
		{0x3,0x136,0x3,0x16a},
		{0x4,0x258,0x3,0x16a},
		{0x4,0x3e8,0x3,0x16a},
		{0x4,0xbb8,0x3,0x16a},
		{0x3,0x1770,0x2,0x16a},
		{0x0,0x2ee0,0x2,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},

	{
		{0x6,0x3c,0x1,0x16a},
		{0x4,0xaa,0x3,0x16a},
		{0x1,0x136,0x3,0x16a},
		{0x0,0x258,0x3,0x16a},
		{0x0,0x3e8,0x3,0x16a},
		{0x3,0xbb8,0x6,0x16a},
		{0x3,0x1770,0x6,0x16a},
		{0x5,0x2ee0,0x5,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},

	{
		{0x6,0x3c,0x1,0x16a},
		{0x6,0xaa,0x3,0x16a},
		{0x6,0x136,0x3,0x16a},
		{0x3,0x258,0x3,0x16a},
		{0x0,0x3e8,0x3,0x16a},
		{0x2,0xbb8,0x6,0x16a},
		{0x3,0x1770,0x6,0x16a},
		{0x6,0x2ee0,0x5,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},

	{
		{0x4,0x3c,0x1,0x16a},
		{0x3,0xaa,0x3,0x16a},
		{0x0,0x136,0x3,0x16a},
		{0x4,0x258,0x6,0x16a},
		{0x3,0x3e8,0x6,0x16a},
		{0x1,0xbb8,0x3,0x16a},
		{0x4,0x1770,0x3,0x16a},
		{0x6,0x2ee0,0x2,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},

	{
		{0x6,0x3c,0x4,0x16a},
		{0x6,0xaa,0x6,0x16a},
		{0x6,0x136,0x6,0x16a},
		{0x3,0x258,0x6,0x16a},
		{0x2,0x3e8,0x3,0x16a},
		{0x6,0xbb8,0x3,0x16a},
		{0x9,0x1770,0x3,0x16a},
		{0x9,0x2ee0,0x2,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},

	{
		{0x3,0x3c,0x1,0x16a},
		{0x6,0xaa,0x3,0x16a},
		{0x3,0x136,0x3,0x16a},
		{0x3,0x258,0x3,0x16a},
		{0x0,0x3e8,0x3,0x16a},
		{0x3,0xbb8,0x6,0x16a},
		{0x3,0x1770,0x6,0x16a},
		{0x3,0x2ee0,0x5,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},

	{
		{0x6,0x3c,0x1,0x16a},
		{0x6,0xaa,0x3,0x16a},
		{0x6,0x136,0x3,0x16a},
		{0x3,0x258,0x3,0x16a},
		{0x0,0x3e8,0x3,0x16a},
		{0x2,0xbb8,0x6,0x16a},
		{0x3,0x1770,0x6,0x16a},
		{0x6,0x2ee0,0x5,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},

	{
		{0x3,0x3c,0x4,0x16a},
		{0x0,0xaa,0x3,0x16a},
		{0x2,0x136,0x3,0x16a},
		{0x2,0x258,0x3,0x16a},
		{0x3,0x3e8,0x3,0x16a},
		{0x3,0xbb8,0x3,0x16a},
		{0x3,0x1770,0x3,0x16a},
		{0x2,0x2ee0,0x2,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},

	{
		{0x4,0x3c,0x1,0x16a},
		{0x4,0xaa,0x3,0x16a},
		{0x0,0x136,0x3,0x16a},
		{0x0,0x258,0x3,0x16a},
		{0x0,0x3e8,0x3,0x16a},
		{0x0,0xbb8,0x3,0x16a},
		{0x0,0x1770,0x3,0x16a},
		{0x4,0x2ee0,0x2,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},

	{
		{0x1,0x3c,0x4,0x16a},
		{0x3,0xaa,0x3,0x16a},
		{0x4,0x136,0x3,0x16a},
		{0x5,0x258,0x3,0x16a},
		{0x3,0x3e8,0x3,0x16a},
		{0x0,0xbb8,0x3,0x16a},
		{0x1,0x1770,0x6,0x16a},
		{0x1,0x2ee0,0x5,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},

	{
		{0x0,0x3c,0x1,0x16a},
		{0x0,0xaa,0x3,0x16a},
		{0x1,0x136,0x6,0x16a},
		{0x3,0x258,0x6,0x16a},
		{0x0,0x3e8,0x3,0x16a},
		{0x4,0xbb8,0x3,0x16a},
		{0x4,0x1770,0x3,0x16a},
		{0x0,0x2ee0,0x2,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},

	{
		{0x5,0x3c,0x1,0x16a},
		{0x3,0xaa,0x3,0x16a},
		{0x3,0x136,0x6,0x16a},
		{0x5,0x258,0x6,0x16a},
		{0x2,0x3e8,0x6,0x16a},
		{0x2,0xbb8,0x3,0x16a},
		{0x6,0x1770,0x3,0x16a},
		{0x6,0x2ee0,0x2,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},

	{
		{0x2,0x3c,0x4,0x16a},
		{0x3,0xaa,0x6,0x16a},
		{0x3,0x136,0x6,0x16a},
		{0x1,0x258,0x6,0x16a},
		{0x3,0x3e8,0x3,0x16a},
		{0x4,0xbb8,0x3,0x16a},
		{0x5,0x1770,0x3,0x16a},
		{0x5,0x2ee0,0x2,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},

	{
		{0x3,0x3c,0x1,0x16a},
		{0x1,0xaa,0x3,0x16a},
		{0x1,0x136,0x6,0x16a},
		{0x2,0x258,0x6,0x16a},
		{0x1,0x3e8,0x6,0x16a},
		{0x2,0xbb8,0x3,0x16a},
		{0x4,0x1770,0x3,0x16a},
		{0x6,0x2ee0,0x2,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},

	{
		{0x3,0x3c,0x1,0x16a},
		{0x3,0xaa,0x3,0x16a},
		{0x1,0x136,0x3,0x16a},
		{0x1,0x258,0x6,0x16a},
		{0x2,0x3e8,0x6,0x16a},
		{0x3,0xbb8,0x6,0x16a},
		{0x2,0x1770,0x6,0x16a},
		{0x3,0x2ee0,0x2,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},

	{
		{0x5,0x3c,0x1,0x16a},
		{0x3,0xaa,0x3,0x16a},
		{0x0,0x136,0x3,0x16a},
		{0x3,0x258,0x6,0x16a},
		{0x3,0x3e8,0x6,0x16a},
		{0x0,0xbb8,0x3,0x16a},
		{0x5,0x1770,0x3,0x16a},
		{0x6,0x2ee0,0x2,0x16a},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},
	{
		{0xFFF4, 190, 1, 0x016A},
		{0x6, 0x1F4, 3, 0x016A},
		{0x3, 0x7D0, 3, 0x016A},
		{0x3, 0xFA0, 3, 0x016A},
		{0xFFFD, 1388, 3, 0x016A},
		{0, 0x1F40, 3, 0x016A},
		{0x3, 2328, 3, 0x016A},
		{0xFFF4, 4650, 2, 0x016A},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
	},
};

struct eq_cmd {
	uint16_t num_bands;
	uint16_t coeffs[132];
};

struct rx_iir_filter {
	uint16_t num_bands;
	uint16_t iir_params[48];
};

struct vol_pan {
	uint16_t volume;
	uint16_t pan;
};

struct qconcert_plus {
	signed short                            output_mode;
	signed short                            gain;
	signed short                            expansion;
	signed short                            delay;
	unsigned short                          stages_per_mode;
	unsigned short                          reverb_enable;
	unsigned short                          decay_msw;
	unsigned short                          decay_lsw;
	unsigned short                          decay_time_ratio_msw;
	unsigned short                          decay_time_ratio_lsw;
	unsigned short                          reflection_delay_time;
	unsigned short                          late_reverb_gain;
	unsigned short                          late_reverb_delay;
	unsigned short                          delay_buff_size_msw;
	unsigned short                          delay_buff_size_lsw;
	unsigned short                          partition_num;
	unsigned short                          delay_buff_start_msw;
	unsigned short                          delay_buff_start_lsw;
};

struct qconcert_plus qconcert_plus_params = {
  AUDPP_DATA_QCONCERT_DEFAULT_MODE,        /* mode             */
  0x558C,                                /* gain             */
  0x7FFF,                                /* spread           */
  -1,             /* Geometry - default corresponding to 48Khz*/
  1,              /* stages */
};

struct vol_pan vol_pan_params = {
	0x7FFF, 0x10,
};

struct adrc_filter adrc[] = {
	{0x1000, 0xf333, 0x05cc, 0x6a8f, 0x0d69, 0x7f88, 0x1217, 0x0090},
	{0x1200, 0xf5c2, 0x0176, 0x76f3, 0x0209, 0x7f88, 0x1217, 0x0090},
	{0x1d27, 0xf5c2, 0x012C, 0x0176, 0x0209, 0x7f88, 0x1217, 0x0090},
	{0x1d27, 0xf5c2, 0x012C, 0x0209, 0x0176, 0x7f88, 0x1217, 0x0090},
	{0x1e0c, 0xcccd, 0x0100, 0x7fc3, 0xfaf9, 0x7ffb, 0xfeca, 0x0030},
};

struct mbadrc_filter mbadrc[] = {
	//Bass Boost
	{
		0x0002, // uint16 mbADRCNumBands
		0x0008, // uint16 mbADRCDownSampleLevel
		0x0090, // uint16 mbADRCDelay
		0x00C2, // uint16 mbADRCExtBufSize
		0x0, // uint16 mbADRCExtPartition
		0x0, // uint16 mbADRCExtBufStartMSW (will be filled at runtime)
		0x0, // uint16 mbADRCExtBufStartLSW (will be filled at runtime)
		{
		/* band0 */
		{0x0001, 0x0000, 0x0176, 0x1d27, 0xf5c2, 0x76f3, 0x0209, 0x7f88, 0x1217, 0x8565},
		/* band1 */
		{0x0001, 0x0000, 0x05cc, 0x23a7, 0xf333, 0x6a8f, 0x0d69, 0x7f88, 0x1217, 0x0a1e},
		},
		{
		0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0002,
		0x0005, 0x0009, 0x000C, 0x000A, 0x0004, 0xFFFA, 0xFFEC, 0xFFE0,
		0xFFDB, 0xFFE1, 0xFFF7, 0x0017, 0x003D, 0x005B, 0x0064, 0x004E,
		0x0015, 0xFFC4, 0xFF6D, 0xFF2C, 0xFF20, 0xFF5B, 0xFFDE, 0x0091,
		0x0148, 0x01C7, 0x01D5, 0x014D, 0x0031, 0xFEB3, 0xFD2E, 0xFC1C,
		0xFBFA, 0xFD26, 0xFFC5, 0x03AF, 0x0871, 0x0D5B, 0x11A3, 0x148D,
		0x1595, 0x148D, 0x11A3, 0x0D5B, 0x0871, 0x03AF, 0xFFC5, 0xFD26,
		0xFBFA, 0xFC1C, 0xFD2E, 0xFEB3, 0x0031, 0x014D, 0x01D5, 0x01C7,
		0x0148, 0x0091, 0xFFDE, 0xFF5B, 0xFF20, 0xFF2C, 0xFF6D, 0xFFC4,
		0x0015, 0x004E, 0x0064, 0x005B, 0x003D, 0x0017, 0xFFF7, 0xFFE1,
		0xFFDB, 0xFFE0, 0xFFEC, 0xFFFA, 0x0004, 0x000A, 0x000C, 0x0009,
		0x0005, 0x0002, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
		0x0000,
		}
	},
	//Trebble Boost
	{
		0x0002, // uint16 mbADRCNumBands
		0x0008, // uint16 mbADRCDownSampleLevel
		0x0090, // uint16 mbADRCDelay
		0x00C2, // uint16 mbADRCExtBufSize
		0x0, // uint16 mbADRCExtPartition
		0x0, // uint16 mbADRCExtBufStartMSW (will be filled at runtime)
		0x0, // uint16 mbADRCExtBufStartLSW (will be filled at runtime)
		{
		/* band0 */
	        {0x0001, 0x0000, 0x0176, 0x1d27, 0xf5c2, 0x76f3, 0x0209, 0x7f88, 0x1217, 0x086a},
		/* band1 */
	        {0x0001, 0x0000, 0x05cc, 0x23a7, 0xf333, 0x6a8f, 0x0d69, 0x7f88, 0x1217, 0xf2be},
		},
		{
		0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0002,
		0x0005, 0x0009, 0x000C, 0x000A, 0x0004, 0xFFFA, 0xFFEC, 0xFFE0,
		0xFFDB, 0xFFE1, 0xFFF7, 0x0017, 0x003D, 0x005B, 0x0064, 0x004E,
		0x0015, 0xFFC4, 0xFF6D, 0xFF2C, 0xFF20, 0xFF5B, 0xFFDE, 0x0091,
		0x0148, 0x01C7, 0x01D5, 0x014D, 0x0031, 0xFEB3, 0xFD2E, 0xFC1C,
		0xFBFA, 0xFD26, 0xFFC5, 0x03AF, 0x0871, 0x0D5B, 0x11A3, 0x148D,
		0x1595, 0x148D, 0x11A3, 0x0D5B, 0x0871, 0x03AF, 0xFFC5, 0xFD26,
		0xFBFA, 0xFC1C, 0xFD2E, 0xFEB3, 0x0031, 0x014D, 0x01D5, 0x01C7,
		0x0148, 0x0091, 0xFFDE, 0xFF5B, 0xFF20, 0xFF2C, 0xFF6D, 0xFFC4,
		0x0015, 0x004E, 0x0064, 0x005B, 0x003D, 0x0017, 0xFFF7, 0xFFE1,
		0xFFDB, 0xFFE0, 0xFFEC, 0xFFFA, 0x0004, 0x000A, 0x000C, 0x0009,
		0x0005, 0x0002, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
		0x0000,
		}
	},
	//Bass_512Hz_treble cutbands_5
	{
		0x0005, // uint16 mbADRCNumBands
		0x0008, // uint16 mbADRCDownSampleLevel
		0x0090, // uint16 mbADRCDelay
		0x0188, // uint16 mbADRCExtBufSize
		0x0, // uint16 mbADRCExtPartition
		0x0, // uint16 mbADRCExtBufStartMSW (will be filled at runtime)
		0x0, // uint16 mbADRCExtBufStartLSW (will be filled at runtime)
		{
		/* band0 */
		{0x0001, 0x0000, 0x0176, 0x1d27, 0xf5c2, 0x76f3, 0x0209, 0x7f88, 0x1217, 0xfe2f},
		/* band1 */
		{0x0001, 0x0000, 0x05cc, 0x23a7, 0xf333, 0x6a8f, 0x0d69, 0x7f88, 0x1217, 0x4b03},
		/* band2 */
		{0x0001, 0x0000, 0x12d3, 0x2aa7, 0xe666, 0x58b5, 0x8e16, 0x7f88, 0x1217, 0x17ff},
		/* band3 */
		{0x0001, 0x0000, 0x12d3, 0x2aa7, 0xe666, 0x58b5, 0x8e16, 0x7f88, 0x1217, 0x093a},
		/* band4 */
		{0x0001, 0x0000, 0x12d3, 0x2aa7, 0xe666, 0x58b5, 0x8e16, 0x7f88, 0x1217, 0x05f5},
		},
		{
		0x0001, 0x0001, 0x0001, 0x0002, 0x0002, 0x0004, 0x0005, 0x0007,
		0x0009, 0x000D, 0x0010, 0x0015, 0x001B, 0x0022, 0x002A, 0x0034,
		0x003F, 0x004C, 0x005B, 0x006B, 0x007E, 0x0092, 0x00A9, 0x00C2,
		0x00DD, 0x00FA, 0x0119, 0x013A, 0x015C, 0x0181, 0x01A6, 0x01CC,
		0x01F3, 0x021B, 0x0242, 0x0269, 0x0290, 0x02B5, 0x02D8, 0x02FA,
		0x031A, 0x0336, 0x0350, 0x0366, 0x0379, 0x0387, 0x0392, 0x0398,
		0x039B, 0x0398, 0x0392, 0x0387, 0x0379, 0x0366, 0x0350, 0x0336,
		0x031A, 0x02FA, 0x02D8, 0x02B5, 0x0290, 0x0269, 0x0242, 0x021B,
		0x01F3, 0x01CC, 0x01A6, 0x0181, 0x015C, 0x013A, 0x0119, 0x00FA,
		0x00DD, 0x00C2, 0x00A9, 0x0092, 0x007E, 0x006B, 0x005B, 0x004C,
		0x003F, 0x0034, 0x002A, 0x0022, 0x001B, 0x0015, 0x0010, 0x000D,
		0x0009, 0x0007, 0x0005, 0x0004, 0x0002, 0x0002, 0x0001, 0x0001,
		0x0001,
		0xFFFF, 0xFFFC, 0xFFF8, 0xFFF7, 0x0000, 0x0021, 0x006A, 0x00EF,
		0x01C1, 0x02EA, 0x0468, 0x0625, 0x07FE, 0x09C1, 0x0B37, 0x0C2F,
		0x0C86, 0x0C2F, 0x0B37, 0x09C1, 0x07FE, 0x0625, 0x0468, 0x02EA,
		0x01C1, 0x00EF, 0x006A, 0x0021, 0x0000, 0xFFF7, 0xFFF8, 0xFFFC,
		0xFFFF,
		0x0002, 0x0006, 0x000B, 0x0009, 0xFFF4, 0xFFB9, 0xFF56, 0xFEE4,
		0xFEA6, 0xFF03, 0x006A, 0x0323, 0x0717, 0x0BC0, 0x1036, 0x1370,
		0x149E, 0x1370, 0x1036, 0x0BC0, 0x0717, 0x0323, 0x006A, 0xFF03,
		0xFEA6, 0xFEE4, 0xFF56, 0xFFB9, 0xFFF4, 0x0009, 0x000B, 0x0006,
		0x0002,
		0x0000, 0xFFFE, 0x000F, 0xFFFA, 0xFFCE, 0x0057, 0x0036, 0xFEE1,
		0x00A7, 0x01EA, 0xFCB3, 0xFF31, 0x07B7, 0xF9FC, 0xF3DD, 0x25BE,
		0x4E07, 0x25BE, 0xF3DD, 0xF9FC, 0x07B7, 0xFF31, 0xFCB3, 0x01EA,
		0x00A7, 0xFEE1, 0x0036, 0x0057, 0xFFCE, 0xFFFA, 0x000F, 0xFFFE,
		0x0000,
		}
	},
	//Bass and trebble boost_3
	{
		0x0003, // uint16 mbADRCNumBands
		0x0008, // uint16 mbADRCDownSampleLevel
		0x0090, // uint16 mbADRCDelay
		0x0104, // uint16 mbADRCExtBufSize
		0x0, // uint16 mbADRCExtPartition
		0x0, // uint16 mbADRCExtBufStartMSW (will be filled at runtime)
		0x0, // uint16 mbADRCExtBufStartLSW (will be filled at runtime)
		{
		/* band0 */
		{0x0001, 0x0000, 0x0176, 0x1d27, 0xf5c2, 0x76f3, 0x0209, 0x7f88, 0x1217, 0xfe2f},
		/* band1 */
		{0x0001, 0x0000, 0x05cc, 0x23a7, 0xf333, 0x6a8f, 0x0d69, 0x7f88, 0x1217, 0x0a1e},
		/* band2 */
		{0x0001, 0x0000, 0x12d3, 0x2aa7, 0xe666, 0x58b5, 0x8e16, 0x7f88, 0x1217, 0xf2be},
		},
		{
		0x0001, 0x0001, 0x0001, 0x0002, 0x0002, 0x0004, 0x0005, 0x0007,
		0x0009, 0x000D, 0x0010, 0x0015, 0x001B, 0x0022, 0x002A, 0x0034,
		0x003F, 0x004C, 0x005B, 0x006B, 0x007E, 0x0092, 0x00A9, 0x00C2,
		0x00DD, 0x00FA, 0x0119, 0x013A, 0x015C, 0x0181, 0x01A6, 0x01CC,
		0x01F3, 0x021B, 0x0242, 0x0269, 0x0290, 0x02B5, 0x02D8, 0x02FA,
		0x031A, 0x0336, 0x0350, 0x0366, 0x0379, 0x0387, 0x0392, 0x0398,
		0x039B, 0x0398, 0x0392, 0x0387, 0x0379, 0x0366, 0x0350, 0x0336,
		0x031A, 0x02FA, 0x02D8, 0x02B5, 0x0290, 0x0269, 0x0242, 0x021B,
		0x01F3, 0x01CC, 0x01A6, 0x0181, 0x015C, 0x013A, 0x0119, 0x00FA,
		0x00DD, 0x00C2, 0x00A9, 0x0092, 0x007E, 0x006B, 0x005B, 0x004C,
		0x003F, 0x0034, 0x002A, 0x0022, 0x001B, 0x0015, 0x0010, 0x000D,
		0x0009, 0x0007, 0x0005, 0x0004, 0x0002, 0x0002, 0x0001, 0x0001,
		0x0001,
		0xFFFF, 0xFFFC, 0xFFF8, 0xFFF7, 0x0000, 0x0021, 0x006A, 0x00EF,
		0x01C1, 0x02EA, 0x0468, 0x0625, 0x07FE, 0x09C1, 0x0B37, 0x0C2F,
		0x0C86, 0x0C2F, 0x0B37, 0x09C1, 0x07FE, 0x0625, 0x0468, 0x02EA,
		0x01C1, 0x00EF, 0x006A, 0x0021, 0x0000, 0xFFF7, 0xFFF8, 0xFFFC,
		0xFFFF,
		}
	},
	//Bass and trebble boost_3_00
	{
		0x0003, // uint16 mbADRCNumBands
		0x0008, // uint16 mbADRCDownSampleLevel
		0x0090, // uint16 mbADRCDelay
		0x0104, // uint16 mbADRCExtBufSize
		0x0, // uint16 mbADRCExtPartition
		0x0, // uint16 mbADRCExtBufStartMSW (will be filled at runtime)
		0x0, // uint16 mbADRCExtBufStartLSW (will be filled at runtime)
		{
		/* band0 */
		{0x0001, 0x0000, 0x0176, 0x1d27, 0xf5c2, 0x76f3, 0x0209, 0x7f88, 0x1217, 0xfe2f},
		/* band1 */
		{0x0001, 0x0000, 0x05cc, 0x23a7, 0xf333, 0x6a8f, 0x0d69, 0x7f88, 0x1217, 0x0a1e},
		/* band2 */
		{0x0001, 0x0000, 0x12d3, 0x2aa7, 0xe666, 0x58b5, 0x8e16, 0x7f88, 0x1217, 0xf2be},
		},
		{
		0x0001, 0x0001, 0x0001, 0x0002, 0x0002, 0x0004, 0x0005, 0x0007,
		0x0009, 0x000D, 0x0010, 0x0015, 0x001B, 0x0022, 0x002A, 0x0034,
		0x003F, 0x004C, 0x005B, 0x006B, 0x007E, 0x0092, 0x00A9, 0x00C2,
		0x00DD, 0x00FA, 0x0119, 0x013A, 0x015C, 0x0181, 0x01A6, 0x01CC,
		0x01F3, 0x021B, 0x0242, 0x0269, 0x0290, 0x02B5, 0x02D8, 0x02FA,
		0x031A, 0x0336, 0x0350, 0x0366, 0x0379, 0x0387, 0x0392, 0x0398,
		0x039B, 0x0398, 0x0392, 0x0387, 0x0379, 0x0366, 0x0350, 0x0336,
		0x031A, 0x02FA, 0x02D8, 0x02B5, 0x0290, 0x0269, 0x0242, 0x021B,
		0x01F3, 0x01CC, 0x01A6, 0x0181, 0x015C, 0x013A, 0x0119, 0x00FA,
		0x00DD, 0x00C2, 0x00A9, 0x0092, 0x007E, 0x006B, 0x005B, 0x004C,
		0x003F, 0x0034, 0x002A, 0x0022, 0x001B, 0x0015, 0x0010, 0x000D,
		0x0009, 0x0007, 0x0005, 0x0004, 0x0002, 0x0002, 0x0001, 0x0001,
		0x0001,
		0xFFFF, 0xFFFE, 0x0002, 0x0016, 0x003D, 0x005D, 0x0042, 0xFFAE,
		0xFE97, 0xFD6F, 0xFD2D, 0xFEFF, 0x039F, 0x0AB2, 0x1290, 0x18C4,
		0x1B20, 0x18C4, 0x1290, 0x0AB2, 0x039F, 0xFEFF, 0xFD2D, 0xFD6F,
		0xFE97, 0xFFAE, 0x0042, 0x005D, 0x003D, 0x0016, 0x0002, 0xFFFE,
		0xFFFF,
		}
	},
};

struct acdb_gain_rx {
	unsigned short audppcalgain;
	unsigned short reserved;
};

struct acdb_cmd{
	unsigned	command_id;
	unsigned	device_id;
	unsigned	network_id;
	unsigned	sample_rate_id;
	unsigned	interface_id;
	unsigned	algorithm_block_id;
	unsigned	total_bytes;
	unsigned	*phys_buf;
};


#define AUDPREPROC_IIR_FILTER_NUM 4
#define AUDPREPROC_IIR_FILTER_PAN_LEFT   0xFFFF
#define AUDPREPROC_IIR_FILTER_PAN_BOTH   0x0000
#define AUDPREPROC_IIR_FILTER_PAN_RIGHT  0x0001

#define AUDPP_CAL_AUDFMT_IIR_FILTER_PAN_LEFT   0xFFFF
#define AUDPP_CAL_AUDFMT_IIR_FILTER_PAN_BOTH   0x0000
#define AUDPP_CAL_AUDFMT_IIR_FILTER_PAN_RIGHT  0x0001

#define COEFF(msb,lsb) (lsb), (msb)

struct tx_agc {
	unsigned short  cmd_id;
	unsigned short  tx_agc_param_mask;
	unsigned short  tx_agc_enable_flag;

	unsigned short  static_gain;
        signed short    adaptive_gain_flag;
	unsigned short  expander_th;
	unsigned short  expander_slope;
	unsigned short  compressor_th;
	unsigned short  compressor_slope;

	unsigned short  param_mask;
	unsigned short  aig_attackk;
	unsigned short  aig_leak_down;
	unsigned short  aig_leak_up;
	unsigned short  aig_max;
	unsigned short  aig_min;
	unsigned short  aig_releasek;
	unsigned short  aig_leakrate_fast;
	unsigned short  aig_leakrate_slow;
	unsigned short  attackk_msw;
	unsigned short  attackk_lsw;
	unsigned short  delay;
	unsigned short  releasek_msw;
	unsigned short  releasek_lsw;
	unsigned short  rms_tav;
};

struct tx_agc tx_agc_cfg[] = {
	{
	0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	},

	/* for sampling rate 8 khz */
	{
	0,0,0,0x2000,0,0x0780,0xFF9A,0x1B0C,0xF333,
	0,0xffee,0x7f69,0x8097,0x7f65,0x200a,0xfff9,
	0x0020,0x0004,0x0000,0xc273,0x0018,0x0000,0xffee,0x01ec
	},

	/* for sampling rate 11.025 khz */
	{
	0,0,0,0x2000,0,0x0780,0xFF9A,0x1B0C,0xF333,
	0,0xffee,0x7f69,0x8097,0x7f65,0x200a,0xfff9,
	0x0020,0x0004,0x0000,0xd1b1,0x0018,0x0000,0xfff3,0x01ec
	},

	/* for sampling rate 12 khz */
	{
	0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	},

	/* for sampling rate 16 khz */
	{
	0,0,0,0x7F65,0,0x0780,0xFF9A,0x1B0C,0xF333,
	0,0xfff7,0x7f69,0x8097,0x7f65,0x200a,0xfffc,
	0x0020,0x0004,0xDF1C,0xE00B,0x0030,0xFFF6,0xFD4B,0x00F6
	},

	/* for sampling rate 22.050 khz */
	{
	0,0,0,0x2000,0,0x0780,0xFF9A,0x1B0C,0xF333,
	0,0xffee,0x7f69,0x8097,0x7f65,0x200a,0xfff9,
	0x0020,0x0004,0xE7B0,0xDF86,0x0030,0xFFF9,0x762B,0x00F6
	},

	/* for sampling rate 24 khz */
	{
	0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	},

	/* for sampling rate 32 khz */
	{
	0,0,0,0x2000,0,0x0780,0xFF9A,0x210C,0xF333,
	0,0xffee,0x7f69,0x8097,0x7f65,0x200a,0xfff9,
	0x0020,0x0004,0xEEFD,0xCA75,0x0030,0xFFFB,0x7E9B,0x00F6
	},
	/* 44.1 MHz */
	{
	0,0,0,0x2000,0,0x0780,0xFF9A,0x210C,0xF333,
	0,0xffee,0x7f69,0x8097,0x7f65,0x200a,0xfff9,
	0x0020,0x0004,0xF38A,0xD708,0x0030,0xFFFC,0xBB10,0x00F6
	},

	/* for sampling rate 48 khz */
	{
	0,0,0,0x2000,0,0x0780,0xFF9A,0x210C,0xF333,
	0,0xffee,0x7f69,0x8097,0x7f65,0x200a,0xfff9,
	0x0020,0x0004,0xF488,0x1001,0x0030,0xFFFC,0xFF10,0x00F6
	},
};

struct ns {
	unsigned short  cmd_id;
	unsigned short  ec_mode_new;
	unsigned short  dens_gamma_n;
	unsigned short  dens_nfe_block_size;
	unsigned short  dens_limit_ns;
	unsigned short  dens_limit_ns_d;
	unsigned short  wb_gamma_e;
	unsigned short  wb_gamma_n;
};

struct ns ns_cfg[] = {
	/* when Disabled the ns  */
	{
	0,0,0,0,0,0,0,0
	},

	/* for sampling rate 8 khz */
	{
	0,0,0x01E6,0x0190,0x287A,0x287A,0x0001,0x0008
	},

	/* for sampling rate 11.025 khz */
	{
	0,0,0,0,0,0,0,0
	},

	/* for sampling rate 12 khz */
	{
	0,0,0,0,0,0,0,0
	},

	/* for sampling rate 16 khz */
	{
	0,0,0x01E6,0x0190,0x287A,0x287A,0x0001,0x0008
	},

	/* for sampling rate 22.050 khz */
	{
	0,0,0,0,0,0,0,0
	},

	/* for sampling rate 24 khz */
	{
	0,0,0,0,0,0,0,0
	},

	/* for sampling rate 32 khz */
	{
	0,0,0,0,0,0,0,0
	},

	/* for sampling rate 44.1 khz */
	{
	0,0,0,0,0,0,0,0
	},

	/* for sampling rate 48 khz */
	{
	0,0,0,0,0,0,0,0
	},
};

struct tx_iir {
	unsigned short  cmd_id;
	unsigned short  active_flag;
	unsigned short  num_bands;
	unsigned short  numerator_coeff_b0_filter0_lsw;
	unsigned short  numerator_coeff_b0_filter0_msw;
	unsigned short  numerator_coeff_b1_filter0_lsw;
	unsigned short  numerator_coeff_b1_filter0_msw;
	unsigned short  numerator_coeff_b2_filter0_lsw;
	unsigned short  numerator_coeff_b2_filter0_msw;
	unsigned short  numerator_coeff_b0_filter1_lsw;
	unsigned short  numerator_coeff_b0_filter1_msw;
	unsigned short  numerator_coeff_b1_filter1_lsw;
	unsigned short  numerator_coeff_b1_filter1_msw;
	unsigned short  numerator_coeff_b2_filter1_lsw;
	unsigned short  numerator_coeff_b2_filter1_msw;
	unsigned short  numerator_coeff_b0_filter2_lsw;
	unsigned short  numerator_coeff_b0_filter2_msw;
	unsigned short  numerator_coeff_b1_filter2_lsw;
	unsigned short  numerator_coeff_b1_filter2_msw;
	unsigned short  numerator_coeff_b2_filter2_lsw;
	unsigned short  numerator_coeff_b2_filter2_msw;
	unsigned short  numerator_coeff_b0_filter3_lsw;
	unsigned short  numerator_coeff_b0_filter3_msw;
	unsigned short  numerator_coeff_b1_filter3_lsw;
	unsigned short  numerator_coeff_b1_filter3_msw;
	unsigned short  numerator_coeff_b2_filter3_lsw;
	unsigned short  numerator_coeff_b2_filter3_msw;
	unsigned short  denominator_coeff_a0_filter0_lsw;
	unsigned short  denominator_coeff_a0_filter0_msw;
	unsigned short  denominator_coeff_a1_filter0_lsw;
	unsigned short  denominator_coeff_a1_filter0_msw;
	unsigned short  denominator_coeff_a0_filter1_lsw;
	unsigned short  denominator_coeff_a0_filter1_msw;
	unsigned short  denominator_coeff_a1_filter1_lsw;
	unsigned short  denominator_coeff_a1_filter1_msw;
	unsigned short  denominator_coeff_a0_filter2_lsw;
	unsigned short  denominator_coeff_a0_filter2_msw;
	unsigned short  denominator_coeff_a1_filter2_lsw;
	unsigned short  denominator_coeff_a1_filter2_msw;
	unsigned short  denominator_coeff_a0_filter3_lsw;
	unsigned short  denominator_coeff_a0_filter3_msw;
	unsigned short  denominator_coeff_a1_filter3_lsw;
	unsigned short  denominator_coeff_a1_filter3_msw;

	unsigned short  shift_factor_filter0;
	unsigned short  shift_factor_filter1;
	unsigned short  shift_factor_filter2;
	unsigned short  shift_factor_filter3;

	unsigned short  channel_selected0;
	unsigned short  channel_selected1;
	unsigned short  channel_selected2;
	unsigned short  channel_selected3;

};

struct tx_iir iir_cfg[] = {
	/* When disabling IIR */
	{
	0,0,
	0x0,
	/* Band NCoeff  */
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	/* Band MCoeff  */
	0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0,
	/* Shift factor */
	0x0,
	0x0,
	0x0,
	0x0,
	/* channel selection */
	0x0,
	0x0,
	0x0,
	0x0,
	},

	/* for sampling rate 8 khz */
	{
	0,0,
	AUDPREPROC_IIR_FILTER_NUM,
	/* Band NCoeff  */
	COEFF(0x4068, 0x1d6f), COEFF(0x8218, 0x5c25), COEFF(0x3d9f, 0x1f28),
	COEFF(0x4119, 0x47df), COEFF(0x8436, 0x5aef), COEFF(0x3b24, 0xe51b),
	COEFF(0x41ca, 0x6229), COEFF(0x875b, 0xea73), COEFF(0x3816, 0x3449),
	COEFF(0x44bc, 0x5515), COEFF(0x9a00, 0xbf27), COEFF(0x2b12, 0x2f1e),
	/* Band MCoeff  */
	COEFF(0x8218, 0x5c25), COEFF(0x3e07, 0x3cf0),
	COEFF(0x8436, 0x5aef), COEFF(0x3c3e, 0x2d37),
	COEFF(0x875b, 0xea73), COEFF(0x39e0, 0x9695),
	COEFF(0x9a00, 0xbf27), COEFF(0x2fce, 0x8435),
	/* Shift factor */
	0x0002,
	0x0002,
	0x0002,
	0x0002,
	/* channel selection */
	AUDPREPROC_IIR_FILTER_PAN_LEFT,
	AUDPREPROC_IIR_FILTER_PAN_RIGHT,
	AUDPREPROC_IIR_FILTER_PAN_BOTH,
	AUDPREPROC_IIR_FILTER_PAN_BOTH
	},

	/* for sampling rate 11.025 khz */
	{
	0,0,
	AUDPREPROC_IIR_FILTER_NUM,
	/* Band NCoeff  */
	COEFF(0x4068, 0x1d6f), COEFF(0x8218, 0x5c25), COEFF(0x3d9f, 0x1f28),
	COEFF(0x4119, 0x47df), COEFF(0x8436, 0x5aef), COEFF(0x3b24, 0xe51b),
	COEFF(0x41ca, 0x6229), COEFF(0x875b, 0xea73), COEFF(0x3816, 0x3449),
	COEFF(0x44bc, 0x5515), COEFF(0x9a00, 0xbf27), COEFF(0x2b12, 0x2f1e),
	/* Band MCoeff  */
	COEFF(0x8218, 0x5c25), COEFF(0x3e07, 0x3cf0),
	COEFF(0x8436, 0x5aef), COEFF(0x3c3e, 0x2d37),
	COEFF(0x875b, 0xea73), COEFF(0x39e0, 0x9695),
	COEFF(0x9a00, 0xbf27), COEFF(0x2fce, 0x8435),
	/* Shift factor */
	0x0002,
	0x0002,
	0x0002,
	0x0002,
	/* channel selection */
	AUDPREPROC_IIR_FILTER_PAN_LEFT,
	AUDPREPROC_IIR_FILTER_PAN_RIGHT,
	AUDPREPROC_IIR_FILTER_PAN_BOTH,
	AUDPREPROC_IIR_FILTER_PAN_BOTH
	},

	/* for sampling rate 12 khz */
	{
	0,0,
	AUDPREPROC_IIR_FILTER_NUM,
	/* Band NCoeff  */
	COEFF(0x4068, 0x1d6f), COEFF(0x8218, 0x5c25), COEFF(0x3d9f, 0x1f28),
	COEFF(0x4119, 0x47df), COEFF(0x8436, 0x5aef), COEFF(0x3b24, 0xe51b),
	COEFF(0x41ca, 0x6229), COEFF(0x875b, 0xea73), COEFF(0x3816, 0x3449),
	COEFF(0x44bc, 0x5515), COEFF(0x9a00, 0xbf27), COEFF(0x2b12, 0x2f1e),
	/* Band MCoeff  */
	COEFF(0x8218, 0x5c25), COEFF(0x3e07, 0x3cf0),
	COEFF(0x8436, 0x5aef), COEFF(0x3c3e, 0x2d37),
	COEFF(0x875b, 0xea73), COEFF(0x39e0, 0x9695),
	COEFF(0x9a00, 0xbf27), COEFF(0x2fce, 0x8435),
	/* Shift factor */
	0x0002,
	0x0002,
	0x0002,
	0x0002,
	/* channel selection */
	AUDPREPROC_IIR_FILTER_PAN_LEFT,
	AUDPREPROC_IIR_FILTER_PAN_RIGHT,
	AUDPREPROC_IIR_FILTER_PAN_BOTH,
	AUDPREPROC_IIR_FILTER_PAN_BOTH
	},

	/* for sampling rate 16 khz */
	{
	0,0,
	AUDPREPROC_IIR_FILTER_NUM,
	/* Band NCoeff  */
	COEFF(0x4068, 0x1d6f), COEFF(0x8218, 0x5c25), COEFF(0x3d9f, 0x1f28),
	COEFF(0x4119, 0x47df), COEFF(0x8436, 0x5aef), COEFF(0x3b24, 0xe51b),
	COEFF(0x41ca, 0x6229), COEFF(0x875b, 0xea73), COEFF(0x3816, 0x3449),
	COEFF(0x44bc, 0x5515), COEFF(0x9a00, 0xbf27), COEFF(0x2b12, 0x2f1e),
	/* Band MCoeff  */
	COEFF(0x8218, 0x5c25), COEFF(0x3e07, 0x3cf0),
	COEFF(0x8436, 0x5aef), COEFF(0x3c3e, 0x2d37),
	COEFF(0x875b, 0xea73), COEFF(0x39e0, 0x9695),
	COEFF(0x9a00, 0xbf27), COEFF(0x2fce, 0x8435),
	/* Shift factor */
	0x0002,
	0x0002,
	0x0002,
	0x0002,
	/* channel selection */
	AUDPREPROC_IIR_FILTER_PAN_LEFT,
	AUDPREPROC_IIR_FILTER_PAN_RIGHT,
	AUDPREPROC_IIR_FILTER_PAN_BOTH,
	AUDPREPROC_IIR_FILTER_PAN_BOTH
	},

	/* for sampling rate 22.050 khz */
	{
	0,0,
	AUDPREPROC_IIR_FILTER_NUM,
	/* Band NCoeff  */
	COEFF(0x4068, 0x1d6f), COEFF(0x8218, 0x5c25), COEFF(0x3d9f, 0x1f28),
	COEFF(0x4119, 0x47df), COEFF(0x8436, 0x5aef), COEFF(0x3b24, 0xe51b),
	COEFF(0x41ca, 0x6229), COEFF(0x875b, 0xea73), COEFF(0x3816, 0x3449),
	COEFF(0x44bc, 0x5515), COEFF(0x9a00, 0xbf27), COEFF(0x2b12, 0x2f1e),
	/* Band MCoeff  */
	COEFF(0x8218, 0x5c25), COEFF(0x3e07, 0x3cf0),
	COEFF(0x8436, 0x5aef), COEFF(0x3c3e, 0x2d37),
	COEFF(0x875b, 0xea73), COEFF(0x39e0, 0x9695),
	COEFF(0x9a00, 0xbf27), COEFF(0x2fce, 0x8435),
	/* Shift factor */
	0x0002,
	0x0002,
	0x0002,
	0x0002,
	/* channel selection */
	AUDPREPROC_IIR_FILTER_PAN_LEFT,
	AUDPREPROC_IIR_FILTER_PAN_RIGHT,
	AUDPREPROC_IIR_FILTER_PAN_BOTH,
	AUDPREPROC_IIR_FILTER_PAN_BOTH
	},
	/* for sampling rate 24 khz */
	{
	0,0,
	AUDPREPROC_IIR_FILTER_NUM,
	/* Band NCoeff  */
	COEFF(0x4068, 0x1d6f), COEFF(0x8218, 0x5c25), COEFF(0x3d9f, 0x1f28),
	COEFF(0x4119, 0x47df), COEFF(0x8436, 0x5aef), COEFF(0x3b24, 0xe51b),
	COEFF(0x41ca, 0x6229), COEFF(0x875b, 0xea73), COEFF(0x3816, 0x3449),
	COEFF(0x44bc, 0x5515), COEFF(0x9a00, 0xbf27), COEFF(0x2b12, 0x2f1e),
	/* Band MCoeff  */
	COEFF(0x8218, 0x5c25), COEFF(0x3e07, 0x3cf0),
	COEFF(0x8436, 0x5aef), COEFF(0x3c3e, 0x2d37),
	COEFF(0x875b, 0xea73), COEFF(0x39e0, 0x9695),
	COEFF(0x9a00, 0xbf27), COEFF(0x2fce, 0x8435),
	/* Shift factor */
	0x0002,
	0x0002,
	0x0002,
	0x0002,
	/* channel selection */
	AUDPREPROC_IIR_FILTER_PAN_LEFT,
	AUDPREPROC_IIR_FILTER_PAN_RIGHT,
	AUDPREPROC_IIR_FILTER_PAN_BOTH,
	AUDPREPROC_IIR_FILTER_PAN_BOTH
	},

	/* for sampling rate 32 khz */
	{
	0,0,
	AUDPREPROC_IIR_FILTER_NUM,
	/* Band NCoeff  */
	COEFF(0x4068, 0x1d6f), COEFF(0x8218, 0x5c25), COEFF(0x3d9f, 0x1f28),
	COEFF(0x4119, 0x47df), COEFF(0x8436, 0x5aef), COEFF(0x3b24, 0xe51b),
	COEFF(0x41ca, 0x6229), COEFF(0x875b, 0xea73), COEFF(0x3816, 0x3449),
	COEFF(0x44bc, 0x5515), COEFF(0x9a00, 0xbf27), COEFF(0x2b12, 0x2f1e),
	/* Band MCoeff  */
	COEFF(0x8218, 0x5c25), COEFF(0x3e07, 0x3cf0),
	COEFF(0x8436, 0x5aef), COEFF(0x3c3e, 0x2d37),
	COEFF(0x875b, 0xea73), COEFF(0x39e0, 0x9695),
	COEFF(0x9a00, 0xbf27), COEFF(0x2fce, 0x8435),
	/* Shift factor */
	0x0002,
	0x0002,
	0x0002,
	0x0002,
	/* channel selection */
	AUDPREPROC_IIR_FILTER_PAN_LEFT,
	AUDPREPROC_IIR_FILTER_PAN_RIGHT,
	AUDPREPROC_IIR_FILTER_PAN_BOTH,
	AUDPREPROC_IIR_FILTER_PAN_BOTH
	},

	/* for sampling rate 44.1 khz */
	{
	0,0,
	AUDPREPROC_IIR_FILTER_NUM,
	/* Band NCoeff  */
	COEFF(0x4068, 0x1d6f), COEFF(0x8218, 0x5c25), COEFF(0x3d9f, 0x1f28),
	COEFF(0x4119, 0x47df), COEFF(0x8436, 0x5aef), COEFF(0x3b24, 0xe51b),
	COEFF(0x41ca, 0x6229), COEFF(0x875b, 0xea73), COEFF(0x3816, 0x3449),
	COEFF(0x44bc, 0x5515), COEFF(0x9a00, 0xbf27), COEFF(0x2b12, 0x2f1e),
	/* Band MCoeff  */
	COEFF(0x8218, 0x5c25), COEFF(0x3e07, 0x3cf0),
	COEFF(0x8436, 0x5aef), COEFF(0x3c3e, 0x2d37),
	COEFF(0x875b, 0xea73), COEFF(0x39e0, 0x9695),
	COEFF(0x9a00, 0xbf27), COEFF(0x2fce, 0x8435),
	/* Shift factor */
	0x0002,
	0x0002,
	0x0002,
	0x0002,
	/* channel selection */
	AUDPREPROC_IIR_FILTER_PAN_LEFT,
	AUDPREPROC_IIR_FILTER_PAN_RIGHT,
	AUDPREPROC_IIR_FILTER_PAN_BOTH,
	AUDPREPROC_IIR_FILTER_PAN_BOTH
	},

	/* for sampling rate 48 khz */
	{
	0,0,
	AUDPREPROC_IIR_FILTER_NUM,
	/* Band NCoeff  */
	COEFF(0x4068, 0x1d6f), COEFF(0x8218, 0x5c25), COEFF(0x3d9f, 0x1f28),
	COEFF(0x4119, 0x47df), COEFF(0x8436, 0x5aef), COEFF(0x3b24, 0xe51b),
	COEFF(0x41ca, 0x6229), COEFF(0x875b, 0xea73), COEFF(0x3816, 0x3449),
	COEFF(0x44bc, 0x5515), COEFF(0x9a00, 0xbf27), COEFF(0x2b12, 0x2f1e),
	/* Band MCoeff  */
	COEFF(0x8218, 0x5c25), COEFF(0x3e07, 0x3cf0),
	COEFF(0x8436, 0x5aef), COEFF(0x3c3e, 0x2d37),
	COEFF(0x875b, 0xea73), COEFF(0x39e0, 0x9695),
	COEFF(0x9a00, 0xbf27), COEFF(0x2fce, 0x8435),
	/* Shift factor */
	0x0002,
	0x0002,
	0x0002,
	0x0002,
	/* channel selection */
	AUDPREPROC_IIR_FILTER_PAN_LEFT,
	AUDPREPROC_IIR_FILTER_PAN_RIGHT,
	AUDPREPROC_IIR_FILTER_PAN_BOTH,
	AUDPREPROC_IIR_FILTER_PAN_BOTH
	},

};
/* Default values for IIR Filter */
struct rx_iir_filter iir = {
	/* Num of Bands */
	4,
	/* Band NCoeff  */
	0x0000, 0x4000, 0x0000, 0x8000, 0x0000, 0x4000,
	0x6A48, 0x3DE3, 0x2B71, 0x8439, 0x6A48, 0x3DE3,
	0x6A48, 0x3DE3, 0x2B71, 0x8439, 0x6A48, 0x3DE3,
	0x6A48, 0x3DE3, 0x2B71, 0x8439, 0x6A48, 0x3DE3,
	/* Band MCoeff  */
	0x23E7, 0x8112, 0xEE41, 0x3F06,
	0x3072, 0x82D4, 0x88D1, 0x3D44,
	0x4EDB, 0x8424, 0x2804, 0x3BF4,
	0x4A43, 0x84D7, 0x0942, 0x3B41,
	/* Shift factor */
	0x0002,
	0x0002,
	0x0002,
	0x0002,
	/* channel selection */
	AUDPP_CAL_AUDFMT_IIR_FILTER_PAN_BOTH,
	AUDPP_CAL_AUDFMT_IIR_FILTER_PAN_BOTH,
	AUDPP_CAL_AUDFMT_IIR_FILTER_PAN_BOTH,
	AUDPP_CAL_AUDFMT_IIR_FILTER_PAN_BOTH
};

int samp_rate;
int vol, pan;
int pp_flag;
int pre_flag;
unsigned int pp_mask;
unsigned int pre_mask;
int adrc_option;
int mbadrc_option;
int eq_option;
int device_id;

int test_preprocessing(unsigned int enable_mask)
{
	int fd;
	int cfg_fd;
	int ret;
	int samp_index;
	struct msm_audio_config cfg;


	switch (samp_rate) {
        	case 48000:
			samp_index = 9;
			break;
		case 44100:
			samp_index = 8;
			break;
		case 32000:
			samp_index = 7;
			break;
		case 24000:
			samp_index = 6;
			break;
		case 22050:
			samp_index = 5;
			break;
		case 16000:
			samp_index = 4;
			break;
		case 12000:
			samp_index = 3;
			break;
		case 11025:
			samp_index = 2;
			break;
		case 8000:
			samp_index = 1;
			break;
		default:
			return -EINVAL;
			break;
	}
	
	fd = open("/dev/msm_preproc_ctl", O_RDWR);
	if (fd < 0) {
		printf("Unable to open msm_pcm_ctl\n");
		return -1;
	}
	if (enable_mask & AGC_ENABLE) {
		ret = ioctl(fd, AUDIO_SET_AGC, &tx_agc_cfg[samp_index]);
		printf("ret = %d\n", ret);
		if (ret)
			return ret;
	}

	if (enable_mask & NS_ENABLE) {
		ret = ioctl(fd, AUDIO_SET_NS, &ns_cfg[samp_index]);
		printf("ret = %d\n", ret);
		if (ret)
			return ret;
	}

	if (enable_mask & TX_IIR_ENABLE) {
		ret = ioctl(fd, AUDIO_SET_TX_IIR, &iir_cfg[samp_index]);
		printf("ret = %d\n", ret);
		if (ret)
			return ret;
	}

	ret = ioctl(fd, AUDIO_ENABLE_AUDPRE, &enable_mask);
	printf("ret = %d\n", ret);

	return ret;
}

int test_postprocessing(unsigned int enable_mask)
{
	int fd;
	int ret;
	int qconcert_pmem_fd = -1;
	void *ptr = NULL;
	int i,j;
	int samp_rate, samp_rate_div_2;
	uint16_t coeff_filter_tbl[12];
	uint16_t coeff[12 * 11];
	struct eq_cmd eq_temp = {0};
	uint16_t offset = 0;
	uint16_t temp_num, temp_den, temp_shift;
	struct eq_filter *filter;
	uint16_t numerator[6];
	uint16_t denominator[4];
	uint16_t shift[2];

#ifdef AUDIOV2
	fd = open("/dev/msm_acdb", O_RDWR);
	if (fd < 0) {
		printf("Unable to open msm_acdb\n");
		return -1;
	}
#else
	fd = open("/dev/msm_pcm_ctl", O_RDWR);
	if (fd < 0) {
		printf("Unable to open msm_pcm_ctl\n");
		return -1;
	}
#endif

	if (enable_mask & ADRC_ENABLE) {
		ret = ioctl(fd, AUDIO_SET_ADRC, &adrc[adrc_option - 1]);
		printf("ret = %d\n", ret);
		if (ret)
			return ret;
	}

	if (enable_mask & MBADRC_ENABLE) {
		ret = ioctl(fd, AUDIO_SET_MBADRC, &mbadrc[mbadrc_option - 1]);
		printf("ret = %d\n", ret);
		if (ret)
			return ret;
	}

	if (enable_mask & EQ_ENABLE) {
		samp_rate = 48000;
		samp_rate_div_2 = samp_rate >> 1;
		j = 0;

		eq_temp.num_bands = 8;

		for (i = 0; i < eq_temp.num_bands; i++) {
			if ((eq[eq_option - 1][i].freq < samp_rate_div_2) &&
				(eq[eq_option - 1][i].freq <= 20000)) {
				coeff_filter_tbl[j] = i;
				j++;
			}
		}
		eq_temp.num_bands = j;
		printf("num_bands = %d\n", eq_temp.num_bands);	
		for (i = 0; i < eq_temp.num_bands; i++) {
			filter = &eq[eq_option - 1][coeff_filter_tbl[i]];
			printf("%d %d %d %d\n", filter->gain, filter->freq, filter->type, filter->qf);
			audioeq_calccoefs(filter->gain, filter->freq, samp_rate,
						filter->type, filter->qf, 
						(int32_t *)numerator,
						(int32_t *)denominator,
						shift);
			for (j = 0; j < 6; j++)
				eq_temp.coeffs[( i * 6) + j] = numerator[j];
			for (j = 0; j < 4; j++)
				eq_temp.coeffs[(eq_temp.num_bands * 6) + (i * 4) + j] = denominator[j];
			eq_temp.coeffs[(eq_temp.num_bands * 10) + i] = shift[0];
			
		}
		ret = ioctl(fd, AUDIO_SET_EQ, &eq_temp);
#ifdef AUDIOV2
		close(fd);
		return ret;
#endif
		if (ret)
			return ret;
	}

	if (enable_mask & IIR_ENABLE) {
		ret = ioctl(fd, AUDIO_SET_RX_IIR, &iir);
		printf("ret = %d\n", ret);
		if (ret)
			return ret;
	}

	if (vol != -1) {
		ret = ioctl(fd, AUDIO_SET_VOLUME, vol);
		printf("ret = %d\n", ret);
		if (ret)
			return ret;
	}

	if (pan != -1) {
		ret = ioctl(fd, AUDIO_SET_PAN, pan);
		printf("ret = %d\n", ret);
		if (ret)
			return ret;
	}

	if (enable_mask & QCONCERT_PLUS_ENABLE) {
		ret = ioctl(fd, AUDIO_SET_QCONCERT_PLUS, &qconcert_plus_params);
		printf("ret = %d\n", ret);
		if (ret)
			return ret;
	}

	ret = ioctl(fd, AUDIO_ENABLE_AUDPP, &enable_mask);
	printf("ret = %d\n", ret);

	return ret;
}

static struct option long_options[] =
{
	{"post", no_argument, 0, 'P'},
	{"pre", required_argument, 0, 'R'},
	{"adrc", required_argument, 0, 'a'},
	{"mbadrc", required_argument, 0, 'm'},
	{"eq", required_argument, 0, 'e'},
	{"iir", no_argument, 0, 'i'},
	{"qconcert", no_argument, 0, 'q'},
	{"vol", required_argument, 0, 'v'},
	{"pan", required_argument, 0, 'p'},
	{"agc", no_argument, 0, 'g'},
	{"tx_iir", no_argument, 0, 't'},
	{"ns", no_argument, 0, 'n'},
	{"help", no_argument, 0, 'h'},
	{"set", required_argument, 0, 'S'},
	{"get", required_argument, 0, 'G'},
	{0, 0, 0, 0}
};

void print_help_menu(void)
{
	printf("Perform Post or Pre processing Tests\n\n");
	printf("Post Processing Tests\n");
	printf("-P, --post			Post processing Test\n");
	printf("-a, --adrc=[1..5]		choose adrc parameters to test with\n");
	printf("    				1 - Tune1\n");
	printf("    				2 - Tune2\n");
	printf("    				3 - Tune3\n");
	printf("    				4 - Tune4\n");
	printf("    				5 - Tune5\n");
	printf("-m, --mbadrc=[1..5]		choose mbadrc prameters to test with\n");
	printf("    				1 - Bass boost\n");
	printf("    				2 - Trebble boost\n");
	printf("    				3 - Bass_512Hz_treble cutbands_5\n");
	printf("    				4 - Bass and trebble boost_3\n");
	printf("				5 - Bass and trebble boost_3_00\n");
	printf("-e, --eq=[1..17]		choose Equalization parameters to test with\n");
	printf("    				1  - club\n");
	printf("    				2  - dance\n");
	printf("    				3  - fullbass\n");
	printf("    				4  - basstrebble\n");
	printf("    				5  - fulltrebble\n");
	printf("    				6  - laptop\n");
	printf("    				7  - lhall\n");
	printf("    				8  - live\n");
	printf("    				9  - party\n");
	printf("    				10 - pop\n");
	printf("    				11 - reggae\n");
	printf("    				12 - rock\n");
	printf("    				13 - ska\n");
	printf("    				14 - soft\n");
	printf("    				15 - softrock\n");
	printf("    				16 - techno\n");
	printf("    				17 - g-preset\n");
	printf("-i, --iir			Test IIR\n");
	printf("-q, --qconcert			Test QConcert\n");
	printf("-v, --vol=[0x0..0x7FFF]		Sets Volume, volume in Hex\n");
	printf("				0x0    - Mute\n");
	printf("				0x7FFF - Max volume\n");
	printf("-p, --pan=[-0x40..0x40]		Sets Pan, pan in Hex\n");
	printf("				-0x40 - Full volume in left channel\n");
	printf("				0x40  - Full volume in right channel\n");
	printf("				0x0   - Equal volume in two channels\n");
	printf("\nPre Processing Tests\n");
	printf("-R, --pre=[8000..48000]		Test Preprocessing features at samp freq\n");
	printf("-g, --agc			Test automatic gain control\n");
	printf("-t, --tx_iir			Test IIR\n");
	printf("-n, --ns			Test noise suppression\n");
	printf("\n ACDB user space interface testing\n");
	printf("-S                              Test AUDIO_SET_ACDB_BLK ioctl in ACDB driver\n");
	printf("                                This option need device_id as argument\n");
	printf("-G                              Test AUDIO_GET_ACDB_BLK ioctl in ACDB driver\n");
	printf("                                This option need device_id as argument\n");
	printf("These two options -S and -G are valid for 7x30 target only\n");
}

static int set_calibration()
{
	int fd1;
	int fd2;
	int result = 0;
	struct acdb_cmd cmd;
	unsigned short *vir_addr = NULL;
	unsigned blk_size = 0;
	struct acdb_gain_rx algo_blk;
	struct msm_audio_pmem_info pmem_info;

	fd1 = open("/dev/msm_acdb", O_RDWR);
	if (fd1 < 0) {
		printf("Failed to open acdb_driver returned %d \n", fd1);
		return -1;
	}

	fd2 = open("/dev/pmem_adsp", O_RDWR);
	if (fd2 < 0) {
		printf("Failed to open pmem driver returned %d \n", fd2);
		close(fd1);
		return -1;
	}

	blk_size = sizeof(struct acdb_gain_rx);
	blk_size = (blk_size + 4095) & (~4095);
	printf("blk_size = %d\n",blk_size);
	algo_blk.audppcalgain = 0;
	algo_blk.reserved = 0;

	vir_addr = (unsigned short *)mmap( NULL,blk_size,
		     PROT_READ | PROT_WRITE,
		     MAP_SHARED,
		     fd2,
		     0 );
	if(MAP_FAILED == vir_addr)
	{
		printf("\n mmap() failed");
		close(fd1);
		close(fd2);
		return -1;
	}

	memcpy(vir_addr,&algo_blk, sizeof(struct acdb_gain_rx));
	pmem_info.fd = fd2;
	pmem_info.vaddr = vir_addr;
	if (ioctl(fd1, AUDIO_REGISTER_PMEM, &pmem_info) < 0) {
                printf("Couldnot register pmem memory \n");
                close(fd1);
                close(fd2);
                return -1;
        }


        cmd.command_id = 0x0108bb93 ; //AUDIO_SET_DEVICE;
        cmd.device_id = device_id; //Device ID
        cmd.network_id = 0x0108B153;
        cmd.sample_rate_id = 48000;
        cmd.interface_id = 0x00011163; //IID of the algorithm block
        cmd.algorithm_block_id = 0x00011162; //ABID of the algorithm block
	cmd.total_bytes = 4; //Size of the physical memory allocated
        cmd.phys_buf = vir_addr; // Driver will override this addr with physical addr

	if (ioctl(fd1, AUDIO_SET_ACDB_BLK, &cmd) < 0) {
                printf("Couldnot set calib block \n");
                close(fd1);
                close(fd2);
                return -1;
        }

	if (ioctl(fd1, AUDIO_DEREGISTER_PMEM, &pmem_info) < 0) {
		printf("Couldnot deregister pmem memory \n");
		close(fd1);
		close(fd2);
		return -1;
	}
	close(fd1);
	close(fd2);
	return 0;
}

static int get_calibration()
{
	int fd1;
	int fd2;
	int result = 0;
	struct acdb_cmd cmd;
	unsigned short *vir_addr = NULL;
	unsigned blk_size = 0;
	struct acdb_gain_rx algo_blk;
	struct msm_audio_pmem_info pmem_info;

	fd1 = open("/dev/msm_acdb", O_RDWR);
	if (fd1 < 0) {
		printf("Failed to open acdb_driver returned %d\n", fd1);
		return -1;
	}

	fd2 = open("/dev/pmem_adsp", O_RDWR);
	if (fd2 < 0) {
		printf("Failed to open pmem driver returned %d\n", fd2);
		close(fd1);
		return -1;
	}

	blk_size = sizeof(struct acdb_gain_rx);
	blk_size = (blk_size + 4095) & (~4095);
	printf("blk_size = %d\n",blk_size);

	vir_addr = (unsigned short *)mmap( NULL,blk_size,
		     PROT_READ | PROT_WRITE,
		     MAP_SHARED,
		     fd2,
		     0 );
	if(MAP_FAILED == vir_addr)
	{
		printf("\n mmap() failed");
		close(fd1);
		close(fd2);
		return -1;
	}

	memset(vir_addr,0,sizeof(struct acdb_gain_rx));
	pmem_info.fd = fd2;
	pmem_info.vaddr = vir_addr;
	if (ioctl(fd1, AUDIO_REGISTER_PMEM, &pmem_info) < 0) {
                printf("Couldnot register pmem memory \n");
                close(fd1);
                close(fd2);
                return -1;
        }


        cmd.command_id = 0x0108bb92 ; //AUDIO_GET_DEVICE;
        cmd.device_id = device_id; //Device ID
        cmd.network_id = 0x0108B153;
        cmd.sample_rate_id = 48000;
        cmd.interface_id = 0x00011163; //IID of the algorithm block
        cmd.algorithm_block_id = 0x00011162; //ABID of the algorithm block
        cmd.total_bytes = 4;  //Size of the physical memory allocated
        cmd.phys_buf = vir_addr; // Driver will override this addr with physical addr

	if (ioctl(fd1, AUDIO_GET_ACDB_BLK, &cmd) < 0) {
                printf("Couldnot set calib block \n");
                close(fd1);
                close(fd2);
                return -1;
        }
	printf("Gain after get ioctl %d \n",*vir_addr++);
	printf("Reserved bit after get ioctl %d \n",*vir_addr);
	if (ioctl(fd1, AUDIO_DEREGISTER_PMEM, &pmem_info) < 0) {
                printf("Couldnot deregister pmem memory \n");
                close(fd1);
                close(fd2);
                return -1;
        }
	close(fd1);
	close(fd2);
	return 0;

}

int main(int argc, char *argv[])
{
	int c;
	int option_index = 0;
	char *ps = NULL;
	int exit_flag = 0;
	int result = 0;

	vol = -1;
	pan = -1;

	while (1) {
		c = getopt_long (argc, argv, "PR:a:m:e:iqv:p:S:G:gtnh", long_options,
								&option_index);

		if (c == -1)
			break;

		switch (c)
		{
		case 'a':
			pp_mask |= ADRC_ENABLE;
			adrc_option = strtol(optarg, &ps, 10);
			if (adrc_option > 5 || adrc_option <= 0) {
				printf("Invalid option entered for ADRC\n");
				print_help_menu();
				return -EINVAL;
			}
			break;

		case 'm':
			pp_mask |= MBADRC_ENABLE;
			mbadrc_option = strtol(optarg, &ps, 10);
			if (mbadrc_option > 5 || mbadrc_option <= 0) {
				printf("Invalid option entered for MBADRC\n");
				print_help_menu();
				return -EINVAL;
			}
			break;

		case 'e':
			pp_mask |= EQ_ENABLE;
			eq_option = strtol(optarg, &ps, 10);
			if (eq_option > 17 || eq_option <= 0) {
				printf("Invalid option entered for EQ\n");
				print_help_menu();
				return -EINVAL;
			}
			break;

		case 'i':
			pp_mask |= IIR_ENABLE;
			break;

		case 'q':
			pp_mask |= QCONCERT_PLUS_ENABLE;
			break;

		case 'v':
			vol = strtol(optarg, &ps, 16);
			break;

		case 'p':
			pan = strtol(optarg, &ps, 16);
			break;

		case 'g':
			pre_mask |= AGC_ENABLE;
			break;

		case 't':
			pre_mask |= TX_IIR_ENABLE;
			break;

		case 'n':
			pre_mask |= NS_ENABLE;
			break;

		case '?':
		case ':':
			print_help_menu();
			return -EINVAL;

		case 'h':
			print_help_menu();
			return 0;

		case 'P':
			pp_flag = 1;
			printf("Selected post processing tests\n");
			break;

		case 'R':
			pre_flag = 1;
			samp_rate = strtol(optarg, &ps, 16);
			printf("Selected pre processing tests\n");
			break;
		case 'S':
			#ifdef AUDIOV2
				printf("Selected option to set acdb block\n");
				device_id = strtol(optarg, &ps, 10);
				result = set_calibration();
				if(result == 0)
					printf("Calibration block was set\n");
				else
				{
					printf("Failed to set calibration block\n");
					return result;
				}
			#else
				printf("Invalid option -> This option valid only for 7x30 target\n");
				return -1;
			#endif
			return 0;
		case 'G':
			#ifdef AUDIOV2
				printf("Selected option to get acdb block\n");
				device_id = strtol(optarg, &ps, 10);
				result = get_calibration();
				if(result == 0)
					printf("Got calibration block\n");
				else
				{
					printf("Failed to get calibration block\n");
					return result;
				}
			#else
				printf("Invalid option -> This option valid only for 7x30 target\n");
				return -1;
			#endif
			return 0;
		default:
			break;
		}
	}

	if (pp_flag) {
		if (pre_mask) {
			printf("Selection of Post and Pre processing in single run not allowed\n");
			print_help_menu();
			return -EINVAL;
		}
		if ((pp_mask & ADRC_ENABLE) && (pp_mask & MBADRC_ENABLE)) {
			printf("Not allowed to select both ADRC & MBADRC in single test. Select only one of them\n");
			print_help_menu();
			return -EINVAL;
		}
		printf("pp_mask = %d\n", pp_mask);
		test_postprocessing(pp_mask);
	} else if (pre_flag) {
		if (pp_mask || vol != -1 || pan != -1) {
			printf("Selection of Post and Pre processing in single run not allowed\n");
			print_help_menu();
			return -EINVAL;
		}
		printf("pre_mask and samp_rate= %d %x\n", pre_mask, samp_rate);
		test_preprocessing(pre_mask);
	} else {
		printf("None(Pre or Post) selected for testing\n");
		print_help_menu();
		return -EINVAL;
	}

	return 0;
}
