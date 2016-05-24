/* listen_hw.c
 *
 * Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#define LOG_TAG "listen_hw"
#define LOG_NDEBUG 0
/* #define LOG_NDDEBUG 0 */
#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/sched_policy.h>
#include <system/thread_defs.h>
#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <dlfcn.h>
#include <hardware/audio.h>
#include "listen_hw.h"

#define LIB_ACDB_LOADER "libacdbloader.so"

#define DEVICE_HANDSET_MONO_LISTEN_LOW_POWER_ACDB_ID    (100)
#define DEVICE_HANDSET_MONO_LISTEN_HIGH_POWER_ACDB_ID   (127)

#define RETRY_NUMBER 10
#define RETRY_US 500000

#define SET_STATE(a, b) (a |= b)
#define CLEAR_STATE(a, b) (a &= ~b)
#define CHECK_STATE(a, b) (b == (a & b))

static int route_open_session_adsp(audio_listen_session_t *p_ses);
static int route_close_session_adsp(audio_listen_session_t *p_ses);
static int route_reg_sm_adsp (audio_listen_session_t *p_ses,
                              listen_sound_model_params_t *params);
static int route_dereg_sm_adsp  (audio_listen_session_t* p_ses);
static int route_start_adsp(audio_listen_session_t* p_ses);
static int route_stop_adsp(audio_listen_session_t* p_ses);

static struct pcm_config pcm_config_listen = {
    .channels = LISTEN_DEFAULT_CHANNEL_MODE,
    .rate = LISTEN_DEFAULT_SAMPLING_RATE,
    .period_size = LISTEN_DEFAULT_BUFFER_SIZE,
    .period_count = 2,
    .format = PCM_FORMAT_S16_LE,
};

/* Table of PCM Device Id for Listen maximum sessions.
 * The Device Ids are different depending on what DSP/Codec it is executed on
 * and the target as well.
 *
 * TODO: This hardcoded table needs to be replaced by querying target specific xml
 * device list.  See audio_hw code for an example.
 */
struct use_case_info pcm_use_cases[LISTEN_EXEC_MODE_MAX][NUM_USE_CASES] = {
    {
        // msm8974
        // apq8084
        {0, 16, "listen-voice-wakeup-1"},
        {0, 25, "listen-voice-wakeup-2"},
        {0, 26, "listen-voice-wakeup-3"},
        {0, 27, "listen-voice-wakeup-4"},
        {0, 28, "listen-voice-wakeup-5"},
        {0, 29, "listen-voice-wakeup-6"},
        {0, 30, "listen-voice-wakeup-7"},
        {0, 31, "listen-voice-wakeup-8"},
    },
    {
        // msm8x26
        {0, 19, "listen-voice-wakeup-1"},
        {0, 20, "listen-voice-wakeup-2"},
        {0, 21, "listen-voice-wakeup-3"},
        {0, 22, "listen-voice-wakeup-4"},
        {0, 23, "listen-voice-wakeup-5"},
        {0, 24, "listen-voice-wakeup-6"},
        {0, 25, "listen-voice-wakeup-7"},
        {0, 26, "listen-voice-wakeup-8"},
    },
};

/* Array to store sound devices */
static const char * const listen_device_table[LISTEN_DEVICE_MAX] = {
    [LISTEN_DEVICE_NONE] = "none",
    [LISTEN_DEVICE_HANDSET_MIC] = "listen-handset-mic",
};

/* High level Listen HAL functions (called by AudioHAL and ListenService
 * call functions pointers that are set specific to DSP that this operation
 * will be executed on.
 */
/* adsp_fptrs point to functions called specifically to perform operation
 * when executing on ADSP.
 */
struct listen_session_fptrs adsp_fptrs = {
    .open = route_open_session_adsp,
    .close = route_close_session_adsp,
    .reg_sm = route_reg_sm_adsp ,
    .dereg_sm = route_dereg_sm_adsp  ,
    .start = route_start_adsp,
    .stop = route_stop_adsp,
};

typedef void (*acdb_loader_send_listen_cal_t)(int acdb_id, int app_id);
acdb_loader_send_listen_cal_t acdb_loader_send_listen_cal;

static audio_listen_hardware_t *g_listen_hal = NULL;
static pthread_mutex_t listen_hw_init_lock;
static unsigned int listen_hw_ref_cnt = 0;

static void listen_check_for_mad_type(void)
{
    int ret = 0;
    struct mixer_ctl *ctl = NULL;
    const char *mixer_ctl_name = "MADONOFF Switch";

    ctl = mixer_get_ctl_by_name(g_listen_hal->mixer, mixer_ctl_name);
    if (ctl) {
       ALOGI("%s: found ctl for mixer cmd - %s. So HW MAD",
                  __func__, mixer_ctl_name);
        g_listen_hal->hw_mad_codec = true;
    } else {
        ALOGI("%s: could not get ctl for mixer cmd - %s. So SW MAD",
                  __func__, mixer_ctl_name);
        g_listen_hal->hw_mad_codec = false;
    }
}

static void listen_select_device(void)
{
    g_listen_hal->cur_device = LISTEN_DEVICE_HANDSET_MIC;

    ALOGI("%s: current device = %s", __func__,
            listen_device_table[g_listen_hal->cur_device]);
}

static int listen_hw_init(unsigned int snd_card,
                          struct audio_route *audio_route)
{
    int retry_num = 0;
    int status = 0;

    ALOGV("%s: Enter", __func__);

    g_listen_hal->is_capture_active = AUDIO_CAPTURE_INACTIVE;
    g_listen_hal->snd_card = snd_card;
    g_listen_hal->audio_route = audio_route;

    g_listen_hal->mixer = mixer_open(g_listen_hal->snd_card);

    while (!g_listen_hal->mixer && retry_num < RETRY_NUMBER) {
        usleep(RETRY_US);
        g_listen_hal->mixer = mixer_open(g_listen_hal->snd_card);
        retry_num++;
    }

    if (!g_listen_hal->mixer) {
        ALOGE("%s: ERROR. Unable to open the mixer, aborting", __func__);
        return -ENODEV;
    }

