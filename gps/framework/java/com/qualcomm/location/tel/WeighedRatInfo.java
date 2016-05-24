/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  Copyright (c) 2014 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  Copyright (c) 2012 Qualcomm Atheros, Inc.
  All Rights Reserved.
  Qualcomm Atheros Confidential and Proprietary.
=============================================================================*/
package com.qualcomm.location.tel;

import android.content.res.Resources;
import android.util.Log;
import android.telephony.CellLocation;
import android.telephony.cdma.CdmaCellLocation;
import android.telephony.gsm.GsmCellLocation;
import com.qualcomm.location.R;
import java.lang.StringBuilder;

public class WeighedRatInfo {
    private static final String TAG = "WeighedRatInfo";
    static private final boolean VERBOSE_DBG = Log.isLoggable(TAG, Log.VERBOSE);

    public static final int RAT_CDMA = 0;
    public static final int RAT_GSM = 1;
    public static final int RAT_WCDMA = 2;
    public static final int RAT_TDSCDMA = 3;
    public static final int RAT_LTE = 4;
    public static final int RAT_MAX = 5;

    public final int mWeight;
    public final int mRAT;
    public final int mLac;
    public final int mCid;
    public final int mPsc;
    public final int mBsid;
    public final int mBslat;
    public final int mBslon;
    public final int mNid;
    public final int mSid;
    public final int mMcc;
    public final int mMnc;
    public final boolean mRoaming;

    public String dump() {
        StringBuilder sb = new StringBuilder(TAG);
        sb.append(" - \n\t mWeight");
        sb.append(mWeight);
        sb.append("\n\t mRAT: ");
        sb.append(mRAT);
        sb.append("\n\t mLac: ");
        sb.append(mLac);
        sb.append("\n\t mCid: ");
        sb.append(mCid);
        sb.append("\n\t mPsc: ");
        sb.append(mPsc);
        sb.append("\n\t mBsid: ");
        sb.append(mBsid);
        sb.append("\n\t mBslat: 0x");
        sb.append(Integer.toHexString(mBslat));
        sb.append("\n\t mBslon: 0x");
        sb.append(Integer.toHexString(mBslon));
        sb.append("\n\t mNid: ");
        sb.append(mNid);
        sb.append("\n\t mSid: ");
        sb.append(mSid);
        sb.append("\n\t mMcc: ");
        sb.append(mMcc);
        sb.append("\n\t mMnc: ");
        sb.append(mMnc);
        sb.append("\n\t mRoaming: ");
        sb.append(mRoaming);

        return sb.toString();
    }


    private WeighedRatInfo(int weight, int rat, int lac, int cid, int psc,
                           int bsid, int bslat, int bslon, int nid, int sid,
                           int mcc, int mnc, boolean roaming) {
        mWeight = weight;
        mRAT = rat;
        mLac = lac;
        mCid = cid;
        mPsc = psc;
        mBsid = bsid;
        mBslat = bslat;
        mBslon = bslon;
        mNid = nid;
        mSid = sid;
        mMcc = mcc;
        mMnc = mnc;
        mRoaming = roaming;
    }

    /**
     * Constructs info container for unavailable cell
     *
     * @param weight Weight value from configuration
     * @param rat RAT
     */
    private WeighedRatInfo(int weight, int rat) {
        mWeight = weight;
        mRAT = rat;
        mLac = -1;
        mCid = -1;
        mPsc = -1;
        mBsid = -1;
        mBslat = Integer.MAX_VALUE;
        mBslon = Integer.MAX_VALUE;
        mNid = -1;
        mSid = -1;
        mMcc = 0;
        mMnc = 0;
        mRoaming = false;
    }

    public static WeighedRatInfo create(CellLocation cl, TelMgrProxy telMgr) {
        WeighedRatInfo wri = null;

        if (cl != null) {
            int weight = 0;
            int rat = -1;
            int lac = -1;
            int cid = -1;
            int psc = -1;
            int bsid = -1;
            int bslat = Integer.MAX_VALUE;
            int bslon = Integer.MAX_VALUE;
            int nid = -1;
            int sid = -1;
            String mccmnc = telMgr.getMncMccCombo();

            if (cl instanceof CdmaCellLocation) {
                CdmaCellLocation ccl = (CdmaCellLocation)cl;
                bsid = ccl.getBaseStationId();
                bslat = ccl.getBaseStationLatitude();
                bslon = ccl.getBaseStationLongitude();
                nid = ccl.getNetworkId();
                sid = ccl.getSystemId();

                rat = RAT_CDMA;
                weight = WEIGHTS[rat];
            } else if (cl instanceof GsmCellLocation) {
                GsmCellLocation gcl = (GsmCellLocation)cl;
                lac = gcl.getLac();
                cid = gcl.getCid();
                psc = gcl.getPsc();

                if (psc == -1) {
                    rat = RAT_GSM;
                } else if (isTDSCDMA(mccmnc)) {
                    rat = RAT_TDSCDMA;
                } else {
                    rat = RAT_WCDMA;
                }
                weight = WEIGHTS[rat];
            }

            if (weight > 0) {
                int mcc = telMgr.getMcc(mccmnc);
                int mnc = telMgr.getMnc(mccmnc);
                boolean roaming = telMgr.getRoaming();

                wri = new WeighedRatInfo(weight, rat, lac, cid, psc,
                                         bsid, bslat, bslon, nid, sid,
                                         mcc, mnc, roaming);
            }
        }
        return wri;
    }

