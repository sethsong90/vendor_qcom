/******************************************************************************
 * @file    QcNvItems.java
 * @brief   Implementation of the IServiceProgramming interface functions used to get/set
 *          various NV parameters
 *
 * ---------------------------------------------------------------------------
 *  Copyright (C) 2010 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 * ---------------------------------------------------------------------------
 *
 *******************************************************************************/

package com.qualcomm.qcnvitems;

import com.qualcomm.qcnvitems.QcNvItemTypes.*;
import com.qualcomm.qcnvitems.QmiNvItemTypes.AmrStatus;
import com.qualcomm.qcnvitems.QmiNvItemTypes.AutoAnswer;
import com.qualcomm.qcnvitems.QmiNvItemTypes.CdmaChannels;
import com.qualcomm.qcnvitems.QmiNvItemTypes.MinImsi;
import com.qualcomm.qcnvitems.QmiNvItemTypes.PreferredVoiceSo;
import com.qualcomm.qcnvitems.QmiNvItemTypes.SidNid;
import com.qualcomm.qcnvitems.QmiNvItemTypes.Threegpp2Info;
import com.qualcomm.qcnvitems.QmiNvItemTypes.TimerCount;
import com.qualcomm.qcnvitems.QmiNvItemTypes.TrueImsi;
import com.qualcomm.qcnvitems.QmiNvItemTypes.VoiceConfig;
import com.qualcomm.qcrilhook.IQcRilHook;
import com.qualcomm.qcrilhook.PrimitiveParser;
import com.qualcomm.qcrilhook.QcRilHook;

import android.content.Context;
import android.os.AsyncResult;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.security.InvalidParameterException;

public class QcNvItems implements IQcNvItems {

    private static String LOG_TAG = "QC_NV_ITEMS";

    private static final int HEADER_SIZE = 8;

    private QcRilHook mQcRilOemHook;

    private static final boolean enableVLog = true;

    public QcNvItems(Context context) {
        super();
        vLog("QcNvItems instance created.");

        mQcRilOemHook = new QcRilHook(context);
    }

    public void dispose() {
        mQcRilOemHook.dispose();
        mQcRilOemHook = null;
    }

    // IO Functions
    private static void vLog(String logString) {
        if (enableVLog) {
            Log.v(LOG_TAG, logString);
        }
    }

    private void doNvWrite(BaseQCNvItemType nvItem, int itemId) throws IOException {
        vLog(nvItem.toDebugString());

        ByteBuffer buf = ByteBuffer.allocate(HEADER_SIZE + nvItem.getSize());
        buf.order(ByteOrder.nativeOrder());
        buf.putInt(itemId);
        buf.putInt(nvItem.getSize());
        buf.put(nvItem.toByteArray());

        AsyncResult result = mQcRilOemHook.sendQcRilHookMsg(IQcRilHook.QCRILHOOK_NV_WRITE, buf
                .array());

        if (result.exception != null) {
            Log.w(LOG_TAG, String.format("doNvWrite() Failed : %s", result.exception.toString()));
            result.exception.printStackTrace();
            throw new IOException();
        }
    }

    public static String bytesToHexString(byte[] bytes) {
	    if (bytes == null) return null;
	    StringBuilder ret = new StringBuilder(2*bytes.length);
	    for (int i = 0 ; i < bytes.length ; i++) {
		    int b;
		    b = 0x0f & (bytes[i] >> 4);
		    ret.append("0123456789abcdef".charAt(b));
		    b = 0x0f & bytes[i];
		    ret.append("0123456789abcdef".charAt(b));
	    }
	    return ret.toString();
    }

    private byte[] doNvRead(int itemId) throws IOException {

        AsyncResult result = mQcRilOemHook.sendQcRilHookMsg(IQcRilHook.QCRILHOOK_NV_READ, itemId);
        if (result.exception != null) {
            Log.w(LOG_TAG, String.format("doNvRead() Failed : %s", result.exception.toString()));
            result.exception.printStackTrace();
            throw new IOException();
        }

        vLog("Received: " + bytesToHexString((byte[]) result.result));

        return (byte[]) result.result;
    }

    // NAS
    public void updateAkey(String akey) throws IOException, InvalidParameterException {
        // Not implemented in NV
    }

    public Threegpp2Info get3gpp2SubscriptionInfo() throws IOException {
        String dirNum = getDirectoryNumber();
        SidNid sidNid = getHomeSidNid();
        MinImsi minImsi = getMinImsi();
        TrueImsi trueImsi = getTrueImsi();
        CdmaChannels cdmaChannels = getCdmaChannels();
        return new Threegpp2Info("", dirNum, sidNid, minImsi, trueImsi, cdmaChannels);
    }

    public void set3gpp2SubscriptionInfo(Threegpp2Info threegpp2Info) throws IOException,
            InvalidParameterException {
        if (threegpp2Info == null) {
            throw new InvalidParameterException();
        }
        setDirectoryNumber(threegpp2Info.getDirNum());
        setHomeSidNid(threegpp2Info.getSidNid());
        setMinImsi(threegpp2Info.getMinImsi());
        setTrueImsi(threegpp2Info.getTrueImsi());
        setCdmaChannels(threegpp2Info.getCdmaChannels());
    }

    public String getNamName() throws IOException {
        // Not implemented in NV
        return "";
    }

    public int getAnalogHomeSid() throws IOException {
        vLog("getAnalogHomeSid()");

        NvSidType o = new NvSidType(doNvRead(QcNvItemIds.NV_ANALOG_HOME_SID_I));
        vLog(o.toDebugString());

        return o.mSid;
    }

    public void setAnalogHomeSid(int analogHomeSid) throws IOException, InvalidParameterException {
        vLog("setAnalogHomeSid()");

        NvSidType o = new NvSidType();
        try {
            o.mSid = PrimitiveParser.parseShort(analogHomeSid);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }

        doNvWrite(o, QcNvItemIds.NV_ANALOG_HOME_SID_I);
    }

