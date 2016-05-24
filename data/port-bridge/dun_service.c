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

/*===========================================================================

FUNCTION   dun_parse_atcmd

DESCRIPTION This function does parsing atcmd

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
static DUN_ATCMD_E dun_parse_atcmd(const char *buf)
{
  if(
//#ifdef WCDMA
        (strncasecmp(buf,"ATDT*98", 7) == 0)
     || (strncasecmp(buf,"ATDT*99", 7) == 0)
     || (strncasecmp(buf,"ATD*98", 6) == 0)
     || (strncasecmp(buf,"ATD*99", 6) == 0)
//#endif
//#ifdef CDMA
     || (strncasecmp(buf,"ATDT#777", 8) == 0)
     || (strncasecmp(buf,"ATD#777", 7) == 0)
//#endif
     )  {
     LOGI("dun_parse_atcmd buf %s\n", buf);
     return DUN_ATCMD_START;
  } else
     return DUN_ATCMD_INVALID;
}
/*===========================================================================

FUNCTION   dun_smd_ext_ctrl

DESCRIPTION: This function does smd to external port control
Read the smd port status bits and echo to external port
Convert DSR to DTR if it is set on SMD port

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
static int dun_smd_ext_ctrl(int init, int smd_old_status,
                            dun_portparams_s *pportparams)
{
    int smd_new_status = 0;
    int cmd_bits;

    if( ioctl (pportparams->smdport_fd, TIOCMGET, &smd_new_status) <0 ) {
       LOGE("TIOCMGET for smd port failed - %s \n", strerror(errno));
       return -1;
    }

   /* Allow only MSR bits without CTS */
    smd_new_status &= TIOCM_RI | TIOCM_CD | TIOCM_DSR;

    /* Now set corresponding bits in physical interface if needed */
    if (init || (smd_new_status != smd_old_status))
    {
       /* Turn on bits that need turning on */
       cmd_bits = smd_new_status & (smd_new_status ^ smd_old_status);
       LOGI("smdtoextctrl: ext port bits %d, smdoldstatus %d, init = %d\n",
                cmd_bits, smd_old_status, init);

       if (cmd_bits) {
          LOGI("portbridge: Turning ON external port bits %x\n", cmd_bits);
          ioctl (pportparams->extport_fd, TIOCMBIS, &cmd_bits);
       }

       /* Turn off bits that need turning off */
       cmd_bits = ~smd_new_status & (smd_new_status ^ smd_old_status);
       if (cmd_bits) {
          LOGI("portbridge: Turning OFF external port bits %x\n", cmd_bits);
          ioctl (pportparams->extport_fd, TIOCMBIC, &cmd_bits);
       }
    }

    return smd_new_status;
}
/*===========================================================================

FUNCTION   dun_ext_smd_ctrl

DESCRIPTION This function does external to smd port control
First, read the ext port status bits and echo to smd port
Convert DSR to DTR if it is set on external port

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
static int dun_ext_smd_ctrl(int init, int prev_modem_bits,
                            dun_portparams_s *pportparams)
{
   int modem_bits = 0;
   int smd_bits = 0;

   if (ioctl (pportparams->extport_fd, TIOCMGET, &modem_bits) < 0) {
      LOGE("TIOCMGET for external port failed - %s \n", strerror(errno));
      return -1;
   }


   /* Allow only DTR bit */
   modem_bits &= TIOCM_DTR;

   /*
    * Set corresponding bits in physical interface for the first time or
    * modem bits are set
    */
   if (init || (modem_bits != prev_modem_bits))
   {
      LOGI("modem_bits%d, prev_mdm_bits %d, init = %d,\n",
               modem_bits, prev_modem_bits, init);
      smd_bits = modem_bits & (modem_bits ^ prev_modem_bits);
      if(smd_bits) {
         if(ioctl(pportparams->smdport_fd, TIOCMBIS, &smd_bits) < 0) {
            LOGE("Error while setting SMD port bits : %s\n",
                     strerror(errno));
            return -1;
         }
      }
      smd_bits = ~modem_bits & (modem_bits ^ prev_modem_bits);
      if(smd_bits ) {
         if(ioctl(pportparams->smdport_fd, TIOCMBIC, &smd_bits) < 0) {
            LOGE("Error while clearing SMD port bits : %s\n",
                                                        strerror(errno));
            return -1;
         }
      }
      if(!(init || modem_bits))
         dun_post_event(DUN_EVENT_STOP);
   }
   return modem_bits;
}
/*===========================================================================

FUNCTION   dun_port_thread_exit_handler

DESCRIPTION This function does port monitor thread exit handling

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/

static void dun_port_thread_exit_handler(int sig)
{
  (void)sig;
   pthread_exit(0);
}
/*===========================================================================

FUNCTION   dun_port_monitor

DESCRIPTION This function does port monitor for smd port and external port

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
static void* dun_monitor_ports(void *arg)
{
   int prev_ext_bits =0;
   int prev_smd_bits =0;
   int init;
   struct sigaction actions;

   dun_portparams_s *pportparams = (dun_portparams_s *)arg;

   memset(&actions, 0, sizeof(actions));
   sigemptyset(&actions.sa_mask);
   actions.sa_flags = 0;
   actions.sa_handler = dun_port_thread_exit_handler;

   if (sigaction(SIGUSR1,&actions,NULL) < 0) {
    LOGE("Error in sigaction in %s:  %s\n", __func__, strerror(errno));
   }

   /* first time flag set*/
   init=1;

   while(1) {

      usleep(200*1000);
      prev_ext_bits = dun_ext_smd_ctrl(init, prev_ext_bits, pportparams);
      prev_smd_bits = dun_smd_ext_ctrl(init, prev_smd_bits, pportparams);
      if (prev_ext_bits < 0 || prev_smd_bits < 0)
      {
         LOGE("Error in %s \n", __func__);
         pthread_exit((void *)0);
         return (void *)0;
      }
      init=0;
   }

   return NULL;
}

