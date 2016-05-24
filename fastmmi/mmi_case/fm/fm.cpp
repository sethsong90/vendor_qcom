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
#include <errno.h>
#include <semaphore.h>
#include <sys/wait.h>

#define LOG_TAG   "FM"
#include <cutils/log.h>
static hash_map < string, string > paras;
static mmi_module *g_module;
static int module_ret;
static pthread_t processThreadPid;

#define DEFAULT_AUDIO_FTM_APP  "/system/bin/mm-audio-ftm"
#define DEFAULT_AUDIO_FTM_CONFIG  "/etc/ftm_test_config"
#define DEFAULT_FM_APPS_PATH "/system/bin/fmfactorytest"

#define DEFAULT_TUNE "97.7"
#define DEFAULT_PCM_PLAY_DURATION "10"
#define DEFAULT_PCM_PLAY_VOLUME "80"
#define DEFAULT_PCM_PLAY_TC "24"

#define BUF_SIZE 4096
#define BUF_SMALL_SIZE 1024

static mmi_window *window;
static class mmi_button *btn_fm, *btn_next;
static sem_t g_sem;
static char fm_test_cmd[512] = { 0 };

static FILE *pback_pipe;
static bool fm_enabled = false;
static int g_pid = -1;

enum {
    FM_ENABLE = 0,
    FM_DISABLE = 1,
    FM_TUNE = 2,
    FM_SEEK = 3,
    FM_MAX
};

struct input_params {
    char tune[16];
    int play_duration;
    int play_volume;
    int play_tc;
    char app_path[256];
};

int module_main(mmi_module * mod);
void pass(void *);
void fail(void *);
void next_tune_cb(void *);
static int fm_enable(bool enable);
static void construct_cmdline(int type, char *params);
static int fm_tune(char *params);
static int fm_seeknext();
static int fm_config_audio();
static struct input_params input;

void __attribute__ ((constructor)) register_module(void);

void register_module(void) {
    g_module = mmi_module::register_module(LOG_TAG, module_main);
}

void local_exe_cmd(const char *cmd, char *result, int size) {
    char line[BUF_SMALL_SIZE];
    char ps[BUF_SMALL_SIZE] = { 0 };
    char name[BUF_SMALL_SIZE] = { 0, };
    char value[BUF_SMALL_SIZE] = { 0, };
    char indicator = ':';
    FILE *pp;

    strlcpy(ps, cmd, sizeof(ps));
    if((pp = popen(ps, "r")) != NULL) {
        while(fgets(line, sizeof(line), pp) != NULL) {
            char *p = &line[0];

            parse_nv_by_indicator(p, indicator, name, sizeof(name), value, sizeof(value));
            char *pname = trim(name);
            char *pvalue = trim(value);

            if(*pname != '\0' && *pvalue != '\0') {
                ALOGE("getline: %s = %s\n", pname, pvalue);
                if(!strncmp(pname, "result", 6)) {
                    strlcpy(result, pvalue, size);
                    break;
                }
            }
        }
        ALOGE("close the pipe \n");
        pclose(pp);
        pp = NULL;
    } else {
        ALOGE("popen %s error\n", ps);
    }
}

static void exe_set_audio_path() {
    int pid = fork();
    char duration[8] = { 0 };
    char volume[8] = { 0 };
    char testcase[8] = { 0 };

    snprintf(duration, sizeof(duration), "%d", input.play_duration);
    snprintf(volume, sizeof(volume), "%d", input.play_volume);
    snprintf(testcase, sizeof(testcase), "%d", input.play_tc);

    if(pid == 0) {
        char *args[10] =
            { input.app_path, "-tc", testcase, "-c", DEFAULT_AUDIO_FTM_CONFIG, "-d", duration, "-v", volume, NULL };
        int ret = execv(input.app_path, args);

        if(ret == -1) {
            ALOGE("execv fail exit: %s\n", strerror(errno));
            _exit(100);
        }
    } else if(pid > 0) {
        g_pid = pid;
        waitpid(g_pid, NULL, 0);
        g_pid = -1;
        ALOGE("FM module: wait mm audio setting finished\n");
    } else if(pid < 0) {
        perror("fork fail");
    }
}

void signalHandler(int signal) {
    pthread_exit(NULL);
}

void pass(void *) {
    module_ret = 0;
    ALOGE("g_pid:%d\n", g_pid);
    if(g_pid != -1)
        kill(g_pid, SIGTERM);

    pthread_join(processThreadPid, NULL);
    sem_post(&g_sem);
}

void fail(void *) {
    module_ret = -1;
    if(g_pid != -1)
        kill(g_pid, SIGTERM);

    pthread_join(processThreadPid, NULL);
    sem_post(&g_sem);
}

void next_tune_cb(void *) {

    int tries = 5;

    if(!fm_enabled) {
        while(tries > 0 && fm_enable(true)) {
            tries--;
            fm_enable(false);
            sleep(1);
        }
    }
    ALOGE("next_tune ties:%d \n", tries);
    if(tries > 0) {
        fm_enabled = true;
        fm_seeknext();
    }
}

