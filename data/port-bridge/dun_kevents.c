/*
 * ----------------------------------------------------------------------------
 *  Copyright (c) 2007-2009 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 *  ---------------------------------------------------------------------------
 *
 */

#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
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
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <CommandApi.h>

#include "port_bridge.h"

#define DUN_USB_STATE_FILE    "/sys/devices/platform/msm_hsusb/gadget/usb_state"
#define DUN_USB_FUNCTION_FILE    "/sys/devices/virtual/android_usb/android0/functions"
#define DUN_USB_STATE_MODEM   "serial"
#define DUN_USB_CONFIG_STR    "USB_STATE_CONFIGURED"
#define MAX_IFS 8
#define DUN_USB_MAX_FUNCTION_FILE_SIZE  128

pthread_mutex_t dun_post_event_mutex;

struct dun_netlink_socks
{
   int uevent_sock;
   int route_sock;
};

/*===========================================================================

FUNCTION: dun_interface_check_status

DESCRIPTION: This function check rmnet current status for up/down

DEPENDENCIES
  None

RETURN VALUE
  dun_returnCodeType

SIDE EFFECTS
  None

============================================================================*/
static DUN_RMNETSTATE_E dun_interface_check_status(void)
{
    struct ifreq *ifr;
    struct ifreq *ifend;
    struct ifreq ifreq;
    struct ifreq ifs[MAX_IFS];
    struct ifconf ifc;
    int sock_fd;
    char *interface;
    int interface_num_up = 0;

    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    interface = "rmnet";

    if (sock_fd < 0) {
       LOGE("Unable to create AF_INET socket: %s\n", strerror(errno));
       return DUN_RMNETSTATE_ERROR;
    }

    if (ioctl(sock_fd, SIOCGIFCONF, &ifc) < 0) {
       LOGE("ioctl(SIOCGIFCONF) failed \n");
       close(sock_fd);
       return DUN_RMNETSTATE_ERROR;
    }

    ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));

    /* Run through all available interfaces */
    for (ifr = ifc.ifc_req; ifr < ifend; ifr++) {
       if (ifr->ifr_addr.sa_family == AF_INET) {
          strlcpy(ifreq.ifr_name, ifr->ifr_name,sizeof(ifreq.ifr_name));
          /* Filter loopback*/
          if (!strncmp(interface, ifreq.ifr_name, strlen(interface))) {
             if (ioctl(sock_fd, SIOCGIFFLAGS, &ifreq) < 0) {
                LOGE("ioctl(SIOCGIFFLAGS) failed\n");
                close(sock_fd);
                return DUN_RMNETSTATE_ERROR;
             }

             if (IFF_UP & ifreq.ifr_flags ) {
                interface_num_up++;
             }
          }
       }
    }

    close(sock_fd);
    if (interface_num_up > 0) {
        return DUN_RMNETSTATE_UP;
    } else {
        return DUN_RMNETSTATE_DOWN;
    }
}

/*===========================================================================

FUNCTION: dun_open_uevent_sock

DESCRIPTION: This function open uevent socket

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
static int dun_open_uevent_sock(struct dun_netlink_socks *socks)
{
    struct sockaddr_nl socknladdr;

    socks->uevent_sock = -1;
    memset(&socknladdr, 0, sizeof(socknladdr));
    socknladdr.nl_family = AF_NETLINK;
    socknladdr.nl_pid = getpid();
    socknladdr.nl_groups = 0xffffffff;

    if ((socks->uevent_sock = socket(PF_NETLINK, SOCK_DGRAM,
                                     NETLINK_KOBJECT_UEVENT)) < 0) {
       LOGE("Unable to create uevent socket: %s\n", strerror(errno));
       return -1;
    }

    if (bind(socks->uevent_sock, (struct sockaddr *)&socknladdr,
             sizeof(socknladdr)) < 0) {
       LOGE("Unable to bind uevent socket: %s\n", strerror(errno));
       return -1;
    }
    return 0;
}

/*===========================================================================

FUNCTION: dun_parse_usbevent

DESCRIPTION: This function parse usb event

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
static DUN_USBSTATE_E dun_parse_usbevent(char *buf, int cnt)
{
   char *str = buf;
   char *end;
   char subsystem[100];
   char switch_name[100];
   char switch_state[100];

   memset(subsystem, 0, 100);
   memset(switch_name, 0, 100);
   memset(switch_state, 0, 100);

   end = str + cnt;
   /* skip path */
   str += strlen(str) + 1;
   while (str < end) {
       if (!strncmp(str, "SUBSYSTEM=", strlen("SUBSYSTEM=")))
           strlcpy(subsystem, (str + strlen("SUBSYSTEM=")), sizeof(subsystem)-1);
       else if (!strncmp(str, "USB_STATE=", strlen("USB_STATE=")))
           strlcpy(switch_state, (str + strlen("USB_STATE=")), sizeof(subsystem)-1);

       str += strlen(str) + 1;
   }

   if (strstr(subsystem, "android_usb")) {
       LOGI("got usb %s event \n", switch_state);
       if (strstr(switch_state, "CONFIGURED"))
           return DUN_USBSTATE_PLUG;
       else if(strstr(switch_state, "DISCONNECTED"))
           return DUN_USBSTATE_UNPLUG;
   }
   return DUN_USBSTATE_ERROR;
}

