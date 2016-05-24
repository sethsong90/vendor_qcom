//=============================================================================
//
// COPYRIGHT 2012-2013 Qualcomm Technologies, Inc.
// All rights reserved. Qualcomm Technologies proprietary and confidential.
//
// $Header: //depot/asic/msmshared/users/crd_dash/QTA/inc/IHttpHandler.h#3 $
// $DateTime: 2013/09/06 16:09:31 $
// $Change: 4395644 $
//=============================================================================

#ifndef IHttpHandler_H
#define IHttpHandler_H

#include "IHttpStatusNotificationHandler.h"
#include "QTATypes.h"
#include "HTTPCookieStore.h"       //HttpCookie store

namespace QTA
{
///
/// @brief
///
class IHttpHandler  {
public:
   ///
   /// @brief Default Ctor
   ///
   IHttpHandler(){}


   ///
   /// @brief Default Dtor
   ///
   virtual ~IHttpHandler(){}

   ///
   /// @brief Closes all connections to HTTP Server. This should be called only once.
   ///        Every connection will block for a small period of time and then close tcp connection
   ///        in the background if tcp close did not complete during the duration
   ///        the call is blocked.
   /// @return
   ///  HTTP_SUCCESS    - tcp close successful
   ///  HTTP_WAIT       - tcp close in progress.
   ///  HTTP_FAILURE    - failed
   ///
   virtual HttpReturnCode CloseConnection () = 0;

   ///
   /// @brief Creates a new Http Requests (main request)
   /// @param RequestId: [Out] Unique ID used to identify this
   ///        request on subsequent calls.
   /// @param priority: [In] Priority assigned to the request
   /// @param localRequest: [In] Distiniguishes between local and remote requests
   /// @return
   ///  HTTP_SUCCESS    - Successfully created a new request
   ///  HTTP_FAILURE    - Failed to create a new request
   ///
   virtual HttpReturnCode CreateRequest(uint32& requestId,
                                          const HttpRequestPriority & priority = REQ_PRIORITY_MED,
                                          const bool localRequest = false) = 0;

   ///
   /// @brief Deletes the Main Http Request (and all sub-requests, if applicable)
   ///        identified by the requestId
   /// @param requestId: [In] Identifies the Http Request
   /// @return
   ///  HTTP_SUCCESS    - Successfully deleted a new request
   ///  HTTP_FAILURE    - Failed to delete a new request
   ///
   virtual HttpReturnCode DeleteRequest(uint32 requestId) = 0;


   ///
   /// @brief Set Header on the Http Request identified by requestId
   /// @param requestId: [In] Identifies the Http Request
   /// @param key: [In] Header key
   /// @param keyLen: [In] Header key length
   /// @param value: [In] Header value
   /// @param valueLen: [In] Header value length
   /// @return
   /// HTTP_SUCCESS    - on success
   /// HTTP_FAILURE    - on failure
   ///
   virtual HttpReturnCode SetHeader (uint32      requestId,
                                       const char* key,
                                       int         keyLen,
                                       const char* value,
                                       int         valueLen) = 0;

   ///
   /// @brief Unset the Http header
   /// @param requestId: [In] Identifies the Http Request
   /// @param key: [In] Header key
   /// @param keyLen: [In] Header key length
   /// @return
   /// HTTP_SUCCESS    - on success
   /// HTTP_FAILURE    - on failure
   ///
   virtual HttpReturnCode UnsetHeader (uint32      requestId,
                                         const char* key,
                                         int         keyLen) = 0;

   ///
   /// @brief Flushes all information used for constructing Http Request
   /// @param requestId: [In] Identifies the Http Request.
   /// @return
   /// HTTP_SUCCESS    - on success
   /// HTTP_FAILURE    - on failure
   ///
   virtual HttpReturnCode FlushRequest (uint32 requestId) = 0;

   ///
   /// @brief Set the message body that is sent in the Http request
   ///        NOTE: THIS IS CURRENTLY NOT SUPPORTED BY LOWER LAYERS.
   ///              HENCE NOT IMPLEMENTED. JUST A PLACE HOLDER FOR FUTURE
   /// @param requestId: [In] Identifies the Http Request
   /// @param httpReqMsgBody: [In] Pointer to memory area containing the
   ///        request message body.
   /// @param httpReqMsgBodyLen: [In] length of the Http request message body
   /// @return
   /// HTTP_NOTSUPPORTED - Currently not supported
   ///
   virtual HttpReturnCode SetMessageBody (uint32      requestId,
                                            const char* httpReqMsgBody,
                                            int         httpReqMsgBodyLen) = 0;

