#ifndef DS_NET_CLASSIDINSTANTIATOR_H
#define DS_NET_CLASSIDINSTANTIATOR_H
/*===========================================================================
  @file ds_Net_ClassIDInstantiator.h

  This file defines various methods which are used to create instances of
  network interfaces.

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/inc/ds_Net_ClassIDInstantiator.h#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "AEEStdErr.h"
#include "ds_Utils_CSSupport.h"


/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
DSIQI_DECL_CREATE_INSTANCE2( ds, Net, NetworkFactory)

#endif /* FEATURE_DATA_PS */
#endif /* DS_NET_CLASSIDINSTANTIATOR_H */
