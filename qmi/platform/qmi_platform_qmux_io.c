/******************************************************************************
  @file    qmi_qmux_io_platform.c
  @brief   The QMI QMUX linux platform layer

  DESCRIPTION
  Linux-based QMUX layer for the QMI user space driver

  INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

  ---------------------------------------------------------------------------
  Copyright (c) 2007, 2010-2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/


/******************************************************************************
******************************************************************************/

#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <poll.h>
#include "qmi_i.h"
#include "qmi_platform_qmux_io.h"


/* Copied from SMD kernel implementation file. The IOCTL's
** will be properly exported and sanitized in the future, but this
** is a temporary work-around.
*/

#define SMD_PKT_IOCTL_MAGIC (0xC2)

#define SMD_PKT_IOCTL_BLOCKING_WRITE \
        _IOR(SMD_PKT_IOCTL_MAGIC, 0, unsigned int)

/* Defines for flags fields in linux_qmi_qmux_io_conn_info_type */
#define QMI_QMUX_IO_CONN_IS_ACTIVE        0x00000001
#define QMI_QMUX_IO_CONN_IN_MODEM_RESET   0x00000002

/* Define for an invalid file descriptor */
#define LINUX_QMI_QMUX_INVALID_FD         (-1)

/* Function pointer to be called when data is received */
static qmi_qmux_io_platform_rx_cb_ptr     linux_qmi_qmux_io_rx_cb = NULL;
static qmi_qmux_io_platform_event_cb_ptr  linux_qmi_qmux_io_event_cb = NULL;

/* QMI Proxy related definitions */
#define QMI_PROXY_QMUX_IF_CONN_SOCKET_PATH    "/dev/socket/qmux_radio/proxy_qmux_connect_socket"
#define QMI_PROXY_QMUX_IF_CLIENT_SOCKET_PATH  "/dev/socket/qmux_radio/proxy_qmux_client_socket"


/* Wakelock related */
#define QMUX_WAKELOCK_FILE              "/sys/power/wake_lock"
#define QMUX_WAKEUNLOCK_FILE            "/sys/power/wake_unlock"
#define QMUX_WAKELOCK_NAME_PREFIX       "qmuxd_port_wl_"

static int qmux_wl_fd, qmux_wul_fd;

QMI_PLATFORM_MUTEX_DATA_TYPE   qmux_wl_mutex;
QMI_PLATFORM_MUTEX_DATA_TYPE   qmux_wul_mutex;




