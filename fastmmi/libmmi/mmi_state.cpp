/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#include "mmi_state.h"
#include <stdio.h>

using namespace std;

int mmi_state::m_count = 0;
mmi_state *mmi_state::m_instance = NULL;

list < tc_info_s * >mmi_state::tc_info_list;

mmi_state::mmi_state() {
    m_state = MMI_IDLE;
    m_runing_mode = 0;
    m_case_num = 0;
    m_trace_debug_enable = false;
    m_debug_enable = false;
    memset(m_cur_boot_mode, 0, sizeof(m_cur_boot_mode));
    memset(m_cur_config_file, 0, sizeof(m_cur_config_file));
    memset(m_cur_result_file, 0, sizeof(m_cur_result_file));
}
mmi_state::~mmi_state() {
}
mmi_state *mmi_state::get_instance() {
    m_count++;
    if(m_instance == NULL) {
        m_instance = new mmi_state();
    }
    return m_instance;
}
void mmi_state::release_instance() {
    m_count--;
    if(m_count == 0)
        delete m_instance;
}
mmi_state_type mmi_state::get_state() {
    return m_state;
}
void mmi_state::set_state(mmi_state_type state) {
    m_state = state;
}
int mmi_state::get_totoal_fail_count() {
    int total_count = 0;

    list < tc_info_s * >::iterator iter;

    if(tc_info_list.empty())
        return 0;

    for(iter = tc_info_list.begin(); iter != tc_info_list.end(); iter++) {
        tc_info_s *tc = *iter;

        total_count += tc->fail_num;
    }

    return total_count;
}
int mmi_state::get_running_mode() {
    return m_runing_mode;
}
void mmi_state::set_running_mode(int mode) {
    m_runing_mode = mode;
}

boot_mode_type mmi_state::get_cur_boot_type() {
    return m_cur_boot_type;
}
void mmi_state::set_cur_boot_type(boot_mode_type type) {
    m_cur_boot_type = type;
}
bool mmi_state::is_fastmmi_full_mode() {
    return m_cur_boot_type == BOOT_MODE_FASTMMI_FULL;
}
bool mmi_state::is_fastmmi_pcba_mode() {
    return m_cur_boot_type == BOOT_MODE_FASTMMI_PCBA;
}
bool mmi_state::is_fastmmi_ftm_mode() {
    return m_cur_boot_type == BOOT_MODE_FTM;
}
bool mmi_state::is_fastmmi_normal_mode() {
    return m_cur_boot_type == BOOT_MODE_NORMAL;
}
int mmi_state::get_case_num() {
    return m_case_num;
}
void mmi_state::set_case_num(int num) {
    m_case_num = num;
}
lcd_res_s *mmi_state::get_lcd_res() {
    return &m_lcd_res;
}
void mmi_state::set_debug_enable(bool enable) {
    m_debug_enable = enable;
}
bool mmi_state::get_debug_enable() {
    return m_debug_enable;
}
void mmi_state::set_trace_debug_enable(bool enable) {
    m_trace_debug_enable = enable;
}
bool mmi_state::get_trace_debug_enable() {
    return m_trace_debug_enable;
}

void mmi_state::set_lcd_res(lcd_res_s * lcd_res) {
    m_lcd_res.fb_height = lcd_res->fb_height;
    m_lcd_res.fb_width = lcd_res->fb_width;
}
struct input_absinfo *mmi_state::get_ts_abs_X() {
    return &m_ts_iabs[0];
}
struct input_absinfo *mmi_state::get_ts_abs_Y() {
    return &m_ts_iabs[1];
}
char *mmi_state::get_cur_boot_mode() {
    return m_cur_boot_mode;
}
void mmi_state::set_cur_boot_mode(char *mode) {
    strlcpy(m_cur_boot_mode, mode, sizeof(m_cur_boot_mode));
}
char *mmi_state::get_cur_config_file() {
    return m_cur_config_file;
}
void mmi_state::set_cur_config_file(char *filename) {
    strlcpy(m_cur_config_file, filename, sizeof(m_cur_config_file));
}
char *mmi_state::get_cur_result_file() {
    return m_cur_result_file;
}
void mmi_state::set_cur_result_file(char *filename) {
    strlcpy(m_cur_result_file, filename, sizeof(m_cur_result_file));
}
tc_info_s *mmi_state::get_tc_info_by_name(const char *name) {

    list < tc_info_s * >::iterator iter;

    if(tc_info_list.empty() || !name)
        return NULL;

    for(iter = tc_info_list.begin(); iter != tc_info_list.end(); iter++) {
        tc_info_s *tc = *iter;

        if(!strncasecmp(tc->name, name, strlen(tc->name)))
            return tc;
    }

    return NULL;
}
void mmi_state::add_tc_info_item(char *name) {
    tc_info_s *item = new tc_info_s;

    strlcpy(item->name, name, sizeof(item->name));
    tc_info_list.push_back(item);
}

void mmi_state::clear_tc_info_items() {
    while(!tc_info_list.empty()) {
        delete tc_info_list.front();
        tc_info_list.pop_front();
    }
}
