/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <time.h>
#include <mmi_module_manage.h>
#include <mmi_button.h>
#include <mmi_window.h>
#include <mmi_key.h>
#include <mmi_config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cutils/properties.h>

#include <dirent.h>
#include <dlfcn.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>

#include <sys/types.h>
#include <unistd.h>

#include "mmi.h"
#include "draw.h"
#include "input.h"
#include "exec.h"
#include "mmi_utils.h"
#include "diagext.h"

#include <errno.h>
#include <string.h>

extern "C" {
#include <minui.h>
}

#include <list>
#include <vector>
using namespace std;

static const char *boot_mode_string[] = {
    "normal",
    "ffbm-00",
    "ffbm-01",
    "ffbm-02"
};

static const char *test_mode_string[] = {
    "none",
    "pcba",
    "ui",
    "sanity"
};


int dft_rect_x() {
    return gr_fb_width() / 16;
}

int dft_rect_w() {
    return gr_fb_width() / 6;
}

int dft_rect_h() {
    return gr_fb_height() / 16;
}

int module_rect_w() {
    return dft_rect_w() * 3 / 2;
}

int module_rect_h() {
    return dft_rect_h() * 3 / 2;
}

int top_margin() {
    return dft_rect_h();
}

static sem_t g_sem_exit;

static mmi_module *current_module = NULL;
mmi_state *state_instance = NULL;
struct input_params mmi_input;

int g_up_down_count = 0;
int g_return_value;

mmi_module *get_main_module() {
    static mmi_module *main_module = NULL;

    if(main_module == NULL) {
        main_module = new mmi_module("mmi", NULL);
    }

    return main_module;
}

int is_main_module(mmi_module * p) {
    char *main_name = NULL;

    main_name = get_main_module()->get_domain();
    if(main_name == NULL)
        return -1;
    if(strncmp(p->get_domain(), main_name, sizeof(main_name)))
        return -1;

    return 0;
}
mmi_module *get_current_module() {
    if(current_module == NULL) {
        return get_main_module();
    }

    return current_module;
}
void set_current_module(mmi_module * p) {
    if(!is_main_module(p))
        state_instance->set_state(MMI_IDLE);    /*idle */
    else
        state_instance->set_state(MMI_BUSY);    /*excuting */

    current_module = p;
}


void write_result(const char *domain, const char *result) {
    string content;

    content = "[" + (string) domain + "]" + '\n' + "Result = " + result + '\n';
    write_file(state_instance->get_cur_result_file(), content.c_str());
}

void write_result(const char *domain, const int result) {
    const char *res = !result ? "Pass" : "Fail";

    write_result(domain, res);
}

void write_test_time(double dTestTime) {
    char sLine[100] = { 0 };
    snprintf(sLine, sizeof(sLine), "TestTime_Sec = %.f\n", dTestTime);
    write_file(state_instance->get_cur_result_file(), sLine);
}

void write_output_parameters(mmi_module * pMod) {
    if(pMod) {
        hash_map < string, string >::iterator iter;

        iter = pMod->getResults()->begin();
        for(iter = pMod->getResults()->begin(); iter != pMod->getResults()->end(); iter++) {
            string content;

            content = iter->first + " = " + iter->second + "\n";
            write_file(state_instance->get_cur_result_file(), content.c_str());
        }
    }
}

int launch_module(mmi_module * mod) {
    int res;
    char buf[80];
    mmi_text *text;
    double dTestTime = 0;
    tc_info_s *tc_info = state_instance->get_tc_info_by_name(mod->get_domain());

    if(tc_info == NULL)
        return -1;
    ALOGE("%s: TC_INFO = %s\n", mod->get_domain(), tc_info->name);

    /*check if any case is running */
    if(is_main_module(get_current_module())) {
        ALOGE("current module is running : %s\n", current_module->get_domain());
        return -1;
    }
    set_current_module(mod);
    time(&(tc_info->start_time));
    mod->launch();
    res = mod->wait();
    mod->clean_source();

    snprintf(buf, sizeof(buf), "%d/%d", mod->get_suc_num(), mod->get_suc_num() + mod->get_failed_num());

    text = get_main_module()->find_text_match_extra(mod);
    if(text != NULL) {
        text->set_text(buf);
    }

    mod->set_result(res);

    time(&(tc_info->end_time));
    tc_info->result = res;
    tc_info->fail_num = mod->get_failed_num();
    tc_info->suc_num = mod->get_suc_num();
    tc_info->error_code = 1;
    dTestTime = difftime(tc_info->end_time, tc_info->start_time);

    write_result(mod->get_domain(), res);
    write_output_parameters(mod);
    write_test_time(dTestTime);

    set_current_module(get_main_module());
    ALOGE("%s: Result = %s\n", mod->get_domain(), res ? "Fail" : "Pass");

    return res;
}

