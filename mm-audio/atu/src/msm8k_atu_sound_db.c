/*
 *
 * Copyright (C) 2009 Qualcomm Technologies, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "msm8k_atui.h"
#include "msm8k_atu_sound_db_id.h"
#include "msm8k_atu_tone_db_id.h"

const struct atu_compact_tone_type ring_buf[] = {    /* Phone Ringing (Alert) */
	{ATU_RING_A,            25},
	{ATU_RING_B,            25},
	{ATU_LOOP_BACK2,        39},                 /* 2 seconds on  */
	{ATU_CALLBACK_SILENCE,4000},                 /* 4 seconds off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type ring_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) ring_buf
};

const struct atu_compact_tone_type pwrup_buf[] = {   /* Wake-up/Power-up sound */
	{ATU_SILENCE,          500},
	{ATU_PWRUP,            500},
	{ATU_STOP,               0}
};

const struct atu_compact_sound_type pwrup_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) pwrup_buf
};

/* Sounds for EIA/TIA Wideband Spread Spectrum Standard section 7.7.5.5
*/

/* Table 7.7.5.5-3, Tone Signals -  -  -  -  -  -  -  -  -  -  -  -  -  -
** (mostly call progress tones)
*/

const struct atu_compact_tone_type dial_tone_buf[] = {   /* Dial tone */
	{ATU_DIAL_TONE_TONE,  1000},
	{ATU_RPT,                0}
};

const struct atu_compact_sound_type dial_tone_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) dial_tone_buf
};

const struct atu_compact_tone_type call_buf[] = {    /* Ringback, audible ring */
	{ATU_RBACK,           2000},                 /* 2 seconds on  */
	{ATU_SILENCE,         4000},                 /* 4 seconds off */
	{ATU_RPT,                0}
};

const struct atu_compact_sound_type call_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) call_buf
};

const struct atu_compact_tone_type int_buf[] = {     /* Intercept              */
	{ATU_INTERCEPT_A,      250},
	{ATU_INTERCEPT_B,      250},
	{ATU_RPT,                0}
};

const struct atu_compact_sound_type int_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) int_buf
};

const struct atu_compact_tone_type abrv_int_buf[] = {/* Intercept              */
	{ATU_INTERCEPT_A,      250},
	{ATU_INTERCEPT_B,      250},
	{ATU_LOOP_BACK2,         7},                 /* 4 Seconds              */
	{ATU_STOP,               0}
};

const struct atu_compact_sound_type abrv_int_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) abrv_int_buf
};

const struct atu_compact_tone_type reord_buf[] = {   /* Reorder                */
	{ATU_REORDER_TONE,     250},                 /*   (Network congestion) */
	{ATU_SILENCE,          250},
	{ATU_RPT,                0}
};

const struct atu_compact_sound_type reord_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) reord_buf
};

const struct atu_compact_tone_type abrv_reord_buf[] = { /* Abbreviated Reorder */
	{ATU_REORDER_TONE,     250},                 /*   (Network congestion) */
	{ATU_SILENCE,          250},
	{ATU_LOOP_BACK2,         7},                 /* 4 seconds              */
	{ATU_STOP,               0}
};

const struct atu_compact_sound_type abrv_reord_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) abrv_reord_buf
};

const struct atu_compact_tone_type busy_buf[] = {    /* Abbrev. Alert (busy)   */
	{ATU_BUSY,             500},
	{ATU_SILENCE,          500},
	{ATU_RPT,                0}
};

const struct atu_compact_sound_type busy_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) busy_buf
};

const struct atu_compact_tone_type confirm_tone_buf[] = { /* Confirmation Tone */
	{ATU_DIAL_TONE_TONE,   100},
	{ATU_SILENCE,          100},
	{ATU_LOOP_BACK2,         2},                  /* 3 notes */
	{ATU_STOP,               0}
};

const struct atu_compact_sound_type confirm_tone_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) confirm_tone_buf
};

/* Answer tone (European call progress signal, use unknown) */
const struct atu_compact_tone_type answer_buf[] = {   /* Answer tone           */
	{ATU_ANSWER_TONE,      500},
	{ATU_STOP,               0}
};

const struct atu_compact_sound_type answer_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) answer_buf
};

const struct atu_compact_tone_type call_waiting_buf[] = { /* Call Waiting      */
	{ATU_CALL_WT_TONE,     200},
	{ATU_STOP,               0}
};

const struct atu_compact_sound_type call_waiting_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) call_waiting_buf
};

const struct atu_compact_tone_type off_hook_buf[] = { /* Off-Hook Warning      */
	{ATU_OFF_HOOK_TONE,    100},
	{ATU_SILENCE,          100},
	{ATU_RPT,                0}
};

const struct atu_compact_sound_type off_hook_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) off_hook_buf
};

/* Table 7.7.5.5-4, ISDN Alerting -  -  -  -  -  -  -  -  -  -  -  -  -  -
*/

const struct atu_compact_tone_type normal_alert_buf[] = { /* Normal Alerting   */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,        39},                  /* 2 seconds on  */
	{ATU_CALLBACK_SILENCE,4000},                  /* 4 seconds off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type normal_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) normal_alert_buf
};

/* Intergroup Alerting    */
const struct atu_compact_tone_type intergroup_alert_buf[] = {
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,        15}, /* 800 ms on  */
	{ATU_CALLBACK_SILENCE, 400}, /* 400 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type intergroup_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) intergroup_alert_buf
};

/* Special/Priority Alerting */
const struct atu_compact_tone_type special_alert_buf[] = { 
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,        15}, /* 800 ms on  */
	{ATU_SILENCE,          200}, /* 200 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,        15}, /* 800 ms on  */
	{ATU_SILENCE,          200}, /* 200 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,        31}, /* 1600 ms on  */
	{ATU_CALLBACK_SILENCE,4000}, /* 4000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type special_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) special_alert_buf
};

const struct atu_compact_tone_type ping_ring_buf[] = {   /* "Ping ring"          */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         9},                 /* 500 ms on */
	{ATU_STOP,               0}
};

const struct atu_compact_sound_type ping_ring_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) ping_ring_buf
};

/* Table 7.7.5.5-5, IS-54B Alerting (High/Med/Low Tone) -  -  -  -  -  -  -  -
**
** One entry per pitch (High/Medium/Low) per pattern type (11 types)
**   (12 Types with IS-53A Addition)
**
*/

