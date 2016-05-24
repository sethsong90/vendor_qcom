/*===========================================================================
           P S _ D N S _ C A C H E _ C O N F I G . C

DESCRIPTION
  This file contains the access function to get the compile time parameters.

EXTERNALIZED FUNCTIONS
  ps_dns_cache_getconst()
    This function gets the value of the constant parameter asked for.

INITIALIZATION AND SEQUENCING REQUIREMENTS
  None.

Copyright (c) 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_dns_cache_config.c#1 $
  $Author: zhasan $
  $DateTime: 2011/06/17 12:02:33 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
06/30/10    ea     Initial development.

===========================================================================*/
/*===========================================================================

                             INCLUDE FILES

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS
#ifdef FEATURE_DATA_PS_DNS

#include "ps_dns_cache_config.h"
#include "msg.h"
#include "ds_Utils_DebugMsg.h"

/*===========================================================================

                      PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/*===========================================================================
FUNCTION PS_DNS_CACHE_GETCONST

DESCRIPTION
  Function called by DNS library to acquire a configurable compile time
  constant.

DEPENDENCIES
  None.

RETURN VALUE
   0 on Success
   -1 on  Failure

SIDE EFFECTS
  None.
===========================================================================*/
int16 ps_dns_cache_getconst
(
  ps_dns_cache_const_enum_type const_param,
  void                       * const_val,
  uint32                       const_val_size
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2( "ps_dns_cache_getconst():  called.  Args: param=%d, val=%p, size=%d",
            const_param, const_val, const_val_size );

  switch( const_param )
  {
    case PS_DNS_CACHE_CONST_MAX_ENTRIES:
      if ( NULL == const_val || const_val_size < sizeof(uint32) )
      {
        LOG_MSG_ERROR("Insuff memory %d for const %d; const buffer = %p",
          const_val_size, const_param, const_val );
        return -1;
      }
      *((uint32*)const_val) = PS_DNS_CACHE_MAX_ENTRIES;
      break;

    case PS_DNS_CACHE_CONST_MIN_TTL:
      if ( NULL == const_val || const_val_size < sizeof(uint32) )
      {
        LOG_MSG_ERROR("Insuff memory %d for const %d; const buffer = %p",
          const_val_size, const_param, const_val );
        return -1;
      }
      *((uint32*)const_val) = PS_DNS_CACHE_MIN_TTL;
      break;

    case PS_DNS_CACHE_CONST_MAX_TTL:
      if ( NULL == const_val || const_val_size < sizeof(uint32) )
      {
        LOG_MSG_ERROR("Insuff memory %d for const %d; const buffer = %p",
          const_val_size, const_param, const_val );
        return -1;
      }
      *((uint32*)const_val) = PS_DNS_CACHE_MAX_TTL;
      break;

    case PS_DNS_CACHE_CONST_NEGATIVE_TTL:
      if ( NULL == const_val || const_val_size < sizeof(uint32) )
      {
        LOG_MSG_ERROR("Insuff memory %d for const %d; const buffer = %p",
          const_val_size, const_param, const_val );
        return -1;
      }
      *((uint32*)const_val) = PS_DNS_CACHE_NEGATIVE_TTL;
      break;

    default:
      LOG_MSG_ERROR( "Unknown parameter %d", const_param, 0, 0 );
      return -1;
  }

  return 0;
} /* ps_dns_cache_getconst() */

#endif /* FEATURE_DATA_PS_DNS */
#endif /* FEATURE_DATA_PS */
