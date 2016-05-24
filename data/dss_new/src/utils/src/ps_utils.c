/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                           P S _ U T I L S . C

GENERAL DESCRIPTION
  Collection of utility functions being used by various modules in PS.
  Most of these functions assume that the caller is in PS task context.

Copyright (c) 1995-2011 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $PVCSPath: L:/src/asw/MM_DATA/vcs/ps_utils.c_v   1.0   08 Aug 2002 11:19:58   akhare  $
  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ps_utils.c#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
05/20/11    am     Dyn buffer for extra \n in EFS create.
10/26/10    op     Added functions to use EFS items.
11/21/08    pp     Lint fixes.
09/07/07    scb    Added REX signal ext code under FEATURE_REX_SIGS_EXT
11/02/06    mct    Added 64 bit random number generator function.
02/22/06    msr    Using single critical section
10/31/04   msr/ks  Added memdump().
04/30/04    mct    Fixed some lint errors.
08/05/02    usb    Moved lcsum() from this file to ps_iputil.c
08/05/02    usb    Fixed get/set nv functions, moved them out of FEATURE_MIP
07/31/02    usb    Renamed the file from psmisc.c
06/14/02    usb    Removed byte manipulation functions.  Use dsbyte.h now.
04/17/02    rc     Wrapped code in !FEATURE_DATA_WCDMA_PS
12/21/01    dwp    Wrap get16 in !FEATURE_DATA_PS as it is defined else where
                   in MSM5200 archive.
11/12/01    dwp    Add "|| FEATURE_DATA_PS" to whole module.
05/24/00    hcg    Added TARGET_OS_SOLARIS to compile with Solaris DSPE.
04/21/00    mvl    Fixed a #define so compiles properly under COMET
01/09/99    jjw    Changed to generic Browser interface
10/27/98    ldg    For T_ARM included C version of TCP checksum routine.
06/16/98    na     Converted the routine that calculates the TCP checksum
                   into 186 assembly.
06/25/97    jgr    Added ifdef FEATURE_DS over whole file
07/22/95    jjw    Created Initial Version
===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "customer.h"       /* Customer Specific Features */

#ifdef FEATURE_DATA_PS
#include "msg.h"
#include "err.h"
#include "amssassert.h"
#include "ps_crit_sect.h"

#ifndef FEATURE_WINCE
#include "task.h"
#endif /* FEATURE_WINCE */

#include "ps_utils.h"
#include "ran.h"

#if (TG == T_PC)
#include "time.h"
#else
#include "qw.h"
#include "time_svc.h"
#endif

#include "ds_Utils_DebugMsg.h"
#include "ps_system_heap.h"
#include "AEEstd.h"

/*===========================================================================
                              MACRO DEFINITIONS
===========================================================================*/
/*---------------------------------------------------------------------------
  Macro to prevent lint warning 818 'Symbol could be declared as pointing to
   const'
---------------------------------------------------------------------------*/
#ifndef PS_ARG_NOT_CONST
#define PS_ARG_NOT_CONST(arg) /*lint -save -e717 -e506 -e774 */ (arg)=(arg);/*lint -restore*/
#endif

/*===========================================================================

                      LOCAL DECLARATIONS FOR MODULE

===========================================================================*/
/*---------------------------------------------------------------------------
  Command item to NV.
---------------------------------------------------------------------------*/
static nv_cmd_type  ps_nv_cmd_buf;


/*===========================================================================

                      DEFINITIONS FOR MODULE

===========================================================================*/

/*===========================================================================

FUNCTION PS_GET_NV_ITEM

DESCRIPTION
  This function retrieves the specified item from NV.

DEPENDENCIES
  The NV task has been started and is running.
  This functions is Non-reentrant.

RETURN VALUE
  Status returned from the NV read request.  An LOG_MSG_FATAL_ERROR is logged
  if status is other than:
    NV_DONE_S       - request done
    NV_NOTACTIVE_S  - item was not active

SIDE EFFECTS
  While this function is running all other PS task activities are
  suspended except for watchdog kicking, and until the NV item is
  read in.

===========================================================================*/

