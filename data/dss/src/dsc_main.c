/******************************************************************************

                        D S C _ M A I N . C

******************************************************************************/

/******************************************************************************

  @file    dsc_main.c
  @brief   DSC's main function implementation

  DESCRIPTION
  Implementation of DSC's main function.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/src/dsc_main.c#5 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
03/21/08   vk         Incorporated code review comments
11/30/07   vk         Cleaned up lint warnings
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "dsci.h"
#include "dsc_main.h"
#include "dsc_util.h"
#include "dsc_cmd.h"
#include "dsc_kif.h"
#include "dsc_qmi_wds.h"
#include "dsc_dcmi.h"
#include "dsc_call.h"
#include "dsc_test.h"
#include <stdlib.h>
#include <fcntl.h> /* open, read */

#ifndef FEATURE_DS_LINUX_NO_RPC
#include "dsc_dcm_api_rpc.h"
#endif
/*===========================================================================
                     FORWARD DECLARATIONS
===========================================================================*/

/* Note: Remove these once these are available in an oncrpc header file */


#ifndef FEATURE_DS_LINUX_NO_RPC
extern void oncrpc_init(void);
extern void oncrpc_task_start(void);
#endif
/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*---------------------------------------------------------------------------
   Type representing program run mode
---------------------------------------------------------------------------*/
typedef enum {
    DSC_MAIN_RUNMODE_BACK = 0, /* Run as a daemon process */
    DSC_MAIN_RUNMODE_FORE = 1  /* Run as a console process */
} dsc_main_runmode_t;

/*---------------------------------------------------------------------------
   Collection of program configuration info
---------------------------------------------------------------------------*/
struct dsc_main_cfg_s {
    int runmode;        /* Process run mode */
    int logmode;        /* Logging mode */
    int logthreshold;   /* Logging threshold */
	int nint;           /* Number of interfaces */
    char * iname;       /* Name of virtual ethernet interface */
    int skip;           /* Whether to skip driver module loading */
    char * dirpath;     /* Directory pathname to look for script files */
    char * modscr;      /* Name of script to use for loading module */
    char * dhcpscr;     /* Name of dhcp script to pass to udhcpc */
    //char * tech;        /* Technology */
} dsc_main_cfg = {
    -1, -1, -1, -1, 0, -1, 0, 0, 0
}; /* Initialize everything to invalid values */

#define DSC_MAIN_MAX_NINT DSC_MAX_PRICALL
int dsc_main_nint = DSC_MAIN_MAX_NINT;
int dsc_main_interfaces[DSC_MAIN_MAX_NINT] = {1, 1, 1};

#define DSC_MAIN_CFG_PARAM_LEN 11

/*---------------------------------------------------------------------------
    array of strings used to identify which port is used by USB RmNet
    order of strings is important as we use index of the string element
    internally inside dss library.
---------------------------------------------------------------------------*/
char * dsc_main_shared_ports[DSC_MAIN_MAX_NINT] =
{ "DATA5_CNTL",
  "DATA6_CNTL",
  "DATA7_CNTL"
};

