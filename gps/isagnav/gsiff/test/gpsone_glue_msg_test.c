/******************************************************************************
  @file:  gpsone_glue_msg_test.c

  DESCRIPTION
    Unit test for gpsone_glue_msg queue implementation.

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
#include "gpsone_glue_msg.h"

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#define GPSONE_TEST_MSG_Q_PATH "/tmp/gpsone_test_q"

typedef struct ctl_struct {
   size_t size;
   uint8 type;
   uint32 data;
} ctl_struct;

int main (int argc, char *argv[])
{
   int msgqid = gpsone_glue_msgget(GPSONE_TEST_MSG_Q_PATH, O_RDWR);

   ctl_struct tmp;
   tmp.type = 1;
   tmp.data = 0xDEADBEEF;
   TEST(gpsone_glue_msgsnd(msgqid, &tmp, sizeof(tmp)) == sizeof(tmp));

   tmp.type = 2;
   tmp.data = 0xCAFEBABE;
   TEST(gpsone_glue_msgsnd(msgqid, &tmp, sizeof(tmp)) == sizeof(tmp));

   /* Test receiving order and for data integrity */
   memset(&tmp, 0, sizeof(tmp));
   TEST(gpsone_glue_msgrcv(msgqid, &tmp, sizeof(tmp)) == sizeof(tmp));
   TEST(tmp.type == 1);
   TEST(tmp.data == 0xDEADBEEF);

   memset(&tmp, 0, sizeof(tmp));
   TEST(gpsone_glue_msgrcv(msgqid, &tmp, sizeof(tmp)) == sizeof(tmp));
   TEST(tmp.type == 2);
   TEST(tmp.data == 0xCAFEBABE);

   TEST(gpsone_glue_msgremove(GPSONE_TEST_MSG_Q_PATH, msgqid) == 0);

   return(0);
}

