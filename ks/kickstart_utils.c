/*===========================================================================
 *  FILE:
 *  kickstart_utils.c
 *
 *  DESCRIPTION:
 *
 *  The module provides wrappers for file io operations and linked list operations
 *  for storing image id and file mapping
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
 *  kickstart_utils.c : The module provides wrappers for file io operations and linked list operations
 *  for storing image id and file mapping
 * ==========================================================================================
 *   $Header: //source/qcom/qct/core/storage/tools/kickstart/kickstart_utils.c#2 $
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

/* ----------------------------------------------------------------------------- */
/* Include Files */
/* ----------------------------------------------------------------------------- */
#include "common_protocol_defs.h"
#include "kickstart_utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

extern unsigned int malloc_count;
extern unsigned int free_count;

/*root node of <id, file_name> list*/
static struct list_node* list_root = NULL;

#define MAX_ARG_LENGTH 1024

/*===========================================================================
 *  METHOD:
 *  open file
 *
 *  DESCRIPTION:
 *  wrapper for file open system call
 *
 *  PARAMETERS
 *  file_name  - file to be opened
 *
 *  RETURN VALUE:
 *  int        - -1 or valid file descriptor
 *  ===========================================================================*/
int  open_file(const char *file_name)
{
    int fd = -1;
    if (file_name == 0 || file_name[0] == 0) {
        dbg (ERROR, "File not found");
        return -1;
    }

    /* Open the file */
    fd = open (file_name, O_RDONLY);
    if (fd == -1) {
        dbg (ERROR, "Unable to open input file %s.  Error %d: %s",
             file_name,
             errno,
             strerror (errno));
        return -1;
    }
    return fd;
}


/*===========================================================================
 *  METHOD:
 *  get_file_size
 *
 *  DESCRIPTION:
 *  returns the size of the file pointed by file descriptor
 *
 *  PARAMETERS
 *  fd  - file descriptor to be opened
 *
 *  RETURN VALUE:
 *  unsigned long   - -1 or valid file size
 *  ===========================================================================*/
unsigned long get_file_size(int fd)
{
    struct stat fileInfo;
    if (fstat (fd, &fileInfo) != 0) {
        dbg (ERROR, "Unable to get file size for fd: %d.  Error: %s",
             errno,
             strerror (errno));
        return -1;
    }
    return fileInfo.st_size;
}

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
int is_regular_file(int fd)
{
    struct stat fileInfo;
    if (fstat(fd, &fileInfo) != 0) {
        dbg (ERROR, "Unable to stat file. Errno %d. Error: %s",
             errno,
             strerror (errno));
        return -1;
    }
    return S_ISREG(fileInfo.st_mode);
}

/*===========================================================================
 *  METHOD:
 *  close_file
 *
 *  DESCRIPTION:
 *  Closes file handle
 *
 *  PARAMETERS
 *  fd  - file descriptor to be opened
 *
 *  RETURN VALUE:
 *  int   - EFAILED/SUCCESS
 *  ===========================================================================*/
int  close_file(int fd)
{
   if(close(fd))
   {
     dbg (ERROR, "Unable to close fd: %d: System error code %s", fd, strerror(errno));
     return EFAILED;
   }
   return SUCCESS;
}



/*===========================================================================
 *  METHOD:
 *  init_input_file_list
 *
 *  DESCRIPTION:
 *  initializes the <id, file_name> mapping list
 *
 *  PARAMETERS
 *  NONE
 *
 *  RETURN VALUE:
 *  int   - EFAILED/SUCCESS
 *  ===========================================================================*/
int init_input_file_list(void)
{
    /*Creates a root noot*/
    list_root = malloc(sizeof(struct list_node));
    if (NULL == list_root) {
        dbg (ERROR, "Memory allocation failure: %s", strerror(errno));
        return EFAILED;
    }
    malloc_count++;
    list_root->next = NULL;
    return SUCCESS;
}

/*===========================================================================
 *  METHOD:
 *  print_input_file_list
 *
 *  DESCRIPTION:
 *  shows the current list contents
 *
 *  PARAMETERS
 *  NONE
 *
 *  RETURN VALUE:
 *  NONE
 *  ===========================================================================*/
