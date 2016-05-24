/* listen_hw.h
 *
 * Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef ANDROID_LISTEN_HARDWARE_H
#define ANDROID_LISTEN_HARDWARE_H

#include <stdbool.h>
#include <pthread.h>
#include <cutils/list.h>
#include <tinyalsa/asoundlib.h>
#include <audio_route/audio_route.h>
#include <listen_types.h>
#include "sound/lsm_params.h"

#define LISTEN_DEFAULT_SAMPLING_RATE (16000)
#define LISTEN_DEFAULT_CHANNEL_MODE  (1)
#define LISTEN_DEFAULT_BUFFER_SIZE   (1024)

#define LISTEN_ACDB_APP_ID (1)
#define LISTEN_MAX_EVNT_PAYLOAD_SIZE (256)

#define LISTEN_PCM_MAX_RETRY (10)
#define LISTEN_PCM_SLEEP_WAIT (1000)

#define NUM_USE_CASES (8)

#define LISTEN_LOW_POWER_PCM_DEVICE  16
#define LISTEN_HIGH_POWER_PCM_DEVICE  19

/* MAD device enabled/disabled State */
#define MAD_ENABLED   true
#define MAD_DISABLED  false

typedef enum {
    LISTEN_EXEC_MODE_INVALID = -1,
    LISTEN_EXEC_MODE_LOW_POWER = 0,
    LISTEN_EXEC_MODE_HIGH_POWER,
    LISTEN_EXEC_MODE_MAX
} listen_exec_mode_t;

enum {
    LISTEN_DEVICE_NONE = 0,
    LISTEN_DEVICE_MIN,
    LISTEN_DEVICE_HANDSET_MIC = LISTEN_DEVICE_MIN,
    LISTEN_DEVICE_MAX,
};

enum lsm_vw_status{
    LSM_VOICE_WAKEUP_STATUS_RUNNING = 1,
    LSM_VOICE_WAKEUP_STATUS_DETECTED,
    LSM_VOICE_WAKEUP_STATUS_END_SPEECH,
    LSM_VOICE_WAKEUP_STATUS_REJECTED
};
typedef enum lsm_vw_status lsm_vw_status_t;

enum listen_ses_states {
    SES_OPENED = 0x01,
    SES_SM_REGD = 0x02,
    SES_STARTED = 0x04,
    SES_SSR_IN_PROGRESS = 0x08,
    SES_SSR_COMPLETED = 0x10,
    SES_IN_BAD_STATE = 0x20,
};

struct use_case_info {
    bool active;
    int pcm_id;
    const char* use_case;
};

struct audio_listen_session {
    listen_session_t handle;
    struct listnode list_node; /* used to manage this session node in session list */
    struct listen_session_fptrs *lfptrs;
    struct pcm *pcm;
    struct pcm_config  config;
    int pcm_id;
    pthread_mutex_t lock;
    pthread_cond_t  ssr_cond;
    pthread_t event_thread;
    listen_callback_t ses_cb_func;
    listen_sound_model_params_t lsm_params;
    bool kill_event_thread;
    bool event_thread_alive;
    void *client_data;
    listen_exec_mode_t exec_mode;
    unsigned int use_case_idx;
    unsigned int acdb_id;
    unsigned int acdb_app_id;
    unsigned int op_state;
};
typedef struct audio_listen_session audio_listen_session_t;

/* mixer controls, push session calibration, pcm_open, create event thread */
typedef int (*listen_open_t)(audio_listen_session_t *handle);
/* destroy event thread, mixer controls, pcm_close, */
typedef int (*listen_close_t)(audio_listen_session_t *handle);
/* SNDRV_LSM_REG_SND_MODEL, store soundmodel for SSR */
typedef int (*listen_reg_sm_t)(audio_listen_session_t *handle,
                              listen_sound_model_params_t *params);
/* SNDRV_LSM_DEREG_SND_MODEL,  */
typedef int (*listen_dereg_sm_t)(audio_listen_session_t *handle);
/* SNDRV_LSM_START */
typedef int (*listen_start_t)(audio_listen_session_t *handle);
/* SNDRV_LSM_STOP */
typedef int (*listen_stop_t)(audio_listen_session_t *handle);

struct listen_session_fptrs {
    listen_open_t open;
    listen_close_t close;
    listen_reg_sm_t reg_sm;
    listen_dereg_sm_t dereg_sm;
    listen_start_t start;
    listen_stop_t stop;
};

typedef int listen_device_t;

struct audio_listen_hardware
{
    unsigned snd_card;
    struct mixer *mixer;
    struct audio_route *audio_route;
    pthread_mutex_t lock;
    pthread_mutex_t list_lock;
    listen_callback_t mad_cb_func;
    void *acdb_handle;
    struct listnode ses_list;
    event_type_t is_capture_active;
    bool reqd_mad_state;    // requested MAD state: MAD_ENABLED, MAD_DISABLED
    bool actual_mad_state;  // actual MAD state
    bool hw_mad_codec;
    listen_device_t cur_device;
};
typedef struct audio_listen_hardware audio_listen_hardware_t;

#endif    /* ANDROID_LISTEN_HARDWARE_H */