int module_btn_len(list < mmi_button * >*btn_list) {
    int i = 0;

    list < mmi_button * >::iterator iter;
    mmi_button *tmp;

    for(iter = btn_list->begin(); iter != btn_list->end(); iter++) {
        tmp = *iter;
        if(tmp->get_launch_module() != NULL) {
            i++;
        }
    }
    return i;
}

void btn_cb_up(void *btn) {
    list < mmi_item * >*itemlist = get_main_module()->get_item_list();
    list < mmi_item * >::iterator item_iter;
    for(item_iter = itemlist->begin(); item_iter != itemlist->end(); item_iter++) {
        mmi_item *tmp = *item_iter;
        int curIndex = tmp->get_index();

        if(item_iter == itemlist->begin() && curIndex >= 0)
            break;
        tmp->set_index(curIndex + 10);
    }
}

void btn_cb_down(void *btn) {
    list < mmi_item * >*itemlist = get_main_module()->get_item_list();
    list < mmi_item * >::iterator item_iter;
    for(item_iter = itemlist->begin(); item_iter != itemlist->end(); item_iter++) {
        mmi_item *tmp = *item_iter;
        int curIndex = tmp->get_index();

        tmp->set_index(curIndex - 10);
    }
}

void btn_cb_finish(void *component) {
    mmi_button *btn = (mmi_button *) component;

    set_boot_mode(BOOT_MODE_NORMAL);
    btn->set_text("Rebooting...");
    sem_post(&g_sem_exit);
}

void btn_cb_module(mmi_button * btn) {
    mmi_rect_t *rect = btn->get_rect();

    if(rect->y > gr_fb_height()) {
        return;
    }

    int res = launch_module(btn->get_launch_module());

    if(res == 0) {
        /*set color to pale blue */
        btn->set_color(0x00, 0x9A, 0xCD, 0xFF);
    } else {
        btn->set_color(255, 0, 0, 255);
    }
}

void item_cb_module(void *component) {
    mmi_item *item = (mmi_item *) component;

    ALOGE("To test %s\n", item->get_launch_module()->get_domain());
    int res = launch_module(item->get_launch_module());

    ALOGE("Test over for: %s\n", item->get_launch_module()->get_domain());
    if(res == 0) {

    } else {
    }
}

void load_one_test_case(const char *filename, const char *domain) {
    char lib_file[256];
    void (*p_fun) (void);
    const char *error;

    if(filename == NULL || strlen(filename) == 0)
        return;
    snprintf(lib_file, sizeof(lib_file), "%s%s", "/system/vendor/lib/", filename);
    if(access(lib_file, R_OK)) {
        ALOGE("%s not found\n", filename);
        return;
    }

    void *handle;

    handle = dlopen(lib_file, RTLD_LAZY);
    //TODO: dlerror() should be used instead of NULL check here
    if(handle == NULL) {
        ALOGE("Could not open %s: %s\n", lib_file, dlerror());
    } else {
        /* use domain in config file test case no longer need register with DOMAIN */
        ALOGE("library = %p, %s\n", handle, lib_file);
        if(mmi_module::check_module_by_handle(handle)) {
            ALOGE("library = %p, %s already loaded, try to manually call regist\n", handle, lib_file);
            dlerror();          /* Clear any existing error */
            p_fun = (void (*)(void)) dlsym(handle, "register_module");
            if((error = dlerror()) != NULL || p_fun == NULL) {
                ALOGE("error = %s, find register_module fail \n", error);
                return;
            }
            (*p_fun) ();
            ALOGE("library = %p, call register_module manually end \n", handle);

        }
        list < mmi_module * >*module_list;
        module_list = mmi_module::get_module_list();
        if(!module_list->empty()) {
            list < mmi_module * >::iterator iter = mmi_module::get_module_list()->end();
            iter--;
            mmi_module *module = *iter;

            ALOGE("library = %p, %s,%s.  org=%s \n", handle, domain, filename, module->get_domain());
            module->set_domain(domain);
            module->set_libname(filename);
            /*store the handle used by dlclose() */
            module->set_handle(handle);
        }
    }
}