    pthread_mutex_init(&g_listen_hal->lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_init(&g_listen_hal->list_lock, (const pthread_mutexattr_t *) NULL);

    g_listen_hal->acdb_handle = dlopen(LIB_ACDB_LOADER, RTLD_NOW);
    if (g_listen_hal->acdb_handle == NULL) {
        ALOGE("%s: ERROR. dlopen failed for %s", __func__, LIB_ACDB_LOADER);
        status = -ENODEV;
        goto error;
    }

    acdb_loader_send_listen_cal = (acdb_loader_send_listen_cal_t)
              dlsym(g_listen_hal->acdb_handle, "acdb_loader_send_listen_cal");

    if (acdb_loader_send_listen_cal == NULL) {
       ALOGE("%s: ERROR. dlsym Error:%s acdb_loader_send_listen_cal", __func__,
               dlerror());
       status = -ENODEV;
       goto error;
    }

    listen_check_for_mad_type();
    listen_select_device();
    list_init(&g_listen_hal->ses_list);

    ALOGV("%s: Exit", __func__);
    return status;

error:
    if (g_listen_hal->mixer)
        mixer_close(g_listen_hal->mixer);

    if (g_listen_hal->acdb_handle) {
        dlclose(g_listen_hal->acdb_handle);
        g_listen_hal->acdb_handle = NULL;
    }

    return status;
}

static void listen_enable_device(bool enable, listen_device_t listen_device)
{
    if (listen_device < LISTEN_DEVICE_MIN ||
            listen_device >= LISTEN_DEVICE_MAX) {
        ALOGE("%s: Invalid sound device %d", __func__, listen_device);
        return;
    }

    if (enable) {
        ALOGV("%s: enable listen device = %s", __func__,
                listen_device_table[listen_device]);
        // TODO: need to differentiate among HWMAD, CDSP and SWMAD.
        //    Using the same device name won't work because one xml file is used for both
        //    all power modes (e.g. HWMAD and CDSP).
        audio_route_apply_path(g_listen_hal->audio_route,
                listen_device_table[listen_device]);

    } else {
        ALOGV("%s: diable listen device = %s", __func__,
                listen_device_table[listen_device]);
        audio_route_reset_path(g_listen_hal->audio_route,
                    listen_device_table[listen_device]);
    }

    audio_route_update_mixer(g_listen_hal->audio_route);
}

static void listen_enable_use_case(bool enable, audio_listen_session_t *p_ses)
{
    if (enable) {
        ALOGV("%s: enable use case = %s, acdb_id = %u acdb_app_id = %u",
                __func__,
                pcm_use_cases[p_ses->exec_mode][p_ses->use_case_idx].use_case,
                p_ses->acdb_id, p_ses->acdb_app_id);

        acdb_loader_send_listen_cal(p_ses->acdb_id, p_ses->acdb_app_id);
        audio_route_apply_path(g_listen_hal->audio_route,
                 pcm_use_cases[p_ses->exec_mode][p_ses->use_case_idx].use_case);
        audio_route_update_mixer(g_listen_hal->audio_route);

    } else {
        ALOGV("%s: disable use case = %s, acdb_id = %u acdb_app_id = %u",
                __func__,pcm_use_cases[p_ses->exec_mode][p_ses->use_case_idx].use_case,
                p_ses->acdb_id, p_ses->acdb_app_id);

        audio_route_reset_path(g_listen_hal->audio_route,
                pcm_use_cases[p_ses->exec_mode][p_ses->use_case_idx].use_case);
        audio_route_update_mixer(g_listen_hal->audio_route);
    }
}

static int set_listen_detection(audio_listen_session_t *p_ses, bool enable)
{
    int  ret = 0;

    if (CHECK_STATE(p_ses->op_state, SES_SM_REGD)) {
        if (enable) {
            ret = p_ses->lfptrs->start(p_ses);
        } else {
            ret = p_ses->lfptrs->stop(p_ses);
        }
    }
    return ret;
}

/* Called when SSR is complete.
 * Re-enable all ListenDriver session active before SSR.
 */
static void handle_snd_card_online(void)
{
    struct listnode *p_ses_node;
    audio_listen_session_t *p_ses;
    int status = 0;

    ALOGV("%s: Enter", __func__);

    pthread_mutex_lock(&g_listen_hal->lock);
    if(!g_listen_hal->actual_mad_state) {
        g_listen_hal->actual_mad_state = MAD_ENABLED;
        listen_enable_device(true, g_listen_hal->cur_device);

        if (g_listen_hal->mad_cb_func) {
            ALOGV("%s: send LISTEN_EVENT_STARTED",__func__);
            g_listen_hal->mad_cb_func(LISTEN_EVENT_STARTED, NULL, NULL);
        }
    }
    pthread_mutex_unlock(&g_listen_hal->lock);

    /* reload and reactive each previously active session */
    pthread_mutex_lock(&g_listen_hal->list_lock);
    /* list locked while looping thru each session */
    list_for_each(p_ses_node, &g_listen_hal->ses_list) {
        p_ses = node_to_item(p_ses_node, audio_listen_session_t, list_node);

        pthread_mutex_lock(&p_ses->lock);

        status = p_ses->lfptrs->open(p_ses);
        if (status){
            SET_STATE(p_ses->op_state, SES_IN_BAD_STATE);
            goto cleanup;
        }

        status = p_ses->lfptrs->reg_sm(p_ses, &p_ses->lsm_params);
        if (status){
            SET_STATE(p_ses->op_state, SES_IN_BAD_STATE);
            goto cleanup;
        }

        status = p_ses->lfptrs->start(p_ses);
        if (status)
            SET_STATE(p_ses->op_state, SES_IN_BAD_STATE);

        cleanup:
            SET_STATE(p_ses->op_state, SES_SSR_IN_PROGRESS);
            pthread_cond_signal(&p_ses->ssr_cond);
            pthread_mutex_unlock(&p_ses->lock);
    }
    pthread_mutex_unlock(&g_listen_hal->list_lock);

    ALOGV("%s: Exit", __func__);
}

/* Called when SSR is started.
 * Disable all currently active ListenDriver sessions.
 */
static void handle_snd_card_offline(void)
{
    struct listnode *p_ses_node;
    audio_listen_session_t *p_ses;

    ALOGV("%s: Enter", __func__);

    pthread_mutex_lock(&g_listen_hal->lock);
    if(g_listen_hal->actual_mad_state) {
        g_listen_hal->actual_mad_state = MAD_DISABLED;
        listen_enable_device(false, g_listen_hal->cur_device);
        if (g_listen_hal->mad_cb_func) {
            ALOGV("%s: send LISTEN_EVENT_STOPPED",__func__);
            g_listen_hal->mad_cb_func(LISTEN_EVENT_STOPPED, NULL, NULL);
        }
    }
    pthread_mutex_unlock(&g_listen_hal->lock);

    /* teardown each active session */
    pthread_mutex_lock(&g_listen_hal->list_lock);
    /* list locked while looping thru each session */
    list_for_each(p_ses_node, &g_listen_hal->ses_list) {
        p_ses = node_to_item(p_ses_node, audio_listen_session_t, list_node);
        pthread_mutex_lock(&p_ses->lock);
        SET_STATE(p_ses->op_state, SES_SSR_IN_PROGRESS);
        /* No need to check the states as these calls may return failures
        due to DSP is down and we ignore those failures. These calls
        are required to clean listen kernel driver states */
        /* TODO: Consider defining single ListenDriver API to clean up session;
         *       One call that does stop, deregisterSM, & close */
        p_ses->lfptrs->stop(p_ses);
        p_ses->lfptrs->dereg_sm(p_ses);
        p_ses->lfptrs->close(p_ses);
        pthread_mutex_unlock(&p_ses->lock);
    }
    pthread_mutex_unlock(&g_listen_hal->list_lock);

    ALOGV("%s: Exit", __func__);
}

static int handle_set_parameters(const char *kvpairs)
{
    int status = 0;
    struct str_parms *params;
    struct listnode *node;
    audio_listen_session_t *p_ses;
    char *str;
    char value[32];
    char *snd_card_status;
    int ret;
    ALOGV("%s: Enter kvpairs=%s capture=%d", __func__, kvpairs, g_listen_hal->is_capture_active);

    params = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(params, AUDIO_PARAMETER_KEY_MAD, value,
            sizeof(value));
    if (ret > 0) {
        pthread_mutex_lock(&g_listen_hal->lock);
        if (strcmp(value, AUDIO_PARAMETER_VALUE_MAD_ON) == 0)
            g_listen_hal->reqd_mad_state = MAD_ENABLED;
        else
            g_listen_hal->reqd_mad_state = MAD_DISABLED;

        if (g_listen_hal->actual_mad_state == g_listen_hal->reqd_mad_state) {
            ALOGV("%s: nothing to do, actual mad state = %d", __func__,
                  g_listen_hal->actual_mad_state);
            pthread_mutex_unlock(&g_listen_hal->lock);
            goto handled_mad_key;
        }
        if (g_listen_hal->reqd_mad_state &&
            (g_listen_hal->is_capture_active == AUDIO_CAPTURE_ACTIVE)) {
            ALOGV("%s: audio capture active, mad will be enabled/disabled(%d) after "
                  "capture is inactive", __func__, g_listen_hal->reqd_mad_state);
            pthread_mutex_unlock(&g_listen_hal->lock);
            goto handled_mad_key;
        }

        g_listen_hal->actual_mad_state = g_listen_hal->reqd_mad_state;

        /* when mad device to be enabled, then this is done before sessions are started */
        if (g_listen_hal->reqd_mad_state)
            listen_enable_device(true, g_listen_hal->cur_device);

        /* start or stop all active sessions */
        pthread_mutex_lock(&g_listen_hal->list_lock);
        /* list locked while looping thru each session */
        list_for_each(node, &g_listen_hal->ses_list) {
            p_ses = node_to_item(node, audio_listen_session_t, list_node);
            pthread_mutex_lock(&p_ses->lock);
            ret = set_listen_detection(p_ses, g_listen_hal->reqd_mad_state);
            if (ret)
                ALOGE("%s: ERROR. set_listen_detection failed", __func__);
            pthread_mutex_unlock(&p_ses->lock);
        }
        pthread_mutex_unlock(&g_listen_hal->list_lock);

        /* when mad device is disabled, this is done after session stop */
        if (!g_listen_hal->reqd_mad_state)
            listen_enable_device(false, g_listen_hal->cur_device);

        pthread_mutex_unlock(&g_listen_hal->lock);
        goto handled_mad_key;
    }

    ret = str_parms_get_str(params, AUDIO_PARAMETER_KEY_SND_CARD_STATUS, value,
                            sizeof(value));
    if (ret > 0) {
        snd_card_status = value + 2;

        if (strcmp(snd_card_status, "OFFLINE") == 0)
            handle_snd_card_offline();
        else if (strcmp(snd_card_status, "ONLINE") == 0)
            handle_snd_card_online();
        else
            ALOGE("%s: ERROR. unknown snd_card_status", __func__);
    }

handled_mad_key:
    str_parms_destroy(params);
    ALOGV("%s: Exit",__func__);
    return status;
}

static char* handle_get_parameters(const char *kvpairs)
{
    char *str;
    char value[32] = {0};
    int ret;
    struct str_parms *query = str_parms_create_str(kvpairs);
    struct str_parms *reply = str_parms_create();

    ALOGV("%s: Enter, kvpairs - %s", __func__, kvpairs);

    ret = str_parms_get_str(query, AUDIO_PARAMETER_KEY_MAD, value,
            sizeof(value));
    if (ret > 0) {
        pthread_mutex_lock(&g_listen_hal->lock);
        if (g_listen_hal->actual_mad_state == MAD_ENABLED)
            strlcpy(value, AUDIO_PARAMETER_VALUE_MAD_ON, sizeof(value));
        else
            strlcpy(value, AUDIO_PARAMETER_VALUE_MAD_OFF, sizeof(value));
        pthread_mutex_unlock(&g_listen_hal->lock);

        str_parms_add_str(reply, AUDIO_PARAMETER_KEY_MAD, value);
    }

    str = str_parms_to_str(reply);
    str_parms_destroy(query);
    str_parms_destroy(reply);

    ALOGV("%s: Exit returns - %s", __func__, str);
    return str;
}

static void handle_notify_event(event_type_t event_type)
{
    struct listnode *p_ses_node;
    audio_listen_session_t *p_ses;
    bool mad_en, hw_mad_en;
    int ret, status = 0;

    ALOGV("%s: Enter, event type = %d", __func__, event_type);

    if (event_type == AUDIO_CAPTURE_INACTIVE)
        mad_en = true;
    else
        mad_en = false;

    pthread_mutex_lock(&g_listen_hal->lock);
    g_listen_hal->is_capture_active = event_type;

    if (g_listen_hal->reqd_mad_state == MAD_DISABLED)  {
        ALOGV("%s: nothing to do, actual mad state = %d", __func__, g_listen_hal->actual_mad_state);
        pthread_mutex_unlock(&g_listen_hal->lock);
        return;
    }

    if(mad_en) {
        /* Re-enable mad since Capture is now inactive */
        if (g_listen_hal->actual_mad_state == MAD_DISABLED) {
            g_listen_hal->actual_mad_state = MAD_ENABLED;
            listen_enable_device(true, g_listen_hal->cur_device);
        }

        pthread_mutex_lock(&g_listen_hal->list_lock);
        /* list locked while looping thru and restarting each session */
        list_for_each(p_ses_node, &g_listen_hal->ses_list) {
           p_ses = node_to_item(p_ses_node, audio_listen_session_t, list_node);
           pthread_mutex_lock(&p_ses->lock);
           status = p_ses->lfptrs->start(p_ses);
           if (status)
               SET_STATE(p_ses->op_state, SES_IN_BAD_STATE);
           if (!g_listen_hal->hw_mad_codec) {
               listen_enable_device(true, g_listen_hal->cur_device);
               listen_enable_use_case(true, p_ses);
           }
           pthread_mutex_unlock(&p_ses->lock);
        }
        pthread_mutex_unlock(&g_listen_hal->list_lock);

        if (g_listen_hal->mad_cb_func)
            g_listen_hal->mad_cb_func(LISTEN_EVENT_STARTED, NULL, NULL);
    } else {
        pthread_mutex_lock(&g_listen_hal->list_lock);
        /* list locked while looping thru and stop each session */
        list_for_each(p_ses_node, &g_listen_hal->ses_list) {
           p_ses = node_to_item(p_ses_node, audio_listen_session_t, list_node);
           pthread_mutex_lock(&p_ses->lock);
           if (!g_listen_hal->hw_mad_codec) {
               listen_enable_use_case(false, p_ses);
           }
           status = p_ses->lfptrs->stop(p_ses);
           if (status)
               SET_STATE(p_ses->op_state, SES_IN_BAD_STATE);
           pthread_mutex_unlock(&p_ses->lock);
        }
        pthread_mutex_unlock(&g_listen_hal->list_lock);

        /* No need to disable HWMAD during concurency */
        if (g_listen_hal->mad_cb_func)
            g_listen_hal->mad_cb_func(LISTEN_EVENT_STOPPED, NULL, NULL);
    }
    pthread_mutex_unlock(&g_listen_hal->lock);

    ALOGV("%s: Exit", __func__);
}

static int close_device(audio_listen_session_t *p_ses)
{
    ALOGV("%s:[%p] Enter", __func__, p_ses);
    if (p_ses->pcm) {
        ALOGV("%s: closing pcm %p", __func__, p_ses->pcm);
        pcm_close(p_ses->pcm);
        p_ses->pcm = NULL;
    } else
        ALOGV("%s: pcm %p already closed", __func__, p_ses->pcm);

    ALOGV("%s:[%p] Exit", __func__, p_ses);
    return 0;
}

static int open_device(audio_listen_session_t *p_ses)
{
    int status = 0, retry_cnt = 0;

    ALOGV("%s:[%p] Enter. pcm device = %u",__func__, p_ses, p_ses->pcm_id);

    p_ses->pcm = pcm_open(g_listen_hal->snd_card, p_ses->pcm_id,
                          PCM_IN, &p_ses->config);

    if (!p_ses->pcm) {
        ALOGE("%s: ERROR. pcm_open failed", __func__);
        status = -ENODEV;
        goto error;
    }
    if (!pcm_is_ready(p_ses->pcm)) {
        ALOGE("%s: ERROR. pcm_is_ready failed err=%s", __func__,
              pcm_get_error(p_ses->pcm));
        status = -ENODEV;
        goto error;
    }

    status = pcm_start(p_ses->pcm);
    if (status) {
        ALOGE("%s: ERROR. pcm_start failed",__func__);
        status = -ENODEV;
        goto error;
    }

    ALOGV("%s:[%p] Exit", __func__, p_ses);
    return status;

error:
    close_device(p_ses);
    return status;
}

static void process_event(audio_listen_session_t *p_ses,
                          struct snd_lsm_event_status *params)
{
    listen_event_enum_t event_type = LISTEN_ERROR;
    listen_event_data_t payload;

