#ifndef PS_UTILS_H
#define PS_UTILS_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                           P S _ U T I L S . H

GENERAL DESCRIPTION
  Collection of utility functions being used by various modules in PS.
  Most of these functions assume that the caller is in PS task context.

Copyright (c) 1995-2009 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

  $PVCSPath: L:/src/asw/MM_DATA/vcs/ps_utils.h_v   1.0   08 Aug 2002 11:19:58   akhare  $
  $Header: //source/qcom/qct/modem/datamodem/interface/api/rel/11.03/ps_utils.h#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

when      who     what, where, why
-------   ---     ----------------------------------------------------------
10/26/10  op      Added functions to use EFS items.
01/08/09  ar      Added C++ wrapper.
12/17/08  pp      CMI: Public/Private API split.
09/08/06  mct     Added 64 bit random number generator function.
02/22/06  rt      Added macros for rex critical sections (FEATURE_DATA_PS_L4)
02/06/06  msr/ssh Added macros for rex critical sections (FEATURE_DATA_PS_L4)
09/09/04  msr/ks  Added memdump().
08/05/02  usb     Moved lcsum() from this file to ps_iputil.c
08/05/02  usb     Moved inclusion of nv.h out of FEATURE_MIP
07/31/02  usb     Renamed the file from psmisc.h
06/14/02  usb     Removed byte manipulation functions, use dsbyte.h.
04/17/02  rc      Wrapped include of rlcdl.h in FEATURE_DATA_WCDMA_PS
03/04/02  dwp     Add include of rlcdl.h.
12/21/01  dwp     Wrap get16 in !FEATURE_DATA_PS as it is defined else where
                  in MSM5200 archive.
11/12/01  dwp     Add "|| FEATURE_DATA_PS" to whole module.
08/05/99  rc      Added support for UDP debug statistics for Sockets.
03/04/99  hcg     Changed browser interface feature to FEATURE_DS_BROWSER_INTERFACE.
06/16/97  fkm     FEATURE_xxx Updates (via Automatic Scripts)
06/12/97  ldg     Added new FEATURE_DS_PSTATS
04/11/95  jjw     Initial version of file


===========================================================================*/





/*===========================================================================

                      INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "ps_svc.h"
#include "amssassert.h"

#ifndef FEATURE_DSS_LINUX
#include "nv.h"
#include "fs_public.h"
#include "fs_sys_types.h"
#include "fs_fcntl.h"
#include "fs_errno.h"
#endif /* FEATURE_DSS_LINUX */
/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  Buffer to read the file into
---------------------------------------------------------------------------*/
#define PS_EFS_READ_BUFFER_SZ 128

/*-----------------------------------------------------------------------------
  Structure used to parse the efs file
-----------------------------------------------------------------------------*/
typedef struct
{
    int fd;                          /*  File descriptor                     */
    char buffer[PS_EFS_READ_BUFFER_SZ];/*  Buffer to read the file into   */
    char *curr;                     /*  pointer to the current location      */
    char *end_pos;                  /*  ponter to the end of the buffer      */
    char seperator;                  /*  Seperator(;) to be parsed for       */
    boolean eof;                    /*  used to indicate the end of file     */
    boolean skip_line;              /*  identifies  comments in the file     */
    boolean eol;                    /*  used to indicate end of line         */
    boolean bol;                    /*  used to indicate begining of the line*/
}ps_efs_token_type;

/*-----------------------------------------------------------------------------
  Enum to specify the various return values 
  SUCCESS : Success
  EOL     : End of line is reached => record end
  EOF     : End of file is reached => file end => feof
  FAILURE : Failed 
-----------------------------------------------------------------------------*/
typedef enum 
{
  PS_EFS_TOKEN_PARSE_SUCCESS  = 0,
  PS_EFS_TOKEN_PARSE_EOL      = 1,
  PS_EFS_TOKEN_PARSE_EOF      = 2,
  PS_EFS_TOKEN_PARSE_FAILURE  = 3,
  PS_EFS_TOKEN_PARSE_MAX      = 0xFF
}ps_efs_token_parse_status_enum_type;

/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FEATURE_DSS_LINUX
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
extern nv_stat_enum_type ps_get_nv_item
(
  nv_items_enum_type  item_code,       /* Item to get                      */
  nv_item_type        *data_ptr        /* Pointer where to put the item    */
);

/*===========================================================================
FUNCTION PS_PUT_NV_ITEM

DESCRIPTION
  Write an item to NV memory.  Wait until write is completed.

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
extern nv_stat_enum_type ps_put_nv_item(
  nv_items_enum_type item_code,                              /* which item */
  nv_item_type *data_ptr                       /* pointer to data for item */
);
#endif /* FEATURE_DSS_LINUX */

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
extern dword msclock( void);

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
extern void memdump
(
  void * data_ptr,
  int    len
);

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
);
#ifndef FEATURE_DSS_LINUX
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
 -1         - Non-EFS related failures
  efs_errno - EFS related failures. Meaning of this value can be
              found in fs_errno.h

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_create_efs_config_file
(
  const char *conf_file_path
);

/*===========================================================================
FUNCTION PS_CREATE_EFS_ITEM_FILE

DESCRIPTION
  Put(append and add newline) item file path into config file

DEPENDENCIES
  None

PARAMETERS
  conf_file_path      - File path to a specific conf file
  item_file_path      - File path to item in NV
  item_file_path_size - Size of item file path

RETURN VALUE
  0         - Success
 -1         - Non-EFS related failures
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
);

/*===========================================================================
FUNCTION PS_READ_EFS_NV

DESCRIPTION
  This function reads the EFS item from the item file

DEPENDENCIES
  None

PARAMETERS
  item_file_path - File path to item in NV
  nv_info        - Pointer to data type for NV item(s)
  nv_info_size   - Size of NV item data type

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
);

/*===========================================================================
FUNCTION PS_WRITE_EFS_NV

DESCRIPTION
  This function writes the EFS item to the item file

DEPENDENCIES
  None

PARAMETERS
  item_file_path - File path to item in NV
  nv_info        - Pointer to data type for NV item(s)
  nv_info_size   - Size of NV item data type

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
);

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
);

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
);

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
);

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
);

#endif

#ifdef __cplusplus
}
#endif

#endif /* PS_UTILS_H */