nv_stat_enum_type ps_get_nv_item
(
  nv_items_enum_type  item_code,       /* Item to get                      */
  nv_item_type        *data_ptr        /* Pointer where to put the item    */
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ASSERT( rex_self() == &ps_tcb );

  /*-------------------------------------------------------------------------
    Prepare command buffer to get the item from NV.
  -------------------------------------------------------------------------*/
  ps_nv_cmd_buf.cmd        = NV_READ_F;             /* Read request        */
  ps_nv_cmd_buf.tcb_ptr    = &ps_tcb;               /* Notify this task    */
  ps_nv_cmd_buf.sigs       = 1 << (rex_sigs_type)PS_NV_CMD_SIGNAL; /* With this signal    */
  ps_nv_cmd_buf.done_q_ptr = NULL;             /* Do not enqueue when done */
  ps_nv_cmd_buf.item       = item_code;             /* Item to get         */
  ps_nv_cmd_buf.data_ptr   = data_ptr;              /* Where to return it  */

  /*-------------------------------------------------------------------------
   Clear signal, issue the command, and wait for the response.
  -------------------------------------------------------------------------*/
  PS_CLR_SIGNAL( PS_NV_CMD_SIGNAL );               /* Clear signal for NV  */
  nv_cmd( &ps_nv_cmd_buf );                        /* Issue the request    */
  (void)ps_wait((rex_sigs_type) 1 <<
                       (rex_sigs_type)PS_NV_CMD_SIGNAL ); /* Wait for completion*/

  if( ps_nv_cmd_buf.status != NV_DONE_S &&
      ps_nv_cmd_buf.status != NV_NOTACTIVE_S )
  {
    LOG_MSG_FATAL_ERROR( "NV Read Failed Item %d Code %d",
         ps_nv_cmd_buf.item, ps_nv_cmd_buf.status, 0 );
  }
  return( ps_nv_cmd_buf.status );
} /* ps_get_nv_item() */


/*===========================================================================
FUNCTION PS_PUT_NV_ITEM

DESCRIPTION
  Write an item to NV memory.  Wait until write is completed.

RETURN VALUE
  Status returned from the NV read request.  An LOG_MSG_FATAL_ERROR is logged 
  if status is other than:
    NV_DONE_S       - request done

DEPENDENCIES
  This function can only be called from PS task.  Also it is not
  reentrant. Shouldn't be a problem, as it doesn't exit till we're done, and
  it's only called from the PS task.

RETURN VALUE
  Status returned from the NV read request.  An LOG_MSG_FATAL_ERROR is logged 
  if status is other than:
    NV_DONE_S       - request done
    NV_NOTACTIVE_S  - item was not active

SIDE EFFECTS
  While this function is running all other PS task activities are
  suspended except for watchdog kicking, and until the NV item is
  wrote down.

===========================================================================*/
nv_stat_enum_type ps_put_nv_item(
  nv_items_enum_type item_code,                              /* which item */
  nv_item_type *data_ptr                       /* pointer to data for item */
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ASSERT( rex_self() == &ps_tcb );

  /*-------------------------------------------------------------------------
    Prepare command buffer to write the item to NV.
    TODO:NV team to be notified to change the nv_cmd_buf.sigs to an array
  -------------------------------------------------------------------------*/
  ps_nv_cmd_buf.cmd        = NV_WRITE_F;            /* Write request       */
  ps_nv_cmd_buf.tcb_ptr    = &ps_tcb;               /* Notify this task    */
  ps_nv_cmd_buf.sigs       = 1 << (rex_sigs_type)PS_NV_CMD_SIGNAL; /* With this signal    */
  ps_nv_cmd_buf.done_q_ptr = NULL;             /* Do not enqueue when done */
  ps_nv_cmd_buf.item       = item_code;             /* Item to put         */
  ps_nv_cmd_buf.data_ptr   = data_ptr;              /* Data to write       */


  /*-------------------------------------------------------------------------
   Clear signal, issue the command, and wait for the response.
  -------------------------------------------------------------------------*/
  PS_CLR_SIGNAL( PS_NV_CMD_SIGNAL );
  nv_cmd( &ps_nv_cmd_buf );
  (void)ps_wait( (rex_sigs_type) 1 <<
                        (rex_sigs_type)PS_NV_CMD_SIGNAL ); /* Wait for completion*/

  if( ps_nv_cmd_buf.status != NV_DONE_S )
  {
    LOG_MSG_FATAL_ERROR( "NV Write Failed Item %d Code %d",
         ps_nv_cmd_buf.item, ps_nv_cmd_buf.status, 0 );
  }

  return( ps_nv_cmd_buf.status );
} /* ps_put_nv_item() */


