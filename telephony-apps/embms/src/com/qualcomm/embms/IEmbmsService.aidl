/******************************************************************************
 * @file    IEmbmsService.aidl
 * @brief   This interface describes the APIs for the service which allows
 *          clients to enable/disable the eMBMS service and also activate
 *          and de-activate available bearers
 *          Clients should bind to the service name:
 *          "com.qualcomm.embms.IEmbmsService"
 *
 * @version 00.04.22
 *
 * ---------------------------------------------------------------------------
 * Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 * ---------------------------------------------------------------------------
 *
 *******************************************************************************/

package com.qualcomm.embms;

import com.qualcomm.embms.IEmbmsServiceCallback;

interface IEmbmsService {

    /**
     * registerCallback will be used by client to register a callback to be
     * notified asynchronously
     *
     * @param debugTraceId
     *    Defines the trace id of the request for debugging purpose.
     *    0 to 32768 - Represents a valid trace id
     *    -1 - Represents that this parameter is not used
     *
     * @param cb
     *    Defines the callback interface
     *
     * @return
     *    SUCCESS 0
     *       Action taken by eMBMS service is successful
     *    ERROR_UNKNOWN 1
     *       Action taken by eMBMS service has failed due to unknown reason
     *    ERROR_INVALID_PARAMETER 3
     *       Action taken by eMBMS service has failed as atleast one of the
     *       parameter added was invalid
     */
    int registerCallback(in int debugTraceId, in IEmbmsServiceCallback cb);

    /**
     * deregisterCallback will be used by client to deregister a callback to be
     * notified asynchronously
     *
     * @param debugTraceId
     *    Defines the trace id of the request for debugging	purpose.
     *    0 to 32768 - Represents a valid trace id
     *    -1 - Represents that this parameter is not used
     *
     * @param cb
     *    Defines the callback interface
     *
     * @return
     *    SUCCESS 0
     *       Action taken by eMBMS service is successful
     *    ERROR_UNKNOWN 1
     *       Action taken by eMBMS service has failed due to unknown reason
     *    ERROR_INVALID_PARAMETER 3
     *       Action taken by eMBMS service has failed as atleast one of the
     *       parameter added was invalid
     */
    int deregisterCallback(in int debugTraceId, in IEmbmsServiceCallback cb);

    /**
     * enable will be used to enable the broadcast capability of the device
     *
     * @param debugTraceId
     *    Defines the trace id of the request for debugging	purpose.
     *    0 to 32768 - Represents a valid trace id
     *    -1 - Represents that this parameter is not used
     *
     * @return
     *    SUCCESS 0
     *       This call was accepted by eMBMS service. This does not mean that
     *       all actions taken by eMBMS service as response to this call is
     *       successful
     *    ERROR_UNKNOWN 1
     *       Action taken by eMBMS service has failed due to unknown reason
     *    ERROR_REGISTRATION_MISSING 2
     *       Action taken by eMBMS service has failed due missing callback
     *       registration from client
     *    ERROR_INVALID_PARAMETER 3
     *       Action taken by eMBMS service has failed as atleast one of the
     *       parameter added was invalid
     *    ERROR_SERVICE_NOT_READY 4
     *       Action to be taken by eMBMS service has cannot be completed
     *       as it was not ready to accept this request
     */
    int enable (in int debugTraceId);

    /**
     * disable will be used to disable the broadcast capability of the device
     *
     * @param debugTraceId
     *    Defines the trace id of the request for debugging	purpose.
     *    0 to 32768 - Represents a valid trace id
     *    -1 - Represents that this parameter is not used
     *
     * @return
     *    SUCCESS 0
     *       This call was accepted by eMBMS service. This does not mean that
     *       all actions taken by eMBMS service as response to this call is
     *       successful
     *    ERROR_UNKNOWN 1
     *       Action taken by eMBMS service has failed due to unknown reason
     *    ERROR_REGISTRATION_MISSING 2
     *       Action taken by eMBMS service has failed due missing callback
     *       registration from client
     *    ERROR_INVALID_PARAMETER 3
     *       Action taken by eMBMS service has failed as atleast one of the
     *       parameter added was invalid
     *    ERROR_SERVICE_NOT_READY 4
     *       Action to be taken by eMBMS service has cannot be completed
     *       as it was not ready to accept this request
     */
    int disable(in int debugTraceId);

