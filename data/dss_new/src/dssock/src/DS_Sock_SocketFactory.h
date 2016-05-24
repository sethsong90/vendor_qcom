#ifndef DS_SOCK_SOCKETFACTORY_H
#define DS_SOCK_SOCKETFACTORY_H
/*===========================================================================
  @file DS_Sock_SocketFactory.h

  This file defines the class that implements the ISocketFactory interface.
  In addition, it also implements Factory interface in order to maintain a
  list of ISocket objects that are created using SocketFactory object. This
  list is used to relay events from various modules to all ISocket objects.

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_SocketFactory.h#2 $
  $DateTime: 2010/03/02 16:38:08 $ $Author: hmurari $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "DS_Utils_CSSupport.h"
#include "DS_Net_IPolicy.h"
#include "DS_Net_INetwork.h"
#include "DS_Sock_ISocketFactory.h"
#include "DS_Sock_Socket.h"
#include "DS_Utils_Factory.h"


/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace DS
{
  namespace Sock
  {
    //TODO Add class documentation
     /*lint -esym(1510, IQI) */
     /*lint -esym(1510, ISocketFactory) */
    class SocketFactory : public ISocketFactory, public DS::Utils::Factory /*lint !e578 */
    {
      public:
        /**
          @brief Returns a SocketFactory object.

          Returns a SocketFactory object. Since this method implements
          Singleton pattern, an object is created only on the first
          invocation, and subsequent invocations return the same object.

          This instance doesn't support ICMP sockets.

          @param None

          @retval address  Socket factory is created successfully
          @retval 0        Out of memory
        */
        static SocketFactory * CreateInstance
        (
          void
        ) throw();

        /**
          @brief Creates a socket using default network policy.

          An instance of ISocket interface is created using the given family,
          sockType, and protocol parameters. A default network policy is used
          to determine the network interface on which this socket sends and
          receives data.

          DS::Sock::Family, DS::Sock::Type, and DS::Sock::Protocol define the
          supported options for family, sockType, and protocol respectively.
          If protocol is UNSPEC, TCP is used for STREAM sockets and UDP is
          used for DGRAM sockets.

          If protocol is ICMP, this method checks if ICMP socket creation is
          supported by calling a virtual function IsICMPSupported(). This class
          doesn't support ICMP sockets but a derived class can support them
          by overrriding IsICMPSupported().

          Newly created socket is added to the list of ISocket objects.

          This method returns error if
            @li Unsupported values are passed for family, sockType, or protocol
            @li TCP is not used as protocol for STREAM sockets
            @li UDP or ICMP is not used as protocol for DGRAM sockets
            @li newSockPtrPtr is NULL

          @param[in]  family        The family of socket to be created
          @param[in]  sockType      The type of socket to be created
          @param[in]  protocol      The protocol to be used
          @param[out] newSockPtrPtr Updated with the created socket

          @retval DS::Error::SUCCESS             Socket is created successfully
          @retval DS::Error::DSS_EINVAL          Family is invalid or
                                                 newSockPtrPtr is already
                                                 populated
          @retval DS::Error::DSS_ESOCKNOSUPPORT  Type is not supported
          @retval DS::Error::DSS_EPROTONOSUPPORT Protocol is not supported
          @retval DS::Error::DSS_EPROTOTYPE      (sockType, protocol)
                                                 combination is not
                                                 supported
          @retval DS::Error::DSS_EFAULT          NULL newSockPtrPtr
          @retval DS::Error::DSS_ENOMEM          Out of memory

          @see DS::Sock::Family, DS::Sock::Type, DS::Sock::Protocol, DS::Error
        */
        virtual DS::ErrorType CDECL CreateSocket
        (
          FamilyType    family,
          SocketType    sockType,
          ProtocolType  protocol,
          ISocket **    newSockPtrPtr
        );

        /**
          @brief Creates a socket using user specified network policy.

          An instance of ISocket interface is created using the given family,
          sockType, and protocol parameters. The user specified network policy
          is used to determine the network interface on which this socket
          sends and receives data.

          DS::Sock::Family, DS::Sock::Type, and DS::Sock::Protocol define the
          supported options for family, sockType, and protocol respectively.
          If protocol is UNSPEC, TCP is used for STREAM sockets and UDP is
          used for DGRAM sockets.

          If protocol is ICMP, this method checks if ICMP socket creation is
          supported by calling a virtual function IsICMPSupported(). This class
          doesn't support ICMP sockets but a derived class can support them
          by overrriding IsICMPSupported().

          Newly created socket is added to the list of ISocket objects.

          This method returns error if
            @li Unsupported values are passed for family, sockType, or protocol
            @li TCP is not used as protocol for STREAM sockets
            @li UDP or ICMP is not used as protocol for DGRAM sockets
            @li netPolicyPtr is NULL
            @li newSockPtrPtr is NULL

          @param[in]  family        The family of socket to be created
          @param[in]  sockType      The type of socket to be created
          @param[in]  protocol      The protocol to be used
          @param[in]  netPolicyPtr  The policy to be used in data path
          @param[out] newSockPtrPtr Updated with the created socket

          @retval DS::Error::SUCCESS             Socket is created successfully
          @retval DS::Error::DSS_EINVAL          Family is invalid or
                                                 newSockPtrPtr is already
                                                 populated
          @retval DS::Error::DSS_ESOCKNOSUPPORT  Type is not supported
          @retval DS::Error::DSS_EPROTONOSUPPORT Protocol is not supported
          @retval DS::Error::DSS_EPROTOTYPE      (sockType, protocol)
                                                 combination is not valid
          @retval DS::Error::DSS_EFAULT          NULL netPolicyPtr or
                                                newSockPtrPtr
          @retval DS::Error::DSS_ENOMEM          Out of memory

          @see DS::Sock::Family, DS::Sock::Type, DS::Sock::Protocol,
               DS::Net::IPolicy, DS::Error
        */
        virtual DS::ErrorType CDECL CreateSocketByPolicy
        (
          FamilyType          family,
          SocketType          sockType,
          ProtocolType        protocol,
          DS::Net::IPolicy *  netPolicyPtr,
          ISocket **          newSockPtrPtr
        );

        /**
          @brief Creates a socket and binds it to user specified network
                 instance.

          An instance of ISocket interface is created using the given family,
          sockType, and protocol parameters. The socket is bound to the user
          specified network instance and the associated network interface is
          used to send and receive data.

          DS::Sock::Family, DS::Sock::Type, and DS::Sock::Protocol define the
          supported options for family, sockType, and protocol respectively.
          If protocol is UNSPEC, TCP is used for STREAM sockets and UDP is
          used for DGRAM sockets.

          If protocol is ICMP, this method checks if ICMP socket creation is
          supported by calling a virtual function IsICMPSupported(). This class
          doesn't support ICMP sockets but a derived class can support them
          by overrriding IsICMPSupported().

          Newly created socket is added to the list of ISocket objects.

          This method returns error if
            @li Unsupported values are passed for family, sockType, or protocol
            @li TCP is not used as protocol for STREAM sockets
            @li UDP or ICMP is not used as protocol for DGRAM sockets
            @li networkPtr is NULL
            @li newSockPtrPtr is NULL

          @param[in]  family        The family of socket to be created
          @param[in]  sockType      The type of socket to be created
          @param[in]  protocol      The protocol to be used
          @param[in]  networkPtr    The network instance to be used in data path
          @param[out] newSockPtrPtr Updated with the created socket

          @retval DS::Error::SUCCESS             Socket is created successfully
          @retval DS::Error::DSS_EINVAL          Family is invalid or
                                                 newSockPtrPtr is already
                                                 populated
          @retval DS::Error::DSS_ESOCKNOSUPPORT  Type is not supported
          @retval DS::Error::DSS_EPROTONOSUPPORT Protocol is not supported
          @retval DS::Error::DSS_EPROTOTYPE      (sockType, protocol)
                                                 combination is not valid
          @retval DS::Error::DSS_EFAULT          NULL networkPtr or
                                                newSockPtrPtr
          @retval DS::Error::DSS_ENOMEM          Out of memory

          @see DS::Sock::Family, DS::Sock::Type, DS::Sock::Protocol,
               DS::Net::INetwork, DS::Error
        */
        virtual DS::ErrorType CDECL CreateSocketByNetwork
        (
          FamilyType           family,
          SocketType           sockType,
          ProtocolType         protocol,
          DS::Net::INetwork *  networkPtr,
          ISocket **           newSockPtrPtr
        );

        /**
          @brief Deletes a Socket object from the list of ISocket objects.

          Socket object is deleted from the list of ISocket objects maintained
          by the SocketFactory object. From this point onwards, events are
          not relayed to the deleted Socket object.

          @param[in] sockPtr  The socket object to be deleted

          @retval DS::Error::SUCCESS      Socket is deleted successfully
          @retval DS::Error::DSS_EFAULT   NULL Socket object as input

          @see DS::Error
        */
        DS::ErrorType DeleteSocket
        (
          Socket *  sockPtr
        ) throw();

        /*-------------------------------------------------------------------
          Defintions of IQI Methods
        -------------------------------------------------------------------*/
        DSIQI_IMPL_DEFAULTS_SINGLETON( ISocketFactory);

      protected:
        /**
          @brief Constructor for SocketFactory class.

          Initializes a SocketFactory object.

          @param None

          @retval None
        */
        SocketFactory
        (
          void
        ) throw();

        /**
          @brief Destructor for SocketFactory class.

          Resets the socket factory object. Method is defined as virtual
          so that destructors of derived classes are called when Socket
          object is deleted.

          @param None

          @retval None
        */
        virtual ~SocketFactory
        (
          void
        ) throw();

        /**
          @brief Indicates if this socket factory supports creation of
                 ICMP sockets.

          Socket factory checks if a class supports ICMP socket creation using
          this method. This class doesn't support them but derived classes
          can override this method.

          @param None

          @retval true  If ICMP sockets are supported
          @retval false If ICMP sockets are not supported
        */
        virtual bool IsICMPSupported
        (
          void
        );

      private:
        /**
          @brief Overloaded operator new.

          Operator new is overloaded so that memory for SocketFactory is
          allocated from ps_mem instead of from the system heap.

          As the C Runtime Library throws a std::bad_alloc exception if the
          memory allocation fails, and since memory allocation could fail as
          memory is allocated from ps_mem, this method also handles the
          exception.

          @param[in] numBytes  The amount of memory to be allocated

          @retval address  Ptr to object if memory is allocated
          @retval 0        If memory couldn't be allocated
        */                                                 /*lint -e{1511} */
        static void * operator new
        (
          unsigned int numBytes
        ) throw(); 

        /**
          @brief Overloaded operator delete.

          Operator delete is overloaded so that memory for SocketFactory is
          de-allocated and is put back in to ps_mem

          @param[in] bufPtr  The object to be deleted

          @retval None
        */                                                 /*lint -e{1511} */
        static void operator delete
        (
          void *  bufPtr
        ) throw(); 

        /**
          @brief Ptr to singleton instance of this class.

          Set to a non-NULL when CreateInstance(...) is called the first time.
          All subsequent calls to CreateInstance(...) get this instance.
        */
        static SocketFactory *  sockFactoryPtr;

    }; /* class SocketFactory */
  } /* namespace Sock */
} /* namespace DS */

#endif /* DS_SOCK_SOCKETFACTORY_H */