/*===========================================================================

FUNCTION: dun_getusbmodemstate_fromsys

DESCRIPTION: This function checks if  MODEM composition is selected or not

DEPENDENCIES
  None

RETURN VALUE
  DUN_USBMODEMSTATE_E

SIDE EFFECTS
  None

============================================================================*/
static DUN_USBMODEMSTATE_E  dun_getusbmodemstate_fromsys(void)
{
  DUN_USBMODEMSTATE_E  mstate;
  static DUN_USBMODEMSTATE_E prev_mstate = DUN_USBMODEMSTATE_NOTCONFIGURED;
  int fd;
  char c[DUN_USB_MAX_FUNCTION_FILE_SIZE];

  memset(c, 0, DUN_USB_MAX_FUNCTION_FILE_SIZE);

  fd = open(DUN_USB_FUNCTION_FILE, O_RDONLY);

  if(fd < 0) {
          LOGE("Error while reading for modem composition \n");
          mstate = DUN_USBMODEMSTATE_NOTCONFIGURED;
  } else {
      /**
       * This part of code reads checks if serial composition is present
       * in USB_FUNCTION_FILE to support kernel 3.0 changes.
       */

      if(read(fd,c,DUN_USB_MAX_FUNCTION_FILE_SIZE) < 0) {
          LOGE("Error while reading the file %s : %s\n",
                  DUN_USB_FUNCTION_FILE, strerror(errno));
          mstate = DUN_USBMODEMSTATE_ERROR;
      }
      c[sizeof(c) - 1] = '\0';
      if(strstr(c,"serial")) {
              mstate = DUN_USBMODEMSTATE_CONFIGURED;
      } else {
              mstate = DUN_USBMODEMSTATE_NOTCONFIGURED;
      }
  }

  if (prev_mstate != mstate) {
      LOGI("The modem state is changed to %d ", mstate);
      prev_mstate = mstate;
  }
  close(fd);
  return mstate;
}

/*===========================================================================

FUNCTION: dun_getusbstate_fromsys

DESCRIPTION: This function get usb state from sys file

DEPENDENCIES
  None

RETURN VALUE
  DUN_USBSTATE_E

SIDE EFFECTS
  None

============================================================================*/
static DUN_USBSTATE_E dun_getusbstate_fromsys(void)
{
   DUN_USBSTATE_E ustate;
   int fd;
   char buf[1024];

   fd = open(DUN_USB_STATE_FILE, O_RDONLY);
   if(fd < 0) {
      LOGI("Error while opening the file %s : %s \n",
               DUN_USB_STATE_FILE, strerror(errno));
      /* USB device is not registered yet. Treat this as
       * UNPLUGGED state. We will be woken up by CONNECT
       * uevent later.
       */
      return DUN_USBSTATE_UNPLUG;
   }

   memset(buf, 0, 1024);

   if(read(fd, buf, 1024) <0) {
      LOGE("Error while reading the file %s : %s\n",
                 DUN_USB_STATE_FILE, strerror(errno));
      ustate = DUN_USBSTATE_ERROR;
   } else {
      /* send initial USB state to control thread */
      /* if usb is other than NOT_ATTACHED state, treat it as PLUG state */
      if(!memcmp(buf, DUN_USB_CONFIG_STR, strlen(DUN_USB_CONFIG_STR)))
         ustate = DUN_USBSTATE_PLUG;
      else
         ustate = DUN_USBSTATE_UNPLUG;
   }

   LOGI("The value returned from %s is %d ",__func__ , ustate);
   close(fd);
   return ustate;
}