    /**
     * activateTMGI will be used to activate a TMGI
     *
     * @param debugTraceId
     *    Defines the trace id of the request for debugging	purpose.
     *    0 to 32768 - Represents a valid trace id
     *    -1 - Represents that this parameter is not used
     *
     * @param tmgi
     *    A byte array representing a 6 byte TMGI that needs to be
     *    activated
     *
     * @param preemptionPriority
     *    The preemption priority of TMGI. It can take a value from 0 to 5
     *    Any value greater than 5 will be considered as 5 and any value
     *    less than 0 will be considered as 0.
     *    0 - Lowest Priority
     *    5 - Highest Priority
     *
     * @param saiList
     *    Gives the list of SAI where the requested TMGI is present as per
     *    the information available to client
     *
     * @param earfcnList
     *    Gives the list of frequencies(EARFCN) on which the TMGI to be activated
     *    is available. The values of this list can take a range of 0 to 65535
     *
     * @return
     *    SUCCESS 0
     *       This call was accepted by eMBMS service. This does not mean that
     *       all actions taken by eMBMS service as response to this call is
     *       successful
     *    ERROR_UNKNOWN 1
     *       Action taken by eMBMS service has failed due to unknown reason
     *    ERROR_REGISTRATION_MISSING 2
     *       Action taken by eMBMS service has failed due missing callback
     *       registration from client
     *    ERROR_INVALID_PARAMETER 3
     *       Action taken by eMBMS service has failed as atleast one of the
     *       parameter added was invalid
     *    ERROR_SERVICE_NOT_READY 4
     *       Action to be taken by eMBMS service has cannot be completed
     *       as it was not ready to accept this request
     */
    int activateTMGI(in int debugTraceId, in byte[] tmgi, in int preemptionPriority,
                        in int[] saiList, in int[] earfcnList);

    /**
     * deactivateTMGI will be used to deactivate a TMGI
     *
     * @param debugTraceId
     *    Defines the trace id of the request for debugging	purpose.
     *    0 to 32768 - Represents a valid trace id
     *    -1 - Represents that this parameter is not used
     *
     * @param tmgi
     *    A byte array representing a 6 byte TMGI that needs to be
     *    deactivated
     *
     * @return
     *    SUCCESS 0
     *       This call was accepted by eMBMS service. This does not mean that
     *       all actions taken by eMBMS service as response to this call is
     *       successful
     *    ERROR_UNKNOWN 1
     *       Action taken by eMBMS service has failed due to unknown reason
     *    ERROR_REGISTRATION_MISSING 2
     *       Action taken by eMBMS service has failed due missing callback
     *       registration from client
     *    ERROR_INVALID_PARAMETER 3
     *       Action taken by eMBMS service has failed as atleast one of the
     *       parameter added was invalid
     *    ERROR_SERVICE_NOT_READY 4
     *       Action to be taken by eMBMS service has cannot be completed
     *       as it was not ready to accept this request
     */
    int deactivateTMGI(in int debugTraceId, in byte[] tmgi);

    /**
     * actDeactTMGI will be used to activate a TMGI and deactivate another
     * TMGI in the same call.
     *
     * @param debugTraceId
     *    Defines the trace id of the request for debugging	purpose.
     *    0 to 32768 - Represents a valid trace id
     *    -1 - Represents that this parameter is not used
     *
     * @param actTMGI
     *    A byte array representing a 6 byte TMGI that needs to be
     *    activated
     *
     * @param preemptionPriority
     *    The preemption priority of TMGI to be activated. It can take a value
     *    of 0 to 5. Any value greater than 5 will be considered as 5 and any
     *    value less than 0 will be considered as 0.
     *    0 - Lowest Priority
     *    5 - Highest Priority
     *
     * @param saiList
     *    Gives the list of SAI where the requested TMGI is present as per
     *    the information available to client
     *
     * @param earfcnList
     *    Gives the list of frequencies(EARFCN) on which the TMGI to be activated
     *    is available. The values of this list can take a range of 0 to 65535
     *
     * @param deactTMGI
     *    A byte array representing a 6 byte TMGI that needs to be
     *    deactivated
     *
     * @return
     *    SUCCESS 0
     *       This call was accepted by eMBMS service. This does not mean that
     *       all actions taken by eMBMS service as response to this call is
     *       successful
     *    ERROR_UNKNOWN 1
     *       Action taken by eMBMS service has failed due to unknown reason
     *    ERROR_REGISTRATION_MISSING 2
     *       Action taken by eMBMS service has failed due missing callback
     *       registration from client
     *    ERROR_INVALID_PARAMETER 3
     *       Action taken by eMBMS service has failed as atleast one of the
     *       parameter added was invalid
     *    ERROR_SERVICE_NOT_READY 4
     *       Action to be taken by eMBMS service has cannot be completed
     *       as it was not ready to accept this request
     */
    int actDeactTMGI(in int debugTraceId, in byte[] actTMGI, in int preemptionPriority,
                        in int[] saiList, in int[] earfcnList, in byte[] deactTMGI);