    /*
     * NV_DIR_NUMBER is preferred over the MIN-based directory Number here for
     * maximum compatibility with QMI
     */
    public String getDirectoryNumber() throws IOException {
        vLog("getDirectoryNumber()");
        /*
         * NvMin1Type min1 = new
         * QcNvItemTypes.NvMin1Type(doNvRead(QcNvItemIds.NV_MIN1_I)); NvMin2Type
         * min2 = new QcNvItemTypes.NvMin2Type(doNvRead(QcNvItemIds.NV_MIN2_I));
         * return MinImsi.minToPhString(min1.mMin1[0], min2.mMin2[0]);
         */
        return getNvDirNumber();
    }

    /*
     * Sets both the min-constructed MDN and NV_DIR_NUMBER_I.
     */
    public void setDirectoryNumber(String phNumber) throws IOException, InvalidParameterException {
        vLog("setDirectoryNumber()");

        if (phNumber == null) {
            throw new InvalidParameterException();
        }
        NvMin1Type min1 = new QcNvItemTypes.NvMin1Type(doNvRead(QcNvItemIds.NV_MIN1_I));
        NvMin2Type min2 = new QcNvItemTypes.NvMin2Type(doNvRead(QcNvItemIds.NV_MIN2_I));

        min1.mMin1[0] = MinImsi.phStringToMin1(phNumber);
        min2.mMin2[0] = MinImsi.phStringToMin2(phNumber);

        doNvWrite(min1, QcNvItemIds.NV_MIN1_I);
        doNvWrite(min2, QcNvItemIds.NV_MIN2_I);
        setNvDirNumber(phNumber);
    }

    private String getNvDirNumber() throws IOException {
        vLog("getNvDirNumber()");

        NvDirNumberType retVal = new QcNvItemTypes.NvDirNumberType(
                doNvRead(QcNvItemIds.NV_DIR_NUMBER_I));
        vLog(retVal.toDebugString());

        return retVal.getDirNumber();
    }

    private void setNvDirNumber(String dirNumber) throws IOException, InvalidParameterException {
        vLog("setNvDirNumber()");

        if (dirNumber == null) {
            throw new InvalidParameterException();
        }

        NvDirNumberType o = new NvDirNumberType();
        o.mNam = 0;
        o.setDirNumber(dirNumber);

        doNvWrite(o, QcNvItemIds.NV_DIR_NUMBER_I);
    }

    public SidNid getHomeSidNid() throws IOException {
        vLog("getHomeSidNid()");

        NvHomeSidNidType o = new NvHomeSidNidType(doNvRead(QcNvItemIds.NV_HOME_SID_NID_I));
        vLog(o.toDebugString());

        int[] sid = new int[IQcNvItems.NV_MAX_HOME_SID_NID];
        int[] nid = new int[IQcNvItems.NV_MAX_HOME_SID_NID];

        for (int i = 0; i < IQcNvItems.NV_MAX_HOME_SID_NID; i++) {
            sid[i] = PrimitiveParser.toUnsigned(o.mPair[i].mSid);
            nid[i] = PrimitiveParser.toUnsigned(o.mPair[i].mNid);
        }

        return new SidNid(sid, nid);
    }

    public void setHomeSidNid(SidNid homeSidNid) throws IOException, InvalidParameterException {
        vLog("setHomeSidNid()");

        if (homeSidNid == null) {
            throw new InvalidParameterException();
        }

        NvHomeSidNidType o = new NvHomeSidNidType();

        for (int i = 0; i < IQcNvItems.NV_MAX_HOME_SID_NID; i++) {
            short sid;
            short nid;
            try {
                sid = PrimitiveParser.parseShort(homeSidNid.getSid(i));
                nid = PrimitiveParser.parseShort(homeSidNid.getNid(i));
            } catch (NumberFormatException e) {
                throw new InvalidParameterException(e.toString());
            }

            o.mPair[i] = new NvSidNidPairType();
            o.mPair[i].mSid = sid;
            o.mPair[i].mNid = nid;
        }

        doNvWrite(o, QcNvItemIds.NV_HOME_SID_NID_I);
    }

    public MinImsi getMinImsi() throws IOException {
        vLog("getMinImsi()");
        String mcc = getImsiMcc();
        String imsi11_12 = getImsi11_12();
        String num = getMinImsiNumber();
        String s1 = num.substring(0, MinImsi.IMSI_S1_SIZE);
        String s2 = num.substring(num.length() - MinImsi.IMSI_S2_SIZE, num.length());
        return new MinImsi(mcc, imsi11_12, s1, s2);
    }

    public void setMinImsi(MinImsi minImsi) throws IOException, InvalidParameterException {
        vLog("setMinImsi()");
        if (minImsi == null) {
            throw new InvalidParameterException();
        }
        setImsiMcc(minImsi.getMcc());
        setImsi11_12(minImsi.getImsi11_12());
        setMinImsiNumber(minImsi.getImsiNumber());
    }

    public String getMinImsiNumber() throws IOException {
        vLog("getMinImsiNumber()");

        NvMin1Type min1 = new QcNvItemTypes.NvMin1Type(doNvRead(QcNvItemIds.NV_MIN1_I));
        NvMin2Type min2 = new QcNvItemTypes.NvMin2Type(doNvRead(QcNvItemIds.NV_MIN2_I));

        return MinImsi.minToPhString(min1.mMin1[1], min2.mMin2[1]);
    }

