/*
 *
 * Copyright (C) 2009 Qualcomm Technologies, Inc.
 *
 */

#ifndef ATUI_H
#define ATUI_H

#define ATUI_SUCCESS	 1
#define ATUI_FAILURE	-1

#define ATU_DTMF_GRAIN   10		
#define ATU_MAX_PERIOD	(0xFFFF - ATU_DTMF_GRAIN)

/* Type of Tones */
enum atu_type {
	ATU_COMPACT_SOUND,	/* Designates a sound using tones in the data base */
	ATU_FLEXIBLE_SOUND,	/* Sound that use freq. pairs to define tones */
	ATU_INVALID_SOUND	/* Used to indicate an invalid sound type */
};

#define ATU_FIRST_CONTROL_TONE	0
#define ATU_FIRST_AUDIO_TONE	100

enum atu_audio_tones {
	ATU_FREQ = ATU_FIRST_AUDIO_TONE,
	ATU_LAST_AUDIO_TONE
};

enum atu_control_tones {
	ATU_SILENCE = ATU_FIRST_CONTROL_TONE,
	/* Callback repeat  Silence-no tone at all */
	ATU_CALLBACK_SILENCE,
	/* Stop playing list(last item in multi-tone) */
	ATU_STOP,
	/* Repeat list (last item in a multi-tone) */
	ATU_RPT,
	/* Repeat the previous tone */
	ATU_RPT1,
	/* Repeat list - do not call callback */
	ATU_RPT_NOCALLBACK,
	/* Loop back 2 items, use duration as a loop count */
	ATU_LOOP_BACK2,
	/* Label for looping */
	ATU_LABEL,
	/* Back to label use duration as a loop count */
	ATU_BACK_TO_LABEL,
	/* Adjust volume level for a tone */
	ATU_VOL_SCALE,
	ATU_LAST_CONTROL_TONE
};

union atu_tone_data_type {
	unsigned short	duration_ms;	/* Duration in milliseconds */
	unsigned short	data;		/* general purpose data */
};

struct atu_compact_tone_type {
	unsigned short			tone;	/* Tone/DTMF to generate */
	union atu_tone_data_type	param;	/* Tone data */
};

struct atu_flexible_tone_type {
	/* Tone/DTMF to generate */
	unsigned short			tone;
	/* Tone data */
	union atu_tone_data_type	param;
	/* High frequency value in hz */
	unsigned short			freq_hi;
	/* Low frequency value in hz */
	unsigned short			freq_lo;
};

struct atu_compact_sound_type {
	/* Type = ATU_COMPACT_SOUND */
	enum atu_type			type;
	/* Pointer to the array of tones */
	struct atu_compact_tone_type	*tone_array;
};

struct  atu_flexible_sound_type {
	/* Type = ATU_FLEXIBLE_SOUND        */
	enum atu_type			type;
	/* Pointer to the array of tones    */
	struct atu_flexible_tone_type	*tone_array;
};

union atu_sound_type {
	enum atu_type			type;
	struct atu_compact_sound_type	compact_sound;
	struct atu_flexible_sound_type	flexible_sound;
};

struct atu_tdb_dtmf_type {
	unsigned short	hi;	/* High Freq associated with DTMF */
	unsigned short	lo;	/* Low Freq associated with DTMF */
};


/* has to match cad_cmd_gen_dtmf struct */
struct msm_gen_dtmf {
	unsigned short path;
	unsigned short dtmf_hi;
	unsigned short dtmf_low;
	unsigned short duration;
	unsigned short tx_gain;
	unsigned short rx_gain;
	unsigned short mixing;
};

unsigned short atu_db_get_sound(unsigned short index, 
			  const union atu_sound_type **sound);

unsigned short atu_tdb_get_tone_freq(unsigned short tone,
		struct atu_tdb_dtmf_type *dtmf);

#endif
