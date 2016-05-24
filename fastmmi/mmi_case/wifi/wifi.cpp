/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <stdio.h>
#include <unistd.h>
#include <mmi_window.h>
#include <semaphore.h>
#include <mmi_module_manage.h>
#include "mmi_utils.h"
#include "mmi_config.h"
#include <hash_map>
#include <pthread.h>
#include <errno.h>

#define LOG_TAG   "WIFI"
#include <cutils/log.h>

#define DEFAULT_DRIVER "/system/lib/modules/wlan.ko"
#define DEFAULT_CONFIG "/system/bin/ifconfig"
#define DEFAULT_IWLIST "/system/xbin/iwlist"
#define DEFAULT_FTM_DAEMON "/system/bin/ftmdaemon"
#define DEFAULT_PTT "/system/bin/ptt_socket_app"
#define FTM_DAEMON "ftmdaemon"
#define PTT_SOCKET_APP "ptt_socket_app"
#define DEFAULT_ACTION "none"

#define RESULT_SIZE (1024 * 50)
static const char *TAG = "WIFI";
static hash_map < string, string > paras;
static mmi_module *g_module;
static int g_retval;
static sem_t g_sem;

struct input_params {
    char action[16];
    char driver[256];
    char iwlist[256];
    char ifconfig[256];
    char ftmdaemon[256];
    char ptt[256];
};
static struct input_params input;

static pthread_t processThreadPid;

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

static void load_driver() {
    char temp[256] = { 0 };

    if(!check_file_exist(input.driver))
        return;

    snprintf(temp, sizeof(temp), "insmod %s", input.driver);
    system(temp);
    sleep(1);
}

static void config_wlan() {
    char temp[256] = { 0 };
    if(!check_file_exist(input.ifconfig))
        return;

    snprintf(temp, sizeof(temp), "%s wlan0 up", input.ifconfig);
    system(temp);
    sleep(1);
}
static void unload_driver() {
    system("rmmod wlan");
}

static void start_ftm_daemon() {
    if(!check_file_exist(input.ftmdaemon))
        return;
    system(input.ftmdaemon);
}
static void stop_ftm_daemon() {
    char str[64] = { 0 };
    int pid = get_pid_by_name(FTM_DAEMON);

    ALOGE(" pid = %d \n", pid);
    snprintf(str, sizeof(str), "kill %d", pid);
    if(pid != -1)
        system(str);
}
static void start_ptt_socket_app() {
    if(!check_file_exist(input.ptt))
        return;
    system(input.ptt);
}
static void stop_ptt_socket_app() {
    char str[64] = { 0 };
    int pid = get_pid_by_name(PTT_SOCKET_APP);

    ALOGE(" pid = %d \n", pid);
    snprintf(str, sizeof(str), "kill %d", pid);
    if(pid != -1)
        system(str);
}

void *processThread(void *) {
    signal(SIGUSR1, signalHandler);
    char temp[256] = { 0 };
    mmi_window *window = new mmi_window();
    int w = window->get_width();
    int h = window->get_height();

    g_module->add_window(window);

    int last_text_y = h / 10;
    int last_text_x = w / 4;
    mmi_point_t point = { last_text_x, last_text_y };
    mmi_text *text1 = new mmi_text(point, "insmod wlan.ko");

    g_module->add_text(text1);
    last_text_y += 2 * mmi_text::get_font_size_y();

    if(!check_file_exist(input.driver)) {
        text1->set_text("wlan.ko missing!");
        return NULL;
    }

    if(!check_file_exist(input.iwlist)) {
        text1->set_text("iwlist. missing!");
        return NULL;
    }
    if(!check_file_exist(input.ifconfig)) {
        text1->set_text("ifconfig. missing!");
        return NULL;
    }

    load_driver();
    point.x = last_text_x;
    point.y = last_text_y;
    mmi_text *text2 = new mmi_text(point, "open wlan0");

    g_module->add_text(text2);
    last_text_y += 2 * mmi_text::get_font_size_y();

    config_wlan();
    point.x = last_text_x;
    point.y = last_text_y;
    mmi_text *text3 = new mmi_text(point, "scan wifi");

    g_module->add_text(text3);
    last_text_y += 2 * mmi_text::get_font_size_y();
    char result[RESULT_SIZE];

    snprintf(temp, sizeof(temp), "%s wlan0 scan", input.iwlist);
    exe_cmd(temp, result, sizeof(result));

    point.x = last_text_x;
    point.y = last_text_y;
    mmi_text *text4 = new mmi_text(point, "turn off wifi");

    g_module->add_text(text4);
    last_text_y += 2 * mmi_text::get_font_size_y();

    point.x = last_text_x;
    point.y = last_text_y;
    mmi_text *text5 = new mmi_text(point, "scan result. AP list:");

    g_module->add_text(text5);
    last_text_y += 2 * mmi_text::get_font_size_y();

    unload_driver();

    char *p = result;
    char tmp[1024];

    while(p != NULL) {
        p = strstr(p, "ESSID:");
        if(p == NULL) {
            break;
        }
        strlcpy(tmp, p, sizeof(tmp));
        p += strlen(tmp);

        point.x = last_text_x;
        point.y = last_text_y;
        mmi_text *text = new mmi_text(point, tmp);

        g_module->add_text(text);
        last_text_y += 2 * mmi_text::get_font_size_y();
    }
    return NULL;
}

