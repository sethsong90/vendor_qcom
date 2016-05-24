/******************************************************************************
  @file:  gsiff_list_glue_test.c

  DESCRIPTION
    Unit test for gsiff_list_glue list implementation.

  INITIALIZATION AND SEQUENCING REQUIREMENTS

  -----------------------------------------------------------------------------
  Copyright (c) 2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

******************************************************************************/

/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
05/01/11   jb       Initial version

======================================================================*/

#include "comdef.h"
#include "test.h"
#include "linked_list.h"

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

typedef struct ctl_struct {
   size_t size;
   uint8 type;
   uint32 data;
} ctl_struct;

int main (int argc, char *argv[])
{
   void(*dealloc)(void*) = free;
   void* my_list;
   TEST(linked_list_init(&my_list) == eLINKED_LIST_SUCCESS);

   ctl_struct* p_tmp = (ctl_struct*)malloc(sizeof(ctl_struct));
   p_tmp->type = 1;
   p_tmp->data = 0xDEADBEEF;
   TEST(linked_list_add(my_list, p_tmp, dealloc) == eLINKED_LIST_SUCCESS);

   TEST(!linked_list_empty(my_list));

   p_tmp = (ctl_struct*)malloc(sizeof(ctl_struct));
   p_tmp->type = 2;
   p_tmp->data = 0xCAFEBABE;
   TEST(linked_list_add(my_list, p_tmp, dealloc) == eLINKED_LIST_SUCCESS);

   TEST(!linked_list_empty(my_list));

   p_tmp = (ctl_struct*)malloc(sizeof(ctl_struct));
   p_tmp->type = 3;
   p_tmp->data = 0xCAFEDEED;
   TEST(linked_list_add(my_list, p_tmp, dealloc) == eLINKED_LIST_SUCCESS);

   TEST(!linked_list_empty(my_list));

   TEST(linked_list_remove(my_list, (void**)&p_tmp) == eLINKED_LIST_SUCCESS);

   TEST(!linked_list_empty(my_list));
   TEST(p_tmp->type == 1);
   TEST(p_tmp->data == 0xDEADBEEF);

   TEST(linked_list_remove(my_list, (void**)&p_tmp) == eLINKED_LIST_SUCCESS);

   TEST(!linked_list_empty(my_list));
   TEST(p_tmp->type == 2);
   TEST(p_tmp->data == 0xCAFEBABE);

   TEST(linked_list_remove(my_list, (void**)&p_tmp) == eLINKED_LIST_SUCCESS);

   TEST(linked_list_empty(my_list));
   TEST(p_tmp->type == 3);
   TEST(p_tmp->data == 0xCAFEDEED);

   TEST(linked_list_flush(my_list) == 0);

   TEST(linked_list_empty(my_list));

   /* Remove from empty list */
   TEST(linked_list_remove(my_list, (void**)&p_tmp) != eLINKED_LIST_SUCCESS);

   p_tmp = (ctl_struct*)malloc(sizeof(ctl_struct));
   p_tmp->type = 1;
   p_tmp->data = 0xDEADBEEF;
   TEST(linked_list_add(my_list, p_tmp, dealloc) == eLINKED_LIST_SUCCESS);

   TEST(!linked_list_empty(my_list));

   p_tmp = (ctl_struct*)malloc(sizeof(ctl_struct));
   p_tmp->type = 1;
   p_tmp->data = 0xDEADBEEF;
   TEST(linked_list_add(my_list, p_tmp, dealloc) == eLINKED_LIST_SUCCESS);

   TEST(!linked_list_empty(my_list));

   p_tmp = (ctl_struct*)malloc(sizeof(ctl_struct));
   p_tmp->type = 1;
   p_tmp->data = 0xDEADBEEF;
   TEST(linked_list_add(my_list, p_tmp, dealloc) == eLINKED_LIST_SUCCESS);

   TEST(!linked_list_empty(my_list));

   p_tmp = (ctl_struct*)malloc(sizeof(ctl_struct));
   p_tmp->type = 1;
   p_tmp->data = 0xDEADBEEF;
   TEST(linked_list_add(my_list, p_tmp, dealloc) == eLINKED_LIST_SUCCESS);

   TEST(!linked_list_empty(my_list));

   TEST(linked_list_destroy(&my_list) == eLINKED_LIST_SUCCESS);

   TEST(my_list == NULL);

   return(0);
}

