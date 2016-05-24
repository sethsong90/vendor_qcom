#ifndef __TZ_SERVICE_H_
#define __TZ_SERVICE_H_
/*===========================================================================
  Copyright (c) 2011 - 2013 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/core/pkg/modem/mp/arm11/rel/1.0/modem_proc/core/securemsm/playready/tz/common/shared/inc/tz_playready.h#9 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
10/04/13    cz     Added support of returning partition free size
06/06/13    rk     Added support for file rename operation
05/21/13    dm     Added support for file sync operation
04/04/13    dm     Added changes to support internal decrypt testing for QSAPPS
06/07/12    cz     Added a version control service
01/30/12    dm     Replaced protect_mem() with content protection and added new decrypt API.
01/19/12   chm     Added support for QSECOM.
12/22/11    kr     Update for CR#326083.
12/14/11    dm     Modified the value for TZ_CM_CMD_UNKNOWN as in TZ side
11/17/11    cz     Added TZ_PR_CMD_FILE_CHOWN_CHMOD, and fixed CR 313052
09/19/11    cz     Splited tz_playready.h
02/08/11    vs     Added Provisioning APIs
05/11/11    cz     Added a path in tz_prov_provision_rsp_t to support chmod/chown
04/28/10   chm     Added support for decryption using TZBSP Crypto Driver.
04/28/11   chm     Added support for Memory protection API's.
03/24/11   jct     Added testdir request and response structures
03/03/11   jct     Added fs and time command id's
02/09/11   ssm     Initial Version.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
/** @addtogroup tz_playready
  @} */
#include "comdef.h"
#include "common_log.h"
#include "stdlib.h"
#include "app_main.h"

/* Size of PR license or challenge */

#define TZ_CM_MAX_NAME_LEN          256   /* Fixed. Don't increase the size of TZ_CM_MAX_NAME_LEN*/
#define TZ_CM_MAX_DATA_LEN          20000
#define TZ_CM_PROV_PKG_SIZE         10240  //TZ_CM_PROV_PKG_SIZE must be smaller than TZ_CM_MAX_DATA_LEN

#define TZCOMMON_CREATE_CMD(x)  (SVC_TZCOMMMON_ID | x)


typedef struct tzStat {
    unsigned long long  st_dev;          /* ID of device containing file */
    unsigned char       __pad0[4];
    unsigned long       __st_ino;
    unsigned int        st_mode;         /* protection */
    unsigned int        st_nlink;        /* number of hard links */
    unsigned long       st_uid;          /* user ID of owner */
    unsigned long       st_gid;          /* group ID of owner */
    unsigned long long  st_rdev;         /* device ID (if special file) */
    unsigned char       __pad3[4];
    long long           st_size;         /* total size, in bytes */
    unsigned long	    st_blksize;      /* blocksize for filesystem I/O */
    unsigned long long  st_blocks;       /* number of blocks allocated */
    unsigned long       st_atime;        /* time of last access */
    unsigned long       st_atime_nsec;
    unsigned long       st_mtime;        /* time of last modification */
    unsigned long       st_mtime_nsec;
    unsigned long       st_ctime;        /* time of last status change */
    unsigned long       st_ctime_nsec;
    unsigned long long  st_ino;          /* inode number */
}__attribute__ ((packed)) tzStat_t;

typedef struct tztimespec
{
  uint32      tv_sec;         /* seconds */
  long        tv_nsec;        /* nanoseconds */
}__attribute__ ((packed)) tztimespec_t;

typedef struct tztm
{
  int         tm_sec;         /* seconds */
  int         tm_min;         /* minutes */
  int         tm_hour;        /* hours */
  int         tm_mday;        /* day of the month */
  int         tm_mon;         /* month */
  int         tm_year;        /* year */
  int         tm_wday;        /* day of the week */
  int         tm_yday;        /* day in the year */
  int         tm_isdst;       /* daylight saving time */
}__attribute__ ((packed)) tztm_t;


