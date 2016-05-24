/******************************************************************************
  @file    qmi_platform.c
  @brief   The QMI management layer.  This includes system-wide intialization
  and configuration funtions.

  DESCRIPTION
  QMI management.  Routines for client, system-wide initialization
  and de-initialization

  INITIALIZATION AND SEQUENCING REQUIREMENTS
  qmi_init() and qmi_connection_init() needs to be called before starting
  any of the specific service clients.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2013 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/
#include <sys/time.h>
#include <errno.h>
#include "qmi_i.h"
#include "qmi_platform_config.h"
#ifdef FEATURE_QMI_ANDROID
#include <cutils/properties.h>
#else
#define PROPERTY_VALUE_MAX 1
#endif

/* These strings need to correspond to the qmi_connection_id_type enum.  The first
** string must correspond to QMI_CONN_ID_RMNET_1, etc.  Note that we are currently only
** supporting the non-broadcast ports.
*/
static struct {
  qmi_connection_id_type  conn_id;
  const char *dev_name;
} dev_id_table[] =
{
  { QMI_CONN_ID_RMNET_0,            QMI_PORT_RMNET_0         },
  { QMI_CONN_ID_RMNET_1,            QMI_PORT_RMNET_1         },
  { QMI_CONN_ID_RMNET_2,            QMI_PORT_RMNET_2         },
  { QMI_CONN_ID_RMNET_3,            QMI_PORT_RMNET_3         },
  { QMI_CONN_ID_RMNET_4,            QMI_PORT_RMNET_4         },
  { QMI_CONN_ID_RMNET_5,            QMI_PORT_RMNET_5         },
  { QMI_CONN_ID_RMNET_6,            QMI_PORT_RMNET_6         },
  { QMI_CONN_ID_RMNET_7,            QMI_PORT_RMNET_7         },
  { QMI_CONN_ID_RMNET_8,            QMI_PORT_RMNET_8         },
  { QMI_CONN_ID_RMNET_9,            QMI_PORT_RMNET_9         },
  { QMI_CONN_ID_RMNET_10,           QMI_PORT_RMNET_10        },
  { QMI_CONN_ID_RMNET_11,           QMI_PORT_RMNET_11        },

  { QMI_CONN_ID_REV_RMNET_0,        QMI_PORT_REV_RMNET_0     },
  { QMI_CONN_ID_REV_RMNET_1,        QMI_PORT_REV_RMNET_1     },
  { QMI_CONN_ID_REV_RMNET_2,        QMI_PORT_REV_RMNET_2     },
  { QMI_CONN_ID_REV_RMNET_3,        QMI_PORT_REV_RMNET_3     },
  { QMI_CONN_ID_REV_RMNET_4,        QMI_PORT_REV_RMNET_4     },
  { QMI_CONN_ID_REV_RMNET_5,        QMI_PORT_REV_RMNET_5     },
  { QMI_CONN_ID_REV_RMNET_6,        QMI_PORT_REV_RMNET_6     },
  { QMI_CONN_ID_REV_RMNET_7,        QMI_PORT_REV_RMNET_7     },
  { QMI_CONN_ID_REV_RMNET_8,        QMI_PORT_REV_RMNET_8     },

  { QMI_CONN_ID_RMNET_SDIO_0,       QMI_PORT_RMNET_SDIO_0    },
  { QMI_CONN_ID_RMNET_SDIO_1,       QMI_PORT_RMNET_SDIO_1    },
  { QMI_CONN_ID_RMNET_SDIO_2,       QMI_PORT_RMNET_SDIO_2    },
  { QMI_CONN_ID_RMNET_SDIO_3,       QMI_PORT_RMNET_SDIO_3    },
  { QMI_CONN_ID_RMNET_SDIO_4,       QMI_PORT_RMNET_SDIO_4    },
  { QMI_CONN_ID_RMNET_SDIO_5,       QMI_PORT_RMNET_SDIO_5    },
  { QMI_CONN_ID_RMNET_SDIO_6,       QMI_PORT_RMNET_SDIO_6    },
  { QMI_CONN_ID_RMNET_SDIO_7,       QMI_PORT_RMNET_SDIO_7    },

  { QMI_CONN_ID_RMNET_USB_0,        QMI_PORT_RMNET_USB_0     },
  { QMI_CONN_ID_RMNET_USB_1,        QMI_PORT_RMNET_USB_1     },
  { QMI_CONN_ID_RMNET_USB_2,        QMI_PORT_RMNET_USB_2     },
  { QMI_CONN_ID_RMNET_USB_3,        QMI_PORT_RMNET_USB_3     },
  { QMI_CONN_ID_RMNET_USB_4,        QMI_PORT_RMNET_USB_4     },
  { QMI_CONN_ID_RMNET_USB_5,        QMI_PORT_RMNET_USB_5     },
  { QMI_CONN_ID_RMNET_USB_6,        QMI_PORT_RMNET_USB_6     },
  { QMI_CONN_ID_RMNET_USB_7,        QMI_PORT_RMNET_USB_7     },

  { QMI_CONN_ID_REV_RMNET_USB_0,    QMI_PORT_REV_RMNET_USB_0 },
  { QMI_CONN_ID_REV_RMNET_USB_1,    QMI_PORT_REV_RMNET_USB_1 },
  { QMI_CONN_ID_REV_RMNET_USB_2,    QMI_PORT_REV_RMNET_USB_2 },
  { QMI_CONN_ID_REV_RMNET_USB_3,    QMI_PORT_REV_RMNET_USB_3 },
  { QMI_CONN_ID_REV_RMNET_USB_4,    QMI_PORT_REV_RMNET_USB_4 },
  { QMI_CONN_ID_REV_RMNET_USB_5,    QMI_PORT_REV_RMNET_USB_5 },
  { QMI_CONN_ID_REV_RMNET_USB_6,    QMI_PORT_REV_RMNET_USB_6 },
  { QMI_CONN_ID_REV_RMNET_USB_7,    QMI_PORT_REV_RMNET_USB_7 },
  { QMI_CONN_ID_REV_RMNET_USB_8,    QMI_PORT_REV_RMNET_USB_8 },

  { QMI_CONN_ID_RMNET_SMUX_0,       QMI_PORT_RMNET_SMUX_0    },

  { QMI_CONN_ID_RMNET_12,           "not used"               },
  { QMI_CONN_ID_RMNET_13,           "not used"               },

  { QMI_CONN_ID_RMNET_MDM2_0,       QMI_PORT_RMNET2_USB_0    },
  { QMI_CONN_ID_RMNET_MDM2_1,       QMI_PORT_RMNET2_USB_1    },
  { QMI_CONN_ID_RMNET_MDM2_2,       QMI_PORT_RMNET2_USB_2    },
  { QMI_CONN_ID_RMNET_MDM2_3,       QMI_PORT_RMNET2_USB_3    },
  { QMI_CONN_ID_RMNET_MDM2_4,       QMI_PORT_RMNET2_USB_4    },
  { QMI_CONN_ID_RMNET_MDM2_5,       QMI_PORT_RMNET2_USB_5    },
  { QMI_CONN_ID_RMNET_MDM2_6,       QMI_PORT_RMNET2_USB_6    },
  { QMI_CONN_ID_RMNET_MDM2_7,       QMI_PORT_RMNET2_USB_7    },
  { QMI_CONN_ID_PROXY,              QMI_PORT_PROXY           }
};