/*===========================================================================

FUNCTION: dun_open_route_sock

DESCRIPTION: This function open route sock

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
static int dun_open_route_sock(struct dun_netlink_socks *socks)
{
   struct sockaddr_nl nlsock;

   memset(&nlsock, 0, sizeof(nlsock));
   nlsock.nl_family = AF_NETLINK;
   nlsock.nl_pid = getpid();
   nlsock.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;

   if((socks->route_sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0) {
      LOGE("Unable to create uevent socket: %s\n", strerror(errno));
      return -1;
   }

   if(bind(socks->route_sock, (struct sockaddr*)&nlsock, sizeof(nlsock)) < 0) {
      LOGE("Unable to bind route socket: %s\n", strerror(errno));
      return -1;
   }
   return 0;
}
/*===========================================================================

FUNCTION: send_data_to_dun

DESCRIPTION: This function used for passing dun initiated/end information
	     to DUN Service using FIFO

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
int send_data_to_dun(int dun_fd, unsigned char buf)
{
   int ret = 0;

   ret = write(dun_fd, &buf, 1);

   return ret;
}

/*===========================================================================

FUNCTION: dun_disable_data_connection

DESCRIPTION: This function disables data connection. Enable/Disable data
             connection only required for single PDP context. For multiple
             PDP contexts data connection and DUN connection can coexist.

DEPENDENCIES
  None

RETURN VALUE
  dun_returnCodeType

SIDE EFFECTS
  None

============================================================================*/
int dun_disable_data_connection(void)
{
  int  inf_status = DUN_RMNETSTATE_ERROR;
#ifdef LE_PORT_BRIDGE_DBG
  int dun_fd;
#endif

  if (!SINGLE_PDP) {
     LOGI("For Multiple PDP, posting RMNET_DOWN event w/o writing DUN_INITIATED ");
     dun_post_event(DUN_EVENT_RMNET_DOWN);
     return 0;
  }

  inf_status = dun_interface_check_status();
  if ((DUN_RMNETSTATE_DOWN == inf_status) || (DUN_RMNETSTATE_ERROR == inf_status)) {
     LOGI("Rmnet is down, posting RMNET_DOWN event w/o writing DUN_INITIATED %d", inf_status);
     dun_post_event(DUN_EVENT_RMNET_DOWN);
     return 0;
  }

#ifdef LE_PORT_BRIDGE_DBG
  /* Create the FIFO if it does not exist */
  if ( access (DUN_FIFO_FILE, R_OK ) != 0 ) {
   if (mkfifo(DUN_FIFO_FILE,0666) != 0) {
      LOGE("Unable to create /data/dun");
      dun_post_event(DUN_EVENT_ERROR);
   }
  }

  if ( access (DUN_FIFO_FILE, W_OK ) != 0 ) {
        LOGE("Failed write access on /data/dun");
        dun_post_event(DUN_EVENT_ERROR);
        return 0;
  }

   if ((dun_fd = open(DUN_FIFO_FILE, O_WRONLY | O_NONBLOCK)) == NULL) {
     LOGE("Unable to open DUN device file");
     dun_post_event(DUN_EVENT_ERROR);
     return 0;
  }

  send_data_to_dun(dun_fd, DUN_INITIATED);
  close(dun_fd);
#else
  enableDataConnectivity(DUN_INITIATED);
#endif

  LOGI("Sent DUN_INITIATED");

  return 0;
}