    if (!params) {
        ALOGE("%s: ERROR.[%p] params NULL", __func__, p_ses);
        return;
    }

    ALOGV("%s: param->status %d", __func__, params->status);
    switch (params->status){
    case LSM_VOICE_WAKEUP_STATUS_RUNNING:
        return;
    case LSM_VOICE_WAKEUP_STATUS_DETECTED:
        event_type = LISTEN_EVENT_DETECT_SUCCESS;
        break;
    case LSM_VOICE_WAKEUP_STATUS_END_SPEECH:
    case LSM_VOICE_WAKEUP_STATUS_REJECTED:
        event_type = LISTEN_EVENT_DETECT_FAILED;
        break;
    }
    payload.event_detect.status = params->status;
    payload.event_detect.data = (uint8_t *)&params->payload;
    payload.event_detect.size = params->payload_size;

    ALOGV("%s:[%p]-%d Sending event %d to client ", __func__, p_ses, p_ses->pcm_id, event_type);
    p_ses->ses_cb_func(event_type, &payload, p_ses->client_data);
}

static void* event_thread_entry(void *data)
{
    audio_listen_session_t *p_ses;
    struct snd_lsm_event_status *params;
    int ret;
    void *ret_ptr = NULL;
    unsigned int payload_alloc_size = 0;
    char name[50];

    ALOGV("%s:[%p] Enter",__func__, data);

    if (data == NULL) {
        ALOGE("%s: ERROR. session data NULL", __func__);
        return NULL;
    }
    p_ses = data;

    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_DEFAULT);
    snprintf(name, sizeof(name), "Listen HAL Session Event Thread %d", p_ses->pcm_id);
    prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);

    params = (struct snd_lsm_event_status*)
            malloc(sizeof(*params) + LISTEN_MAX_EVNT_PAYLOAD_SIZE);
    if (params == NULL) {
        ALOGE("%s: ERROR. insufficient memory for payload exiting thread", __func__);
        p_ses->event_thread_alive = false;
        return NULL;
    }
    payload_alloc_size = LISTEN_MAX_EVNT_PAYLOAD_SIZE;

    while (!p_ses->kill_event_thread) {
        params->payload_size = payload_alloc_size;
        ret = pcm_ioctl(p_ses->pcm, SNDRV_LSM_EVENT_STATUS, params);
        if (ret < 0) {
            if (errno == ENOMEM) {
                payload_alloc_size = payload_alloc_size << 1;
                ret_ptr = realloc(params, sizeof(*params) + payload_alloc_size);

                if (ret_ptr == NULL) {
                    ALOGE("%s: ERROR. NOT enough memory for"
                            " payload. exiting thread", __func__);
                    p_ses->kill_event_thread = true;
                    break;
                }
                else {
                    params = (struct snd_lsm_event_status*)ret_ptr;
                    continue;
                }
            } else {
                ALOGE("%s: ERROR. SNDRV_LSM_EVENT_STATUS  failed",__func__);
                p_ses->kill_event_thread = true;
                break;
            }
        }
        if (p_ses->kill_event_thread == true)
            break;

        if(p_ses->ses_cb_func != NULL)
            process_event(p_ses, params);
    }
    if (params)
        free(params);

    p_ses->event_thread_alive = false;
    ALOGV("%s:[%p] Exit", __func__, p_ses);
    return NULL;
}