static const char *qmi_linux_invalid_dev_name = "rmnet_invalid";

#define PORT_NAME_TABLE_SIZE (sizeof (dev_id_table) / sizeof (dev_id_table[0]))
#define NANO_SEC 1000000000
int
qmi_linux_wait_for_sig_with_timeout
(
  qmi_linux_signal_data_type  *signal_ptr,
  int                         timeout_milli_secs
)
{
  int rc = QMI_NO_ERR;
  struct timeval curr_time;
  struct timespec wait_till_time;

  /* Get current time of day */
  gettimeofday (&curr_time,NULL);

  /* Set wait time seconds to current + the number of seconds needed for timeout */
  wait_till_time.tv_sec =  curr_time.tv_sec + (timeout_milli_secs/1000);
  wait_till_time.tv_nsec = (curr_time.tv_usec * 1000) +  ((timeout_milli_secs % 1000) * 1000 * 1000);

  /* Check the nano sec overflow */
  if (wait_till_time.tv_nsec >= NANO_SEC ) {

      wait_till_time.tv_sec +=  wait_till_time.tv_nsec/NANO_SEC;
      wait_till_time.tv_nsec %= NANO_SEC;
  }

  while ((signal_ptr)->cond_predicate == FALSE)
  {
    if (pthread_cond_timedwait (&(signal_ptr)->cond_var,
                                &(signal_ptr)->cond_mutex,
                                &wait_till_time) == ETIMEDOUT)
    {
      rc = QMI_TIMEOUT_ERR;
      break;
    }
  }
  pthread_mutex_unlock (&(signal_ptr)->cond_mutex);

  return rc;
}


