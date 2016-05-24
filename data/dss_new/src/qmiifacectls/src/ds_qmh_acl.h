#ifndef DSQMH_ACL_H
#define DSQMH_ACL_H
/*===========================================================================


                       Q M I   M O D E   H A N D L E R

	     R O U T I N G   A C C E S S   C O N T R O L   L I S T
  		           D E F I N I T I O N S
 
GENERAL DESCRIPTION
  This routing ACL is used to enable policy and address based routing
  across the QMI Proxy IFACE.

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2008 by Qualcomm Technologies, Inc.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datahlos/interface/qmiifacectls/rel/11.01.01/src/ds_qmh_acl.h#1 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
07/21/08    ar     Cleanup header to unused elements.
05/06/08    ar     Created module/initial version.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#if defined (FEATURE_DATA_PS) && defined (FEATURE_QMI_CLIENT)

#include "ps_acl.h"


/*===========================================================================

                 ACCESS CONTROL LIST NAME DEFINITIONS

===========================================================================*/

ACL_DEF( dsqmhacl_rt_acl );

ACL_POST_PROC_DEF( dsqmhacl_rt_post_proc );

#endif /* FEATURE_DATA_PS && FEATURE_QMI_CLIENT */

#endif    /* DSQMH_ACL_H */



