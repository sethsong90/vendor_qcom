/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include "exec.h"
#include <list>
#include <pthread.h>
#include <unistd.h>
#include <mmi_module_manage.h>
#include "mmi.h"

using namespace std;

void *main_exec_thread(void *) {
    list < exec_unit_t >::iterator iter;
    exec_unit_t tmp;
    mmi_module *cur_module;

    while(1) {
        cur_module = get_current_module();
        if(cur_module == NULL) {
            usleep(EXEC_DELAY_NS);
            continue;
        }

        cur_module->exec_list_lock();
        list < exec_unit_t > *exec_list = cur_module->get_exec_list();

        if(exec_list->begin() != exec_list->end()) {
            iter = exec_list->begin();
            tmp = *iter;
            exec_list->erase(exec_list->begin());
            cur_module->exec_list_unlock();
            if(tmp.cb != NULL) {
                tmp.cb(tmp.component);
            }
        } else {
            cur_module->exec_list_unlock();
            usleep(EXEC_DELAY_NS);
        }
    }
    return NULL;
}
