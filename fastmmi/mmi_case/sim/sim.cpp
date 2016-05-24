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
#define LOG_TAG   "SIM"
#include <cutils/log.h>

static const char *TAG = "SIM";
static hash_map < string, string > paras;
static mmi_module *g_module;
static int module_ret;
static pthread_t processThreadPid;

#define DEFAULT_APP_PATH "/system/bin/qmi_simple_ril_test"
#define INPUT_FILE "/data/FTM_AP/sim_input.txt"
#define OUTPUT_FILE "/data/FTM_AP/sim_output.txt"
#define DEFAULT_TIMEOUT_MS "3000"

#define KEYWORD_CARD "CARD"
#define KEYWORD_CARD_PRESENT "card state Present"
#define MAX_SIM_NUM 3
#define DEFUALT_SIM_NUM 2
#define BUF_SIZE 4096

static mmi_window *window;
static class mmi_button *btn_sim[MAX_SIM_NUM];
static int sim_state[MAX_SIM_NUM];

static int g_pid;
static sem_t g_sem;
static char sim_test_cmd[512] = { 0 };

struct input_params {
    int sim_num;
    int timeout;
    char app_path[256];
};
static struct input_params input;


int module_main(mmi_module * mod);
void pass(void *);
void fail(void *);

void __attribute__ ((constructor)) register_module(void);

void register_module(void) {
    g_module = mmi_module::register_module(TAG, module_main);
}

void signalHandler(int signal) {
    pthread_exit(NULL);
}

void show_message(int cardnum, bool forall, const char *str) {
    int i = 0;
    char temp[512] = { 0 };
    if(forall) {
        for(i = 0; i < input.sim_num; i++) {
            snprintf(temp, sizeof(temp), "SIMCARD%d: %s", i, str);
            btn_sim[i]->set_text(temp);
        }
    } else {
        if(cardnum < input.sim_num) {
            snprintf(temp, sizeof(temp), "SIMCARD%d: %s", cardnum, str);
            btn_sim[cardnum]->set_text(temp);
        }
    }
}

void exe_sim_command() {
    int pid = fork();
    char inputfile[256] = { 0 };
    char outputfile[256] = { 0 };
    snprintf(inputfile, sizeof(inputfile), "input=%s", INPUT_FILE);
    snprintf(outputfile, sizeof(outputfile), "output=%s", OUTPUT_FILE);
    if(pid == 0) {
        char *args[4] = { input.app_path, inputfile, outputfile, NULL };
        execv(input.app_path, args);
    } else if(pid > 0) {
        g_pid = pid;

        usleep(1000 * input.timeout);
        ALOGE("kill c process \n");
        if(g_pid > 0)
            kill(pid, SIGTERM);

        waitpid(g_pid, NULL, 0);
        g_pid = -1;
        ALOGE("sim module: wait qmi_simple_ril_test  finished\n");
    } else if(pid < 0) {
        perror("fork fail");
    }
}
void get_sim_state() {

    char line[1024] = { 0, };
    bool found = false;
    int index = 0;

    FILE *file = fopen(OUTPUT_FILE, "r");

    if(file == NULL) {
        perror(OUTPUT_FILE);
        return;
    }

    while(fgets(line, sizeof(line), file) != NULL) {

        if(line[0] == '#') {
            continue;
        }

        if(found) {
            found = false;
            ALOGE("found one card : %s\n", line);
            if(strstr(line, KEYWORD_CARD_PRESENT) != NULL) {
                sim_state[index] = 1;
            }
        }

        if(!strncmp(line, "CARD", 4)) {
            ALOGE("card found : %s\n", line);
            get_device_index(line, "CARD", &index);
            found = true;
            continue;
        }
    }

    fclose(file);
}

/**Test process or hardware init*/
void *processThread(void *) {
    char str[128] = { 0 };
    signal(SIGUSR1, signalHandler);
    char result[BUF_SIZE] = { 0 };
    char temp[256] = { 0 };
    show_message(0, true, "reading status ...");
    exe_sim_command();
    get_sim_state();
    for(int i = 0; i < input.sim_num; i++) {
        if(sim_state[i])
            strlcpy(str, "detected", sizeof(str));
        else
            strlcpy(str, "not detected", sizeof(str));
        show_message(i, false, str);
    }
    return NULL;
}