qmi_connection_id_type
qmi_linux_get_conn_id_by_name
(
  const char *name
)
{
  unsigned int i;
  size_t name_len;
  qmi_connection_id_type rc = QMI_CONN_ID_INVALID;

  if (name != NULL)
  {
    name_len = strlen (name);

    for (i = 0; i < PORT_NAME_TABLE_SIZE; i++)
    {
      /* Make sure strings are same length */
      if (name_len != strlen (dev_id_table[i].dev_name))
      {
        continue;
      }

      /* Do string in-sensitive comparison to see if they are
      ** equivalent
      */
      if (strncasecmp (dev_id_table[i].dev_name,name,name_len) == 0)
      {
        break;
      }
    }

    if (i < PORT_NAME_TABLE_SIZE)
    {
      rc = dev_id_table[i].conn_id;
    }
  }

  return rc;
}


const char *
qmi_linux_get_name_by_conn_id
(
  qmi_connection_id_type conn_id
)
{
  unsigned int i;
  const char *rc = qmi_linux_invalid_dev_name;
  for (i = 0; i < PORT_NAME_TABLE_SIZE; i++)
  {
    if (dev_id_table[i].conn_id == conn_id)
    {
      rc = dev_id_table[i].dev_name;
      break;
    }
  }
  return rc;
}

#ifdef FEATURE_DATA_LOG_QXDM

/*=========================================================================
  FUNCTION:  qmi_format_diag_log_msg

===========================================================================*/
/*!
    @brief
    Format debug message for logging.

    @return
    None
*/
/*=========================================================================*/
void qmi_format_diag_log_msg
(
  char *buf_ptr,
  int buf_size,
  char *fmt,
  ...
)
{
  va_list ap;

  /*-----------------------------------------------------------------------*/

  if ( buf_ptr == NULL || buf_size <= 0 )
  {
    return;
  }

  /*-----------------------------------------------------------------------*/

  va_start( ap, fmt );

  vsnprintf( buf_ptr, buf_size, fmt, ap );

  va_end( ap );

} /* dsc_format_log_msg */

#endif /* FEATURE_DATA_LOG_QXDM */


const char * qmi_linux_get_internal_use_port
(
  void
)
{
  const char *rc = NULL;

#ifdef FEATURE_QMI_ANDROID
  char baseband [PROPERTY_VALUE_MAX];
   /* Retrieve the Android baseband property */
  memset(&baseband, 0, sizeof(baseband));

  property_get("ro.baseband", baseband, "");

  if(strlen(baseband) > 0)
  {
    /* CSFB target has only SDIO transport ports */
    if( !strcmp(baseband, "mdm") )
    {
      rc = QMI_PORT_RMNET_USB_0;
    }
    /* MDMUSB target replaces SDIO transport with USB, suppresses SMD/BAM */
    else if( !strcmp(baseband, "csfb") )
    {
      rc = QMI_PORT_RMNET_SDIO_0;
    }
    else if( !strcmp(baseband, "sglte") )
    {
      rc = QMI_PORT_RMNET_SMUX_0;
    }
    else if( !strcmp(baseband, "dsda") )
    {
      rc = QMI_PORT_RMNET_USB_0;
    }
    else if( !strcmp(baseband, "dsda2") )
    {
      rc = QMI_PORT_RMNET_USB_0;
    }
    else
    {
      rc = QMI_PORT_RMNET_0;
    }
  }
  else
  {
    QMI_ERR_MSG_0 ("Baseband property not found, internal use port is unset!!\n");
  }

#else /*FEATURE_QMI_ANDROID*/

  rc = QMI_PORT_RMNET_0;

#endif /*FEATURE_QMI_ANDROID*/
  if (rc)
  {
    QMI_DEBUG_MSG_1 ("Setting internal use port to %s\n",rc);
  }
  else
  {
    QMI_ERR_MSG_0 ("Internal use port is unset!!\n");
  }

  return rc;
}
