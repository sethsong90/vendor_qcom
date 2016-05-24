/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                 O N C R P C _ X P R T R T R _ L I N U X . C

GENERAL DESCRIPTION

  This is the ONCRPC transport OS abstration for Linux

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2006-2011, 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_rtr_linux.c#11$

when        who    what, where, why
--------    ---    ----------------------------------------------------------
28/07/11    as     Enable userspace logging
04/19/11    eh     Updated subsystem restart support
10/09/09    rr     Use pipe to wakeup poll and select
04/06/09    rr     Add SIGABRT handling in selet for stopping tasks.
02/23/09    rr     Add read_remove_handle to support closing of handles.
                   Add mutex to protect pool_nfds
07/17/08    rr     Add Android support
07/09/08    rr     Fix read handle for multiple callbacks CBSP20
12/13/07     al    Loop on read/write/ioctl if the system calls are
                   interrupted
12/05/07     rr    Loop on read if read retuns zero, upper layer cannot
                   handle a failed read, so we must retry.
05/02/07     ih    Initial version

===========================================================================*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#ifndef FEATURE_ANDROID
#include <stropts.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#ifndef FEATURE_USE_LINUX_POLL_H
#include <poll.h>
#else
#include <linux/poll.h>
#endif
#include <pthread.h>
#include "customer.h"
#include "target.h"
#include "comdef.h"
#include "oncrpc_rtr_os.h"
#include "rpc_router_types.h"
#include "rpc_router_os_lnx.h"
#include "oncrpc.h"
#include "oncrpc_xdr_types.h"
#include "oncrpci.h"

/* Define timout as .1 sec */
#define ONCRPC_RTR_LINUX_RESELECT_TIMEOUT_US (100000)
#define ONCRPC_RTR_LINUX_RESELECT_TIMEOUT_NS (ONCRPC_RTR_LINUX_RESELECT_TIMEOUT_US*1000)
//#define ONCRPC_RTR_LINUX_DEBUG
//#define ONCRPC_RTR_LINUX_DEBUG_LOW
//#define ONCRPC_RTR_LINUX_DEBUG_PRINT

#if defined ONCRPC_RTR_LINUX_DEBUG_PRINT
#define DEBUG_PRINT(p...) do { \
	printf(p); \
} while (0)
#else
#define DEBUG_PRINT(p...) do { } while (0)
#endif
/*-----------------------------------------------------------------------
  Local Definitions
 -----------------------------------------------------------------------*/

/* The size of bits in fd_set uses the default FD_SETSIZE which is 1024 */
static volatile fd_set rx_active_fdset;


static pthread_mutex_t restart_mutex;
static pthread_mutex_t nfds_mutex;
static int restart_flag;
/* Set of file descriptors pending restart */
static fd_set fd_restarted_fdset;
static pthread_cond_t restart_flag_cv;

#define RESTART_STATE_NORMAL  0
#define RESTART_STATE_INITIATED 1

#define ONCRPC_NUM_POLL_FDS  (64)
struct pollfd pfds[ONCRPC_NUM_POLL_FDS];
struct pollfd pfds_read[ONCRPC_NUM_POLL_FDS];

typedef struct
{
  int nfds;
  fd_set readfds;
  struct timeval select_timeout;
}select_type;

#define PFD_READ  (0)
#define PFD_WRITE (1)
static int pipe_wakeup_read_pfd[2];
static int pipe_wakeup_restart_pfd[2];

#define CMD_ABORT     'k'
#define CMD_CONTINUE  ' '
#ifdef FEATURE_MODEM_LOAD
static int router_file_handle = -1;
#endif
static void xprtrtr_os_process_restart_up(uint32 handle);

/*-----------------------------------------------------------------------
  Extern references
-----------------------------------------------------------------------*/
extern void oncrpc_restart_handler(uint32 handle);
extern void oncrpc_restart_up_handler(uint32 handle);
extern int poll(struct pollfd *, unsigned long int, int);
/*===========================================================================
FUNCTION      pipe_wakeup

DESCRIPTION   Write CMD_CONTINUE to both pipes for select and poll,
              to break them from infinite wait.

ARGUMENTS     void

RETURN VALUE  N/A

SIDE EFFECTS  Will cause poll and select in this file to return.
===========================================================================*/
static void pipe_wakeup(void)
{
  char data = CMD_CONTINUE;
  write(pipe_wakeup_read_pfd[PFD_WRITE],&data,1);
  write(pipe_wakeup_restart_pfd[PFD_WRITE],&data,1);
}

