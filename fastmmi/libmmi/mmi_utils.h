/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef _MMI_UTILS__
#define __MMI_UTILS__

#include <string>
#include <list>
#include <vector>
#include <hash_map>
#include <mmi_module_manage.h>
using namespace std;
char *trim(char *str);

void trim(string & src);
enum {
    CASE_FAIL = -1,
    CASE_SUCCESS = 0,
    USER_PASS_PCBA_PASS = 0,
    EXT_APP_NOT_FOUND = 1, // An external application cannot be found
    EXT_CONFIG_NOT_FOUND = 2, // An external configuration file cannot be found
    INPUT_PARAM_OUT_OF_RANGE =3, //The input parameter value is out of range
    INPUT_PARAM_MISSING =4, // The input parameter is missing from
                            // configuration file
    SCAN_EXECUTION_TIME_OUT = 5, // The test execution timed out. Applicable to
                                 // test case requireing system scan. Example,
                                 // GPS fix, BT/WiFi Scan, touch
    UNKOWN_ERROR = 6, // Unknown error
    HW_NOT_PRESENT = 7,
    PCBA_VOLUME_UP_KEY_FAIL = 50, // Fail to detect volume up key
    PCBA_VOLUME_DOWN_KEY_FAIL = 51, // Fail to detect volume down key
    PCBA_CAMERA_KEY_FAIL = 52, // Fail to detect camera key
    PCBA_POWER_KEY_FAIL = 53, // Fail to detect power key
    PCBA_SD_CARD_READ_FAIL = 54, // Fail to detect SD card
    PCBA_SIM_CARD_0_FAILURE = 55, // Fail to detect SIM card 0
    PCBA_SIM_CARD_1_FAILURE = 56, // Fail to detect SIM card 1
    PCBA_SENSOR_READ_FAIL = 57, // Fail to open sensor to read data
    PCBA_FAIL_TO_OEPN_CAMERA = 58 // Fail to open camera
};

void parse_parameter(const char *src, hash_map < string, string > &paras);
void parse_parameter(const string src, hash_map < string, string > &paras);

/*create recursive directories*/
int mkdirs(const char *path);

void write_file(const char *path, const char *content);
int write_file_res(const char *path, const char *content);
int copy_file(char *from, char *to);

/*
 * get parameters from hash_map via key
 * if no matched key, return default value
*/
void get_para_value(const hash_map < string, string > paras, const char *key, char *value, int len, const char *def);
void get_para_value(const hash_map < string, string > paras, const char *key, string & value, const char *def);
void get_para_value(const hash_map < string, string > paras, const string key, string & value, const char *def);

/* convert string to long */
long string_to_long(const string src);
long string_to_long(const char *src);
unsigned long string_to_ulong(const string src);
unsigned long string_to_ulong(const char *src);

void exe_cmd(const char *cmd, char *result, int size);
int exe_cmd_res(const char *cmd, char *result, int size);
bool check_file_exist(const char *path);
int get_device_index(char *device, char *path, int *deviceIndex);
int parse_nv_by_indicator(const char *line, char indicator, char *name, int nameLen, char *value, int valueLen);
int get_pid_by_name(const char *name);
#endif