    /**
     * Returns info container for unavailable cell
     *
     * @param rat RAT
     * @return Info for disconnected cell
     */
    public static WeighedRatInfo createUnavailable(int rat) {
        if (rat < 0 || rat >= RAT_MAX) {
            throw new IllegalArgumentException("Invalid RAT value");
        }

        return new WeighedRatInfo(WEIGHTS[rat], rat);
    }

    /**
     * Returns info container with given values
     *
     * @return New container instance or null if computed weight is 0
     */
    public static WeighedRatInfo create(int rat, int lac, int cid, int psc,
                           int bsid, int bslat, int bslon, int nid, int sid,
                           int mcc, int mnc, boolean roaming) {
        if (rat < 0 || rat >= RAT_MAX) {
            throw new IllegalArgumentException("Invalid RAT value");
        }

        int weight = WEIGHTS[rat];
        if (weight == 0) {
            return null;
        }

        return new WeighedRatInfo(weight, rat, lac, cid, psc,
                                  bsid, bslat, bslon, nid, sid,
                                  mcc, mnc, roaming);
    }

    public static void init(Resources r) {
        if (mKnownTdscdmaOperators == null) {
            try{
                mKnownTdscdmaOperators = r.getStringArray(
                    R.array.tdscdma_operators);

                logv("mKnownTdscdmaOperators - ");
                if (mKnownTdscdmaOperators != null) {
                    for (String s : mKnownTdscdmaOperators) {
                        logv("\t"+s);
                    }
                }

            } catch (Resources.NotFoundException rnfe) {
                Log.w(TAG, "resouce tdscdma_operators not defined");
            }

            try{
                WEIGHTS[RAT_CDMA] = r.getInteger(
                    R.integer.weight_cdma);
            } catch (Resources.NotFoundException rnfe) {
                Log.w(TAG, "resouce weight_cdma not defined");
            } finally {
                logv("RAT_CDMA - "+WEIGHTS[RAT_CDMA]);
            }

            try{
                WEIGHTS[RAT_GSM] = r.getInteger(
                    R.integer.weight_gsm);
            } catch (Resources.NotFoundException rnfe) {
                Log.w(TAG, "resouce weight_gsm not defined");
            } finally {
                logv("RAT_GSM - "+WEIGHTS[RAT_GSM]);
            }

            try{
                WEIGHTS[RAT_WCDMA] = r.getInteger(
                    R.integer.weight_wcdma);
            } catch (Resources.NotFoundException rnfe) {
                Log.w(TAG, "resouce weight_wcdma not defined");
            } finally {
                logv("RAT_WCDMA - "+WEIGHTS[RAT_WCDMA]);
            }

            try{
                WEIGHTS[RAT_TDSCDMA] = r.getInteger(
                    R.integer.weight_tdscdma);
            } catch (Resources.NotFoundException rnfe) {
                Log.w(TAG, "resouce weight_tdscdma not defined");
            } finally {
                logv("RAT_TDSCDMA - "+WEIGHTS[RAT_TDSCDMA]);
            }

            try{
                WEIGHTS[RAT_LTE] = r.getInteger(
                    R.integer.weight_lte);
            } catch (Resources.NotFoundException rnfe) {
                Log.w(TAG, "resouce weight_lte not defined");
            } finally {
                logv("RAT_LTE - "+WEIGHTS[RAT_LTE]);
            }
        }
    }

    // the indices of the weights must corresponds to
    // RAT IDs defined above.
    private static int[] WEIGHTS =
    {
        0, 0, 0, 0, 0, 0
    };

    // To detect if a network is TDSCDMA.. Android telephony does
    // not have deterministic way to tell TDSCDMA from WCDMA. The
    // workaround here is we compare the MCC+MNC string with the
    // known TDSCDMA operators defined in R.array.tdscdma_operators.
    private static String[] mKnownTdscdmaOperators;

    private static boolean isTDSCDMA(String mccmnc) {
        if (mKnownTdscdmaOperators != null) {
            for (String s : mKnownTdscdmaOperators) {
                if (s.equals(mccmnc)) {
                    return true;
                }
            }
        }

        return false;
    }

    static private void logv(String s) {
        if (VERBOSE_DBG) Log.v(TAG, s);
    }
}