void print_input_file_list(void)
{
    unsigned int Counter = 0;
    struct list_node *head = list_root;

    dbg(INFO, "-----------Linked List Contains--------------");

    while (head != NULL)
    {
        Counter++;
        dbg(INFO, "%.2i\t%2i:%s", Counter, head->image_id, head->file_name);
        head = head->next;
    }
    dbg(INFO, "---------------------------------------------\n\n");

}

/*===========================================================================
 *  METHOD:
 *  destroy_input_file_list
 *
 *  DESCRIPTION:
 *  delete <id, file_name> mapping list
 *
 *  PARAMETERS
 *  NONE
 *
 *  RETURN VALUE:
 *  NONE
 *  ===========================================================================*/
void destroy_input_file_list(void)
{
    struct list_node *head = list_root;
    struct list_node *temp;
    int Counter = 0;

    // Show us what we have
    print_input_file_list();

    dbg(STATUS, "Removing linked list of input files");

    while (head != NULL)
    {
        Counter++;

        if(head->file_name != NULL)
            dbg(INFO, "free(head) %.2i\t%2i:%s",Counter,head->image_id,head->file_name);
        else
            dbg(INFO, "free(head) %.2i\t%2i:(null)",Counter,head->image_id);

        temp = head->next;

        //file_name is a static buffer, this line below is just a reminder of that
        if(strlen(head->file_name) > 0)
            head->file_name[0] = 0;

        if (NULL!=head) {
            free (head);
            head = NULL;
            free_count++;
        }

        head=temp;
    }

    if (NULL != head) {
        free(head);
        head = NULL;
        free_count++;
    }

    list_root = NULL;   // make sure this pointer is also NULL

    dbg(INFO, "------------Linked List is Destroyed-----------\n");

    return;
}

/*===========================================================================
 *  METHOD:
 *  push_input_to_file_list
 *
 *  DESCRIPTION:
 *  pushes input string to the list with <id, file_name> format
 *
 *  PARAMETERS
 *  arg - input string
 *
 *  RETURN VALUE:
 *  EFAILED/SUCESS
 *  ===========================================================================*/
#define MAX_IMAGE_ID_SIZE 10
int push_input_to_file_list(char* arg)
{
    char img_id[MAX_IMAGE_ID_SIZE];   /*for temporarily storing image id*/
    struct list_node *tail = list_root;
    if (tail != NULL )
    {
        while (tail->next != NULL)
        {
            tail = tail->next;
        }
    }
    else
    {
        dbg (ERROR, "List root not initialized");
        return EFAILED;
    }

    if(tail == NULL)
    {
        dbg (ERROR, "tail is NULL in push_input_to_file_list, cannot continue!!!");
        return EFAILED;
    }

    tail->next = malloc (sizeof(struct list_node));
    if (NULL == tail->next) {
        dbg (ERROR, "Memory allocation failure: %s", strerror(errno));
        return EFAILED;
    }
    malloc_count++;  // sanity test to compare later
    tail->next->next = NULL;

    if(strlen(arg) > MAX_ARG_LENGTH - 1)   // using -1 instead of strlen(arg)+1
    {
        dbg (ERROR, "'%s' is too long (it must fit within %d bytes)",arg,MAX_ARG_LENGTH);
        dbg(ERROR, "Looking at '%s'", arg);
        goto error_case_push_input_to_file_list;
    }

    /*Get the image id and the file name from the input */
    char* pch = strchr(arg, ':');
    if(NULL == pch)
    {
       dbg(ERROR, "Input not in the form <image_id:image_file_name>");
       dbg(ERROR, "Looking at '%s'", arg);
       goto error_case_push_input_to_file_list;
    }

    // to be this far pch is not null, so it's pointing to say ":dbl.mbn"
    // where it is assumed arg is pointing to "123:dbl.mbn"

    if(pch - arg == 0)
    {
        // to be here means user typed in -d :dbl.mbn    <-- i.e. no number specified
        dbg(ERROR, "No image_id specified\nInput not of the form <image_id:image_file_name> WHERE image_id <10 characters, i.e. 123456789");
        dbg(ERROR, "Looking at '%s'", arg);
        goto error_case_push_input_to_file_list;
    }

    if(pch - arg > MAX_IMAGE_ID_SIZE - 1)
    {
        // to be here means user typed in -d 0123456789:dbl.mbn    <-- i.e. number is too huge
        dbg(ERROR, "image_id is too big\nInput not of the form <image_id:image_file_name> WHERE image_id <10 characters, i.e. 123456789");
        dbg(ERROR, "Looking at '%s'", arg);
        goto error_case_push_input_to_file_list;
    }

    memset(img_id, 0, sizeof(img_id));

    // enough checks below ensure pch-arg == MAX_IMAGE_ID_SIZE-1 (where -1 accounts for "\0")
    strlcpy(img_id, arg, pch - arg + 1);    // this returns strlen(arg)+1, but we are only copying (pch-arg) bytes
                                    // strlcpy ensures the null is on img_id
    if (strlen(img_id) > MAX_IMAGE_ID_SIZE) {
        LOGE ("img_id was truncated to '%s' while copying",img_id);
        dbg(ERROR, "Looking at '%s'", arg);
        goto error_case_push_input_to_file_list;
    }

    if(0 == (tail->image_id = atoi(img_id)))
    {
        dbg(ERROR, "image_id was not a number\nInput not of the form <image_id:image_file_name> ");
        dbg(ERROR, "img_id = '%s' and atoi(img_id)='%d'", img_id,atoi(img_id));
        dbg(ERROR, "Looking at '%s'", arg);
        goto error_case_push_input_to_file_list;
    }

    // checks above ensure &arg[pch-arg+1] is < 1024 bytes, size is fixed in kickstart_utils.h
    if(strlcpy(tail->file_name, &arg[pch-arg+1], strlen(arg)+1) >= strlen(arg)+1)
    {
        dbg (ERROR, "String was truncated while copying");
        dbg(ERROR, "Looking at '%s'", arg);
        goto error_case_push_input_to_file_list;
    }

    return SUCCESS;

error_case_push_input_to_file_list:

    if (tail->next != NULL) {
       free (tail->next);
       tail->next = NULL;
       free_count++;
    }
    return EFAILED;

}


