#ifndef DS_SOCK_ICMPSOCKET_H
#define DS_SOCK_ICMPSOCKET_H
/*===========================================================================
  @file DS_Sock_ICMPSocket.h

  This file defines the class which implements the ISocket interface for ICMP
  sockets.

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_ICMPSocket.h#2 $
  $DateTime: 2009/10/09 10:48:28 $ $Author: hmurari $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "DS_Sock_Socket.h"


/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace DS
{
  namespace Sock
  {
    class ICMPSocket : public Socket                          /*lint !e578 */
    {
      public:
        virtual ~ICMPSocket() throw() {}

        static ICMPSocket * CreateInstance
        (
          FamilyType     _family
        );

        virtual DS::ErrorType CDECL Shutdown
        (
          ShutdownDirType  shutdownDir
        );
        
        /*-------------------------------------------------------------------
          IQI interface Methods
        -------------------------------------------------------------------*/
        DSIQI_RELEASE()

      protected:
        virtual bool IsConnectSupported
        (
          const SockAddrIN6Type *  v6RemoteAddrPtr,
          DS::ErrorType *          dsErrnoPtr
        );

        virtual bool IsOptSupported
        (
          OptLevelType  optLevel,
          OptNameType   optName
        );

        virtual bool IsSetNetPolicySupported
        (
          void
        );

        virtual DS::ErrorType FillProtocolInfoInPktInfo
        (
          const SockAddrIN6Type *  v6RemoteAddrPtr,
          const SockAddrIN6Type *  v6LocalAddrPtr,
          ip_pkt_info_type *       pktInfoPtr
        );

        virtual void ProcessNetworkConfigChangedEvent
        (
          DS::ErrorType  reasonForChange
        );

        virtual DS::ErrorType CDECL GetDoSAckInfo
        (
          DS::Sock::DoSAckStatusType *  dosAckStatusPtr,
          uint32 *                      overflowPtr
        )
        {
           // TODO implement
           (void) dosAckStatusPtr;
           (void) overflowPtr;
           return 0;
        }

      private:
        static void * operator new
        (
          unsigned int numBytes
        ) throw();

        static void operator delete
        (
          void *  bufPtr
        ) throw();

    }; /* class ICMPSocket */
  } /* namespace Sock */
} /* namespace DS */

#endif /* DS_SOCK_ICMPSOCKET_H */