void pass(void *) {
    module_ret = 0;
    pthread_join(processThreadPid, NULL);
    sem_post(&g_sem);
}

void fail(void *) {
    module_ret = -1;
    pthread_join(processThreadPid, NULL);
    sem_post(&g_sem);
}

void add_btn(int i, int width, int height) { 
    char temp[64] = { 0 };
    mmi_rect_t rect = { width / 6, height * (i * 3 + 1) / 20, width * 2 / 3, height / 10 };
    snprintf(temp, sizeof(temp), "SIM%d status", i);
    btn_sim[i] = new mmi_button(rect, temp, NULL);
    btn_sim[i]->set_color(0, 125, 125, 255);
    g_module->add_btn(btn_sim[i]);
}

void initUI() {
    window = new mmi_window();
    int width = window->get_width();
    int height = window->get_height();

    for(int i = 0; i < input.sim_num; i++)
        add_btn(i, width, height);

    g_module->add_window(window);
    g_module->add_btn_pass(pass);
    g_module->add_btn_fail(fail);
}

int prepare_input_output() {
    FILE *fp = NULL;
    char str[256] = "card status";

    fp = fopen(INPUT_FILE, "w");
    if(fp == NULL)
        return -1;

    fwrite(str, 1, strlen(str), fp);
    fclose(fp);
    /*output file empty */
    fp = fopen(OUTPUT_FILE, "w");
    if(fp == NULL)
        return -1;
    fclose(fp);
    return 0;
}

void get_input() {
    char temp[256] = { 0 };
    parse_parameter(mmi_config::query_config_value(g_module->get_domain(), "parameter"), paras);
    get_para_value(paras, "sim_num", temp, sizeof(temp), NULL);

    memset(&input, 0, sizeof(struct input_params));
    input.sim_num = atoi(temp);
    ALOGE("config simcard num:%d\n", input.sim_num);
    if(input.sim_num < 0 || input.sim_num > MAX_SIM_NUM)
        input.sim_num = DEFUALT_SIM_NUM;

    get_para_value(paras, "timeout", temp, sizeof(temp), DEFAULT_TIMEOUT_MS);
    input.timeout = atoi(temp);

    get_para_value(paras, "app_path", temp, sizeof(temp), DEFAULT_APP_PATH);
    strlcpy(input.app_path, temp, sizeof(input.app_path));
    ALOGE("SIM config: num=%d. path=%s\n", input.sim_num, input.app_path);

}

/**do some init work for data, UI but not hardware */
void init() {
    module_ret = -1;
    sem_init(&g_sem, 0, 0);
    memset(&sim_state, 0, sizeof(sim_state));
    get_input();
    prepare_input_output();
    snprintf(sim_test_cmd, sizeof(sim_test_cmd), "%s input=%s", input.app_path, INPUT_FILE);
    ALOGE("cmdline:%s\n", sim_test_cmd);
}

/**To clean source; diable hardware; free memory*/
void finish() {
    sem_wait(&g_sem);
    g_module->clean_source();
}

int auto_test() {
    exe_sim_command();
    get_sim_state();

    if (sim_state[0] == 0)
        g_module->addStringResult("SIM0_DETECTED", "no");
    else
        g_module->addStringResult("SIM0_DETECTED", "yes");

    if (sim_state[1] == 0)
        g_module->addStringResult("SIM1_DETECTED", "no");
    else
        g_module->addStringResult("SIM1_DETECTED", "yes");

    if (sim_state[0] == 0)
        return PCBA_SIM_CARD_0_FAILURE;
    else if (sim_state[1] == 0)
        return PCBA_SIM_CARD_1_FAILURE;
    else
        return CASE_SUCCESS;
}

int module_main(mmi_module * mod) {
    init();
    // PCBA tests first. No init needed here.
    if (g_module->get_run_mode() == TEST_MODE_PCBA) {
        return auto_test();
    }

    initUI();
    int ret = pthread_create(&processThreadPid, NULL, processThread, NULL);

    if(ret < 0) {
        ALOGE("can't create pthread: %s\n", strerror(errno));
    } else {
        pthread_join(processThreadPid, NULL);
    }
    finish();
    return module_ret;
}
