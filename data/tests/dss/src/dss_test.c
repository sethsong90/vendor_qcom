/******************************************************************************

                           D S S _ T E S T . C

******************************************************************************/

/******************************************************************************

  @file    dss_test.c
  @brief   DSS API Test Suite Common Framework

  DESCRIPTION
  Implementation of the framework for a DSS API test suite.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2012 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/dss_test.c#7 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/05/08   vk         Added testing of net policy verification.
01/31/08   vk         Added support for Network Controller test app.
01/23/08   vk         Added support for uplink TCP data transfer tests.
09/28/07   vk         Initial version

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include "dss_test.h"
#ifndef FEATURE_DATA_LINUX_LE
#include "dsc_main.h"
#endif

#define MAX_PROG_ARGS 18

#define DEFAULT_BUF_SIZE 128
#define MAX_BUF_SIZE     1460

#define DEFAULT_SERVER_IPADDR "10.46.15.22"
#define DEFAULT_SERVER_PORT   1303

#define DEFAULT_TIME_WAIT     0

#define DEFAULT_TECH  DSS_TEST_TECH_UMTS

typedef struct
{
  char * tech_name;
  int    tech_type;
} dss_test_tech_name_to_type_map_t;

const dss_test_tech_name_to_type_map_t dss_test_tech_name_to_type_map_arr[] = {
  { "umts", DSS_TEST_TECH_UMTS},
  { "gprs", DSS_TEST_TECH_UMTS},
  { "cdma", DSS_TEST_TECH_CDMA},
  { "1x",   DSS_TEST_TECH_CDMA}
};

const char * Test_id_str;
const char * Test_name_str;
const char * Test_desc_str;
int          Test_tech;

dss_test_status_t Test_status = DSS_TEST_FAIL;

struct
{
  FILE * log_fp; 
  int    bufsz;
  int    srvr_port;
  const char * srvr_ipaddr;
  int    time_wait;
  const char * master_script;
  const char * command;
  int    tech;
} dss_test_cfg;

dss_test_prog_args_template_t Prog_args_template[] = {
  { "--logfile", 'l', DSS_TEST_ARG_STR},      /* log file name */
  { "--help", 'h', DSS_TEST_ARG_VOID},        /* print help */
  { "--name", 'n', DSS_TEST_ARG_VOID},        /* print test name */
  { "--desc", 'd', DSS_TEST_ARG_VOID},        /* print test description */
  { "--bufsize", 'b', DSS_TEST_ARG_INT},      /* buf/burst size */
  { "--datafile", 'f', DSS_TEST_ARG_STR},     /* data file name */
  { "--serverip", 'i', DSS_TEST_ARG_STR},     /* server ip addr */
  { "--serverport", 'p', DSS_TEST_ARG_INT},   /* server port number */
  { "--timewait", 'w', DSS_TEST_ARG_INT},     /* time to wait in ms */
  { "--udp", 'u', DSS_TEST_ARG_VOID},         /* peer test app mode udp */
  { "--tcp", 't', DSS_TEST_ARG_VOID},         /* peer test app mode tcp */
  { "--client", 'c', DSS_TEST_ARG_VOID},      /* peer test app mode client */
  { "--master-script", 'm', DSS_TEST_ARG_STR},/* master cmd script to use */
  { "--command", 'C', DSS_TEST_ARG_STR},      /* command to send */
  { "--profile", 'k', DSS_TEST_ARG_INT},      /* pdp profile number to use */
  { "--daemon", '-', DSS_TEST_ARG_VOID},      /* run as daemon */
  { "--tech", 'r', DSS_TEST_ARG_STR},         /* radio technology type */
  { "--family", 'v', DSS_TEST_ARG_INT}        /* IP family version V4/V6*/
};

dss_test_prog_args_t Prog_args[MAX_PROG_ARGS];
int Prog_args_size;

pthread_mutex_t Net_mutx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  Net_cond = PTHREAD_COND_INITIALIZER;
volatile sint15 Net_mutx_dss_nh;
volatile int    Net_ev_pending;

pthread_mutex_t Sock_mutx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  Sock_cond = PTHREAD_COND_INITIALIZER;
volatile sint15 Sock_mutx_dss_nh;
volatile int    Sock_ev_pending;

FILE * 
dss_test_cfg_get_log_fp (void)
{
  return dss_test_cfg.log_fp;
}

const char * 
dss_test_prog_args_get_val_str (const dss_test_prog_args_t * arg_ptr)
{
  ds_assert(arg_ptr);
  ds_assert(arg_ptr->type == DSS_TEST_ARG_STR);
  ds_assert(arg_ptr->val.str_val);

  return arg_ptr->val.str_val;
}

int 
dss_test_prog_args_get_val_int (const dss_test_prog_args_t * arg_ptr)
{
  ds_assert(arg_ptr);
  ds_assert(arg_ptr->type == DSS_TEST_ARG_INT);

  return arg_ptr->val.int_val;
}

const dss_test_prog_args_t * 
dss_test_find_prog_args 
(
char short_name, 
const char * argname, 
const dss_test_prog_args_t * arg_arr, 
int   arg_arr_sz
)
{
  const dss_test_prog_args_t * arg_ptr;
  const dss_test_prog_args_t * ret_ptr = NULL;
  int i;

  for (i = 0; i < arg_arr_sz; ++i)
  {
    arg_ptr = arg_arr + i;
    if (arg_ptr->name == NULL)
    {
      break;
    }
    if (short_name == '-')
    {
      ds_assert(argname);
      if (strcmp(argname, arg_ptr->name) == 0)
      {
        ret_ptr = arg_ptr;
        break;
      }
    }
    else
    {
      if (short_name == arg_ptr->short_name)
      {
        ret_ptr = arg_ptr;
        break;
      }
    }
  }

  return ret_ptr;
}

const dss_test_prog_args_template_t * 
dss_test_find_prog_args_template 
(
const char * argname,
const dss_test_prog_args_template_t * arg_template_arr,
int arg_template_arr_sz
)
{
  int i;
  const dss_test_prog_args_template_t * ret_ptr = NULL;
  const dss_test_prog_args_template_t * tmp_ptr;
  char short_name = '-';


  if ((strlen(argname) == 2) && (argname[0] == '-'))
  {
    short_name = argname[1];
  }

  for (i = 0; i < arg_template_arr_sz; ++i)
  {
    tmp_ptr = arg_template_arr + i;
    if (short_name != '-')
    {
      if (short_name == tmp_ptr->short_name)
      {
        ret_ptr = tmp_ptr;
        break;
      }
    }
    else
    {
      if (strcmp(argname, tmp_ptr->name) == 0)
      {
        ret_ptr = tmp_ptr;
        break;
      }
    }
  }

  return ret_ptr;
}