/*===========================================================================
FUNCTION      xprtrtr_os_read_abort

DESCRIPTION   Write CMD_ABORT to read pipe for select and poll,
              to break them from infinite wait and return.

ARGUMENTS     void

RETURN VALUE  N/A

SIDE EFFECTS  Will cause poll and select in this file to return.
===========================================================================*/
void xprtrtr_os_read_abort(void)
{
  char data = CMD_ABORT;
  write(pipe_wakeup_read_pfd[PFD_WRITE],&data,1);
}

/*===========================================================================
FUNCTION      xprtrtr_os_restart_abort

DESCRIPTION   Write CMD_ABORT to restart pipe for select and poll,
              to break them from infinite wait and return.

ARGUMENTS     void

RETURN VALUE  N/A

SIDE EFFECTS  Will cause poll and select in this file to return.
===========================================================================*/
void xprtrtr_os_restart_abort(void)
{
  char data = CMD_ABORT;
  write(pipe_wakeup_restart_pfd[PFD_WRITE],&data,1);
}

/*===========================================================================
FUNCTION      pipe_wakeup_read_clear

DESCRIPTION   Clear the pipe by reading a command (1 byte) from pipe.
              The file should be set to non-blocking to avoid hanging on
              this call.

ARGUMENTS     void

RETURN VALUE  command read

SIDE EFFECTS  None
===========================================================================*/
static uint8 pipe_wakeup_read_clear(void)
{
  uint8 data;
  read(pipe_wakeup_read_pfd[PFD_READ], &data, 1);
  return data;
}


/*===========================================================================
FUNCTION      pipe_wakeup_restart_clear

DESCRIPTION   Clear the pipe by reading a command (1 byte) from pipe.
              The file should be set to non-blocking to avoid hanging on
              this call.

ARGUMENTS     void

RETURN VALUE  command read

SIDE EFFECTS  None
===========================================================================*/
static uint8 pipe_wakeup_restart_clear(void)
{
  uint8 data;
  read(pipe_wakeup_restart_pfd[PFD_READ], &data, 1);
  return data;
}

/*===========================================================================
FUNCTION      read_add_handle

DESCRIPTION   Add a handle to read for select and poll

ARGUMENTS     handle

RETURN VALUE  N/A

SIDE EFFECTS  Adds to the select list
===========================================================================*/
static void read_add_handle(uint32 handle)
{
  int i;
  int available = -1;

  pthread_mutex_lock(&nfds_mutex);

  /* search for existing file handle */
  for (i=0; i < ONCRPC_NUM_POLL_FDS; i++)
  {
     if (pfds[i].fd == -1 && available == -1)
       available = i;

     if (pfds[i].fd == (int)handle)
       goto exit;
  }

  if (available != -1)
  {
     FD_SET(handle,&rx_active_fdset);

     pfds[available].events = POLLRDHUP;
     pfds[available].fd = handle;

     pfds_read[available].events = POLLIN;
     pfds_read[available].fd = handle;
  }
  else
  {
     printf("%s: Out of file handles\n", __func__);
  }

  pipe_wakeup();

exit:
  pthread_mutex_unlock(&nfds_mutex);
}


/*===========================================================================
FUNCTION      read_remove_handle

DESCRIPTION   Remove a handle to read for select and poll

ARGUMENTS     handle

RETURN VALUE  N/A

SIDE EFFECTS  Adds to the select list
===========================================================================*/
static void read_remove_handle(int32 handle)
{
  uint32 i;
  pthread_mutex_lock(&nfds_mutex);
  FD_CLR(handle,&rx_active_fdset);

  for(i=0; i < ONCRPC_NUM_POLL_FDS; i++)
  {
     if(handle == pfds[i].fd)
     {
        pfds[i].events = 0;
        pfds[i].fd = -1;

        pfds_read[i].events = 0;
        pfds_read[i].fd = -1;
        break;
     }
  }
  if (i >= ONCRPC_NUM_POLL_FDS)
      printf("%s: Unable to find handle\n", __func__);

  pthread_mutex_unlock(&nfds_mutex);
  pipe_wakeup();
}

