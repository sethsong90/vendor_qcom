#ifndef DS_NET_NETWORKFACTORY_CLIENT_H
#define DS_NET_NETWORKFACTORY_CLIENT_H
/*===========================================================================
  @file ds_Net_NetworkFactoryClient.h

  This file defines the class that implements the INetworkFactory and
  INetworkFactoryPriv interfaces. It stores the client privleges
  and act as a proxy to the NetworkFactory singelton.
  When a client calls CreateInstance for creating a network factory,
  an instance of NetworkFactoryClient is returned to the client.
  Client shall use this instance to create network objects.

                    Copyright (c) 2010 Qualcomm Technologies, Inc.
                    All Rights Reserved.
                    Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_NetworkFactoryClient.h#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "ds_Utils_CSSupport.h"
#include "ds_Net_INetwork.h"
#include "ds_Net_INetworkFactory.h"
#include "ds_Net_INetworkFactoryPriv.h"
#include "ds_Net_IPolicy.h"
#include "ds_Utils_Factory.h"
#include "ds_Errors_Def.h"
#include "ds_Net_INatSession.h"
#include "ds_Net_INatSessionPriv.h"

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
    /**
      Forward deceleration for NetworkFactory
    */
    class NetworkFactory;

    /*lint -esym(1510, IQI) */
    /*lint -esym(1510, INetworkFactory) */
    /*lint -esym(1510, INetworkFactoryPriv) */
    class NetworkFactoryClient : public INetworkFactory,
                                public INetworkFactoryPriv
    {                                               /*lint !e578 */
      private:

        IPrivSet          * privSetPtr;
        NetworkFactory    * networkFactoryPtr;

        /*!
        @brief Constructor for NetworkFactoryClient class.

        Initializes a NetworkFactoryClient object.

        @param[in]  privSetPtr    Privileges of the client
        @param[in]  socketFactory NetworkFactory instance that will be used
                                  internally for creating network objects.

        @retval NetworkFactoryClient instance
        */
        NetworkFactoryClient
        (
          IPrivSet          * privSetPtr,
          NetworkFactory    * networkFactory
        )throw();

        /*!
        @brief Destructor for NetworkFactoryClient class.

        Resets the NetworkFactoryClient object. Method is defined as virtual
        so that destructors of derived classes are called when Socket
        object is deleted.

        @param None

        @retval None
        */
        virtual ~NetworkFactoryClient() throw();

        /*!
        @brief Overloaded new operator

        */
        void* operator new (unsigned int num_bytes) throw();

        /*!
        @brief Overloaded delete operator.

        */
        void operator delete (void* objPtr) throw();

    public:

        /*-------------------------------------------------------------------------
          Inherited functions from IQI
        -------------------------------------------------------------------------*/
        DSIQI_IMPL_DEFAULTS_NO_QI()

        /*!
        @function
        QueryInterface()

        @brief
        QueryInterface method of NetworkFactoryClient object.

        @details
        The NetworkFactoryClient object implements the following interfaces:
        IQI
        INetworkFactory
        INetworkFactoryPriv
        All of these interfaces can be got by using the QueryInterface() method
        using appropriate IID.

        @param[in]  iid - Interface Id of the required interface.
        @param[out] ppo - Pointer to the interface returned.

        @see        IQI::QueryInterface()

        @return     AEE_SUCCESS on success
        @return     AEE_ECLASSNOSUPPORT - if the IID is not supported.
        @return     QDS_EFAULT - on invalid inputs.
        */
        virtual int CDECL QueryInterface
        (
          AEEIID                                  iid,
          void **                                 ppo
        );

        /*!
        @function
        CreateInstance()

        @brief
        Returns a NetworkFactoryClient object.

        @param[in] clsID  ClassID for the requested underlying factory.
                          Supported class IDs are:
                          ds::Net::AEECLSID_CNetworkFactory,
                          ds::Net::AEECLSID_CNetworkFactoryService,
                          ds::Net::AEECLSID_CNetworkFactoryPriv,
                          ds::Net::AEECLSID_CNetworkFactoryPrivService.
                          For any other class a NULL will be returned.
        @param[in] privSetPtr  Privileges of the client.

        @retval address  Network factory is created successfully
        @retval 0        Out of memory or unsupported class ID.
        */
        static void* CreateInstance
        (
          AEECLSID   clsID,
          IPrivSet * privSetPtr
        );

        /*-------------------------------------------------------------------------
          Definitions of methods that INetworkFactory and
          INetworkFactoryPriv implement.
          Since the NetworkFactoryClient is only a proxy to the NetworkFactory
          please refer to ds_Net_NetworkFactory.h for the documentation
          on these functions. The only difference with the functions defined
          in ds_Net_NetworkFactory.h is that on ds_Net_NetworkFactory.h
          there is an additional parameter that specifies the privilege.
          The NetworkFactoryClient specifies the privilege
          of the specific client when forwarding the request for network object
          creation to the NetworkFactory.
        -------------------------------------------------------------------------*/
        /*-------------------------------------------------------------------------
          Inherited functions from INetworkFactory
        -------------------------------------------------------------------------*/
        /*!
        @function
        CreateNetwork()

        @brief
        This function creates network object based on the network policy object
        passed in, or default network in case of NULL policy.

        @details
        This function is used to create a network object using the default
        IPolicy object in case of NULL policy, or allows applications to specify a
        network policy instead of just using default policy. Network objects in regular
        or monitored mode can be created using this method.

        @param[in]  networkMode - ACTIVE/MONITORED
        @param[in]  pIPolicy    - IPolicy object specifying the policy,
        if NULL default policy shall be used.
        @param[out] ppINetwork  - Returned INetwork object.
        @param[in]  privSetPtr  - Privileges of the client

        @see        ds::Net::NetworkModeType

        @return     SUCCESS if the network object can be created successfully.
        @return     QDS_EFAULT - Invalid arguments.
        @return     DSS_ENOMEM - Out of memory.
        */

        virtual ds::ErrorType CDECL CreateNetwork
        (
          NetworkModeType     networkMode,
          IPolicy*            pIPolicy,
          INetwork**          ppINetwork
        );

        /*!
        @brief
        Creates instance of IPFilterSpec class.

        @details
        This function creates an instance of IPFilterSpec. IPFilterSpec
        creation is supported only via INetworkFactory.

        @param[out]  ppIIPFilterSpec - Output IPFilterSpec instance.
        @retval      ds::SUCCESS IPFilterSpec created successfully.
        @retval      Other DS designated error codes might be returned.
        */
        virtual ds::ErrorType CDECL CreateIPFilterSpec
        (
          IIPFilterPriv**         ppIIPFilterSpec
        );

        /*!
        @brief
        Creates instance of QoSFlowSpec class.

        @details
        This function creates an instance of QoSFlowSpec. QoSFlowSpec
        creation is supported only via INetworkFactory.

        @param[out]  ppIQoSSpec - Output interface.
        @retval      ds::SUCCESS Object created successfully.
        @retval      Other DS designated error codes might be returned.
        */
        virtual ds::ErrorType CDECL CreateQoSFlowSpec
        (
          IQoSFlowPriv**          ppIQoSSpec
        );

        /*!
        @brief
        Creates instance of Policy object.

        @details
        This function creates an instance of Policy. Policy object
        creation is supported only via INetworkFactory.

        @param[out]  ppIPolicy - Output interface.
        @retval      ds::SUCCESS Object created successfully.
        @retval      Other DS designated error codes might be returned.
        */
        virtual ds::ErrorType CDECL CreatePolicy
        (
          IPolicy**           ppIPolicy
        );

        /*!
        @brief
        This function creates an instance of ITechUMTS object.

        @details
        This function creates an instance of ITechUMTS. ITechUMTS
        object creation is supported only via INetworkFactory.

        @param[out]  newTechUMTS - Output interface.
        @retval      AEE_SUCCESS Object created successfully.
        @retval      Other DS designated error codes might be returned.
        */
        virtual ds::ErrorType CDECL CreateTechUMTS
        (
          ITechUMTS** newTechUMTS
        );

        /*-------------------------------------------------------------------------
        Inherited functions from INetworkFactoryPriv
        -------------------------------------------------------------------------*/
        /*!
        @brief
        Creates instance of INetworkPriv object.

        @details
        This function creates an instance of INetworkPriv. INetworkPriv
        object creation is supported only via INetworkFactory.

        @param[in]   pIPolicy - Policy object to be associated with INetwork.
        If NULL then the default policy will be used.
        @param[out]  ppIPolicy - Output interface.
        @retval      ds::SUCCESS Object created successfully.
        @retval      Other DS designated error codes might be returned.
        */
        virtual ds::ErrorType CDECL CreateNetworkPriv
        (
          IPolicyPriv*        pIPolicyPriv,
          INetworkPriv**      ppINetworkPriv
        );
        /*!
        @brief
        Creates instance of IPolicyPriv object.

        @details
        This function creates an instance of IPolicyPriv. IPolicyPriv
        object creation is supported only via INetworkFactory.

        @param[out]  ppIPolicy - Output interface.
        @retval      ds::SUCCESS Object created successfully.
        @retval      Other DS designated error codes might be returned.
        */
        virtual ds::ErrorType CDECL CreatePolicyPriv
        (
          IPolicyPriv**       ppIPolicy
        );

        /*!
        @brief
        Creates an instance of INatSessionPriv interface.

        @param[in]  pIPolicy - Policy object to be used for NAT Priv session.
        @param[out] ppINatSessionPriv - Output interface.

        @return     DS::SUCCESS - on success
        @return     DSS_EFAULT - Invalid arguments
        @return     DSS_ENOMEM - No memory to create NAT session.

        @see        Interface: INatSessionPriv.
        */
        virtual ds::ErrorType CDECL CreateNatSessionPriv
        (
          ds::Net::IPolicyPriv          *pIPolicy,
          ds::Net::INatSessionPriv     **ppINatSessionPriv
        );
    }; /* class NetworkFactoryClient */
  } /* namespace Net */
} /* namespace ds */

#endif /* DS_NET_NETWORKFACTORY_CLIENT_H */