/**
  Commands for :
  1) TZ Services requested by HLOS
  2) HLOS services requested by TZ
 */
#define SEC_UI_FIRST_COMMAND_ID  TZCOMMON_CREATE_CMD(0x00000501)

typedef enum
{
  /* HLOS to TZ commands -
  ** Following commands represent services that HLOS could request from TZ.
  ** This is the traditional use case where HLOS will be the client and TZ will service the following requests.
  */
  TZ_CM_CMD_INVALID           = TZCOMMON_CREATE_CMD(0x00000000),
  TZ_CM_CMD_INIT_SB_OUT,                                           /**< Initialize the shared buffer */
  TZ_CM_CMD_INIT_SB_LOG,                                           /**< Initialize the logging shared buf */
  TZ_CM_CMD_PROTECT_MEM_UNUSED,                                    /**< Protect content data memory */
  TZ_CM_CMD_REGISTER_LISTENER,
  TZ_CM_CMD_EXEC_TEST_START   = TZCOMMON_CREATE_CMD(0x00000101),
  TZ_CM_CMD_EXEC_TEST,
  TZ_CM_CMD_EXEC_TEST_END,


  /* TZ to HLOS commands -
  ** Following commands represent services that TZ could request from the HLOS.
  ** Essentially, in these instances, TZ will be the client and HLOS will service the requests.
  */
  TZ_CM_CMD_FILE_START        = TZCOMMON_CREATE_CMD(0x00000201),
  TZ_CM_CMD_FILE_OPEN,
  TZ_CM_CMD_FILE_OPENAT,
  TZ_CM_CMD_FILE_UNLINKAT,
  TZ_CM_CMD_FILE_FCNTL,
  TZ_CM_CMD_FILE_CREAT,
  TZ_CM_CMD_FILE_READ,      /**< Read from a file */
  TZ_CM_CMD_FILE_WRITE,     /**< Write to a file */
  TZ_CM_CMD_FILE_CLOSE,     /**< Close a file opened for read/write */
  TZ_CM_CMD_FILE_LSEEK,     /**< Seek to a offset in file */
  TZ_CM_CMD_FILE_LINK,
  TZ_CM_CMD_FILE_UNLINK,
  TZ_CM_CMD_FILE_RMDIR,
  TZ_CM_CMD_FILE_FSTAT,
  TZ_CM_CMD_FILE_LSTAT,
  TZ_CM_CMD_FILE_MKDIR,
  TZ_CM_CMD_FILE_TESTDIR,
  TZ_CM_CMD_FILE_TELLDIR,
  TZ_CM_CMD_FILE_REMOVE,
  TZ_CM_CMD_FILE_CHOWN_CHMOD,
  TZ_CM_CMD_FILE_END,
  TZ_CM_CMD_FILE_SYNC,
  TZ_CM_CMD_FILE_RENAME,
  TZ_CM_CMD_FILE_PAR_FR_SIZE,  /* get partition free size */
  TZ_CM_CMD_TIME_START        = TZCOMMON_CREATE_CMD(0x00000301),
  TZ_CM_CMD_TIME_GET_UTC_SEC,
  TZ_CM_CMD_TIME_GET_SYSTIME,
  TZ_CM_CMD_TIME_GET_TIME_MS,
  TZ_CM_CMD_TIME_END,

  /* TZ to HLOS commands -
  ** HLOS gets the TZ version
  */
  TZ_CM_CMD_VERSION_START     = TZCOMMON_CREATE_CMD(0x00000401),
  TZ_CM_CMD_VERSION_GET_VER,
  TZ_CM_CMD_VERSION_END,
  TZ_CM_CMD_CPCHECK_TOGGLE,   /**< turns on/off the content protection feature on TZ */

  /* TZ to HLOS Secure UI listener commands
  */
  SEC_UI_CMD_GET_SCREEN_PROPERTIES     = SEC_UI_FIRST_COMMAND_ID,
  SEC_UI_CMD_START_SECURE_DISPLAY,
  SEC_UI_CMD_STOP_SECURE_DISPLAY,
  SEC_UI_CMD_DISPLAY_SECURE_BUFF,
  SEC_UI_CMD_GET_SECURE_DISPLAY_STATUS,
  SEC_UI_CMD_PROTECT_NEXT_BUFFER,
  SEC_UI_CMD_INIT_DONE,
  SEC_UI_CMD_START_SECURE_TOUCH,
  SEC_UI_CMD_STOP_SECURE_TOUCH,
  SEC_UI_CMD_WAIT_FOR_TOUCH_EVENT,
  SEC_UI_CMD_HLOS_RELEASE,
  SEC_UI_LAST_COMMAND_ID,

  TZ_CM_CMD_UNKNOWN           = TZCOMMON_CREATE_CMD(0x7FFFFFFF)
} tz_common_cmd_type;