dss_test_arg_t 
dss_test_store_prog_arg 
(
dss_test_prog_args_t * arg_arr,
int arg_arr_sz,
int arr_index, 
int argc, 
char ** argv, 
int arg_index, 
const dss_test_prog_args_template_t * template_ptr
)
{
  dss_test_prog_args_t * arg_ptr;
  char * int_val_chk = NULL;

  if (arg_index >= arg_arr_sz)
  {
    return -1;
  }

  arg_ptr = arg_arr + arr_index;

  if (template_ptr->type != DSS_TEST_ARG_VOID)
  {
    if (arg_index >= argc)
    {
      return -1;
    }
  }

  arg_ptr->name = argv[arg_index];
  arg_ptr->str_val = argv[arg_index+1];
  arg_ptr->type = template_ptr->type;
  arg_ptr->short_name = template_ptr->short_name;

  switch (template_ptr->type)
  {
    case DSS_TEST_ARG_STR:
      arg_ptr->val.str_val = arg_ptr->str_val;
      break;

    case DSS_TEST_ARG_INT:
      {
        int int_val;

        int_val = strtol(arg_ptr->str_val, &int_val_chk, 0);

        /*Make sure we convert the whole string successfully*/
        if ((int_val_chk != NULL) && (*int_val_chk != '\0')) {
          fprintf(stderr, "strtol conversion of arg_val:%s "
                          "unsuccessful", arg_ptr->str_val);

          return -1;
        } else {
          arg_ptr->val.int_val = int_val;
        }
      }
      break;

    default:
      break;
  }

  return template_ptr->type;
}

int 
dss_test_parse_prog_args 
(
int argc, 
char ** argv, 
const dss_test_prog_args_template_t * arg_template_arr,
int   arg_template_arr_sz,
dss_test_prog_args_t * arg_arr,
int   arg_arr_sz
)
{
  int i;
  int rval = -1;
  const dss_test_prog_args_template_t * template_ptr;
  int arg_index = 0;
  dss_test_arg_t arg_type;

  for (i = 1; i < argc; ++i)
  {
    template_ptr = dss_test_find_prog_args_template
                   (
                   argv[i], 
                   arg_template_arr, 
                   arg_template_arr_sz
                   );

    if (!template_ptr)
    {
      fprintf(stderr, "Unknown arg %s specified. Ignoring..\n", argv[i]);
      continue;
    }

    if (arg_index > arg_arr_sz)
    {
      fprintf(stderr, "Too many args specified.\n");
      goto error;
    }
    arg_type = dss_test_store_prog_arg (
         arg_arr, 
         arg_arr_sz, 
         arg_index++, 
         argc, 
         argv, 
         i, 
         template_ptr
                                        );

    if (arg_type > DSS_TEST_ARG_VOID)
    {
      fprintf(stderr, "Error parsing arg %s..\n", argv[i]);
      goto error;
    }
    else if (arg_type != DSS_TEST_ARG_VOID)
    {
      ++i;
    }
  }

  rval = 0;

  error:
  return rval;
}

static int 
dss_test_get_tech_type_from_name (const char * tech_str)
{
  int tech_type = DEFAULT_TECH;
  int i;
  const dss_test_tech_name_to_type_map_t * map_entry;

  for (i = 0; i < (int)ARR_SIZ(dss_test_tech_name_to_type_map_arr); ++i)
  {
    map_entry = dss_test_tech_name_to_type_map_arr + i;

    if (strcasecmp(tech_str, map_entry->tech_name) == 0)
    {
      tech_type = map_entry->tech_type;
      break;
    }
  }

  return tech_type;
}

static int 
dss_test_tech_check_failed (void)
{
  int rval = 0;

  if (Test_tech != DSS_TEST_TECH_ANY)
  {
    if (Test_tech != dss_test_cfg.tech)
    {
      rval = 1;
    }
  }

  return rval;
}

void 
dss_test_print_preamble (void)
{
  const char * test_id; 
  const char * test_name;

  if (Test_id_str == NULL)
  {
    test_id = "UNDEF";
  }
  else
  {
    test_id = Test_id_str;
  }

  if (Test_name_str == NULL)
  {
    test_name = "UNDEF";
  }
  else
  {
    test_name = Test_name_str;
  }

  dss_test_log
  (
  "TEST ID: %s\nNAME: %s\n", 
  test_id, 
  test_name
  );
}

void
dss_test_print_help (dss_test_prog_args_template_t * arg_template_arr, int arg_template_arr_sz)
{
  int i;
  const dss_test_prog_args_template_t * temp_ptr;

  for (i = 0; i < arg_template_arr_sz; ++i)
  {
    temp_ptr = arg_template_arr + i;

    if (!temp_ptr->name)
    {
      break;
    }

    fprintf(stderr, "%s, %c", temp_ptr->name, temp_ptr->short_name);

    switch (temp_ptr->type)
    {
      case DSS_TEST_ARG_INT:
        fprintf(stderr, " <value:int>");
        break;
      case DSS_TEST_ARG_STR:
        fprintf(stderr, " <value:string>");
        break;
      default:
        break;
    }

    fprintf(stderr, "\n");
  }

  return;
}

void
dss_test_id_set (const char * test_id)
{
  Test_id_str = test_id;
}

void
dss_test_name_set (const char * test_name)
{
  Test_name_str = test_name;
}

void
dss_test_desc_set (const char * test_desc)
{
  Test_desc_str = test_desc;
}

void 
dss_test_tech_set (int tech)
{
  Test_tech = tech;
}

int 
dss_test_tech_get (void)
{
  return dss_test_cfg.tech;
}

dss_iface_name_enum_type 
dss_test_get_if_name_for_tech (int tech)
{
  dss_iface_name_enum_type if_name = (dss_iface_name_enum_type)0;

  switch (tech)
  {
    case DSS_TEST_TECH_UMTS:
      if_name = DSS_IFACE_UMTS;
      break;
    case DSS_TEST_TECH_CDMA:
      if_name = DSS_IFACE_CDMA_SN;
      break;
    default:
      break;
  }

  return if_name;
}

void
dss_test_daemonize (void)
{
  pid_t pid;
  pid_t sid;

  if ((pid = fork()) > 0)
  {
    exit(0);
  }

  if (pid < 0)
  {
    dss_test_log("chdir to root failed\n");
    abort();
  }

  sid = setsid();
  if (sid < 0)
  {
    dss_test_log("setsid() failed\n");
    abort();
  }

  (void)umask(0);

  if ((chdir("/")) < 0)
  {
    dss_test_log("chdir to root failed\n");
    abort();
  }

  if (freopen("/dev/null", "r", stdin) == NULL)
  {
    dss_test_log("freopen of stdin failed\n");
    abort();
  }
  if (freopen("/dev/null", "w", stdout) == NULL)
  {
    dss_test_log("freopen of stdout failed\n");
    abort();
  }
  if (freopen("/dev/null", "w", stderr) == NULL)
  {
    dss_test_log("freopen of stderr failed\n");
    abort();
  }

  return;
}

