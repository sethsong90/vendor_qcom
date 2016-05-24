/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <list>
using namespace std;

#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include "mmi_utils.h"
#include "mmi_module_manage.h"

#define LOG_TAG   "mmi_module_manager"
#include <cutils/log.h>

list < mmi_module * >mmi_module::module_list;

mmi_module::mmi_module(const char *name, module_entry_t entry) {
    strlcpy(this->m_domain, name, sizeof(this->m_domain));
    this->m_entry = entry;
    m_module_ptid = 0;
    m_exec_ptid = 0;

    pthread_mutex_init(&m_win_btn_text_mutex, NULL);
    pthread_mutex_init(&m_exec_list_mutex, NULL);

    m_suc_num = 0;
    m_fail_num = 0;
    m_success = 0xff;
    m_run_mode = TEST_MODE_UI;
    m_handle = NULL;
    memset(m_libname, 0, sizeof(m_libname));
}

mmi_module *mmi_module::register_module(const char *initialDomain, module_entry_t entry) {
    mmi_module *p = new mmi_module(initialDomain, entry);

    p->add_module_to_list();
    ALOGE("register module: %s \n", initialDomain);
    return p;
}

void mmi_module::add_module_to_list() {
    module_list.push_back(this);
}

list < mmi_module * >*mmi_module::get_module_list() {
    return &module_list;
}

char *mmi_module::get_domain() {
    return m_domain;
}

void mmi_module::set_domain(const char *domain) {
    strlcpy(m_domain, domain, sizeof(m_domain));
}

void *mmi_module::get_handle() {
    return m_handle;
}

void mmi_module::set_handle(void *handle) {
    m_handle = handle;
}

void *mmi_module::get_libname() {
    return m_libname;
}

void mmi_module::set_libname(const char *libname) {
    strlcpy(m_libname, libname, sizeof(m_libname));
}

void mmi_module::add_btn(mmi_button * btn) {
    this->win_btn_text_list_lock();
    this->m_btn_list.push_back(btn);
    this->win_btn_text_list_unlock();
}

void mmi_module::add_btn_with_title(mmi_rect_t rect, const char *str, mmi_cb_t cb) {
    class mmi_button *btn = new mmi_button(rect, str, cb);

    this->add_btn(btn);
}

void mmi_module::add_btn_pass(mmi_cb_t cb) {
    mmi_rect_t rect = { 10, gr_fb_height() * 4 / 5, gr_fb_width() / 2 - 20, gr_fb_height() / 5 - 60 };
    class mmi_button *btn = new mmi_button(rect, "PASS", cb);

    btn->set_color(0, 125, 125, 255);
    this->add_btn(btn);
}

void mmi_module::add_btn_fail(mmi_cb_t cb) {
    mmi_rect_t rect =
        { gr_fb_width() / 2 + 10, gr_fb_height() * 4 / 5, gr_fb_width() / 2 - 20, gr_fb_height() / 5 - 60 };
    class mmi_button *btn = new mmi_button(rect, "FAIL", cb);

    btn->set_color(125, 125, 0, 255);
    this->add_btn(btn);
}
void mmi_module::add_text(mmi_text * text) {
    this->win_btn_text_list_lock();
    this->m_text_list.push_back(text);
    this->win_btn_text_list_unlock();
}

void mmi_module::add_window(mmi_window * window) {
    this->win_btn_text_list_lock();
    this->m_window_list.push_back(window);
    this->win_btn_text_list_unlock();
}

void mmi_module::add_item(mmi_item * item) {
    this->win_btn_text_list_lock();
    this->m_item_list.push_back(item);
    this->win_btn_text_list_unlock();
}

list < mmi_button * >*mmi_module::get_btn_list() {
    return &m_btn_list;
}

list < mmi_text * >*mmi_module::get_text_list() {
    return &m_text_list;
}

list < mmi_window * >*mmi_module::get_window_list() {
    return &m_window_list;
}

list < mmi_item * >*mmi_module::get_item_list() {
    return &m_item_list;
}


void mmi_module::win_btn_text_list_lock() {
    pthread_mutex_lock(&m_win_btn_text_mutex);
}

void mmi_module::win_btn_text_list_unlock() {
    pthread_mutex_unlock(&m_win_btn_text_mutex);
}

module_entry_t mmi_module::get_module_entry() {
    return m_entry;
}