void pass(void *) {
    g_retval = 0;
    pthread_kill(processThreadPid, SIGUSR1);
    sem_post(&g_sem);
}

void fail(void *) {
    g_retval = -1;
    pthread_kill(processThreadPid, SIGUSR1);
    sem_post(&g_sem);
}

void initUI() {
    g_module->add_btn_pass(pass);
    g_module->add_btn_fail(fail);
}
void get_input() {
    memset(&input, 0, sizeof(struct input_params));
    parse_parameter(mmi_config::query_config_value(g_module->get_domain(), "parameter"), paras);
    get_para_value(paras, "driver", input.driver, sizeof(input.driver), DEFAULT_DRIVER);
    get_para_value(paras, "ifconfig", input.ifconfig, sizeof(input.ifconfig), DEFAULT_CONFIG);
    get_para_value(paras, "iwlist", input.iwlist, sizeof(input.iwlist), DEFAULT_IWLIST);
    get_para_value(paras, "ftmdaemon", input.ftmdaemon, sizeof(input.ftmdaemon), DEFAULT_FTM_DAEMON);
    get_para_value(paras, "ptt", input.ptt, sizeof(input.ptt), DEFAULT_PTT);
    get_para_value(paras, "action", input.action, sizeof(input.action), DEFAULT_ACTION);
    ALOGE("parameter:%s , %s,  %s, %s, %s,%s\n", input.driver, input.ifconfig, input.iwlist, input.ptt, input.ftmdaemon,
          input.action);
}
void init() {
    g_retval = -1;
    sem_init(&g_sem, 0, 0);
    initUI();
}

void finish() {
    sem_wait(&g_sem);
    usleep(500 * 1000);
    g_module->clean_source();
}
static int ftm_test(char *action) {

    if(!strncmp(input.action, "on", 2)) {
        ALOGE("action ON start \n");
        load_driver();
        start_ftm_daemon();
        start_ptt_socket_app();
        return CASE_SUCCESS;
    } else if(!strncmp(input.action, "off", 3)) {
        ALOGE("action OFF start \n");
        stop_ptt_socket_app();
        stop_ftm_daemon();
        unload_driver();
        return CASE_SUCCESS;
    } else {
        ALOGE("No action specified. please check the mmi.cfg file \n");
        return CASE_FAIL;
    }
}
int auto_test() {
    char temp[1024] = { 0 };
    char result[1024 * 50];
    bool found = false;

    if(!check_file_exist(input.driver) || !check_file_exist(input.iwlist) || !check_file_exist(input.ifconfig))
        return CASE_FAIL;

    load_driver();
    config_wlan();
    snprintf(temp, sizeof(temp), "%s wlan0 scan", input.iwlist);
    exe_cmd(temp, result, (1024 * 48));
    unload_driver();

    char *p = &result[0];

    while(p != NULL) {
        p = strstr(p, "ESSID:");
        if(p != NULL) {
            found = true;
            break;
        }
    }
    return found ? CASE_SUCCESS : CASE_FAIL;
}
int manual_test() {
    init();
    int res = pthread_create(&processThreadPid, NULL, processThread, NULL);

    if(res < 0)
        ALOGE("can't create pthread: %s\n", strerror(errno));
    else
        pthread_join(processThreadPid, NULL);
    finish();
    return g_retval;
}
int module_main(mmi_module * mod) {

    if(mod == NULL)
        return -1;
    g_module = mod;

    get_input();
    case_run_mode_t mode = g_module->get_run_mode();

    if(mode == TEST_MODE_PCBA) {
        if (input.action != NULL && (!strncmp(input.action, "on", 2) ||
             !strncmp(input.action, "off", 3))) {
            return ftm_test(input.action);
        }
        // In PCBA mode only on & off test cases are supported. Probably config
        // is wrong.
        return CASE_FAIL;
    } else if(mode == TEST_MODE_SANITY) {
        return CASE_FAIL;
    } else {
        if (input.action != NULL && (!strncmp(input.action, "on", 2) ||
             !strncmp(input.action, "off", 3))) {
            return ftm_test(input.action);
        }
        return manual_test();
    }
}