void 
dss_test_init (int argc, char ** argv)
{
  const dss_test_prog_args_t * arg_ptr;
  const char * log_f_name;
  dss_test_prog_args_t * arg_arr;
  int arg_arr_sz;
  char tech_cdma[4] = "cdma";

  /*Addition of function call to dsc_main
    used when dss and dsc are compiled 
    into a library without the stub code.*/   
   
  char *dscmain_input[13] = {
    "testprog", 
    "-f",
    "-s",
    "-k",  
    "-l",
    "0",
    "-i",
    "rmnet",
    "-u",
    "/opt/qcom/bin/udhcpc.sh",
    "-m",   
    "/opt/qcom/bin/qcomdsc-kif.sh",
    "",     
  };   

  arg_arr = &Prog_args[0];
  arg_arr_sz = ARR_SIZ(Prog_args);
  Prog_args_size = arg_arr_sz;

  if (dss_test_parse_prog_args
      (
      argc, 
      argv, 
      Prog_args_template, 
      ARR_SIZ(Prog_args_template),
      Prog_args,
      ARR_SIZ(Prog_args)
      ) < 0)
  {
    exit(1);
  }

  /* log_fp init */
  if ((arg_ptr = dss_test_find_prog_args('l', NULL, arg_arr, arg_arr_sz)) == NULL)
  {
    dss_test_cfg.log_fp = stdout;
  }
  else
  {
    log_f_name = dss_test_prog_args_get_val_str(arg_ptr);
    dss_test_cfg.log_fp = fopen(log_f_name, "a");

    if (dss_test_cfg.log_fp == NULL)
    {
      dss_test_cfg.log_fp = stdout;
    }
  }

  /* Initialize DS logging */
  ds_log_init(dss_test_cfg.log_fp);

  /* bufsz init */
  dss_test_cfg.bufsz = DEFAULT_BUF_SIZE;
  if ((arg_ptr = dss_test_find_prog_args('b', NULL, arg_arr, arg_arr_sz)) != NULL)
  {
    dss_test_cfg.bufsz = dss_test_prog_args_get_val_int(arg_ptr);
    if (dss_test_cfg.bufsz < 0)
    {
      dss_test_cfg.bufsz = DEFAULT_BUF_SIZE;
    }
    if (dss_test_cfg.bufsz > MAX_BUF_SIZE)
    {
      dss_test_cfg.bufsz = MAX_BUF_SIZE;
    }
  }

  /* srvr_ipaddr init */
  if ((arg_ptr = dss_test_find_prog_args('i', NULL, arg_arr, arg_arr_sz)) == NULL)
  {
    dss_test_cfg.srvr_ipaddr = DEFAULT_SERVER_IPADDR;
  }
  else
  {
    dss_test_cfg.srvr_ipaddr = dss_test_prog_args_get_val_str(arg_ptr);
  }

  /* srvr_port init */
  if ((arg_ptr = dss_test_find_prog_args('p', NULL, arg_arr, arg_arr_sz)) == NULL)
  {
    dss_test_cfg.srvr_port = DEFAULT_SERVER_PORT;
  }
  else
  {
    dss_test_cfg.srvr_port = dss_test_prog_args_get_val_int(arg_ptr);
  }

  /* time_wait init */
  if ((arg_ptr = dss_test_find_prog_args('w', NULL, arg_arr, arg_arr_sz)) == NULL)
  {
    dss_test_cfg.time_wait = DEFAULT_TIME_WAIT;
  }
  else
  {
    dss_test_cfg.time_wait = dss_test_prog_args_get_val_int(arg_ptr);
  }

  /* master script init */
  if ((arg_ptr = dss_test_find_prog_args('m', NULL, arg_arr, arg_arr_sz)) == NULL)
  {
    dss_test_cfg.master_script = NULL;
  }
  else
  {
    dss_test_cfg.master_script = dss_test_prog_args_get_val_str(arg_ptr);
  }

  /* command init */
  if ((arg_ptr = dss_test_find_prog_args('C', NULL, arg_arr, arg_arr_sz)) == NULL)
  {
    dss_test_cfg.command = NULL;
  }
  else
  {
    dss_test_cfg.command = dss_test_prog_args_get_val_str(arg_ptr);
  }

  /* technology type init */
  if ((arg_ptr = dss_test_find_prog_args('r', NULL, arg_arr, arg_arr_sz)) == NULL)
  {
    dss_test_cfg.tech = DEFAULT_TECH;
  }
  else
  {
    dss_test_cfg.tech = dss_test_get_tech_type_from_name
                        (
                        dss_test_prog_args_get_val_str(arg_ptr)
                        ); 
    dscmain_input[12] = tech_cdma;
  }

  /* print help message */
  if (dss_test_find_prog_args('h', NULL, arg_arr, arg_arr_sz) != NULL)
  {
    dss_test_print_help(&Prog_args_template[0], ARR_SIZ(Prog_args_template));
    exit(0);
  }

  /* run as daemon if needed */
  if ((arg_ptr = dss_test_find_prog_args('-', "--daemon", &Prog_args[0], Prog_args_size)) != NULL)
  {
    dss_test_daemonize();
  }
  
#ifndef FEATURE_DATA_LINUX_LE
  dsc_main(13,dscmain_input);     
#else
  dsnet_init();
  dss_powerup();
  dss_init();
  sleep(2);
#endif /*FEATURE_DATA_LINUX_LE*/
  return;
}

void
dss_test_enter (void)
{
  dss_test_print_preamble();
  dss_test_fail();

  /* Don't run test if technology doesn't match */
  if (dss_test_tech_check_failed())
  {
    dss_test_log("Skipped test on non-applicable tech\n");
    exit(DSS_TEST_SKIP);
  }
}

int 
dss_test_exit (void)
{
  if (!dss_test_is_pass())
  {
    dss_test_log("Failed");
    if (dss_test_is_abort())
    {
      dss_test_log(" - Unrecoverable error!\n");
      return DSS_TEST_ABORT;
    }
    else
    {
      dss_test_log("!\n");
      return DSS_TEST_FAIL;
    }
  }
  dss_test_log("Passed\n");
  return DSS_TEST_PASS;
}

static __inline__ int
dss_test_get_bufsize (void)
{
  return dss_test_cfg.bufsz;
}

static __inline__ FILE * 
dss_test_open_data_file (FILE * def_fp, const char * mode)
{
  const dss_test_prog_args_t * arg_ptr;
  FILE * fp;
  const char * data_f_name;

  if ((arg_ptr = dss_test_find_prog_args('f', NULL, &Prog_args[0], ARR_SIZ(Prog_args))) == NULL)
  {
    fp = def_fp;
  }
  else
  {
    data_f_name = dss_test_prog_args_get_val_str(arg_ptr);
    fp = fopen(data_f_name, mode);

    if (fp == NULL)
    {
      fp = def_fp;
    }
  }

  return fp;
}

static __inline__ int
dss_test_get_srvr_port (void)
{
  return dss_test_cfg.srvr_port;
}

static __inline__ const char * 
dss_test_get_srvr_ipaddr (void)
{
  return dss_test_cfg.srvr_ipaddr;
}

static __inline__ int
dss_test_get_time_wait (void)
{
  return dss_test_cfg.time_wait;
}

static __inline__ const char *
dss_test_get_master_script (void)
{
  return dss_test_cfg.master_script;
}

static __inline__ const char *
dss_test_get_command (void)
{
  return dss_test_cfg.command;
}