/*===========================================================================

FUNCTION msclock

DESCRIPTION
  This function will return the time in milliseconds since ....

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/

#if(TG==T_PC)

dword msclock()
{
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  return( time( NULL));
} /* msclock() */

#else

dword msclock( void)
{
  qword qw_time;
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  time_get_ms( qw_time);

  return( qw_time[0]);

} /* msclock() */

#endif


/*===========================================================================
FUNCTION MEMDUMP()

DESCRIPTION
  Debug routine to dump memory to DM

PARAMETERS
  data_ptr -> address of memory to dump
  len      -> number of bytes to dump

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  Memory access may go up to 7 bytes beyond last dumped byte, which could
  cause a data abort if dumping the last bytes of RAM.

===========================================================================*/
void memdump
(
  void * data_ptr,
  int    len
)
{
  char * data = (char *) data_ptr;
  int    i;                                                /* current byte */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2 ("Dumping %d bytes @ %x", len, data,0);

  for (i=0; i < len; i+= 8)
  {
    MSG_8( MSG_SSID_DFLT,
           MSG_LEGACY_MED,
           "%02x %02x %02x %02x %02x %02x %02x %02x",
           data[i], data[i+1], data[i+2], data[i+3],
           data[i+4], data[i+5], data[i+6], data[i+7]);
  }

} /* memdump() */


/*===========================================================================
FUNCTION PS_UTILS_GENERATE_RAND_64BIT_NUM()

DESCRIPTION
  This function generates a random 64 bit number.

PARAMETERS
  *random_num - Pointer to the 64 bit number to be returned by this function.

RETURN VALUE
  None

DEPENDENCIES
  As a pre-condition, this function assumes that ran_seed() has been called
  at least once. Currently, ran_seed() is called by the main control task
  as part of initialization.

SIDE EFFECTS
  None
===========================================================================*/
void ps_utils_generate_rand_64bit_num
(
  uint64 *rand_num                 /* Pointer to the 64bit num be returned */
)
{
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifdef TEST_FRAMEWORK

  static boolean  is_seeded = FALSE;

  if (FALSE == is_seeded)
  {
    ran_seed (0x8765ABCD);
    is_seeded = TRUE;
  }

#endif

  *rand_num = ran_next();
  *rand_num = (*rand_num << 32) + ran_next();

} /* ps_utils_generate_ipv6_iid */

