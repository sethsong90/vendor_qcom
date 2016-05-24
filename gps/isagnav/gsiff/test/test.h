/******************************************************************************
  @file:  test.h

  DESCRIPTION
    Simple Macro used for unit testing.

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

#ifndef __GSIFF_TEST_H__
#define __GSIFF_TEST_H__

#include <stdio.h>

#define TEST(EX) (void)((EX) || \
(fprintf( stderr, "FAIL: %s File: %s Line: %d\n" , #EX, __FILE__, __LINE__)))

#endif /* __GSIFF_TEST_H__ */
