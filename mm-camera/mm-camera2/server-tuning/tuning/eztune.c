/***************************************************************************
* Copyright (c) 2010-2013 Qualcomm Technologies, Inc.  All Rights Reserved.*
* Qualcomm Technologies Proprietary and Confidential.                      *
***************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <string.h>
#include "eztune_items.h"
#include "eztune.h"

extern eztune_item_t *eztune_items;
static uint16_t num_to_start = 0;
int eztune_item_counter = 0;
static my_list_t  apply_clist;
static int8_t eztune_status = 0;
static eztune_t eztune_ctrl;

/** eztune_get_status:
 *
 *  @void: NULL
 *
 *  This function returns the host status of the eztune
 *  connection
 *
 *  Return: 1 for successful connection and  0 for unsuccessful
**/

int8_t eztune_get_status(void)
{
  return eztune_status;
}

/** eztune_set_status:
 *
 *  @int: ez_enable 1 for enable
 *
 *  This function sets the host status of the eztune connection
 *
 *  Return: void
**/

static void eztune_set_status(int ez_enable)
{
  eztune_status = ez_enable;
}

/** eztune_initialize:
 *
 *  @ctrl: eztune control structure pointer
 *  @client_socketid: client socuket id
 *
 *  This function Intialize the tuning parameters
 *
 *  Return: 0 for success and negative error on failure
**/

static int eztune_initialize(eztune_t *ezctrl,
  mm_camera_lib_handle *lib_handle, char *send_buf, uint32_t send_len) {
  if (!lib_handle) {
    CDBG_EZ_ERROR("%s %d NULL\n", __func__, __LINE__);
    return -1;
  }
  CDBG_EZ("%s %d\n", __func__, __LINE__);
  ezctrl->lib_handle = lib_handle;
  init_list(&apply_clist.list);
  tuning_process_get(ezctrl->lib_handle, TUNING_GET_CHROMATIX,
    &ezctrl->get_data);
  ezctrl->chromatixptr = (chromatix_parms_type *)
    &(ezctrl->get_data.chromatix.chromatixData);
  ezctrl->snapchromatixptr = (chromatix_parms_type *)
    &(ezctrl->get_data.chromatix.snapchromatixData);
  ezctrl->common_chromatixptr = (chromatix_VFE_common_type *)
    &(ezctrl->get_data.chromatix.common_chromatixData);
  tuning_process_get(ezctrl->lib_handle, TUNING_GET_AFTUNE,
    &ezctrl->get_data);
  ezctrl->af_tune_ptr = (af_tune_parms_t *)
    &(ezctrl->get_data.autofocus.af_tuneData);
  tuning_set_vfe(ezctrl,
    VFE_MODULE_ALL, SET_STATUS, 1);
  tuning_set_pp(ezctrl,
    PP_MODULE_ALL, SET_STATUS, 1);
  tuning_set_3a(ezctrl, EZ_STATUS, 1);

  eztune_set_status (1);
  ezctrl->fn_table.eztune_status = eztune_get_status;
  CDBG_EZ("%s %d\n", __func__, __LINE__);
  return 0;

}

/** eztune_deinitialize:
 *
 *  @ctrl: eztune control structure pointer
 *
 *  This function Deintialize the tuning parameters
 *
 *  Return: 0 for success and negative error on failure
**/

static int eztune_deinitialize(eztune_t *ezctrl,
  char *send_buf, uint32_t send_len) {

  if (!ezctrl) {
    CDBG_EZ_ERROR("%s %d NULL\n", __func__, __LINE__);
    return -1;
  }
  CDBG_EZ("%s %d\n", __func__, __LINE__);
  tuning_set_vfe(ezctrl,
    VFE_MODULE_ALL, SET_STATUS, 0);
  tuning_set_pp(ezctrl,
    PP_MODULE_ALL, SET_STATUS, 0);
  tuning_set_3a(ezctrl, EZ_STATUS, 0);
  eztune_set_status (0);
  ezctrl->lib_handle = 0;
  CDBG_EZ("%s %d\n", __func__, __LINE__);
  return 0;

}

/** eztune_get_list_process:
 *
 *  @ez: pointer to the recv command
 *  @ctrl: eztune control structure pointer
 *
 *  This function sends the host with the parameter list
 *
 *  Return: 0 for success and negative error on failure
**/