/** Command structure for initializing shared buffers (SB_OUT
    and SB_LOG)
*/
typedef struct tz_init_sb_req_s
{
  /** First 4 bytes should always be command id */
  tz_common_cmd_type        cmd_id;
  /**<-- svc for which the shared buffer */
  uint32                    listener_id;
  /**<-- Pointer to the physical location of sb_out buffer */
  uint32                    sb_ptr;
  /**<-- length of shared buffer */
  uint32                    sb_len;
} __attribute__ ((packed)) tz_init_sb_req_t;

typedef struct tz_init_qsecom_sb_req_s
{
  /** First 4 bytes should always be command id */
  tz_common_cmd_type        cmd_id;
  /**<-- Pointer to the physical location of sb_out buffer */
  uint32                    sb_ptr;
  /**<-- length of shared buffer */
  uint32                    sb_len;
  /**<-- svc for which the shared buffer */
  uint32                    svc_id;
} __attribute__ ((packed)) tz_init_qsecom_sb_req_t;

typedef struct tz_init_sb_rsp_s
{
  /** First 4 bytes should always be command id */
  tz_common_cmd_type        cmd_id;
  /**<-- Return code, 0 for success, Approp error code otherwise */
  int32                     ret;
} __attribute__ ((packed)) tz_init_sb_rsp_t;


/** Command structure for file open
*/
typedef struct tz_file_open_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** Pointer to file name with complete path */
  const char              pathname[TZ_CM_MAX_NAME_LEN];
  /** File status flags */
  int                     flags;
} __attribute__ ((packed)) tz_file_open_req_t;

typedef struct tz_file_open_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** File descriptor */
  int                     ret;
} __attribute__ ((packed)) tz_file_open_rsp_t;


/** Command structure for file openat
*/
typedef struct tz_file_openat_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  int                     dirfd;
  /** Pointer to file name with complete path */
  const char              pathname[TZ_CM_MAX_NAME_LEN];
  /** File status flags */
  int                     flags;
} __attribute__ ((packed)) tz_file_openat_req_t;

typedef struct tz_file_openat_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** File descriptor */
  int                     ret;
} __attribute__ ((packed)) tz_file_openat_rsp_t;


/** Command structure for file unlinkat
*/
typedef struct tz_file_unlinkat_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** File descriptor */
  int                     dirfd;
  /** Pathname of file */
  const char              pathname[TZ_CM_MAX_NAME_LEN];
  /** Flags */
  int                     flags;
} __attribute__ ((packed)) tz_file_unlinkat_req_t;

typedef struct tz_file_unlinkat_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_unlinkat_rsp_t;


/** Command structure for file fcntl
*/
typedef struct tz_file_fcntl_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** File descriptor */
  int                     fd;
  /** Operation to be performed */
  int                     cmd;
} __attribute__ ((packed)) tz_file_fcntl_req_t;

typedef struct tz_file_fcntl_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type       cmd_id;
  /** Return value depends on operation */
  int                     ret;
} __attribute__ ((packed)) tz_file_fcntl_rsp_t;