void mmi_module::exec_add_cb(mmi_cb_t cb, void *component) {
    exec_unit_t exec_unit = { cb, component };

    this->exec_list_lock();
    m_exec_list.push_back(exec_unit);
    this->exec_list_unlock();
}

list < exec_unit_t > *mmi_module::get_exec_list() {
    return &m_exec_list;
}

void mmi_module::exec_list_lock() {
    pthread_mutex_lock(&m_exec_list_mutex);
}

void mmi_module::exec_list_unlock() {
    pthread_mutex_unlock(&m_exec_list_mutex);
}

void *mmi_module::exec_thread() {
    int res;

    while(1) {
        res = pthread_kill(m_module_ptid, 0);
        if(res == ESRCH) {
            return NULL;
        }

        list < exec_unit_t > *exec_list = this->get_exec_list();
        list < exec_unit_t >::iterator iter;
        exec_unit_t tmp_exeu;

        this->exec_list_lock();
        if(exec_list->begin() != exec_list->end()) {
            iter = exec_list->begin();
            tmp_exeu = *iter;
            exec_list->erase(exec_list->begin());
            this->exec_list_unlock();
            if(tmp_exeu.cb != NULL) {
                this->win_btn_text_list_lock();
                int is_valid = 0;

                {
                    list < mmi_button * >::iterator btnIter;
                    for(btnIter = m_btn_list.begin(); btnIter != m_btn_list.end(); btnIter++) {
                        mmi_button *tmp_btn = *btnIter;

                        if(tmp_exeu.component == tmp_btn) {
                            is_valid = 1;
                            break;
                        }
                    }

                }
                this->win_btn_text_list_unlock();

                if(is_valid) {
                    tmp_exeu.cb(tmp_exeu.component);
                }
            }
        } else {
            this->exec_list_unlock();
            usleep(1000 * 10);
        }
    }

    return NULL;
}

void mmi_module::launch() {
    int res;

    res = pthread_create(&m_module_ptid, NULL, (thread_t) get_module_entry(), (void *) this);
    if(res < 0) {
        fprintf(stderr, "can't create pthread: %s\n", strerror(errno));
    }

    res = pthread_create(&m_exec_ptid, NULL, (thread_t) & mmi_module::exec_thread, (void *) this);
    if(res < 0) {
        fprintf(stderr, "can't create pthread: %s\n", strerror(errno));
    }
}

int mmi_module::wait() {
    void *thread_result;

    pthread_join(m_exec_ptid, NULL);
    m_exec_ptid = 0;

    pthread_join(m_module_ptid, &thread_result);
    m_module_ptid = 0;

    if(m_run_mode == TEST_MODE_UI) {
        if((int) thread_result == 0) {
            m_suc_num++;
            m_success = 0;
        } else {
            m_fail_num++;
            m_success = -1;
        }
    }

    return (int) thread_result;
}

int mmi_module::get_suc_num() {
    return m_suc_num;
}

int mmi_module::get_failed_num() {
    return m_fail_num;
}

int mmi_module::get_result() {
    return m_success;
}

void mmi_module::set_result(int res) {
    m_success = res;
}

void mmi_module::clean_source() {
    clean_btn();
    clean_text();
    clean_window();
}

mmi_text *mmi_module::find_text_match_extra(void *extra) {
    list < mmi_text * >::iterator iter;
    for(iter = m_text_list.begin(); iter != m_text_list.end(); iter++) {
        mmi_text *tmp = *iter;

        if(extra == tmp->get_extra()) {
            return tmp;
        }
    }
    return NULL;
}

void mmi_module::clean_btn() {
    list < mmi_button * >::iterator iter;
    mmi_button *tmp;

    this->win_btn_text_list_lock();

    while(m_btn_list.begin() != m_btn_list.end()) {
        tmp = *m_btn_list.begin();
        delete tmp;

        m_btn_list.erase(m_btn_list.begin());
    }
    this->win_btn_text_list_unlock();
}

void mmi_module::clean_text() {
    list < mmi_text * >::iterator iter;
    mmi_text *tmp;

    this->win_btn_text_list_lock();

    while(m_text_list.begin() != m_text_list.end()) {
        tmp = *m_text_list.begin();
        delete tmp;

        m_text_list.erase(m_text_list.begin());
    }
    this->win_btn_text_list_unlock();
}

