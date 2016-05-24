/******************************************************************************
  @file:  gsiff_msg_glue_test.c

  DESCRIPTION
    Unit test for gsiff_msg_glue_test message queue implementation.

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
08/01/11   jb       1. Misc typedef changes

======================================================================*/

#include "comdef.h"
#include "test.h"
#include "msg_q.h"

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

typedef struct ctl_struct {
   size_t size;
   uint8_t type;
   uint32_t data;
} ctl_struct;

void* my_q;

void* test_func(void* arg)
{
   ctl_struct* p_tmp;
   fprintf(stderr, "Function should wait for approx 5 seconds\n");
   TEST(msg_q_rcv(my_q, (void**)&p_tmp) != eMSG_Q_SUCCESS);

   fprintf(stderr, "Woke up from unblock\n");

   return NULL;
}

int main (int argc, char *argv[])
{
   pthread_t threads[3];
   void(*dealloc)(void*) = free;

   TEST(msg_q_init(&my_q) == 0);

   ctl_struct* p_tmp = (ctl_struct*)malloc(sizeof(ctl_struct));
   p_tmp->type = 1;
   p_tmp->data = 0xDEADBEEF;
   TEST(msg_q_snd(my_q, p_tmp, dealloc) == eMSG_Q_SUCCESS);

   p_tmp = (ctl_struct*)malloc(sizeof(ctl_struct));
   p_tmp->type = 2;
   p_tmp->data = 0xCAFEBABE;
   TEST(msg_q_snd(my_q, p_tmp, dealloc) == eMSG_Q_SUCCESS);

   p_tmp = (ctl_struct*)malloc(sizeof(ctl_struct));
   p_tmp->type = 3;
   p_tmp->data = 0xCAFEDEED;
   TEST(msg_q_snd(my_q, p_tmp, dealloc) == eMSG_Q_SUCCESS);

   fprintf(stderr, "Finished adding elements\n");

   TEST(msg_q_rcv(my_q, (void**)&p_tmp) == eMSG_Q_SUCCESS);

   TEST(p_tmp->type == 1);
   TEST(p_tmp->data == 0xDEADBEEF);

   p_tmp = (ctl_struct*)malloc(sizeof(ctl_struct));
   p_tmp->type = 1;
   p_tmp->data = 0xDEADBEEF;
   TEST(msg_q_snd(my_q, p_tmp, dealloc) == eMSG_Q_SUCCESS);

   TEST(msg_q_rcv(my_q, (void**)&p_tmp) == eMSG_Q_SUCCESS);

   TEST(p_tmp->type == 2);
   TEST(p_tmp->data == 0xCAFEBABE);

   TEST(msg_q_rcv(my_q, (void**)&p_tmp) == eMSG_Q_SUCCESS);

   TEST(p_tmp->type == 3);
   TEST(p_tmp->data == 0xCAFEDEED);

   TEST(msg_q_rcv(my_q, (void**)&p_tmp) == eMSG_Q_SUCCESS);

   TEST(p_tmp->type == 1);
   TEST(p_tmp->data == 0xDEADBEEF);

   pthread_create(&threads[0], NULL, test_func, NULL);
   pthread_create(&threads[1], NULL, test_func, NULL);
   pthread_create(&threads[2], NULL, test_func, NULL);

   fprintf(stderr, "Sleeping for 5 seconds\n");
   sleep(5);

   /* Unblock the queue making it useless */
   TEST(msg_q_unblock(my_q) == 0);

   TEST(msg_q_rcv(my_q, (void**)&p_tmp) != eMSG_Q_SUCCESS);
   TEST(msg_q_rcv(my_q, (void**)&p_tmp) != eMSG_Q_SUCCESS);
   TEST(msg_q_rcv(my_q, (void**)&p_tmp) != eMSG_Q_SUCCESS);
   TEST(msg_q_rcv(my_q, (void**)&p_tmp) != eMSG_Q_SUCCESS);
   TEST(msg_q_rcv(my_q, (void**)&p_tmp) != eMSG_Q_SUCCESS);
   TEST(msg_q_rcv(my_q, (void**)&p_tmp) != eMSG_Q_SUCCESS);
   TEST(msg_q_rcv(my_q, (void**)&p_tmp) != eMSG_Q_SUCCESS);

   pthread_join(threads[0], NULL);
   pthread_join(threads[1], NULL);
   pthread_join(threads[2], NULL);

   fprintf(stderr, "Destroying message queue\n");
   TEST(msg_q_destroy(&my_q) == 0);

   return(0);
}