const struct atu_compact_tone_type is54b_long_h_alert_buf[] = {  /* Long      */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,        39},                         /* 2 seconds on  */
	{ATU_CALLBACK_SILENCE,4000},                         /* 4 seconds off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_long_h_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_long_h_alert_buf
};

const struct atu_compact_tone_type is54b_ss_h_alert_buf[] = {/* Short-short   */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,        15},                         /* 800 ms on     */
	{ATU_CALLBACK_SILENCE, 400},                         /* 400 ms off    */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_ss_h_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_ss_h_alert_buf
};

const struct atu_compact_tone_type is54b_ssl_h_alert_buf[] = {/* Short-short-long */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         7},                         /*  400 ms on     */
	{ATU_SILENCE,          200},                         /*  200 ms off    */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         7},                         /*  400 ms on     */
	{ATU_SILENCE,          200},                         /*  200 ms off    */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,        15},                         /*  800 ms on     */
	{ATU_CALLBACK_SILENCE,4000},                         /* 4000 ms off    */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_ssl_h_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_ssl_h_alert_buf
};

const struct atu_compact_tone_type is54b_ss2_h_alert_buf[] = {/* Short-short-2    */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,        19}, /* 1000 ms on  */
	{ATU_SILENCE,         1000}, /* 1000 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,        19}, /* 1000 ms on  */
	{ATU_CALLBACK_SILENCE,3000}, /* 3000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_ss2_h_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_ss2_h_alert_buf
};

const struct atu_compact_tone_type is54b_sls_h_alert_buf[] = {/* Short-long-short */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_SILENCE,          500}, /*  500 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,        19}, /* 1000 ms on  */
	{ATU_SILENCE,          500}, /*  500 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_CALLBACK_SILENCE,3000}, /* 3000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_sls_h_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_sls_h_alert_buf
};

/* Short-short-short-short */
const struct atu_compact_tone_type is54b_ssss_h_alert_buf[] = {
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_SILENCE,          500}, /*  500 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_SILENCE,          500}, /*  500 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_SILENCE,          500}, /*  500 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_CALLBACK_SILENCE,2500}, /* 2500 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_ssss_h_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_ssss_h_alert_buf
};

const struct atu_compact_tone_type is54b_pbx_long_h_alert_buf[] = { /* PBX Long  */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,        19}, /* 1000 ms on  */
	{ATU_CALLBACK_SILENCE,2000}, /* 2000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_pbx_long_h_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_pbx_long_h_alert_buf
};

/* PBX Short-short */
const struct atu_compact_tone_type is54b_pbx_ss_h_alert_buf[] = {
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_SILENCE,          200}, /* 200 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_CALLBACK_SILENCE,2000}, /* 2000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_pbx_ss_h_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_pbx_ss_h_alert_buf
};

/* PBX Short-short-long */
const struct atu_compact_tone_type is54b_pbx_ssl_h_alert_buf[] = {
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,        15}, /*  800 ms on  */
	{ATU_CALLBACK_SILENCE,1000}, /* 1000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_pbx_ssl_h_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_pbx_ssl_h_alert_buf
};

/* PBX Short-long-short */
const struct atu_compact_tone_type is54b_pbx_sls_h_alert_buf[] = {
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,        15}, /*  800 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_CALLBACK_SILENCE,1000}, /* 1000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_pbx_sls_h_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_pbx_sls_h_alert_buf
};

/* PBX Short-short-short-short  */
const struct atu_compact_tone_type is54b_pbx_ssss_h_alert_buf[] = {
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_SILENCE,          200}, /* 200 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_SILENCE,          200}, /* 200 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_SILENCE,          200}, /* 200 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_CALLBACK_SILENCE, 800}, /* 800 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_pbx_ssss_h_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_pbx_ssss_h_alert_buf
};

/* Pip-Pip-Pip-Pip */
const struct atu_compact_tone_type is53a_pppp_h_alert_buf[] = {
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         1}, /* 100 ms on  */
	{ATU_SILENCE,          100}, /* 100 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         1}, /* 100 ms on  */
	{ATU_SILENCE,          100}, /* 100 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         1}, /* 100 ms on  */
	{ATU_SILENCE,          100}, /* 100 ms off */
	{ATU_HIGH_PITCH_A,      25},
	{ATU_HIGH_PITCH_B,      25},
	{ATU_LOOP_BACK2,         1}, /* 100 ms on  */
	{ATU_CALLBACK_SILENCE, 100}, /* 100 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is53a_pppp_h_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is53a_pppp_h_alert_buf
};

const struct atu_compact_tone_type is54b_long_m_alert_buf[] = {  /* Long    */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,        39}, /* 2 seconds on  */
	{ATU_CALLBACK_SILENCE,4000}, /* 4 seconds off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_long_m_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_long_m_alert_buf
};

const struct atu_compact_tone_type is54b_ss_m_alert_buf[] = { /* Short-short  */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,        15}, /* 800 ms on  */
	{ATU_CALLBACK_SILENCE, 400}, /* 400 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_ss_m_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_ss_m_alert_buf
};

const struct atu_compact_tone_type is54b_ssl_m_alert_buf[] = {/* Short-short-long */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,        15}, /*  800 ms on  */
	{ATU_CALLBACK_SILENCE,4000}, /* 4000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_ssl_m_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_ssl_m_alert_buf
};

const struct atu_compact_tone_type is54b_ss2_m_alert_buf[] = {/* Short-short-2   */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,        19}, /* 1000 ms on  */
	{ATU_SILENCE,         1000}, /* 1000 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,        19}, /* 1000 ms on  */
	{ATU_CALLBACK_SILENCE,3000}, /* 3000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_ss2_m_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_ss2_m_alert_buf
};

const struct atu_compact_tone_type is54b_sls_m_alert_buf[] = {/* Short-long-short */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_SILENCE,          500}, /*  500 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,        19}, /* 1000 ms on  */
	{ATU_SILENCE,          500}, /*  500 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_CALLBACK_SILENCE,3000}, /* 3000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_sls_m_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_sls_m_alert_buf
};

/* Short-short-short-short */
const struct atu_compact_tone_type is54b_ssss_m_alert_buf[] = {
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_SILENCE,          500}, /*  500 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_SILENCE,          500}, /*  500 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_SILENCE,          500}, /*  500 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_CALLBACK_SILENCE,2500}, /* 2500 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_ssss_m_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_ssss_m_alert_buf
};

const struct atu_compact_tone_type is54b_pbx_long_m_alert_buf[] = { /* PBX Long  */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,        19}, /* 1000 ms on  */
	{ATU_CALLBACK_SILENCE,2000}, /* 2000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_pbx_long_m_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_pbx_long_m_alert_buf
};

/* PBX Short-short */
const struct atu_compact_tone_type is54b_pbx_ss_m_alert_buf[] = {
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_SILENCE,          200}, /* 200 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_CALLBACK_SILENCE,2000}, /* 2000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_pbx_ss_m_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_pbx_ss_m_alert_buf
};

/* PBX Short-short-long */
const struct atu_compact_tone_type is54b_pbx_ssl_m_alert_buf[] = {
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,        15}, /*  800 ms on  */
	{ATU_CALLBACK_SILENCE,1000}, /* 1000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_pbx_ssl_m_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_pbx_ssl_m_alert_buf
};

/* PBX Short-long-short */
const struct atu_compact_tone_type is54b_pbx_sls_m_alert_buf[] = { 
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,        15}, /*  800 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_CALLBACK_SILENCE,1000}, /* 1000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_pbx_sls_m_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_pbx_sls_m_alert_buf
};

/* PBX Short-short-short-short */
const struct atu_compact_tone_type is54b_pbx_ssss_m_alert_buf[] = {
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_SILENCE,          200}, /* 200 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_SILENCE,          200}, /* 200 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_SILENCE,          200}, /* 200 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_CALLBACK_SILENCE, 800}, /* 800 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_pbx_ssss_m_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_pbx_ssss_m_alert_buf
};

/* Pip-Pip-Pip-Pip  */
const struct atu_compact_tone_type is53a_pppp_m_alert_buf[] = {
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         1}, /* 100 ms on  */
	{ATU_SILENCE,          100}, /* 100 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         1}, /* 100 ms on  */
	{ATU_SILENCE,          100}, /* 100 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         1}, /* 100 ms on  */
	{ATU_SILENCE,          100}, /* 100 ms off */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         1}, /* 100 ms on  */
	{ATU_CALLBACK_SILENCE, 100}, /* 100 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is53a_pppp_m_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is53a_pppp_m_alert_buf
};

const struct atu_compact_tone_type is54b_long_l_alert_buf[] = {  /* Long      */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,        39}, /* 2 seconds on  */
	{ATU_CALLBACK_SILENCE,4000}, /* 4 seconds off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_long_l_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_long_l_alert_buf
};

const struct atu_compact_tone_type is54b_ss_l_alert_buf[] = { /* Short-short   */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,        15}, /* 800 ms on  */
	{ATU_CALLBACK_SILENCE, 400}, /* 400 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_ss_l_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_ss_l_alert_buf
};

const struct atu_compact_tone_type is54b_ssl_l_alert_buf[] = {/* Short-short-long */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,        15}, /*  800 ms on  */
	{ATU_CALLBACK_SILENCE,4000}, /* 4000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_ssl_l_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_ssl_l_alert_buf
};

const struct atu_compact_tone_type is54b_ss2_l_alert_buf[] = {/* Short-short-2    */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,        19}, /* 1000 ms on  */
	{ATU_SILENCE,         1000}, /* 1000 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,        19}, /* 1000 ms on  */
	{ATU_CALLBACK_SILENCE,3000}, /* 3000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_ss2_l_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_ss2_l_alert_buf
};

const struct atu_compact_tone_type is54b_sls_l_alert_buf[] = {/* Short-long-short */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_SILENCE,          500}, /*  500 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,        19}, /* 1000 ms on  */
	{ATU_SILENCE,          500}, /*  500 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_CALLBACK_SILENCE,3000}, /* 3000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_sls_l_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_sls_l_alert_buf
};

/* Short-short-short-short */
const struct atu_compact_tone_type is54b_ssss_l_alert_buf[] = { 
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_SILENCE,          500}, /*  500 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_SILENCE,          500}, /*  500 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_SILENCE,          500}, /*  500 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         9}, /*  500 ms on  */
	{ATU_CALLBACK_SILENCE,2500}, /* 2500 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_ssss_l_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_ssss_l_alert_buf
};

const struct atu_compact_tone_type is54b_pbx_long_l_alert_buf[] = { /* PBX Long */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,        19}, /* 1000 ms on  */
	{ATU_CALLBACK_SILENCE,2000}, /* 2000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_pbx_long_l_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_pbx_long_l_alert_buf
};

/* PBX Short-short */
const struct atu_compact_tone_type is54b_pbx_ss_l_alert_buf[] = {
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_SILENCE,          200}, /* 200 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_CALLBACK_SILENCE,2000}, /* 2000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_pbx_ss_l_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_pbx_ss_l_alert_buf
};

/* PBX Short-short-long */
const struct atu_compact_tone_type is54b_pbx_ssl_l_alert_buf[] = { 
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,        15}, /*  800 ms on  */
	{ATU_CALLBACK_SILENCE,1000}, /* 1000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_pbx_ssl_l_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_pbx_ssl_l_alert_buf
};

/* PBX Short-long-short */
const struct atu_compact_tone_type is54b_pbx_sls_l_alert_buf[] = {
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,        15}, /*  800 ms on  */
	{ATU_SILENCE,          200}, /*  200 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /*  400 ms on  */
	{ATU_CALLBACK_SILENCE,1000}, /* 1000 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_pbx_sls_l_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_pbx_sls_l_alert_buf
};

/* PBX Short-short-short-short */
const struct atu_compact_tone_type is54b_pbx_ssss_l_alert_buf[] = {
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_SILENCE,          200}, /* 200 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_SILENCE,          200}, /* 200 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_SILENCE,          200}, /* 200 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         7}, /* 400 ms on  */
	{ATU_CALLBACK_SILENCE, 800}, /* 800 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is54b_pbx_ssss_l_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is54b_pbx_ssss_l_alert_buf
};

/* Pip-Pip-Pip-Pip  */
const struct atu_compact_tone_type is53a_pppp_l_alert_buf[] = { 
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         1}, /* 100 ms on  */
	{ATU_SILENCE,          100}, /* 100 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         1}, /* 100 ms on  */
	{ATU_SILENCE,          100}, /* 100 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         1}, /* 100 ms on  */
	{ATU_SILENCE,          100}, /* 100 ms off */
	{ATU_LOW_PITCH_A,       25},
	{ATU_LOW_PITCH_B,       25},
	{ATU_LOOP_BACK2,         1}, /* 100 ms on  */
	{ATU_CALLBACK_SILENCE, 100}, /* 100 ms off */
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type is53a_pppp_l_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) is53a_pppp_l_alert_buf
};