   ///
   /// @brief Unset the Http request message body
   ///        NOTE: THIS IS CURRENTLY NOT SUPPORTED BY LOWER LAYERS.
   ///              HENCE NOT IMPLEMENTED. JUST A PLACE HOLDER FOR FUTURE
   /// @param requestId: [In] Identifies the Http Request
   /// @return
   /// HTTP_NOTSUPPORTED - Currently not supported
   ///
   virtual HttpReturnCode UnsetMessageBody (uint32 requestId) = 0;


   ///
   /// @brief Signals the QTA that the Http Request is now ready to be sent
   ///        Cannot modify any headers after this call is made
   /// @param requestId: [In] Identifies the Http Request
   /// @param method: [In] Http Method type like GET, POST etc., defined
   ///        in  HTTPStackInterface.h
   /// @param relativeUrl: [In] String containing the relative URL of the Http request
   /// @param relativeUrlLen: [In] Length of the Relative URL string
   /// @return:
   /// HTTP_SUCCESS    - on success
   /// HTTP_FAILURE    - on failure
   ///
   virtual HttpReturnCode SendRequest (uint32           requestId,
                                         HttpMethodType method,
                                         const char*      relativeUrl,
                                         int               relativeUrlLen) = 0;


   ///
   /// @brief Gets the IP Address of the interface. (TODO: Not implemented)
   ///
   virtual char* GetIPAddr(int& size) = 0;

   ///
   /// @brief Helper function to check if a response to an
   ///        outstanding request has been received or not
   /// @param requestId: [In] Identifies the Http Request
   /// @return:
   /// HTTP_SUCCESS    - Yes, Response has been received for this request
   /// HTTP_FAILURE    - Error, request is in invalid state
   /// HTTP_WAIT       - No, waiting for response for a non-blocking socket
   ///
   virtual HttpReturnCode IsResponseReceived (uint32 requestId) = 0;

   ///
   /// @brief Gets the Http header
   /// (TODO: Not implemented, as we do not know if this is request header or response header)
   /// (Need to find out how this is used. Does not look like it from greping the source)
   /// Also the API needs an extra keyLenReq parameter, which is absent
   ///
   virtual HttpReturnCode GetHeader (uint32 requestId,
                                       const char* key,
                                       int keyLen,
                                       char* value,
                                       int valueLen,
                                       int* valueLenReq) = 0;

   ///
   /// @brief get Http data received in the response
   /// @param requestId: [In] Identifies the Http Request
   /// @param preAllocatedData: [In] Pre-allocated area in memory where this
   ///        function will memcpy the data received in the response
   /// @param preAllocatedDataLen: [In] Maximum length of pre-allocated area
   ///        in memory where this function will memcpy the data received in the response
   /// @param preAllocatedDataLenRequired: [Out] Actual size of data memcpy'd
   /// @return:
   ///   HTTP_SUCCESS     - Data available to read (*preAllocatedDataLenRequired indicates
   ///                      number of bytes read)
   ///   HTTP_FAILURE     - Invalid state for request or Connection aborted prematurely
   ///                      or generic failure to process request
   ///   HTTP_WAIT        - Zero bytes available to read (underrun), check back later
   ///   HTTP_NOMOREDATA  - End of file (data download complete)
   ///
   virtual HttpReturnCode GetData (uint32 requestId,
                                     char*  preAllocatedData,
                                     int    preAllocatedDataLen,
                                     int*   preAllocatedDataLenRequired) = 0;


   ///
   /// @brief Get the server response code. This should be called only
   ///        after the http response headers are received.
   /// @param requestId: [In] Identifies the Http Request
   /// @param nVal: [Out] Numeric value of response code.
   /// @return:
   ///   HTTP_SUCCESS     - Successfully obtained the response code.
   ///   HTTP_FAILURE     - Failed to get the response code.
   ///
   virtual HttpReturnCode GetResponseCode(uint32 requestId,
                                          uint32& nVal) = 0;

   ///
   /// @brief Get the Length of data received in response
   ///        to the Http request.
   /// @param requestId: [In] Identifies the Http Request
   /// @param contentLength: [Out] Content length for the main request
   /// @param bTotal: [In] ?? (Not Used here)
   /// @return:
   /// HTTP_SUCCESS    - on success
   /// HTTP_FAILURE    - on failure
   ///
   virtual HttpReturnCode GetContentLength (uint32 requestId,
                                              int64* contentLength,
                                              bool bTotal = false) = 0;