    /**
     * getAvailableTMGIList will be used by client if it needs the current list
     * of available TMGI's on broadcast
     *
     * @param debugTraceId
     *    Defines the trace id of the request for debugging	purpose.
     *    0 to 32768 - Represents a valid trace id
     *    -1 - Represents that this parameter is not used
     *
     * @return
     *    SUCCESS 0
     *       This call was accepted by eMBMS service. This does not mean that
     *       all actions taken by eMBMS service as response to this call is
     *       successful
     *    ERROR_UNKNOWN 1
     *       Action taken by eMBMS service has failed due to unknown reason
     *    ERROR_REGISTRATION_MISSING 2
     *       Action taken by eMBMS service has failed due missing callback
     *       registration from client
     *    ERROR_INVALID_PARAMETER 3
     *       Action taken by eMBMS service has failed as atleast one of the
     *       parameter added was invalid
     *    ERROR_SERVICE_NOT_READY 4
     *       Action to be taken by eMBMS service has cannot be completed
     *       as it was not ready to accept this request
     */
    int getAvailableTMGIList(in int debugTraceId);

    /**
     * getActiveTMGIList will be used by client if it needs the current list of
     * active TMGI's
     *
     * @param debugTraceId
     *    Defines the trace id of the request for debugging	purpose.
     *    0 to 32768 - Represents a valid trace id
     *    -1 - Represents that this parameter is not used
     *
     * @return
     *    SUCCESS 0
     *       This call was accepted by eMBMS service. This does not mean that
     *       all actions taken by eMBMS service as response to this call is
     *       successful
     *    ERROR_UNKNOWN 1
     *       Action taken by eMBMS service has failed due to unknown reason
     *    ERROR_REGISTRATION_MISSING 2
     *       Action taken by eMBMS service has failed due missing callback
     *       registration from client
     *    ERROR_INVALID_PARAMETER 3
     *       Action taken by eMBMS service has failed as atleast one of the
     *       parameter added was invalid
     *    ERROR_SERVICE_NOT_READY 4
     *       Action to be taken by eMBMS service has cannot be completed
     *       as it was not ready to accept this request
     */
    int getActiveTMGIList(in int debugTraceId);

    /**
     * getCoverageState will be used by client to get the the current coverage
     * state
     *
     * @param debugTraceId
     *    Defines the trace id of the request for debugging	purpose.
     *    0 to 32768 - Represents a valid trace id
     *    -1 - Represents that this parameter is not used
     *
     * @return
     *    SUCCESS 0
     *       This call was accepted by eMBMS service. This does not mean that
     *       all actions taken by eMBMS service as response to this call is
     *       successful
     *    ERROR_UNKNOWN 1
     *       Action taken by eMBMS service has failed due to unknown reason
     *    ERROR_REGISTRATION_MISSING 2
     *       Action taken by eMBMS service has failed due missing callback
     *       registration from client
     *    ERROR_INVALID_PARAMETER 3
     *       Action taken by eMBMS service has failed as atleast one of the
     *       parameter added was invalid
     *    ERROR_SERVICE_NOT_READY 4
     *       Action to be taken by eMBMS service has cannot be completed
     *       as it was not ready to accept this request
     */
    int getCoverageState(in int debugTraceId);

    /**
     * getSignalStrength will be used by client to get the the current signal
     * strength
     *
     * @param debugTraceId
     *    Defines the trace id of the request for debugging	purpose.
     *    0 to 32768 - Represents a valid trace id
     *    -1 - Represents that this parameter is not used
     *
     * @return
     *    SUCCESS 0
     *       This call was accepted by eMBMS service. This does not mean that
     *       all actions taken by eMBMS service as response to this call is
     *       successful
     *    ERROR_UNKNOWN 1
     *       Action taken by eMBMS service has failed due to unknown reason
     *    ERROR_REGISTRATION_MISSING 2
     *       Action taken by eMBMS service has failed due missing callback
     *       registration from client
     *    ERROR_INVALID_PARAMETER 3
     *       Action taken by eMBMS service has failed as atleast one of the
     *       parameter added was invalid
     *    ERROR_SERVICE_NOT_READY 4
     *       Action to be taken by eMBMS service has cannot be completed
     *       as it was not ready to accept this request
     */
    int getSignalStrength(in int debugTraceId);

