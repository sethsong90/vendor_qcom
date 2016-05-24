#ifndef ERR_H_
#define ERR_H_

/*----------------------------------------------------------------------------
 Copyright (c) 2007,2010 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential. 
----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>


static inline void __msg_printf(const char * fmt, const char * file, int line, int x1, int x2, int x3)
{
  printf(fmt,x1,x2,x3);
  printf("  :FILE %s:LINE %d:ARG1 %d:ARG2 %d:ARG3 %d:\n",file,line,x1,x2,x3);
}

#define MSG_PRINTF(lvl, fmt, file, line, a,b,c) \
do { \
  printf("%s :MSG ", lvl); \
  __msg_printf(fmt, file, line, a, b, c); \
} while(0)


#define ERR_FATAL(fmt, a,b,c)   { MSG_PRINTF(" FATAL",  fmt,__FILE__,__LINE__,(int)a,(int)b,(int)c); exit(1); }
#define ERR(fmt, a,b,c)         { MSG_PRINTF(" ERR",  fmt,__FILE__,__LINE__,(int)a,(int)b,(int)c); }

#endif /* ERR_H_ */