/*===========================================================================
FUNCTION      set_restart_state_on

DESCRIPTION   Set retart state for specified file handle

ARGUMENTS     handle

RETURN VALUE  N/A

SIDE EFFECTS  Handle will need IOCTL to be re-enabled in kernel.
===========================================================================*/
static void set_restart_state_on(unsigned int handle)
{
   pthread_mutex_lock(&restart_mutex);
   /* Process cleanup on FH and mark it set
    * we want to process restart only once per FH
   * per restart sequence */
   if( ! FD_ISSET(handle,&fd_restarted_fdset ) )
   {
      //printf("Process restart for handle %d \n",(unsigned int) handle);
      xprtrtr_os_process_restart(handle);
      FD_SET(handle,&fd_restarted_fdset );
      FD_CLR(handle,&rx_active_fdset);
   }
   restart_flag |= RESTART_STATE_INITIATED;
   pthread_cond_signal(&restart_flag_cv);
   pthread_mutex_unlock(&restart_mutex);
}

/*===========================================================================
FUNCTION      set_restart_state_ready

DESCRIPTION   Global restart state, once all FH are ready for restarting
              set global restart state off, and re-enable to active
              set all FH that were in the fd_restarted_fdset.

ARGUMENTS     N/A

RETURN VALUE  N/A

SIDE EFFECTS  Handle will need IOCTL to be re-enabled in kernel.
===========================================================================*/
static void set_restart_state_ready(void )
{
   uint32 i;
   pthread_mutex_lock(&restart_mutex);
   restart_flag = RESTART_STATE_NORMAL;
   for(i=0;i<FD_SETSIZE;i++)
   {
     if(FD_ISSET(i,&fd_restarted_fdset ) )
     {
       FD_SET(i,&rx_active_fdset);
     }
   }

   for (i=PFD_READ + 1; i < ONCRPC_NUM_POLL_FDS; i++)
   {
     if (pfds[i].fd != -1)
       xprtrtr_os_process_restart_up(pfds[i].fd);
   }

   FD_ZERO(&fd_restarted_fdset);
   pthread_cond_signal(&restart_flag_cv);
   pthread_mutex_unlock(&restart_mutex);
}


/*===========================================================================
FUNCTION      xprtrtr_os_process_restarting_handles

DESCRIPTION   Process Restart Handles, use infinite timeout.
              File handles will return POLLERR when the fh is closed by router
              due to remote processor restart.
              When fh are added, pipe wakeup will break poll so it can get restarted
              with the new set of file handles.

ARGUMENTS     N/A

RETURN VALUE  N/A

SIDE EFFECTS  None

===========================================================================*/
void xprtrtr_os_process_restarting_handles(void)
{
  int result;
  unsigned int i;
  int files_in_reset = 0;

  /* wait for restart state changes */
  result = poll(pfds, ONCRPC_NUM_POLL_FDS, -1);
  if (result < 0)
  {
    perror("Polling error");
    return;
  }

  if (pfds[PFD_READ].revents)
    pipe_wakeup_restart_clear();

  for (i=PFD_READ + 1; i < ONCRPC_NUM_POLL_FDS; i++)
  {
    if ((pfds[i].fd != -1) && (pfds[i].revents & POLLRDHUP))
    {
      /* file entered reset state */
      pfds[i].events = POLLOUT;
      set_restart_state_on(pfds[i].fd);
      files_in_reset = 1;
    }
    else if ((pfds[i].fd != -1) && (pfds[i].revents & POLLOUT))
    {
      /* file exited reset state */
      pfds[i].events = POLLRDHUP;
    }
  }

  /* clear restart state if all handles are out of reset */
  if (!files_in_reset && (restart_flag != RESTART_STATE_NORMAL))
    set_restart_state_ready();
}


/*===========================================================================
FUNCTION     xprtrtr_os_process_restart

DESCRIPTION   Handle Restart for file handles that are active and
              pending restart.

ARGUMENTS     handle

RETURN VALUE  N/A

SIDE EFFECTS  Files will restart reading
===========================================================================*/
void xprtrtr_os_process_restart(uint32 handle )
{
  oncrpc_addr_type addr;
  uint32 prog;
  uint32 version;

  addr = (oncrpc_addr_type) handle;
  if( oncrpc_lookup_get_prog_vers(addr,&prog, &version ))
  {
    printf(" Handle restart cleanup for , fh:%d, prog:0x%08x, vers:0x%08x\n",(unsigned int)handle,(unsigned int)prog,(unsigned int)version);
    oncrpc_restart_handler(addr);
    ioctl(handle,RPC_ROUTER_IOCTL_CLEAR_NETRESET);
  }

}

