/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <time.h>
#include <list>
#include <mmi_button.h>
#include <mmi_module_manage.h>
#include <mmi_key.h>
#include "input.h"
#include "mmi.h"
#include "mmi_utils.h"
#include "exec.h"
#include <errno.h>

enum {
    VERSION = 0,
    KEY_CODE,
    CENTER_X,
    CENTER_Y,
    WIDTH,
    HEIGHT,
} VKEY_SEQUENCE;

struct vkey_map_struct {
    int version;
    int key_code;
    int center_x;
    int center_y;
    int width;
    int height;
};

union vkey_map {
    struct vkey_map_struct map;
    int vkey_map_value[sizeof(struct vkey_map_struct) / sizeof(int)];
};

#define MAX_KEYMAP_LINES 10

using namespace std;
static const char *TAG = "mmi_input";
static unsigned int lcd_x;
static unsigned int lcd_y;
static unsigned int ts_x_max;
static unsigned int ts_x_min;
static unsigned int ts_y_max;
static unsigned int ts_y_min;

struct key_state keys[KEY_MAX + 1];
union vkey_map g_key_map[MAX_KEYMAP_LINES];

#define BITS_PER_LONG  (sizeof(unsigned long) * 8)
#define BITS_TO_LONGS(x)  (((x) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#define test_bit(bit, array)   ((array)[(bit)/BITS_PER_LONG] & (1 << ((bit) % BITS_PER_LONG)))
extern mmi_state *state_instance;


touch_event_t touch_event;

/* current time in milliseconds */
static int64_t curr_time_ms(void) {
    struct timespec tm;

    clock_gettime(CLOCK_MONOTONIC, &tm);
    return tm.tv_sec * MSEC_PER_SEC + (tm.tv_nsec / NSEC_PER_MSEC);
}

static int64_t ts_to_time_ms(struct timeval *tv) {
    return tv->tv_sec * MSEC_PER_SEC + (tv->tv_usec / USEC_PER_MSEC);
}

static int set_key_callback(int type, int code, int value, void *data) {
    int down = ! !value;
    int64_t now = curr_time_ms();

    if(code > KEY_MAX)
        return -1;

    /* ignore events that don't modify our state */
    if(keys[code].down == down)
        return 0;

    /* only record the down even timestamp, as the amount
     * of time the key spent not being pressed is not useful */
    if(down) {
        keys[code].timestamp = now;
        keys[code].pending = true;
    }
    keys[code].down = down;
    return 0;
}

static int vkey_to_keycode(union vkey_map *key_map, int x, int y) {
    int i;

    for(i = 0; i < MAX_KEYMAP_LINES; i++) {
        if((abs(x - key_map[i].map.center_x) <= (key_map[i].map.width >> 1)) &&
           (abs(y - key_map[i].map.center_y) <= (key_map[i].map.height >> 1))) {
            return key_map[i].map.key_code;
        }
    }

    return -1;
}

static int parse_vkey_map(char *tp_name, union vkey_map *key_map) {
    char path[PATH_MAX];
    char *buffer;
    off_t length;
    int fd_map;
    char *str;
    char *save_ptr1, *save_ptr2;
    char *main_token;
    char *sub_token;
    char *substr;
    int val;
    int i, j;

    strlcpy(path, "/sys/board_properties/virtualkeys.", sizeof(path));
    strlcat(path, tp_name, sizeof(path));

    fd_map = open(path, O_RDONLY | O_CLOEXEC);
    if(fd_map < 0) {
        ALOGE("could not open virtual key map file:%s (%s)\n", path, strerror(errno));
        return -1;
    }

    length = lseek(fd_map, 0, SEEK_END);
    if(length < 0) {
        ALOGE("could not seek to the end of file:%s (%s)\n", path, strerror(errno));
        close(fd_map);
        return -1;
    }
    lseek(fd_map, 0, SEEK_SET);

    buffer = (char *) malloc((size_t) length);
    if(buffer == NULL) {
        ALOGE("malloc %ld bytes failed:(%s)\n", length, strerror(errno));
        close(fd_map);
        return -1;
    }

    if(read(fd_map, buffer, length) <= 0) {
        ALOGE("read from virtual key map file failed.(%s)\n", strerror(errno));
        close(fd_map);
        free(buffer);
        return -1;
    }
    buffer[length - 1] = '\0';

    /* Parse the virtual key map finally */
    for(str = buffer, i = 0;; str = NULL) {
        main_token = strtok_r(str, "\n", &save_ptr1);
        if(main_token == NULL)
            break;

        if(i >= MAX_KEYMAP_LINES) {
            ALOGE("lines exceeds max supported soft keys\n");
            break;
        }
        /* The comment line starts with '#' */
        if(main_token[0] == '#')
            continue;
        for(j = 0, substr = main_token;; substr = NULL, j++) {
            sub_token = strtok_r(substr, ":", &save_ptr2);
            if(sub_token == NULL)
                break;
            val = strtol(sub_token, NULL, 0);
            key_map[i].vkey_map_value[j] = val;
        }
        i++;
    }

    free(buffer);
    close(fd_map);
    return 0;
}