/*===========================================================================
FUNCTION PS_CREATE_EFS_CONFIG_FILE

DESCRIPTION
  Create a config file in EFS which stores the path of EFS item files.

DEPENDENCIES
  None

PARAMETERS
  conf_file_path - File path of config file

RETURN VALUE
  0         - Success
 -1         - Non-EFS related Failures
  efs_errno - EFS related failures. Meaning of this value can be
              found in fs_errno.h

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_create_efs_config_file
(
  const char *conf_file_path
)
{
  int32                 config_fd, result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if ( NULL == conf_file_path )
  {
    LOG_MSG_ERROR("conf_file_path is NULL", 0, 0, 0);
    return -1;
  }
  
  /*-------------------------------------------------------------------------
    Create common directories if needed.
  -------------------------------------------------------------------------*/
  LOG_MSG_INFO2("EFS: Creating conf file if necessary", 0, 0, 0);
  result = ps_path_is_directory("/nv");
  if( 0 != result )
  {
    /* Directory doesn't exist yet */
    LOG_MSG_INFO1("Create /nv dir in EFS", 0, 0, 0);
    result = efs_mkdir( "/nv", S_IREAD|S_IWRITE|S_IEXEC);
    if ( -1 == result )
    {
      LOG_MSG_ERROR("Create EFS Dir Failed: error %d", efs_errno, 0, 0);
      return efs_errno;
    } 
  }

  result = ps_path_is_directory("/nv/item_files");
  if( 0 != result )
  {
    /* Directory doesn't exist yet */
    LOG_MSG_INFO1("Create /nv/item_files dir in EFS", 0, 0, 0);
    result = efs_mkdir( "/nv/item_files", S_IREAD|S_IWRITE|S_IEXEC);
    if ( -1 == result )
    {
      LOG_MSG_ERROR("Create EFS Dir Failed: error %d", efs_errno, 0, 0);
      return efs_errno;
    }
  }

  result = ps_path_is_directory("/nv/item_files/conf");
  if( 0 != result )
  {
    /* Directory doesn't exist yet */
    LOG_MSG_INFO1("Create /nv/item_file/conf dir in EFS", 0, 0, 0);
    result = efs_mkdir( "/nv/item_files/conf", S_IREAD|S_IWRITE|S_IEXEC);
    if ( -1 == result )
    {
      LOG_MSG_ERROR("Create EFS Dir Failed: error %d", efs_errno, 0, 0);
      return efs_errno;
    }
  }

  /*-------------------------------------------------------------------------
    Open conf file. Create conf file if does not exist.
    Resulting file is truncated to zero bytes.
  -------------------------------------------------------------------------*/  
  config_fd = efs_open (conf_file_path, O_WRONLY|O_CREAT|O_TRUNC, ALLPERMS);
  if ( 0 > config_fd )
  {
    LOG_MSG_ERROR("Error creating config file, error %d", efs_errno, 0, 0);
    return efs_errno;
  }

  result = efs_close (config_fd);
  if ( 0 != result )
  {
    LOG_MSG_ERROR("Error closing config file, error %d", efs_errno, 0, 0);
    return efs_errno;
  }
  return 0;

} /* ps_create_efs_config_file() */

