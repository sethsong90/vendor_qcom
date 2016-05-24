/*
 * Copyright (c) 2012-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <stdio.h>
#include <unistd.h>
#include <mmi_window.h>
#include <mmi_module_manage.h>
#include "mmi_utils.h"
#include "mmi_config.h"
#include <hash_map>
#include <pthread.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <errno.h>

#define LOG_TAG   "audio"
#include <cutils/log.h>

static const char *TAG = "AUDIO";
static hash_map < string, string > paras;
static mmi_module *g_module;
static class mmi_button *btn_show;
static int g_retval;
static sem_t g_sem;

static pthread_t processThreadPid;
enum {
    TC_NONE = 0,
    TC_PCM_REC_HANDSET_MIC = 1,
    TC_PCM_PLAY_HANDSET_EARPHONE = 2,
    TC_PCM_REC_HEADSET_MIC = 3,
    TC_PCM_PLAY_HEADSET_MONO_LR = 4,
    TC_PCM_PLAY_HEADSET_STEREO_LR = 5,
    TC_PCM_PLAY_HEADSET_L = 6,
    TC_PCM_PLAY_HEADSET_R = 7,
    TC_PCM_PLAY_HANDSET_LOUDSPEAKER = 8,
    TC_PCM_REC_HANDSET_SECOND_MIC = 13,
    TC_MAX
};

struct mm_audio_ftm_params {
    char filepath[256];
    char config[256];
    int testcase;
    int duration;
    int volume;
    int freq_low;
    int freq_hight;
};

#define MM_AUDIO_FTM "/system/bin/mm-audio-ftm "
#define AUDIO_CONFIG "/etc/ftm_test_config"
#define RECORD_FILE "/data/ftm_pcm_record.wav"
#define BUF_SIZE 4096
static char args_record_tc[512] = { 0 };
static char args_play_tc[512] = { 0 };

static struct mm_audio_ftm_params rec_params;
static struct mm_audio_ftm_params play_params;


int module_main(mmi_module * mod);
void pass(void *);
void fail(void *);

extern "C" void __attribute__ ((constructor)) register_module(void);

void register_module(void) {
    g_module = mmi_module::register_module(TAG, module_main);
}

void signalHandler(int signal) {
    pthread_exit(NULL);
}

void get_string(char *str, struct mm_audio_ftm_params *params, size_t size) {

    switch (params->testcase) {
    case TC_PCM_REC_HANDSET_MIC:
        snprintf(str, size, "Please  speak into handset mic for %d seconds", params->duration);
        break;
    case TC_PCM_PLAY_HANDSET_EARPHONE:
        snprintf(str, size, "Please verify playback of the previously recorded message via the handset earphone");
        break;
    case TC_PCM_REC_HEADSET_MIC:
        snprintf(str, size, "Please speak into headset mic for %d seconds", params->duration);
        break;
    case TC_PCM_PLAY_HEADSET_STEREO_LR:
        snprintf(str, size, "Please verify playback of the previously recorded message via the handset");
        break;
    case TC_PCM_PLAY_HANDSET_LOUDSPEAKER:
        snprintf(str, size, "Please verify playback of the previously recorded message via the loud speaker");
        break;
    case TC_PCM_REC_HANDSET_SECOND_MIC:
        snprintf(str, size, "Please speak into second mic for %d seconds", params->duration);
        break;
    default:
        break;
    }

}
void *processThread(void *) {
    signal(SIGUSR1, signalHandler);
    char result[BUF_SIZE] = { 0 };
    char temp[256] = { 0 };
    get_string(temp, &rec_params, sizeof(temp));
    btn_show->set_text(temp);
    exe_cmd(args_record_tc, result, BUF_SIZE);
    sleep(2);
    get_string(temp, &play_params, sizeof(temp));
    btn_show->set_text(temp);
    btn_show->set_color(125, 125, 0, 255);
    exe_cmd(args_play_tc, result, BUF_SIZE);
    sleep(2);

    btn_show->set_text("finished !");
    btn_show->set_color(125, 0, 125, 255);
    return NULL;
}

void pass(void *) {
    g_retval = 0;
    pthread_join(processThreadPid, NULL);
    sem_post(&g_sem);
}

void fail(void *) {
    g_retval = -1;
    pthread_join(processThreadPid, NULL);
    sem_post(&g_sem);
}

void initUI() {
    mmi_window *window = new mmi_window();
    int width = window->get_width();
    int height = window->get_height();
    mmi_rect_t rect = { 10, height / 3, width - 10, height / 10 };
    btn_show = new mmi_button(rect, "Audio Test start... ", NULL);
    btn_show->set_color(0, 125, 125, 255);
    g_module->add_btn(btn_show);
    g_module->add_btn_pass(pass);
    g_module->add_btn_fail(fail);
}

void construct_cmdline(char *dest, int len, struct mm_audio_ftm_params *params) {
    char temp[256] = { 0 };
    snprintf(dest, len, "%s -c %s -file %s", MM_AUDIO_FTM, AUDIO_CONFIG, RECORD_FILE);

    if(params->testcase != 0) {
        snprintf(temp, sizeof(temp), " -tc %d", params->testcase);
        strlcat(dest, temp, len);
    }

    if(params->duration != 0) {
        snprintf(temp, sizeof(temp), " -d %d", params->duration);
        strlcat(dest, temp, len);
    }

    if(params->volume != 0) {
        snprintf(temp, sizeof(temp), " -v %d", params->volume);
        strlcat(dest, temp, len);
    }

    if(params->freq_low != 0) {
        snprintf(temp, sizeof(temp), " -fl %d", params->freq_low);
        strlcat(dest, temp, len);
    }

    if(params->freq_hight != 0) {
        snprintf(temp, sizeof(temp), " -fh %d", params->freq_hight);
        strlcat(dest, temp, len);
    }
}
void init() {
    g_retval = -1;
    char temp[1024] = { 0 };
    ALOGE("init domain: %s\n", g_module->get_domain());
    parse_parameter(mmi_config::query_config_value(g_module->get_domain(), "parameter"), paras);
    memset(&rec_params, 0, sizeof(struct mm_audio_ftm_params));
    get_para_value(paras, "PCM_RECORD_TC", temp, sizeof(temp), NULL);
    rec_params.testcase = atoi(temp);
    get_para_value(paras, "PCM_RECORD_DUR", temp, sizeof(temp), NULL);
    rec_params.duration = atoi(temp);
    get_para_value(paras, "PCM_RECORD_VOL", temp, sizeof(temp), NULL);
    rec_params.volume = atoi(temp);
    construct_cmdline(args_record_tc, sizeof(args_record_tc), &rec_params);

    memset(&play_params, 0, sizeof(struct mm_audio_ftm_params));
    get_para_value(paras, "PCM_PLAY_TC", temp, sizeof(temp), NULL);
    play_params.testcase = atoi(temp);
    get_para_value(paras, "PCM_PLAY_VOL", temp, sizeof(temp), NULL);
    play_params.volume = atoi(temp);
    construct_cmdline(args_play_tc, sizeof(args_play_tc), &play_params);

    ALOGE("record:%s; play:%s; \n", args_record_tc, args_play_tc);
}

void finish() {
    sem_wait(&g_sem);
    g_module->clean_source();
}

int auto_test() {
    char result[BUF_SIZE] = { 0 };
    char temp[256] = { 0 };
    int res = 0;

    if (rec_params.testcase != TC_PCM_REC_HEADSET_MIC ||
        play_params.testcase != TC_PCM_PLAY_HEADSET_STEREO_LR) {
        return CASE_FAIL;
    }

    get_string(temp, &rec_params, sizeof(temp));
    res |= exe_cmd_res(args_record_tc, result, BUF_SIZE);
    get_string(temp, &play_params, sizeof(temp));
    res |= exe_cmd_res(args_play_tc, result, BUF_SIZE);
    return res;
}

int module_main(mmi_module * mod) {

    if(mod == NULL)
        return -1;
    g_module = mod;

    init();

    if (g_module->get_run_mode() == TEST_MODE_PCBA) {
        return auto_test();
    }

    sem_init(&g_sem, 0, 0);
    initUI();
    int res = pthread_create(&processThreadPid, NULL, processThread, NULL);

    if(res < 0) {
        ALOGE("can't create pthread: %s\n", strerror(errno));
    } else {
        pthread_join(processThreadPid, NULL);
    }

    finish();
    return g_retval;
}