/* Additional Alerts -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
*/

const struct atu_compact_tone_type fade_tone_buf[] = { /* Fade tone            */
	{ATU_SILENCE,          150}, /* 150ms delay on fade start for voc reset */
	{ATU_SILENCE,           25},
	{ATU_LOW_PITCH_A,      120},
	{ATU_LOOP_BACK2,         3}, /* 4 notes */
	{ATU_STOP,               0}
};

const struct atu_compact_sound_type fade_tone_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) fade_tone_buf
};

const struct atu_compact_tone_type svc_change_buf[] = { /* Service change tone */
	{ATU_SILENCE,           25},
	{ATU_MED_PITCH_A,      120},
	{ATU_LOOP_BACK2,         1}, /* 2 notes */
	{ATU_STOP,               0}
};

const struct atu_compact_sound_type svc_change_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) svc_change_buf
};

/* changed durations from 250 to 175 and number from 5 to 4 */
const struct atu_compact_tone_type message_waiting_buf[] = { /* Msg Waiting Alert    */
	{ATU_MSG_WAITING,      175},
	{ATU_SILENCE,          175},
	{ATU_LOOP_BACK2,         3},                   /* Four Beeps  */
	{ATU_STOP,               0}
};

const struct atu_compact_sound_type message_waiting_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) message_waiting_buf
};

