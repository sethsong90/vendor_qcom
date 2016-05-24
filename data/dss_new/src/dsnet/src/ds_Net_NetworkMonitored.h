#ifndef DS_NET_NETWORK_MONITORED_H
#define DS_NET_NETWORK_MONITORED_H

/*===========================================================================
  @file DS_Net_NetworkMonitored.h

  This file defines the NetworkMonitored class, which is derived from the
  Network class.


  Monitored mode network objects are used by special applications that just
  need to monitor the state of an interface (Example: UI application). When
  such a network object is created, it associates with a certain interface,
  but does not try to bring it up.

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_NetworkMonitored.h#1 $ 
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-09-13 ea  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "ds_Net_Network.h"

/*===========================================================================
                     FORWARD DECLERATION
===========================================================================*/
struct IPrivSet;

/*===========================================================================

                     PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace ds
{
  namespace Net
  {

    class NetworkMonitored : public Network
    {
    protected:
      /*!
      @function
      ProcessIfaceStateEvent()

      @brief
      Internal method that performs processing of the iface state event.

      @details
      Special processing for iface state is required by the network object.
      1. If the network object's earlier bringup failed because of CLOSE_IN_PROG
      then DSNET needs to perform bring up again.
      2. If IFACE_DOWN event is reported, then network object needs to update
      the last network down reason.

      @param[in]  pEventInfo - Event info pointer passed by EventManager.
      @return     None.
      @see        ds::Net::EventManager
      */
      virtual void ProcessIfaceStateEvent
      (
        EventInfoType   *pEventInfo
      );

    public:
      /*!
      @function
      NetworkMonitored()

      @brief
      NetworkMonitored class constructor.

      @details
      NetworkActice class constructor. NetworkActice object always must have a
      corresponding policy object associated. To create a NetworkActice object,
      use the CreateNetwork() method from INetworkFactory, with QDS_ACTIVE as
      the networkMode parameter.

      @param[in]  pIPolicy- Pointer to the network policy object.
      @param[in]  privSetPtr- Privileges provided by client.
      @return     Returns pointer to network object created.
      @see        INetworkFactory
      */
      NetworkMonitored
      (
        Policy   *pPolicy,
        IPrivSet *privSetPtr
      );

      /*!
      @function
      Stop()

      @brief
      This function stops the network connection.

      @details
      Not supported for NetworkMonitored.

      @param      None
      */
      virtual ds::ErrorType CDECL Stop
      (
        void
      );

      /*!
      @function
      BringUpInterface()

      @brief
      Bring up the network interface.

      @details
      Not supported for NetworkMonitored

      @param      None
      */
      virtual ds::ErrorType CDECL BringUpInterface
      (
        void
      );

    };/* class Network */
  } /* namespace Net */
} /* namespace ds */

#endif /* DS_NET_NETWORK_MONITORED_H */