static void 
dss_test_set_sockaddr (struct sockaddr_in * saddr, const char * ipaddr, int port)
{
  memset(saddr, 0, sizeof(struct sockaddr_in));
  saddr->sin_family = AF_INET;
  saddr->sin_port = port;

  if (inet_aton(ipaddr, &saddr->sin_addr) == 0)
  {
    dss_test_log("error in inet_aton call!\n");
    ds_assert(0);
  }
  return;
}

static void 
dss_test_get_srvr_addr (struct sockaddr_in * saddr)
{
  int port;
  const char * ipaddr;

  port = dss_test_get_srvr_port();
  ipaddr = dss_test_get_srvr_ipaddr();

  dss_test_set_sockaddr(saddr, ipaddr, port);
  return;
}

int
dss_test_wait_for_net_event (sint15 dss_nh)
{
  sint15 dss_nh_saved;

  if (pthread_mutex_lock(&Net_mutx) != 0)
  {
    ds_assert(0);
  }

  Net_mutx_dss_nh = dss_nh;

  if (!Net_ev_pending)
  {
    if (pthread_cond_wait(&Net_cond, &Net_mutx) != 0)
    {
      ds_assert(0);
    }
  }

  Net_ev_pending = 0;
  dss_nh_saved = Net_mutx_dss_nh;

  if (pthread_mutex_unlock(&Net_mutx) != 0)
  {
    ds_assert(0);
  }

  if (dss_nh_saved != dss_nh)
  {
    return -1;
  }

  return 0;
}

int
dss_test_wait_for_sock_event (sint15 dss_nh)
{
  sint15 dss_nh_saved;

  if (pthread_mutex_lock(&Sock_mutx) != 0)
  {
    ds_assert(0);
  }

  Sock_mutx_dss_nh = dss_nh;

  if (!Sock_ev_pending)
  {
    if (pthread_cond_wait(&Sock_cond, &Sock_mutx) != 0)
    {
      ds_assert(0);
    }
  }

  Sock_ev_pending = 0;
  dss_nh_saved = Sock_mutx_dss_nh;

  if (pthread_mutex_unlock(&Sock_mutx) != 0)
  {
    ds_assert(0);
  }

  if (dss_nh_saved != dss_nh)
  {
    return -1;
  }

  return 0;
}

void dss_test_dss_net_cb_fcn
(
sint15            dss_nethandle,                       /* Application id */
dss_iface_id_type iface_id,                    /* Interfcae id structure */
sint15            dss_errno, /* type of network error, ENETISCONN, ENETNONET.*/
void            * net_cb_user_data               /* Call back User data  */
)
{
  dss_test_log("In dss_test_dss_net_cb_fcn: nethandle = %d, iface_id = %ld, errno = %d, net_cb_user_data = %d\n",
               dss_nethandle, iface_id, dss_errno, (int) net_cb_user_data);

  if (pthread_mutex_lock(&Net_mutx) < 0)
  {
    ds_assert(0);
  }

  Net_mutx_dss_nh = dss_nethandle;
  Net_ev_pending = 1;

  if (pthread_cond_signal(&Net_cond) < 0)
  {
    ds_assert(0);
  }

  if (pthread_mutex_unlock(&Net_mutx) < 0)
  {
    ds_assert(0);
  }

  return;
}

void dss_test_dss_sock_cb_fcn
(
sint15 dss_nethandle,                                  /* Application id */
sint15 sockfd,                                      /* socket descriptor */
uint32 event_mask,                                     /* Event occurred */
void * sock_cb_user_data       /* User data specfied during registration */
)
{
  dss_test_log("In dss_test_dss_sock_cb_fcn: nethandle = %d, sockfd = %d, event_mask = %ld, sock_cb_user_data = %d\n",
               dss_nethandle, sockfd, event_mask, (int) sock_cb_user_data);

  if (pthread_mutex_lock(&Sock_mutx) < 0)
  {
    ds_assert(0);
  }

  Sock_mutx_dss_nh = dss_nethandle;
  Sock_ev_pending = 1;

  if (pthread_cond_signal(&Sock_cond) < 0)
  {
    ds_assert(0);
  }

  if (pthread_mutex_unlock(&Sock_mutx) < 0)
  {
    ds_assert(0);
  }

  return;
}

int 
dss_test_dss_init_net_policy_info (dss_net_policy_info_type * net_policy)
{
  dss_init_net_policy_info(net_policy);

  return 0;
}

int 
dss_test_dsnet_set_policy 
(
sint15 dss_nh, 
dss_net_policy_info_type * net_policy,
sint15 dss_status_val,
sint15 dss_errno_val
)
{
  sint15 dss_status;
  sint15 dss_errno;

  dss_status = dsnet_set_policy(dss_nh, net_policy, &dss_errno);

  if (dss_status != dss_status_val)
  {
    return -1;
  }

  if (dss_status == DSS_SUCCESS)
  {
    return 0;
  }

  if (dss_errno_val == dss_errno)
  {
    return 0;
  }

  return -1;
}

int
dss_test_dsnet_get_handle (dss_net_policy_info_type * net_policy, sint15 * dss_nh)
{
  sint15 nh;
  sint15 dss_errno;

  nh = dsnet_get_handle(&dss_test_dss_net_cb_fcn, (void *)0,
                        &dss_test_dss_sock_cb_fcn, (void *)0,
                        net_policy, &dss_errno);

  if (nh == DSS_ERROR)
  {
    return -1;
  }

  *dss_nh = nh;

  return 0;
}

int
dss_test_ex_dsnet_get_handle 
(
dss_net_policy_info_type * net_policy, 
sint15 * dss_nh, 
sint15 dss_status_val, 
sint15 dss_errno_val
)
{
  sint15 nh;
  sint15 dss_errno;

  nh = dsnet_get_handle(&dss_test_dss_net_cb_fcn, (void *)0,
                        &dss_test_dss_sock_cb_fcn, (void *)0,
                        net_policy, &dss_errno);

  *dss_nh = nh;

  if (dss_status_val == DSS_SUCCESS)
  {
    if (nh != DSS_ERROR)
    {
      return 0;
    }
    else
    {
      return -1;
    }
  }

  if (nh == DSS_ERROR)
  {
    if (dss_errno_val == dss_errno)
    {
      return 0;
    }
    else
    {
      return -1;
    }
  }

  return -1;
}

int
dss_test_dsnet_release_handle (sint15 dss_nh)
{
  sint15 dss_errno;
  int rval;

  rval = dsnet_release_handle(dss_nh, &dss_errno);

  if (rval == DSS_ERROR)
  {
    return -1;
  }

  return 0;
}

#ifndef FEATURE_DS_NO_DCM

int
dss_test_dsnet_start (sint15 dss_nh)
{
  int rval;
  sint15 dss_errno;

  rval = dsnet_start(dss_nh, &dss_errno);

  if (rval == DSS_SUCCESS)
  {
    return 0;
  }

  if (dss_errno != DS_EWOULDBLOCK)
  {
    return -1;
  }

  return 0;
}

