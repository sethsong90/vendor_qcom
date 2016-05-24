/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __LIBMMI_KEY__
#define __LIBMMI_KEY__

#define LOG_TAG   "mmi_key"
#include <cutils/log.h>

#include <linux/input.h>
typedef void (*mmi_key_cb_t) (int);

class mmi_key {

  public:
    static void set_key_cb(unsigned int key, mmi_key_cb_t cb);
    static mmi_key_cb_t get_key_cb(unsigned int key);
    static void clear();
  private:
    static mmi_key_cb_t key_cbs[KEY_CNT];
};

#endif