/*****************************************************************************
** Data declarations
*****************************************************************************/
linux_qmi_qmux_io_conn_info_type linux_qmi_qmux_io_conn_info[QMI_MAX_CONNECTIONS] =
  {
    /*--- MSM Modem ---*/
    {
      SMD_DEVICE_NAME "0",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_DEVICE_NAME "1",
      1,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_DEVICE_NAME "2",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_DEVICE_NAME "3",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_DEVICE_NAME "4",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_DEVICE_NAME "5",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_DEVICE_NAME "6",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_DEVICE_NAME "7",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_DEVICE_NAME "8",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_DEVICE_NAME "9",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_DEVICE_NAME "10",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_DEVICE_NAME "11",
      0,
      -1,
      NULL,
      0,
      0
    },
    /*--- MSM Modem Reverse Rmnet ---*/
    {
      SMD_REV_DEVICE_NAME "0",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_REV_DEVICE_NAME "1",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_REV_DEVICE_NAME "2",
      1,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_REV_DEVICE_NAME "3",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_REV_DEVICE_NAME "4",
      1,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_REV_DEVICE_NAME "5",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_REV_DEVICE_NAME "6",
      1,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_REV_DEVICE_NAME "7",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_REV_DEVICE_NAME "8",
      1,
      -1,
      NULL,
      0,
      0
    },
    /*--- SDIO Modem ---*/
    {
      SDIO_DEVICE_NAME "0",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SDIO_DEVICE_NAME "1",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SDIO_DEVICE_NAME "2",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SDIO_DEVICE_NAME "3",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SDIO_DEVICE_NAME "4",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SDIO_DEVICE_NAME "5",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SDIO_DEVICE_NAME "6",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SDIO_DEVICE_NAME "7",
      0,
      -1,
      NULL,
      0,
      0
    },
    /* Forward MDM Modem */
    {
      HSIC_DEVICE_NAME "0",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      HSIC_DEVICE_NAME "1",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      HSIC_DEVICE_NAME "2",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      HSIC_DEVICE_NAME "3",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      HSIC_DEVICE_NAME "4",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      HSIC_DEVICE_NAME "5",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      HSIC_DEVICE_NAME "6",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      HSIC_DEVICE_NAME "7",
      0,
      -1,
      NULL,
      0,
      0
    },
    /*--- Reverse MDM Modem ---*/
    {
      HSIC_DEVICE_NAME "8",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      HSIC_DEVICE_NAME "9",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      HSIC_DEVICE_NAME "10",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      HSIC_DEVICE_NAME "11",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      HSIC_DEVICE_NAME "12",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      HSIC_DEVICE_NAME "13",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      HSIC_DEVICE_NAME "14",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      HSIC_DEVICE_NAME "15",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      HSIC_DEVICE_NAME "16",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_DEVICE_NAME "12",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      SMD_DEVICE_NAME "13",
      0,
      -1,
      NULL,
      0,
      0
    },
    /* SMUX port */
    {
      SMUX_DEVICE_NAME "32",
      0,
      -1,
      NULL,
      0,
      0
    },
    /* USB ports */
    {
      USB_DEVICE_NAME "0",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      USB_DEVICE_NAME "1",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      USB_DEVICE_NAME "2",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      USB_DEVICE_NAME "3",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      USB_DEVICE_NAME "4",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      USB_DEVICE_NAME "5",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      USB_DEVICE_NAME "6",
      0,
      -1,
      NULL,
      0,
      0
    },
    {
      USB_DEVICE_NAME "7",
      0,
      -1,
      NULL,
      0,
      0
    },
    /* QMI Proxy */
    {
      QMI_PROXY_QMUX_IF_CONN_SOCKET_PATH,
      0,
      -1,
      NULL,
      0,
      0
    }
  };



/*===========================================================================

  FUNCTION:   linux_qmi_qmux_io_wake_lock

  ===========================================================================*/
/*!
  @brief Grabs the wakelock to prevent system suspend

  @param[i] wl_name: Name of wakelock to grab
*/
/*=========================================================================*/
static void linux_qmi_qmux_io_wake_lock( const char *wl_name )
{
  int nbytes;

  if( qmux_wl_fd >= 0 )
  {
    QMI_PLATFORM_MUTEX_LOCK (&qmux_wl_mutex);
    nbytes = write( qmux_wl_fd, wl_name, strlen(wl_name) );
    QMI_PLATFORM_MUTEX_UNLOCK (&qmux_wl_mutex);

    if( nbytes != (int)strlen(wl_name) )
    {
      QMI_ERR_MSG_3 ("linux_qmi_qmux_io_wake_lock: Err in writing wakelock=%s, error [%d:%s]\n",
                     wl_name, errno, strerror(errno));
    }
  }
}

/*===========================================================================

  FUNCTION:   linux_qmi_qmux_io_wake_unlock

  ===========================================================================*/
/*!
  @brief Releases wakelock to allow system to suspend

  @param[i] wl_name: Name of wakelock to grab
*/
/*=========================================================================*/
static void linux_qmi_qmux_io_wake_unlock( const char *wul_name )
{
  int nbytes;

  if( qmux_wul_fd >= 0 )
  {
    QMI_PLATFORM_MUTEX_LOCK (&qmux_wul_mutex);
    nbytes = write( qmux_wul_fd, wul_name, strlen(wul_name) );
    QMI_PLATFORM_MUTEX_UNLOCK (&qmux_wul_mutex);

    if( nbytes != (int)strlen(wul_name) )
    {
      QMI_ERR_MSG_3 ("linux_qmi_qmux_io_wake_unlock: Err in writing wakelock=%s, error [%d:%s]\n",
                      wul_name, errno, strerror(errno));
    }
  }
}