int 
dss_test_dss_netstatus (sint15 dss_nh, sint15 dss_errno_val)
{
  sint15 dss_errno;
  int rval;

  rval = dss_netstatus(dss_nh, &dss_errno);

  if (rval != DSS_ERROR)
  {
    return -1;
  }

  if (dss_errno != dss_errno_val)
  {
    return -1;
  }

  return 0;
}

int
dss_test_dss_get_iface_id (sint15 dss_nh, dss_iface_id_type * if_id, sint15 dss_status_val)
{
  dss_iface_id_type iface_id;

  iface_id = dss_get_iface_id(dss_nh);

  if (if_id != NULL)
  {
    *if_id = iface_id;
  }

  if (dss_status_val == DSS_SUCCESS)
  {
    if (iface_id != DSS_IFACE_INVALID_ID)
    {
      return 0;
    }
  }
  else
  {
    if (iface_id == DSS_IFACE_INVALID_ID)
    {
      return 0;
    }
  }

  return -1;
}

int 
dss_test_dss_get_iface_id_by_policy
(
const dss_net_policy_info_type * net_policy, 
dss_iface_id_type * if_id, 
sint15 dss_status_val,
sint15 dss_errno_val
)
{
  dss_iface_id_type iface_id;
  sint15 dss_errno;

  iface_id = dss_get_iface_id_by_policy(*net_policy, &dss_errno);

  if (if_id)
  {
    *if_id = iface_id;
  }

  if (dss_status_val == DSS_SUCCESS)
  {
    if (iface_id != DSS_IFACE_INVALID_ID)
    {
      return 0;
    }
  }
  else
  {
    if ((iface_id == DSS_IFACE_INVALID_ID) && 
        (dss_errno_val == dss_errno))
    {
      return 0;
    }
  }

  return -1;
}

int 
dss_test_dss_iface_ioctl_get_iface_name
(
dss_iface_id_type        iface_id,
dss_iface_name_enum_type iface_name,
sint15                   dss_status_val,
sint15                   dss_errno_val
)
{
  sint15 dss_errno;
  int rval;
  dss_iface_name_enum_type if_name;

  rval = dss_iface_ioctl(iface_id, DSS_IFACE_IOCTL_GET_IFACE_NAME, &if_name, &dss_errno);
  dss_test_log("if_name:%d, iface_name:%d", if_name, (int)iface_name);
  if ((rval == 0) && (dss_status_val == DSS_SUCCESS))
  {
    if (iface_name == if_name)
    {
      return 0;
    }
  }
  else if ((rval != 0) && (dss_status_val == DSS_ERROR))
  {
    if (dss_errno_val == dss_errno)
    {
      return 0;
    }
  }

  return -1;
}

int
dss_test_dss_iface_ioctl_get_state
(
dss_iface_id_type           iface_id,
dss_iface_ioctl_state_type  iface_state,
sint15                      dss_status_val,
sint15                      dss_errno_val
)
{
  sint15 dss_errno;
  int rval;
  dss_iface_ioctl_state_type if_state;

  rval = dss_iface_ioctl(iface_id, DSS_IFACE_IOCTL_GET_STATE, &if_state, &dss_errno);

  if ((rval == 0) && (dss_status_val == DSS_SUCCESS))
  {
    if (iface_state == if_state)
    {
      return 0;
    }
  }
  else if ((rval != 0) && (dss_status_val == DSS_ERROR))
  {
    if (dss_errno_val == dss_errno)
    {
      return 0;
    }
  }

  return -1;
}

int 
dss_test_dss_iface_ioctl_reg_event_cb
(
dss_iface_id_type            iface_id,
dss_iface_ioctl_ev_cb_type * iface_ev_cb,
sint15                       dss_status_val,
sint15                       dss_errno_val
)
{
  sint15 dss_errno;
  int rval;

  rval = dss_iface_ioctl(iface_id, DSS_IFACE_IOCTL_REG_EVENT_CB, iface_ev_cb, &dss_errno);

  if ((rval == 0) && (dss_status_val == DSS_SUCCESS))
  {
    return 0;
  }
  else if ((rval != 0) && (dss_status_val == DSS_ERROR))
  {
    if (dss_errno_val == dss_errno)
    {
      return 0;
    }
  }

  return -1;
}

int 
dss_test_dss_iface_ioctl_dereg_event_cb
(
dss_iface_id_type            iface_id,
dss_iface_ioctl_ev_cb_type * iface_ev_cb,
sint15                       dss_status_val,
sint15                       dss_errno_val
)
{
  sint15 dss_errno;
  int rval;

  rval = dss_iface_ioctl(iface_id, DSS_IFACE_IOCTL_DEREG_EVENT_CB, iface_ev_cb, &dss_errno);

  if ((rval == 0) && (dss_status_val == DSS_SUCCESS))
  {
    return 0;
  }
  else if ((rval != 0) && (dss_status_val == DSS_ERROR))
  {
    if (dss_errno_val == dss_errno)
    {
      return 0;
    }
  }

  return -1;
}

int 
dss_test_dsnet_start_sync (sint15 dss_nh, int dss_status_val, sint15 dss_errno_val)
{
  int dss_status;
  sint15 dss_errno;

  dss_status = dsnet_start(dss_nh, &dss_errno);

  if ((dss_status == DSS_SUCCESS) || (dss_errno != DS_EWOULDBLOCK))
  {
    goto done;
  }

  if (dss_status_val == dss_status)
  {
    goto done;
  }

  while (1)
  {
    if (dss_test_wait_for_net_event(dss_nh) < 0)
    {
      ds_assert(0);
    }
    dss_status = dss_netstatus(dss_nh, &dss_errno);

    if (dss_status != DSS_ERROR)
    {
      goto error;
    }

    if ((dss_status_val == DSS_SUCCESS) && (dss_errno == DS_ENETINPROGRESS))
    {
      continue;
    }
    else
    {
      if (dss_errno == DS_ENETISCONN)
      {
        dss_status = DSS_SUCCESS;
      }
      break;
    }
  }

  done:

  if ((dss_status == dss_status_val) && 
      ((dss_status == DSS_SUCCESS) || (dss_errno_val == dss_errno)))
  {
    return 0;
  }

  error:
  return -1;
}

int 
dss_test_dsnet_stop_sync (sint15 dss_nh, int dss_status_val, sint15 dss_errno_val)
{
  int dss_status;
  sint15 dss_errno;

  dss_status = dsnet_stop(dss_nh, &dss_errno);

  if ((dss_status == DSS_SUCCESS) || (dss_errno != DS_EWOULDBLOCK))
  {
    goto done;
  }

  if (dss_status_val == dss_status)
  {
    goto done;
  }

  while (1)
  {
    if (dss_test_wait_for_net_event(dss_nh) < 0)
    {
      ds_assert(0);
    }
    dss_status = dss_netstatus(dss_nh, &dss_errno);

    if (dss_status != DSS_ERROR)
    {
      goto error;
    }

    if ((dss_status_val == DSS_SUCCESS) && (dss_errno == DS_ENETCLOSEINPROGRESS))
    {
      continue;
    }
    else
    {
      if (dss_errno == DS_ENETNONET)
      {
        dss_status = DSS_SUCCESS;
      }
      break;
    }
  }

  done:

  if ((dss_status == dss_status_val) && 
      ((dss_status == DSS_SUCCESS) || (dss_errno_val == dss_errno)))
  {
    return 0;
  }

  error:
  return -1;
}

