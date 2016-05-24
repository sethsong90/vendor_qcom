#ifndef DS_SOCK_ISOCKET_H
#define DS_SOCK_ISOCKET_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "AEEIPort1.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Sock_Def.h"
#define ds_Sock_AEEIID_ISocket 0x106c549

/** @interface ds_Sock_ISocket
  * 
  * The Socket interface extends the IPort1 interface to provide 
  * access to Sockets.
  */
#define INHERIT_ds_Sock_ISocket(iname) \
   INHERIT_IPort1(iname); \
   AEEResult (*Bind)(iname* _pif, const ds_SockAddrStorageType localAddr); \
   AEEResult (*Listen)(iname* _pif, int backLog); \
   AEEResult (*Accept)(iname* _pif, ds_SockAddrStorageType remoteAddrOut, ds_Sock_ISocket** newConnSock); \
   AEEResult (*Connect)(iname* _pif, const ds_SockAddrStorageType remoteAddr); \
   AEEResult (*SendTo)(iname* _pif, const byte* buf, int bufLen, const ds_SockAddrStorageType remoteAddr, unsigned int flags, int* numSent); \
   AEEResult (*RecvFrom)(iname* _pif, byte* buf, int bufLen, int* bufLenReq, unsigned int flags, ds_SockAddrStorageType remoteAddr); \
   AEEResult (*GetSockName)(iname* _pif, ds_SockAddrStorageType localAddr); \
   AEEResult (*GetPeerName)(iname* _pif, ds_SockAddrStorageType remoteAddr); \
   AEEResult (*Shutdown)(iname* _pif, ds_Sock_ShutdownDirType how); \
   AEEResult (*GetOpt)(iname* _pif, ds_Sock_OptLevelType level, ds_Sock_OptNameType name, int* value); \
   AEEResult (*SetOpt)(iname* _pif, ds_Sock_OptLevelType level, ds_Sock_OptNameType name, int value); \
   AEEResult (*RecvMsg)(iname* _pif, ds_SockAddrStorageType msg_name, byte* msg_buf, int msg_bufLen, int* msg_bufLenReq, byte* msg_control, int msg_controlLen, int* msg_controlLenReq, unsigned int* msg_flags, unsigned int flags); \
   AEEResult (*SendMsg)(iname* _pif, const ds_SockAddrStorageType msg_name, const byte* msg_buf, int msg_bufLen, int* numWritten, const byte* msg_control, int msg_controlLen, unsigned int flags); \
   AEEResult (*GetSOLingerReset)(iname* _pif, ds_Sock_LingerType* value); \
   AEEResult (*SetSOLingerReset)(iname* _pif, const ds_Sock_LingerType* value)
AEEINTERFACE_DEFINE(ds_Sock_ISocket);

/** @memberof ds_Sock_ISocket
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Sock_ISocket_AddRef(ds_Sock_ISocket* _pif)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->AddRef(_pif);
}

/** @memberof ds_Sock_ISocket
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Sock_ISocket_Release(ds_Sock_ISocket* _pif)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->Release(_pif);
}

/** @memberof ds_Sock_ISocket
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Sock_ISocket_QueryInterface(ds_Sock_ISocket* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Sock_ISocket
  * 
  * Copy bytes from the source stream to the specified buffer.  The read
  * pointer is advanced.
  *
  * @param _pif Pointer to interface
  * @param buf the buffer to receive the read bytes
  * @param bufLen Length of sequence
  * @param bufLenReq Required length of sequence
  * 
  * @retval AEE_SUCCESS bytes were successfully read into the buffer.  If
  *                     the buffer was of non-zero size but is returned
  *                     empty, the end of the stream has been reached.
  * @retval AEE_ENOTALLOWED caller does not have the necessary capability to
  *                         perform the operation
  * @retval AEE_EWOULDBLOCK no data available; call Readable() to wait
  *
  * @return Another appropriate error core may be returned if the operation
  *         is not successful.
  */
static __inline AEEResult ds_Sock_ISocket_Read(ds_Sock_ISocket* _pif, byte* buf, int bufLen, int* bufLenReq)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->Read(_pif, buf, bufLen, bufLenReq);
}