int32_t eztune_get_list_process(void *ez, void *ctrl,
  char *send_buf, uint32_t send_len) {

  int rc = 0;
  int offset_buflen = 3;
  uint16_t i = 0;
  uint16_t total_item = EZT_PARMS_MAX;

  if (send_len < EZTUNE_MAX_SEND) {
     CDBG_EZ_ERROR("%s:Invalid send_len=%d\n", __func__, send_len);
     return -1;
  }

  if (send_buf == NULL ) {
     CDBG_EZ_ERROR("%s:send_buf is NULL\n", __func__);
     return -1;
  }
  eztune_t *ezctrl = (eztune_t *) ctrl;

  CDBG_EZ("offset_buflen = %d\n", offset_buflen);
  CDBG_EZ("num_to_start = %d\n", num_to_start);

  do {
    eztune_item_t item = eztune_get_item(num_to_start);
    CDBG_EZ("item.name = %s\n", item.name);
    CDBG_EZ("item.id = %d\n", item.id);

    if ((offset_buflen + 8 + strlen(item.name) + 1) > EZTUNE_MAX_SEND) {
      CDBG_EZ("for next buffer items!!!!");
      memcpy(send_buf, "\x01", 1);
      break;
    }
    memcpy(send_buf + offset_buflen, &num_to_start, 2);
    offset_buflen += 2;
    CDBG_EZ("offset_buflen_item.index = %d\n", offset_buflen);

    if (item.offset) {
      memcpy(send_buf + offset_buflen, &item.size, 2);
    } else {
      memcpy(send_buf + offset_buflen, "\x00\x00", 2);
    }
    offset_buflen += 2;
    CDBG_EZ("offset_buflen_item.offset = %d\n", offset_buflen);

    if (item.reg_flag == EZT_WRITE_FLAG) {
      memcpy(send_buf + offset_buflen, "\x00\x00\x00\x00", 4);
    }else if (item.reg_flag == EZT_READ_FLAG || item.reg_flag == EZT_3A_FLAG) {
      memcpy(send_buf + offset_buflen, "\x01\x00\x00\x00", 4);
    } else if (item.reg_flag == EZT_CHROMATIX_FLAG) {
      memcpy(send_buf + offset_buflen, "\x40\x00\x00\x00", 4);
    } else if (item.reg_flag == EZT_AUTOFOCUS_FLAG) {
      memcpy(send_buf + offset_buflen, "\x00\x04\x00\x00", 4);
    } else if (item.reg_flag == (EZT_AUTOFOCUS_FLAG | EZT_READ_FLAG)) {
      memcpy(send_buf + offset_buflen, "\x01\x04\x00\x00", 4);
    }

    offset_buflen += 4;
    CDBG_EZ("offset_buflen_item.type = %d\n", offset_buflen);

    strlcpy(send_buf + offset_buflen, item.name, strlen(item.name) + 1);
    offset_buflen = offset_buflen + strlen(item.name) + 1;
    CDBG_EZ("offset_buflen_item.name = %d\n", offset_buflen);

    num_to_start += 1;
    i++;

    if (total_item <= num_to_start) {
      memcpy(send_buf, "\x00", 1);
      num_to_start = 0;
      break;
    }
  } while (1);

  memcpy(send_buf + 1, &i, 2);

  CDBG_EZ("eztune_get_list_process: num_to_start = %d\n",
    num_to_start);

  return rc;
}

/** eztune_add_to_list:
 *
 *  @item: eztune item to be added in the list
 *
 *  This function adds the item to the list
 *
 *  Return: 0 for success and negative error on failure
**/