/*===========================================================================
 *  METHOD:
 *  is_input_file_list_empty
 *
 *  DESCRIPTION:
 *  checks if the input file list is empty or not
 *
 *  PARAMETERS
 *  NONE
 *
 *  RETURN VALUE:
 *  NONE
 *  ===========================================================================*/
int is_input_file_list_empty(void)
{
    if (list_root->next == NULL)
    {
        return 1;
    }
    return 0;
}

/*===========================================================================
 *  METHOD:
 *  get_file_from_file_list
 *
 *  DESCRIPTION:
 *  finds the file matching the image id in the file list
 *
 *  PARAMETERS
 *  image_id     - image id
 *
 *  RETURN VALUE:
 *  char*        - matching file name or NULL
 *  ===========================================================================*/
char*  get_file_from_file_list(int image_id)
{
    struct list_node *node = list_root;
    unsigned int Counter = 0;

    dbg(INFO, "Looking for IMAGE_ID %i",image_id);

    if (list_root == NULL) {
       dbg(ERROR, "ERROR: list_root==NULL!! Can't look for IMAGE_ID %i in the list!\n\n",image_id);
       return NULL;
    }

    while(NULL != node->next)
    {
        Counter++;
        dbg(INFO, "Testing %.2i\t%2i:%s",Counter,node->image_id,node->file_name);

        if(node->image_id == image_id)
        {
            return node->file_name;
        }
        node = node->next;
    }
    dbg(ERROR, "ERROR: IMAGE_ID %i was not found in the list!\n\n",image_id);
    return NULL;
}

/*===========================================================================
 *  METHOD:
 *  startswith
 *
 *  DESCRIPTION:
 *  Checks whether string 'haystack' begins with string 'needle'
 *
 *  PARAMETERS
 *  haystack     - string to check within
 *  needle       - string to check for
 *
 *  RETURN VALUE:
 *  int          - 1 if haystack starts with needle, 0 otherwise
 *  ===========================================================================*/
int startswith(const char* haystack, const char* needle)
{
	if(strlen(needle) > strlen(haystack))
		return 0;
	int i;
	for(i = 0; needle[i] != '\0' && haystack[i] == needle[i]; i++);
	if (needle[i] == '\0')
		return 1;
	return 0;
}

/*===========================================================================
 *  METHOD:
 *  create_path
 *
 *  DESCRIPTION:
 *  Creates a directory path if it does not already exist
 *
 *  PARAMETERS
 *  path         - directory path to check for
 *  mode         - mode to be used for any directory creation
 *
 *  RETURN VALUE:
 *  int          - 0 if path is successfully created
 *  ===========================================================================*/