    public void setMinImsiNumber(String phNumber) throws IOException, InvalidParameterException {
        vLog("setMinImsiNumber()");

        if (phNumber == null) {
            throw new InvalidParameterException();
        }
        NvMin1Type min1 = new QcNvItemTypes.NvMin1Type(doNvRead(QcNvItemIds.NV_MIN1_I));
        NvMin2Type min2 = new QcNvItemTypes.NvMin2Type(doNvRead(QcNvItemIds.NV_MIN2_I));

        min1.mMin1[1] = MinImsi.phStringToMin1(phNumber);
        min2.mMin2[1] = MinImsi.phStringToMin2(phNumber);

        doNvWrite(min1, QcNvItemIds.NV_MIN1_I);
        doNvWrite(min2, QcNvItemIds.NV_MIN2_I);
    }

    public String getImsiMcc() throws IOException {
        vLog("getImsiMcc()");

        NvImsiMccType o = new QcNvItemTypes.NvImsiMccType(doNvRead(QcNvItemIds.NV_IMSI_MCC_I));
        vLog(o.toDebugString());

        return PrimitiveParser.toUnsignedString(o.mImsiMcc);
    }

    public void setImsiMcc(String imsiMcc) throws IOException, InvalidParameterException {
        vLog("setImsiMcc()");

        if (imsiMcc == null || imsiMcc.length() != MinImsi.MCC_SIZE) {
            throw new InvalidParameterException();
        }

        NvImsiMccType o = new NvImsiMccType();
        o.mNam = 0;
        try {
            o.mImsiMcc = PrimitiveParser.parseUnsignedShort(imsiMcc);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }

        doNvWrite(o, QcNvItemIds.NV_IMSI_MCC_I);
    }

    public String getImsi11_12() throws IOException {
        vLog("getImsi11_12()");

        NvImsi1112Type o = new QcNvItemTypes.NvImsi1112Type(doNvRead(QcNvItemIds.NV_IMSI_11_12_I));
        vLog(o.toDebugString());

        return PrimitiveParser.toUnsignedString(o.mImsi1112);
    }

    public void setImsi11_12(String imsi11_12) throws IOException, InvalidParameterException {
        vLog("setImsi11_12()");

        if (imsi11_12 == null || imsi11_12.length() != MinImsi.IMSI11_12_SIZE) {
            throw new InvalidParameterException();
        }

        NvImsi1112Type o = new NvImsi1112Type();
        o.mNam = 0;
        try {
            o.mImsi1112 = PrimitiveParser.parseUnsignedByte(imsi11_12);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }
        doNvWrite(o, QcNvItemIds.NV_IMSI_11_12_I);
    }

    public TrueImsi getTrueImsi() throws IOException {
        vLog("getTrueImsi()");
        String mcc = getTrueImsiMcc();
        String imsi11_12 = getTrueImsi11_12();
        String num = getTrueImsiNumber();
        String s1 = num.substring(0, TrueImsi.IMSI_S1_SIZE);
        String s2 = num.substring(num.length() - TrueImsi.IMSI_S2_SIZE, num.length());
        short imsiAddrNum = getTrueImsiAddrNum();
        return new TrueImsi(mcc, imsi11_12, s1, s2, imsiAddrNum);
    }

    public void setTrueImsi(TrueImsi trueImsi) throws IOException, InvalidParameterException {
        vLog("setTrueImsi()");
        if (trueImsi == null) {
            throw new InvalidParameterException();
        }
        setImsiMcc(trueImsi.getMcc());
        setImsi11_12(trueImsi.getImsi11_12());
        setMinImsiNumber(trueImsi.getImsiNumber());
        setTrueImsiAddrNum(trueImsi.getImsiAddrNum());
    }

    public String getTrueImsiNumber() throws IOException {
        vLog("getTrueImsiNumber()");

        NvMin1Type min1 = new QcNvItemTypes.NvMin1Type(doNvRead(QcNvItemIds.NV_IMSI_T_S1_I));
        NvMin2Type min2 = new QcNvItemTypes.NvMin2Type(doNvRead(QcNvItemIds.NV_IMSI_T_S2_I));

        return MinImsi.minToPhString(min1.mMin1[1], min2.mMin2[1]);
    }

    public void setTrueImsiNumber(String phNumber) throws IOException, InvalidParameterException {
        vLog("setTrueImsiNumber()");

        if (phNumber == null) {
            throw new InvalidParameterException();
        }
        NvMin1Type min1 = new QcNvItemTypes.NvMin1Type(doNvRead(QcNvItemIds.NV_IMSI_T_S1_I));
        NvMin2Type min2 = new QcNvItemTypes.NvMin2Type(doNvRead(QcNvItemIds.NV_IMSI_T_S2_I));

        min1.mMin1[1] = MinImsi.phStringToMin1(phNumber);
        min2.mMin2[1] = MinImsi.phStringToMin2(phNumber);

        doNvWrite(min1, QcNvItemIds.NV_IMSI_T_S1_I);
        doNvWrite(min2, QcNvItemIds.NV_IMSI_T_S2_I);
    }

    public String getTrueImsiMcc() throws IOException {
        vLog("getTrueImsiMcc()");

        NvImsiMccType o = new QcNvItemTypes.NvImsiMccType(doNvRead(QcNvItemIds.NV_IMSI_T_MCC_I));
        vLog(o.toDebugString());

        return PrimitiveParser.toUnsignedString(o.mImsiMcc);
    }

    public void setTrueImsiMcc(String imsiTMcc) throws IOException, InvalidParameterException {
        vLog("setTrueImsiMcc()");

        if (imsiTMcc == null || imsiTMcc.length() != TrueImsi.MCC_SIZE) {
            throw new InvalidParameterException();
        }

        NvImsiMccType o = new NvImsiMccType();
        o.mNam = 0;
        try {
            o.mImsiMcc = PrimitiveParser.parseUnsignedShort(imsiTMcc);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }

        doNvWrite(o, QcNvItemIds.NV_IMSI_T_MCC_I);
    }

