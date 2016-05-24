/*
 * ----------------------------------------------------------------------------
 *  Copyright (c) 2007-2009 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 *  ---------------------------------------------------------------------------
 *
 */

#ifndef DUN_SERVICE_H
#define DUN_SERVICE_H
#include <errno.h>

#define LOG_TAG "dun_service"

#ifdef LE_PORT_BRIDGE_DBG
#define LOGE(...) fprintf(stderr, "I:" __VA_ARGS__)
#ifndef LOGI
#define LOGI(...) fprintf(stderr, "I:" __VA_ARGS__)
#endif
#else
#define LOG_NIDEBUG 0
#include <utils/Log.h>
#include "common_log.h"
#endif

#define DUN_MAXBUFSIZE 8192

/* Enable to route the debug messages to file */
/*#define DUN_LOGFILE_ENABLE */

#ifdef DUN_LOGFILE_ENABLE
#define DUN_LOGFILE "/data/dunlog.txt"
#endif

#ifdef LE_PORT_BRIDGE_DBG
#define DUN_FIFO_FILE "/data/dun"
#define DUN_INITIATED (int)'1'
#define DUN_END (int)'2'
#else
#define DUN_INITIATED 0
#define DUN_END 1
#endif

#define TRUE 1
#define FALSE 0
#define SINGLE_PDP 1

typedef enum {
   DUN_ATCMD_INVALID,
   DUN_ATCMD_START,
   DUN_ATCMD_STOP
}DUN_ATCMD_E;

typedef struct {
   char smdportfname[100];
   char extportfname[100];
   int smdport_fd;
   int extport_fd;
   pthread_t portsmonitor_thread;
   pthread_t portdataxfr_thread_up;
   pthread_t portdataxfr_thread_dn;
}dun_portparams_s;

typedef enum {
   DUN_STATE_ERROR = 0,
   DUN_STATE_USB_UNPLUG,
   DUN_STATE_IDLE,
   DUN_STATE_DCDSBL_WAIT1,
   DUN_STATE_DCDSBL_WAIT2,
   DUN_STATE_CONNECTED,
   DUN_STATE_DCENBL_WAIT1,
   DUN_STATE_DCENBL_WAIT2,
   DUN_STATE_MAX
} DUN_STATE_E;

typedef enum {
   DUN_EVENT_ERROR = 0,
   DUN_EVENT_USB_UNPLUG,
   DUN_EVENT_USB_PLUG,
   DUN_EVENT_RMNET_DOWN,
   DUN_EVENT_RMNET_UP,
   DUN_EVENT_START,
   DUN_EVENT_STOP,
   DUN_EVENT_MAX,
} DUN_EVENT_E;

typedef struct {
   DUN_EVENT_E event;
   char        *unused;
} dun_event_msg_s;

/* USB states */
typedef enum {
   DUN_USBSTATE_ERROR,
   DUN_USBSTATE_UNPLUG,
   DUN_USBSTATE_PLUG,
} DUN_USBSTATE_E;


typedef enum {
   DUN_USBMODEMSTATE_ERROR,
   DUN_USBMODEMSTATE_NOTCONFIGURED,
   DUN_USBMODEMSTATE_CONFIGURED,
} DUN_USBMODEMSTATE_E;


/* rmnet states */
typedef enum {
   DUN_RMNETSTATE_ERROR,
   DUN_RMNETSTATE_DOWN,
   DUN_RMNETSTATE_UP,
} DUN_RMNETSTATE_E;

extern int dun_rmnet_pipefds[2];
extern int dun_ctrl_pipefds[2];

/* for debugging use */
extern char *DUN_EVENT_STR[DUN_EVENT_MAX];
extern char *DUN_STATE_STR[DUN_STATE_MAX];

extern dun_portparams_s dun_portparams;

extern void* dun_monitor_kevents(void *arg);
extern int dun_disable_data_connection(void);
extern int dun_enable_data_connection(void);
extern int dun_rmnet_status_msg(void);
extern int dun_start_ports_threads(dun_portparams_s *);
extern int dun_stop_ports_threads(dun_portparams_s *);


/* utility APIs */
extern void  dun_loge(const char *fmt, ...);
extern void  dun_logv(const char *fmt, ...);
extern void  dun_post_event(DUN_EVENT_E event);

#endif /* DUN_SERVICE_H */