const struct atu_compact_tone_type abrv_alert_buf[] = { /* Abreviate Alert  */
	{ATU_MED_PITCH_A,       25},
	{ATU_MED_PITCH_B,       25},
	{ATU_LOOP_BACK2,         9}, /* 500 ms on */
	{ATU_STOP,               0}
};

const struct atu_compact_sound_type abrv_alert_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) abrv_alert_buf
};

const struct atu_compact_tone_type pip_tone_buf[] = { /* Pip-Pip-Pip-Pip  */
	{ATU_PIP_TONE_TONE,    100}, /* 100 ms on  */
	{ATU_SILENCE,          100}, /* 100 ms off */
	{ATU_LOOP_BACK2,         3}, /* Four Beeps */
	{ATU_STOP,               0}
};

const struct atu_compact_sound_type pip_tone_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) pip_tone_buf
};

/* Any new sound tables will be defined here */

/* Sound for call-failed or reorder for some configurations.
*/
const struct atu_compact_tone_type alternate_reorder_buf[] = {
	{ATU_INTERCEPT_A,       90},
	{ATU_SILENCE,           90},
	{ATU_LOOP_BACK2,         2},
	{ATU_SILENCE,          710},
	{ATU_INTERCEPT_A,       90},
	{ATU_SILENCE,           90},
	{ATU_LOOP_BACK2,         2},
	{ATU_STOP,               0}
};

const struct atu_compact_sound_type alternate_reorder_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) alternate_reorder_buf
};

/* DTACO roam tone        */
const struct atu_compact_tone_type dtaco_roam_tone_buf[] = {    
	{ATU_DIAL_TONE_TONE,    1100},
	{ATU_DT_DTACO_TONE,       60},
	{ATU_DIAL_TONE_TONE,     900},
	{ATU_RPT_NOCALLBACK,       0}
};

const struct atu_compact_sound_type dtaco_roam_tone_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) dtaco_roam_tone_buf
};

/* Sound tables defining user-selectable rings
*/
const struct atu_compact_tone_type user_ring_1_buf[] = {
// The first tone is a zero duration silence to compensate for a bug in ATU
// that does not see control tones when they are first.
	{ATU_SILENCE,            0},
	{ATU_LABEL,              0},
	{ATU_RING_A,            20},
	{ATU_SILENCE,           20},
	{ATU_RING_B,            20},
	{ATU_SILENCE,          340},
	{ATU_BACK_TO_LABEL,      3},
	{ATU_CALLBACK_SILENCE,2400},
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type user_ring_1_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) user_ring_1_buf
};

const struct atu_compact_tone_type user_ring_2_buf[] = {
// The first tone is a zero duration silence to compensate for a bug in ATU
// that does not see control tones when they are first.
	{ATU_SILENCE,            0},
	{ATU_LABEL,              0},
	{ATU_RING_A,            20},
	{ATU_SILENCE,           20},
	{ATU_RING_B,            20},
	{ATU_SILENCE,           20},
	{ATU_BACK_TO_LABEL,      3},
	{ATU_SILENCE,          645},
	{ATU_LABEL,              0},
	{ATU_RING_A,            20},
	{ATU_SILENCE,           20},
	{ATU_RING_B,            20},
	{ATU_SILENCE,           20},
	{ATU_BACK_TO_LABEL,      3},
	{ATU_SILENCE,          645},
	{ATU_LABEL,              0},
	{ATU_RING_A,            20},
	{ATU_SILENCE,           20},
	{ATU_RING_B,            20},
	{ATU_SILENCE,           20},
	{ATU_BACK_TO_LABEL,      3},
	{ATU_SILENCE,          645},
	{ATU_LABEL,              0},
	{ATU_RING_A,            20},
	{ATU_SILENCE,           20},
	{ATU_RING_B,            20},
	{ATU_SILENCE,           20},
	{ATU_BACK_TO_LABEL,      3},
	{ATU_CALLBACK_SILENCE,3750},
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type user_ring_2_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) user_ring_2_buf
};

const struct atu_compact_tone_type user_ring_3_buf[] = {
// The first tone is a zero duration silence to compensate for a bug in ATU
// that does not see control tones when they are first.
	{ATU_SILENCE,            0},
	{ATU_LABEL,              0},
	{ATU_RING_A,            30},
	{ATU_SILENCE,           20},
	{ATU_RING_B,            30},
	{ATU_SILENCE,           20},
	{ATU_RING_C,            30},
	{ATU_SILENCE,           20},
	{ATU_BACK_TO_LABEL,      5},
	{ATU_CALLBACK_SILENCE,2000},
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type user_ring_3_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) user_ring_3_buf
};

const struct atu_compact_tone_type user_ring_4_buf[] = { 
// The first tone is a zero duration silence to compensate for a bug in ATU
// that does not see control tones when they are first.
	{ATU_SILENCE,            0},
	{ATU_LABEL,              0},
	{ATU_RING_A,            30},
	{ATU_SILENCE,           20},
	{ATU_RING_C,            30},
	{ATU_SILENCE,           20},
	{ATU_RING_B,            30},
	{ATU_SILENCE,           20},
	{ATU_RING_C,            30},
	{ATU_SILENCE,          240},
	{ATU_BACK_TO_LABEL,      1},
	{ATU_SILENCE,         1120},
	{ATU_LABEL,              0},
	{ATU_RING_A,            30},
	{ATU_SILENCE,           20},
	{ATU_RING_C,            30},
	{ATU_SILENCE,           20},
	{ATU_RING_B,            30},
	{ATU_SILENCE,           20},
	{ATU_RING_C,            30},
	{ATU_SILENCE,          240},
	{ATU_BACK_TO_LABEL,      1},
	{ATU_CALLBACK_SILENCE,2760},
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type user_ring_4_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) user_ring_4_buf
};

