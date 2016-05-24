/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                           P S _ U T I L S . C

GENERAL DESCRIPTION
  Collection of utility functions being used by various modules in PS.
  Most of these functions assume that the caller is in PS task context.

Copyright (c) 2011 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================*/
#include <sys/time.h>
#include "ps_utils.h"
#include "msg.h"
#include "errno.h"

/*===========================================================================

FUNCTION msclock

DESCRIPTION
  This function will return the time of the day in milliseconds

DEPENDENCIES
  None

RETURN VALUE
  Returns the time of day.

SIDE EFFECTS
  None
===========================================================================*/

dword msclock()
{
  int ret = -1;
  struct timeval now;
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ret = gettimeofday(&now, NULL);
  if(ret == 0) 
  {
    MSG_MED("msclock:time = %u.%06u", now.tv_sec, now.tv_usec, 0);
    return (now.tv_sec * 1000);
  }
  MSG_ERROR("msclock:gettimeofday() failed,errno = %d\n", errno, 0, 0);
  return -1;
}
