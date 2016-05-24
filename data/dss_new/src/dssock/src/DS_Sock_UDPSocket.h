#ifndef DS_SOCK_UDPSOCKET_H
#define DS_SOCK_UDPSOCKET_H
/*===========================================================================
  @file DS_Sock_UDPSocket.h

  This file defines the class which implements the ISocket interface for UDP
  sockets.

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_UDPSocket.h#2 $
  $DateTime: 2009/10/09 10:48:28 $ $Author: hmurari $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "DS_Sock_Socket.h"
#include "ps_iface_defs.h"


/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace DS
{
  namespace Sock
  {
    class UDPSocket : public Socket                           /*lint !e578 */
    {
      public:
        virtual ~UDPSocket() throw() {}

        static UDPSocket * CreateInstance
        (
          FamilyType    family
        );

        virtual DS::ErrorType CDECL SendToDSMChain
        (
          dsm_item_type **                       dsmItemPtrPtr,
          const DS::Sock::SockAddrStorageType *  remoteAddrPtr,
          unsigned int                           flags,
          int32 *                                numBytesSentPtr
        );

        virtual DS::ErrorType CDECL RecvFromDSMChain
        (
          dsm_item_type **                 dsmItemPtrPtr,
          DS::Sock::SockAddrStorageType *  remoteAddrPtr,
          unsigned int                     flags,
          int32 *                          numBytesRcvdPtr
        );

        virtual DS::ErrorType CDECL AddIPMembership
        (
          const IPMembershipInfoType *  ipMembershipPtr
        );

        virtual DS::ErrorType CDECL DropIPMembership
        (
          const IPMembershipInfoType *  ipMembershipPtr
        );

        virtual DS::ErrorType CDECL GetDoSAckInfo
        (
          DoSAckStatusType *  dosAckStatusPtr,
          uint32 *            overflowPtr
        );
        
        /*-------------------------------------------------------------------
          IQI interface Methods
        -------------------------------------------------------------------*/
        DSIQI_RELEASE()

      protected:
        virtual bool IsOptSupported
        (
          OptLevelType  optLevel,
          OptNameType   optName
        );

        virtual bool IsMulticastSupported
        (
          void
        );

        virtual bool IsConnectSupported
        (
          const SockAddrIN6Type *  v6RemoteAddrPtr,
          DS::ErrorType *          dsErrnoPtr
        );

        virtual bool IsPktInfoDifferent
        (
          const SockAddrStorageType *  remoteAddrPtr
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

        virtual void ProcessDoSAckEvent
        (
          DS::Sock::Event::DoSAckEventInfo *  dosEventInfoPtr
        );

      private:
        static void * operator new
        (
          unsigned int numBytes
        ) throw();

        static void operator delete
        (
          void *  bufPtr
        ) throw();

        DS::Sock::DoSAckStatusType  dosAckStatus;
        uint32                      overflow;
        ps_iface_mcast_handle_type  mcastHandle;

    }; /* class UDPSocket */
  } /* namespace Sock */
} /* namespace DS */


#endif /* FEATURE_DATA_PS */
#endif /* DS_SOCK_UDPSOCKET_H */