/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_main_process_arg
===========================================================================*/
/*!
@brief
  Populates program configuration information for the specified argument and
  argument value.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void
dsc_main_process_arg(char argid, char * argval)
{
    switch (argid) {
    case 'f':
        /* run in foreground, i.e. don't run as a daemon process */
        dsc_main_cfg.runmode = (int)DSC_MAIN_RUNMODE_FORE;
        fprintf(stderr, "running in foreground..\n");
        break;
    case 's':
        /* Log to syslog. By default program will log to stderr */
        dsc_main_cfg.logmode = (int)DSC_LOG_MODE_SYSLOG;
        fprintf(stderr, "using syslog..\n");
        break;
    case 'l':
        /* Logging threshold as an integer value */
        dsc_main_cfg.logthreshold = dsc_atoi(argval);
        fprintf(stderr, "using log level %d..\n", dsc_atoi(argval));
        break;
    case 'n':
        /* Number of interfaces to create */
        dsc_main_cfg.nint = dsc_atoi(argval);
	fprintf(stderr, "cfging %d interfaces..\n", dsc_atoi(argval));
	break;
    case 'i':
        /* Interface name to use */
        dsc_main_cfg.iname = argval;
        fprintf(stderr, "using interface name %s\n", argval);
        break;
    case 'k':
        /* Skip driver module load. Module will be loaded externally */
        dsc_main_cfg.skip = DSC_KIF_SKIP;
        fprintf(stderr, "skipping module load\n");
        break;
    case 'd':
        /* Directory pathname to search for script files */
        dsc_main_cfg.dirpath = argval;
        fprintf(stderr, "using relative path %s\n", argval);
        break;
    case 'm':
        /* Name of driver module load script */
        dsc_main_cfg.modscr = argval;
        fprintf(stderr, "using module load script %s\n", argval);
        break;
    case 'u':
        /* Name of dhcp script passed to udhcpc program */
        dsc_main_cfg.dhcpscr = argval;
        fprintf(stderr, "using dhcp script %s\n", argval);
        break;
    //case 't':
        /* Technology (CDMA/UMTS) to use */
    //  dsc_main_cfg.tech = argval;
    //  fprintf(stderr, "using technology %s\n", argval);
    //  break;
    default:
        /* Ignore unknown argument */
        fprintf(stderr, "ignoring unknown arg '%c'\n", argid);
        break;
    }
    return;
}

/*===========================================================================
  FUNCTION  dsc_main_parse_args
===========================================================================*/
/*!
@brief
  Parses all specified program command line arguments and populates
  configuration information.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void
dsc_main_parse_args (int argc, char ** argv)
{
    int i;
    char a;

    for (i = 1; i < argc; ++i) {
        if (strlen(argv[i]) < 2) {
            /* Minimum length of a valid argument is 2, as each arg is
            ** prefixed by a '-' char.
            */
            continue;
        }

        if (*argv[i] != '-') {
            /* Every valid arg should begin with a '-' */
            continue;
        }

        /* Get next char in argument, which is the arg type */
        a = *(argv[i] + 1);

        /* Process based on type of argument */
        switch (a) {
        case 'l':
        case 'n':
        case 'i':
        case 'd':
        case 'm':
        case 'u':
        case 't':
            /* These arguments have an associated value immediately following
            ** the argument.
            */
            if (++i < argc) {
                dsc_main_process_arg(a, argv[i]);
            }
            break;
        case 'f':
        case 's':
        case 'k':
            /* These arguments do not have any value following the argument */
            dsc_main_process_arg(a, 0);
            break;
        default:
            /* Everything else is an unknown arg that is ignored */
            fprintf(stderr, "unknown arg %s specified\n", argv[i]);
        }
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_main_reset_interfaces
===========================================================================*/
/*!
@brief
  selects all the interfaces for use by dss library. Typically,
  this is the default behavior unless another sub system (e.g.
  USB rmnet) wanted to use one of the default smd ports.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
inline void
dsc_main_reset_interfaces()
{
    int i=0;

    dsc_log_high("dsc_main_reset_interfaces: " \
                 "reset dsc_main_nint to %d",
                 DSC_MAIN_MAX_NINT);
    dsc_main_nint = DSC_MAIN_MAX_NINT;
    for(i=0; i<DSC_MAIN_MAX_NINT; i++)
    {
        dsc_main_interfaces[i] = 1;
    }
}

/*===========================================================================
  FUNCTION  dsc_main_read_data_ctl_port
===========================================================================*/
/*!
@brief
  reads one data control port string at a time

@return
  bytes read

@note
  valid data control port strings are : DATAXX_CNTL
  examples: "DATA7_CNTL DATA77_CNTL"
  multiple values must be delimited by exactly one delimeter char
  pick your own delimeter that doesn't collide with any alphanumeric
  char or '_'
*/
/*=========================================================================*/
static int dsc_main_read_data_ctl_port
(
  int fd,
  char * buf
)
{
  int bytes_read = 0, temp = 0;
  char ch;

  if (buf == NULL)
    return 0;

  do
  {
    temp = read(fd, &ch, 1);

    if (temp)
    {
      /* we only care about alphanumeric chars
         and '_'; rest are ignored */
      if (('0' <= ch && ch <= '9') ||
          ('a' <= ch && ch <= 'z') ||
          ('A' <= ch && ch <= 'Z') ||
          ('_' == ch))
      {
        bytes_read += temp;
        *buf++ = ch;
        *buf = '\0';
        /* read the delimiter if we reached max len */
        if (bytes_read == DSC_MAIN_CFG_PARAM_LEN)
        {
          read(fd, &ch, 1);
          break;
        }
      }
      else
      {
        break;
      }
    }

  } while ( bytes_read != 0 );

  return bytes_read;
}