/*===========================================================================

FUNCTION   dun_data_transfer_to_ext_port

DESCRIPTION This function transfers data from smd port to external port

DEPENDENCIES
  None

RETURN VALUE
  dun_return_code_type

SIDE EFFECTS
  None

============================================================================*/
static int dun_data_transfer_to_ext_port(dun_portparams_s *pportparams)
{
   int num_read;
   unsigned char xfer_buf[DUN_MAXBUFSIZE];

   while (1) {
      num_read = read(pportparams->smdport_fd, (void *) xfer_buf,
                      (size_t)DUN_MAXBUFSIZE);
      if (num_read < 0 || num_read > DUN_MAXBUFSIZE) {
         LOGE("Read from SMD port failed: %s\n", strerror(errno));
         return -1;
      }

      if  (num_read > 0) {
         if (write(pportparams->extport_fd, (void *) xfer_buf, num_read) < 0) {
            LOGE("Write to external port failed: %s\n", strerror(errno));
         }
      }
   }

   return 0;
}

/*===========================================================================

FUNCTION   dun_data_transfer_to_smd_port

DESCRIPTION This function transfers data from external port to smd port

DEPENDENCIES
  None

RETURN VALUE


SIDE EFFECTS
  None

============================================================================*/
static int dun_data_transfer_to_smd_port(dun_portparams_s *pportparams)
{
   int num_read;
   unsigned char xfer_buf[DUN_MAXBUFSIZE];

   while (1) {
      num_read = read (pportparams->extport_fd, (void *) xfer_buf,
                      (size_t) DUN_MAXBUFSIZE);
      if (num_read < 0 || num_read > DUN_MAXBUFSIZE) {
         LOGE("Read from ext port failed: %s\n", strerror(errno));
         return -1;
      }

      if (num_read > 0) {
         if (write (pportparams->smdport_fd, (void *) xfer_buf, num_read) < 0) {
            LOGE("Write to smd port failed: %s\n", strerror(errno));
            return -1;
         }
      }
   }
   return 0;
}