/* When called the first time, this create and initializes ListenHAL structures/state
 * When call subsequent times, just increments reference count
 */
int create_listen_hw(unsigned int snd_card, struct audio_route *audio_route)
{
    int ret;

    ALOGV("%s: Enter. snd_card = %u. audio_route = %p",
            __func__, snd_card,  audio_route);

    pthread_mutex_lock(&listen_hw_init_lock);
    if (listen_hw_ref_cnt != 0){
        listen_hw_ref_cnt++;
        ALOGV("%s: listen hw already created", __func__);
        ALOGV("%s: Exit", __func__);
        pthread_mutex_unlock(&listen_hw_init_lock);
        return 0;
    }

    if (!audio_route) {
        ALOGE("%s: ERROR. audio_route = %p",
                __func__, audio_route);
        pthread_mutex_unlock(&listen_hw_init_lock);
        return -EINVAL;
    }

    g_listen_hal = calloc(1, sizeof(audio_listen_hardware_t));
    if (g_listen_hal == NULL) {
        ALOGE("%s: ERROR. Failed to create listen hardware", __func__);
        ret = -ENOMEM;
    } else {
        ret = listen_hw_init(snd_card, audio_route);
        if (ret) {
            ALOGE("%s: ERROR. Failed listen hardware init", __func__);
            free(g_listen_hal);
            g_listen_hal = NULL;
        } else
            listen_hw_ref_cnt++;
    }

    pthread_mutex_unlock(&listen_hw_init_lock);
    ALOGV("%s: Exit", __func__);
    return ret;
}