/*===========================================================================
  FUNCTION  linux_qmi_qmux_io_open_conn_proxy
===========================================================================*/
/*!
@brief
  Function used to open a connection to the QMI Proxy server. This function
  must be called prior to sending any messages or receiving any indications

@return
  QMI_NO_ERR if function is successful, QMI_INTERNAL_ERR if not.

*/
/*=========================================================================*/
int linux_qmi_qmux_io_open_conn_proxy
(
  const char *sock_name
)
{
  int sock_fd = LINUX_QMI_QMUX_INVALID_FD;
  struct sockaddr_un server_addr, client_addr;
  int pid;
  int i, len, rc;
  int ret = QMI_NO_ERR;


  if (NULL == sock_name)
  {
    QMI_ERR_MSG_0("linux_qmi_qmux_io_open_conn_proxy: bad param\n");
    ret = QMI_INTERNAL_ERR;
    goto init_exit;
  }

  QMI_DEBUG_MSG_1("linux_qmi_qmux_io_open_conn_proxy: opening socket=%s\n", sock_name);

  /* Initialize the addr variables */
  memset (&server_addr,0,sizeof (struct sockaddr_un));
  memset (&client_addr,0,sizeof (struct sockaddr_un));
  pid = (int) getpid();

  /* Get the connection listener socket */
  if ((sock_fd = socket (AF_UNIX,SOCK_STREAM,0)) < 0)
  {
    QMI_ERR_MSG_2("qmi_proxy_client %x: unable to open client socket, rc = %d\n",(int) pid,sock_fd);
    ret = QMI_INTERNAL_ERR;
    goto init_exit;
  }

  client_addr.sun_family = AF_UNIX;
  snprintf (client_addr.sun_path,
            sizeof(client_addr.sun_path),
            "%s%7d", QMI_PROXY_QMUX_IF_CLIENT_SOCKET_PATH, pid);

  len = offsetof (struct sockaddr_un, sun_path) + strlen (client_addr.sun_path);

  /* Delete file in case it exists */
  unlink (client_addr.sun_path);

  /* Bind socket to address */
  if ((rc = bind (sock_fd, (struct sockaddr *)&client_addr, len)) < 0)
  {
    QMI_ERR_MSG_2("qmi_proxy_client %x: unable to bind to client socket, rc = %d\n",(int)pid,rc);
    ret = QMI_INTERNAL_ERR;
    goto init_exit;
  }

  server_addr.sun_family = AF_UNIX;
  snprintf (server_addr.sun_path,
            sizeof(server_addr.sun_path),
            "%s", sock_name);

  len = offsetof (struct sockaddr_un, sun_path) + strlen (server_addr.sun_path);

  /* Connect to the servers connection socket */
  for (i = 0; i < 60; i++)
  {
    if ((rc = connect (sock_fd, (struct sockaddr *) &server_addr, len)) < 0)
    {
      QMI_ERR_MSG_2("qmi_qmux_proxy_client %x: unable to connect to server, rc = %d\n",(int) pid,rc);
      sleep (1);
    }
    else
    {
      QMI_ERR_MSG_2("qmi_qmux_proxy_client %x: connected to server, rc = %d\n",(int) pid,rc);
      break;
    }
  }
  if (rc < 0)
  {
    QMI_ERR_MSG_1("qmi_qmux_proxy_client:  unable to connect to server after %d tries... giving up\n",i);
    ret = QMI_INTERNAL_ERR;
    goto init_exit;
  }

init_exit:
  if (QMI_NO_ERR != ret)
  {
    /* Delete file in case it exists */
    unlink (client_addr.sun_path);

    if (LINUX_QMI_QMUX_INVALID_FD != sock_fd)
    {
      close(sock_fd);
      sock_fd = LINUX_QMI_QMUX_INVALID_FD;
    }
  }

  QMI_DEBUG_MSG_1("linux_qmi_qmux_io_open_conn_proxy: returning sock_fd=%d\n", sock_fd);

  return sock_fd;
}


