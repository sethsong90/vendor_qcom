#ifndef DS_SOCK_TCPSOCKET_H
#define DS_SOCK_TCPSOCKET_H
/*===========================================================================
  @file DS_Sock_TCPSocket.h

  This file defines the class which implements the ISocket interface for TCP
  sockets.

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_TCPSocket.h#2 $
  $DateTime: 2009/10/09 10:48:28 $ $Author: hmurari $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "DS_Sock_Socket.h"


/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace DS
{
  namespace Sock
  {
    class TCPSocket : public Socket                           /*lint !e578 */
    {
      public:
        virtual ~TCPSocket() throw() {}

        static TCPSocket * CreateInstance
        (
          FamilyType    family
        );

        virtual DS::ErrorType CDECL Listen
        (
          int32  backlog
        );

        virtual DS::ErrorType CDECL Accept
        (
          SockAddrStorageType *  remoteAddrPtr,
          ISocket **             newSockPtr
        );

        virtual DS::ErrorType CDECL WriteDSMChain
        (
          dsm_item_type **  dsmItemPtrPtr,
          int32 *           numBytesWrittenPtr
        );

        virtual DS::ErrorType CDECL ReadDSMChain
        (
          dsm_item_type **  dsmItemPtrPtr,
          int32 *           numBytesReadPtr
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

      private:
        static void * operator new
        (
          unsigned int numBytes
        ) throw();

        static void operator delete
        (
          void *  bufPtr
        ) throw();

    }; /* class TCPSocket */
  } /* namespace Sock */
} /* namespace DS */


#endif /* FEATURE_DATA_PS */
#endif /* DS_SOCK_TCPSOCKET_H */