/* When called when there is only one client using ListenHAL structures/state are cleaned up
 * Otherwise, just decrement reference count
 */
void destroy_listen_hw()
{
    ALOGE("%s: Enter hw=%p", __func__, g_listen_hal);

    pthread_mutex_lock(&listen_hw_init_lock);
    if (!g_listen_hal || (--listen_hw_ref_cnt != 0)) {
        pthread_mutex_unlock(&listen_hw_init_lock);
        return;
    }

    if (g_listen_hal->mixer)
        mixer_close(g_listen_hal->mixer);

    if (g_listen_hal->acdb_handle) {
        dlclose(g_listen_hal->acdb_handle);
        g_listen_hal->acdb_handle = NULL;
    }

    pthread_mutex_destroy(&g_listen_hal->lock);
    pthread_mutex_destroy(&g_listen_hal->list_lock);

    free(g_listen_hal);
    g_listen_hal = NULL;

    pthread_mutex_unlock(&listen_hw_init_lock);

    ALOGV("%s: Exit", __func__);
}

static void adsp_destroy_event_thread(audio_listen_session_t *p_ses)
{
    int ret;

    ALOGV("%s:[%p] Enter", __func__, p_ses);

    if (!p_ses->event_thread_alive)
        return;

    p_ses->kill_event_thread = true;

    if ( pcm_ioctl(p_ses->pcm, SNDRV_LSM_ABORT_EVENT) < 0 )
        ALOGE("%s: ERROR. ABORT_EVENT failed", __func__);

    ret = pthread_join(p_ses->event_thread, NULL);
    if (ret)
        ALOGE("%s: ERROR. pthread_join returned %d", __func__, ret);

    ALOGV("%s:[%p] Exit", __func__, p_ses);
}

static void adsp_create_event_thread(audio_listen_session_t *p_ses)
{
    pthread_attr_t attr;
    ALOGV("%s:[%p] Enter", __func__, p_ses);

    p_ses->kill_event_thread = false;
    p_ses->event_thread_alive = true;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&p_ses->event_thread, &attr, event_thread_entry, p_ses);

    ALOGV("%s:[%p] Exit", __func__, p_ses);
}

static int get_pcm_device_id(audio_listen_session_t *p_ses)
{
    int i, ret = -1;

    pthread_mutex_lock(&g_listen_hal->lock);
    for (i = 0; i < NUM_USE_CASES; i++) {
        if(!pcm_use_cases[p_ses->exec_mode][i].active) {
            pcm_use_cases[p_ses->exec_mode][i].active = true;
            ret = pcm_use_cases[p_ses->exec_mode][i].pcm_id;
            p_ses->use_case_idx = i;
            break;
        }
    }
    if (ret < 0) {
        ALOGE("%s: ERROR. no free pcm device available", __func__);
    }
    pthread_mutex_unlock(&g_listen_hal->lock);

    return ret;
}

static void free_pcm_device_id(audio_listen_session_t *p_ses)
{
    int i;

    pthread_mutex_lock(&g_listen_hal->lock);
    for (i = 0; i < NUM_USE_CASES; i++) {
        if(pcm_use_cases[p_ses->exec_mode][i].pcm_id == p_ses->pcm_id) {
            pcm_use_cases[p_ses->exec_mode][i].active = false;
            break;
        }
    }
    pthread_mutex_unlock(&g_listen_hal->lock);
}

static int adsp_open_session(audio_listen_session_t *p_ses)
{
    int status = 0;

    p_ses->pcm_id = get_pcm_device_id(p_ses);
    if(p_ses->pcm_id < 0)
        return -ENODEV;

    listen_enable_use_case(true, p_ses);

    p_ses->config = pcm_config_listen;
    status = open_device(p_ses);
    if (status) {
        ALOGE("%s: Device open failed", __func__);
        listen_enable_use_case(false, p_ses);
        free_pcm_device_id(p_ses);
    }

    return status;
}

static int adsp_close_session(audio_listen_session_t *p_ses)
{
    int status = 0;

    if (!p_ses->pcm) {
        ALOGV("%s:[%p] pcm NULL", __func__, p_ses);
        return status;
    }

    close_device(p_ses) ;
    listen_enable_use_case(false, p_ses);
    free_pcm_device_id(p_ses);

    if(p_ses->lsm_params.sound_model_data) {
        if(p_ses->lsm_params.sound_model_data->data) {
            free(p_ses->lsm_params.sound_model_data->data);
            p_ses->lsm_params.sound_model_data->data = NULL;
        }
        free(p_ses->lsm_params.sound_model_data);
        p_ses->lsm_params.sound_model_data = NULL;
    }

    return status;
}