    public String getTrueImsi11_12() throws IOException {
        vLog("getTrueImsi11_12()");

        NvImsi1112Type o = new QcNvItemTypes.NvImsi1112Type(doNvRead(QcNvItemIds.NV_IMSI_T_11_12_I));
        vLog(o.toDebugString());

        return PrimitiveParser.toUnsignedString(o.mImsi1112);
    }

    public void setTrueImsi11_12(String imsiT11_12) throws IOException, InvalidParameterException {
        vLog("setTrueImsi11_12()");

        if (imsiT11_12 == null || imsiT11_12.length() != TrueImsi.IMSI11_12_SIZE) {
            throw new InvalidParameterException();
        }

        NvImsi1112Type o = new NvImsi1112Type();
        o.mNam = 0;
        try {
            o.mImsi1112 = PrimitiveParser.parseUnsignedByte(imsiT11_12);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }

        doNvWrite(o, QcNvItemIds.NV_IMSI_T_11_12_I);
    }

    public short getTrueImsiAddrNum() throws IOException {
        vLog("getTrueImsiAddrNum()");

        NvImsiAddrNumType o = new QcNvItemTypes.NvImsiAddrNumType(
                doNvRead(QcNvItemIds.NV_IMSI_T_ADDR_NUM_I));
        vLog(o.toDebugString());

        return PrimitiveParser.toUnsigned(o.mNum);
    }

    public void setTrueImsiAddrNum(short imsiTAddrNum) throws IOException,
            InvalidParameterException {
        vLog("setTrueImsiAddrNum()");

        NvImsiAddrNumType o = new NvImsiAddrNumType();
        o.mNam = 0;
        try {
            o.mNum = PrimitiveParser.parseByte(imsiTAddrNum);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }

        doNvWrite(o, QcNvItemIds.NV_IMSI_T_ADDR_NUM_I);
    }

    public CdmaChannels getCdmaChannels() throws IOException {
        int[] primaryChannels = getPrimaryCdmaChannels();
        int[] secondaryChannels = getSecondaryCdmaChannels();
        return new CdmaChannels(primaryChannels[0], primaryChannels[1], secondaryChannels[0],
                secondaryChannels[1]);
    }

    public void setCdmaChannels(CdmaChannels cdmaChannels) throws IOException,
            InvalidParameterException {
        if (cdmaChannels == null) {
            throw new InvalidParameterException();
        }
        int[] primaryChannels = new int[2];
        primaryChannels[0] = cdmaChannels.getPrimaryChannelA();
        primaryChannels[1] = cdmaChannels.getPrimaryChannelB();
        setPrimaryCdmaChannels(primaryChannels);

        int[] secondaryChannels = new int[2];
        secondaryChannels[0] = cdmaChannels.getSecondaryChannelA();
        secondaryChannels[1] = cdmaChannels.getSecondaryChannelB();
        setSecondaryCdmaChannels(secondaryChannels);
    }

    public int[] getPrimaryCdmaChannels() throws IOException {
        vLog("getPrimaryCdmaChannels()");

        NvCdmaChType r = new NvCdmaChType(doNvRead(QcNvItemIds.NV_PCDMACH_I));
        vLog(r.toDebugString());

        int[] retVal = new int[2];
        retVal[0] = PrimitiveParser.toUnsigned(r.mChannelA);
        retVal[1] = PrimitiveParser.toUnsigned(r.mChannelB);

        return retVal;
    }

    public void setPrimaryCdmaChannels(int[] primaryChannels) throws IOException,
            InvalidParameterException {
        vLog("setPrimaryCdmaChannels()");

        if (primaryChannels == null || primaryChannels.length != 2) {
            throw new InvalidParameterException();
        }

        NvCdmaChType o = new NvCdmaChType();
        o.mNam = 0;
        try {
            o.mChannelA = PrimitiveParser.parseShort(primaryChannels[0]);
            o.mChannelB = PrimitiveParser.parseShort(primaryChannels[1]);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }

        doNvWrite(o, QcNvItemIds.NV_PCDMACH_I);
    }

    public int[] getSecondaryCdmaChannels() throws IOException {
        vLog("getSecondaryCdmaChannels()");

        NvCdmaChType r = new NvCdmaChType(doNvRead(QcNvItemIds.NV_SCDMACH_I));
        vLog(r.toDebugString());

        int[] retVal = new int[2];
        retVal[0] = PrimitiveParser.toUnsigned(r.mChannelA);
        retVal[1] = PrimitiveParser.toUnsigned(r.mChannelB);

        return retVal;
    }

    public void setSecondaryCdmaChannels(int[] secondaryChannels) throws IOException,
            InvalidParameterException {
        vLog("setSecondaryCdmaChannels()");

        if (secondaryChannels == null || secondaryChannels.length != 2) {
            throw new InvalidParameterException();
        }

        NvCdmaChType o = new NvCdmaChType();
        o.mNam = 0;
        try {
            o.mChannelA = PrimitiveParser.parseShort(secondaryChannels[0]);
            o.mChannelB = PrimitiveParser.parseShort(secondaryChannels[1]);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }

        doNvWrite(o, QcNvItemIds.NV_SCDMACH_I);
    }

    public short getMobCaiRev() throws IOException {
        vLog("getMobCaiRev()");

        NvByte o = new NvByte(doNvRead(QcNvItemIds.NV_MOB_CAI_REV_I));
        vLog(o.toDebugString());

        return PrimitiveParser.toUnsigned(o.mVal);
    }

    public void setMobCaiRev(short mobCaiRev) throws IOException, InvalidParameterException {
        vLog("setMobCaiRev()");

        NvByte o = new NvByte();
        try {
            o.mVal = PrimitiveParser.parseByte(mobCaiRev);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }
        doNvWrite(o, QcNvItemIds.NV_MOB_CAI_REV_I);
    }

