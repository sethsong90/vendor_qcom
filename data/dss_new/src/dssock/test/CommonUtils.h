#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H
/*===========================================================================
  @file CommonUtils.h

  This file is a helper containing functions, structures used in the test
  module

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/main/latest/
  test/CommonUtils.h $
  $DateTime: 2009/09/28 14:45:31 $ $Author: dmudgal $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "qtf_msgr_msg.h"
#include "msg.h"

/*===========================================================================

MACRO PS_QTF_OBJECTIVE

DESCRIPTION
  This macro will print the objective of the test passed in the standard
  format.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
#define PS_QTF_OBJECTIVE(objective) \
  TF_MSG("OBJECTIVE %s:", objective)
#define QTF_TEST_OBJECTIVE_HERE(here, args) \
    do \
    {  \
      TF_MSG("OBJECTIVE <<" here "\n" args ) ; \
      TF_MSG("\n" here ":"); \
    } while (0)

// This macro will print the name of the group that wrote the test.
#define PS_QTF_OWNER(owner) \
  TF_MSG("OWNER %s:", owner);

// This marro will print the
#define STA_TEST_SUBSYSTEM(subsystem) \
  TF_MSG("SUBSYSTEM %s:",subsystem);

#define PS_QTF_GROUP(group) \
  if(group) TF_MSG("GROUP %s:",group);

#define STA_TEST_SUBGROUP(subgroup) \
  if(subgroup) TF_MSG("SUBGROUP %s:",subgroup);


/*===========================================================================
 

MACRO PS_QTF_VERSION

DESCRIPTION
  This macro will print the version of the test passed in the standard
  format.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/

#define PS_QTF_VERSION(version) \
  TF_MSG("VERSION <<HERE\n%s\nHERE:", version)

/*===========================================================================

MACRO PS_QTF_CLASSIFICATION

DESCRIPTION
  This macro will print the classifications of the test passed in the
  standard format.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
#define PS_QTF_CLASSIFICATION(class) \
  TF_MSG("CLASSIFICATION %s:", class)

#define QTF_FUNCTIONAL "FUNCTIONAL"
#define STA_ADVERSARIAL "ADVERSARIAL"
#define STA_ADVERSARIAL_ITERATIVE "ADVERSARIAL ITERATIVE"
#define STA_PERFORMANCE "PERFORMANCE"
 
 

/*===========================================================================

                   CDPS UNIT TEST TAGS HEADER FILE

===========================================================================*/
#define PS_QTF_CDPS_SMOKE_TEST(i)\
        TF_MSG("CDPS_SMOKE %s:",i?"TRUE":"FALSE");
#define PS_QTF_CDPS_REGRESSION_TEST(i)\
        TF_MSG("CDPS_REGRESSION %s:",i?"TRUE":"FALSE");
#define STA_CDPS_MANUAL_TEST(i)\
        TF_MSG("CDPS_MANUAL %s:", i?"TRUE":"FALSE");
#define STA_CDPS_TIME_CONSUMING_TEST(i)\
        TF_MSG("CDPS_TIME_CONSUMING %s:", i?"TRUE":"FALSE");
#define STA_CDPS_1X_TEST(i)\
        TF_MSG("CDPS_1X %s:",i?"TRUE":"FALSE");
#define STA_CDPS_UMTS_TEST(i)\
        TF_MSG("CDPS_UMTS %s:",i?"TRUE":"FALSE");
#define STA_CDPS_WLAN_TEST(i)\
        TF_MSG("CDPS_WLAN %s:",i?"TRUE":"FALSE");
#define STA_CDPS_IPV6_TEST(i)\
        TF_MSG("CDPS_IPV6 %s:",i?"TRUE":"FALSE");
#define STA_CDPS_ROHC_TEST(i)\
        TF_MSG("CDPS_ROHC %s:",i?"TRUE":"FALSE");

#define STA_CDPS_DEVEL_TEST(i)\
        TF_MSG("CDPS_DEVEL %s:",i?"TRUE":"FALSE");

/*===========================================================================

                     STRUCTURE DECLARATIONS

===========================================================================*/
struct PosixTaskMsg
{
  msgr_hdr_s  hdr;
  int         msgType;
  void    ( * startRoutine )( void * );
  void  *     arg;
};

#endif /* COMMON_UTILS_H */
