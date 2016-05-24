/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#include <stdio.h>
#include <string.h>
#include <string>
#include <hash_map>
#include <mmi_utils.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define LOG_TAG   "mmi_util"
#include <cutils/log.h>

using namespace std;

char *trim(char *str) {
    char *p = str;

    if(*p == '\0') {
        return p;
    }

    while(*p == ' ' || *p == '\n' || *p == '\t') {
        p++;
    }
    if(*p == '\0') {
        return p;
    }

    char *end = p + strlen(p) - 1;

    while(*end == ' ' || *end == '\n' || *end == '\t') {
        *end = '\0';
        end--;
    }
    return p;
}

void trim(string & src) {
    const string delim = " \n\t";

    src.erase(src.find_last_not_of(delim) + 1);
    src.erase(0, src.find_first_not_of(delim));
}

int mkdirs(const char *path) {
    int len = strlen(path);
    char *dir = (char *) malloc(len + 1);

    if(dir == NULL) {
        ALOGE("virtual memory exhausted");
        return -1;
    }

    strlcpy(dir, path, len);
    int i;

    for(i = 0; i < len; i++) {
        if(i > 0 && dir[i] == '/') {
            dir[i] = '\0';
            if(access(dir, F_OK)) {
                if(mkdir(dir, 0770)) {
                    free(dir);
                    return -1;
                }
            }
            dir[i] = '/';
        }
    }
    if(access(dir, F_OK)) {
        if(mkdir(dir, 0777)) {
            free(dir);
            return -1;
        }
    }
    free(dir);
    return 0;
}

void parse_parameter(const string src, hash_map < string, string > &paras) {
    string key, value;

    paras.clear();
    if(src.length() < 3)
        return;

    unsigned int first = 0, next = 0;
    string sub;

    while(1) {
        next = src.find_first_of(';', first);
        if(first > src.length() - 3)
            break;
        if(next < first)
            next = src.length();
        sub = src.substr(first, next - first);
        int index = sub.find(':');

        if(index < 0)
            break;
        if(index > 0 && index < sub.length()) {
            key = sub.substr(0, index);
            trim(key);
            value = sub.substr(index + 1);
            trim(value);
            key.find_first_not_of(' ');
            paras[key] = value;
        }
        first = next + 1;
    }

}

void parse_parameter(const char *src, hash_map < string, string > &paras) {
    if(src != NULL)
        parse_parameter(string(src), paras);
}

void write_file(const char *path, const char *content) {
    FILE *fp = fopen(path, "a");

    if(fp == NULL) {
        perror(path);
        return;
    }
    fwrite(content, sizeof(char), strlen(content), fp);
    fclose(fp);
}


// This version returns an error if file failed to open
int write_file_res(const char *path, const char *content) {
    FILE *fp = fopen(path, "a");

    if(fp == NULL) {
        perror(path);
        return CASE_FAIL;
    }
    fwrite(content, sizeof(char), strlen(content), fp);
    fclose(fp);
    return CASE_SUCCESS;
}

int copy_file(char *from, char *to) {
    int childExitStatus;
    pid_t pid;
    int ret = -1;

    if (!from || !to) {
        return ret;
    }

    pid = fork();

    if (pid == 0) {
        char *args[5] =
            { "/system/bin/cp", from, to, NULL };
        if (execv("/system/bin/cp", args))
            exit(1);
        exit(0);
    }
    else if (pid < 0) {
        ALOGE("Copy error %s -> %s", from, to);
        return ret;
    }
    else {
        return (pid == waitpid(pid, NULL, 0)?0:-1);
    }
    return ret;
}

void get_para_value(const hash_map < string, string > paras, const char *key, char *value, int len, const char *def) {
    hash_map < string, string > parameters = paras;
    strlcpy(value, parameters[key].c_str(), len);
    if(strlen(value) == 0 && def != NULL) {
        strlcpy(value, def, len);
    }
}
void get_para_value(const hash_map < string, string > paras, const char *key, string & value, const char *def) {
    hash_map < string, string > parameters = paras;
    value = parameters[key];
    if(value.empty())
        value = (string) def;
}