static int eztune_add_to_list(eztune_set_val_t item)
{
  my_list_t * tmp;
  tmp = ( my_list_t*)malloc(sizeof(my_list_t));
  if (!tmp) {
    perror("malloc");
    exit(1);
  }
  CDBG_EZ("%s %d item.item_num=%d\n", __func__, __LINE__, item.item_num);
  tmp->data.item_num = item.item_num;
  tmp->data.table_index = item.table_index;
  tmp->data.value_string = (char *) malloc(strlen (item.value_string)+1);
  if (!tmp->data.value_string) {
    free(tmp);
    perror("malloc error ");
    exit(1);
  }
  strlcpy(tmp->data.value_string, item.value_string,
    strlen(item.value_string) + 1);
  add_node(&(tmp->list), &(apply_clist.list));
  CDBG_EZ("eztune_add_to_list: LIST HEad2 = 0x%x\n",
    (uint32_t)(&apply_clist.list));
  CDBG_EZ("eztune_add_to_list: Added element to list and index is %d,\
    item_num=%d\n", tmp->data.table_index, tmp->data.item_num );
  CDBG_EZ("eztune_add_to_list: list pointer is tmp->list=0x%x, tmp is 0x%x\n",
    (uint32_t)(&(tmp->list)), (uint32_t)(tmp));

  usleep(1000);
  eztune_item_counter++;
  return TRUE;
}

/** eztune_print_list:
 *
 *  @void: NULL
 *
 *  This function to print the items in the list
 *
 *  Return: void
**/

static void eztune_print_list(void)
{
  struct ezlist *pos;
  struct ezlist *clist = &(apply_clist.list);
  my_list_t *tmp;
  CDBG_EZ("eztune: printList\n");
  for (pos = clist->next ; pos != clist ; pos = pos->next ) {
    tmp= member_of(pos, my_list_t , list);
    CDBG_EZ("eztune: item_num = %d,table_index = %d,value is %s\n",
      tmp->data.item_num, tmp->data.table_index,tmp->data.value_string);
  }
}

/** eztune_delete_list:
 *
 *  @void: NULL
 *
 *  This function deletes the items in the list
 *
 *  Return: void
**/

static void eztune_delete_list(void)
{
  struct ezlist * pos, *q;
  struct ezlist *clist = &(apply_clist.list);
  my_list_t * tmp;
  CDBG_EZ("eztune: delete_list\n");

  for (pos =(clist)->next, q =pos->next; pos != (clist);
    pos = q, q = pos->next) {
    tmp= member_of(pos, my_list_t,list);
 /* CDBG_EZ("eztune_apply_changes: Freeing item_num = %d,
      Table_index = %d,value is %s\n",
      tmp->data.item_num, tmp->data.table_index,
      tmp->data.value_string); */
    del_node(pos);
    free(tmp->data.value_string) ;
    free(tmp);
  }
  eztune_item_counter = 0;
}

/** eztune_set_parms_process:
 *
 *  @ez: pointer to the recv command
 *  @ctrl: eztune control structure pointer
 *
 *  This function sets the values of the items in the list
 *
 *  Return: 0 for success and negative error on failure
**/

int32_t eztune_set_parms_process(void *ez, eztune_t *ezctrl,
  char *send_buf, uint32_t send_len) {

  int i;
  int rc = 0;
  eztune_set_val_t item;
  uint16_t num_items = *(uint16_t *)ez;

  if (send_len < EZTUNE_MAX_SEND) {
     CDBG_EZ_ERROR("%s:Invalid send_len=%d\n", __func__, send_len);
     return -1;
  }

  if (send_buf == NULL ) {
     CDBG_EZ_ERROR("%s:send_buf is NULL\n", __func__);
     return -1;
  }

  ez = (uint8_t *)ez + sizeof(uint16_t);
  CDBG_EZ("eztune_set_parms_process: num_items = %d\n", num_items);

  CDBG_EZ("eztune_set_parms_process: LIST HEad = 0x%x\n",
    (uint32_t)(&apply_clist.list));

  for (i = 0; i < num_items; ++i) {
    item.item_num = *(uint16_t *)ez;
    ez = (uint8_t *)ez + sizeof(uint16_t);
    item.table_index = *(uint16_t *)ez;
    ez = (uint8_t *)ez + sizeof(uint16_t);
    item.value_string = (char *) malloc(strlen (ez)+1);
    if (!item.value_string) {
      CDBG_EZ(" %s malloc \n ", __func__);
      exit(1);
    }
    strlcpy(item.value_string, ez, strlen(ez) + 1);
    ez = (uint8_t *)ez + strlen (ez) + 1;
    rc = eztune_add_to_list(item);
    free(item.value_string);
  }
  if (i ==  num_items) {
    memcpy(send_buf , "\x01", 1);
  }
  return rc;
}