/*===========================================================================
  FUNCTION  dsc_main_parse_cfg
===========================================================================*/
/*!
@brief
  Parses the given cfg file (file descriptor) in order to populate
  dsc_main_nint and dsc_main_interfaces variables.

  The given file should contain a set of strings indicating
  which ports are going to be used by other sub system
  (such as USB rmnet)

  The only strings we recognize are listed as a global array
  called dsc_main_shared_ports.

  dsc_main_nint should indicate how many interfaces are enabled.

  dsc_main_interfaces contain boolean value (0/1) that indicates if
  corresponding interface is enabled or not.

  For example, if the given file contains DATA7_CNTL,
  dsc_main_nint = 2
  dsc_main_interfaces[0] = 1 ; in-use by dss
  dsc_main_interfaces[1] = 1 ; in-use by dss
  dsc_main_interfaces[2] = 0 ; not in-use by dss

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - dsc_main_nint and dsc_main_interfaces[] will be updated according to
      the file contents
    - if the given file is malformed, by default, all interfaces will be
      selected to be used by dss
*/
/*=========================================================================*/
static void dsc_main_parse_cfg(int fd)
{
    int bytes_read=0, i=0;
    /* buffer should be big enough to hold the string
       specified by USB rmnet driver in sysfs file,
       add 1 for \0 character */
    char buffer[DSC_MAIN_CFG_PARAM_LEN+1];
    int nint=0;

    ds_assert(fd > 0);

    /* by default, we use all interfaces */
    dsc_main_reset_interfaces();

    do
    {
      memset(buffer, 0, DSC_MAIN_CFG_PARAM_LEN+1);

      /* read next data control port from the config file into buffer  */
      bytes_read = dsc_main_read_data_ctl_port(fd,
                                               buffer);


      if (!bytes_read)
      {
        dsc_log_err ("dsc_main_parse_cfg: " \
                     "no more data control ports found in cfg file");
        break;
      }
      else
      {
        dsc_log_high ("dsc_main_parse_config: " \
                      "data control port %s found in cfg file",
                      buffer);
      }

      for (nint=0; nint<DSC_MAIN_MAX_NINT; nint++)
      {
        /* if one of the dsc_main_shared_ports found in buffer,
           disable it's use in dss */
        if (!strcmp(dsc_main_shared_ports[nint], buffer))
        {
          dsc_main_interfaces[nint] = 0;
          dsc_main_nint--;
        }
      }

    } while (1);

    /* print out configuration used */
    dsc_log_high("dsc_main_parse_cfg: set dsc_main_nint to %d",
                 dsc_main_nint);
    for(i=0; i<DSC_MAIN_MAX_NINT; i++)
    {
        dsc_log_high("dsc_main_interfaces[%d] = %d", i, dsc_main_interfaces[i]);
    }

}

