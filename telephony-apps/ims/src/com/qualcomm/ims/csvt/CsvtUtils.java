/* Copyright (c) 2014 Qualcomm Technologies, Inc.
 * All Rights Reserved. Qualcomm Technologies Confidential and Proprietary.
 */

package com.qualcomm.ims.csvt;

import com.android.internal.telephony.Call;
import com.android.internal.telephony.CallDetails;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.Phone;

import org.codeaurora.ims.ImsConnection;

public class CsvtUtils {

    /**
     * Tests if the connection is a Csvt connection.
     * @param c Connection to test.
     * @return true if {@code c} is a Csvt connection, false otherwise.
     */
    public static boolean isCsvtConnection(Connection c) {
        if (c == null || !(c instanceof ImsConnection)) {
            return false;
        }
        CallDetails cd = ((ImsConnection) c).getCallDetails();
        return (cd.call_type == CallDetails.CALL_TYPE_VT)
                && (cd.call_domain == CallDetails.CALL_DOMAIN_CS);
    }
    /**
     * Tests if the call contains a Csvt connection.
     * @param call call to test.
     * @return true if {@code call} contains a Csvt connection,
     * false otherwise.
     */

    public static boolean hasCsvtConnection(Call call)
    {
        return getCsvtConnection(call) != null ? true : false;
    }

    public static Connection getCsvtConnection(Call call)
    {
        if (call != null) {
            for (Connection c : call.getConnections() ) {
                if( isCsvtConnection(c) ) {
                    return c;
                }
            }
        }
        return null;
    }

    public static boolean hasActiveCsvtConnection(Call call) {
        return (call != null) && (! call.isIdle()) && hasCsvtConnection(call);
    }

    public static boolean hasActiveCsvtConnection(Phone phone) {
        return hasActiveFgCsvtConnection(phone) || hasActiveBgCsvtConnection(phone);
    }

    public static boolean hasActiveFgCsvtConnection(Phone phone) {
        return phone != null && hasActiveCsvtConnection(
                phone.getForegroundCall() );
    }

    public static boolean hasActiveBgCsvtConnection(Phone phone) {
        return phone != null && hasActiveCsvtConnection(
                phone.getBackgroundCall() );
    }

    public static boolean hasActiveRingingCsvtConnection(Phone phone) {
        return phone != null && hasActiveCsvtConnection(
                phone.getRingingCall() );
    }

}
