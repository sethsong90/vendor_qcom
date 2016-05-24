#ifndef DS_NET_QOS_UMTS_H
#define DS_NET_QOS_UMTS_H

#if 0
/*===========================================================================
  @file QoSUMTS.h

  This file defines the class that implements the IQoSUMTS 
  interface.

  TODO: Detailed explaination about the class here.

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_QoSUMTS.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-03-25 hm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Net_IQoSUMTS.h"
#include "ds_Net_QoS.h"
#include "ds_Net_NetworkUMTS.h"
#include "ds_Net_Platform.h"

namespace ds
{
  namespace Net
  {

    class QoSUMTS : public IQoSUMTS, 
                    public QoS,
                    public NetworkUMTS
    {
    private:

    public:
      QoSUMTS();
      virtual ~QoSUMTS();

    }; /* class QoSUMTS */

  } /* namespace Net */
} /* namespace ds */

#endif 

#endif