static int adsp_start(audio_listen_session_t* p_ses)
{
    int status = 0;

    ALOGV("%s:[%p] Enter", __func__, p_ses);
    if (!p_ses->pcm) {
        ALOGV("%s:[%p] pcm NULL", __func__, p_ses);
        return status;
    }
    if (pcm_ioctl(p_ses->pcm, SNDRV_LSM_START)) {
        ALOGE("%s: ERROR. SNDRV_LSM_START failed", __func__);
        status = -errno;
    }
    ALOGV("%s:[%p] Exit, status=%d", __func__, p_ses, status);
    return status;
}

static int adsp_stop(audio_listen_session_t* p_ses)
{
    int status = 0;

    ALOGV("%s:[%p] Enter", __func__, p_ses);
    if (!p_ses->pcm) {
        ALOGV("%s:[%p] pcm NULL", __func__, p_ses);
        return status;
    }
    if (pcm_ioctl(p_ses->pcm, SNDRV_LSM_STOP) < 0) {
        ALOGE("%s: ERROR. SNDDRV_LSM_STOP failed", __func__);
        status = -errno;
    }
    ALOGV("%s:[%p] Exit, status=%d", __func__, p_ses, status);
    return status;
}

static int adsp_reg_sm(audio_listen_session_t *p_ses,
                       listen_sound_model_params_t *params)
{
    int status = 0;
    struct snd_lsm_sound_model lsm_params;

    ALOGV("%s:[%p] Enter", __func__, p_ses);
    if (!p_ses->pcm) {
        ALOGV("%s:[%p] pcm NULL", __func__, p_ses);
        return status;
    }
    if ((params == NULL) || (params->sound_model_data->data == NULL)) {
        ALOGE("%s: params NULL", __func__);
        status = -EINVAL;
        goto error;
    }

    lsm_params.data = params->sound_model_data->data;
    lsm_params.data_size = params->sound_model_data->size;
    lsm_params.min_keyw_confidence = params->min_keyword_confidence;
    lsm_params.min_user_confidence = params->min_user_confidence;
    lsm_params.detect_failure = params->detect_failure;

    if(params->detection_mode == LISTEN_MODE_KEYWORD_ONLY_DETECTION)
        lsm_params.detection_mode = LSM_MODE_KEYWORD_ONLY_DETECTION;
    else if (params->detection_mode == LISTEN_MODE_USER_KEYWORD_DETECTION)
        lsm_params.detection_mode = LSM_MODE_USER_KEYWORD_DETECTION;
    else {
        ALOGV("%s: Unknown detection mode..Setting keyword only", __func__);
        lsm_params.detection_mode = LSM_MODE_KEYWORD_ONLY_DETECTION;
    }

    if(!p_ses->lsm_params.sound_model_data) {
        memcpy(&p_ses->lsm_params, params, sizeof(*params));
        p_ses->lsm_params.sound_model_data = (listen_sound_model_data_t*)malloc(sizeof(listen_sound_model_data_t));
        if (!p_ses->lsm_params.sound_model_data) {
            ALOGE("%s: ERROR. lsm_params alloc failed, size=%d", __func__,
                  params->sound_model_data->size);
            status = -ENOMEM;
            goto error;
        }

        p_ses->lsm_params.sound_model_data->data = (uint8_t *)malloc(params->sound_model_data->size * sizeof(uint8_t));
        if (!p_ses->lsm_params.sound_model_data->data) {
            ALOGE("%s: lsm_params data alloc failed, size=%d", __func__,
                  params->sound_model_data->size);
            status = -ENOMEM;
            goto error;
        }

        p_ses->lsm_params.sound_model_data->size = params->sound_model_data->size;
        memcpy(p_ses->lsm_params.sound_model_data->data, params->sound_model_data->data, params->sound_model_data->size);
    }

    ALOGV("%s: data=%p, data_size=%d, min_keyw_confidence=%d, "
            "min_user_confidence=%d, detect_failure=%d, detection_mode=%d",
            __func__, lsm_params.data, lsm_params.data_size,
            lsm_params.min_keyw_confidence, lsm_params.min_user_confidence,
            lsm_params.detect_failure, lsm_params.detection_mode);

    if(pcm_ioctl(p_ses->pcm, SNDRV_LSM_REG_SND_MODEL, &lsm_params) < 0)
    {
        ALOGE("%s: ERROR. SNDRV_LSM_REG_SND_MODEL failed", __func__);
        status = -errno;
        goto error;
    }

    ALOGV("%s:[%p] Exit", __func__, p_ses);
    return 0;

error:
    if(p_ses->lsm_params.sound_model_data) {
        if(p_ses->lsm_params.sound_model_data->data) {
            free(p_ses->lsm_params.sound_model_data->data);
            p_ses->lsm_params.sound_model_data->data = NULL;
        }
        free(p_ses->lsm_params.sound_model_data);
        p_ses->lsm_params.sound_model_data = NULL;
    }

    ALOGE("%s:[%p] ERROR. Exit status=%d", __func__, p_ses, status);
    return status;
}

static int adsp_dereg_sm(audio_listen_session_t *p_ses)
{
    ALOGV("%s:[%p] Enter", __func__, p_ses);
    if (!p_ses->pcm) {
        ALOGV("%s:[%p] pcm NULL", __func__, p_ses);
        return 0;
    }
    if (pcm_ioctl(p_ses->pcm, SNDRV_LSM_DEREG_SND_MODEL) < 0) {
        ALOGE("%s: ERROR. SNDRV_LSM_DEREG_SND_MODEL failed", __func__);
        return -errno;
    }

    if(p_ses->lsm_params.sound_model_data) {
        if(p_ses->lsm_params.sound_model_data->data) {
            free(p_ses->lsm_params.sound_model_data->data);
            p_ses->lsm_params.sound_model_data->data = NULL;
        }
        free(p_ses->lsm_params.sound_model_data);
        p_ses->lsm_params.sound_model_data = NULL;
    }

    ALOGV("%s:[%p] Exit", __func__, p_ses);
    return 0;
}