/*===========================================================================
  FUNCTION  linux_qmi_qmux_io_rx_msg
===========================================================================*/
/*!
@brief
  Routine for handling QMI messages received from SMD QMI control port.
  This function will be called by a thread that is spawned for each
  QMI connection to handle receive messages.

@return
  NULL... but this function should never return

@note

  Connection is assumed to be opened and valid data

  - Side Effects
    -

*/
/*=========================================================================*/
static void *
linux_qmi_qmux_io_rx_msg
(
  void *in_param
)
{
  /* Pointer to QMUX connection info */
  linux_qmi_qmux_io_conn_info_type *conn_info;
  qmi_connection_id_type        conn_id;
  char                          *wl_name;
  struct pollfd                 pollfd;
  int                           rc;

  /* Input parameter is conn_id */
  conn_id = (qmi_connection_id_type) in_param;

  /* Verify that conn_id was passed in correctly */
  if (conn_id >= QMI_MAX_CONNECTIONS || conn_id < 0)
  {
    assert (FALSE);
    return NULL;
  }


  /* No need to check validity of conn_id since it
  ** is passed in after being validated
  */
  conn_info = &linux_qmi_qmux_io_conn_info[(int) conn_id];


  /* Initialize Wakelock name for this thread */
  asprintf (&wl_name,"%s%d",QMUX_WAKELOCK_NAME_PREFIX,conn_id);

  if(NULL == wl_name)
  {
    QMI_ERR_MSG_0("linux_qmi_qmux_io_rx_msg: NULL wl_name\n");
    return NULL;
  }

  /* TMR_HACK .... for some reason, this is make sdio read work
  */
  memset (conn_info->rx_buf, 0, conn_info->rx_buf_len);

  pollfd.fd = conn_info->f_desc;;
  pollfd.events = POLLIN | POLLPRI | POLLERR;


  /* Loop forever */
  while (1)
  {
    int num_read;

    QMI_UPDATE_THREAD_STATE(conn_id, RX_THREAD_WAIT_POLL);

    /* Call poll first */
    if ((rc = poll( &pollfd, 1, -1 )) < 0)
    {
      QMI_ERR_MSG_3 ("Got error from poll() call = %d, errno=%d, %s\n",rc,errno,strerror(errno));
      sleep (1);
      continue;
    }

    linux_qmi_qmux_io_wake_lock (wl_name);

    QMI_UPDATE_THREAD_STATE(conn_id, RX_THREAD_WAIT_READ);

    /* Blocking read call */
    num_read = read (conn_info->f_desc,
                     (void *) conn_info->rx_buf,
                     (size_t) conn_info->rx_buf_len);
    if (num_read <= 0)
    {
      if (errno == ENETRESET)
      {
        if (!(conn_info->flags & QMI_QMUX_IO_CONN_IN_MODEM_RESET))
        {
          conn_info->flags |= QMI_QMUX_IO_CONN_IN_MODEM_RESET;
          QMI_ERR_MSG_1 ("QMI platform I/O... MODEM RESET detected on conn_id=%d\n",conn_id);
          linux_qmi_qmux_io_event_cb (conn_id, QMI_QMUX_IO_PORT_READ_ERR_MODEM_RESET_EVT, NULL);
        }
      }
      else
      {
        QMI_ERR_MSG_2 ("Got invalid number of bytes from read = %d, errno=%d\n",num_read,errno);
      }

      linux_qmi_qmux_io_wake_unlock (wl_name);
      sleep (1);
      memset (conn_info->rx_buf, 0, conn_info->rx_buf_len);
      continue;
    }
    else if (conn_info->flags & QMI_QMUX_IO_CONN_IN_MODEM_RESET)
    {
      conn_info->flags &= ~QMI_QMUX_IO_CONN_IN_MODEM_RESET;
      QMI_ERR_MSG_1 ("QMI platform I/O... read error cleared after MODEM RESET on conn_id=%d\n",conn_id);
      linux_qmi_qmux_io_event_cb (conn_id, QMI_QMUX_IO_PORT_READ_ERR_CLEARED_EVT, NULL);
    }

    /* Make QMUX callback with newly read data */
    if (linux_qmi_qmux_io_rx_cb != NULL)
    {
      QMI_UPDATE_THREAD_STATE(conn_id, RX_THREAD_CLIENT_TX);

      linux_qmi_qmux_io_rx_cb (conn_id, conn_info->rx_buf, num_read);
    }

    linux_qmi_qmux_io_wake_unlock (wl_name);

  } /* while (1) */

  /* This will never be hit */
  free (wl_name);

  return NULL;

} /* linux_qmi_qmux_io_rx_msg */



