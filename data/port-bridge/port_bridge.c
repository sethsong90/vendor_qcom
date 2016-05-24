/*
 * ----------------------------------------------------------------------------
 *  Copyright (c) 2007-2009 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 *  ---------------------------------------------------------------------------
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <termios.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "port_bridge.h"

#ifdef DUN_LOGFILE_ENABLE
int dun_logfile_fd;
#endif

int dun_ctrl_pipefds[2];
int dun_rmnet_pipefds[2];

char *DUN_EVENT_STR[] = {
    "DUN_EVENT_ERROR",
    "DUN_EVENT_USB_UNPLUG",
    "DUN_EVENT_USB_PLUG",
    "DUN_EVENT_RMNET_DOWN",
    "DUN_EVENT_RMNET_UP",
    "DUN_EVENT_START",
    "DUN_EVENT_STOP"
};
char *DUN_STATE_STR[] = {
    "DUN_STATE_ERROR",
    "DUN_STATE_USB_UNPLUG",
    "DUN_STATE_IDLE",
    "DUN_STATE_DCDSBL_WAIT1",
    "DUN_STATE_DCDSBL_WAIT2",
    "DUN_STATE_CONNECTED",
    "DUN_STATE_DCENBL_WAIT1",
    "DUN_STATE_DCENBL_WAIT2",
};
DUN_STATE_E dun_state;
dun_portparams_s dun_portparams;

/* Forward declaration */
int dun_rmnet_post_msg(void);

/*===========================================================================

FUNCTION: dun_process_state_usb_unplug

DESCRIPTION: State during the USB cable unplugged

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  No DUN call possible

============================================================================*/
static void dun_process_state_usb_unplug(dun_event_msg_s msg)
{
    LOGI("received event(%s) in state(%s)\n",
             DUN_EVENT_STR[msg.event], DUN_STATE_STR[dun_state]);

    switch(msg.event)
    {
       case DUN_EVENT_USB_PLUG:
            dun_state = DUN_STATE_IDLE;
            dun_start_ports_threads(&dun_portparams);
            LOGI("Moved to state(%s)\n", DUN_STATE_STR[dun_state]);
            break;

       case DUN_EVENT_ERROR:
       case DUN_EVENT_RMNET_UP:
       case DUN_EVENT_STOP:
       case DUN_EVENT_START:
       case DUN_EVENT_RMNET_DOWN:
       case DUN_EVENT_USB_UNPLUG:
       default:
            break;
    }

    return;
}

/*===========================================================================

FUNCTION: dun_process_state_idle

DESCRIPTION: This is the initial state where there is no DUN call and is
             waiting for DUN command from external port by host.
DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  No DUN call possible

============================================================================*/
static void dun_process_state_idle(dun_event_msg_s msg)
{
  LOGI("received event(%s) in state(%s)\n",
            DUN_EVENT_STR[msg.event], DUN_STATE_STR[dun_state]);
  switch(msg.event)
  {
   case DUN_EVENT_USB_UNPLUG:
        dun_stop_ports_threads(&dun_portparams);
        dun_state = DUN_STATE_USB_UNPLUG;
        LOGI("Moved to state(%s)\n", DUN_STATE_STR[dun_state]);
        break;
   case DUN_EVENT_START:
#ifdef LE_PORT_BRIDGE_DBG
        dun_rmnet_post_msg();
        dun_state = DUN_STATE_CONNECTED;
#else
        dun_disable_data_connection();
        dun_state = DUN_STATE_DCDSBL_WAIT1;
#endif
        LOGI("Moved to state(%s)\n", DUN_STATE_STR[dun_state]);
        break;
   case DUN_EVENT_ERROR:
        dun_stop_ports_threads(&dun_portparams);
        dun_start_ports_threads(&dun_portparams);
        LOGI("Moved to state(%s)\n", DUN_STATE_STR[dun_state]);
        break;
   case DUN_EVENT_STOP:
   case DUN_EVENT_RMNET_DOWN:
   case DUN_EVENT_RMNET_UP:
   case DUN_EVENT_USB_PLUG:
   default:
        break;
  }

  return;
}