/*===========================================================================
FUNCTION PS_CREATE_EFS_ITEM_FILE

DESCRIPTION
  Put(append and add newline) item_file_path into conf_file_path

DEPENDENCIES
  None

PARAMETERS
  conf_file_path - File path to a specific conf file
  item_file_path - File path to item in NV
  item_file_path_size - Size of item file path

RETURN VALUE
  0         - Success
 -1         - Non-EFS related Failures
  efs_errno - EFS related failures. Meaning of this value can be
              found in fs_errno.h

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_create_efs_item_file
(
  const char   *conf_file_path,
  char         *item_file_path, 
  fs_size_t     item_file_path_size
)
{
  int32              config_fd, result;
  char              *file_loc;
  int32              ret_val = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/ 
  if ( NULL == conf_file_path || NULL == item_file_path )
  {
    LOG_MSG_ERROR("conf_file_path or item_file_path is NULL", 0, 0, 0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Open conf file. If it doesn't exist create it and reopen it.
  -------------------------------------------------------------------------*/  
  config_fd = efs_open (conf_file_path, O_RDWR|O_APPEND);
  if ( 0 > config_fd )
  {
    if ( ENOENT == efs_errno ) /* Conf file does not exist */
    {
      LOG_MSG_ERROR( "EFS: Config file not present", 0, 0, 0);
      LOG_MSG_INFO1("EFS: Creating config file", 0, 0, 0);
      result = ps_create_efs_config_file(conf_file_path);
      if ( 0 != result )
      {
        LOG_MSG_ERROR("EFS: Error creating conf file, error %d",
                      efs_errno, 0, 0);
        return efs_errno;
      }
	  
      config_fd = efs_open (conf_file_path, O_RDWR|O_APPEND);
      if ( 0 > config_fd )
      {
        LOG_MSG_ERROR("EFS: Error opening config file, error %d",
                       efs_errno, 0, 0);
        return efs_errno;
      }
    }
    else /* Could not open conf file for some other reason */
    {
      LOG_MSG_ERROR("Error opening config file, error %d",
                    efs_errno, 0, 0);
      return efs_errno;
    }
  }

  file_loc = (char*)ps_system_heap_mem_alloc(item_file_path_size + 1);
  if (NULL == file_loc)
  {
    LOG_MSG_ERROR("Out of mem, can't create file", 0, 0, 0);
    ret_val = -1;
    goto bail;
  }

  (void)std_strlcpy(file_loc, item_file_path, item_file_path_size);
  file_loc[item_file_path_size] = '\n';
  result = efs_write (config_fd, file_loc, (item_file_path_size + 1));
  if ( (item_file_path_size + 1) != result )
  {
    LOG_MSG_ERROR("Error writing into config file, error %d",
                  efs_errno, 0, 0);
    ret_val = efs_errno;
    goto bail;
  }

bail:  
  if (file_loc)
  {
    PS_SYSTEM_HEAP_MEM_FREE(file_loc);
  }

  result = efs_close (config_fd);
  if ( 0 != result )
  {
    LOG_MSG_ERROR("Error closing config file, error %d",
                  efs_errno, 0, 0);
    return efs_errno;
  }

  return ret_val;
} /* ps_create_efs_item_file() */

/*===========================================================================
FUNCTION PS_READ_EFS_NV

DESCRIPTION
  This function reads the EFS item from the item file

DEPENDENCIES
  None

PARAMETERS
  item_file_path - File path to item in NV
  nv_info - Struct for NV item(s)
  nv_info_size - Size of NV item structure

RETURN VALUE
  0         - Success
 -1         - Non-EFS related Failures
  efs_errno - EFS related failures. Meaning of this value can be
              found in fs_errno.h

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_read_efs_nv
(
  const char   *item_file_path, 
  void         *nv_info_ptr, 
  fs_size_t    nv_info_size
)
{
  int32                 retval = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO2("Reading from EFS", 0, 0, 0);

  if ( NULL == item_file_path || NULL == nv_info_ptr )
  {
    LOG_MSG_ERROR("item_file_path is NULL or nv_info_ptr is NULL",
                  0, 0, 0);
    return -1;
  }

  memset (nv_info_ptr, 0, nv_info_size);

  /*-------------------------------------------------------------------------
    Read the item file from EFS into nv_info_ptr
  -------------------------------------------------------------------------*/
  retval = efs_get( item_file_path, 
                    (void *)nv_info_ptr, 
                    nv_info_size); 

  if( 0 >= retval ) /* Error or 0 bytes read */
  {
    LOG_MSG_ERROR("Unable to read EFS item, error %d ", efs_errno, 0, 0);
    return efs_errno;
  }

  return 0;
} /* ps_read_efs_nv() */