    /**
     * getVersion will be used by client to get the version of the interface
     * implemented by eMBMS service
     *
     * @param debugTraceId
     *    Defines the trace id of the request for debugging purpose.
     *    0 to 32768 - Represents a valid trace id
     *    -1 - Represents that this parameter is not used
     *
     * @return
     *     Version of the interface implemented by eMBMS service.
     */
    String getVersion(in int debugTraceId);

    /**
     * getActiveLogPacketIDs will be used by client to get the
     * current list of active client log packets ids
     *
     * @param debugTraceId
     *    Defines the trace id of the request for debugging purpose.
     *    0 to 32768 - Represents a valid trace id
     *    -1 - Represents that this parameter is not used
     *
     * @param supportedLogPacketIdList
     *    Defines the list of client log packet id's which the client supports.
     *
     * @return
     *    SUCCESS 0
     *       This call was accepted by eMBMS service. This does not mean that
     *       all actions taken by eMBMS service as response to this call is
     *       successful
     *    ERROR_UNKNOWN 1
     *       Action taken by eMBMS service has failed due to unknown reason
     *    ERROR_REGISTRATION_MISSING 2
     *       Action taken by eMBMS service has failed due missing callback
     *       registration from client
     *    ERROR_INVALID_PARAMETER 3
     *       Action taken by eMBMS service has failed as atleast one of the
     *       parameter added was invalid
     *    ERROR_SERVICE_NOT_READY 4
     *       Action to be taken by eMBMS service has cannot be completed
     *       as it was not ready to accept this request
     */
    int getActiveLogPacketIDs(in int debugTraceId, in int[] supportedLogPacketIdList);

    /**
     * deliverLogPacket will be used by client to deliver a specific client log
     * packet.
     *
     * @param debugTraceId
     *    Defines the trace id of the request for debugging purpose.
     *    0 to 32768 - Represents a valid trace id
     *    -1 - Represents that this parameter is not used
     *
     * @param logPacketId
     *    Defines the log packet id of the logPacket which is being
     *    delivered with this call.
     *    0 to 255 - Represents a valid range of client log packet id's
     *
     * @param logPacket
     *    Contains the client log packet data in byte array format
     *    corresponding to the logPacketId
     *
     * @return
     *    SUCCESS 0
     *       This call was accepted by eMBMS service. This does not mean that
     *       all actions taken by eMBMS service as response to this call is
     *       successful
     *    ERROR_UNKNOWN 1
     *       Action taken by eMBMS service has failed due to unknown reason
     *    ERROR_REGISTRATION_MISSING 2
     *       Action taken by eMBMS service has failed due missing callback
     *       registration from client
     *    ERROR_INVALID_PARAMETER 3
     *       Action taken by eMBMS service has failed as atleast one of the
     *       parameter added was invalid
     *    ERROR_SERVICE_NOT_READY 4
     *       Action to be taken by eMBMS service has cannot be completed
     *       as it was not ready to accept this request
     */
    int deliverLogPacket(in int debugTraceId, in int logPacketId, in byte[] logPacket);

    /**
     * getTime will be used by client when it wants the synchronized time.
     *
     * @param debugTraceId
     *    Defines the trace id of the request for debugging purpose.
     *    0 to 32768 - Represents a valid trace id
     *    -1 - Represents that this parameter is not used
     *
     * @return
     *    SUCCESS 0
     *       This call was accepted by eMBMS service. This does not mean that
     *       all actions taken by eMBMS service as response to this call is
     *       successful
     *    ERROR_UNKNOWN 1
     *       Action taken by eMBMS service has failed due to unknown reason
     *    ERROR_REGISTRATION_MISSING 2
     *       Action taken by eMBMS service has failed due missing callback
     *       registration from client
     *    ERROR_INVALID_PARAMETER 3
     *       Action taken by eMBMS service has failed as atleast one of the
     *       parameter added was invalid
     *    ERROR_SERVICE_NOT_READY 4
     *       Action to be taken by eMBMS service has cannot be completed
     *       as it was not ready to accept this request
     */
    int getTime(in int debugTraceId);

}