    public short getRtreConfig() throws IOException {
        // NOT IMPLEMENTED IN NV
        return 0;
    }

    public void setRtreConfig(short rtreCfg) throws IOException, InvalidParameterException {
        // NOT IMPLEMENTED IN NV
    }

    // VOICE
    public VoiceConfig getVoiceConfig() throws IOException {
        vLog("getVoiceConfig()");
        AutoAnswer aa = getAutoAnswerStatus();
        TimerCount atc = getAirTimerCount();
        TimerCount rtc = getRoamTimerCount();
        short tty = getCurrentTtyMode();
        PreferredVoiceSo pvs = getPreferredVoiceSo();
        AmrStatus amr = getAmrStatus();
        short vp = getVoicePrivacyPref();
        return new VoiceConfig(aa, atc, rtc, tty, pvs, amr, vp);
    }

    public void setVoiceConfig(VoiceConfig voiceConfig) throws IOException,
            InvalidParameterException {
        vLog("setVoiceConfig()");
        if (voiceConfig == null) {
            throw new InvalidParameterException();
        }
        setAutoAnswerStatus(voiceConfig.getAutoAnswerStatus());
        setAirTimerCount(voiceConfig.getAirTimerCount());
        setRoamTimerCount(voiceConfig.getRoamTimerCount());
        setCurrentTtyMode(voiceConfig.getCurrentTtyMode());
        setPreferredVoiceSo(voiceConfig.getPreferredVoiceSo());
    }

    public AutoAnswer getAutoAnswerStatus() throws IOException {
        vLog("getAutoAnswerStatus()");

        NvAutoAnswerType mAutoAnswer = getAutoAnswer();

        return new AutoAnswer(mAutoAnswer.enable, mAutoAnswer.rings);
    }

    public void setAutoAnswerStatus(AutoAnswer autoAnswer) throws IOException,
            InvalidParameterException {
        vLog("setAutoAnswerStatus()");
        if (autoAnswer == null) {
            throw new InvalidParameterException();
        }
        NvAutoAnswerType mAutoAnswer = new NvAutoAnswerType();
        mAutoAnswer.enable = autoAnswer.isEnabled();
        mAutoAnswer.rings = PrimitiveParser.parseByte(autoAnswer.getRings());
        setAutoAnswer(mAutoAnswer);
    }

    public void disableAutoAnswer() throws IOException {
        vLog("disableAutoAnswer()");

        NvAutoAnswerType mAutoAnswer = new NvAutoAnswerType();
        mAutoAnswer.enable = false;
        mAutoAnswer.rings = 0;
        setAutoAnswer(mAutoAnswer);
    }

    public void enableAutoAnswer(short rings) throws IOException, InvalidParameterException {
        vLog("enableAutoAnswer()");

        NvAutoAnswerType mAutoAnswer = new NvAutoAnswerType();
        mAutoAnswer.enable = true;
        mAutoAnswer.rings = PrimitiveParser.parseByte(rings);
        setAutoAnswer(mAutoAnswer);
    }

    public void enableAutoAnswer() throws IOException {
        enableAutoAnswer(AutoAnswer.DEFAULT_RINGS);
    }

    public TimerCount getAirTimerCount() throws IOException {
        vLog("getAirTimerCount()");

        NvCallCntType o = new NvCallCntType(doNvRead(QcNvItemIds.NV_AIR_CNT_I));
        vLog(o.toDebugString());

        return new TimerCount(PrimitiveParser.toUnsigned(o.mCount));
    }

    public void setAirTimerCount(TimerCount timerCnt) throws IOException, InvalidParameterException {
        vLog("setAirTimerCount()");

        if (timerCnt == null) {
            throw new InvalidParameterException();
        }

        NvCallCntType o = new NvCallCntType();
        try {
            o.mCount = PrimitiveParser.parseInt(timerCnt.getTimerCount());
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }

        doNvWrite(o, QcNvItemIds.NV_AIR_CNT_I);
    }

    public TimerCount getRoamTimerCount() throws IOException {
        vLog("getRoamTimerCount()");

        NvCallCntType o = new NvCallCntType(doNvRead(QcNvItemIds.NV_ROAM_CNT_I));
        vLog(o.toDebugString());

        return new TimerCount(PrimitiveParser.toUnsigned(o.mCount));
    }

    public void setRoamTimerCount(TimerCount timerCnt) throws IOException,
            InvalidParameterException {
        vLog("setRoamCount()");

        if (timerCnt == null) {
            throw new InvalidParameterException();
        }

        NvCallCntType o = new NvCallCntType();
        try {
            o.mCount = PrimitiveParser.parseInt(timerCnt.getTimerCount());
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }

        doNvWrite(o, QcNvItemIds.NV_ROAM_CNT_I);
    }

    public short getCurrentTtyMode() throws IOException {
        // NOT IMPLEMENTED BY NV
        return 0;
    }

    public void setCurrentTtyMode(short ttyMode) throws IOException, InvalidParameterException {
        // NOT IMPLEMENTED BY NV
    }

    public PreferredVoiceSo getPreferredVoiceSo() throws IOException {
        vLog("getPreferredVoiceSo()");

        NvPrefVoiceSoType r = new NvPrefVoiceSoType(doNvRead(QcNvItemIds.NV_PREF_VOICE_SO_I));
        vLog(r.toDebugString());

        short evrc = r.mEvrcCapabilityEnabled ? (short) 1 : (short) 0;
        int hpvs = PrimitiveParser.toUnsigned(r.mHomePageVoiceSo);
        int hovs = PrimitiveParser.toUnsigned(r.mHomeOrigVoiceSo);
        int rovs = PrimitiveParser.toUnsigned(r.mRoamOrigVoiceSo);

        return new PreferredVoiceSo(evrc, hpvs, hovs, rovs);
    }