int create_path(const char* path, mode_t mode) {
        /* usage if(create_path(argv[1], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1) { successful } */
        struct stat status_buf;
        int retval;
        int permission_checked;
        permission_checked = 0;

        retval = stat(path, &status_buf);
        if (retval == 0) {
            if (access(path, W_OK) == -1) {
                dbg (WARN, "Does not have write permissions for '%s'. Continuing.", path);
            }
            return 0;
        }
        dbg (ERROR, "Path %s does not exist. Attempting to create..", path);
        int path_len = strlen(path);
        int i;
        char* temp_dir;
        for(i = 0; i < path_len; i++) {
                if (i > 0 && (path[i] == '/' || i == path_len - 1)) {
                    temp_dir = (char*) malloc(i+2);
                    if (NULL == temp_dir){
                            dbg (ERROR, "Memory allocation failure: %s", strerror(errno));
                            return -1;
                    }
                    malloc_count++;
                    strlcpy(temp_dir, path, i+2);
                    temp_dir[i+1] = '\0';
                    retval = stat(temp_dir, &status_buf);
                    if(retval != 0){
                            retval = mkdir(temp_dir, mode);
                            if (retval != 0) {
                                    dbg (ERROR, "Error creating directory '%s'. %s.\n", temp_dir, strerror(errno));
                                    return -1;
                            }
                            dbg (INFO, "Successfully created dir '%s'", temp_dir);
                    }
                    else {
                        if (permission_checked == 0 && access(temp_dir, W_OK) == -1) {
                            permission_checked = 1;
                            dbg (ERROR, "Does not have write permissions for %s. Pausing...", temp_dir);
                            sleep(25);
                        }
                    }
                    if (NULL!=temp_dir) {
                        free(temp_dir);
                        temp_dir = NULL;
                        free_count++;
                    }
                }
        }
        return 0;
}

int use_wakelock (wakelock_action action) {

    const char wake_lock_name[] = "kickstart";
    const char wake_lock_file[] = "/sys/power/wake_lock";
    const char wake_unlock_file[] = "/sys/power/wake_unlock";
    char *filename;
    int ret = 0;
    int wake_lock_name_length = strlen(wake_lock_name);
    int fd;

    if (action == WAKELOCK_ACQUIRE) {
        filename = wake_lock_file;
    }
    else {
        filename = wake_unlock_file;
    }
    fd = open(filename, O_WRONLY|O_APPEND);

    if (fd < 0) {
        dbg (ERROR, "Failed to open wake lock file %s: %s", filename, strerror(errno));
        return EFAILED;
    }

    ret = write(fd, wake_lock_name, wake_lock_name_length);
    if (ret != wake_lock_name_length) {
        dbg (ERROR, "Error writing to %s: %s", filename, strerror(errno));
        ret = EFAILED;
    }
    else {
        dbg (STATUS, "Wrote to %s", filename);
        ret = SUCCESS;
    }

    close(fd);
    return ret;
}

#ifdef LINUXPC
size_t strlcpy(char *dst, const char *src, size_t size) {
    size_t i = 0;
    while (i < size && ((dst[i] = src[i]) != '\0'))
        i++;
    if (i == size && size != 0)
        dst[i] = '\0';
    while (src[i])
        i++;
    return i;
}

size_t strlcat(char *dst, const char *src, size_t size) {
    size_t i = 0;
    // Iterate through till
    // 1. the end of the string in dst, or
    // 2. the end of the buffer indicated by size
    //
    while (i < size && dst[i] != '\0')
        i++;

    // IF we reach the end of the buffer space without having encountered a
    // NULL, OR
    // IF we reach the end of the string in dst and it turns out that
    // taking the NULL-termination into account there's no more space left,
    // THEN just do the simple calculation and return
    //
    if (i == size || (i == size - 1 && dst[i] == '\0'))
        return i+strlen(src);

    // Iterate only till size - 1 so as to leave space for the NULL
    while (i < size - 1 && ((dst[i] = src[i]) != '\0'))
        i++;

    // If we exited the loop not due to NULL-termination, then NULL terminate
    // the destination
    //
    if (i == size - 1) {
        i++;
        dst[i] = '\0';
    }
    return i;
}
#endif