void get_para_value(const hash_map < string, string > paras, const string key, string & value, const char *def) {
    get_para_value(paras, key.c_str(), value, def);
}

long string_to_long(const string src) {
    string s = src;

    return strtol(s.c_str(), NULL, 0);
}

long string_to_long(const char *src) {
    return string_to_long((string) src);
}

unsigned long string_to_ulong(const string src) {
    string s = src;

    return strtoul(s.c_str(), NULL, 0);
}

unsigned long string_to_ulong(const char *src) {
    return string_to_ulong((string) src);
}

void exe_cmd(const char *cmd, char *result, int size) {
    char buf_ps[1024];
    char ps[1024] = { 0 };
    static FILE *ptr;

    strlcpy(ps, cmd, sizeof(ps));
    if((ptr = popen(ps, "r")) != NULL) {
        while(fgets(buf_ps, sizeof(buf_ps), ptr) != NULL) {
            strlcat(result, buf_ps, size);
            if(strlen(result) >= (unsigned int) size - 1) {
                break;
            }
        }
        ALOGE("close the pipe \n");
        pclose(ptr);
        ptr = NULL;
    } else {
        ALOGE("popen %s error\n", ps);
    }
}

int exe_cmd_res(const char *cmd, char *result, int size) {
    char buf_ps[1024];
    char ps[1024] = { 0 };
    static FILE *ptr;
    int res = 0;

    strlcpy(ps, cmd, sizeof(ps));
    if((ptr = popen(ps, "r")) != NULL) {
        while(fgets(buf_ps, sizeof(buf_ps), ptr) != NULL) {
            strlcat(result, buf_ps, size);
            if(strlen(result) >= (unsigned int) size - 1) {
                break;
            }
        }
        ALOGE("close the pipe \n");
        res = pclose(ptr);
        ptr = NULL;
    } else {
        ALOGE("popen %s error\n", ps);
        res = CASE_FAIL;
    }
    return res;
}

bool check_file_exist(const char *path) {
    struct stat buffer;
    int err = stat(path, &buffer);

    if(err == 0)
        return true;
    if(errno == ENOENT) {
        ALOGE("file: %s  do not exist \n", path);
        return false;
    }
    return false;
}

int get_device_index(char *device, char *path, int *deviceIndex) {
    if(device == NULL || path == NULL)
        return -1;
    string input(device);
    string num;
    int index = input.find_first_of(path);

    if(index >= 0) {
        num = input.substr(strlen(path));
        *deviceIndex = (int) strtol(num.c_str(), NULL, 10);
        return 0;
    } else
        return -1;
}

/*parser "name indicator value" format,eg:name=value */
int parse_nv_by_indicator(const char *line, char indicator, char *name, int nameLen, char *value, int valueLen) {
    if(line == NULL || name == NULL || value == NULL)
        return -1;
    string input(line);
    int splitIndex = input.find_first_of(indicator);

    if(splitIndex > 0) {
        if(splitIndex < nameLen && strlen(line) - splitIndex - 1 < valueLen) {
            strlcpy(name, line, nameLen);
            name[splitIndex] = '\0';
            strlcpy(value, line + splitIndex + 1, valueLen);
            value[strlen(line) - splitIndex - 1] = '\0';
            return 0;
        } else
            return -1;
    } else
        return -1;
}

int get_pid_by_name(const char *name) {
    char line[1024] = { 0 };
    FILE *ptr;
    int pid = -1;
    char *token, *subtoken;

    if((ptr = popen("ps", "r")) != NULL) {
        while(fgets(line, sizeof(line), ptr) != NULL) {
            char *p = &line[0];

            p = strstr(line, name);
            if(p != NULL) {
                ALOGE("getline: %s \n", line);
                char *save_ptr;

                token = strtok_r(line, "  ", &save_ptr);

                if(token != NULL) {
                    ALOGE("%s\n", token);
                    subtoken = strtok_r(NULL, "  ", &save_ptr);
                    if(subtoken != NULL)
                        pid = atoi(subtoken);
                }
                break;
            }
        }
        ALOGE("close the pipe \n");
        pclose(ptr);
        ptr = NULL;
    } else {
        ALOGE("popen  error\n");
    }

    return pid;
}