/*===========================================================================

FUNCTION: dun_enable_data_connection

DESCRIPTION: This function enables data connection. Enable/Disable data
             connection only required for single PDP context. For multiple
             PDP contexts data connection and DUN connection can coexist.
DEPENDENCIES
  None

RETURN VALUE
  dun_returnCodeType

SIDE EFFECTS
  None

============================================================================*/
int dun_enable_data_connection(void)
{
#ifdef LE_PORT_BRIDGE_DBG
  FILE *dun_fd;
#endif

  if (!SINGLE_PDP) {
     LOGI("For Multiple PDP, posting RMNET_UP event w/o writing DUN_END");
     dun_post_event(DUN_EVENT_RMNET_UP);
     return 0;
  }

  if (DUN_RMNETSTATE_UP == dun_interface_check_status()) {
     LOGI("Rmnet is up, posting RMNET_UP event w/o writing DUN_END");
     dun_post_event(DUN_EVENT_RMNET_UP);
     return 0;
  }
#ifdef LE_PORT_BRIDGE_DBG
  /* Create the FIFO if it does not exist */
  if ( access (DUN_FIFO_FILE, R_OK ) != 0 ) {
     if (mkfifo(DUN_FIFO_FILE,0666) != 0) {
        LOGE("Unable to create /data/dun");
        dun_post_event(DUN_EVENT_ERROR);
     }
  }

  if ( access (DUN_FIFO_FILE, W_OK ) != 0 ) {
        LOGE("Failed write access on /data/dun");
        dun_post_event(DUN_EVENT_ERROR);
        return 0;
  }

  if( (dun_fd = fopen(DUN_FIFO_FILE,"w")) == NULL) {
     LOGE("Unable to open DUN device file");
     dun_post_event(DUN_EVENT_ERROR);
     return 0;
  }

  fputc(DUN_END, dun_fd);
  fclose(dun_fd);
#else
  enableDataConnectivity(DUN_END);
#endif
  LOGI("Sent DUN_END ");

  return 0;
}