static int route_open_session_adsp(audio_listen_session_t *p_ses)
{
    int status = 0;

    if (CHECK_STATE(p_ses->op_state, SES_OPENED )) {
        // nothing to do
        ALOGV("%s: session %p already openned", __func__, p_ses);
        return status;
    }
    status = adsp_open_session(p_ses);
    if (!status) {
        adsp_create_event_thread(p_ses);
        SET_STATE(p_ses->op_state, SES_OPENED );
    }
    return status;
}

static int route_close_session_adsp(audio_listen_session_t *p_ses)
{
    int status = 0;

    if (!CHECK_STATE(p_ses->op_state, SES_OPENED )) {
        // nothing to do
        ALOGV("%s: session %p already closed", __func__, p_ses);
        return status;
    }
    adsp_destroy_event_thread(p_ses);
    status = adsp_close_session(p_ses);
    if (!status) {
        CLEAR_STATE(p_ses->op_state, SES_OPENED );
    }

    return status;
}

static int route_reg_sm_adsp(audio_listen_session_t *p_ses,
                              listen_sound_model_params_t *params)
{
    int status = 0;
    if (CHECK_STATE(p_ses->op_state, SES_SM_REGD)) {
        // nothing to do
        ALOGV("%s: session %p already registered", __func__, p_ses);
        return status;
    }
    status = adsp_reg_sm(p_ses, params);
    if (!status) {
        SET_STATE(p_ses->op_state, SES_SM_REGD);
    }
    return status;
}

static int route_dereg_sm_adsp(audio_listen_session_t* p_ses)
{
    int status = 0;
    if (!CHECK_STATE(p_ses->op_state, SES_SM_REGD)) {
        // nothing to do
        ALOGV("%s: session %p already deregistered", __func__, p_ses);
        return status;
    }
    status = adsp_dereg_sm(p_ses);
    if (!status) {
        CLEAR_STATE(p_ses->op_state, SES_SM_REGD);
    }
    return status;
}

static int route_start_adsp(audio_listen_session_t* p_ses)
{
    int status = 0;
    if (CHECK_STATE(p_ses->op_state, SES_STARTED )) {
        // nothing to do
        ALOGV("%s: session %p already started", __func__, p_ses);
        return status;
    }
    status = adsp_start(p_ses);
    if (!status) {
        SET_STATE(p_ses->op_state, SES_STARTED );
    }
    return status;
}

static int route_stop_adsp(audio_listen_session_t* p_ses)
{
    int status = 0;
    if (!CHECK_STATE(p_ses->op_state, SES_STARTED)) {
        // nothing to do
        ALOGV("%s: session %p already deregistered", __func__, p_ses);
        return status;
    }
    status = adsp_stop(p_ses);
    if (!status) {
        CLEAR_STATE(p_ses->op_state, SES_STARTED);
    }
    return status;
}

int register_sound_model(listen_session_t* handle,
                         listen_sound_model_params_t *params)
{
    int status = 0;
    audio_listen_session_t *p_ses;

    p_ses = (audio_listen_session_t *)(handle);
    ALOGV("%s:[%p] Enter", __func__, p_ses);

    if (!g_listen_hal) {
        ALOGE("%s: ERROR. listen hardware is not created", __func__);
        return -ENODEV;
    }
    if (!p_ses || !p_ses->lfptrs) {
        ALOGE("%s: session handle is NULL", __func__);
        return -EINVAL;
    }

    pthread_mutex_lock(&p_ses->lock);
    if (CHECK_STATE(p_ses->op_state, SES_SSR_IN_PROGRESS)) {
        /* Wait till SSR completes */
        ALOGE("%s: waiting on SSR completion", __func__);
        // TODO: time out after some time. Check on how much time out based on 8084 and other targets.
        pthread_cond_wait(&p_ses->ssr_cond, &p_ses->lock);
    }
    /* Session may have been in bad state after SSR */
    if (CHECK_STATE(p_ses->op_state, SES_IN_BAD_STATE)) {
        ALOGE("%s: ERROR. session in bad state", __func__);
        pthread_mutex_unlock(&p_ses->lock);
        return -EIO;
    }

    status = p_ses->lfptrs->reg_sm(p_ses, params);
    if (!status)
        status = p_ses->lfptrs->start(p_ses);  // start only if registration successful
    // not set to BAD_STATE if either status not success, just return error

    pthread_mutex_unlock(&p_ses->lock);

    ALOGV("%s:[%p] Exit status=%d", __func__, p_ses, status);
    return status;
}

int deregister_sound_model(listen_session_t* handle)
{
    int status = 0;
    audio_listen_session_t *p_ses;

    p_ses = (audio_listen_session_t *)(handle);
    ALOGV("%s:[%p] Enter", __func__, p_ses);

    if (!g_listen_hal) {
        ALOGE("%s: ERROR. listen hardware is not created", __func__);
        return -ENODEV;
    }
    if (!p_ses || !p_ses->lfptrs) {
        ALOGE("%s: session handle is NULL", __func__);
        return -EINVAL;
    }

    pthread_mutex_lock(&p_ses->lock);
    if (CHECK_STATE(p_ses->op_state, SES_SSR_IN_PROGRESS)) {
        /* Wait till SSR completes */
        ALOGE("%s: waiting on SSR completion", __func__);
        // TODO: time out after some time. Check on how much time out based on 8084 and other targets.
        pthread_cond_wait(&p_ses->ssr_cond, &p_ses->lock);
    }
    /* Session may have been in bad state after SSR */
    if (CHECK_STATE(p_ses->op_state, SES_IN_BAD_STATE)) {
        ALOGE("%s: ERROR. session in bad state", __func__);
        pthread_mutex_unlock(&p_ses->lock);
        return -EIO;
    }

    status = p_ses->lfptrs->stop(p_ses);
    if (!status )
        status = p_ses->lfptrs->dereg_sm(p_ses);
    // not set to BAD_STATE if either status not success, just return error

    pthread_mutex_unlock(&p_ses->lock);

    ALOGV("%s:[%p] Exit status=%d", __func__, p_ses, status);
    return status;
}

int set_session_observer(listen_session_t* handle,
                         listen_callback_t cb_func,
                         void *priv)
{
    audio_listen_session_t *p_ses;

    p_ses = (audio_listen_session_t *)handle;
    ALOGV("%s:[%p] Enter cb=%p", __func__, p_ses, cb_func);

    if (!g_listen_hal) {
        ALOGE("%s: ERROR. listen hardware is not created", __func__);
        return -ENODEV;
    }
    if (!handle) {
        ALOGE("%s: ERROR. NULL param handle=%p", __func__, handle);
        return -EINVAL;
    }

    p_ses->client_data = priv;
    p_ses->ses_cb_func = cb_func;

    ALOGV("%s:[%p] Exit", __func__, p_ses);
    return 0;
}