static int set_touch_callback(int type, int code, int value, void *data) {
    static int x_last = -1;
    static int y_last = -1;
    int key_code;

    int x = x_last;
    int y = y_last;

    if(type == EV_ABS) {
        if(code == ABS_X || code == ABS_MT_POSITION_X) {
            x_last = x = (value * lcd_x) / (ts_x_max - ts_x_min);
        } else if(code == ABS_Y || code == ABS_MT_POSITION_Y) {
            y_last = y = (value * lcd_y) / (ts_y_max - ts_y_min);
        } else if(code == ABS_MT_TRACKING_ID && value != 0xffffffff) {
            touch_event.trace_debug.clear();
        }
        touch_event.is_sync_lastevent = 0;
    } else if(type == EV_SYN) {
        static touch_point_t last_point = { -1, -1 };
        touch_point_t tu = { x, y };
        if(memcmp(&last_point, &tu, sizeof(tu)) != 0) {
            last_point = tu;
            key_code = vkey_to_keycode((union vkey_map *) data, tu.x, tu.y);
            if(key_code < 0)
                touch_event.trace.push_back(tu);
            else {
                ALOGI("Got virtual keycode:%d\n", key_code);
                set_key_callback(EV_KEY, key_code, 1, NULL);
                set_key_callback(EV_KEY, key_code, 0, NULL);
            }

            /*Store debug trace point */
            if(state_instance->get_trace_debug_enable()) {
                touch_event.trace_debug.push_back(tu);
            } else {
                touch_event.trace_debug.clear();
            }
        }
        touch_event.is_sync_lastevent = 1;
    }

    return 0;
}

int input_callback(int fd, short revents, void *data) {
    struct input_event ev;
    int ret;

    ret = ev_get_input(fd, revents, &ev);
    if(ret < 0)
        return -1;
    if(ev.type == EV_KEY) {
        set_key_callback(ev.type, ev.code, ev.value, NULL);
    } else if(ev.type == EV_ABS || ev.type == EV_SYN) {
        set_touch_callback(ev.type, ev.code, ev.value, g_key_map);
    }

    return 0;
}

static void process_key(int code, int64_t now) {
    struct key_state *key = &keys[code];

    if(key->down) {
        int64_t timeout = key->timestamp + TIME_KEY_LONG_PRESS;

        if(now >= timeout && key->pending) {
            key->pending = false;
        } else {
        }
    } else {
        /* if the power key got released, force screen state cycle */
        if(key->pending) {

            mmi_key_cb_t tmp = mmi_key::get_key_cb(code);

            if(tmp != NULL) {
                tmp(code);
            }
            key->pending = false;
        }
    }
}

bool ispoint_in_rect(int x, int y, mmi_rect_t * rect) {
    return (x >= rect->x && x <= rect->x + rect->w && y >= rect->y && y <= rect->y + rect->h);
}

bool isItem(int x, int y, mmi_item_pos_t * item) {

    return (x >= item->x_min && x <= item->x_max && y >= item->y_min && y <= item->y_max);
}