/** eztune_apply_changes:
 *
 *  @ctrl: eztune control structure pointer
 *
 *  This function triggers the cahnged values of the items in
 *  the listto be applied in the respective modules
 *
 *  Return: 0 for success and negative error on failure
**/

static int32_t eztune_apply_changes(eztune_t *ezctrl)
{
  struct ezlist * pos;
  struct ezlist *clist = &(apply_clist.list);
  my_list_t * tmp;
  int32_t cnt = 0;
  eztune_item_t temp;
  static int chromatix_item_count = 0;
  static int autofocus_item_count = 0;

  if (!ezctrl) {
    CDBG_EZ_ERROR("%s:ezctrl is NULL\n", __func__);
     return -1;
  }

  CDBG_EZ("eztune_apply_changes: LIST HEad = 0x%x\n",
    (uint32_t)(&apply_clist.list));

  for ( pos = clist->next ; pos != clist ; pos = pos->next ) {
    /*list_for_each(pos,&apply_clist.list)*/

    /* at this point: pos->next points to the next item's 'list' variable and
     * pos->prev points to the previous item's 'list' variable. Here item is
     * of type struct kool_list. But we need to access the item itself not the
     * variable 'list' in the item! macro list_entry() does just that. See "How
     * does this work?" below for an explanation of how this is done.
     */
    tmp= member_of(pos, my_list_t , list);

    temp=eztune_get_item( tmp->data.item_num);
    /* given a pointer to struct ezlist1, type of data structure it is part of,
     * and it's name (struct ezlist1's name in the data structure) it returns a
     * pointer to the data structure in which the pointer is part of.
     * For example, in the above line list_entry() will return a pointer to the
     * struct kool_list item it is embedded in!
     */
    CDBG_EZ("eztune_apply_changes: list pointer is tmp->list=0x%x,\
      list is 0x%x tmp is 0x%x\n", (uint32_t)(&(tmp->list)),
      (uint32_t)pos, (uint32_t)(tmp));
    CDBG_EZ("eztune_apply_changes: Got element at index  %d\n",
      tmp->data.table_index);
    eztune_change_item(tmp, ezctrl);

    if(temp.type == EZT_T_CHROMATIX)
      chromatix_item_count++;
    if((temp.id >= EZT_PARMS_AFTUNE_PROCESSTYPE) &&
       (temp.id < EZT_PARMS_AFTUNE_TUNING_TEST_LINEAR_ENABLE))
      autofocus_item_count++;
  }

  CDBG_EZ("eztune_apply_changes: Applied all the changes \n");
  if(chromatix_item_count > 0) {
    tune_set_data_t set_data;
    set_data.chromatix = &ezctrl->get_data.chromatix;
    CDBG_EZ("%s calling reload chromatix\n", __func__);
    tuning_process_set(ezctrl->lib_handle, TUNING_SET_RELOAD_CHROMATIX,
      &set_data);
    chromatix_item_count = 0;
  }
 if(autofocus_item_count > 0) {
    tune_set_data_t set_data;
    set_data.autofocus = &ezctrl->get_data.autofocus;
    CDBG_EZ("%s calling reload autofoucs\n", __func__);
    tuning_process_set(ezctrl->lib_handle, TUNING_SET_RELOAD_AFTUNE,
      &set_data);
    autofocus_item_count = 0;
  }
  eztune_delete_list();
  CDBG_EZ("eztune_apply_changes: Deleted the list\n");

  return TRUE;
}

/** eztune_misc_cmds_process:
 *
 *  @ez: pointer to the receive command
 *  @ctrl: eztune control structure pointer
 *
 *  This function process the miscellaneous commands from the
 *  host
 *
 *  Return: 0 for success and negative error on failure
**/

