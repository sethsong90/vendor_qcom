/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __LIBMMI_MODULE_MANAGE__
#define __LIBMMI_MODULE_MANAGE__
#include <list>
#include <string>
#include <pthread.h>
#include <mmi_button.h>
#include <mmi_text.h>
#include <mmi_window.h>
#include <mmi_item.h>
#include <hash_map>
#include "mmi_component.h"

class mmi_button;
using namespace std;

typedef int (*module_entry_t) (mmi_module * mod);

typedef void *(*thread_t) (void *);

typedef struct {
    mmi_cb_t cb;
    void *component;
} exec_unit_t;

typedef enum {
    TEST_MODE_NONE = 0,
    TEST_MODE_PCBA = 1,
    TEST_MODE_UI = 2,
    TEST_MODE_SANITY = 3,
    MAX_TEST_MODE
} case_run_mode_t;

typedef pair < string, string > Result_Pair;

class mmi_module {

  public:
    mmi_module(const char *name, module_entry_t entry);
    char *get_domain();
    void set_domain(const char *);
    void add_btn(mmi_button * btn);
    void add_btn_with_title(mmi_rect_t rect, const char *str, mmi_cb_t cb);
    void add_btn_pass(mmi_cb_t cb);
    void add_btn_fail(mmi_cb_t cb);
    void delete_btn(mmi_button * btn);
    void add_text(mmi_text * text);
    void delete_text(mmi_text * text);
    void add_window(mmi_window * window);
    void delete_window(mmi_window * window);
    void add_item(mmi_item * item);
    void delete_item(mmi_item * item);
      list < mmi_button * >*get_btn_list();
      list < mmi_text * >*get_text_list();
      list < mmi_window * >*get_window_list();
      list < mmi_item * >*get_item_list();
    int get_btn_count();
    void add_module_to_list();
    void win_btn_text_list_lock();
    void win_btn_text_list_unlock();
    module_entry_t get_module_entry();
    void exec_add_cb(mmi_cb_t cb, void *btn);
      list < exec_unit_t > *get_exec_list();
    mmi_text *find_text_match_extra(void *extra);
    void exec_list_lock();
    void exec_list_unlock();
    void launch();
    int wait();
    int get_suc_num();
    int get_failed_num();
    void set_result(int);
    int get_result();
    void *get_handle(void);
    void set_handle(void *handle);
    void *get_libname(void);
    void set_libname(const char *);
    void clean_source();
    void clean_btn();
    void clean_text();
    void clean_window();
    void free_modules();
    void *exec_thread();
    void set_run_mode(case_run_mode_t);
    case_run_mode_t get_run_mode();
    static mmi_module *register_module(const char *domain, module_entry_t entry);
    static bool check_module_by_handle(void *handle);
    static list < mmi_module * >*get_module_list();
    /* handle output parameters from the test */
    void clearResults();
    void addStringResult(string key, string value);
    void addIntResult(string key, int value);
    void addDoubleResult(string key, double value);
    void  addFloatResult(string key, float value);
      hash_map < string, string > *getResults();

  private:
    void *m_handle;
    char m_domain[64];
    char m_libname[64];
    module_entry_t m_entry;
      list < mmi_button * >m_btn_list;
    pthread_mutex_t m_win_btn_text_mutex;
      list < mmi_text * >m_text_list;
      list < mmi_window * >m_window_list;
      list < mmi_item * >m_item_list;
      list < exec_unit_t > m_exec_list;
    pthread_mutex_t m_exec_list_mutex;
    pthread_t m_module_ptid;
    pthread_t m_exec_ptid;
    int m_suc_num;
    int m_fail_num;
    int m_success;
    case_run_mode_t m_run_mode;
      hash_map < string, string > m_results;    /* a map of key/value for the parametric results */
    static list < mmi_module * >module_list;
};

mmi_module *get_module_by_domain(const string domain);
mmi_module *get_module_by_domain(const char *domain);

#endif