void load_all_test_case() {

    vector < string > domain_list = mmi_config::get_config_domain_list();
    if(!domain_list.empty()) {
        vector < string >::iterator iter;
        for(iter = domain_list.begin(); iter != domain_list.end(); iter++) {
            const char *domain = (*iter).c_str();
            const char *enable = mmi_config::query_config_value(domain,
                                                                "enable");
            const char *lib_name = mmi_config::query_config_value(domain,
                                                                  "lib_name");

            if(enable != NULL && strncmp(enable, "0", 1))
                load_one_test_case(lib_name, domain);
        }
    } else
        ALOGE("No config file found");

}

void build_main_ui() {
    int i;

    list < mmi_module * >*module_list;
    list < mmi_module * >::iterator iter;
    mmi_rect_t rect;

    module_list = mmi_module::get_module_list();

    for(iter = module_list->begin(), i = 0; iter != module_list->end(); iter++, i++) {
        mmi_module *mod = *iter;
        char text[80] = { 0 };
        if(!mmi_config::query_config_value(mod->get_domain(), "display_name"))
            continue;

        snprintf(text, sizeof(text), "%s",
                 mmi_config::query_config_value(mod->get_domain(), "display_name", mod->get_domain()));
        class mmi_item *item = new mmi_item(i, text, item_cb_module);

        item->set_launch_module(mod);
        get_main_module()->add_item(item);
    }

    rect.x = gr_fb_width() - dft_rect_w();
    rect.y = gr_fb_height() / 2 - 2 * dft_rect_w();
    rect.w = dft_rect_w();
    rect.h = dft_rect_w();
    class mmi_button *btn = new mmi_button(rect, "PageUp", btn_cb_up);

    btn->set_color(0, 0, 0, 0);
    get_main_module()->add_btn(btn);

    rect.y = gr_fb_height() / 2;
    btn = new mmi_button(rect, "PageDown", btn_cb_down);
    btn->set_color(0, 0, 0, 0);
    get_main_module()->add_btn(btn);

    rect.y = gr_fb_height() - dft_rect_w();
    btn = new mmi_button(rect, "finish", btn_cb_finish);
    btn->set_color(0, 0, 0, 0);
    get_main_module()->add_btn(btn);
}