/*===========================================================================
  FUNCTION  dsc_rpc_init
===========================================================================*/
/*!
@brief
  Initializes RPC service for exporting API to remote clients.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void
dsc_rpc_init (void)
{
    #ifndef FEATURE_DS_LINUX_NO_RPC

   	 oncrpc_init();
	dsc_dcm_api_app_init();
	oncrpc_task_start();

    #endif
}


/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_main
===========================================================================*/
/*!
@brief
  Main entry point of the dsc program. Performs all program initialization.

@return
  int - 0 always

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
dsc_main (int argc, char ** argv)
{
    int fd = 0;

    /*Initialize Diag services*/\
#ifdef FEATURE_DATA_LOG_QXDM
    boolean ret_val = FALSE;
    ret_val = Diag_LSM_Init(NULL);

    if ( !ret_val )
    {

    }
#endif
  /* Parse command line arguments and populate configuration blob */
    dsc_main_parse_args(argc, argv);

    /* Initialize logging as per desired mode */
#ifdef FEATURE_DATA_LOG_SYSLOG
    dsc_log_init(dsc_main_cfg.logthreshold, dsc_main_cfg.logmode);
#endif

    /* Run as a daemon, if requested */
    if (dsc_main_cfg.runmode != (int)DSC_MAIN_RUNMODE_FORE) {
        //dsc_daemonize();
        dsc_log_low( "daemonize completed" );
        (void)sleep(1);
    }

    /* Initialize and start Command thread */
    dsc_cmdthrd_init();
    dsc_log_high( "cmd thread completed" );
    //(void)sleep(1);

    /* if a file node  exists in sysfs, use it */
    fd = open("/sys/module/f_rmnet/parameters/rmnet_ctl_ch", O_RDONLY);
    if (fd < 0)
    {
      fd = open("/sys/module/rmnet/parameters/rmnet_ctl_ch", O_RDONLY);
    }
    if (fd < 0)
    {
        dsc_log_high("couldn't open port config file. " \
                     "using %d default interfaces",
                     DSC_MAIN_MAX_NINT);
        dsc_main_reset_interfaces();
    }
    else
    {
        dsc_main_parse_cfg(fd);
    }

    /* Initialize DCM module */
    dsc_dcm_init();
    dsc_log_low( "dcm init completed" );
    //(void)sleep(1);

    /* Initialize Call SM */
    dsc_call_init(dsc_main_nint);
    dsc_log_low( "call init completed" );
    //(void)sleep(1);

    /* Initialize QMI WDS if module and qmi driver */
    dsc_qmi_init(dsc_main_nint, dsc_main_interfaces);
    dsc_log_low( "qmi init completed" );
    //(void)sleep(1);

    /* Initialize kernel if module */
    dsc_kif_init
    (
        dsc_main_nint,
        dsc_main_interfaces,
        dsc_main_cfg.iname,
        dsc_main_cfg.skip,
        dsc_main_cfg.dirpath,
        dsc_main_cfg.modscr,
        dsc_main_cfg.dhcpscr
    );
    dsc_log_low( "kif init completed" );
    //(void)sleep(1);

    dcm_debug_print_iface_array();

    /* Initialize ONCRPC. This is done last as after this we can begin to
    ** receive application requests.
    */
#ifndef FEATURE_DS_LINUX_NO_RPC
        dsc_log_low( "now call RPC init" );
	//dsc_rpc_init();
        dsc_log_low( "RPC init complete" );
#endif
    dsc_log(DSC_DBG_LEVEL_LOW, "Initialization complete..\n");

    /* DEBUG code */
	#ifdef FEATURE_DS_DEBUG_INTERACTIVE

    dsc_test();

	#else

#ifndef FEATURE_DS_LINUX_NO_RPC
	for (;;) {
		(void)sleep(5);
	}
#endif
	#endif
    return 0;
}