/* Sound tables defining user-selectable rings that use volume scaling.
*/
const struct atu_compact_tone_type user_ring_5_buf[] = { 
// The first tone is a zero duration silence to compensate for a bug in ATU
// that does not see control tones when they are first.
 { ATU_SILENCE,            0},
 { ATU_LABEL,              0},
 { ATU_VOL_SCALE,          0},
 { ATU_RING_E6,           60},
 { ATU_VOL_SCALE,          1},
 { ATU_RING_A7,           60},
 { ATU_VOL_SCALE,          0},
 { ATU_RING_E6,           60},
 { ATU_VOL_SCALE,          1},
 { ATU_RING_A7,           60},
 { ATU_VOL_SCALE,          1},
 { ATU_RING_E6,           60},
 { ATU_VOL_SCALE,          2},
 { ATU_RING_A7,           60},
 { ATU_VOL_SCALE,          1},
 { ATU_RING_E6,           60},
 { ATU_VOL_SCALE,          2},
 { ATU_RING_A7,           60},
 { ATU_VOL_SCALE,          3},
 { ATU_RING_E6,           60},
 { ATU_VOL_SCALE,          3},
 { ATU_RING_A7,           60},
 { ATU_VOL_SCALE,          3},
 { ATU_RING_E6,           60},
 { ATU_VOL_SCALE,          3},
 { ATU_RING_A7,           60},
 { ATU_VOL_SCALE,          0},
 { ATU_BACK_TO_LABEL,      1},
 { ATU_CALLBACK_SILENCE,2760},
 { ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type user_ring_5_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) user_ring_5_buf
};

const struct atu_compact_tone_type user_ring_6_buf[] = {
// The first tone is a zero duration silence to compensate for a bug in ATU
// that does not see control tones when they are first.
	{ATU_SILENCE,            0},
	{ATU_LABEL,              0},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_D,           100},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_A7,          100},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_D,           100},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_A7,          100},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_D,           100},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_A7,          100},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_D,           100},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_A7,          100},
	{ATU_VOL_SCALE,          3},
	{ATU_RING_D,           100},
	{ATU_VOL_SCALE,          3},
	{ATU_RING_A7,          100},
	{ATU_VOL_SCALE,          3},
	{ATU_RING_D,           100},
	{ATU_VOL_SCALE,          3},
	{ATU_RING_A7,          100},
	{ATU_BACK_TO_LABEL,      1},
	{ATU_CALLBACK_SILENCE,2760},
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type user_ring_6_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) user_ring_6_buf
};

const struct atu_compact_tone_type user_ring_7_buf[] = {
// The first tone is a zero duration silence to compensate for a bug in ATU
// that does not see control tones when they are first.
	{ATU_SILENCE,            0},
	{ATU_LABEL,              0},
	{ATU_VOL_SCALE,          3},
	{ATU_RING_C5,           60},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_E5,           60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_G5,           60},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_C6,           60},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_E6,           60},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_C6,           60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_G5,           60},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_E5,           60},
	{ATU_VOL_SCALE,          3},
	{ATU_RING_C5,           60},
	{ATU_BACK_TO_LABEL,      1},
	{ATU_CALLBACK_SILENCE,2760},
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type user_ring_7_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) user_ring_7_buf
};

const struct atu_compact_tone_type user_ring_8_buf[] = {
// The first tone is a zero duration silence to compensate for a bug in ATU
// that does not see control tones when they are first.
	{ATU_SILENCE,            0},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_C4,           60},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_CS4,          60},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_D4,           60},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_DS4,          60},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_E4,           60},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_F4,           60},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_G4,           60},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_GS4,          60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_C5,           60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_CS5,          60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_D5,           60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_DS5,          60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_E5,           60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_F5,           60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_G5,           60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_GS5,          60},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_C6,           60},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_D6,           60},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_DS6,          60},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_E6,           60},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_F6,           60},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_G6,           60},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_GS6,          60},
	{ATU_CALLBACK_SILENCE,2760},
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type user_ring_8_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) user_ring_8_buf
};

const struct atu_compact_tone_type user_ring_9_buf[] = {
// The first tone is a zero duration silence to compensate for a bug in ATU
// that does not see control tones when they are first.
	{ATU_SILENCE,            0},
	{ATU_LABEL,              0},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_E6,           60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_A7,           60},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_E6,           60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_A7,           60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_E6,           60},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_A7,           60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_E6,           60},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_A7,           60},
	{ATU_VOL_SCALE,          3},
	{ATU_RING_E6,           60},
	{ATU_VOL_SCALE,          3},
	{ATU_RING_A7,           60},
	{ATU_VOL_SCALE,          3},
	{ATU_RING_E6,           60},
	{ATU_VOL_SCALE,          3},
	{ATU_RING_A7,           60},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_D,            60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_A7,           60},
	{ATU_VOL_SCALE,          0},
	{ATU_RING_D,            60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_A7,           60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_D,            60},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_A7,           60},
	{ATU_VOL_SCALE,          1},
	{ATU_RING_D,            60},
	{ATU_VOL_SCALE,          2},
	{ATU_RING_A7,           60},
	{ATU_VOL_SCALE,          3},
	{ATU_RING_D,            60},
	{ATU_VOL_SCALE,          3},
	{ATU_RING_A7,           60},
	{ATU_VOL_SCALE,          3},
	{ATU_RING_D,            60},
	{ATU_VOL_SCALE,          3},
	{ATU_RING_A7,           60},
	{ATU_BACK_TO_LABEL,      1},
	{ATU_CALLBACK_SILENCE,2760},
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type user_ring_9_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) user_ring_9_buf
};

const struct atu_compact_tone_type roam_ring_1_buf[]= {
	{ATU_SILENCE,            0},
	{ATU_LABEL,              0},
	{ATU_RING_A,            30},
	{ATU_RING_B,            30},
	{ATU_SILENCE,           20},
	{ATU_RING_A,            30},
	{ATU_RING_C,            30},
	{ATU_SILENCE,           20},
	{ATU_LOOP_BACK2,         2},
	{ATU_BACK_TO_LABEL,      1},
	{ATU_CALLBACK_SILENCE,2000},
	{ATU_RPT_NOCALLBACK,     0}
};

const struct atu_compact_sound_type roam_ring_1_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) roam_ring_1_buf
};

/* These are Service Transition Alerts.
*/
/* Entering service alert          */
const struct atu_compact_tone_type atu_svc_acq_buf[] = {  
	{ATU_RING_E4,          80},
	{ATU_SILENCE,          40},
	{ATU_RING_E4,          80},        
	{ATU_SILENCE,          40},
	{ATU_RING_G4,          80},
	{ATU_STOP,              0}
};

