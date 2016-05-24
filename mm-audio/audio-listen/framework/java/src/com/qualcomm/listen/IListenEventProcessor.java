/*
 *     Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
 *     Qualcomm Technologies Proprietary and Confidential.
 */

package com.qualcomm.listen;

import com.qualcomm.listen.ListenTypes.EventData;

/**
 * Applications must provide an implementation of this
 * interface to receive events from ListenEngine.
 */
public interface IListenEventProcessor {

    /**
     * Processes event received from ListenEngine
     * <p>
     *
     * Acts as a callback to client when Listen event is serviced.
     * The application can ignore these events or look at the event
     * type and take some appropriate action.
     * Each class that inherits from ListenReceiver
     * will send a different set of events to this callback.
     * See ListenTypes in ListenTypes.java for a list of possible
     * events this method could receive.
     * @param  eventType [in] type of event from ListenTypes
     * @param  eventData [in] event payload
     */
	public void processEvent(int                 eventType,
                             EventData           eventData);

}