   ///
   /// @brief Get the content type from the Http Response
   /// @param requestId: [In] Identifies the Http Request
   /// @param preAllocatedContentType: [In] Preallocated area of memory
   ///        in which the content type will be written by this function
   /// @param preAllocatedContentTypeLen: [In] Length of the pre-allocated
   ///        are in memory where content type will be written by this function
   /// @param contentTypeLenRequired: [Out] Actual length of the content type field
   /// @return:
   /// HTTP_SUCCESS    - on success
   /// HTTP_FAILURE    - on failure
   ///
   virtual HttpReturnCode GetContentType (uint32 requestId,
                                            char*  preAllocatedContentType,
                                            int    preAllocatedContentTypeLen,
                                            int*   contentTypeLenRequired) = 0;


   ///
   /// @brief Helper function to set the option on the HttpStack
   /// @param optionType: [In] Options (like HTTP_DISABLE_AUTO_CLOSE)
   /// @param value: [In] Value of the option
   /// @return:
   /// HTTP_SUCCESS    - on success
   /// HTTP_FAILURE    - on failure
   ///
   virtual HttpReturnCode SetOption (HttpStackOption optionType,
                                       uint32 value) = 0;

   ///
   /// @brief Sets the network interface
   /// @param iface: [In] Interface Id to set
   ///
   virtual void SetNetworkInterface(int32 iface) = 0;


   ///
   /// @brief Set the primary PDP profile (DEPRECATED)
   /// @param profileNo: [In]
   ///
   virtual void SetPrimaryPDPProfile(int32 profileNo) = 0;

   ///
   /// @brief Returns the network interface
   /// @param iface: [Out] Interface Id
   ///
   virtual HttpReturnCode GetNetworkInterface(int32 &iface) = 0;

   ///
   /// @brief Get the primary PDP profile (DEPRECATED)
   /// @param profileNo: [Out]
   ///
   virtual HttpReturnCode GetPrimaryPDPProfile(int32 &profileNo) = 0;

   ///
   /// @brief Set the settings for the proxy server
   /// @param proxyServer: [In] Pointer to the string containing the proxyserver name
   /// @param proxyServerLen: [In] Size of the proxyserver string
   /// @return:
   /// HTTP_SUCCESS    - on success
   /// HTTP_FAILURE    - on failure
   ///
   virtual HttpReturnCode SetProxyServer (const char* proxyServer,
                                            int32 proxyServerLen) = 0;


   ///
   /// @brief Unset the proxy server information
   /// @return:
   /// HTTP_SUCCESS    - on success
   /// HTTP_FAILURE    - on failure
   ///
   virtual HttpReturnCode UnsetProxyServer(void) = 0;

   ///
   /// @brief Get the Proxy server settings
   /// @param preAllocatedProxyServer: [In] Pre-allocated area in memory where the
   ///        proxy server name will be copied by this function
   /// @param preAllocatedProxyServerLen: [In] Size of the pre-allocated area in memory
   /// @param preAllocatedProxyServerLenRequired: [Out] Actual size of proxyserver name
   /// @return:
   /// HTTP_SUCCESS    - on success
   /// HTTP_FAILURE    - on failure
   ///
   virtual HttpReturnCode GetProxyServer (char* preAllocatedProxyServer,
                                            int32 preAllocatedProxyServerLen,
                                            int32 & preAllocatedProxyServerLenRequired) = 0;

   ///
   /// @brief Sets the Socket mode to either blocking or non-blocking. ONLY
   ///        non-blocking mode is allowed
   /// @param socketMode: [In] Socket mode (blocking or non-blocking)
   /// @return:
   /// HTTP_SUCCESS    - on success
   /// HTTP_FAILURE    - on failure
   ///
   virtual HttpReturnCode SetSocketMode (HttpSocketType socketMode) = 0;

   ///
   /// @brief Gets the Socket mode that has been previously set
   /// @param socketMode: [Out] Socket mode (blocking or non-blocking)
   /// @return:
   /// HTTP_SUCCESS    - on success
   /// HTTP_FAILURE    - on failure
   ///
   virtual HttpReturnCode GetSocketMode (HttpSocketType* socketMode) = 0;

   ///
   /// @brief Abort
   ///
   virtual void SetNetAbort() = 0;

   ///
   /// @brief Helper function to determine if we can process a new request
   /// @return:
   /// 1    - Processing a request. Cannot accept a new request
   /// 0    - Can accept a new request
   ///
   virtual int IsProcessingARequest(const bool localRequest = false) = 0;

   ///
   /// @brief
   ///
   ///
   virtual void updateEstimates() = 0;

};
} //end of QTA namespace

#endif