/*===========================================================================

FUNCTION   dun_port_dataxfr_up

DESCRIPTION This function reads ext port

DEPENDENCIES
  None

RETURN VALUE
  dun_returnCodeType

SIDE EFFECTS
  None

============================================================================*/
static void* dun_port_dataxfr_up(void *arg) //ext to smd
{
   DUN_ATCMD_E atcmd;
   int num_read;
   char xfer_buf[DUN_MAXBUFSIZE];
   int message;
   struct sigaction actions;
   dun_portparams_s *pportparams = (dun_portparams_s *)arg;

   memset(&actions, 0, sizeof(actions));
   sigemptyset(&actions.sa_mask);
   actions.sa_flags = 0;
   actions.sa_handler = dun_port_thread_exit_handler;

   if (sigaction(SIGUSR1,&actions,NULL) < 0) {
      LOGE("Error in sigaction in %s: %s \n", __func__, strerror(errno));
      dun_post_event(DUN_EVENT_ERROR);
      return (void *)0;
   }

   while (1) {
      /* Data transfer from External to SMD port */
         num_read = read(pportparams->extport_fd, (void *)xfer_buf,
                             DUN_MAXBUFSIZE);
         if(num_read < 0 || num_read > DUN_MAXBUFSIZE) {
            LOGE("Ext port read failed: errno %s", strerror(errno));
            dun_post_event(DUN_EVENT_ERROR);
            return (void *)0;
         } else if (num_read != 0) {
            //LOGE("DUN at command to parse %s\n", xfer_buf);
            atcmd = dun_parse_atcmd(xfer_buf);

            if(atcmd == DUN_ATCMD_START) {

              dun_post_event(DUN_EVENT_START);

              /*Read blocked on pipe for rmnet status message from main thread*/
              read (dun_rmnet_pipefds[0], &message, sizeof(message));
              if (message)
                 LOGI("Received rmnet down message in %s\n", __func__);

              if(write(pportparams->smdport_fd, (void *)xfer_buf, num_read)<0){
                 LOGE("Write AT command to smd port failed in %s: %s\n",
                          __func__, strerror(errno));
                 dun_post_event(DUN_EVENT_ERROR);
                 return (void *)0;
              }

              if (dun_data_transfer_to_smd_port(pportparams) <0) {
                 dun_post_event(DUN_EVENT_ERROR);
                 return (void*)-1;
              }
            } else  {
               /* All other AT commands write it to SMD port as it is */
              if (write (pportparams->smdport_fd, (void *)xfer_buf,
                         num_read) < 0) {
                 LOGE("Write to smd port failed in %s: %s\n",
                          __func__, strerror(errno));
                 dun_post_event(DUN_EVENT_ERROR);
                 return (void *)0;
              }
            }
      }
   } /* while */
}

/*===========================================================================

FUNCTION   dun_port_dataxfr_dn

DESCRIPTION This function reads smd port

DEPENDENCIES
  None

RETURN VALUE
  dun_returnCodeType

SIDE EFFECTS
  None

============================================================================*/
static void* dun_port_dataxfr_dn (void *arg)
{
   struct sigaction actions;
   extern DUN_STATE_E dun_state;
   dun_portparams_s *pportparams = (dun_portparams_s *)arg;

   memset(&actions, 0, sizeof(actions));
   sigemptyset(&actions.sa_mask);
   actions.sa_flags = 0;
   actions.sa_handler = dun_port_thread_exit_handler;

   if (sigaction(SIGUSR1,&actions,NULL) < 0) {
      LOGE("Error in sigaction in %s: %s \n", __func__, strerror(errno));
      dun_post_event(DUN_EVENT_ERROR);
      return (void *)0;
   }

   while (1) {
      /* Data transfer from SMD to External port in IDLE state*/
      if ((dun_state == DUN_STATE_CONNECTED)||(dun_state == DUN_STATE_IDLE)){
         if (dun_data_transfer_to_ext_port(pportparams) < 0) {
            dun_post_event(DUN_EVENT_ERROR);
            return (void *)0;
         }
      }
   } /* while */
}
/*===========================================================================

FUNCTION   dun_reset_ports

DESCRIPTION This function reset ports

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
static int dun_reset_ports(dun_portparams_s *pportparams)
{
   int orig_ext_bits = 0x120;

   if(ioctl(pportparams->extport_fd, TIOCMBIS, &orig_ext_bits) < 0)
   LOGE("Failed to reset ext_port with error = %s", strerror(errno));

   return 0;
}
/*===========================================================================

FUNCTION   dun_init_ports

DESCRIPTION This function init ports to raw mode

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

============================================================================*/
static int dun_init_ports(dun_portparams_s *pportparms)
{
   struct termios term_params;
   struct termios orig_term_params;

 /*
   memset(pportparms->smdportfname, 0, sizeof(pportparms->smdportfname));
   memset(pportparms->smdportfname, 0, sizeof(pportparms->extportfname));

   strcpy(pportparms->smdportfname, smdportfname);
   strcpy(pportparms->extportfname, extportfname);
 */
   /* open the SMD port */
   if ((pportparms->smdport_fd = open(pportparms->smdportfname, O_RDWR)) < 0) {
      LOGE("Unable to open SMD port %s : %s\n",
                pportparms->smdportfname, strerror(errno));
      exit(0);
   }

   /* open external port */
   if ((pportparms->extport_fd = open(pportparms->extportfname, O_RDWR)) < 0){
      LOGE("Unable to open external port %s : %s\n",
                   pportparms->extportfname, strerror(errno));
      exit(0);
   }
   LOGI("Successfully opened SMD port: %x, ext port: %x\n",
                    pportparms->smdport_fd, pportparms->extport_fd);

   /* Put external port into RAW mode.  Note that this is not
    * necessary for SMD port as a raw mode is the only one available
    */
   if (tcgetattr (pportparms->extport_fd, &term_params) < 0) {
      LOGE ("tcgetattr() call fails : %s\n", strerror(errno));
      return -1;
   }

   orig_term_params = term_params;
   term_params.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
   term_params.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
   term_params.c_cflag &= ~(CSIZE | PARENB);
   term_params.c_cflag |= CS8;
   term_params.c_oflag &= ~(OPOST);
   term_params.c_cc[VMIN] = 1;
   term_params.c_cc[VTIME] = 0;

   if (tcsetattr (pportparms->extport_fd, TCSAFLUSH, &term_params) < 0) {
      LOGE ("tcsetattr() call fails : %s\n", strerror(errno));
      return -1;
   }
   LOGI("Configured external host port in RAW mode\n");
   return 0;
}
/*===========================================================================

FUNCTION   dun_start_ports_threads

DESCRIPTION This function start threads

DEPENDENCIES
  None

RETURN VALUE
  dun_returnCodeType

SIDE EFFECTS
  None

============================================================================*/
int dun_start_ports_threads(dun_portparams_s *pportparams)
{
    if( dun_init_ports(pportparams) < 0) {
       LOGE("Error while initiating ports \n");
       dun_post_event(DUN_EVENT_ERROR);
    }

    dun_reset_ports(pportparams);

    /*create threads monitors external port and data transfer */
    if(pthread_create(&(pportparams->portsmonitor_thread), NULL,
                       dun_monitor_ports, (void *)pportparams) < 0) {
       LOGE("Unable to create extportmonitor : %s\n", strerror(errno));
       dun_post_event(DUN_EVENT_ERROR);
    }

     /* create up thread for port bridge*/
    if( pthread_create(&(pportparams->portdataxfr_thread_up), NULL,
                       dun_port_dataxfr_up,  (void *)pportparams) < 0) {
       LOGE("Unable to create extportread_thread : %s\n", strerror(errno));
       dun_post_event(DUN_EVENT_ERROR);
    }

     /* create dn thread for port bridge*/
    if( pthread_create(&(pportparams->portdataxfr_thread_dn), NULL,
                       dun_port_dataxfr_dn,  (void *)pportparams) < 0) {
       LOGE("Unable to create extportread_thread : %s\n", strerror(errno));
       dun_post_event(DUN_EVENT_ERROR);
    }
    return 0;
}