/*===========================================================================
FUNCTION     xprtrtr_os_process_restart_up

DESCRIPTION   Handle server-up notification for file handles that are active
              and are in the restart state.

ARGUMENTS     handle

RETURN VALUE  N/A

SIDE EFFECTS  Files will restart reading
===========================================================================*/
void xprtrtr_os_process_restart_up(uint32 handle)
{
  oncrpc_addr_type addr;
  uint32 prog;
  uint32 version;

  addr = (oncrpc_addr_type) handle;
  if( oncrpc_lookup_get_prog_vers(addr, &prog, &version ))
  {
    printf(" Handle restart-up for fh:%d, prog:0x%08x, vers:0x%08x\n",
      (unsigned int)handle, (unsigned int)prog, (unsigned int)version);
    oncrpc_restart_up_handler(addr);
  }
}


/*===========================================================================
FUNCTION      xprtrtr_os_init

DESCRIPTION   Initialize the os rtr transport

ARGUMENTS     N/A

RETURN VALUE  N/A

SIDE EFFECTS  None
===========================================================================*/
void xprtrtr_os_init()
{
   int i;
   FD_ZERO((fd_set*)&rx_active_fdset);
   FD_ZERO(&fd_restarted_fdset);
   pthread_mutex_init(&restart_mutex, NULL);
   pthread_mutex_init(&nfds_mutex, NULL);
   restart_flag= RESTART_STATE_NORMAL;
   pthread_cond_init(&restart_flag_cv, NULL);

   if (pipe(pipe_wakeup_read_pfd) == -1 )
   {
     perror("pipe open failed read_pfd");
     exit(EXIT_FAILURE);
   }
   if (pipe(pipe_wakeup_restart_pfd) == -1 )
   {
     perror("pipe open failed restart_pfd");
     exit(EXIT_FAILURE);
   }
   /* The add handle for poll only polls for errors */
   /* For the pipe, we need to poll on read */
   pthread_mutex_lock(&nfds_mutex);
   FD_SET(pipe_wakeup_read_pfd[PFD_READ],&rx_active_fdset);

   /* initialize restart file descriptors */
   for (i=0; i < ONCRPC_NUM_POLL_FDS; i++)
   {
      pfds[i].events = 0;
      pfds[i].fd = -1;

      pfds_read[i].events = 0;
      pfds_read[i].fd = -1;
   }

   /* add pipe to polling events for self-pipe unblocking method */
   pfds[PFD_READ].events = POLLIN;
   pfds[PFD_READ].fd = pipe_wakeup_restart_pfd[PFD_READ];

   pfds_read[PFD_READ].events = POLLIN;
   pfds_read[PFD_READ].fd = pipe_wakeup_read_pfd[PFD_READ];

   fcntl(pipe_wakeup_read_pfd[PFD_READ], F_SETFL, O_NONBLOCK);  // set to non-blocking
   fcntl(pipe_wakeup_restart_pfd[PFD_READ], F_SETFL, O_NONBLOCK);  // set to non-blocking
   pthread_mutex_unlock(&nfds_mutex);

#ifdef FEATURE_MODEM_LOAD
  char name[25];

  snprintf(name, sizeof(name), "/dev/oncrpc/00000000:0");
  if(xprtrtr_os_access(name))
    snprintf(name, sizeof(name), "/dev/00000000:0");

  router_file_handle = open(name, O_RDWR | O_NONBLOCK, 0);
  if( router_file_handle < 0 )
  {
    perror("Unable to open router port");
  }
#endif
}

/*===========================================================================
FUNCTION      xprtrtr_os_open

DESCRIPTION   Open a port to the router.  The router will open a port and
              assign a handle.  This handle is to be use in all subsequent
              operations on the rpc_router for this port.

ARGUMENTS

RETURN VALUE  File descriptor

SIDE EFFECTS  None
===========================================================================*/
XPORT_HANDLE_TYPE xprtrtr_os_open(const char *name)
{
  int handle;

  handle = open(name, O_RDWR | O_NONBLOCK, 0);

  if( handle < 0 )
  {
    perror("Unable to open router port");
  }
  else
  {
    read_add_handle(handle);
  }
  DEBUG_PRINT("FILE:%s LINE:%d open:%s, handle:%d\n",__FILE__,__LINE__,name,(int)handle);
  return(XPORT_HANDLE_TYPE) handle;
}

