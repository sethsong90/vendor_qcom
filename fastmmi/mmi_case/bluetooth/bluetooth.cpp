/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mmi_window.h>
#include <mmi_module_manage.h>
#include "mmi_utils.h"
#include "mmi_config.h"
#include <hash_map>
#include <pthread.h>
#include <errno.h>
#include <sys/wait.h>

#define LOG_TAG   "BLUETOOTH"
#include <cutils/log.h>

static hash_map < string, string > paras;
static mmi_module *g_module;
static int g_retval;
sem_t g_sem;

#define DEFAULT_APP_PATH "/system/bin/bdt"
#define DEFAULT_FTM_DAEMON "/system/bin/ftmdaemon"
#define FTM_DAEMON "ftmdaemon"
#define DEFAULT_ACTION "none"

struct input_params {
    char action[16];
    char ftmdaemon[256];
    char app_path[256];
};
static struct input_params input;

static pthread_t processThreadPid;

int module_main(mmi_module * mod);
void pass(void *);
void fail(void *);

extern "C" void __attribute__ ((constructor)) register_module(void);

void register_module(void) {
    g_module = mmi_module::register_module(LOG_TAG, module_main);
}

void signalHandler(int signal) {
    pthread_exit(NULL);
}
static int start_ftm_damenon() {
    if(!check_file_exist(input.ftmdaemon))
        return -1;
    // Do not allow users to start several daemons
    if (get_pid_by_name(FTM_DAEMON)>0)
        return -1;

    int pid = fork();

    if (pid == 0) {
        char *args[2] =
            { input.ftmdaemon, NULL };
        int ret = execv(input.ftmdaemon, args);
        // We shouldn't be here...
        if(ret == -1)
            ALOGE("execv fail exit: %s\n", strerror(errno));
        exit(0);
    } else if (pid > 0) {
        ALOGI("Main thread will exit successfully");
        return 0;
    } else if (pid < 0) {
        perror("fork failed");
        return -1;
    }
    return -1;
}
static int stop_ftm_damenon() {
    int pid;

    pid = get_pid_by_name(FTM_DAEMON);
    if (pid>0) {
        // Kill child
        kill(pid, SIGKILL);
        return (pid == waitpid(pid, NULL, 0)?0:-1);
    }
    return -1;
}

void local_exe_cmd(const char *cmd, char *result, int size) {
    char line[1024];
    char ps[1024] = { 0 };
    FILE *pp;

    strlcpy(ps, cmd, sizeof(ps));
    if((pp = popen(ps, "r")) != NULL) {
        while(fgets(line, sizeof(line), pp) != NULL) {
            if(strlen(line) > 1)
                strlcat(result, line, size);
        }
        ALOGE("close the pipe \n");
        pclose(pp);
        pp = NULL;
    } else {
        ALOGE("popen %s error\n", ps);
    }
}

void *processThread(void *) {
    signal(SIGUSR1, signalHandler);

    mmi_window *window = new mmi_window();
    int w = window->get_width();
    int h = window->get_height();

    g_module->add_window(window);

    int last_text_y = h / 10;
    int last_text_x = w / 4;
    mmi_point_t point;

    point.x = last_text_x;
    point.y = last_text_y;
    mmi_text *text1 = new mmi_text(point, "start bdt");

    g_module->add_text(text1);
    last_text_y += 2 * mmi_text::get_font_size_y();
    if(!check_file_exist(input.app_path)) {
        text1->set_text("bdt missing!");
        return NULL;
    }

    point.x = last_text_x;
    point.y = last_text_y;
    mmi_text *text_waiting = new mmi_text(point, "");

    text_waiting->set_text("waiting...");
    g_module->add_text(text_waiting);
    last_text_y += 2 * mmi_text::get_font_size_y();

    char result[1024 * 10];
    char cmdstr[256];

    snprintf(cmdstr, sizeof(cmdstr), "%s get_ap_list", input.app_path);
    ALOGE("parameter cmdstr:%s \n", cmdstr);
    local_exe_cmd(cmdstr, result, sizeof(result));
    char *p = result;
    char tmp[1024];
    char *ptr;
    while(*p != '\0') {         /*print every line of scan result information */
        ptr = tmp;
        while(*p != '\n' && *p != '\0') {
            *ptr++ = *p++;
        }

        p++;
        *ptr = '\0';
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
    get_para_value(paras, "app_path", input.app_path, sizeof(input.app_path), DEFAULT_APP_PATH);
    get_para_value(paras, "ftmdaemon", input.ftmdaemon, sizeof(input.ftmdaemon), DEFAULT_FTM_DAEMON);
    get_para_value(paras, "action", input.action, sizeof(input.action), DEFAULT_ACTION);

    ALOGE("parameter app_path:%s, %s \n", input.app_path, input.ftmdaemon,input.action);
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
        ALOGE("action ON start");
        return start_ftm_damenon();
    } else if(!strncmp(input.action, "off", 3)) {
        ALOGE("action OFF start");
        return stop_ftm_damenon();
    } else {
        ALOGE("No action specified. please check the mmi.cfg file \n");
        return CASE_FAIL;
    }
}


int auto_test() {
    char result[1024 * 10];
    char cmdstr[256];

    if(!check_file_exist(input.app_path)) {
        return CASE_FAIL;
    }

    snprintf(cmdstr, sizeof(cmdstr), "%s get_ap_list", input.app_path);
    ALOGE("parameter cmdstr:%s \n", cmdstr);
    local_exe_cmd(cmdstr, result, sizeof(result));
    char *p = result;
    char tmp[(1 << 10)];
    char *ptr;
    while(*p != '\0') {         /*print every line of scan result information */
        ptr = tmp;
        while(*p != '\n' && *p != '\0') {
            *ptr++ = *p++;
        }

        p++;
        *ptr = '\0';

    }
    return CASE_SUCCESS;
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