const struct atu_compact_sound_type atu_svc_acq_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) atu_svc_acq_buf
};

/* Leaving service alert           */
const struct atu_compact_tone_type atu_svc_lost_buf[] = {     
	{ATU_RING_E4,          80},
	{ATU_SILENCE,          40},
	{ATU_RING_E4,          80},
	{ATU_SILENCE,          40},
	{ATU_RING_C4,          80},
	{ATU_STOP,              0}
};

const struct atu_compact_sound_type atu_svc_lost_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) atu_svc_lost_buf
};

/* Change to a more preferred service */
const struct atu_compact_tone_type atu_svc_chng_more_pref_buf[] = {
	{ATU_RING_E4,          80},        
	{ATU_SILENCE,          40}, 
	{ATU_RING_G4,          80},
	{ATU_STOP,              0} 
};

const struct atu_compact_sound_type atu_svc_chng_more_pref_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) atu_svc_chng_more_pref_buf
};

/* Change to a less preferred service */
const struct atu_compact_tone_type atu_svc_chng_less_pref_buf[] = {
	{ATU_RING_E4,          80},
	{ATU_SILENCE,          40}, 
	{ATU_RING_C4,          80},
	{ATU_STOP,              0} 
};

const struct atu_compact_sound_type atu_svc_chng_less_pref_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) atu_svc_chng_less_pref_buf
};

/* These sounds define detection or loss of External Power
*/
/* External power on sound            */
const struct atu_compact_tone_type atu_ext_pwr_on_buf[] = { 
	{ATU_RING_D4,       50},
	{ATU_SILENCE,       50},
	{ATU_RING_FS4,      50},
	{ATU_STOP,           0}
};

const struct atu_compact_sound_type atu_ext_pwr_on_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) atu_ext_pwr_on_buf
};

/* External power off sound          */
const struct atu_compact_tone_type atu_ext_pwr_off_buf[] = { 
	{ATU_RING_FS4,         50},
	{ATU_SILENCE,          50},
	{ATU_RING_D4,          50},
	{ATU_STOP,              0}
};

const struct atu_compact_sound_type atu_ext_pwr_off_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) atu_ext_pwr_off_buf
};

/* Call answer sound when in HFK     */
const struct atu_compact_tone_type atu_vrhfk_call_received_buf[] = {
/* The sound silence at the start of this sound is here because it is possible
** that the first tone will be cut off while the vocoder is being acquired for
** the call. Only a temporary fix.
*/
	{ATU_SILENCE,         250},
	{ATU_RING_B4,         125},
	{ATU_RING_D4,         125},
	{ATU_RING_FS4,        125},
	{ATU_STOP,              0}
};

const struct atu_compact_sound_type atu_vrhfk_call_received_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) atu_vrhfk_call_received_buf
};

/* Call origination sound when in HFK     */
const struct atu_compact_tone_type atu_hfk_call_orig_buf[] = {
/* The sound silence at the start of this sound is here because it is possible
** that the first tone will be cut off while the vocoder is being acquired for
** the call. Only a temporary fix.
*/
	{ATU_SILENCE,         250},
	{ATU_RING_AS4,        125},
	{ATU_RING_D4,         125},
	{ATU_RING_F4,         125},
	{ATU_RING_D4,         125},
	{ATU_RING_AS4,        125},
	{ATU_STOP,              0}
};

const struct atu_compact_sound_type atu_hfk_call_orig_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) atu_hfk_call_orig_buf
};

const struct atu_compact_tone_type special_info_cadence_buf[] = {
	{ATU_RING_AS5,        330},
	{ATU_RING_F5,         330},
	{ATU_RING_A6,         330},
	{ATU_SILENCE,        1000},
	{ATU_STOP,              0}
};

const struct atu_compact_sound_type special_info_cadence_sound = {
	ATU_COMPACT_SOUND,
	(struct atu_compact_tone_type *) special_info_cadence_buf
};

const struct atu_compact_tone_type subscriber_busy_buf[] = { 
	{ATU_DT_TONE_BRAZIL,   500}, /* 425Hz tone for 500 ms */ 
	{ATU_SILENCE,          500}, /* 500 ms silence        */ 
	{ATU_RPT,              0} 
}; 
    
const struct atu_compact_sound_type subscriber_busy_sound = { 
	ATU_COMPACT_SOUND, 
	(struct atu_compact_tone_type *) subscriber_busy_buf 
}; 


const struct atu_compact_tone_type congestion_buf[] = { 
	{ATU_DT_TONE_BRAZIL,   200}, /* 425Hz tone for 200 ms */ 
	{ATU_SILENCE,          200}, /* 200 ms silence        */ 
	{ATU_RPT,              0} 
}; 

const struct atu_compact_sound_type congestion_sound = { 
	ATU_COMPACT_SOUND, 
	(struct atu_compact_tone_type *) congestion_buf 
}; 


const struct atu_flexible_tone_type error_information_buf[] = { 
	{ATU_FREQ,     330, 950, 950}, /* 950Hz tone for 330 ms          */ 
	{ATU_SILENCE, 1000,   0,   0}, /* 1000 ms silence                */ 
	{ATU_RPT,        2,   0,   0}  /* Repeat everything 2 more times */ 
}; 

const struct atu_flexible_sound_type error_information_sound = { 
	ATU_FLEXIBLE_SOUND, 
	(struct atu_flexible_tone_type *) error_information_buf 
}; 


const struct atu_flexible_tone_type number_unobtainable_buf[] = { 
	{ATU_FREQ,     330, 1400, 1400}, /* 1400Hz tone for 330 ms         */ 
	{ATU_SILENCE, 1000,    0,    0}, /* 1000 ms silence                */ 
	{ATU_RPT,        2,    0,    0}  /* Repeat everything 2 more times */ 
}; 

const struct atu_flexible_sound_type number_unobtainable_sound = { 
	ATU_FLEXIBLE_SOUND, 
	(struct atu_flexible_tone_type *) number_unobtainable_buf 
}; 


const struct atu_flexible_tone_type auth_failure_buf[] = { 
	{ATU_FREQ,     330, 1800, 1800}, /* 1800Hz tone for 330 ms         */ 
	{ATU_SILENCE, 1000,    0,    0}, /* 1000 ms silence                */ 
	{ATU_RPT,        2,    0,    0}  /* Repeat everything 2 more times */ 
}; 
 
const struct atu_flexible_sound_type auth_failure_sound = { 
	ATU_FLEXIBLE_SOUND, 
	(struct atu_flexible_tone_type *) auth_failure_buf 
}; 