/** @memberof ds_Sock_ISocket
  * 
  * Register a signal to be set when Read() would return something other
  * than AEE_EWOULDBLOCK.
  * 
  * @param _pif Pointer to interface
  * @param ps the signal to register
  *
  * @return AEE_SUCCESS if successful, or another appropriate error code if
  *         operation was not successful.
  */
static __inline AEEResult ds_Sock_ISocket_Readable(ds_Sock_ISocket* _pif, ISignal* ps)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->Readable(_pif, ps);
}

/** @memberof ds_Sock_ISocket
  * 
  * Copy bytes from the specified buffer to the source stream.  The write
  * pointer is advanced.
  *
  * @param _pif Pointer to interface
  * @param buf the buffer from which bytes are taken to write to the port
  * @param bufLen Length of sequence
  * @param written size of data written to the port
  *
  * @retval AEE_SUCCESS successfully wrote 'written' bytes to the port
  * @retval AEE_ENOTALLOWED caller does not have the necessary capability to
  *                         perform the operation
  * @retval AEE_EWOULDBLOCK no data available; call Writeable() to wait
  *
  * @return Another appropriate error core may be returned if the operation
  *         is not successful.
  */
static __inline AEEResult ds_Sock_ISocket_Write(ds_Sock_ISocket* _pif, const byte* buf, int bufLen, int* written)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->Write(_pif, buf, bufLen, written);
}

/** @memberof ds_Sock_ISocket
  * 
  * Register a signal to be set when Write() would return something other
  * than AEE_EWOULDBLOCK. 
  * 
  * @param _pif Pointer to interface
  * @param ps the signal to register
  *
  * @return AEE_SUCCESS if successful, or another appropriate error code if
  *         operation was not successful.
  */
static __inline AEEResult ds_Sock_ISocket_Writeable(ds_Sock_ISocket* _pif, ISignal* ps)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->Writeable(_pif, ps);
}

/** @memberof ds_Sock_ISocket
  * 
  * This function allows some implementation-specific control over the
  * behavior of an IPort1, e.g. setting/getting configuration.
  * 
  * Control() here is analogous to SYSV's Control.  Its intended use is for
  * special, out-of-object-signature behavior.  For that reason, it is
  * super-generic.
  *
  * This method could also be used to selectively close the capabilities
  * of the port.
  *
  * @par Comments:
  *    Data passed in the buffer arguments must always be arranged as if
  *    they've already been marshalled, i.e. the precise byte ordering must
  *    be specified.  The data must not include pointers or objects. 
  *
  * @param _pif Pointer to interface
  * @param uid uid of the control operation, defined by the class
  *            implementing IPort1
  * @param inbuf input buffer for the operation
  * @param inbufLen Length of sequence
  * @param outbuf output buffer for the operation
  * @param outbufLen Length of sequence
  * @param outbufLenReq Required length of sequence
  *
  * @return AEE_SUCCESS if successful, or another appropriate error code if
  *         operation was not successful.
  */
static __inline AEEResult ds_Sock_ISocket_Control(ds_Sock_ISocket* _pif, AEEUID uid, const byte* inbuf, int inbufLen, byte* outbuf, int outbufLen, int* outbufLenReq)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->Control(_pif, uid, inbuf, inbufLen, outbuf, outbufLen, outbufLenReq);
}

/** @memberof ds_Sock_ISocket
  * 
  * This function closes the port.  Any further Read() and Write() on the
  * port will fail, and the IPort1 will become readable and writeable.
  *
  * @param _pif Pointer to interface
  * @return AEE_SUCCESS if successful, or another appropriate error code if
  *         operation was not successful.
  */
static __inline AEEResult ds_Sock_ISocket_Close(ds_Sock_ISocket* _pif)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->Close(_pif);
}

