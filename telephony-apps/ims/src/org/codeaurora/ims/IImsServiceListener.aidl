/* Copyright (c)2012-2013 Qualcomm Technologies,Inc.
 * All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 */

package org.codeaurora.ims;

/**
 * Interface used to interact with IMS phone. {@hide}
 */
interface IImsServiceListener {

    /**
     * Get the changed ims registration state
     */
    void imsRegStateChanged(int regstate);

    /**
     * IMS registration state change request failure callback
     */
    void imsRegStateChangeReqFailed();

    /**
     * Service Status Update from ImsService
     * service - CALL_TYPE_VOICE, CALL_TYPE_VT_TX, CALL_TYPE_VT_RX
     *           CALL_TYPE_VT, CALL_TYPE_VT_NODIR, CALL_TYPE_CS_VS_TX
     *           CALL_TYPE_CS_VS_RX, CALL_TYPE_PS_VS_TX, CALL_TYPE_PS_VS_RX
     *           CALL_TYPE_UNKNOWN, CALL_TYPE_SMS
     * status -  STATUS_DISABLED, STATUS_PARTIALLY_ENABLED,
     *           STATUS_ENABLED , STATUS_NOT_SUPPORTED
     */
    void imsUpdateServiceStatus(int service, int status);
}