const struct atu_compact_tone_type radio_path_ack_buf[] = { 
	{ATU_DT_TONE_BRAZIL,   200}, /* 425Hz tone for 200 ms */ 
	{ATU_SILENCE,          500}, /* 500 ms silence        */ 
	{ATU_STOP,               0} 
}; 

const struct atu_compact_sound_type radio_path_ack_sound = { 
	ATU_COMPACT_SOUND, 
	(struct atu_compact_tone_type *) radio_path_ack_buf 
}; 


const struct atu_compact_tone_type radio_path_not_avail_buf[] = { 
	{ATU_DT_TONE_BRAZIL,   200}, /* 425Hz tone for 200 ms */ 
	{ATU_SILENCE,          200}, /* 200 ms silence        */ 
	{ATU_RPT,                3}, /* Repeat everything 3 more times */ 
	{ATU_STOP,               0} 
}; 

const struct atu_compact_sound_type radio_path_not_avail_sound = { 
	ATU_COMPACT_SOUND, 
	(struct atu_compact_tone_type *) radio_path_not_avail_buf 
}; 


const struct atu_compact_tone_type cept_call_waiting_buf[] = { 
	{ATU_DT_TONE_BRAZIL,   200}, /* 425Hz tone for 200 ms */ 
	{ATU_SILENCE,          600}, /* 600 ms silence        */ 
	{ATU_DT_TONE_BRAZIL,   200}, /* 425Hz tone for 200 ms */ 
	{ATU_SILENCE,         3000}, /* 3000 ms silence        */ 
	{ATU_RPT,                1}, /* Repeat everything 1 more time */ 
	{ATU_STOP,               0} 
}; 
   
const struct atu_compact_sound_type cept_call_waiting_sound = { 
	ATU_COMPACT_SOUND, 
	(struct atu_compact_tone_type *) cept_call_waiting_buf 
}; 
    

const struct atu_compact_tone_type cept_ring_buf[] = { 
	{ATU_DT_TONE_BRAZIL,  1000}, /* 425Hz tone for 200 ms */ 
	{ATU_SILENCE,         4000 }, /* 600 ms silence        */ 
	{ATU_RPT,                0 }  /* Repeat forever        */ 
}; 

const struct atu_compact_sound_type cept_ring_sound = { 
	ATU_COMPACT_SOUND, 
	(struct atu_compact_tone_type *) cept_ring_buf 
}; 

const struct atu_compact_tone_type cept_dial_tone_buf[] = { /* CEPT Dial tone */ 
	{ATU_DT_TONE_BRAZIL,  1000 }, 
	{ATU_RPT,                0} 
}; 

const struct atu_compact_sound_type cept_dial_tone_sound = { 
	ATU_COMPACT_SOUND, 
	(struct atu_compact_tone_type *) cept_dial_tone_buf 
}; 


static enum atu_type atu_dummy_sound = ATU_COMPACT_SOUND;


union atu_sound_type const * const atu_sound_ptrs[] = {

	(union atu_sound_type *) &ring_sound, /* Phone Ringing (Alert)                   */
	(union atu_sound_type *) &pwrup_sound, /* Wake-up/Power-up sound                 */
	(union atu_sound_type *) &dial_tone_sound, /* Dial tone                          */
	(union atu_sound_type *) &dtaco_roam_tone_sound, /* DTACO roam tone              */
	(union atu_sound_type *) &call_sound, /* Ringback                                */
	(union atu_sound_type *) &int_sound, /* Intercept                                */
	(union atu_sound_type *) &reord_sound, /* Reorder                                */
	(union atu_sound_type *) &busy_sound, /* Abbreviated Alert (Busy)                */
	(union atu_sound_type *) &confirm_tone_sound, /* Confirmation Tone               */
	(union atu_sound_type *) &call_waiting_sound, /* Call Waiting                    */
	(union atu_sound_type *) &answer_sound, /* Answer Tone                           */
	(union atu_sound_type *) &off_hook_sound, /* Off-Hook Warning                    */

	(union atu_sound_type *) &normal_alert_sound, /* Normal Alerting                 */
	(union atu_sound_type *) &intergroup_alert_sound, /* Intergroup Alerting         */
	(union atu_sound_type *) &special_alert_sound, /* Special/Priority Alerting      */
	(union atu_sound_type *) &ping_ring_sound, /* "Ping ring"                        */

	(union atu_sound_type *) &is54b_long_h_alert_sound, /* IS-54B High Long          */
	(union atu_sound_type *) &is54b_ss_h_alert_sound, /* IS-54B High Short-short     */
	(union atu_sound_type *) &is54b_ssl_h_alert_sound, /* IS-54B High Short-short-long */
	(union atu_sound_type *) &is54b_ss2_h_alert_sound,      /* IS-54B High Short-short-2                 */
	(union atu_sound_type *) &is54b_sls_h_alert_sound,      /* IS-54B High Short-long-short              */
	(union atu_sound_type *) &is54b_ssss_h_alert_sound,     /* IS-54B High Short-short-short-short       */
	(union atu_sound_type *) &is54b_pbx_long_h_alert_sound, /* IS-54B High PBX Long                      */
	(union atu_sound_type *) &is54b_pbx_ss_h_alert_sound,   /* IS-54B High PBX Short-short               */
	(union atu_sound_type *) &is54b_pbx_ssl_h_alert_sound,  /* IS-54B High PBX Short-short-long          */
	(union atu_sound_type *) &is54b_pbx_sls_h_alert_sound,  /* IS-54B High PBX Short-long-short          */
	(union atu_sound_type *) &is54b_pbx_ssss_h_alert_sound, /* IS-54B High PBX Short-short-short-short   */
	(union atu_sound_type *) &is53a_pppp_h_alert_sound,     /* IS-53A High Pip-Pip-Pip-Pip               */