int create_draw_thread(void) {
    pthread_t ptid;
    int res;

    res = pthread_create(&ptid, NULL, draw_thread, NULL);
    if(res < 0) {
        ALOGE("Can't create pthread: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int create_input_thread(void) {
    pthread_t ptid;
    int res;

    res = pthread_create(&ptid, NULL, input_thread, NULL);
    if(res < 0) {
        ALOGE("Can't create pthread: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int create_main_exec_thread() {
    pthread_t ptid;
    int res;

    res = pthread_create(&ptid, NULL, main_exec_thread, NULL);
    if(res < 0) {
        ALOGE("Can't create pthread: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

void print_usage() {
    const char *usage = "mmi usage:\n\
					mmi -b [normal pcba full ftm]   : change boot mode and reboot\n";

    ALOGE("%s\n", usage);
}

void set_boot_mode(boot_mode_type mode) {
    int fd;

    ALOGI("In set_boot_mode: misc dev:%s\n", mmi_input.misc_dev);
    if(strlen(mmi_input.misc_dev) == 0) {
        strlcpy(mmi_input.misc_dev, DEFAULT_MISC_DEV_PATH, sizeof(mmi_input.misc_dev));
    }

    fd = open(mmi_input.misc_dev, O_WRONLY);
    if(fd < 0) {
        ALOGE("open misc fail");
    } else {
        if(write(fd, boot_mode_string[mode], strlen(boot_mode_string[mode])) != strlen(boot_mode_string[mode])) {
            ALOGE("write misc fail ");
        }
        fsync(fd);
        close(fd);
    }
}

void myreboot() {
    system("reboot");
    exit(0);
}

void set_all_modules_run_mode(case_run_mode_t mode) {
    list < mmi_module * >*module_list;
    list < mmi_module * >::iterator iter;

    module_list = mmi_module::get_module_list();

    for(iter = module_list->begin(); iter != module_list->end(); iter++) {
        mmi_module *module = *iter;

        module->set_run_mode(mode);
    }
}

/*Diag used function */
int excute_single_test(const char *domain) {
    list < mmi_module * >*module_list;
    list < mmi_module * >::iterator iter;
    int res = 0;

    module_list = mmi_module::get_module_list();

    if(module_list->empty())
        return FTM_FAIL;
    mmi_module *module = get_module_by_domain(domain);

    if(module == NULL) {
        ALOGE("No %s found\n", domain);
        return FTM_FAIL;
    }

    res = launch_module(module);
    return (res<0) ? FTM_FAIL : res;
}

int excute_all_test() {
    list < mmi_module * >*module_list;
    list < mmi_module * >::iterator iter;
    int res = 0;

    module_list = mmi_module::get_module_list();
    if(module_list->empty())
        return FTM_FAIL;

    for(iter = module_list->begin(); iter != module_list->end(); iter++) {
        mmi_module *module = *iter;

        res = launch_module(module);
        if(res)
            break;
    }
    return res ? FTM_FAIL : FTM_SUCCESS;
}

int get_mmi_state(void) {
    return state_instance->get_state();
}

int get_mmi_fail_count(void) {
    return state_instance->get_totoal_fail_count();
}

int reconfig_mmi(char *path, int *numOfTestCaseFromCfgFile) {

    FILE *file = NULL;
    int ret = CFG_SUCCESS;

    if(!check_file_exist(path)) {
        ALOGE(" %s . did not exist ..\n\n", path);
        if(!check_file_exist(DEFAULT_CFG))
            return CFG_DEFUAL_NOT_FOUND;
        return CFG_NOT_FOUND;
    }

/*restore the cfg file path to cfg.seq*/
    ALOGE(" %s . start cfg file to seq.cfg..\n\n", path);

    file = fopen(CURRENT_CFG_SEQ, "w");
    if(file != NULL) {
        fwrite(path, 1, strlen(path), file);
        fclose(file);
        clean_up();
        init_config();
        load_all_test_case();
        set_all_modules_run_mode(TEST_MODE_PCBA);
        *numOfTestCaseFromCfgFile = mmi_config::get_config_domain_list().size();
    }

    return ret;
}
int test_list_to_file(char *filepath) {

    list < mmi_module * >*module_list;
    list < mmi_module * >::iterator iter;
    FILE *fp = NULL;

    ALOGE("%s \n", filepath);
    char buf[256] = { 0 };

    module_list = mmi_module::get_module_list();
    if(module_list->empty())
        return FTM_FAIL;

    fp = fopen(filepath, "w");
    if(!fp)
        return FTM_FAIL;

    for(iter = module_list->begin(); iter != module_list->end(); iter++) {
        mmi_module *module = *iter;

        memset(buf, 0, sizeof(buf));
        ALOGE("%s \n", module->get_domain());
        strlcat(buf, module->get_domain(), sizeof(buf));
        strlcat(buf, "\n", sizeof(buf));
        buf[sizeof(buf) - 1] = '\0';
        fwrite(buf, strlen(buf), 1, fp);
    }
    fclose(fp);
    fp = NULL;

    return FTM_SUCCESS;
}


/*Erase the content of the test result file (.res)
for the current selected sequence loaded
by command FTM_AP_SELECT_SEQUENCE*/
int clear_cur_test_result() {
    FILE *fp = NULL;

    /*open as Write, and then close, mean clear file content */
    fp = fopen(state_instance->get_cur_result_file(), "w");
    if(!fp)
        return FTM_FAIL;
    fclose(fp);
    return FTM_SUCCESS;

}

/* if you want to debugdup2(logfd, STDOUT_FILENO);dup2(logfd, STDERR_FILENO);*/
int init_log() {
    int fd;

    fd = open(DEBUG_LOG, (O_RDWR | O_CREAT | O_APPEND), S_IRWXU | S_IRWXG);
    if(fd < 0) {
        perror("open /data/FTM_AP/mmi.log:");
        return -1;
    }
    close(fd);
    return 0;
}

void init_config() {

    FILE *file = NULL;
    int count = 0;
    int i = 0;
    char cfg_file[256] = { 0 };
    char res_file[256] = { 0 };
    bool found = false;


    ALOGE("-------fastmmi begin------------\n");

    // PCBA config file needs to be in /data/FTM_AP, so we copy it there:
    if (copy_file(DEFAULT_PCBA_CFG_INSTALATION, DEFAULT_PCBA_CFG))
        ALOGE("Error copying PCBA config to FTM dir");
    if(check_file_exist(CURRENT_CFG_SEQ)) {
        file = fopen(CURRENT_CFG_SEQ, "r");
        if(file != NULL) {
            count = fread(cfg_file, 1, 255, file);
            fclose(file);

        }
        file = fopen(CURRENT_CFG_SEQ, "w");
        if(file != NULL) {
            /*restore the default config file */
            fwrite(DEFAULT_CFG, 1, strlen(DEFAULT_CFG), file);
            fclose(file);
        }
    }
    /*check if the cfg is exist */
    if(cfg_file == NULL || !check_file_exist(cfg_file)) {
        strlcpy(cfg_file, DEFAULT_CFG, sizeof(cfg_file));
    }
    ALOGI(" cfg file= %s \n", cfg_file);

    // Res filename should not contain the extension of the config file...
    string res_file_name = strrchr(cfg_file, '/');
    int last_index = res_file_name.find_last_of(".");
    if (last_index != string::npos) {
        res_file_name = res_file_name.substr(0, last_index);
    }
    res_file_name = BASE_DIR + res_file_name;
    res_file_name += ".res";
    strlcpy(res_file, res_file_name.c_str(), sizeof(res_file));
    ALOGE("res file:: %s \n", res_file);

    mmi_config::load_config_from_file(cfg_file, false);
    state_instance->set_cur_config_file(cfg_file);
    state_instance->set_cur_result_file(res_file);

    /*initial MMI input params */
    strlcpy(mmi_input.misc_dev, mmi_config::query_config_value(MMI_DOMAIN, "misc_dev", DEFAULT_MISC_DEV_PATH),
            sizeof(mmi_input.misc_dev));
    strlcpy(mmi_input.sys_backlight, mmi_config::query_config_value(MMI_DOMAIN, "sys_backlight", DEFAULT_MISC_DEV_PATH),
            sizeof(mmi_input.sys_backlight));
    mmi_input.refresh_interval =
        atoi(mmi_config::query_config_value(MMI_DOMAIN, "refresh_interval", DEFAULT_REFLRESH_INTERVAL_MS));
    const char *mode = mmi_config::query_config_value(MMI_DOMAIN, "test_mode");

    if(mode != NULL) {
        for(i = 0; i < MAX_TEST_MODE; i++) {
            if(!strncmp(test_mode_string[i], mode, strlen(test_mode_string[i]))) {
                mmi_input.test_mode = (case_run_mode_t) i;
                found = true;
                break;
            }
        }
    }
    if(!found)
        mmi_input.test_mode = TEST_MODE_NONE;

    ALOGE("mmi input: misc_dev:%s;  backlight:%s; refresh_interval:%d,test_mode=%s\n", mmi_input.misc_dev,
          mmi_input.sys_backlight, mmi_input.refresh_interval, test_mode_string[mmi_input.test_mode]);
}

static int init_boot_mode(char *mode) {
    /* PCBA + Modem + MMI + ... */
    state_instance->set_cur_boot_type(BOOT_MODE_FASTMMI_FULL);

    if(!strncmp(boot_mode_string[BOOT_MODE_FASTMMI_PCBA], mode, strlen(boot_mode_string[BOOT_MODE_FASTMMI_PCBA]))) {
        state_instance->set_cur_boot_type(BOOT_MODE_FASTMMI_PCBA);  /* PCBA only */
    } else
        if(!strncmp(boot_mode_string[BOOT_MODE_FASTMMI_FULL], mode, strlen(boot_mode_string[BOOT_MODE_FASTMMI_FULL]))) {
        state_instance->set_cur_boot_type(BOOT_MODE_FASTMMI_FULL);  /* PCBA + MMI */
    } else if(!strncmp(boot_mode_string[BOOT_MODE_FTM], mode, strlen(boot_mode_string[BOOT_MODE_FTM]))) {
        state_instance->set_cur_boot_type(BOOT_MODE_FTM);   /* Modem */
    } else {
        state_instance->set_cur_boot_type(BOOT_MODE_NORMAL);    /* Modem */
        return -1;
    }
    state_instance->set_cur_boot_mode(mode);
    ALOGE("ro.bootmode=%s: 0x%x\n", mode, state_instance->get_cur_boot_type());
    return 0;
}
static case_run_mode_t get_test_mode() {
    case_run_mode_t mode = TEST_MODE_NONE;

    if(mmi_input.test_mode != TEST_MODE_NONE) {
        mode = mmi_input.test_mode;
    } else {
        if(state_instance->is_fastmmi_full_mode())
            mode = TEST_MODE_UI;
        else if(state_instance->is_fastmmi_pcba_mode())
            mode = TEST_MODE_PCBA;
        else
            mode = TEST_MODE_NONE;
    }
    return mode;
}
static bool is_ui_mode() {
    return get_test_mode() == TEST_MODE_UI;
}
static bool is_pcba_mode() {
    return get_test_mode() == TEST_MODE_PCBA;
}
static bool is_sanity_mode() {
    return get_test_mode() == TEST_MODE_SANITY;
}

static void do_cmdline(int argc, char *argv[]) {
    int ch;

    while((ch = getopt(argc, argv, "b:")) != EOF) {
        switch (ch) {
        case 'b':
            if(optarg == NULL) {
                ALOGE("please specify boot mode: mmi -b [normal pcba full ftm].\n");
                exit(-1);
            }
            if(!strncmp(optarg, "pcba", 4)) {
                set_boot_mode(BOOT_MODE_FASTMMI_PCBA);
                myreboot();
            } else if(!strncmp(optarg, "full", 4)) {
                set_boot_mode(BOOT_MODE_FASTMMI_FULL);
                myreboot();
            } else if(!strncmp(optarg, "ftm", 3)) {
                set_boot_mode(BOOT_MODE_FTM);
                myreboot();
            } else if(!strncmp(optarg, "normal", 6)) {
                set_boot_mode(BOOT_MODE_NORMAL);
                myreboot();
            } else {
                ALOGE("Error: error boot mode,[normal pcba full ftm]  is ok\n");
                exit(-1);
            }
            break;
        default:
            print_usage();
            exit(-1);
            break;
        }
    }
}

void clean_up() {
    mmi_config::free_config();
    get_main_module()->free_modules();
    state_instance->clear_tc_info_items();
}

int main(int argc, char *argv[]) {
    lcd_res_s lcd_res;
    char boot_mode[PROPERTY_VALUE_MAX] = { 0 };

    property_get("ro.bootmode", boot_mode, "normal");

    /*Not ffbm mode , block here */
    if(strncmp(boot_mode, "ffbm", 4)) {
        ALOGE("MMI app can only be run in ffbm mode");
        do {
            usleep(1000 * 1000 * 1000);
        } while(1);
    }

    /*cmdline checking */
    do_cmdline(argc, argv);

    write_file("sys/power/wake_lock", "mmi");
    mkdirs(BASE_DIR);
    state_instance = mmi_state::get_instance();
    if(!state_instance) {
        ALOGE("Fail to get state instance , exit \n");
        return -1;
    }
    if(init_log())
        return -1;

    if(init_boot_mode(boot_mode)) {
        set_boot_mode(BOOT_MODE_NORMAL);
        ALOGI("Rebooting to normal mode");
        myreboot();
        return -1;
    }

    init_config();
    load_all_test_case();
    sem_init(&g_sem_exit, 0, 0);
    if(diag_init())
        ALOGE("ffbm did not support diag yet\n");

    if(is_ui_mode()) {
        ALOGI("Manual cases begining\n");
        set_all_modules_run_mode(TEST_MODE_UI);
        /*initial UI */
        gr_init();
        lcd_res.fb_height = gr_fb_height();
        lcd_res.fb_width = gr_fb_width();
        ALOGI("lcd: w=%d,h=%d\n", lcd_res.fb_width, lcd_res.fb_height);
        state_instance->set_lcd_res(&lcd_res);

        build_main_ui();
        if(create_draw_thread()) {
            ALOGE("create draw thread fail\n");
            goto out;
        }
        if(create_input_thread()) {
            ALOGE("create input thread fail\n");
            goto out;
        }
        if(create_main_exec_thread()) {
            ALOGE("create main exec thread fail\n");
            goto out;
        }
    } else if(is_pcba_mode()) {
        if(create_input_thread()) {
            ALOGE("create input thread fail");
            goto out;
        }
        ALOGI("PCBA test cases begining\n");
        set_all_modules_run_mode(TEST_MODE_PCBA);
        if(create_main_exec_thread()) {
            ALOGE("create main exec thread fail\n");
            goto out;
        }
    } else if(is_sanity_mode()) {
        ALOGE("SANITY test cases begining\n");
        set_all_modules_run_mode(TEST_MODE_SANITY);
        if(create_main_exec_thread()) {
            ALOGE("create main exec thread fail\n");
            goto out;
        }
    } else {
        ALOGE("No test mode found\n");
        goto out;
    }

    sem_wait(&g_sem_exit);
  out:
    sem_close(&g_sem_exit);
    diag_deinit();
    ALOGE("Restarting  \n");
    fflush(stdout);
    write_file("sys/power/wake_unlock", "mmi");
    myreboot();
    return 0;
}