/** Command structure for file creat
*/
typedef struct tz_file_creat_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** Pathname of file */
  const char              pathname[TZ_CM_MAX_NAME_LEN];
  /** Access modes */
  uint32                  mode; //uint32 is typedef unsigned short in Jade's fs stub
} __attribute__ ((packed)) tz_file_creat_req_t;

typedef struct tz_file_creat_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_creat_rsp_t;


/** Command structure for file read
*/
typedef struct tz_file_read_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** File descriptor */
  int                     fd;
  /** Number of bytes to read */
  uint32                  count;
} __attribute__ ((packed)) tz_file_read_req_t;

typedef struct tz_file_read_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** Buffer containing read data */
  uint8                   buf[TZ_CM_MAX_DATA_LEN];
  /**<-- Number of bytes read */
  int32                   ret;
} __attribute__ ((packed)) tz_file_read_rsp_t;


/** Command structure for file write
*/
typedef struct tz_file_write_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** File descriptor */
  int                     fd;
  /** Buffer to write from */
  uint8                   buf[TZ_CM_MAX_DATA_LEN];
  /** Number of bytes to write */
  uint32                  count;
} __attribute__ ((packed)) tz_file_write_req_t;

typedef struct tz_file_write_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type    cmd_id;
  /**<-- Number of bytes written */
  int32                 ret;
} __attribute__ ((packed)) tz_file_write_rsp_t;


/** Command structure for file close
*/
typedef struct tz_file_close_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** File descriptor */
  int                     fd;
} __attribute__ ((packed)) tz_file_close_req_t;

typedef struct tz_file_close_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_close_rsp_t;


/** Command structure for file lseek
*/
typedef struct tz_file_lseek_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** File descriptor */
  int                     fildes;
  /** New offset */
  int32                   offset;
  /** Directive */
  int                     whence;
} __attribute__ ((packed)) tz_file_lseek_req_t;

typedef struct tz_file_lseek_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** Resulting offset */
  int32                   ret;
} __attribute__ ((packed)) tz_file_lseek_rsp_t;


/** Command structure for file link
*/
typedef struct tz_file_link_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** Pathname of existing file */
  const char              oldpath[TZ_CM_MAX_NAME_LEN];
  /** Pathname of new link to existing file */
  const char              newpath[TZ_CM_MAX_NAME_LEN];
} __attribute__ ((packed)) tz_file_link_req_t;

typedef struct tz_file_link_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_link_rsp_t;


/** Command structure for file unlink
*/
typedef struct tz_file_unlink_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** Pathname of file */
  const char              pathname[TZ_CM_MAX_NAME_LEN];
} __attribute__ ((packed)) tz_file_unlink_req_t;

typedef struct tz_file_unlink_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_unlink_rsp_t;


/** Command structure for file rmdir
*/
typedef struct tz_file_rmdir_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Pathname of file */
  const char              path[TZ_CM_MAX_NAME_LEN];
} __attribute__ ((packed)) tz_file_rmdir_req_t;

typedef struct tz_file_rmdir_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_rmdir_rsp_t;


/** Command structure for file fstat
*/
typedef struct tz_file_fstat_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** File descriptor */
  int                     filedes;
} __attribute__ ((packed)) tz_file_fstat_req_t;

typedef struct tz_file_fstat_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Pointer to status structure */
  struct tzStat           buf;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_fstat_rsp_t;


/** Command structure for file lstat
*/
typedef struct tz_file_lstat_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Pathname of file */
  const char              path[TZ_CM_MAX_NAME_LEN];
} __attribute__ ((packed)) tz_file_lstat_req_t;

typedef struct tz_file_lstat_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Pointer to status structure */
  struct tzStat           buf;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_lstat_rsp_t;


/** Command structure for file mkdir
*/
typedef struct tz_file_mkdir_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Pathname of directory */
  const char              pathname[TZ_CM_MAX_NAME_LEN];
  /** Permissions mode */
  uint32                  mode;
} __attribute__ ((packed)) tz_file_mkdir_req_t;