int 
dss_test_match_nh_ifaces(sint15 dss_nh_1, sint15 dss_nh_2, sint15 dss_status_val)
{
  dss_iface_id_type if_1, if_2;

  if_1 = dss_get_iface_id(dss_nh_1);
  if_2 = dss_get_iface_id(dss_nh_2);

  if ((if_1 == DSS_IFACE_INVALID_ID) || (if_2 == DSS_IFACE_INVALID_ID))
  {
    return -1;
  }

  if (dss_status_val == DSS_SUCCESS)
  {
    if (if_1 == if_2)
    {
      return 0;
    }
  }
  else
  {
    if (if_1 != if_2)
    {
      return 0;
    }
  }

  return -1;
}

int 
dss_test_match_nh_ifaces_3
(
sint15 dss_nh_1, 
sint15 dss_nh_2, 
sint15 dss_nh_3, 
sint15 dss_status_val
)
{
  dss_iface_id_type if_1, if_2, if_3;
  sint15 all_same = DSS_ERROR;

  if_1 = dss_get_iface_id(dss_nh_1);
  if_2 = dss_get_iface_id(dss_nh_2);
  if_3 = dss_get_iface_id(dss_nh_3);

  if ((if_1 == DSS_IFACE_INVALID_ID) || 
      (if_2 == DSS_IFACE_INVALID_ID) ||
      (if_3 == DSS_IFACE_INVALID_ID))
  {
    return -1;
  }

  if ((if_1 == if_2) && (if_2 == if_3) && (if_1 == if_3))
  {
    all_same = DSS_SUCCESS;
  }

  if (dss_status_val == all_same)
  {
    return 0;
  }

  return -1;
}

#endif /* !FEATURE_DS_NO_DCM */

#ifndef FEATURE_DSS_NO_DSSOCK

int 
dss_test_dss_socket 
(
sint15 dss_nh,
byte   family,
byte   type,
byte   protocol,
int    dss_status_val,
sint15 dss_errno_val,
sint15 * dss_fd
)
{
  sint15 dss_errno;
  sint15 dss_sfd;
  int dss_status;

  dss_sfd = dss_socket(dss_nh, family, type, protocol, &dss_errno);

  dss_status = dss_sfd;
  if (dss_sfd != DSS_ERROR)
  {
    dss_status = DSS_SUCCESS;
    *dss_fd = dss_sfd;
  }

  if ((dss_status == dss_status_val) && 
      ((dss_status == DSS_SUCCESS) || (dss_errno_val == dss_errno)))
  {
    return 0;
  }

  dss_test_log("dss_test_dss_socket failed!\n");
  return -1;
}

int 
dss_test_dss_close (sint15 dss_fd, int dss_status_val, sint15 dss_errno_val)
{
  int dss_status;
  sint15 dss_errno;

  dss_status = dss_close(dss_fd, &dss_errno);

  if ((dss_status == dss_status_val) && 
      ((dss_status == DSS_SUCCESS) || (dss_errno_val == dss_errno)))
  {
    return 0;
  }

  return -1;
}

int 
dss_test_dss_connect_sync 
(
sint15 dss_nh, 
sint15 dss_fd, 
int dss_status_val, 
sint15 dss_errno_val
)
{
  int dss_status;
  sint15 dss_errno;
  sint15 dss_sfd;
  struct sockaddr_in saddr_to;
  sint31 event;

  dss_sfd = dss_fd;
  dss_test_get_srvr_addr(&saddr_to);

  while (1)
  {
    dss_status = dss_connect(dss_sfd, (struct sockaddr *)&saddr_to, sizeof(struct sockaddr_in), &dss_errno);

    dss_test_log
    (
    "dss_test_dss_connect_sync: dss_connect returned status %d, errno %d\n",
    dss_status, dss_errno
    );

    if ((dss_status == DSS_ERROR) && (dss_errno == DS_EISCONN))
    {
      dss_status = DSS_SUCCESS;
    }

    if ((dss_status == DSS_SUCCESS) || (dss_errno != DS_EWOULDBLOCK))
    {
      break;
    }

    if (dss_status_val == dss_status)
    {
      dss_test_log
      (
      "dss_test_dss_connect_sync: dss_status_val == dss_status\n"
      );
      break;
    }

    if (dss_async_select(dss_sfd, DS_WRITE_EVENT, &dss_errno) != DSS_SUCCESS)
    {
      dss_test_log
      (
      "dss_test_dss_connect_sync: dss_async_select failed!\n"
      );
      goto error;
    }

    if (dss_test_wait_for_sock_event(dss_nh) < 0)
    {
      dss_test_log
      (
      "dss_test_dss_connect_sync: dss_test_wait_for_sock_event failed!"
      );
      goto error;
    }

    if ((event = dss_getnextevent(dss_nh, &dss_sfd, &dss_errno)) == DSS_ERROR)
    {
      dss_test_log
      (
      "dss_test_dss_connect_sync: dss_getnextevent failed!"
      );
      goto error;
    }

    if (event != DS_WRITE_EVENT)
    {
      dss_test_log
      (
      "dss_test_dss_connect_sync: event != DS_WRITE_EVENT"
      );
      goto error;
    }
  }

  if ((dss_status == dss_status_val) && 
      ((dss_status == DSS_SUCCESS) || (dss_errno_val == dss_errno)))
  {
    return 0;
  }

  error:
  return -1;
}

int
dss_test_tcp_client_open (sint15 dss_nh, sint15 * dss_fd)
{
  int rval;
  sint15 dss_sfd;

  rval = dss_test_dss_socket
         (
         dss_nh, 
         AF_INET, 
         SOCK_STREAM, 
         IPPROTO_TCP, 
         DSS_SUCCESS,
         0,
         &dss_sfd
         );

  if (rval < 0)
  {
    dss_test_log("dss_test_tcp_client_open failed!\n");
    return -1;
  }

  *dss_fd = dss_sfd;
  return 0;
}

int
dss_test_udp_client_open (sint15 dss_nh, sint15 * dss_fd)
{
  int rval;
  sint15 dss_sfd;

  rval = dss_test_dss_socket
         (
         dss_nh, 
         AF_INET, 
         SOCK_DGRAM, 
         IPPROTO_UDP, 
         DSS_SUCCESS,
         0,
         &dss_sfd
         );

  if (rval < 0)
  {
    dss_test_log("dss_test_udp_client_open failed!\n");
    return -1;
  }

  *dss_fd = dss_sfd;
  return 0;
}

int 
dss_test_tcp_client_close (sint15 dss_fd)
{
  int rval;

  rval = dss_test_dss_close(dss_fd, DSS_SUCCESS, 0);

  return rval;
}