/** @memberof ds_Sock_ISocket
  * 
  * This function associates a local address and port value to the socket.
  * @param _pif Pointer to interface
  * @param localAddr A specification of the address and port to attach.
  *                  @See SockAddrStorageType
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval AEE_EWOULDBLOCK Request is processed asynchronously.
  *         Application should register to be notified when progress can
  *         be made and call Bind again when the notification has been
  *         received. Application should repeat this process until
  *         AEE_SUCCESS is returned from the call.
  *         Registration for the notification can be done via 
  *         IPort1_Readable or ISocketExt_RegEvent (READ event).
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocket_Bind(ds_Sock_ISocket* _pif, const ds_SockAddrStorageType localAddr)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->Bind(_pif, localAddr);
}

/** @memberof ds_Sock_ISocket
  * 
  * This function initiates passive open for TCP connections.
  * @param _pif Pointer to interface
  * @param backLog Maximum number of half-open TCP connections to track at one time.
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocket_Listen(ds_Sock_ISocket* _pif, int backLog)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->Listen(_pif, backLog);
}

/** @memberof ds_Sock_ISocket
  * 
  * This function returns a newly created socket for a new passively opened connection.
  * @param _pif Pointer to interface
  * @param remoteAddrOut Output Address of the remote end of the new connection.
  *                             @See SockAddrStorageType
  * @param newConnSock Output The newly created socket.
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval AEE_EWOULDBLOCK Request is processed asynchronously.
  *         Application should register to be notified when progress can
  *         be made and call Accept again when the notification has been
  *         received. Application should repeat this process until
  *         AEE_SUCCESS is returned from the call.
  *         Registration for the notification can be done via 
  *         IPort1_Readable or ISocketExt_RegEvent (ACCEPT event).
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocket_Accept(ds_Sock_ISocket* _pif, ds_SockAddrStorageType remoteAddrOut, ds_Sock_ISocket** newConnSock)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->Accept(_pif, remoteAddrOut, newConnSock);
}

/** @memberof ds_Sock_ISocket
  * 
  * This function initiates an active open for TCP connection;
  * For UDP sockets this function sets the peer∆s IP address and port value of the socket
  * @param _pif Pointer to interface
  * @param remoteAddr Address to connect.
  *                   @See SockAddrStorageType
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval AEE_EWOULDBLOCK Request is processed asynchronously.
  *         Application should register to be notified when progress can
  *         be made and call Connect again when the notification has been
  *         received. Application should repeat this process until
  *         AEE_SUCCESS is returned from the call.
  *         Registration for the notification can be done via 
  *         IPort1_Writeable or ISocketExt_RegEvent (WRITE event).
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocket_Connect(ds_Sock_ISocket* _pif, const ds_SockAddrStorageType remoteAddr)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->Connect(_pif, remoteAddr);
}

/** @memberof ds_Sock_ISocket
  * 
  * This function sends specified number of bytes across the UDP transport
  * @param _pif Pointer to interface
  * @param buf Byte array that contains the data to be sent.
  * @param bufLen Length of sequence
  * @param bufLen size in bytes of data to be sent.
  * @param remoteAddr A specification of the address and port to send to.
  *                   @See SockAddrStorageType 
  * @param flags Supported values are:
  *    <ul>
  *       <li> 0: No special handling.
  *       <li> MSG_EXPEDITE: Indicates that the packet should be sent on
  *                          ACH or REACH if traffic channel is not UP
  *       <li> MSG_FASTEXPEDITE: Indicates that the packet should be sent on REACH if
  *                              traffic channel is not UP
  *    </ul>
  * @param numSent If AEE_SUCCESS is returned, indicates the number of bytes actually sent.
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval AEE_EWOULDBLOCK Request is processed asynchronously.
  *         Application should register to be notified when progress can
  *         be made and call SendTo again when the notification has been
  *         received. Application should repeat this process until
  *         AEE_SUCCESS is returned from the call.
  *         Registration for the notification can be done via 
  *         IPort1_Writeable or ISocketExt_RegEvent (WRITE event).
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocket_SendTo(ds_Sock_ISocket* _pif, const byte* buf, int bufLen, const ds_SockAddrStorageType remoteAddr, unsigned int flags, int* numSent)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->SendTo(_pif, buf, bufLen, remoteAddr, flags, numSent);
}

/** @memberof ds_Sock_ISocket
  * 
  * This function reads specified number of bytes from the UDP transport
  * @param _pif Pointer to interface
  * @param buf Input/Output Bytes array into which data can be read.
  *                         On output contains the received data.
  * @param bufLen Length of sequence
  * @param bufLenReq Required length of sequence
  * @param flags Not supported - must be set to zero.
  * @param remoteAddr Output A specification of the address and port from which data was received.
  *                          @See SockAddrStorageType 
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval AEE_EWOULDBLOCK Request is processed asynchronously.
  *         Application should register to be notified when progress can
  *         be made and call RecvFrom again when the notification has been
  *         received. Application should repeat this process until
  *         AEE_SUCCESS is returned from the call.
  *         Registration for the notification can be done via 
  *         IPort1_Readable or ISocketExt_RegEvent (READ event).
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocket_RecvFrom(ds_Sock_ISocket* _pif, byte* buf, int bufLen, int* bufLenReq, unsigned int flags, ds_SockAddrStorageType remoteAddr)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->RecvFrom(_pif, buf, bufLen, bufLenReq, flags, remoteAddr);
}

/** @memberof ds_Sock_ISocket
  * 
  * Use this function to get the local address assigned to the socket
  * @param _pif Pointer to interface
  * @param localAddr Output The address assigned to the socket.
  *                         @See SockAddrStorageType
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocket_GetSockName(ds_Sock_ISocket* _pif, ds_SockAddrStorageType localAddr)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->GetSockName(_pif, localAddr);
}

/** @memberof ds_Sock_ISocket
  * 
  * Use this function to get the remote address of the connected socket.
  * @param _pif Pointer to interface
  * @param remoteAddr Output The remote address of the connected socket.
  *                          @See SockAddrStorageType
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocket_GetPeerName(ds_Sock_ISocket* _pif, ds_SockAddrStorageType remoteAddr)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->GetPeerName(_pif, remoteAddr);
}

/** @memberof ds_Sock_ISocket
  * 
  * This function causes all or part of a full-duplex connection to be terminated gracefully.
  * @param _pif Pointer to interface
  * @param how specifies which direction(s) of the connection to shutdown.
  * @see ShutdownDirType
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocket_Shutdown(ds_Sock_ISocket* _pif, ds_Sock_ShutdownDirType how)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->Shutdown(_pif, how);
}

/** @memberof ds_Sock_ISocket
  * 
  * Gets options for the socket. (compatible to socket options that use
  * a 32 bit integer or boolean as the socket option data type).
  * @param _pif Pointer to interface
  * @param level option level (see ds_Sock_OptLevelType)
  * @param name option name (see ds_Sock_OptNameType)
  * @param value option value
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocket_GetOpt(ds_Sock_ISocket* _pif, ds_Sock_OptLevelType level, ds_Sock_OptNameType name, int* value)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->GetOpt(_pif, level, name, value);
}

/** @memberof ds_Sock_ISocket
  * 
  * Sets options for the socket. (compatible to socket options that use
  * a 32 bit integer or boolean as the socket option data type).
  * @param _pif Pointer to interface
  * @param level option level (see ds_Sock_OptLevelType)
  * @param name option name (see ds_Sock_OptNameType)
  * @param value option value
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocket_SetOpt(ds_Sock_ISocket* _pif, ds_Sock_OptLevelType level, ds_Sock_OptNameType name, int value)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->SetOpt(_pif, level, name, value);
}

/** @memberof ds_Sock_ISocket
  * 
  * This function reads data and control (ancillary) data buffers over
  * the transport specified by the socket. 
  * The parameters with msg_ prefix follow the convention of the msghdr 
  * defined in RFC 3542. 
  * @param _pif Pointer to interface
  * @param msg_bufLen Length of sequence
  * @param msg_bufLenReq Required length of sequence
  * @param msg_controlLen Length of sequence
  * @param msg_controlLenReq Required length of sequence
  * @param msg_name Output Address of the source. @See SockAddrStorageType
  * @param msg_buf Output Message payload.
  * @param msg_control Output Ancillary data, if applicable.
  *
  *        Note1: This sequence does *not* follow the regular 
  *        "output sequence" semantics. The LenReq parameter
  *        (implicitly included here) shall indicate the actual size of
  *        AncillaryData returned, which may be smaller or equal to the 
  *        buffer size provided by the application. Information on
  *        additional Ancillary Data that is available but does not fit
  *        into the buffer provided by the application is not supported.
  *        @See socket options QDS_IP_RECVIF and QDS_IP_RECVERR.
  *
  *        Note2: Language specific headers may be available to
  *        facilitate API usage of Ancillary Data. For C++/C see 
  *        ds_Sock_CMsg_Helpers.h
  *
  *        Note3: The ancillary header (len, level, type) inside the
  *        msg_control buffer is on little-endian byte order. For the
  *        byte ordering of the specific ancillary data part (currently
  *        ExtendedErrInfoType or RecvIfaceInfoType) please refer to
  *        its definition.
  *  
  *        Note4: Ancillary Data types:
  *
  *               cmsg_level: QDS_LEVEL_IP
  *               cmsg_type: QDS_IP_RECVIF
  *               structure of data: ds_Sock_RecvIfaceInfoType
  *
  *               cmsg_level: QDS_LEVEL_IP
  *               cmsg_type: QDS_IP_RECVERR
  *               structure of data: ds_Sock_ExtendedErrInfoType
  *
  * @See ds_Sock_RecvIfaceInfoType, ds_Sock_ExtendedErrInfoType
  *
  * @param msg_flags Output Output flags.
  *        <ul>
  *           <li> MSG_CTRUNC: Indicates amount of incoming ancillary
  *                            data that is larger than buffer supplied
  *                            by the application. The part of 
  *                            ancillary data that fits into the buffer
  *                            is provided to the application. The rest
  *                            of the ancillary data cannot be fetched.
  *           <li> MSG_TRUNC:  Indicates amount of incoming data that is
  *                            larger than buffer supplied by the
  *                            application. The part of data that fits 
  *                            into the buffer is provided to the 
  *                            application. The rest of the data cannot be fetched.
  *        </ul>
  * @param flags Input flags.
  *        <ul>
  *           <li> 0: No special handling.
  *           <li> MSG_ERRQUEUE: Retrieve ICMP errors.
  *                              @See QDS_IP_RECVERR socket option
  *        </ul>
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval AEE_EWOULDBLOCK Request is processed asynchronously.
  *         Application should register to be notified when progress can
  *         be made and call RecvMsg again when the notification has been
  *         received. Application should repeat this process until
  *         AEE_SUCCESS is returned from the call.
  *         Registration for the notification can be done via 
  *         IPort1_Readable or ISocketExt_RegEvent (READ event).
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocket_RecvMsg(ds_Sock_ISocket* _pif, ds_SockAddrStorageType msg_name, byte* msg_buf, int msg_bufLen, int* msg_bufLenReq, byte* msg_control, int msg_controlLen, int* msg_controlLenReq, unsigned int* msg_flags, unsigned int flags)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->RecvMsg(_pif, msg_name, msg_buf, msg_bufLen, msg_bufLenReq, msg_control, msg_controlLen, msg_controlLenReq, msg_flags, flags);
}

/** @memberof ds_Sock_ISocket
  * 
  * This function sends data from the provided data and ancillary buffers
  * over the transport specified by the socket.
  * The parameters with msg_ prefix follow the convention of the msdhdr 
  * defined in RFC 3542. 
  * @param _pif Pointer to interface
  * @param msg_bufLen Length of sequence
  * @param msg_controlLen Length of sequence
  * @param msg_name Address of the destination. @See SockAddrStorageType
  * @param msg_buf Message payload.
  * @param numWritten Output Number of bytes actually sent.
  * @param msg_control Ancillary data, if applicable. 
  *        For SendMsg AncillaryData is an input only parameter.
  *
  * @param flags Input flags. Currently none are supported.
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval AEE_EWOULDBLOCK Request is processed asynchronously.
  *         Application should register to be notified when progress can
  *         be made and call SendMsg again when the notification has been
  *         received. Application should repeat this process until
  *         AEE_SUCCESS is returned from the call.
  *         Registration for the notification can be done via 
  *         IPort1_Writeable or ISocketExt_RegEvent (WRITE event).
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocket_SendMsg(ds_Sock_ISocket* _pif, const ds_SockAddrStorageType msg_name, const byte* msg_buf, int msg_bufLen, int* numWritten, const byte* msg_control, int msg_controlLen, unsigned int flags)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->SendMsg(_pif, msg_name, msg_buf, msg_bufLen, numWritten, msg_control, msg_controlLen, flags);
}

/** @memberof ds_Sock_ISocket
  * 
  * This attribute represents the SO_LINGER_RESET socket option.
  * Level: Socket
  * Value type: LingerType
  * Description: Linger and reset on timeout.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Sock_ISocket_GetSOLingerReset(ds_Sock_ISocket* _pif, ds_Sock_LingerType* value)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->GetSOLingerReset(_pif, value);
}

/** @memberof ds_Sock_ISocket
  * 
  * This attribute represents the SO_LINGER_RESET socket option.
  * Level: Socket
  * Value type: LingerType
  * Description: Linger and reset on timeout.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Sock_ISocket_SetSOLingerReset(ds_Sock_ISocket* _pif, const ds_Sock_LingerType* value)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocket)->SetSOLingerReset(_pif, value);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "AEEIPort1.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Sock_Def.h"
namespace ds
{
   namespace Sock
   {
      const ::AEEIID AEEIID_ISocket = 0x106c549;
      
      /** @interface ISocket
        * 
        * The Socket interface extends the IPort1 interface to provide 
        * access to Sockets.
        */
      struct ISocket : public ::IPort1
      {
         
         /**
           * This function associates a local address and port value to the socket.
           * @param localAddr A specification of the address and port to attach.
           *                  @See SockAddrStorageType
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval AEE_EWOULDBLOCK Request is processed asynchronously.
           *         Application should register to be notified when progress can
           *         be made and call Bind again when the notification has been
           *         received. Application should repeat this process until
           *         AEE_SUCCESS is returned from the call.
           *         Registration for the notification can be done via 
           *         IPort1::Readable or ISocketExt::RegEvent (READ event).
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Bind(const ::ds::SockAddrStorageType localAddr) = 0;
         
         /**
           * This function initiates passive open for TCP connections.
           * @param backLog Maximum number of half-open TCP connections to track at one time.
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Listen(int backLog) = 0;
         
         /**
           * This function returns a newly created socket for a new passively opened connection.
           * @param remoteAddrOut Output Address of the remote end of the new connection.
           *                             @See SockAddrStorageType
           * @param newConnSock Output The newly created socket.
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval AEE_EWOULDBLOCK Request is processed asynchronously.
           *         Application should register to be notified when progress can
           *         be made and call Accept again when the notification has been
           *         received. Application should repeat this process until
           *         AEE_SUCCESS is returned from the call.
           *         Registration for the notification can be done via 
           *         IPort1::Readable or ISocketExt::RegEvent (ACCEPT event).
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Accept(::ds::SockAddrStorageType remoteAddrOut, ISocket** newConnSock) = 0;
         
         /**
           * This function initiates an active open for TCP connection;
           * For UDP sockets this function sets the peer∆s IP address and port value of the socket
           * @param remoteAddr Address to connect.
           *                   @See SockAddrStorageType
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval AEE_EWOULDBLOCK Request is processed asynchronously.
           *         Application should register to be notified when progress can
           *         be made and call Connect again when the notification has been
           *         received. Application should repeat this process until
           *         AEE_SUCCESS is returned from the call.
           *         Registration for the notification can be done via 
           *         IPort1::Writeable or ISocketExt::RegEvent (WRITE event).
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Connect(const ::ds::SockAddrStorageType remoteAddr) = 0;
         
         /**
           * This function sends specified number of bytes across the UDP transport
           * @param buf Byte array that contains the data to be sent.
           * @param bufLen size in bytes of data to be sent.
           * @param remoteAddr A specification of the address and port to send to.
           *                   @See SockAddrStorageType 
           * @param flags Supported values are:
           *    <ul>
           *       <li> 0: No special handling.
           *       <li> MSG_EXPEDITE: Indicates that the packet should be sent on
           *                          ACH or REACH if traffic channel is not UP
           *       <li> MSG_FASTEXPEDITE: Indicates that the packet should be sent on REACH if
           *                              traffic channel is not UP
           *    </ul>
           * @param numSent If AEE_SUCCESS is returned, indicates the number of bytes actually sent.
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval AEE_EWOULDBLOCK Request is processed asynchronously.
           *         Application should register to be notified when progress can
           *         be made and call SendTo again when the notification has been
           *         received. Application should repeat this process until
           *         AEE_SUCCESS is returned from the call.
           *         Registration for the notification can be done via 
           *         IPort1::Writeable or ISocketExt::RegEvent (WRITE event).
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SendTo(const ::byte* buf, int bufLen, const ::ds::SockAddrStorageType remoteAddr, unsigned int flags, int* numSent) = 0;
         
         /**
           * This function reads specified number of bytes from the UDP transport
           * @param buf Input/Output Bytes array into which data can be read.
           *                         On output contains the received data.
           * @param flags Not supported - must be set to zero.
           * @param remoteAddr Output A specification of the address and port from which data was received.
           *                          @See SockAddrStorageType 
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval AEE_EWOULDBLOCK Request is processed asynchronously.
           *         Application should register to be notified when progress can
           *         be made and call RecvFrom again when the notification has been
           *         received. Application should repeat this process until
           *         AEE_SUCCESS is returned from the call.
           *         Registration for the notification can be done via 
           *         IPort1::Readable or ISocketExt::RegEvent (READ event).
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL RecvFrom(::byte* buf, int bufLen, int* bufLenReq, unsigned int flags, ::ds::SockAddrStorageType remoteAddr) = 0;
         
         /**
           * Use this function to get the local address assigned to the socket
           * @param localAddr Output The address assigned to the socket.
           *                         @See SockAddrStorageType
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetSockName(::ds::SockAddrStorageType localAddr) = 0;
         
         /**
           * Use this function to get the remote address of the connected socket.
           * @param remoteAddr Output The remote address of the connected socket.
           *                          @See SockAddrStorageType
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetPeerName(::ds::SockAddrStorageType remoteAddr) = 0;
         
         /**
           * This function causes all or part of a full-duplex connection to be terminated gracefully.
           * @param how specifies which direction(s) of the connection to shutdown.
           * @see ShutdownDirType
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Shutdown(::ds::Sock::ShutdownDirType how) = 0;
         
         /**
           * Gets options for the socket. (compatible to socket options that use
           * a 32 bit integer or boolean as the socket option data type).
           * @param level option level (see ds::Sock::OptLevelType)
           * @param name option name (see ds::Sock::OptNameType)
           * @param value option value
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetOpt(::ds::Sock::OptLevelType level, ::ds::Sock::OptNameType name, int* value) = 0;
         
         /**
           * Sets options for the socket. (compatible to socket options that use
           * a 32 bit integer or boolean as the socket option data type).
           * @param level option level (see ds::Sock::OptLevelType)
           * @param name option name (see ds::Sock::OptNameType)
           * @param value option value
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetOpt(::ds::Sock::OptLevelType level, ::ds::Sock::OptNameType name, int value) = 0;
         
         /**
           * This function reads data and control (ancillary) data buffers over
           * the transport specified by the socket. 
           * The parameters with msg_ prefix follow the convention of the msghdr 
           * defined in RFC 3542. 
           * @param msg_name Output Address of the source. @See SockAddrStorageType
           * @param msg_buf Output Message payload.
           * @param msg_control Output Ancillary data, if applicable.
           *
           *        Note1: This sequence does *not* follow the regular 
           *        "output sequence" semantics. The LenReq parameter
           *        (implicitly included here) shall indicate the actual size of
           *        AncillaryData returned, which may be smaller or equal to the 
           *        buffer size provided by the application. Information on
           *        additional Ancillary Data that is available but does not fit
           *        into the buffer provided by the application is not supported.
           *        @See socket options QDS_IP_RECVIF and QDS_IP_RECVERR.
           *
           *        Note2: Language specific headers may be available to
           *        facilitate API usage of Ancillary Data. For C++/C see 
           *        ds_Sock_CMsg_Helpers.h
           *
           *        Note3: The ancillary header (len, level, type) inside the
           *        msg_control buffer is on little-endian byte order. For the
           *        byte ordering of the specific ancillary data part (currently
           *        ExtendedErrInfoType or RecvIfaceInfoType) please refer to
           *        its definition.
           *  
           *        Note4: Ancillary Data types:
           *
           *               cmsg_level: QDS_LEVEL_IP
           *               cmsg_type: QDS_IP_RECVIF
           *               structure of data: ds::Sock::RecvIfaceInfoType
           *
           *               cmsg_level: QDS_LEVEL_IP
           *               cmsg_type: QDS_IP_RECVERR
           *               structure of data: ds::Sock::ExtendedErrInfoType
           *
           * @See ds::Sock::RecvIfaceInfoType, ds::Sock::ExtendedErrInfoType
           *
           * @param msg_flags Output Output flags.
           *        <ul>
           *           <li> MSG_CTRUNC: Indicates amount of incoming ancillary
           *                            data that is larger than buffer supplied
           *                            by the application. The part of 
           *                            ancillary data that fits into the buffer
           *                            is provided to the application. The rest
           *                            of the ancillary data cannot be fetched.
           *           <li> MSG_TRUNC:  Indicates amount of incoming data that is
           *                            larger than buffer supplied by the
           *                            application. The part of data that fits 
           *                            into the buffer is provided to the 
           *                            application. The rest of the data cannot be fetched.
           *        </ul>
           * @param flags Input flags.
           *        <ul>
           *           <li> 0: No special handling.
           *           <li> MSG_ERRQUEUE: Retrieve ICMP errors.
           *                              @See QDS_IP_RECVERR socket option
           *        </ul>
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval AEE_EWOULDBLOCK Request is processed asynchronously.
           *         Application should register to be notified when progress can
           *         be made and call RecvMsg again when the notification has been
           *         received. Application should repeat this process until
           *         AEE_SUCCESS is returned from the call.
           *         Registration for the notification can be done via 
           *         IPort1::Readable or ISocketExt::RegEvent (READ event).
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL RecvMsg(::ds::SockAddrStorageType msg_name, ::byte* msg_buf, int msg_bufLen, int* msg_bufLenReq, ::byte* msg_control, int msg_controlLen, int* msg_controlLenReq, unsigned int* msg_flags, unsigned int flags) = 0;
         
         /**
           * This function sends data from the provided data and ancillary buffers
           * over the transport specified by the socket.
           * The parameters with msg_ prefix follow the convention of the msdhdr 
           * defined in RFC 3542. 
           * @param msg_name Address of the destination. @See SockAddrStorageType
           * @param msg_buf Message payload.
           * @param numWritten Output Number of bytes actually sent.
           * @param msg_control Ancillary data, if applicable. 
           *        For SendMsg AncillaryData is an input only parameter.
           *
           * @param flags Input flags. Currently none are supported.
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval AEE_EWOULDBLOCK Request is processed asynchronously.
           *         Application should register to be notified when progress can
           *         be made and call SendMsg again when the notification has been
           *         received. Application should repeat this process until
           *         AEE_SUCCESS is returned from the call.
           *         Registration for the notification can be done via 
           *         IPort1::Writeable or ISocketExt::RegEvent (WRITE event).
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SendMsg(const ::ds::SockAddrStorageType msg_name, const ::byte* msg_buf, int msg_bufLen, int* numWritten, const ::byte* msg_control, int msg_controlLen, unsigned int flags) = 0;
         
         /**
           * This attribute represents the SO_LINGER_RESET socket option.
           * Level: Socket
           * Value type: LingerType
           * Description: Linger and reset on timeout.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetSOLingerReset(::ds::Sock::LingerType* value) = 0;
         
         /**
           * This attribute represents the SO_LINGER_RESET socket option.
           * Level: Socket
           * Value type: LingerType
           * Description: Linger and reset on timeout.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetSOLingerReset(const ::ds::Sock::LingerType* value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_SOCK_ISOCKET_H
