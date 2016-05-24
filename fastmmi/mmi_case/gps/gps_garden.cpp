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

#define LOG_TAG   "GPS_GARDEN"
#include <cutils/log.h>

static hash_map < string, string > paras;
static mmi_module *g_module;
static int module_ret;
static pthread_t processThreadPid;

#define GPS_GARDEN_APP "/system/bin/garden_app"
#define SATELITE_SEARCH_STRING "gps_sv_status_callback"
#define LOC_LATITUDE "LAT"
#define LOC_LONGITUDE "LON"

#define BUF_SIZE_1K 1024

static mmi_window *window;
static class mmi_button *btn_show;
static class mmi_text *text_show[3];
static int g_pid = -1;
struct text_scroll_str {
    char str0[128];
    char str1[128];
    char str2[128];
};
static struct text_scroll_str g_str;
static sem_t g_sem;
struct input_params {
    char config[128];
};
struct input_params input;

int module_main(mmi_module * mod);
void pass(void *);
void fail(void *);

void __attribute__ ((constructor)) register_module(void);

void register_module(void) {
    g_module = mmi_module::register_module(LOG_TAG, module_main);
}

void update_text(char *str) {
    strlcpy(g_str.str0, g_str.str1, sizeof(g_str.str0));
    strlcpy(g_str.str1, g_str.str2, sizeof(g_str.str1));
    strlcpy(g_str.str2, str, sizeof(g_str.str2));
    text_show[0]->set_text(g_str.str0);
    text_show[1]->set_text(g_str.str1);
    text_show[2]->set_text(g_str.str2);
}
void local_exe_cmd() {
    char line[BUF_SIZE_1K];
    char *p = NULL;
    char *q = NULL;
    int pipefd[2];
    int cpid, i = 0, ret = -1;

    if(pipe(pipefd) == -1) {
        ALOGE("pipe fail: %s\n", strerror(errno));
        return;
    }

    cpid = fork();
    if(cpid == -1) {
        ALOGE("fork fail: %s\n", strerror(errno));
        return;
    }
    if(cpid == 0) {             /* Child stdout from pipe */

        close(pipefd[0]);
        char *args[2] = { GPS_GARDEN_APP, NULL };
        if(dup2(pipefd[1], STDOUT_FILENO) == -1) {
            ALOGE(" fail to redirect std output: %s\n", strerror(errno));
            _exit(100);
        }

        if(dup2(pipefd[1], STDERR_FILENO) == -1) {
            ALOGE(" fail to redirect std err: %s\n", strerror(errno));
            _exit(100);
        }
        int ret = execv(GPS_GARDEN_APP, args);

        if(ret == -1) {
            ALOGE("execv fail exit: %s\n", strerror(errno));
            _exit(100);
        }

    } else {                    /* Parent read */
        signal(SIGCHLD, SIG_IGN);
        g_pid = cpid;
        close(pipefd[1]);
        while(read(pipefd[0], &line[i], 1) > 0) {

            if(i < BUF_SIZE_1K - 1 && line[i++] == '\n') {
                ALOGE("line:%s\n", line);
                update_text(line);
                p = strstr(line, LOC_LATITUDE);
                q = strstr(line, LOC_LONGITUDE);
                if(p != NULL && q != NULL) {
                    btn_show->set_text(line);
                    break;
                }
                memset(line, 0, sizeof(line));
                i = 0;
            }
        }
        ALOGE("pipe read failed: %s\n", strerror(errno));
        kill(g_pid, SIGTERM);

        do {
            ret = waitpid(g_pid, NULL, 0);
        } while(ret == -1 && errno == EINTR);

        close(pipefd[0]);
        g_pid = -1;
        ALOGE("GPS : wait garden_app  finished\n");
    }
}

void signalHandler(int signal) {
    pthread_exit(NULL);
}

/**Test process or hardware init*/
void *processThread(void *) {
    signal(SIGUSR1, signalHandler);
    btn_show->set_text("Run garden_app... ");
    local_exe_cmd();
    return NULL;
}

void do_exit() {

    if(g_pid > 0) {
        kill(g_pid, SIGTERM);
        g_pid = -1;
    }

    pthread_kill(processThreadPid, SIGUSR1);
    processThreadPid = NULL;
    sem_post(&g_sem);
}
void pass(void *) {
    module_ret = 0;
    do_exit();
}

void fail(void *) {
    module_ret = -1;
    do_exit();
}

void initUI() {
    int i = 0;
    mmi_point_t point;

    window = new mmi_window();
    int width = window->get_width();
    int height = window->get_height();

    for(i = 0; i < 3; i++) {
        point.x = 40;
        point.y = i * 30 + height * 2 / 10;
        text_show[i] = new mmi_text(point, "text");
        g_module->add_text(text_show[i]);
    }

    mmi_rect_t rect = { 10, height / 3, width - 10, height / 10 };
    btn_show = new mmi_button(rect, "GPS Test start... ", NULL);
    btn_show->set_color(0, 125, 125, 255);
    g_module->add_btn(btn_show);

    g_module->add_window(window);
    g_module->add_btn_pass(pass);
    g_module->add_btn_fail(fail);
}

void get_input() {
    parse_parameter(mmi_config::query_config_value(g_module->get_domain(), "parameter"), paras);
    get_para_value(paras, "config", input.config, sizeof(input.config), "1");
    ALOGE("config :%s\n", input.config);
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
    ALOGE("gps test finished\n");

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
        pthread_join(processThreadPid, NULL);
    }
    finish();
    return module_ret;
}