/*===========================================================================
  FUNCTION  linux_qmi_qmux_io_send_qmi_msg
===========================================================================*/
/*!
@brief
  Function to send a QMUX PDU on the Linux platform

@return
  0 if function is successful, negative value if not.

@note

  - Connection must have been previously opened.

  - Side Effects
    - Sends a QMUX PDU to modem processor

*/
/*=========================================================================*/
int
linux_qmi_qmux_io_send_qmi_msg
(
  qmi_connection_id_type  conn_id,
  unsigned char           *msg_ptr,
  int                     msg_len
)
{
  linux_qmi_qmux_io_conn_info_type *conn_info;
  int                           rc;

  /* Verifiy the conn_id parameter since it will be used to index
  ** into an array
  */
  if (conn_id >= QMI_MAX_CONNECTIONS || conn_id < 0)
  {
    QMI_ERR_MSG_1 ("linux_qmi_qmux_io.c: bad conn_id = %d!\n",conn_id);
    return QMI_INTERNAL_ERR;
  }

  /* Set up pointer to connection info */
  conn_info = &linux_qmi_qmux_io_conn_info[conn_id];

  if (!(conn_info->flags & QMI_QMUX_IO_CONN_IS_ACTIVE))
  {
    QMI_ERR_MSG_1 ("linux_qmi_qmux_io.c: connection not active conn_id = %d!\n",conn_id);
    return QMI_INTERNAL_ERR;
  }

  if (conn_info->flags & QMI_QMUX_IO_CONN_IN_MODEM_RESET)
  {
    QMI_ERR_MSG_1 ("linux_qmi_qmux_io.c: connection in modem reset conn_id = %d!\n",conn_id);
    return QMI_INTERNAL_ERR;
  }


  /* Send to proper SMD port... no need to have any mutex here... writing will be
  ** made serial by above layer
  */
  if ((rc = write (conn_info->f_desc,
             (void *) msg_ptr,
              msg_len)) < 0)
  {
    QMI_ERR_MSG_2 ("qmi_qmux.c: Write failed, rc = %d, errno = %d!\n",rc,errno);
    rc = QMI_INTERNAL_ERR;
  }
  else
  {
    rc = QMI_NO_ERR;
  }

  return rc;
}



/*===========================================================================
  FUNCTION  linux_qmi_qmux_io_pwr_up_init
===========================================================================*/
/*!
@brief
  Initialization function to be called once at power-up.  Must be called
  prior to calling the linux_qmi_qmux_io_open_conn()

@return
  0 if function is successful, negative value if not.

@note

  - Connection is assumed to be opened with valid data before this
  function starts to execute

  - Side Effects
    -

*/
/*=========================================================================*/
int
linux_qmi_qmux_io_pwr_up_init
(
  qmi_qmux_io_platform_rx_cb_ptr      rx_cb_ptr,
  qmi_qmux_io_platform_event_cb_ptr   event_cb_ptr
)
{
  int rc;

  if (linux_qmi_qmux_io_rx_cb == NULL)
  {
    linux_qmi_qmux_io_rx_cb = rx_cb_ptr;
    linux_qmi_qmux_io_event_cb = event_cb_ptr;
    QMI_PLATFORM_MUTEX_INIT (&qmux_wl_mutex);
    QMI_PLATFORM_MUTEX_INIT (&qmux_wul_mutex);

    if ((qmux_wl_fd = open( QMUX_WAKELOCK_FILE, O_WRONLY|O_APPEND )) < 0)
    {
      QMI_ERR_MSG_3 ("linux_qmi_qmux_io_pwr_up_init.c: Unable to open wakelock file = %s, error [%d:%s]\n",
                      QMUX_WAKELOCK_FILE, errno, strerror(errno));
      rc = QMI_INTERNAL_ERR;
    }
    else if ((qmux_wul_fd = open( QMUX_WAKEUNLOCK_FILE, O_WRONLY|O_APPEND )) < 0)
    {
      QMI_ERR_MSG_3 ("linux_qmi_qmux_io_pwr_up_init.c: Unable to open wakeunlock file = %s, error [%d:%s]\n",
                     QMUX_WAKEUNLOCK_FILE, errno, strerror(errno));
      rc = QMI_INTERNAL_ERR;
    }
    else
    {
      rc = QMI_NO_ERR;
    }
  }
  else
  {
    rc = QMI_INTERNAL_ERR;
  }

  return rc;
}