/*===========================================================================

FUNCTION   dun_stop_ports_threads

DESCRIPTION This function stop threads

DEPENDENCIES
  None

RETURN VALUE
  dun_returnCodeType

SIDE EFFECTS
  None

============================================================================*/
int dun_stop_ports_threads(dun_portparams_s *pportparams)
{
    int status;

    /* Allowing portsmonitor thread to transfer CD bit from SMD to ext */
    usleep(500*1000);

    /* kill thread which monitors external port bits */
    if((status = pthread_kill(pportparams->portsmonitor_thread,
                              SIGUSR1)) != 0) {
       LOGE("Error cancelling thread %d, error = %d (%s)",
            (int)pportparams->portsmonitor_thread, status, strerror(status));
    }

    /* kill thread which reads external port */
    if((status = pthread_kill(pportparams->portdataxfr_thread_up, SIGUSR1)) != 0)
    {
       LOGE("Error cancelling thread %d, error = %d (%s)",
            (int)pportparams->portdataxfr_thread_up, status, strerror(status));
    }

    /* kill thread which reads external port */
    if((status = pthread_kill(pportparams->portdataxfr_thread_dn, SIGUSR1)) != 0)
    {
       LOGE("Error cancelling thread %d, error = %d (%s)",
            (int)pportparams->portdataxfr_thread_dn, status, strerror(status));
    }

    pthread_join(pportparams->portsmonitor_thread, NULL);
    LOGI("Joined port monitor thread \n");

    pthread_join(pportparams->portdataxfr_thread_up, NULL);
    LOGI("Joined the port read thread \n");

    pthread_join(pportparams->portdataxfr_thread_dn, NULL);
    LOGI("Joined the port read thread \n");

    close(pportparams->smdport_fd);
    close(pportparams->extport_fd);
    return 0;
}