/*===========================================================================
FUNCTION      xprtrtr_os_close

DESCRIPTION   Close an opened port to the router.

ARGUMENTS     handle - File handle

RETURN VALUE  None

SIDE EFFECTS  None
===========================================================================*/
void xprtrtr_os_close( XPORT_HANDLE_TYPE handle )
{
  read_remove_handle(handle);
  if( close((int) handle) < 0 ) perror("Close error");
}

/*===========================================================================
FUNCTION      xprtrtr_os_read

DESCRIPTION   Read data from RPC router

ARGUMENTS     handle - client handle returned from xprtrtr_os_open
              buf - user-space double pointer. This function will allocate
              space, copy data and then copy the starting address in this
              pointer variable.
              size - maximum size of data to read

RETURN VALUE  number of bytes read, -1 on error
              handle

SIDE EFFECTS  read_handle is determined by select based on current set
              of valid read_handles ready for read, and by the rr_read_handle
			  variable which tracks round-robin fair allocation of read handles.
			  read handle is returned in the pointer handle.
===========================================================================*/
int xprtrtr_os_read
  (
  XPORT_HANDLE_TYPE *handle,
  char **buf,
  uint32 size
  )
{
  int bRead;
  int poll_retval;
#ifdef ONCRPC_RTR_LINUX_DEBUG
  int i;
#endif
  int read_handle = -1;
  int pkt_size;
  char * temp_buf;

  static int rr_next_read_index = PFD_READ + 1;

  /* Select and Read do-while loop */
  do
  {
     /* Select do-while loop */
     do
     {
       /* poll for available input */
       poll_retval = poll(pfds_read, ONCRPC_NUM_POLL_FDS, -1);

       if(pfds_read[PFD_READ].revents & POLLIN)
       {
         if(pipe_wakeup_read_clear() == CMD_ABORT)
         {
           DEBUG_PRINT("read abort received %s %d \n",__FUNCTION__,__LINE__);
           return -SIGABRT;
         }

         /* Reduce count for pipe FH */
         poll_retval--;
       }
     }while (poll_retval < 1 );

     /* find next file handle to service using round-robin */
     do
     {
        if (pfds_read[rr_next_read_index].revents & POLLIN)
          read_handle = pfds_read[rr_next_read_index].fd;

        if (++rr_next_read_index >= ONCRPC_NUM_POLL_FDS)
          rr_next_read_index = PFD_READ + 1;

     } while (read_handle == -1);

     bRead = -1;
     if((*buf) == NULL)
     {
        pkt_size = ioctl(read_handle, RPC_ROUTER_IOCTL_GET_CURR_PKT_SIZE);
        if(pkt_size < 0)
        {
           perror("Get Head Packet Size Error");
           return pkt_size;
        }
        else if(pkt_size == 0)
           continue;

        temp_buf = (char *)malloc(pkt_size);
        if(temp_buf == NULL)
           return -ENOMEM;
        size = (uint32)pkt_size;
     }
     else
        temp_buf = *buf;

     bRead = read( read_handle, (void *)temp_buf, size);

     /* Handle all the supported error cases from Read */
     if(bRead < 0)
     {
        if(*buf == NULL)
           free(temp_buf);
        ONCRPC_SYSTEM_LOGE("%s: Read error number: %d\n", __func__, errno);
        switch(errno)
        {
           case ENETRESET:
              set_restart_state_on(read_handle);
              printf("Setting handle to pending restart state: handle %d \n",(unsigned int)read_handle);
              goto read_bail;
              break;
           case EAGAIN:
              /* Go through the entire select / read process again.*/
              continue;
           default:
              perror("ReadFile Error");
              break;
        }
     }
     else
     {
        if(*buf == NULL)
           *buf = temp_buf;
     }

   #ifdef ONCRPC_RTR_LINUX_DEBUG
     printf("xprtrtr_os_read handle 0x%08x size %d data:\n",(unsigned int)read_handle,(unsigned int) bRead);
     for( i=0;i<bRead;i++ )
     {
       printf("%02x",temp_buf[i]);
     }
     printf("\n");
   #endif

  }while(bRead < 0);

  read_bail:
     *handle = read_handle;

  return bRead;
}

