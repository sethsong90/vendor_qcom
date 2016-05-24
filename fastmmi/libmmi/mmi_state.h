/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __SYSTEM_CORE_MMI_STATE__
#define __SYSTEM_CORE_MMI_STATE__
#include <string.h>
#include <linux/input.h>
#include <list>


using namespace std;
typedef enum {
    BOOT_MODE_NORMAL = 0,
    BOOT_MODE_FASTMMI_PCBA = 1,
    BOOT_MODE_FASTMMI_FULL = 2,
    BOOT_MODE_FTM = 3,
} boot_mode_type;

typedef enum {
    MMI_IDLE = 0x0,
    MMI_BUSY = 0x1,
} mmi_state_type;

typedef struct lcd_resolution {
    int fb_width;
    int fb_height;
} lcd_res_s;
typedef struct tc_info {
    int is_used;                // 1 mean used by test case
    char name[256];
    int result;
    time_t start_time;
    time_t end_time;
    int suc_num;
    int fail_num;
    int error_code;
    int sub_error_code;
    char input_param[512];
    char output_data[512];
} tc_info_s;

#define MAX_SUPPORTED_CASE 50

class mmi_state {
  public:
    static mmi_state *get_instance(void);
    void release_instance(void);
     ~mmi_state(void);
    mmi_state_type get_state(void);
    void set_state(mmi_state_type state);
    int get_totoal_fail_count(void);
    int get_running_mode(void);
    void set_running_mode(int mode);
    int get_case_num(void);
    void set_case_num(int num);
    lcd_res_s *get_lcd_res(void);
    void set_lcd_res(lcd_res_s * lcd_res);
    struct input_absinfo *get_ts_abs_X(void);
    struct input_absinfo *get_ts_abs_Y(void);
    char *get_cur_boot_mode(void);
    void set_cur_boot_mode(char *mode);
    boot_mode_type get_cur_boot_type(void);
    bool is_fastmmi_full_mode(void);
    bool is_fastmmi_pcba_mode(void);
    bool is_fastmmi_ftm_mode(void);
    bool is_fastmmi_normal_mode(void);
    void set_cur_boot_type(boot_mode_type type);
    char *get_cur_config_file(void);
    void set_cur_config_file(char *filename);
    char *get_cur_result_file();
    void set_cur_result_file(char *filename);
    tc_info_s *get_tc_info_by_name(const char *name);
    void add_tc_info_item(char *name);
    void clear_tc_info_items(void);
    bool get_debug_enable(void);
    void set_debug_enable(bool enable);
    bool get_trace_debug_enable(void);
    void set_trace_debug_enable(bool enable);

  private:
      mmi_state(void);
    static mmi_state *m_instance;
    static int m_count;

    mmi_state_type m_state;     //idle , excuting
    bool m_debug_enable;
    bool m_trace_debug_enable;
    unsigned int m_runing_mode;
    unsigned int m_case_num;

    lcd_res_s m_lcd_res;
    struct input_absinfo m_ts_iabs[2];  //touch sreen

    char m_cur_boot_mode[256];
    char m_cur_config_file[256];
    char m_cur_result_file[256];
    boot_mode_type m_cur_boot_type;

    static list < tc_info_s * >tc_info_list;
};
#endif