void mmi_module::clean_window() {
    list < mmi_window * >::iterator iter;
    mmi_window *tmp;

    this->win_btn_text_list_lock();

    while(m_window_list.begin() != m_window_list.end()) {
        tmp = *m_window_list.begin();
        delete tmp;

        m_window_list.erase(m_window_list.begin());
    }
    this->win_btn_text_list_unlock();
}

void mmi_module::delete_btn(mmi_button * btn) {
    this->win_btn_text_list_lock();
    list < mmi_button * >::iterator iter;
    for(iter = m_btn_list.begin(); iter != m_btn_list.end(); iter++) {
        mmi_button *tmp = *iter;

        if(btn == tmp) {
            delete tmp;

            m_btn_list.erase(iter);
            break;
        }
    }
    this->win_btn_text_list_unlock();
}

void mmi_module::delete_text(mmi_text * text) {
    this->win_btn_text_list_lock();
    list < mmi_text * >::iterator iter;
    for(iter = m_text_list.begin(); iter != m_text_list.end(); iter++) {
        mmi_text *tmp = *iter;

        if(text == tmp) {
            delete tmp;

            m_text_list.erase(iter);
            this->win_btn_text_list_unlock();
            return;
        }
    }
    this->win_btn_text_list_unlock();
}

void mmi_module::delete_window(mmi_window * window) {
    this->win_btn_text_list_lock();
    list < mmi_window * >::iterator iter;
    for(iter = m_window_list.begin(); iter != m_window_list.end(); iter++) {
        mmi_window *tmp = *iter;

        if(window == tmp) {
            delete tmp;

            m_window_list.erase(iter);
            this->win_btn_text_list_unlock();
            return;
        }
    }
    this->win_btn_text_list_unlock();
}

void mmi_module::delete_item(mmi_item * item) {
    this->win_btn_text_list_lock();
    list < mmi_item * >::iterator iter;
    for(iter = m_item_list.begin(); iter != m_item_list.end(); iter++) {
        mmi_item *tmp = *iter;

        if(item == tmp) {
            delete tmp;

            m_item_list.erase(iter);
            this->win_btn_text_list_unlock();
            return;
        }
    }
    this->win_btn_text_list_unlock();
}

int mmi_module::get_btn_count() {
    int i;

    this->win_btn_text_list_lock();
    i = m_btn_list.size();
    this->win_btn_text_list_unlock();
    return i;
}

void mmi_module::set_run_mode(case_run_mode_t mode) {
    m_run_mode = mode;
}

case_run_mode_t mmi_module::get_run_mode() {
    return m_run_mode;
}

bool mmi_module::check_module_by_handle(void *handle) {
    list < mmi_module * >::iterator iter;

    if(module_list.empty() || !handle)
        return false;

    for(iter = module_list.begin(); iter != module_list.end(); iter++) {
        mmi_module *module = *iter;

        if(module->m_handle == handle)
            return true;
    }
    return false;
}

void mmi_module::clearResults() {
    m_results.clear();
}


void mmi_module::addStringResult(string key, string value) {
    m_results.insert(Result_Pair(key, value));
}

void mmi_module::addIntResult(string key, int value) {
    char sValue[256] = { 0 };
    snprintf(sValue, sizeof(sValue), "%d", value);
    m_results.insert(Result_Pair(key, sValue));
}

void mmi_module::addDoubleResult(string key, double value) {
    char sValue[256] = { 0 };
    snprintf(sValue, sizeof(sValue), "%6.2f", value);
    m_results.insert(Result_Pair(key, sValue));
}

void mmi_module::addFloatResult(string key, float value) {
    char sValue[256] = { 0 };
    snprintf(sValue, sizeof(sValue), "%5.1f", value);
    m_results.insert(Result_Pair(key, sValue));
}

hash_map < string, string > *mmi_module::getResults() {
    return &m_results;
}


mmi_module *get_module_by_domain(const char *domain) {

    list < mmi_module * >*module_list;
    list < mmi_module * >::iterator iter;

    module_list = mmi_module::get_module_list();

    if(module_list->empty())
        return NULL;
    for(iter = module_list->begin(); iter != module_list->end(); iter++) {
        mmi_module *module = *iter;

        if(!strncasecmp(module->get_domain(), domain, strlen(domain)))
            return module;
    }
    return NULL;

}

mmi_module *get_module_by_domain(const string domain) {
    return get_module_by_domain(domain.c_str());
}

void mmi_module::free_modules() {
    while(!module_list.empty()) {
        dlclose(module_list.front()->get_handle());
        delete module_list.front();
        module_list.pop_front();
    }
}
