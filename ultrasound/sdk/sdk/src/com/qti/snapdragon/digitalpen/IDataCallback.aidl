/*===========================================================================
                           IDataCallback.aidl

Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/
package com.qti.snapdragon.digitalpen;

import com.qti.snapdragon.digitalpen.util.DigitalPenData;

/**
 * Note that this is a one-way interface so the server does not block waiting
 * for the client.
 */
oneway interface IDataCallback {
    /**
     * Called when the service gets new proprietary data from the daemon.
     */
    void onDigitalPenPropData(in DigitalPenData data);
}