typedef struct tz_file_mkdir_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_mkdir_rsp_t;

/** Command structure for file testdir
*/
typedef struct tz_file_testdir_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Pathname of directory */
  const char              pathname[TZ_CM_MAX_NAME_LEN];
} __attribute__ ((packed)) tz_file_testdir_req_t;

typedef struct tz_file_testdir_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_testdir_rsp_t;

/** Command structure for file telldir
*/
typedef struct tz_file_telldir_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Pathname of directory */
  const char              pathname[TZ_CM_MAX_NAME_LEN];
} __attribute__ ((packed)) tz_file_telldir_req_t;

typedef struct tz_file_telldir_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Current location of directory stream */
  int32                   ret;
} __attribute__ ((packed)) tz_file_telldir_rsp_t;

/** Command structure for file remove
*/
typedef struct tz_file_remove_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Pathname of directory */
  const char              pathname[TZ_CM_MAX_NAME_LEN];
} __attribute__ ((packed)) tz_file_remove_req_t;

typedef struct tz_file_remove_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_remove_rsp_t;

/** Command structure for file chown and chmod
*/
typedef struct tz_file_chown_chmod_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** memory for path */
  char                    path[TZ_CM_MAX_NAME_LEN];
  /** the length of path */
  uint32                  path_len;
  /** memory for word */
  char                    word[TZ_CM_MAX_NAME_LEN];
  /** the length of word */
  uint32                  word_len;
  /** memory for owner e.g., media.system */
  char                    owner[TZ_CM_MAX_NAME_LEN];
  /** the length of owner */
  uint32                  owner_len;
  /** memory for mod, e.g., 777 */
  char                    mod[TZ_CM_MAX_NAME_LEN];
  /** the length of mod */
  uint32                  mod_len;
   /** the level */
  uint32                  level;
} __attribute__ ((packed)) tz_file_chown_chmod_req_t;

typedef struct tz_file_chown_chmod_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** 0 for Success/ -1 for failure */
  int                     ret;
} __attribute__ ((packed)) tz_file_chown_chmod_rsp_t;

/** Command structure for file end
*/
typedef struct tz_file_end_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
} __attribute__ ((packed)) tz_file_end_req_t;

typedef struct tz_file_end_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_end_rsp_t;

/* Command structure for file sync*/
typedef struct tz_file_sync_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** File descriptor */
  int              fd;
} __attribute__ ((packed)) tz_file_sync_req_t;

typedef struct tz_file_sync_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_sync_rsp_t;

/* Command structure for file rename*/
typedef struct tz_file_rename_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;

  /** file name to be changed from */
  const char              oldfilename[TZ_CM_MAX_NAME_LEN];

  /** file name to be changed to */
  const char              newfilename[TZ_CM_MAX_NAME_LEN];
} __attribute__ ((packed)) tz_file_rename_req_t;

/* Command structure for file rename*/
typedef struct tz_file_rename_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;

  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_rename_rsp_t;

typedef struct tz_file_err_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_err_rsp_t;

/** Command structure for getting partition free size
*/
typedef struct tz_file_par_free_size_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** partition name*/
  uint8                       partition[TZ_CM_MAX_NAME_LEN];
} __attribute__ ((packed)) tz_file_par_free_size_req_t;

typedef struct tz_file_par_free_size_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** free size in Byte */
  uint64                      size;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_file_par_free_size_rsp_t;


/** Command structure for getutcsec
*/
typedef struct tz_time_getutcsec_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
} __attribute__ ((packed)) tz_time_getutcsec_req_t;

typedef struct tz_time_getutcsec_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Pointer to timespec structure */
  struct tztimespec       tzTimeSpec;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_time_getutcsec_rsp_t;


/** Command structure for systime
*/
typedef struct tz_time_getsystime_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
} __attribute__ ((packed)) tz_time_getsystime_req_t;

typedef struct tz_time_getsystime_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Pointer to time structure */
  struct tztm             tzTime;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_time_getsystime_rsp_t;