void initUI() {

    window = new mmi_window();
    int width = window->get_width();
    int height = window->get_height();

    mmi_rect_t rect;

    rect.x = width / 10;
    rect.y = height * 2 / 20;
    rect.w = width * 8 / 10;
    rect.h = height / 10;
    btn_fm = new mmi_button(rect, "FM test start ...", NULL);
    btn_fm->set_color(0, 125, 125, 255);
    g_module->add_btn(btn_fm);

    rect.x = width / 10;
    rect.y = height * 7 / 20;
    rect.w = width * 8 / 10;
    rect.h = height * 2 / 10;

    btn_next = new mmi_button(rect, "SEARCH NEXT >>", next_tune_cb);
    btn_next->set_color(0, 125, 125, 255);
    g_module->add_btn(btn_next);

    g_module->add_window(window);
    g_module->add_btn_pass(pass);
    g_module->add_btn_fail(fail);
}

static void construct_cmdline(int type, char *params) {

    char temp[128] = { 0 };
    switch (type) {
    case FM_ENABLE:
        strlcat(temp, " enable", 8);
        break;
    case FM_DISABLE:
        strlcat(temp, " disable", 9);
        break;
    case FM_TUNE:
        strlcat(temp, " tune", 6);
        strlcat(temp, params, sizeof(temp));
        break;
    case FM_SEEK:
        strlcat(temp, " seeknext", 10);
        break;
    default:
        break;
    }
    memset(fm_test_cmd, 0, sizeof(fm_test_cmd));
    snprintf(fm_test_cmd, sizeof(fm_test_cmd), "%s %s", DEFAULT_FM_APPS_PATH, temp);
}

static int fm_enable(bool enable) {
    char result[BUF_SIZE] = { 0 };
    construct_cmdline(enable ? FM_ENABLE : FM_DISABLE, NULL);
    ALOGE("cmd:%s \n", fm_test_cmd);
    if(enable)
        btn_fm->set_text("FM enable");

    local_exe_cmd(fm_test_cmd, result, sizeof(result));
    fm_enabled = !atoi(result);

    ALOGE("enable result:%s \n", result);
    sleep(2);
    return 0;
}

static int fm_tune(char *params) {
    char result[BUF_SIZE] = { 0 };
    char temp[128] = { 0 };
    snprintf(temp, sizeof(temp), "Playing tune:%s", input.tune);
    construct_cmdline(FM_TUNE, params);
    ALOGE("cmd:%s \n", fm_test_cmd);
    btn_fm->set_text(temp);
    local_exe_cmd(fm_test_cmd, result, sizeof(result));
    sleep(2);
    return 0;
}

static int fm_seeknext() {
    char result[BUF_SIZE] = { 0 };
    char temp[128] = { 0 };
    construct_cmdline(FM_SEEK, NULL);
    ALOGE("cmd:%s \n", fm_test_cmd);
    btn_fm->set_text("Searching available tune ...");
    local_exe_cmd(fm_test_cmd, result, sizeof(result));
    ALOGE("fm_seeknext result:%s \n", result);
    snprintf(temp, sizeof(temp), "Playing tune:%s", result);
    btn_fm->set_text(temp);
    sleep(2);
    return 0;
}

static int fm_config_audio() {
    char result[BUF_SIZE] = { 0 };
    ALOGE("start to config audio \n");
    btn_fm->set_text("Start to config audio path for FM");
    exe_set_audio_path();
    ALOGE("end to config audio \n");
    sleep(2);
    return 0;
}

/**Test process or hardware init*/
void *processThread(void *) {
    signal(SIGUSR1, signalHandler);
    fm_config_audio();
    return NULL;
}
void get_input() {
    char temp[256] = { 0 };
    parse_parameter(mmi_config::query_config_value(g_module->get_domain(), "parameter"), paras);
    get_para_value(paras, "tune", input.tune, sizeof(input.tune), DEFAULT_TUNE);
    get_para_value(paras, "PCM_PLAY_DURATION", temp, sizeof(temp), DEFAULT_PCM_PLAY_DURATION);
    input.play_duration = atoi(temp);

    get_para_value(paras, "PCM_PLAY_VOLUME", temp, sizeof(temp), DEFAULT_PCM_PLAY_VOLUME);
    input.play_volume = atoi(temp);

    get_para_value(paras, "PCM_PLAY_TC", temp, sizeof(temp), DEFAULT_PCM_PLAY_TC);
    input.play_tc = atoi(temp);

    get_para_value(paras, "app_path", input.app_path, sizeof(input.app_path), DEFAULT_AUDIO_FTM_APP);
    ALOGE("d:%d, v:%d  app_path:%s\n", input.play_duration, input.play_volume, input.app_path);
}

/**do some init work for data, UI but not hardware */
void init() {
    module_ret = -1;
    sem_init(&g_sem, 0, 0);
    get_input();
    initUI();
}

/**To clean source; diable hardware; free memory*/
void finish() {
    sem_wait(&g_sem);
    g_module->clean_source();
}

int module_main(mmi_module * mod) {
    // Check if we've entered here by mistake. Normally we shouldn't
    // be here.
    if (mod->get_run_mode() == TEST_MODE_PCBA) {
        return CASE_FAIL;
    }

    init();
    int ret = pthread_create(&processThreadPid, NULL, processThread, NULL);

    if(ret < 0) {
        ALOGE("can't create pthread: %s\n", strerror(errno));
    } else {
        if(!fm_enable(true)) {
            fm_seeknext();
        }
        pthread_join(processThreadPid, NULL);
    }

    finish();
    return module_ret;
}
