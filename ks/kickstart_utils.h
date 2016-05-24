/*===========================================================================
 *  FILE:
 *  kickstart_utils.h
 *
 *  DESCRIPTION:
 *  File io wrapper functions and <id, file_name> linked list interfaces
 *
 *  Copyright (C) 2012 Qualcomm Technologies, Inc. All rights reserved.
 *                  Qualcomm Technologies Proprietary/GTDR
 *
 *  All data and information contained in or disclosed by this document is
 *  confidential and proprietary information of Qualcomm Technologies, Inc. and all
 *  rights therein are expressly reserved.  By accepting this material the
 *  recipient agrees that this material and the information contained therein
 *  is held in confidence and in trust and will not be used, copied, reproduced
 *  in whole or in part, nor its contents revealed in any manner to others
 *  without the express written permission of Qualcomm Technologies, Inc.
 *  ===========================================================================
 *
 *
 *  kickstart_utils.h : File io wrapper functions and <id, file_name> linked list interfaces
 * ==========================================================================================
 *   $Header: //source/qcom/qct/core/storage/tools/kickstart/kickstart_utils.h#2 $
 *   $DateTime: 2010/05/03 22:12:02 $
 *   $Author: niting $
 *
 *  Edit History:
 *  YYYY-MM-DD		who		why
 *  -----------------------------------------------------------------------------
 *
 *  Copyright 2012 by Qualcomm Technologies, Inc.  All Rights Reserved.
 *
 *==========================================================================================
 */
#ifndef KICKSTART_UTILS_H
#define KICKSTART_UTILS_H

//#define LINUXPC

#if defined(LINUXPC)
    size_t strlcpy(char *dst, const char *src, size_t size);
    size_t strlcat(char *dst, const char *src, size_t size);
#endif

#define PREFIX_STRING_SIZE 256
extern char upload_prefix_string[PREFIX_STRING_SIZE];


/*list node that holds the image id and file mapping*/
struct list_node
{
   int image_id;
   char file_name[1024];
   struct list_node* next;
};


/******************************************************************************
* Name: init_input_file_list
*
* Description:
*    initialize <id, file_name> mapping list
*
* Arguments:
*   NONE
*
* Returns:
*  NONE
******************************************************************************/
int   init_input_file_list(void);
/******************************************************************************
* Name: destroy_input_file_list
*
* Description:
*    destroys <id, file_name> mapping list
*
* Arguments:
*   NONE
*
* Returns:
*  NONE
******************************************************************************/
void   destroy_input_file_list(void);
/******************************************************************************
* Name: push_input_to_file_list
*
* Description:
*    pushes input file to  <id, file_name> mapping list
*
* Arguments:
*   input from the command line of the form id:file_name
*
* Returns:
*  SUCCESS/EFAILED
******************************************************************************/
int   push_input_to_file_list(char* arg);
/******************************************************************************
* Name: delete_file_from_list
*
* Description:
*    deletes node matching the the file_name
*
* Arguments:
*   file_name
*
* Returns:
*  NONE
******************************************************************************/
void   delete_file_from_list(char* file_name);
/******************************************************************************
* Name: is_input_file_list_empty
*
* Description:
*    checks if input file list is empty or not
*
* Arguments:
*  NONE
*
* Returns:
*  TRUE/FALSE
******************************************************************************/
int  is_input_file_list_empty(void);
/******************************************************************************
* Name: get_file_from_file_list
*
* Description:
*    get file from file list which matches the image id
*
* Arguments:
*  image id
*
* Returns:
*  file_name or NULL
******************************************************************************/
char*  get_file_from_file_list(int image_id);

/******************************************************************************
* Name: open_file
*
* Description:
*    wrapper for file open system call
*
* Arguments:
*   file_name  - file to be opened
*
* Returns:
*  int         - -1 or valid file descriptor
******************************************************************************/
int  open_file(const char *file_name);
/******************************************************************************
* Name: get_file_size
*
* Description:
*    get the file size
*
* Arguments:
*   fd - descriptor of the file
*
* Returns:
*  unsigned long         - size of the file
*
******************************************************************************/
unsigned long get_file_size(int fd);
/*===========================================================================
 *  METHOD:
 *  is_regular_file
 *
 *  DESCRIPTION:
 *  returns positive value if the file descriptor points to a regular file
 *  Useful to distinguish between files and device nodes
 *
 *  PARAMETERS
 *  fd  - file descriptor to be opened
 *
 *  RETURN VALUE:
 *  int   - -1 if cannot stat, 0 or non-zero if non-regular or regular file
 *  ===========================================================================*/
int is_regular_file(int fd);
/******************************************************************************
* Name: close_file
*
* Description:
*    wrapper for file close system call
*
* Arguments:
*   fd  - descriptor of the file to be closed
*
* Returns:
*  int  - SUCCESS/EFAILED
*
******************************************************************************/
int  close_file(int fd);

/******************************************************************************
* Name: startswith
*
* Description:
*    Checks whether string 'haystack' begins with string 'needle'
*
* Arguments:
*   haystack   - string to check within
*   needle     - string to check for
*
* Returns:
*  int         - 1 if haystack starts with needle, 0 otherwise
******************************************************************************/
int startswith(const char* haystack, const char* needle);

/******************************************************************************
* Name: create_path
*
* Description:
*    Creates a directory path if it does not already exist
*
* Arguments:
*   path       - directory path to check for
*   mode       - mode to be used for any directory creation
*
* Returns:
*  int         - 0 if path is successfully created
******************************************************************************/
int create_path(const char* path, mode_t mode);

typedef enum {
    WAKELOCK_RELEASE,
    WAKELOCK_ACQUIRE
} wakelock_action;
int use_wakelock (wakelock_action action);

#endif