/*===========================================================================
FUNCTION PS_WRITE_EFS_NV

DESCRIPTION
  This function writes the EFS item to the item file

DEPENDENCIES
  None

PARAMETERS
  item_file_path - File path to item in NV
  nv_info - Pointer to NV item
  nv_info_size - Size of NV item

RETURN VALUE
  0         - Success
 -1         - Non-EFS related Failures
  efs_errno - EFS related failures. Meaning of this value can be
              found in fs_errno.h

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_write_efs_nv
(
  const char   *item_file_path, 
  void         *nv_info_ptr, 
  fs_size_t    nv_info_size
)
{
  int32                 result = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO2("Writing to EFS", 0, 0, 0);

  if ( NULL == item_file_path || NULL == nv_info_ptr )
  {
    LOG_MSG_ERROR("item_file_path is NULL or nv_info_ptr is NULL",
                  0, 0, 0);
    return -1;
  }
  /*-------------------------------------------------------------------------
    Write the item into the item file. If file or file path does not exist,
    create the file path and file
  -------------------------------------------------------------------------*/
  result = efs_put( item_file_path, 
                    (void *)nv_info_ptr, 
                    nv_info_size,
                    O_CREAT|O_AUTODIR, 
                    ALLPERMS );
  if( 0 != result )
  {
    LOG_MSG_ERROR("Unable to write EFS item, error %d ", efs_errno, 0, 0);
    return efs_errno;
  }

  return 0;
} /* ps_write_efs_nv() */

/*===========================================================================
FUNCTION PS_PATH_IS_DIRECTORY

DESCRIPTION
  To check if the EFS directory exists

DEPENDENCIES
  None

PARAMETERS
  dirname - Directory path

RETURN VALUE
   0         - success
   efs_errno - EFS error
   -1        - Other error

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_path_is_directory
(
  const char *dirname
)
{
  int                           rsp;
  struct fs_stat                stat_info;
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  rsp = efs_lstat( dirname, &stat_info);
  if( 0 != rsp )
  {
    rsp = efs_errno;
    if( ENOENT != rsp )
    {
      LOG_MSG_ERROR("efs_lstat error %d", rsp, 0, 0);
      return rsp;
    }
  }
  else if( S_ISDIR (stat_info.st_mode))
  {
    return 0;
  }

  return -1;
} /* ps_path_is_directory */

/*===========================================================================
FUNCTION      PS_EFS_FILE_INIT

DESCRIPTION   The function intializes the state machine and 
              also opens the file

DEPENDENCIES  None.

RETURN VALUE   0  : SUCCESS: The file is good, readable, 
                             State Machine Initialized.
              -1 : FAILURE: The file cannot be opened/ readable. 

SIDE EFFECTS  None.
===========================================================================*/
int ps_efs_file_init
( 
  const char *file_path, 
  ps_efs_token_type *sm
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*-------------------------------------------------------------------------
    Sanity checks on input parameters
  -------------------------------------------------------------------------*/
  if((NULL == file_path) || (NULL == sm))
  {
    LOG_MSG_INFO1("Input parameters are NULL!", 0, 0, 0);
    return -1;
  }
  
  /*-------------------------------------------------------------------------
    Initialize the structure variables and open the file in read mode.
  -------------------------------------------------------------------------*/    
  sm->seperator   = ';';
  sm->curr        = sm->buffer;
  sm->end_pos     = sm->buffer;
  
  sm->eof         = FALSE;
  sm->eol         = FALSE;
  sm->skip_line   = FALSE;
  sm->bol         = TRUE;

  sm->fd = efs_open( file_path, O_RDONLY);
  if(sm->fd != -1) 
  {
    return 0;
  }
  else 
  {
    LOG_MSG_INFO1("Cannot open file", 0, 0, 0);
    return -1;
  }
} /* ps_efs_file_init() */

/*===========================================================================
FUNCTION      PS_EFS_FILE_CLOSE

DESCRIPTION   The function closes file and releases the state machine 

DEPENDENCIES  The file should have opened already.

RETURN VALUE  NONE

SIDE EFFECTS  None.
===========================================================================*/
void ps_efs_file_close
( 
  ps_efs_token_type *sm
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if(NULL == sm)
  {
    LOG_MSG_INFO1("Input parameters are NULL!", 0, 0, 0);
    return;
  }

  /* lint fix */
  PS_ARG_NOT_CONST(sm);
  (void) efs_close( sm->fd );
  return;
}