/** Command structure for timems
*/
typedef struct tz_time_gettimems_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
} __attribute__ ((packed)) tz_time_gettimems_req_t;

typedef struct tz_time_gettimems_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Time in milliseconds */
  unsigned long           ret;
} __attribute__ ((packed)) tz_time_gettimems_rsp_t;

/** Command structure for time end
*/
typedef struct tz_time_end_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
} __attribute__ ((packed)) tz_time_end_req_t;

typedef struct tz_time_end_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_time_end_rsp_t;

typedef struct tz_time_err_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type      cmd_id;
  /** Success/failure value */
  int                     ret;
} __attribute__ ((packed)) tz_time_err_rsp_t;

typedef struct tz_protect_mem_req_s
{
  /** First 4 bytes should always be command id */
  tz_common_cmd_type        cmd_id;
  uint32                    buf_start_ptr;   /** Physical address of input content buffer. */
  uint32                    buf_end_ptr;
  uint8						lock;		// TRUE - Lock   FALSE - Unlock
} __attribute__ ((packed)) tz_protect_mem_req_t;

typedef struct tz_protect_mem_rsp_s
{
  /** First 4 bytes should always be command id */
  tz_common_cmd_type        cmd_id;
  long                      ret;             /**<-- E_SUCCESS for success and E_FAILURE for failure */
} __attribute__ ((packed)) tz_protect_mem_rsp_t;


typedef struct tz_exec_test_req_s
{
  /** First 4 bytes should always be command id */
  tz_common_cmd_type        cmd_id;
  /** test module */
  uint32                    module;
} __attribute__ ((packed)) tz_exec_test_req_t;

typedef struct tz_exec_test_rsp_s
{
  /** First 4 bytes should always be command id */
  tz_common_cmd_type        cmd_id;
  /** Results of the tests; 0 - success; otherwise failure */
  int8                      resultVector[50];
  /** Error messages for failed tests */
  uint8                     errorLogs[TZ_CM_MAX_DATA_LEN];
  /** Number of tests executed */
  long                      ret;
} __attribute__ ((packed)) tz_exec_test_rsp_t;


typedef struct tz_unknown_rsp_s
{
  /** First 4 bytes should always be command id */
  tz_common_cmd_type       cmd_id;
} __attribute__ ((packed)) tz_unknown_rsp_t;


/** Command structure for getting tzapps version
*/
typedef struct tz_qsappsver_get_ver_req_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
} __attribute__ ((packed)) tz_qsappsver_get_ver_req_t;


typedef struct tz_qsappsver_get_ver_rsp_s
{
  /** First 4 bytes are always command id */
  tz_common_cmd_type          cmd_id;
  /** Version of tz apps */
  uint32                      version;
  /**<-- Return value for maintenance */
  int32                   ret;
} __attribute__ ((packed)) tz_qsappsver_get_ver_rsp_t;


/** Command structure to set the DRM CP flag
*/
typedef struct tz_qsapp_cpcheck_req_s
{
  /** First 4 bytes should always be command id */
  tz_common_cmd_type          cmd_id;
  /*flag for CP turning on/off*/
  uint8                       bContentProtection;
} __attribute__ ((packed)) tz_qsapp_cpcheck_req_t;

typedef struct tz_qsapp_cpcheck_rsp_s
{
  /** First 4 bytes should always be command id */
  tz_common_cmd_type          cmd_id;
  long                        ret;
} __attribute__ ((packed)) tz_qsapp_cpcheck_rsp_t;

/**
 * @brief  This function will return the address of shared buf
 *         output pointer
 *
 * @return Pointer to shared output buffer or NULL if shared
 *         buffer is not initialized
 */
uint8 * com_get_sb_out(uint32 listener_id);

/**
 * @brief  This function will return the address of shared buf
 *         for logging
 *
 * @return Pointer to shared output buffer or NULL if shared
 *         buffer is not initialized
 */
uint8 * com_get_sb_log(void);



#endif /* __TZ_SERVICE_H_ */