    public void setPreferredVoiceSo(PreferredVoiceSo prefVoiceSo) throws IOException,
            InvalidParameterException {
        vLog("setPreferredVoiceSo()");

        if (prefVoiceSo == null) {
            throw new InvalidParameterException();
        }
        NvPrefVoiceSoType o = new NvPrefVoiceSoType();
        o.mEvrcCapabilityEnabled = (prefVoiceSo.getEvrcCapability() == 0) ? false : true;
        try {
            o.mHomePageVoiceSo = PrimitiveParser.parseShort(prefVoiceSo.getHomePageVoiceSo());
            o.mHomeOrigVoiceSo = PrimitiveParser.parseShort(prefVoiceSo.getHomeOrigVoiceSo());
            o.mRoamOrigVoiceSo = PrimitiveParser.parseShort(prefVoiceSo.getRoamOrigVoiceSo());
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }

        doNvWrite(o, QcNvItemIds.NV_PREF_VOICE_SO_I);
    }

    public AmrStatus getAmrStatus() throws IOException {
        // NOT IMPLEMENTED BY NV
        return new AmrStatus();
    }

    public short getVoicePrivacyPref() throws IOException {
        // NOT IMPLEMENTED BY NV
        return 0;
    }

    // DMS
    public String getFtmMode() throws IOException {
        // TODO Auto-generated method stub
        return null;
    }

    public String getSwVersion() throws IOException {
        // TODO Auto-generated method stub
        return null;
    }

    public void updateSpCode() throws IOException {
        // TODO Auto-generated method stub
    }

    public String[] getDeviceSerials() throws IOException {
        // TODO Auto-generated method stub
        return null;
    }

    public Boolean getSpcChangeEnabled() throws IOException {
        vLog("getSpcChangeEnabled()");

        NvByte o = new NvByte(doNvRead(QcNvItemIds.NV_SPC_CHANGE_ENABLED_I));
        vLog(o.toDebugString());

        return o.mVal == 1 ? true : false;
    }

    public void setSpcChangeEnabled(Boolean spcChangeEnabled) throws IOException,
            InvalidParameterException {
        vLog("setSpcChangeEnabled()");

        NvByte o = new NvByte();
        o.mVal = (byte) (spcChangeEnabled == true ? 1 : 0);

        doNvWrite(o, QcNvItemIds.NV_SPC_CHANGE_ENABLED_I);
    }

    // WDS
    public String getDefaultBaudRate() throws IOException {
        vLog("getDefaultBaudRate()");

        NvShort o = new NvShort(doNvRead(QcNvItemIds.NV_DS_DEFAULT_BAUDRATE_I));
        vLog(o.toDebugString());

        return PrimitiveParser.toUnsignedString(o.mVal);
    }

    public void setDefaultBaudRate(String defaultBaudrate) throws IOException,
            InvalidParameterException {
        vLog("setDefaultBaudRate()");

        if (defaultBaudrate == null) {
            throw new InvalidParameterException();
        }

        NvShort o = new NvShort();
        try {
            o.mVal = PrimitiveParser.parseUnsignedShort(defaultBaudrate);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }
        doNvWrite(o, QcNvItemIds.NV_DS_DEFAULT_BAUDRATE_I);
    }

    // Miscellaneous
    public String getEmailGateway() throws IOException {
        // TODO Auto-generated method stub
        return null;
    }

    public void setEmailGateway(String gateway) throws IOException, InvalidParameterException {
        // TODO Auto-generated method stub
    }

    public String[] getEccList() throws IOException {
        vLog("getEccList()");

        EccListType r = new EccListType(doNvRead(QcNvItemIds.NV_ECC_LIST_I));
        vLog(r.toDebugString());

        return r.getEcclist();
    }

    public void setEccList(String[] eccList) throws IOException, InvalidParameterException {
        vLog("setEccList()");

        if (eccList == null) {
            throw new InvalidParameterException();
        }
        EccListType o = new EccListType();
        o.setEccList(eccList);

        doNvWrite(o, QcNvItemIds.NV_ECC_LIST_I);
    }

    public String getSecCode() throws IOException {
        vLog("getSecCode()");

        NvSecCodeType o = new NvSecCodeType(doNvRead(QcNvItemIds.NV_SEC_CODE_I));
        vLog(o.toDebugString());

        return o.getSecCode();
    }

    public void setSecCode(String securityCode) throws IOException, InvalidParameterException {
        vLog("setSecCode()");

        if (securityCode == null) {
            throw new InvalidParameterException();
        }
        NvSecCodeType o = new NvSecCodeType();
        o.setSecCode(securityCode);

        doNvWrite(o, QcNvItemIds.NV_SEC_CODE_I);
    }

    public String getLockCode() throws IOException {
        vLog("getLockCode()");

        NvLockCodeType o = new NvLockCodeType(doNvRead(QcNvItemIds.NV_LOCK_CODE_I));
        vLog(o.toDebugString());

        return o.getLockCode();
    }

    public void setLockCode(String lockCode) throws IOException, InvalidParameterException {
        vLog("setLockCode()");

        if (lockCode == null) {
            throw new InvalidParameterException();
        }
        NvLockCodeType o = new NvLockCodeType();
        o.setLockCode(lockCode);

        doNvWrite(o, QcNvItemIds.NV_LOCK_CODE_I);
    }

    public String getGpsOnePdeAddress() throws IOException {
        vLog("getGpsOnePdeAddress()");

        NvInteger o = new NvInteger(doNvRead(QcNvItemIds.NV_GPS1_PDE_ADDRESS_I));
        vLog(o.toDebugString());

        return intToIpAddress(o.mVal);
    }