void screen_moved(list < touch_point_t > *trace) {
    touch_point_t pstart, pend;

    list < touch_point_t >::iterator tp_iter;
    tp_iter = trace->end();
    tp_iter--, tp_iter--;
    pstart = *tp_iter;
    tp_iter++;
    pend = *tp_iter;

    mmi_module *cur_module = get_current_module();

    if(cur_module == NULL)
        return;

    list < mmi_button * >*blist = cur_module->get_btn_list();
    list < mmi_button * >::iterator iter;

    cur_module->win_btn_text_list_lock();
    for(iter = blist->begin(); iter != blist->end(); iter++) {
        mmi_button *tmp = *iter;

        if(!ispoint_in_rect(pstart.x, pstart.y, tmp->get_rect()) && ispoint_in_rect(pend.x, pend.y, tmp->get_rect())) {
            if(tmp->get_cb_movein() != NULL) {
                cur_module->exec_add_cb(tmp->get_cb_movein(), tmp);
            }
            break;
        } else if(ispoint_in_rect(pstart.x, pstart.y, tmp->get_rect()) &&
                  !ispoint_in_rect(pend.x, pend.y, tmp->get_rect())) {
            if(tmp->get_cb_moveout() != NULL) {
                cur_module->exec_add_cb(tmp->get_cb_moveout(), tmp);
            }
            break;
        }
    }
    cur_module->win_btn_text_list_unlock();
}

void screen_pressed(int x, int y) {
    mmi_module *cur_module = get_current_module();

    if(cur_module == NULL)
        return;

    list < mmi_button * >*btnList = cur_module->get_btn_list();
    list < mmi_item * >*itemList = cur_module->get_item_list();
    list < mmi_button * >::iterator btnIter;
    list < mmi_item * >::iterator itemIter;

    cur_module->win_btn_text_list_lock();
    for(btnIter = btnList->begin(); btnIter != btnList->end(); btnIter++) {
        mmi_button *btn = *btnIter;

        if(ispoint_in_rect(x, y, btn->get_rect())) {
            if(btn->get_cb_pressed() != NULL) {
                cur_module->exec_add_cb(btn->get_cb_pressed(), btn);
            }
            break;
        }
    }
    for(itemIter = itemList->begin(); itemIter != itemList->end(); itemIter++) {
        mmi_item *item = *itemIter;

        if(isItem(x, y, item->getPosition())) {
            if(item->get_cb_pressed() != NULL) {
                cur_module->exec_add_cb(item->get_cb_pressed(), item);
            }
            break;
        }
    }
    cur_module->win_btn_text_list_unlock();
}
touch_xy_min_max_t find_min_max_in_trace(list < touch_point_t > *trace) {
    list < touch_point_t >::iterator iter;
    touch_point_t point;
    touch_xy_min_max_t ret;

    iter = trace->begin();
    point = *iter;

    ret.x_max = ret.x_min = point.x;
    ret.y_max = ret.y_min = point.y;

    for(iter = trace->begin(); iter != trace->end(); iter++) {
        point = *iter;

        if(point.x > ret.x_max) {
            ret.x_max = point.x;
        } else if(point.x < ret.x_min) {
            ret.x_min = point.x;
        }
        if(point.y > ret.y_max) {
            ret.y_max = point.y;
        } else if(point.y < ret.y_min) {
            ret.y_min = point.y;
        }
    }
    return ret;
}
void process_touch_abs() {
    static int last_event_size = 0;
    struct key_state *key = &keys[BTN_TOUCH];

    if(touch_event.is_sync_lastevent == 0) {
    } else if(!touch_event.trace.empty()) {
        if(key->down == true) {
            if(touch_event.trace.size() > 1) {
                if(last_event_size != (int) touch_event.trace.size()) {
                    screen_moved(&touch_event.trace);
                }
            }
        } else {
            touch_xy_min_max_t tmp = find_min_max_in_trace(&touch_event.trace);

            if(tmp.x_max - tmp.x_min <= PRESS_SHAKE_X && tmp.y_max - tmp.y_min <= PRESS_SHAKE_Y) {
                screen_pressed((tmp.x_max + tmp.x_min) / 2, (tmp.y_max + tmp.y_min) / 2);
            }
            touch_event.trace.clear();
            memset(key, 0, sizeof(*key));
            last_event_size = 0;
        }
    }
    last_event_size = touch_event.trace.size();
}