int 
dss_test_udp_client_close (sint15 dss_fd)
{
  int rval;

  rval = dss_test_dss_close(dss_fd, DSS_SUCCESS, 0);

  return rval;
}
#endif/*FEATURE_DSS_NO_DSSOCK*/

int 
dss_test_fread_data_buf (FILE * fp, char * buf, int * lbuf)
{
  *lbuf = fread(buf, 1, *lbuf, fp);

  return *lbuf;
}

int 
dss_test_fwrite_data_buf_complete (FILE * fp, char * buf, int lbuf)
{
  int nwrote = 0; 

  while (lbuf)
  {
    nwrote = fwrite(buf, 1, lbuf, fp);
    if (nwrote <= 0)
    {
      nwrote = -1;
      goto error;
    }

    lbuf -= nwrote;
    ds_assert(lbuf >= 0);

    buf += nwrote;
  }

  fflush(fp);

  error:
  return nwrote;
}

#ifndef FEATURE_DSS_NO_DSSOCK
int
dss_test_write_buf_complete 
(
sint15 dss_nh,
sint15 dss_fd, 
char * buf, 
int    lbuf
)
{
  int nbytes;
  int index = 0;
  sint15 dss_errno;
  int rval = -1;
  sint31 event;

  ds_assert(dss_fd != 0);

  while (lbuf)
  {
    nbytes = dss_write
             (
             dss_fd, 
             buf + index,
             lbuf, 
             &dss_errno
             );

    if (nbytes == DSS_ERROR)
    {
      if (dss_errno == DS_EWOULDBLOCK)
      {
        if (dss_async_select(dss_fd, DS_WRITE_EVENT, &dss_errno) != DSS_SUCCESS)
        {
          goto error;
        }
        if (dss_test_wait_for_sock_event(dss_nh) < 0)
        {
          goto error;
        }
        if ((event = dss_getnextevent(dss_nh, &dss_fd, &dss_errno)) == DSS_ERROR)
        {
          goto error;
        }
        if (event != DS_WRITE_EVENT)
        {
          goto error;
        }
      }
      else
      {
        ds_assert(0);
      }
      continue;
    }
    lbuf -= nbytes;
    ds_assert(lbuf >= 0);
    index += nbytes;
  }

  rval = 0;

  error:
  return rval;
}

int
dss_test_sendto_buf_complete 
(
sint15 dss_nh,
sint15 dss_fd, 
struct sockaddr_in * saddr_to,
char * buf, 
int    lbuf
)
{
  int nbytes;
  int index = 0;
  sint15 dss_errno;
  int rval = -1;
  sint31 event;

  ds_assert(dss_fd != 0);

  while (lbuf)
  {
    nbytes = dss_sendto
             (
             dss_fd, 
             buf + index,
             lbuf, 
             0, 
             (struct sockaddr *)saddr_to,
             sizeof(struct sockaddr_in),
             &dss_errno
             );

    if (nbytes == DSS_ERROR)
    {
      if (dss_errno == DS_EWOULDBLOCK)
      {
        if (dss_async_select(dss_fd, DS_WRITE_EVENT, &dss_errno) != DSS_SUCCESS)
        {
          goto error;
        }
        if (dss_test_wait_for_sock_event(dss_nh) < 0)
        {
          goto error;
        }
        if ((event = dss_getnextevent(dss_nh, &dss_fd, &dss_errno)) == DSS_ERROR)
        {
          goto error;
        }
        if (event != DS_WRITE_EVENT)
        {
          goto error;
        }
      }
      else
      {
        ds_assert(0);
      }
      continue;
    }
    lbuf -= nbytes;
    ds_assert(lbuf >= 0);
    index += nbytes;
  }

  rval = 0;

  error:
  return rval;
}

int 
dss_test_tcp_client_send_file (sint15 dss_nh, sint15 dss_fd)
{
  int rval = -1;
  int bufsz;
  FILE * dfp;
  char * wbuf;
  int lbuf;

  bufsz = dss_test_get_bufsize();
  dfp = dss_test_open_data_file(stdin, "r");
  wbuf = malloc(bufsz);
  if(!wbuf)
    return rval;
  lbuf = bufsz;
  while (dss_test_fread_data_buf(dfp, wbuf, &lbuf) > 0)
  {
    if (dss_test_write_buf_complete(dss_nh, dss_fd, wbuf, lbuf) < 0)
    {
      goto error;
    }
    lbuf = bufsz;
  }

  rval = 0;

  error:
  free(wbuf);
  return rval;
}

int 
dss_test_udp_client_send_file (sint15 dss_nh, sint15 dss_fd)
{
  int rval = -1;
  int bufsz;
  FILE * dfp;
  char * wbuf;
  int lbuf;
  struct sockaddr_in saddr_to;
  int time_wait;

  bufsz = dss_test_get_bufsize();
  dfp = dss_test_open_data_file(stdin, "r");
  wbuf = malloc(bufsz);
  if(!wbuf)
    return rval;
  time_wait = dss_test_get_time_wait();
  time_wait *= 1000;
  dss_test_get_srvr_addr(&saddr_to);

  lbuf = bufsz;
  while (dss_test_fread_data_buf(dfp, wbuf, &lbuf) > 0)
  {
    if (dss_test_sendto_buf_complete(dss_nh, dss_fd, &saddr_to, wbuf, lbuf) < 0)
    {
      goto error;
    }
    if (time_wait != 0)
    {
      usleep(time_wait);
    }
    lbuf = bufsz;
  }

  rval = 0;

  error:
  free(wbuf);
  return rval;
}

#endif /*FEATURE_DSS_NO_DSSOCK*/

dss_test_peer_mode_t
dss_test_peer_get_mode (void)
{
  dss_test_peer_mode_t mode = DSS_TEST_PEER_MODE_SRVR;

  if (dss_test_find_prog_args('c', NULL, &Prog_args[0], ARR_SIZ(Prog_args)) != NULL)
  {
    mode = DSS_TEST_PEER_MODE_CLNT;
  }
  return mode;
}

int 
dss_test_peer_get_transport_mode (void)
{
  int mode = IPPROTO_UDP;

  if (dss_test_find_prog_args('t', NULL, &Prog_args[0], ARR_SIZ(Prog_args)) != NULL)
  {
    mode = IPPROTO_TCP;
  }

  return mode;
}

