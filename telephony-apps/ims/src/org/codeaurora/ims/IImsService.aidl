/* Copyright (c) 2012 Qualcomm Technologies, Inc.
 * All Rights Reserved. Qualcomm Technologies Confidential and Proprietary.
 */

package org.codeaurora.ims;

import org.codeaurora.ims.IImsServiceListener;
import android.os.Messenger;
import android.os.Message;

/**
 * Interface used to interact with IMS phone.
 *
 * {@hide}
 */
interface IImsService {

    /**
     * Register callback
     * @param imsServListener - IMS Service Listener
     */
    int registerCallback(IImsServiceListener imsServListener);

    /**
     * Deregister callback
     * @param imsServListener - IMS Service Listener
     */
    int deregisterCallback(IImsServiceListener imsServListener);

    /**
     * Set IMS Registration state
     * @param imsRegState - IMS Registration state
     */
    void setRegistrationState(int imsRegState);

    /**
     * Get IMS Registration state
     */
    int getRegistrationState();

    /**
     * Query the Service State for all IMS services
     */
    void queryImsServiceStatus(int event, in Messenger msgr);

    /**
     * Set the Service State for a services
     */
    void setServiceStatus(int service, int networkType, int enabled, int restrictCause,
            int event, in Messenger msgr);

    /**
     * Query for current video call quality.
     * @param response - Message object is used to send back the status and quality value.
     * Message.arg1 contains 0 if the request succeeded, non-zero otherwise.
     * Message.obj int[] array, which int[0] element contains video quality value: 0-LOW; 1-HIGH.
     * Message.replyTo must be a valid Messenger.
     */
    void queryVtQuality(in Message response);

    /**
     * Set for current video call quality.
     * @param quality - Video call quality to set: 0-LOW; 1-HIGH.
     * @param response - Message object is used to send back the status and quality value.
     * Message.arg1 contains 0 if the request succeeded, non-zero otherwise.
     * Message.replyTo must be a valid Messenger.
     */
    void setVtQuality(int quality, in Message response);
}