int get_ts_resolution() {

    DIR *dir;
    struct dirent *de;
    int fd;
    int i = 0;
    bool found = false;
    int max_event_index = 0;
    int index = 0;
    unsigned long keyBitmask[BITS_TO_LONGS(KEY_MAX)];
    unsigned long absBitmask[BITS_TO_LONGS(ABS_MAX)];
    char filepath[256] = { 0 };

    dir = opendir("/dev/input");
    if(dir == 0)
        return -1;

    while((de = readdir(dir))) {
        ALOGE("/dev/input/%s\n", de->d_name);
        if(strncmp(de->d_name, "event", 5))
            continue;
        get_device_index(de->d_name, "event", &index);
        if(index > max_event_index)
            max_event_index = index;
        ALOGE("/dev/input/%s: max:%d, index:%d\n", de->d_name, max_event_index, index);
    }

    for(i = 0; i < max_event_index + 1; i++) {
        unsigned long ev_bits[BITS_TO_LONGS(EV_MAX)];

        snprintf(filepath, sizeof(filepath), "/dev/input/event%d", i);
        fd = open(filepath, O_RDONLY);
        if(fd < 0)
            continue;

        /* read the evbits of the input device */
        if(ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
            close(fd);
            continue;
        }

        /* TODO: add ability to specify event masks. For now, just assume
         * that only EV_KEY and EV_REL event types are ever needed. */
        if(!test_bit(EV_KEY, ev_bits) && !test_bit(EV_REL, ev_bits)) {
            close(fd);
            continue;
        }

        /* read the evbits of the input device */
        if(ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBitmask)), keyBitmask) < 0) {
            close(fd);
            continue;
        }

        if(ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absBitmask)), absBitmask) < 0) {
            close(fd);
            continue;
        }
        /*See if this is a touch pad. Is this a new modern multi-touch driver */
        if(test_bit(ABS_MT_POSITION_X, absBitmask)
           && test_bit(ABS_MT_POSITION_Y, absBitmask)) {
            char buffer[80];

            ALOGE("ts dev: %s\n", filepath);
            found = true;

            if(ioctl(fd, EVIOCGNAME(sizeof(buffer) - 1), &buffer) < 1) {
                ALOGE("could not get device name for fd:%d, %s\n", fd, strerror(errno));
                close(fd);
                continue;
            } else {
                buffer[sizeof(buffer) - 1] = '\0';
                memset(g_key_map, 0, sizeof(g_key_map));
                parse_vkey_map(buffer, g_key_map);
            }

            if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), state_instance->get_ts_abs_X()) < 0) {
                close(fd);
                continue;
            }

            if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), state_instance->get_ts_abs_Y()) < 0) {
                close(fd);
                continue;
            }

            ALOGE("touchscreen resolution:(%d,%d)\n", state_instance->get_ts_abs_X()->maximum,
                  state_instance->get_ts_abs_Y()->maximum);
        }

        if(found)
            break;

    }
    closedir(dir);
    if(!found)
        return -1;

    return 0;

}


void *input_thread(void *) {
    int ret;

    ev_init(input_callback, NULL);
    if(get_ts_resolution()) {
        ALOGE("can not found a valid ts resolution \n");
        return NULL;
    }

    lcd_x = state_instance->get_lcd_res()->fb_width;
    lcd_y = state_instance->get_lcd_res()->fb_height;
    ts_x_max = state_instance->get_ts_abs_X()->maximum;
    ts_x_min = state_instance->get_ts_abs_X()->minimum;
    ts_y_max = state_instance->get_ts_abs_Y()->maximum;
    ts_y_min = state_instance->get_ts_abs_Y()->minimum;
    ALOGI("LCD(%d,%d)  TS(%d,%d;%d,%d)\n", lcd_x, lcd_y, ts_x_max, ts_x_min, ts_y_max, ts_y_min);

    while(1) {
        ret = ev_wait(0);
        if(!ret)
            ev_dispatch();
        process_key(KEY_VOLUMEDOWN, curr_time_ms());
        process_key(KEY_VOLUMEUP, curr_time_ms());

        process_touch_abs();

        /* Panel buttons test */
        process_key(KEY_BACK, curr_time_ms());
        process_key(KEY_HOME, curr_time_ms());
        process_key(KEY_HOMEPAGE, curr_time_ms());
        process_key(KEY_MENU, curr_time_ms());
        process_key(KEY_CAMERA_SNAPSHOT, curr_time_ms());
    }
    return NULL;
}
