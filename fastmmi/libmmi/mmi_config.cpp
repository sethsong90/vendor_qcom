/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#include <mmi_config.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <list>
#include <vector>
#include <hash_map>
#include <mmi_utils.h>
#include <unistd.h>
#include "mmi_state.h"

#define LOG_TAG   "mmi_config"
#include <cutils/log.h>

using namespace std;

list < mmi_config_item_t * >mmi_config::config_list;

int mmi_config::load_config_from_file(const char *path, bool is_check) {
    char cur_domain[1024] = { 0, };
    char line[1024] = { 0, };
    char buf[256] = { 0, };
    char indicator = '=';
    mmi_state *sinstance = mmi_state::get_instance();
    FILE *file = fopen(path, "rw");

    if(file == NULL) {
        perror(path);
        return CONFIG_NOT_FOUND_ERR;
    }

    while(fgets(line, sizeof(line), file) != NULL) {
        char name[1024] = { 0, }, value[1024] = { 0,};

        if(line[0] == '#') {
            continue;
        }
        if(line[0] == '[') {
            parse_domain(line, cur_domain, sizeof(cur_domain));

            sinstance->add_tc_info_item(cur_domain);
            ALOGE("config: current domain = %s\n", cur_domain);
            continue;
        }
        if(cur_domain[0] != '\0') {
            parse_nv_by_indicator(line, indicator, name, sizeof(name), value, sizeof(value));
            char *pname = trim(name);
            char *pvalue = trim(value);

            if(*pname != '\0' && *pvalue != '\0') {
                if(!strncmp(pname, "lib_name", strlen(pname))) {
                    snprintf(buf, sizeof(buf), "/system/vendor/lib/%s", pvalue);
                    if(access(buf, F_OK)) {
                        ALOGE("%s not found\n", buf);
                        return CONFIG_TEST_CASE_ERR;
                    }
                }
                if(!is_check) {
                    add_config_item(cur_domain, pname, pvalue);
                }
                ALOGE("config: %s = %s\n", pname, pvalue);
            }
        }
    }

    fclose(file);
    return CONFIG_SUCCESS;
}
void mmi_config::add_config_item(char *domain, char *name, char *value) {
    mmi_config_item_t *item = new mmi_config_item_t;

    strlcpy(item->domain, domain, sizeof(item->domain));
    strlcpy(item->name, name, sizeof(item->name));
    strlcpy(item->value, value, sizeof(item->value));
    config_list.push_back(item);
}

void mmi_config::free_config() {
    while(!config_list.empty()) {
        delete config_list.front();
        config_list.pop_front();
    }
}

const char *mmi_config::query_config_value(const char *domain, const char *name) {
    list < mmi_config_item_t * >::iterator iter;
    for(iter = config_list.begin(); iter != config_list.end(); iter++) {
        mmi_config_item_t *config = *iter;

        if(!strncasecmp(config->domain, domain, sizeof(config->domain))
           && !strncasecmp(config->name, name, sizeof(config->domain))) {
            return config->value;
        }
    }
    return NULL;
}

const char *mmi_config::query_config_value(const char *domain, const char *name, const char *def) {
    list < mmi_config_item_t * >::iterator iter;
    for(iter = config_list.begin(); iter != config_list.end(); iter++) {
        mmi_config_item_t *config = *iter;

        if(!strncasecmp(config->domain, domain, sizeof(config->domain))
           && !strncasecmp(config->name, name, sizeof(config->domain))) {
            return config->value;
        }
    }
    return def;
}

const char *mmi_config::get_config_domain_name(mmi_module * m) {
    const char *str = m->get_domain();
    return str;
}

const vector < string > mmi_config::get_config_domain_list() {
    static vector < string > config_domain_list;

    config_domain_list.clear();
    list < mmi_config_item_t * >::iterator iter;
    string last, cur;

    for(iter = config_list.begin(); iter != config_list.end(); iter++) {
        mmi_config_item_t *config = *iter;

        cur = string(config->domain);
        if(last != cur)
            config_domain_list.push_back(string(config->domain));
        last = cur;
    }
    return config_domain_list;
}
int mmi_config::parse_domain(const char *line, char *domain, int domainLen) {
    if(line == NULL || domain == NULL)
        return -1;
    string input(line);
    int startIndex = input.find_first_of('[');
    int endIndex = input.find_first_of(']');

    if(startIndex >= 0 && endIndex >= 0 && endIndex > startIndex + 1) {
        if(endIndex - startIndex - 1 < domainLen) {
            strlcpy(domain, line + startIndex + 1, domainLen);
            domain[endIndex - startIndex - 1] = '\0';
            return 0;
        } else
            return -1;
    } else
        return -1;
}
