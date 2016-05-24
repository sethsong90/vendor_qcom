#ifndef DS_NET_NETWORK_ACTIVE_H
#define DS_NET_NETWORK_ACTIVE_H

/*===========================================================================
  @file DS_Net_NetworkActive.h

  This file defines the NetworkActive class, which is derived from the
  Network class.

  When created, active networks perform a bring-up of the network interface
  they associate with. When the network object is returned, it is not
  guaranteed that the network interface is brought up. The clients need to
  register for state changed events on the returned object to check when the
  interface is brought up.

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_NetworkActive.h#1 $ 
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

    class NetworkActive : public Network
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
      NetworkActive()

      @brief
      NetworkActive class constructor.

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
      NetworkActive
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
      Initiates termination to bring down PPP and the traffic channel. After
      successful teardown of the network connection, a signal is set to
      inform the application that the PPP connection has been closed.

      If there are other applications using the network interface, then Stop()
      can succeed immediately without actual teardown of the iface.

      @param      None

      @return     AEE_SUCCESS - on success
      @return     QDS_EFAULT - Invalid arguments

      @see        GoNull()
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
      This method is only supported for INetworkPriv interface. For
      INetworkPriv, the network object can be created without having to bring
      it up first (Lookup mode). This is required to register for events on
      iface ids, to support backward compatibility. New applications should use
      only INetwork interface, wherein getting the interface would guarentee
      Network bringup.

      @param      None.

      @return     AEE_SUCCESS - on success
      @return     AEE_EWOULDBLOCK - If the network interface bringup is blocking
      and would complete asynchronously. Applications should register
      for state changed event in this scenario.

      @see        INetworkPriv
      */
      virtual ds::ErrorType CDECL BringUpInterface
      (
        void
      );

    };/* class Network */
  } /* namespace Net */
} /* namespace ds */

#endif /* DS_NET_NETWORK_ACTIVE_H */