/*===========================================================================
FUNCTION      PS_EFS_TOKENIZER

DESCRIPTION   The is the function that reads data from the opened file.
              The data read is looked for tokens 
              1. Each token is seperated by ';'
              2. Successive ';' means empty token
              3. Line begining with '#' is comment
              4. '\n' is the end of token and record
              5. Empty line is ignored
              6. Insufficient tokens is a record is considered bad record
              
DEPENDENCIES  File should have already been opened.

RETURN VALUE   
              SUCCESS : Success => Found Token.
                        *begin points to the begining of token.
                        *end points to the end of token.
              EOL     : End of line is reached => record end 
                        => no token extracted
              END     : End of file is reached => file end => feof
                        => no token extracted
              FAILURE : Failed 

SIDE EFFECTS  None.
===========================================================================*/
ps_efs_token_parse_status_enum_type ps_efs_tokenizer
(
  ps_efs_token_type *sm,
  char **begin,
  char **end
)
{
  int bytes_read = 0;
  char *dummy;
  ps_efs_token_parse_status_enum_type retval;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if ((NULL == sm) || (NULL == begin) || (NULL == end))
  {
    LOG_MSG_ERROR("Input paramters are NULL", 0, 0, 0);
    return PS_EFS_TOKEN_PARSE_FAILURE;
  }
 
  *begin = 0;
  *end   = 0; 
  /*------------------------------------------------------------x------------
     Traversed to end of file => return 
  ---------------------------------------------------------------------------*/
  if( sm->eof ) 
  {
    return PS_EFS_TOKEN_PARSE_EOF;
  }
   
  /*------------------------------------------------------------------------
     Have some bytes to read from the buffer
  ---------------------------------------------------------------------------*/
  while( sm->curr < sm->end_pos ) 
  {

    /*----------------------------------------------------------------------
      Skip over all carriage return characters (\r) added if file was
      editted using a windows machine
    -----------------------------------------------------------------------*/

    if (*sm->curr == '\r')         
    {
      sm->skip_line = FALSE;
      sm->curr++;
      continue;
    }

    /*-----------------------------------------------------------------------
       Lines begining the record with # are comments. Continue to read 
       until we reach the end of file.
    -----------------------------------------------------------------------*/
    if( sm->bol && *sm->curr ==  '#' ) 
    {
      sm->skip_line = TRUE;
      sm->bol = FALSE;
      sm->curr++;
      continue;
    } 

    if( sm->skip_line )                 /* reading a comment */
    {
      if( *sm->curr == '\n' )           /* End of comment */
      {  
        sm->skip_line = FALSE;
        sm->eol = TRUE;
        sm->bol = TRUE;
      }
      sm->curr++;
      continue;                         /*Continue to read until the end of line */
    }
    
    /*--------------------------------------------------------------------------
      Look for the token. If ';' found at the begining then it is 
      an empty token.
      There could be a  case where we hit '\n' while we are looking for a token
      so skip over all the new lines.
    ----------------------------------------------------------------------------*/
    if( *begin == 0 )                   /* Find the beginning of token */
    {                          
      if( *sm->curr == sm->seperator )  /* an empty token */
      {                             

        if( sm->bol == TRUE ) 
        {
          sm->bol = FALSE;
        }
        
        *begin = sm->curr;
        *end   = sm->curr;
        sm->curr++;
        return PS_EFS_TOKEN_PARSE_SUCCESS;
      }

      if( *sm->curr == '\n' )           /* Possibly an empty token */
      {    
        if( sm->eol )                   /* Skip over any successive new lines */
        {     
          sm->curr++;
          continue;
        }
        *begin  = sm->curr;
        *end    = sm->curr;
        sm->eol = TRUE;
        sm->bol = TRUE;
        sm->curr++;
        return PS_EFS_TOKEN_PARSE_SUCCESS;
      }

      /*-------------------------------------------------------------------------
       Before beginning a new token, return an end of record for previous record. 
      --------------------------------------------------------------------------*/
      if( sm->eol ) 
      {                             
        sm->eol = FALSE;
        return PS_EFS_TOKEN_PARSE_EOL;
      }

      *begin = sm->curr;                /* Initialize to beginning of token */
    }
    else if( *sm->curr == sm->seperator || *sm->curr == '\n' )
    {
      *end = sm->curr++;                /* Found end of token */
      
      /*--------------------------------------------------------------------------
         This is a end of line. Save the state and send 
         end of line event when a next token is requested .
      --------------------------------------------------------------------------*/
      if( **end == '\n' ) 
      {       
        sm->eol = TRUE;
        sm->bol = TRUE;
      }
      return PS_EFS_TOKEN_PARSE_SUCCESS;
    }
    
    sm->curr++;
  }/* while */

  /*-------------------------------------------------------------------------- 
    In the middle of token and we ran out characters in the buffer 
  --------------------------------------------------------------------------*/
  if( *begin ) 
  {      
    
    if( *begin != sm->buffer )
    {
      /*---------------------------------------------------------------------- 
        Move the partial token over to beginning of buffer 
      -----------------------------------------------------------------------*/
      /*lint -e732 */
      memcpy( sm->buffer, *begin, (sm->curr - *begin) );
      /*lint +e732 */
      sm->curr = sm->buffer + (sm->curr - *begin);
      *begin = sm->buffer;
    }
    else 
    {
      LOG_MSG_INFO1("Token is larger than PS_EFS_READ_BUFFER_SZ", 0, 0, 0);
      return PS_EFS_TOKEN_PARSE_FAILURE;
    }
  }
  else 
  {
    /*--------------------------------------------------------------------
      No token or data exists in the buffer 
    ---------------------------------------------------------------------*/
    sm->curr = sm->buffer;
  }
  
  /*----------------------------------------------------------------------
      Read data from the efs file.
  -----------------------------------------------------------------------*/
  {
    /*lint -e732 */
    bytes_read = efs_read( sm->fd, sm->curr, 
                           PS_EFS_READ_BUFFER_SZ - (sm->curr - sm->buffer));
    /*lint +e732 */
    
    if( bytes_read > 0 ) 
    {
      sm->end_pos = sm->curr + bytes_read;
      sm->eof    = FALSE;
      
      if(*begin != 0)
      {
        retval= ps_efs_tokenizer( sm, &dummy, end ); /* Call the function 
                                              again because you could be in the
                                              middle of reading a token */
      }
      else
      {
        retval = ps_efs_tokenizer( sm, begin, end);
      }

      return retval;
    }
    else 
    {

      /*
        No bytes read => reached the end of file.
      */
      if(*begin == 0) 
      {
        sm->eof = 1;
        return PS_EFS_TOKEN_PARSE_EOL;
      }
      else
      {

        /*------------------------------------------------------------------
          If a token was found return the token and 
          when next token is requested send EOF 
        --------------------------------------------------------------------*/
        *end = sm->curr;
        if(bytes_read == 0)
        {
          /*---------------------------------------------------------------
           If the EOF character is missing in the file and the bytes read
           are zero all the time then we are trying to bail out of this.
           
           NOTE: We might have to revisit this later again if different 
           modules
           
          ----------------------------------------------------------------*/
          sm->eof = 1;
          return PS_EFS_TOKEN_PARSE_EOL;
        }
        return PS_EFS_TOKEN_PARSE_SUCCESS;
      }      
    }
  }/* End of bytes read*/
}/* ps_efs_tokenizer() */

#endif /* FEATURE_DATA_PS */