/*===========================================================================

FUNCTION: dun_rmnet_post_msg

DESCRIPTION: This function send rmnet status message

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
int dun_rmnet_post_msg(void)
{
    int message = TRUE;

    write(dun_rmnet_pipefds[1], &message, sizeof(message));
    return 0;
}
/*===========================================================================

FUNCTION: dun_post_event

DESCRIPTION: dun post event

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
void dun_post_event(DUN_EVENT_E event)
{
   dun_event_msg_s msg;

   pthread_mutex_lock (&dun_post_event_mutex);
   msg.event = event;
   msg.unused = NULL;
   write(dun_ctrl_pipefds[1], &msg, sizeof(msg));
   pthread_mutex_unlock (&dun_post_event_mutex);

   return;
}
/*===========================================================================

FUNCTION: dun_monitor_kevents

DESCRIPTION: This function monitor usb plug/unplug kernel events and network
             status update events on netlink socket.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
void *dun_monitor_kevents(void *arg)
{
   struct dun_netlink_socks socks;
   DUN_USBMODEMSTATE_E mstate;
   DUN_USBSTATE_E ustate;
   DUN_RMNETSTATE_E rstate;
   fd_set fds;
   int max = 0;
   char *uevent_buf;
   int uevent_buf_size;
   char rmnet_buf[1024];
   int cnt;
   struct msghdr msg;
   struct iovec iov;
   (void)arg;

   uevent_buf_size = sizeof(char) * 64 * 1024;
   uevent_buf = (char *)malloc(uevent_buf_size);
   if (uevent_buf == NULL) {
      LOGE("%s thread out of memory", __func__);
      return NULL;
   }

   /* open usb event socket */
   if(dun_open_uevent_sock(&socks) < 0) {
      free(uevent_buf);
      LOGE("Error while opening the netlink kevent socket\n");
      return NULL;
   }

   /* open netlink route socket */
   if(dun_open_route_sock(&socks) < 0) {
      free(uevent_buf);
      LOGE("Error while opening the netlink route socket\n");
      return NULL;
   }

   /* post initial USB state to control thread */
   /* check modem interface status in current usb composition */
   mstate = dun_getusbmodemstate_fromsys();
   if (mstate == DUN_USBMODEMSTATE_CONFIGURED) {
      LOGI("modem is present in configuration\n");
      ustate = dun_getusbstate_fromsys();
      if(ustate == DUN_USBSTATE_ERROR) {
         free(uevent_buf);
         LOGE("ustate == DUN_USBSTATE_ERROR in %s\n", __func__);
         return NULL;
      }
      else if(ustate == DUN_USBSTATE_PLUG)
         dun_post_event(DUN_EVENT_USB_PLUG);
      else
         dun_post_event(DUN_EVENT_USB_UNPLUG);
   }
   else if (mstate == DUN_USBMODEMSTATE_NOTCONFIGURED) {
      LOGI("modem is not present in configuration\n");
      dun_post_event(DUN_EVENT_USB_UNPLUG);
   }
   else {
      free(uevent_buf);
      LOGE("Error reading modem configuration \n");
      return NULL;
   }

   /* post initial netowrk state to control thread */
   rstate =  dun_interface_check_status();

   /* When phone is rebooted the rmnet status can return ERROR.
    * We should proceed further and  register for netlink route socket */
   if(rstate == DUN_RMNETSTATE_ERROR) {
      LOGI("rstate == DUN_RMNETSTATE_ERROR in %s\n", __func__);
   } else if(rstate == DUN_RMNETSTATE_DOWN)
      dun_post_event(DUN_EVENT_RMNET_DOWN);
   else
      dun_post_event(DUN_EVENT_RMNET_UP);

   while(1) {

      FD_ZERO(&fds);

      FD_SET(socks.uevent_sock, &fds);
      if (socks.uevent_sock > max)
         max = socks.uevent_sock;

      FD_SET(socks.route_sock, &fds);
      if (socks.route_sock > max)
         max = socks.route_sock;

      if (select(max + 1, &fds, NULL, NULL, NULL) < 0) {
         free(uevent_buf);
         LOGE("select() failed (%s)", strerror(errno));
         return NULL;
      }

      if (FD_ISSET(socks.uevent_sock, &fds)) {
         memset(uevent_buf, 0, uevent_buf_size);
         if ((cnt = recv(socks.uevent_sock, uevent_buf,
                         uevent_buf_size, 0)) < 0) {
            free(uevent_buf);
            LOGE("Error receiving uevent (%s)\n", strerror(errno));
            return NULL;
         }

         mstate = dun_getusbmodemstate_fromsys();
         if (mstate == DUN_USBMODEMSTATE_CONFIGURED) {
            ustate = dun_parse_usbevent(uevent_buf, cnt);
            if(ustate == DUN_USBSTATE_PLUG) {
               dun_post_event(DUN_EVENT_USB_PLUG);
            }
            else if(ustate == DUN_USBSTATE_UNPLUG) {
               dun_post_event(DUN_EVENT_USB_UNPLUG);
            }
         } else if(mstate == DUN_USBMODEMSTATE_NOTCONFIGURED) {
               dun_post_event(DUN_EVENT_USB_UNPLUG);
         } else {
             free(uevent_buf);
             return NULL;
         }
      }

      if(FD_ISSET(socks.route_sock, &fds)) {
         LOGI("process rmnet event\n");
         memset((void *)rmnet_buf, 0, 1024);
         memset((void *)&msg, 0, sizeof(msg));
         memset((void *)&iov, 0, sizeof(iov));
         iov.iov_base = (void *)rmnet_buf;
         iov.iov_len = sizeof(rmnet_buf);

         msg.msg_name = NULL;
         msg.msg_namelen = 0;
         msg.msg_iov = &iov;
         msg.msg_iovlen = 1;

         cnt = recvmsg(socks.route_sock, &msg, 0);
         if (cnt < 0 ) {
            free(uevent_buf);
            LOGE("recvmsg failed in netlink: %s\n", strerror(errno));
            return NULL;
         }

         rstate =  dun_interface_check_status();
         if(rstate == DUN_RMNETSTATE_UP)
            dun_post_event(DUN_EVENT_RMNET_UP);
         else if(rstate == DUN_RMNETSTATE_DOWN)
            dun_post_event(DUN_EVENT_RMNET_DOWN);
         else if(rstate == DUN_RMNETSTATE_ERROR)
            LOGI(" rstate == DUN_RMNETSTATE_ERROR in %s\n", __func__);
        }
   }
   free(uevent_buf);
   return NULL;
}