/*===========================================================================
FUNCTION      xprtrtr_os_write

DESCRIPTION   Write data to RPC router

ARGUMENTS     handle - client handle returned from xprtrtr_os_open
              buf - user-space pointer to copy data
              size - maximum size of data to read

RETURN VALUE  number of bytes read, -1 on error

SIDE EFFECTS
===========================================================================*/
int xprtrtr_os_write
   (
   XPORT_HANDLE_TYPE handle,
   const char *buf,
   uint32 size
   )
{
  int bWritten=0;
  uint32 count=0;

#ifdef ONCRPC_RTR_LINUX_DEBUG
   {
      int i;
      printf("xprtrtr_os_write handle 0x%08x size %d data:\n",(unsigned int)handle,(unsigned int) size);
      for( i=0;i<size;i++ )
      {
         printf("%02x",buf[i]);
      }
      printf("\n");
   }
#endif

#ifdef ONCRPC_RTR_LINUX_DEBUG_LOW
   printf("FILE:%s LINE:%d write size:%d, handle:%d\n",__FILE__,__LINE__,(int)size,(int)handle);
#endif

  if( handle < 0 ) {
      return -1;
   }
  do {
      bWritten = write((int) handle, (void *)buf, size);

      if( ( bWritten != ((int)size)) )
      {
         ONCRPC_SYSTEM_LOGE("%s: Write error number: %d\n", __func__, errno);
         if (errno == ENETRESET)
         {
            printf("%s: aborting write due to ENETRESET\n", __func__);
            set_restart_state_on(handle);
         }

         pthread_mutex_lock(&restart_mutex);
         if (restart_flag != RESTART_STATE_NORMAL)
         {
            /* block until client has been notified of the reset */
            while( restart_flag != RESTART_STATE_NORMAL )
              pthread_cond_wait(&restart_flag_cv, &restart_mutex);

            pthread_mutex_unlock(&restart_mutex);
            return -1;
         }
         pthread_mutex_unlock(&restart_mutex);

         printf("WriteFile Error, handle=%d, error=%s (%d)\n", (unsigned int) handle, strerror( errno ),errno );
         printf("xprtrtr_os_write handle=%d size=%d data=%p\n",(unsigned int)handle,(unsigned int) size, buf);
         count++;
      }
   }  while( ( ( (bWritten != ((int)size)) && (count < 2) ) || (bWritten == -1 && ( errno == EINTR) ) ) );
   if( (bWritten != ((int)size)) && (errno == ENETRESET) )
   {
      sleep(1);
      printf("Write to handle failed, retuned ENETRESET waiting 1 sec for restart....\n");
   }

   if( bWritten != ((int)size) )
   {
      perror("WriteFile Error");
   }

   return bWritten;
}

/*===========================================================================
FUNCTION      xprtrtr_os_control

DESCRIPTION   Send IOCTL to RPC router

ARGUMENTS     handle - client handle returned from xprtrtr_os_open
              cmd - IOCTL command
              arg - IOCTL argument

RETURN VALUE  0 on success, -1 on error

SIDE EFFECTS
===========================================================================*/
int xprtrtr_os_control
  (
  XPORT_HANDLE_TYPE handle,
  const uint32 cmd,
  void *arg
  )
{
  int err;

  do
  {
    err = ioctl((int) handle, cmd, arg);
  }
  while( err == -1 && errno == EINTR );

  if( err < 0 )
  {
    perror("DeviceIoControl Error");
    return -1;
  }

  return 0;
}


/*===========================================================================
FUNCTION      xprtrtr_os_access

DESCRIPTION   Determine if file exists

ARGUMENTS     filename - filename

RETURN VALUE  0 on success, -1 on error

SIDE EFFECTS
===========================================================================*/
int xprtrtr_os_access( const char *filename )
{
  return access(filename,F_OK);
}

/*===========================================================================
FUNCTION      xprtrtr_os_deinit

DESCRIPTION   Deinitialize the os router transport.

ARGUMENTS     None

RETURN VALUE  None

SIDE EFFECTS
===========================================================================*/
void xprtrtr_os_deinit( void )
{
  close(pipe_wakeup_read_pfd[0]);
  close(pipe_wakeup_read_pfd[1]);
  close(pipe_wakeup_restart_pfd[0]);
  close(pipe_wakeup_restart_pfd[1]);
#ifdef FEATURE_MODEM_LOAD
  if (router_file_handle < 0)
    return;

  if( close(router_file_handle) < 0 )
  {
    perror("Unable to close router port");
  }
  router_file_handle = -1;
#endif
}