    public void setGpsOnePdeAddress(String gpsOnePdeAddress) throws IOException,
            InvalidParameterException {
        vLog("setGpsOnePdeAddress()");

        if (gpsOnePdeAddress == null) {
            throw new InvalidParameterException();
        }

        NvInteger o = new NvInteger();
        try {
            o.mVal = ipAddressStringToInt(gpsOnePdeAddress);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }

        doNvWrite(o, QcNvItemIds.NV_GPS1_PDE_ADDRESS_I);
    }

    public String getGpsOnePdePort() throws IOException {
        vLog("getGpsOnePdePort()");

        NvInteger o = new NvInteger(doNvRead(QcNvItemIds.NV_GPS1_PDE_PORT_I));
        vLog(o.toDebugString());

        return PrimitiveParser.toUnsignedString(o.mVal);
    }

    public void setGpsOnePdePort(String gpsOnePdePort) throws IOException,
            InvalidParameterException {
        vLog("setGpsOnePdePort()");

        if (gpsOnePdePort == null) {
            throw new InvalidParameterException();
        }

        NvInteger o = new NvInteger();
        try {
            o.mVal = PrimitiveParser.parseUnsignedInt(gpsOnePdePort);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }
        doNvWrite(o, QcNvItemIds.NV_GPS1_PDE_PORT_I);
    }

    // NV Helper Functions
    private NvAutoAnswerType getAutoAnswer() throws IOException {

        AsyncResult result;

        NvAutoAnswerType mAutoAnswer = new NvAutoAnswerType();

        result = mQcRilOemHook.sendQcRilHookMsg(IQcRilHook.QCRILHOOK_NV_READ,
                QcNvItemIds.NV_AUTO_ANSWER_I);

        if ((result.exception == null) && (result.result != null)
                && ((byte[]) (result.result)).length >= NvAutoAnswerType.getSize()) {
            byte[] responseData = (byte[]) result.result;

            mAutoAnswer.enable = (responseData[0] == 0) ? false : true;
            mAutoAnswer.rings = responseData[1];
        } else {
            Log.w(LOG_TAG, "Unable to read Auto Answer Value from NV Memory");
            throw new IOException();
        }

        return mAutoAnswer;
    }

    private void setAutoAnswer(NvAutoAnswerType mAutoAnswer) throws IOException {
        byte[] requestData = new byte[HEADER_SIZE + NvAutoAnswerType.getSize()];
        ByteBuffer buf = QcRilHook.createBufferWithNativeByteOrder(requestData);

        buf.putInt(QcNvItemIds.NV_AUTO_ANSWER_I); // ItemID
        buf.putInt(NvAutoAnswerType.getSize()); // ItemSize

        // NV Item
        buf.put((byte) (mAutoAnswer.enable ? 1 : 0));
        buf.put(mAutoAnswer.rings);

        AsyncResult result = mQcRilOemHook.sendQcRilHookMsg(IQcRilHook.QCRILHOOK_NV_WRITE,
                requestData);

        if (result.exception != null) {
            Log.w(LOG_TAG, "Unable to Set Auto Answer");
            throw new IOException();
        }
    }

    public static int ipAddressStringToInt(String ipAddress) throws InvalidParameterException {

        String add[] = ipAddress.split("\\.");
        if (add.length != 4) {
            throw new InvalidParameterException("Incorrectly formatted IP Address.");
        }

        int ip = 0;
        for (int i = 0; i < 4; i++) {
            int t = Integer.parseInt(add[i]);
            if ((t & 0xFFFFFF00) != 0) {
                throw new InvalidParameterException("Incorrectly formatted IP Address.");
            }
            ip = ((ip << 8) + (t & 0xFF));
            vLog("t=" + (t & 0xFF));
            vLog("ip=" + ip);
        }

        return ip;
    }

    public static String intToIpAddress(int ip) {
        String ipAddr = "";
        ipAddr += ((ip >> 24) & 0xFF) + ".";
        ipAddr += ((ip >> 16) & 0xFF) + ".";
        ipAddr += ((ip >> 8) & 0xFF) + ".";
        ipAddr += ((ip) & 0xFF);
        return ipAddr;
    }

    // NV Private
    public class SidNidPair {
        String sid, nid;

        public SidNidPair() {

        }

        public SidNidPair(String sid, String nid) {
            this.sid = sid;
            this.nid = nid;
        }

        public String getSid() {
            return sid;
        }

        public void setSid(String sid) {
            this.sid = sid;
        }

        public String getNid() {
            return nid;
        }

        public void setNid(String nid) {
            this.nid = nid;
        }
    }

    public SidNidPair[][] getSidNid() throws IOException {
        vLog("getSidNid()");

        NvSidNidType o = new NvSidNidType(doNvRead(QcNvItemIds.NV_SID_NID_I));
        vLog(o.toDebugString());

        SidNidPair[][] retVal = new SidNidPair[IQcNvItems.NV_MAX_MINS][IQcNvItems.NV_MAX_SID_NID];
        for (int i = 0; i < IQcNvItems.NV_MAX_MINS; i++) {
            for (int j = 0; j < IQcNvItems.NV_MAX_SID_NID; j++) {
                retVal[i][j] = new SidNidPair();
                retVal[i][j].setSid(PrimitiveParser.toUnsignedString(o.mPair[i][j].mSid));
                retVal[i][j].setNid(PrimitiveParser.toUnsignedString(o.mPair[i][j].mNid));
            }
        }

        return retVal;
    }

