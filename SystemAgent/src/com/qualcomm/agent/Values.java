/**
 * Copyright (c) 2012-2013, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 * Developed by QRD Engineering team.
 */
package com.qualcomm.agent;

public class Values {
    public static String ACTION_AGENT = "android.system.agent";
    public static String ACTION_FULL_AGENT = "android.system.fullagent";
    public static String ACTION_PHONEPROCESS_AGENT = "android.phoneprocess.agent";
    public static String SET_SYSTEM_PROPERTIES = "setSystemProperties";
    public static String GET_SYSTEM_PROPERTIES = "getSystemProperties";
    public static String WRITE_SYSTEM_FILES = "writeSystemFiles";
    public static String TAKE_SCREENSHOT = "takeLogs";
    public static String SET_SCREENSHOT_PATH = "setScreeshotPath";
    public static String GET_DEVICE_ID = "getDeviceId";
    public static String REBOOT = "reboot";
    public static int SCREENSHOT_NOTIFICATION_ID = 120508;
    public static String AGENT_RESULT_PROP = "persist.debug.agentres";
    public static String META_ID_PROPERTY = "debug.agent.metainfo";
    public static String META_INFO_FILE = "/firmware/verinfo/ver_info.txt";
    public static String AGENT_RESPONSE_ACTION = "qualcomm.intent.action.AGENT_RESPONSE";
}