/*===========================================================================

FUNCTION: dun_process_state_dcdsbl_wait1

DESCRIPTION: System will enter into this state when it is waiting for data
             disable event to initiate DUN call

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
static void dun_process_state_dcdsbl_wait1(dun_event_msg_s msg)
{
  LOGI("received event(%s) in state(%s)\n",
              DUN_EVENT_STR[msg.event],
              DUN_STATE_STR[dun_state]);
  switch(msg.event) {
    case DUN_EVENT_RMNET_DOWN:
         dun_rmnet_post_msg();
         dun_state = DUN_STATE_CONNECTED;
         break;
    case DUN_EVENT_STOP:
         dun_enable_data_connection();
         dun_state = DUN_STATE_DCDSBL_WAIT2;
         break;
    case DUN_EVENT_USB_UNPLUG:
         dun_state = DUN_STATE_USB_UNPLUG;
         dun_enable_data_connection();
         dun_stop_ports_threads(&dun_portparams);
         break;
    case DUN_EVENT_START:
    case DUN_EVENT_ERROR:
    case DUN_EVENT_RMNET_UP:
    case DUN_EVENT_USB_PLUG:
    default:
         break;
  }
  LOGI("Moved to state(%s)\n", DUN_STATE_STR[dun_state]);
  return;
}

/*===========================================================================

FUNCTION: dun_process_state_dcdsbl_wait2

DESCRIPTION: System will enter into this state when system receives DUN_STOP
             event before disable data call event.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
static void dun_process_state_dcdsbl_wait2(dun_event_msg_s msg)
{
  LOGI("received event(%s) in state(%s)\n",
              DUN_EVENT_STR[msg.event],
              DUN_STATE_STR[dun_state]);
  switch(msg.event) {
    case DUN_EVENT_RMNET_UP:
         dun_state = DUN_STATE_IDLE;
         break;
    case DUN_EVENT_RMNET_DOWN:
         dun_state = DUN_STATE_DCENBL_WAIT1;
         break;
    case DUN_EVENT_USB_UNPLUG:
         dun_state = DUN_STATE_USB_UNPLUG;
         dun_enable_data_connection();
         dun_stop_ports_threads(&dun_portparams);
         break;
    case DUN_EVENT_START:
         dun_state = DUN_STATE_DCDSBL_WAIT1;
         break;
    case DUN_EVENT_ERROR:
         dun_enable_data_connection();
         dun_state = DUN_STATE_IDLE;
         break;
    case DUN_EVENT_STOP:
    case DUN_EVENT_USB_PLUG:
    default:
         break;
  }
  LOGI("Moved to state(%s)\n", DUN_STATE_STR[dun_state]);
  return;
}
/*===========================================================================

FUNCTION: dun_process_state_connected

DESCRIPTION: This is the state where DUN call is active.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
static void dun_process_state_connected(dun_event_msg_s msg)
{
  LOGI("received event(%s) in state(%s)\n",
              DUN_EVENT_STR[msg.event],
              DUN_STATE_STR[dun_state]);
  switch(msg.event) {
    case DUN_EVENT_USB_UNPLUG:
         dun_state = DUN_STATE_USB_UNPLUG;
         dun_enable_data_connection();
         dun_stop_ports_threads(&dun_portparams);
         break;
    case DUN_EVENT_STOP:
         dun_state = DUN_STATE_IDLE;
         dun_enable_data_connection();
         dun_stop_ports_threads(&dun_portparams);
         dun_start_ports_threads(&dun_portparams);
         break;
    case DUN_EVENT_RMNET_UP:
    case DUN_EVENT_ERROR:
    case DUN_EVENT_RMNET_DOWN:
    case DUN_EVENT_START:
    case DUN_EVENT_USB_PLUG:
    default:
         break;
  }
  LOGI("Moved to state(%s)\n", DUN_STATE_STR[dun_state]);
  return;
}

/*===========================================================================

FUNCTION: ddun_process_state_dcenbl_wait1

DESCRIPTION: This is the state where it is waiting for data call enable
             (RMNET_UP) event to go to idle state.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
static void dun_process_state_dcenbl_wait1(dun_event_msg_s msg)
{
  LOGI("received event(%s) in state(%s)\n",
              DUN_EVENT_STR[msg.event],
              DUN_STATE_STR[dun_state]);
  switch(msg.event) {
    case DUN_EVENT_RMNET_UP:
         dun_stop_ports_threads(&dun_portparams);
         dun_start_ports_threads(&dun_portparams);
         dun_state = DUN_STATE_IDLE;
         break;
    case DUN_EVENT_USB_UNPLUG:
         dun_state = DUN_STATE_USB_UNPLUG;
         dun_enable_data_connection();
         dun_stop_ports_threads(&dun_portparams);
         break;
    case DUN_EVENT_START:
         dun_disable_data_connection();
         dun_state = DUN_STATE_DCENBL_WAIT2;
         break;
    case DUN_EVENT_ERROR:
    case DUN_EVENT_USB_PLUG:
    case DUN_EVENT_RMNET_DOWN:
    case DUN_EVENT_STOP:
    default:
         break;
  }
  LOGI("Moved to state(%s)\n", DUN_STATE_STR[dun_state]);
  return;
}
/*===========================================================================

FUNCTION: ddun_process_state_dcenbl_wait1

DESCRIPTION: System enters into this state when it receives DUN_START event
             before data call enable event receives.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
static void dun_process_state_dcenbl_wait2(dun_event_msg_s msg)
{
  LOGI("received event(%s) in state(%s)\n",
              DUN_EVENT_STR[msg.event],
              DUN_STATE_STR[dun_state]);
  switch(msg.event) {
    case DUN_EVENT_USB_UNPLUG:
         dun_state = DUN_STATE_USB_UNPLUG;
         dun_enable_data_connection();
         dun_stop_ports_threads(&dun_portparams);
         LOGI("Moved to DUN_STATE_USB_UNPLUG\n");
         break;
    case DUN_EVENT_STOP:
         dun_enable_data_connection();
         dun_state = DUN_STATE_DCENBL_WAIT1;
         break;
    case DUN_EVENT_RMNET_UP: /* Datacall Enable OK*/
         dun_state = DUN_STATE_DCDSBL_WAIT1;
         break;
    case DUN_EVENT_RMNET_DOWN:
    case DUN_EVENT_START:
    case DUN_EVENT_ERROR:
    case DUN_EVENT_USB_PLUG:
    default:
         break;
  }
  LOGI("Moved to state(%s)\n", DUN_STATE_STR[dun_state]);
  return;
}
/*===========================================================================

FUNCTION: dun_process_event

DESCRIPTION: Process the DUN events based on the current state

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
static void dun_process_event(dun_event_msg_s msg)
{
  switch(dun_state) {
    case DUN_STATE_USB_UNPLUG:
         dun_process_state_usb_unplug(msg);
         break;

    case DUN_STATE_IDLE:
         dun_process_state_idle(msg);
         break;

    case DUN_STATE_CONNECTED:
         dun_process_state_connected(msg);
         break;

    case DUN_STATE_DCDSBL_WAIT1:
         dun_process_state_dcdsbl_wait1(msg);
         break;

    case DUN_STATE_DCDSBL_WAIT2:
         dun_process_state_dcdsbl_wait2(msg);
         break;

    case DUN_STATE_DCENBL_WAIT1:
         dun_process_state_dcenbl_wait1(msg);
         break;

    case DUN_STATE_DCENBL_WAIT2:
         dun_process_state_dcenbl_wait2(msg);
         break;

    default:
         break;
  }
  return;
}

/*===========================================================================
 *
 * main()
 *
============================================================================*/
int main(int argc, char *argv[])
{
   pthread_t kevent_thread;
   dun_event_msg_s event_msg;

   if(argc < 3) {
      LOGE("Usage: %s <SMD port dev node> <USB serial port dev node>\n",
               argv[0]);
      return -1;
   }

#ifdef DUN_LOGFILE_ENABLE
    dun_logfile_fd = open(DUN_LOGFILE, O_WRONLY | O_TRUNC, 0666);
    if(dun_logfile_fd < 0) {
       LOGE("Unable to create %s file to log the messages: %s\n",
             DUN_LOGFILE, strerror(errno));
    }
#endif

   memset(dun_portparams.smdportfname, 0, sizeof(dun_portparams.smdportfname));
   memset(dun_portparams.extportfname, 0, sizeof(dun_portparams.extportfname));
   strlcpy(dun_portparams.smdportfname, argv[1], sizeof(dun_portparams.smdportfname)-1);
   strlcpy(dun_portparams.extportfname, argv[2], sizeof(dun_portparams.extportfname)-1);

   /* create pipe for IPC */
   if(0 > pipe(dun_ctrl_pipefds)) {
      LOGE("Error while creating pipe for IPC : %s \n", strerror(errno));
      return -1;
   }

   /*Create pipe for communication with data transfer thread */
   if (0 > pipe (dun_rmnet_pipefds)) {
      LOGE("failure in creating a rm_fd pipe %s\n",strerror(errno));
      return -1;
   }

   /* create kernel events  thread */
   if( pthread_create(&kevent_thread, NULL, dun_monitor_kevents,
                      (void *)NULL) < 0) {
      LOGE("Unable to create usb event thread : %s\n", strerror(errno));
      return -1;
   }

   /* initial DUN system state */
   dun_state = DUN_STATE_USB_UNPLUG;

   while(1) {
      /* read messages from pipe */
      if(read(dun_ctrl_pipefds[0], &event_msg, sizeof(event_msg))
             != sizeof(event_msg)) {
         LOGE("Error while reading the message from pipe : %s\n",
                  strerror(errno));
         return -1;
      }
      if(event_msg.event > DUN_EVENT_MAX){
         LOGE("Error: event_msg.event does not exist : %s\n", strerror(errno));
         return -1;
      }
      dun_process_event(event_msg);
   }

#ifdef DUN_LOGFILE_ENABLE
   if(dun_logfile_fd)
      close(dun_logfile_fd);
#endif

   return 0;
}