int32_t eztune_misc_cmds_process(void *ez, void *ctrl,
  char *send_buf, uint32_t send_len) {

  int rc = 0;
  uint8_t message_id;
  eztune_t *ezctrl;

  if (send_len < 128) {
     CDBG_EZ_ERROR("%s:Invalid send_len=%d\n", __func__, send_len);
     return -1;
  }

  if (send_buf == NULL ) {
     CDBG_EZ_ERROR("%s:send_buf is NULL\n", __func__);
     return -1;
  }

  message_id = *(uint8_t *)ez;
  CDBG_EZ("eztune_misc_cmds_process: message_id = %d\n",message_id);
  if (!ctrl) {
    CDBG_EZ("%s NULL control\n", __func__);
    return FALSE;
  }
  ezctrl = (eztune_t *) ctrl;

  CDBG_EZ("eztune_misc_cmds_process: message_id = %d\n",message_id);

  switch (message_id) {
    case EZTUNE_MISC_GET_VERSION:
      strlcpy(send_buf, "2.1.0", strlen("2.1.0") + 1);
      break;

    case EZTUNE_MISC_APPLY_CHANGES:
      rc = eztune_apply_changes(ezctrl);

      if (rc <  0)
        memcpy(send_buf,"\x01", 1);
      break;
    default:
      break;
  }
  return TRUE;
}

/** tuning_command_process:
 *
 *  @ez: pointer to the recv command
 *  @cmd: tuning command for processing
 *
 *  This function procees the eztune commands
 *
 *  Return: 0 for success and negative error on failure
**/

int32_t tuning_command_process(void *ez_recv, mm_camera_tune_cmd_t cmd, void *param,
  char *send_buf, uint32_t send_len) {

  int rc = 0;
  eztune_t *ezctrl = (eztune_t *)&eztune_ctrl;

  switch (cmd) {
     case TUNE_CMD_INIT:
        rc = eztune_initialize(ezctrl, (mm_camera_lib_handle *)ez_recv, send_buf, send_len);
      break;
    case TUNE_CMD_GET_LIST:
      rc = eztune_get_list_process(ez_recv, ezctrl, send_buf, send_len);
      break;
    case TUNE_CMD_GET_PARAMS:
      rc = eztune_get_parms_process(ez_recv, ezctrl, send_buf, send_len);
      break;
    case TUNE_CMD_SET_PARAMS:
      rc = eztune_set_parms_process(ez_recv, ezctrl, send_buf, send_len);
      break;
    case TUNE_CMD_MISC:
      rc = eztune_misc_cmds_process(ez_recv, ezctrl, send_buf, send_len);
      break;
    case TUNE_CMD_DEINIT:
      rc = eztune_deinitialize(ezctrl, send_buf, send_len);
      break;
     default:
       break;
  }

  return rc;
}

/** tuning_prevcommand_process:
 *
 *  @ez: pointer to the recv command
 *  @cmd: tuning command for processing
 *
 *  This function procees the eztune commands
 *
 *  Return: 0 for success and negative error on failure
**/

int32_t tuning_prevcommand_process(void *ez_recv, mm_camera_tune_prevcmd_t cmd, void *param,
  char **send_buf, uint32_t *send_len) {

  int rc = 0;

  CDBG_EZ("%s  %d cmd =%d\n", __func__, __LINE__, cmd);
  switch (cmd) {
     case TUNE_PREVCMD_INIT:
       rc = eztune_preview_init(*(int *)param);
      break;
     case TUNE_PREVCMD_SETDIM:
       rc = eztune_preview_set_dimension(*(cam_dimension_t *)param);
       break;
     case TUNE_PREVCMD_DEINIT:
       rc = eztune_preview_deinit();
      break;
     case TUNE_PREVCMD_GETINFO:
       rc = eztune_prev_get_info(send_buf, send_len);
       break;
     case TUNE_PREVCMD_GETCHUNKSIZE:
       rc = eztune_prev_ch_cnk_size(*(uint32_t *)param, send_buf, send_len);
       break;
     case TUNE_PREVCMD_GETFRAME:
       rc = eztune_get_preview_frame(send_buf, send_len);
       break;
     case TUNE_PREVCMD_UNSUPPORTED:
       rc = eztune_prev_write_status(send_buf, send_len);
       break;
     default:
       break;
  }

  return rc;
}

static mm_camera_tune_func_t tuning_func_ptr = {
 .command_process = tuning_command_process,
 .prevcommand_process = tuning_prevcommand_process,
 .prevframe_callback = eztune_preview_usercb,
};

/** open_tuning_lib:
 *
 *  @void:
 *
 *  This function return the pointer to the processing function
 *  for tuning commands
 *
 *  Return: void *
**/
void* open_tuning_lib(void) {

  return &tuning_func_ptr;
}