	(union atu_sound_type *) &is54b_long_m_alert_sound,     /* IS-54B Medium Long                        */
	(union atu_sound_type *) &is54b_ss_m_alert_sound,       /* IS-54B Medium Short-short                 */
	(union atu_sound_type *) &is54b_ssl_m_alert_sound,      /* IS-54B Medium Short-short-long            */
	(union atu_sound_type *) &is54b_ss2_m_alert_sound,      /* IS-54B Medium Short-short-2               */
	(union atu_sound_type *) &is54b_sls_m_alert_sound,      /* IS-54B Medium Short-long-short            */
	(union atu_sound_type *) &is54b_ssss_m_alert_sound,     /* IS-54B Medium Short-short-short-short     */
	(union atu_sound_type *) &is54b_pbx_long_m_alert_sound, /* IS-54B Medium PBX Long                    */
	(union atu_sound_type *) &is54b_pbx_ss_m_alert_sound,   /* IS-54B Medium PBX Short-short             */
	(union atu_sound_type *) &is54b_pbx_ssl_m_alert_sound,  /* IS-54B Medium PBX Short-short-long        */
	(union atu_sound_type *) &is54b_pbx_sls_m_alert_sound,  /* IS-54B Medium PBX Short-long-short        */
	(union atu_sound_type *) &is54b_pbx_ssss_m_alert_sound, /* IS-54B Medium PBX Short-short-short-short */
	(union atu_sound_type *) &is53a_pppp_m_alert_sound,     /* IS-53A Medium Pip-Pip-Pip-Pip             */
	(union atu_sound_type *) &is54b_long_l_alert_sound,     /* IS-54B Low Long                           */
	(union atu_sound_type *) &is54b_ss_l_alert_sound,       /* IS-54B Low Short-short                    */
	(union atu_sound_type *) &is54b_ssl_l_alert_sound,      /* IS-54B Low Short-short-long               */
	(union atu_sound_type *) &is54b_ss2_l_alert_sound,      /* IS-54B Low Short-short-2                  */
	(union atu_sound_type *) &is54b_sls_l_alert_sound,      /* IS-54B Low Short-long-short               */
	(union atu_sound_type *) &is54b_ssss_l_alert_sound,     /* IS-54B Low Short-short-short-short        */
	(union atu_sound_type *) &is54b_pbx_long_l_alert_sound, /* IS-54B Low PBX Long                       */
	(union atu_sound_type *) &is54b_pbx_ss_l_alert_sound,   /* IS-54B Low PBX Short-short                */
	(union atu_sound_type *) &is54b_pbx_ssl_l_alert_sound,  /* IS-54B Low PBX Short-short-long           */
	(union atu_sound_type *) &is54b_pbx_sls_l_alert_sound,  /* IS-54B Low PBX Short-long-short           */
	(union atu_sound_type *) &is54b_pbx_ssss_l_alert_sound, /* IS-54B Low PBX Short-short-short-short    */
	(union atu_sound_type *) &is53a_pppp_l_alert_sound,     /* IS-53A Low Pip-Pip-Pip-Pip                */
	(union atu_sound_type *) &fade_tone_sound,              /* Fade tone                                 */
	(union atu_sound_type *) &svc_change_sound,             /* Service change tone                       */
	(union atu_sound_type *) &svc_change_sound,             /* Pad for obsolete - Horn alert             */
	(union atu_sound_type *) &abrv_reord_sound,             /* Abbreviated REORDER (4 Seconds)           */
	(union atu_sound_type *) &abrv_int_sound,               /* Abbreviated INTERCEPT (4 Seconds)         */
	(union atu_sound_type *) &alternate_reorder_sound,      /* Alternate Reorder sound (1.79 seconds)    */
	(union atu_sound_type *) &message_waiting_sound,        /* Message Waiting Alert                     */
	(union atu_sound_type *) &abrv_alert_sound,             /* Abbreviated Alert                         */
	(union atu_sound_type *) &pip_tone_sound,               /* Pip-Pip-Pip-Pip In Call (Vocoder) Version */
	(union atu_sound_type *) &roam_ring_1_sound,             /* Roaming Ring                             */
	(union atu_sound_type *) &atu_svc_acq_sound,             /* Service acquired sound                   */
	(union atu_sound_type *) &atu_svc_lost_sound,            /* Service lost sound                       */
	(union atu_sound_type *) &atu_svc_chng_more_pref_sound,  /* Change to a more preferred service sound */
	(union atu_sound_type *) &atu_svc_chng_less_pref_sound,  /* Change to a less preferred service sound */
	(union atu_sound_type *) &atu_ext_pwr_on_sound,          /* External power on sound                  */
	(union atu_sound_type *) &atu_ext_pwr_off_sound,         /* External power off sound                 */
	(union atu_sound_type *) &user_ring_1_sound, /* User selectable ring 1           */
	(union atu_sound_type *) &user_ring_2_sound, /* User selectable ring 2           */
	(union atu_sound_type *) &user_ring_3_sound, /* User selectable ring 3           */
	(union atu_sound_type *) &user_ring_4_sound, /* User selectable ring 4           */
	(union atu_sound_type *) &user_ring_5_sound, /* User selectable ring 5           */
	(union atu_sound_type *) &user_ring_6_sound, /* User selectable ring 6           */
	(union atu_sound_type *) &user_ring_7_sound, /* User selectable ring 7           */
	(union atu_sound_type *) &user_ring_8_sound, /* User selectable ring 8           */
	(union atu_sound_type *) &user_ring_9_sound, /* User selectable ring 9           */
	(union atu_sound_type *) &atu_vrhfk_call_received_sound,/* Call answer sound for
						                                      when in HFK           */
	(union atu_sound_type *) &atu_hfk_call_orig_sound, /* Call origination sound for
                   						                 when in HFK                */
	(union atu_sound_type *) &special_info_cadence_sound, /* Special info sound      */
	(union atu_sound_type *) &subscriber_busy_sound,        /* Subscriber busy sound                     */ 
	(union atu_sound_type *) &congestion_sound,             /* Congestion sound                          */ 
	(union atu_sound_type *) &error_information_sound,      /* Error information sound                   */ 
	(union atu_sound_type *) &number_unobtainable_sound,    /* Number unobtainable sound                 */ 
	(union atu_sound_type *) &auth_failure_sound,           /* Authentication failure sound              */ 
	(union atu_sound_type *) &radio_path_ack_sound,         /* Radio path acknowlegement sound           */ 
	(union atu_sound_type *) &radio_path_not_avail_sound,   /* Radio path not available sound            */ 
	(union atu_sound_type *) &cept_call_waiting_sound,      /* CEPT call waiting sound                   */ 
	(union atu_sound_type *) &cept_ring_sound,              /* CEPT ring sound                           */ 
	(union atu_sound_type *) &cept_dial_tone_sound,         /* CEPT dial tone                            */      
	(union atu_sound_type *) &atu_dummy_sound, /* Returned when an RPC call to mARM
                             					 needs to be made                    */
	NULL                          /* MUST BE LAST - See atu_init() for Details */
};





unsigned short atu_db_get_sound(unsigned short index,
			  const union atu_sound_type **sound)
{
	unsigned short ret_val = ATUI_FAILURE;

	if (((index > ATU_MIN_COMPACT_ATU) && (index < ATU_MAX_COMPACT_ATU)) ||
	    (index == ATU_ALERT)) {
		*sound = atu_sound_ptrs[index];
		ret_val =ATUI_SUCCESS;
	}

	return(ret_val);
}