int open_listen_session(struct audio_hw_device *dev,
                        listen_open_params_t *params,
                        listen_session_t **handle)
{
    audio_listen_session_t *p_ses = NULL;
    int status = 0;

    ALOGV("%s: Enter", __func__);
    *handle = NULL;
    if (!g_listen_hal) {
        ALOGE("%s: ERROR. listen hardware is not created", __func__);
        return -ENODEV;
    }

    p_ses = (audio_listen_session_t *)calloc(1, sizeof(audio_listen_session_t));
    if(!p_ses) {
        ALOGV("%s: ERROR. listen session alloc failed", __func__);
        return -ENOMEM;
    }
    /* TODO- call a function which decides whether to run CDSP or ADSP
       based on the number of keywords, sessions opened */
    /* if adsp solution */
    p_ses->lfptrs = &adsp_fptrs;

    /* TODO: do not understand the need for these - please revisit or add comments */
    p_ses->handle.register_sound_model = register_sound_model;
    p_ses->handle.deregister_sound_model = deregister_sound_model;
    p_ses->handle.set_session_observer = set_session_observer;

    /* TODO- need to add logic to get the acdb_id for CDSP */
    if (g_listen_hal->hw_mad_codec) {
        p_ses->exec_mode = LISTEN_EXEC_MODE_LOW_POWER;
        p_ses->acdb_id = DEVICE_HANDSET_MONO_LISTEN_LOW_POWER_ACDB_ID;
    } else {
        p_ses->exec_mode = LISTEN_EXEC_MODE_HIGH_POWER;
        p_ses->acdb_id = DEVICE_HANDSET_MONO_LISTEN_HIGH_POWER_ACDB_ID;
    }
    /*TODO- get the app_id from client */
    p_ses->acdb_app_id = LISTEN_ACDB_APP_ID;
    pthread_mutex_init(&p_ses->lock, (const pthread_mutexattr_t *) NULL);

    pthread_mutex_lock(&g_listen_hal->list_lock);
    list_add_tail(&g_listen_hal->ses_list, &p_ses->list_node);
    pthread_mutex_unlock(&g_listen_hal->list_lock);

    pthread_mutex_lock(&p_ses->lock);
    if (CHECK_STATE(p_ses->op_state, SES_SSR_IN_PROGRESS)) {
        /* Wait till SSR completes */
        ALOGV("%s: waiting on SSR completion", __func__);
        // TODO: time out after some time. Check on how much time out based on 8084 and other targets.
        pthread_cond_wait(&p_ses->ssr_cond, &p_ses->lock);
    }

    status = p_ses->lfptrs->open(p_ses);

    if (!status) {
        SET_STATE(p_ses->op_state, SES_OPENED);
        *handle = &p_ses->handle;
    }
    pthread_mutex_unlock(&p_ses->lock);

    /* cleanup in case of error */
    if (status) {
        pthread_mutex_destroy(&p_ses->lock);

        pthread_mutex_lock(&g_listen_hal->list_lock);
        list_remove(&p_ses->list_node);
        pthread_mutex_unlock(&g_listen_hal->list_lock);
        free(p_ses);
        p_ses = NULL;
    }
    ALOGV("%s: Exit ses=%p status=%d", __func__, *handle, status);
    return status;
}

int close_listen_session(struct audio_hw_device *dev,
                           listen_session_t *handle)
{
    audio_listen_session_t *p_ses;
    int status = 0;

    p_ses = (audio_listen_session_t *)(handle);
    ALOGV("%s:[%p] Enter", __func__, p_ses);

    if (!g_listen_hal) {
        ALOGE("%s: ERROR. listen hardware is not created", __func__);
        return -ENODEV;
    }
    if (!handle) {
        ALOGE("%s: ERROR. session handle is NULL", __func__);
        return -EINVAL;
    }

    pthread_mutex_lock(&p_ses->lock);
    if (CHECK_STATE(p_ses->op_state, SES_SSR_IN_PROGRESS)) {
        /* Wait till SSR completes */
        ALOGE("%s: waiting on SSR completion", __func__);
        // TODO: time out after some time. Check on how much time out based on 8084 and other targets.
        pthread_cond_wait(&p_ses->ssr_cond, &p_ses->lock);
    }

    status = p_ses->lfptrs->close(p_ses);

    if (!status)
        CLEAR_STATE(p_ses->op_state, SES_OPENED);
    pthread_mutex_unlock(&p_ses->lock);

    pthread_mutex_destroy(&p_ses->lock);

    pthread_mutex_lock(&g_listen_hal->list_lock);
    list_remove(&p_ses->list_node);
    pthread_mutex_unlock(&g_listen_hal->list_lock);

    free(p_ses);

    ALOGV("%s:[%p] Exit status=%d", __func__, p_ses, status);
    return status;
}

int listen_hw_set_parameters(audio_hw_device_t *dev, const char *kvpairs)
{
    int ret = 0;
    ALOGV("%s: Enter", __func__);
    if (!g_listen_hal) {
        ALOGE("%s: ERROR. listen hardware is not created", __func__);
        return -ENODEV;
    }
    ret = handle_set_parameters(kvpairs);
    ALOGV("%s: Exit, ret=%d", __func__, ret);
    return ret;
}

char* listen_hw_get_parameters(audio_hw_device_t *dev, const char *kvpairs)
{
    char *str;
    ALOGV("%s: Enter", __func__);

    if (!g_listen_hal) {
        ALOGE("%s: ERROR. listen hardware is not created", __func__);
        return NULL;
    }
    str = handle_get_parameters(kvpairs);

    ALOGV("%s: Exit str=%s", __func__, str);
    return str;
}

void listen_hw_notify_event(event_type_t event_type)
{
    ALOGV("%s: Enter", __func__);

    if (!g_listen_hal) {
        ALOGW("%s: ERROR. listen hardware is not created", __func__);
        return;
    }
    handle_notify_event(event_type);
}

int set_mad_observer(audio_hw_device_t *dev, listen_callback_t cb_func)
{
    ALOGV("%s: Enter cb_func=%p", __func__, cb_func);

    if (!g_listen_hal) {
        ALOGE("%s: ERROR. listen hardware is not created", __func__);
        return -ENODEV;
    }

    g_listen_hal->mad_cb_func = cb_func;
    ALOGV("%s: Exit", __func__);
    return 0;
}