/*===========================================================================
  FUNCTION  linux_qmi_qmux_io_open_conn
===========================================================================*/
/*!
@brief
  Function used to open a connection.  This function must be called
  prior to sending any messages or receiving any indications

@return
  0 if function is successful, negative value if not.

@note
  - Side Effects
    - Opens up SMD port and spawns a thread for RX handling.

*/
/*=========================================================================*/
int
linux_qmi_qmux_io_open_conn
(
  qmi_connection_id_type  conn_id,
  unsigned char           *rx_buf,
  int                     rx_buf_len
)
{
  linux_qmi_qmux_io_conn_info_type  *conn_info;

  /* Verifiy the conn_id parameter since it will be used to index
  ** into an array
  */
  if (conn_id >= QMI_MAX_CONNECTIONS || conn_id < 0)
  {
    QMI_ERR_MSG_1 ("linux_qmi_qmux_io.c: bad conn_id = %d!\n",conn_id);
    return QMI_INTERNAL_ERR;
  }

  if (rx_buf == NULL)
  {
    QMI_ERR_MSG_0 ("linux_qmi_qmux_io.c: NULL rxbuf pointer\n");
    return QMI_INTERNAL_ERR;
  }

  /* Assure that power-up init function has been called */
  if (linux_qmi_qmux_io_rx_cb == NULL)
  {
    QMI_ERR_MSG_0 ("linux_qmi_qmux_io.c: power-up init not called\n");
    return QMI_INTERNAL_ERR;
  }

  /* Set up pointer to connection info */
  conn_info = &linux_qmi_qmux_io_conn_info[conn_id];

  if (conn_info->flags & QMI_QMUX_IO_CONN_IS_ACTIVE)
  {
    QMI_ERR_MSG_1 ("linux_qmi_qmux_io.c: connection already opened = %d!\n",conn_id);
    return QMI_INTERNAL_ERR;
  }

  /* Set the RX buffer in connection info */
  conn_info->rx_buf = rx_buf;
  conn_info->rx_buf_len = rx_buf_len;

  if ( QMI_CONN_ID_IS_PROXY( conn_id ) )
  {
    /* Open a QMI Proxy server connection */
    conn_info->f_desc = linux_qmi_qmux_io_open_conn_proxy(conn_info->port_id_name);
  }
  else
  {
    /* Open the SMD port */
    QMI_DEBUG_MSG_1("linux_qmi_qmux_io_open_conn: open SMD port %s for RDWR\n",
                     conn_info->port_id_name);
    conn_info->f_desc = open (conn_info->port_id_name,O_RDWR);
  }

  if (conn_info->f_desc < 0)
  {
    QMI_ERR_MSG_3 ("linux_qmi_qmux_io.c: Unable to open port id %s, error [%d:%s]\n",
                   conn_info->port_id_name, errno, strerror(errno));
    return QMI_INTERNAL_ERR;
  }

  /* Set write call to be blocking for SMD ports */
  if ((conn_id >= QMI_CONN_ID_RMNET_0) && (conn_id <= QMI_CONN_ID_RMNET_8))
  {
    int blocking_write = 1;
    ioctl(conn_info->f_desc, SMD_PKT_IOCTL_BLOCKING_WRITE, &blocking_write);
  }

  /* Spawn RX thread and pass to it the QMUX connection ID */
  if ((pthread_create (&conn_info->th_id,
                       NULL,
                       linux_qmi_qmux_io_rx_msg,
                       (void *) conn_id)) != 0)
  {
    QMI_ERR_MSG_0 ("qmux_open_connection: can't create RX thread");
    return QMI_INTERNAL_ERR;
  }

  /* We are done, mark connection as active and return success code */
  conn_info->flags |= QMI_QMUX_IO_CONN_IS_ACTIVE;

  return QMI_NO_ERR;
}

