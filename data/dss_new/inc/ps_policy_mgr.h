#ifndef PS_POLICY_MGR_H
#define PS_POLICY_MGR_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                       P S _ P O L I C Y _ M G R . H


GENERAL DESCRIPTION
  This file defines the API exported by policy manager module

EXTERNAL FUNCTIONS

  PS_POLICY_MGR_ADD_RECORD()
    Adds a new record in the Policy database

  PS_POLICY_MGR_CLEAN_DB()
    Cleans up the Policy database

  PS_POLICY_MGR_REG_GET_PROFILE_CBACK()
    Registers DS function for retrieving profile based on app_identifier

Copyright (c) 2007-2009 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_policy_mgr.h#1 $
  $Author: maldinge $ $DateTime: 2011/01/10 09:44:56 $
===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

/*===========================================================================

                      EXTERNAL DATA DECLARATIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  IFACE_PRIORITY_MAX - indicates the size of ref_cnt array in iface_private
    structure - [n]

  PRIORITY_MAX -indicates max priority values supported per iface - [n - 1]
---------------------------------------------------------------------------*/
#define PS_POLICY_MGR_IFACE_PRIORITY_MAX      32
#define PS_POLICY_MGR_PRIORITY_MAX            (PS_POLICY_MGR_IFACE_PRIORITY_MAX - 1)

/*---------------------------------------------------------------------------
  Legacy priority value used for apps/users when policy database is
  uintialized.
  Legacy priority will have highest priority, so it does not get affected
  by arbitration.
---------------------------------------------------------------------------*/
#define PS_POLICY_MGR_LEGACY_PRIORITY         PS_POLICY_MGR_PRIORITY_MAX

/*---------------------------------------------------------------------------
  Dont care values are used by users of Policy manager to indicate that the 
  user is not interested in matching of that particular field (profile / 
  app_identifier) while priority is retrieved from the Policy Manager
  database.
---------------------------------------------------------------------------*/
#define PS_POLICY_MGR_PROFILE_DONT_CARE       0x7FFFFFFF
#define PS_POLICY_MGR_APP_DONT_CARE           0x7FFFFFFF

/*---------------------------------------------------------------------------
  Any application/profile value dictate the default entry behavior in Policy
  Manager.
  Multiple default profiles of types [ANY_APP, ANY_PROFILE],
  [particular app, ANY_PROFILE] or [ANY_APP, particular profile] of which the
  type [ANY_APP, ANY_PROFILE] would be serving as the default entry matching
  any sort of inputs.

  Considering inputs are not matched to any other entry in database, default
  entries will be hit in the following manner (provided order is maintained).

  [app, ANY_PROFILE]     - will match inputs (APP_DONT_CARE, profile)
  [ANY_APP, profile]     - will match inputs (app, PROFILE_CONT_CARE)
  [ANY_APP, ANY_PROFILE] - will match inputs (app, profile)

  Subsequently these ANY_* values cannot be used as input during get_priority,
  and SHOULD only be used while filling entries into the database.

  The onus of the order of filling the deafult profile entries in the Policy
  database is on the mode handler. Policy Manager by itself will not be
  monitoring order of records filled in the database.
---------------------------------------------------------------------------*/
#define PS_POLICY_MGR_ANY_PROFILE             0x7EFEFEFE
#define PS_POLICY_MGR_ANY_APP                 0x7FEFEFEF

/*---------------------------------------------------------------------------
  Invalid values are used to indicate unsuccessful operations of
  get_profile() and get_priority().
---------------------------------------------------------------------------*/
#define PS_POLICY_MGR_PRIORITY_INVALID        (-1)
#define PS_POLICY_MGR_PROFILE_INVALID         (-1)

/* TODO: planned to move these to ps_policy_mgr_config.h, 
so max entries can be configured externally */
#define PS_POLICY_MGR_MAX_NUM_PROFILE_SUPPORTED 16
#define PS_POLICY_MGR_MIN_NUM_DEFAULT_ENTRIES    1

#define PS_POLICY_MGR_MAX_POLICY_ENTRIES        \
          (PS_POLICY_MGR_MAX_NUM_PROFILE_SUPPORTED + \
           PS_POLICY_MGR_MIN_NUM_DEFAULT_ENTRIES)

/*---------------------------------------------------------------------------
  Callback type registered by DS for retrieving profile based on application
  identifier.
---------------------------------------------------------------------------*/
typedef int32 (* get_app_profile_cb)(uint32 app_identifier);

/*===========================================================================

                        PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION PS_POLICY_MGR_ADD_RECORD()

DESCRIPTION
  Adds an entry in the policy database.

PARAMETERS
  app_identifier : App identifier value
  profile_id     : Profile of the App
  priority       : App profile's priority

RETURN VALUE
  0 on SUCCESS 
  -1 on FAILURE

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_policy_mgr_add_record
(
  int64  app_identifier,
  int32  profile_id,
  int32  priority
);

/*===========================================================================
FUNCTION PS_POLICY_MGR_CLEAN_DB()

DESCRIPTION
  Cleans up the Policy database.

PARAMETERS
  None

RETURN VALUE
  void

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_policy_mgr_clean_db
(
  void
);

/*===========================================================================
FUNCTION PS_POLICY_MGR_REG_GET_PROFILE_CBACK()

DESCRIPTION
  Registers callback from DS to get application profile based on
  application identifier.

PARAMETERS
  cback_f_ptr  : Ptr to DS function to be registered

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_policy_mgr_reg_get_profile_cback
(
  get_app_profile_cb cback_f_ptr
);

#endif /* PS_POLICY_MGR_H */
