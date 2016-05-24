/*===========================================================================
                           IEventCallback.aidl

Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/
package com.qti.snapdragon.digitalpen;

import com.qti.snapdragon.digitalpen.util.DigitalPenEvent;

/**
 * Note that this is a one-way interface so the server does not get blocked
 * waiting for the client.
 */
oneway interface IEventCallback {
    /**
     * Called when the service gets new proprietary event from the daemon.
     */
    void onDigitalPenPropEvent(in DigitalPenEvent event);
}
