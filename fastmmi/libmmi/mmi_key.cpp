/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#include <mmi_key.h>
#include <string.h>

mmi_key_cb_t mmi_key::key_cbs[KEY_CNT] = { NULL };

void mmi_key::set_key_cb(unsigned int key, mmi_key_cb_t cb) {
    if(key >= KEY_CNT) {
        return;
    }
    key_cbs[key] = cb;
}

mmi_key_cb_t mmi_key::get_key_cb(unsigned int key) {
    if(key >= KEY_CNT) {
        return NULL;
    }
    return key_cbs[key];
}

void mmi_key::clear() {
    memset(&key_cbs, 0, sizeof(key_cbs));
}