    public void setSidNid(SidNidPair[][] sn) throws IOException, InvalidParameterException {
        vLog("setSidNid()");

        if (sn == null) {
            throw new InvalidParameterException();
        }
        NvSidNidType o = new NvSidNidType();

        for (int i = 0; i < IQcNvItems.NV_MAX_MINS; i++) {
            for (int j = 0; j < IQcNvItems.NV_MAX_SID_NID; j++) {
                short sid;
                short nid;
                try {
                    sid = PrimitiveParser.parseUnsignedShort(sn[i][j].getSid());
                    nid = PrimitiveParser.parseUnsignedShort(sn[i][j].getNid());
                } catch (NumberFormatException e) {
                    throw new InvalidParameterException(e.toString());
                }
                if ((sid & 0x8000) != 0) {
                    throw new InvalidParameterException("Parameter out of range : 0<=sid<=32767");
                }
                o.mPair[i][j] = new QcNvItemTypes.NvSidNidPairType();
                o.mPair[i][j].mSid = sid;
                o.mPair[i][j].mNid = nid;
            }
        }
        doNvWrite(o, QcNvItemIds.NV_SID_NID_I);
    }

    public String[] getImsiMin1() throws IOException {
        vLog("getImsiMin1()");

        NvMin1Type o = new QcNvItemTypes.NvMin1Type(doNvRead(QcNvItemIds.NV_MIN1_I));
        vLog(o.toDebugString());

        String retVal[] = new String[IQcNvItems.NV_MAX_MINS];
        retVal[0] = PrimitiveParser.toUnsignedString(o.mMin1[0]);
        retVal[1] = PrimitiveParser.toUnsignedString(o.mMin1[1]);

        return retVal;
    }

    public void setImsiMin1(String[] minString) throws IOException, InvalidParameterException {
        vLog("setImsiMin1()");

        if (minString == null || minString.length != IQcNvItems.NV_MAX_MINS) {
            throw new InvalidParameterException();
        }

        NvMin1Type min;
        min = new NvMin1Type();
        try {
            min.mNam = 0;
            min.mMin1[0] = PrimitiveParser.parseUnsignedInt(minString[0]);
            min.mMin1[1] = PrimitiveParser.parseUnsignedInt(minString[1]);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }
        doNvWrite(min, QcNvItemIds.NV_MIN1_I);
    }

    public String[] getImsiMin2() throws IOException {
        vLog("getImsiMin2()");

        NvMin2Type o = new QcNvItemTypes.NvMin2Type(doNvRead(QcNvItemIds.NV_MIN2_I));
        vLog(o.toDebugString());

        String retVal[] = new String[IQcNvItems.NV_MAX_MINS];
        retVal[0] = PrimitiveParser.toUnsignedString(o.mMin2[0]);
        retVal[1] = PrimitiveParser.toUnsignedString(o.mMin2[1]);

        return retVal;
    }

    public void setImsiMin2(String[] minString) throws IOException, InvalidParameterException {
        vLog("setImsiMin2()");

        if (minString == null || minString.length != IQcNvItems.NV_MAX_MINS) {
            throw new InvalidParameterException();
        }

        NvMin2Type min;
        min = new NvMin2Type();
        try {
            min.mNam = 0;
            min.mMin2[0] = PrimitiveParser.parseUnsignedShort(minString[0]);
            min.mMin2[1] = PrimitiveParser.parseUnsignedShort(minString[1]);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }
        doNvWrite(min, QcNvItemIds.NV_MIN2_I);
    }

    public String[] getTrueImsiS1() throws IOException {
        vLog("getTrueImsiS1()");

        NvMin1Type o = new QcNvItemTypes.NvMin1Type(doNvRead(QcNvItemIds.NV_IMSI_T_S1_I));
        vLog(o.toDebugString());

        String retVal[] = new String[IQcNvItems.NV_MAX_MINS];
        retVal[0] = PrimitiveParser.toUnsignedString(o.mMin1[0]);
        retVal[1] = PrimitiveParser.toUnsignedString(o.mMin1[1]);

        return retVal;
    }

    public void setTrueImsiS1(String[] imsiTS1) throws IOException, InvalidParameterException {
        vLog("setTrueImsiS1()");

        if (imsiTS1 == null || imsiTS1.length != IQcNvItems.NV_MAX_MINS) {
            throw new InvalidParameterException();
        }

        NvMin1Type min;
        min = new NvMin1Type();
        try {
            min.mNam = 0;
            min.mMin1[0] = PrimitiveParser.parseUnsignedInt(imsiTS1[0]);
            min.mMin1[1] = PrimitiveParser.parseUnsignedInt(imsiTS1[1]);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }
        doNvWrite(min, QcNvItemIds.NV_IMSI_T_S1_I);
    }

    public String[] getTrueImsiS2() throws IOException {
        vLog("getTrueImsiS2()");

        NvMin2Type o = new QcNvItemTypes.NvMin2Type(doNvRead(QcNvItemIds.NV_IMSI_T_S2_I));
        vLog(o.toDebugString());

        String retVal[] = new String[IQcNvItems.NV_MAX_MINS];
        retVal[0] = PrimitiveParser.toUnsignedString(o.mMin2[0]);
        retVal[1] = PrimitiveParser.toUnsignedString(o.mMin2[1]);

        return retVal;
    }

    public void setTrueImsiS2(String[] imsiTS2) throws IOException, InvalidParameterException {
        vLog("setTrueImsiS2()");

        if (imsiTS2 == null || imsiTS2.length != IQcNvItems.NV_MAX_MINS) {
            throw new InvalidParameterException();
        }

        NvMin2Type min;
        min = new NvMin2Type();
        try {
            min.mNam = 0;
            min.mMin2[0] = PrimitiveParser.parseUnsignedShort(imsiTS2[0]);
            min.mMin2[1] = PrimitiveParser.parseUnsignedShort(imsiTS2[1]);
        } catch (NumberFormatException e) {
            throw new InvalidParameterException(e.toString());
        }
        doNvWrite(min, QcNvItemIds.NV_IMSI_T_S2_I);
    }
}