int 
dss_test_peer_srvr_open (int * fd, int sock_mode, int transport_mode)
{
  int rval = -1;
  struct sockaddr_in saddr;

  if ((*fd = socket(AF_INET, sock_mode, transport_mode)) < 0)
  {
    perror("socket call returned error!\n");
    goto error;
  }

  dss_test_get_srvr_addr(&saddr);

  if (bind(*fd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
  {
    perror("error in bind!\n");
    goto error;
  }

  rval = 0;

  error:
  return rval;
}

int 
dss_test_peer_clnt_open (int * fd, int sock_mode, int transport_mode)
{
  int rval = -1;
  struct sockaddr_in saddr;

  if ((*fd = socket(AF_INET, sock_mode, transport_mode)) < 0)
  {
    perror("socket call returned error!\n");
    goto error;
  }

  dss_test_get_srvr_addr(&saddr);

  if (connect(*fd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
  {
    perror("error in connect!\n");
    goto error;
  }

  rval = 0;

  error:
  return rval;
}

int 
dss_test_peer_srvr_accept (int fd, int * cfd)
{
  int rval = -1;
  struct sockaddr_in saddr_clnt;
  socklen_t saddr_clnt_len;

  if ((*cfd = accept(fd, (struct sockaddr *)&saddr_clnt, &saddr_clnt_len)) < 0)
  {
    perror("error in accept!\n");
    goto error;
  }

  rval = 0;

  error:
  return rval;
}

int 
dss_test_peer_read_buf (int fd, char * buf, int bufsz) 
{
  int nread;

  nread = read(fd, buf, bufsz);

  return nread;
}

int 
dss_test_peer_write_buf (int fd, const char * buf, int len) 
{
  int nwrote;

  nwrote = write(fd, buf, len);

  return nwrote;
}

int 
dss_test_peer_read_buf_complete (int fd, char * buf, size_t lmsg)
{
  int nread = 0;
  size_t rem;

  rem = lmsg;

  for (;;)
  {
    nread = read(fd, buf, rem);

    if (nread < 0)
    {
      perror("ds_read_msg: error in read: ");
      continue;
    }

    if (nread == 0)
    {
      break;
    }

    ds_assert((size_t)nread <= rem);
    rem -= (size_t)nread;
    buf += nread;

    if (rem == 0)
    {
      break;
    }
  }

  return(int)(lmsg - rem);
}

int 
dss_test_peer_write_buf_complete (int fd, const char * buf, size_t lmsg)
{
  int nwrite = 0;
  size_t rem;

  rem = lmsg;

  for (;;)
  {
    nwrite = write(fd, buf, rem);

    if (nwrite < 0)
    {
      perror("ds_write_msg: error in write: ");
      break;
    }

    if (nwrite == 0)
    {
      continue;
    }

    ds_assert((size_t)nwrite <= rem);
    rem -= (size_t)nwrite;
    buf += nwrite;

    if (rem == 0)
    {
      break;
    }
  }

  return(int)(lmsg - rem);
}

void
dss_test_peer_srvr_udp (void)
{
  int fd;
  char * buf;
  int bufsz;
  int nread;
  FILE * fp;
  int nwrote;

  bufsz = dss_test_get_bufsize();
  buf = malloc(bufsz);
  ds_assert(buf);

  fp = dss_test_open_data_file(stdout, "w");

  if (dss_test_peer_srvr_open(&fd, SOCK_DGRAM, IPPROTO_UDP) < 0)
  {
    goto error;
  }

  while (1)
  {
    nread = dss_test_peer_read_buf(fd, buf, bufsz);
    if (nread <= 0)
    {
      goto error;
    }

    if ((nwrote = dss_test_fwrite_data_buf_complete(fp, buf, nread)) != nread)
    {
      goto error;
    }
  }

  error:
  free(buf);
  return;
}

void
dss_test_peer_srvr_tcp (void)
{
  int fd, rfd;
  char * buf;
  int bufsz;
  int nread;
  FILE * fp;
  int nwrote;

  bufsz = dss_test_get_bufsize();
  buf = malloc(bufsz);
  ds_assert(buf);

  fp = dss_test_open_data_file(stdout, "w");

  if (dss_test_peer_srvr_open(&fd, SOCK_STREAM, IPPROTO_TCP) < 0)
  {
    goto error;
  }

  if (listen(fd, 1) < 0)
  {
    perror("error in listen!\n");
    goto error;
  }

  for (;;)
  {
    if (dss_test_peer_srvr_accept(fd, &rfd) < 0)
    {
      goto error;
    }

    while (1)
    {
      nread = dss_test_peer_read_buf(rfd, buf, bufsz);
      if (nread <= 0)
      {
        break;
      }

      if ((nwrote = dss_test_fwrite_data_buf_complete(fp, buf, nread)) != nread)
      {
        break;
      }
    }

    close(rfd);
  }

  error:
  free(buf);
  return;
}

void
dss_test_peer_srvr (void)
{
  int transport;

  transport = dss_test_peer_get_transport_mode();

  if (transport == IPPROTO_UDP)
  {
    dss_test_peer_srvr_udp();
  }
  else if (transport == IPPROTO_TCP)
  {
    dss_test_peer_srvr_tcp();
  }

  return;
}

void
dss_test_peer (void)
{
  dss_test_peer_mode_t mode;

  mode = dss_test_peer_get_mode();

  if (mode == DSS_TEST_PEER_MODE_SRVR)
  {
    dss_test_peer_srvr();
  }

  return;
}

int 
dss_test_str_isascii (char * str, int len)
{
  int i; 

  for (i = 0; i < len; ++i)
  {
    if (isascii(*(str++)) == 0)
    {
      return -1;
    }
  }

  return 0;
}

int
dss_test_master_run_script (char * buf, int bufsz, int len)
{
  int retval = -1;
  char * cmd;
  int cmdsz;
  int cmdlen;
  (void)bufsz;

  cmd = malloc(cmdsz = dss_test_get_bufsize());
  ds_assert(cmd);

  if (dss_test_str_isascii(buf, len) < 0)
  {
    goto error;
  }

  cmdlen = snprintf(cmd, cmdsz, "%s -c \"%s\"", 
                    (char*)dss_test_get_master_script(), buf);
  ds_assert(cmdlen < cmdsz);

  if (system(cmd) == -1)
  {
    goto error;
  }

  retval = 0;

  error:
  free(cmd);
  return retval;
}

void
dss_test_master (void)
{
  int fd;
  char * buf;
  int bufsz;
  int nread;

  bufsz = dss_test_get_bufsize();
  buf = malloc(bufsz);
  ds_assert(buf);

  if (dss_test_peer_srvr_open(&fd, SOCK_DGRAM, IPPROTO_UDP) < 0)
  {
    goto error;
  }

  while (1)
  {
    memset(buf, 0, bufsz);

    nread = dss_test_peer_read_buf(fd, buf, bufsz);
    if (nread <= 0)
    {
      goto error;
    }

    if (dss_test_master_run_script(buf, bufsz, nread) < 0)
    {
      goto error;
    }
  }

  error:
  free(buf);
  return;
}

int
dss_test_master_client_send_cmd (const char * msg, int msglen)
{
  int fd;
  int nwrote;
  int retval = -1;

  if (dss_test_peer_clnt_open(&fd, SOCK_DGRAM, IPPROTO_UDP) < 0)
  {
    goto error;
  }

  nwrote = dss_test_peer_write_buf(fd, msg, msglen);

  if (nwrote != msglen)
  {
    goto error;
  }

  retval = 0;

  error:
  close(fd);
  return retval;
}

void 
dss_test_master_client (void)
{
  const char * cmd; 

  cmd = dss_test_get_command();

  if (cmd == NULL)
  {
    return;
  }

  (void)dss_test_master_client_send_cmd(cmd, strlen(cmd));
  return;
}
