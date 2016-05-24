/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/
#define LOG_NDDEBUG 0
#define LOG_TAG "LocSvc_LBSApiV02"
#include <LBSApiV02.h>
#include <log_util.h>
#include <location_service_v02.h>
#include <loc_api_sync_req.h>
#include <IzatApiV02.h>
#include <LocApiProxy.h>

#define LAT_SCALE_FACTOR 23860929.4222
#define LONG_SCALE_FACTOR 11930464.7111
#define LAT_SCALE_DOWN(x) ((double)(x / LAT_SCALE_FACTOR))
#define LONG_SCALE_DOWN(x) ((double)(x / LONG_SCALE_FACTOR))


using namespace loc_core;
using namespace izat_core;

namespace lbs_core {

LBSApiV02 :: LBSApiV02(const MsgTask* msgTask,
                       LOC_API_ADAPTER_EVENT_MASK_T exMask):
    LocApiV02(msgTask, exMask),
    LBSApiBase(new LocApiProxyV02(this))
{
    LOC_LOGD("%s:%d]: LBSApiV02 created. lbsapi: %p; locApiProxy:%p\n",
             __func__, __LINE__, this, mLocApiProxy);
}

LBSApiV02 :: ~LBSApiV02()
{
    delete mLocApiProxy;
}

void LBSApiV02::eventCb(locClientHandleType clientHandle,
                         uint32_t eventId,
                         locClientEventIndUnionType eventPayload)
{
    switch(eventId) {
    case  QMI_LOC_EVENT_WIFI_REQ_IND_V02:
        WpsEvent(eventPayload.pWifiReqEvent);
        break;
    case QMI_LOC_EVENT_INJECT_WIFI_AP_DATA_REQ_IND_V02:
        requestWifiApData();
        break;
    default:
        //Check with IzatApiV02 if the event can be handled
        if(!((LocApiProxyV02 *)mLocApiProxy)->eventCb(clientHandle, eventId, eventPayload))
            //Event handled in IzatApiV02. Break.
            break;
        //Check with LocApiV02 if event can be handled
        LocApiV02::eventCb(clientHandle, eventId, eventPayload);
    }
}

void LBSApiV02::WpsEvent(const qmiLocEventWifiReqIndMsgT_v02* request)
{
    enum WifiRequestType type;

    switch (request->requestType) {
    case eQMI_LOC_WIFI_START_PERIODIC_HI_FREQ_FIXES_V02:
        type = HIGH;
        break;
    case eQMI_LOC_WIFI_START_PERIODIC_KEEP_WARM_V02:
        type = LOW;
        break;
    case eQMI_LOC_WIFI_STOP_PERIODIC_FIXES_V02:
        type = STOP;
        break;
    default:
        type = UNKNOWN;
    }

    requestWps(type);
}

int LBSApiV02::wifiStatusInform()
{
    qmiLocNotifyWifiStatusReqMsgT_v02 wifiStatusReq;
    int status = 1;
    memset(&wifiStatusReq, 0, sizeof(wifiStatusReq));
    wifiStatusReq.wifiStatus = (qmiLocWifiStatusEnumT_v02)status;

    LOC_LOGV("%s:%d] informing wifi status ...\n", __func__, __LINE__);
    LOC_SEND_SYNC_REQ(NotifyWifiStatus,
                      NOTIFY_WIFI_STATUS,
                      wifiStatusReq,  LocApiV02::clientHandle);

    EXIT_LOG(%d, rv);
    return rv;
}

int LBSApiV02::injectCoarsePosition(CoarsePositionInfo cpInfo)
{
    qmiLocInjectPositionReqMsgT_v02 injectPositionReq;
    memset(&injectPositionReq, 0, sizeof(injectPositionReq));

    injectPositionReq.latitude_valid = cpInfo.latitudeValid;
    injectPositionReq.latitude = cpInfo.latitude;

    injectPositionReq.longitude_valid = cpInfo.longitudeValid;
    injectPositionReq.longitude = cpInfo.longitude;

    if(cpInfo.horUncCircular < 1000)
    {
        cpInfo.horUncCircular = 1000;
    }

    injectPositionReq.horUncCircular_valid = cpInfo.horUncCircularValid;
    injectPositionReq.horUncCircular = cpInfo.horUncCircular;

    injectPositionReq.horConfidence_valid = cpInfo.horConfidenceValid;
    injectPositionReq.horConfidence = cpInfo.horConfidence;

    injectPositionReq.horReliability_valid = cpInfo.horReliabilityValid;
    injectPositionReq.horReliability = mapReliabilityValue(cpInfo.horReliability);

    injectPositionReq.altitudeWrtEllipsoid_valid = cpInfo.altitudeWrtEllipsoidValid;
    injectPositionReq.altitudeWrtEllipsoid = cpInfo.altitudeWrtEllipsoid;

    injectPositionReq.altitudeWrtMeanSeaLevel_valid = cpInfo.altitudeWrtMeanSeaLevelValid;
    injectPositionReq.altitudeWrtMeanSeaLevel = cpInfo.altitudeWrtMeanSeaLevel;

    injectPositionReq.vertUnc_valid = cpInfo.vertUncValid;
    injectPositionReq.vertUnc = cpInfo.vertUnc;

    injectPositionReq.vertConfidence_valid = cpInfo.vertConfidenceValid;
    injectPositionReq.vertConfidence = cpInfo.vertConfidence;

    injectPositionReq.vertReliability_valid = cpInfo.vertReliabilityValid;
    injectPositionReq.vertReliability = mapReliabilityValue(cpInfo.vertReliability);

    injectPositionReq.timestampUtc_valid = cpInfo.timestampUtcValid;
    injectPositionReq.timestampUtc = cpInfo.timestampUtc;

    injectPositionReq.timestampAge_valid = cpInfo.timestampAgeValid;
    injectPositionReq.timestampAge = cpInfo.timestampAge;

    injectPositionReq.positionSrc_valid = cpInfo.positionSrcValid;
    injectPositionReq.positionSrc = mapSourceType(cpInfo.positionSrc);

    /* Log */
    LOC_LOGD("%s:%d]: Lat=%lf, Lon=%lf, Acc=%.2lf\n", __func__, __LINE__,
             injectPositionReq.latitude, injectPositionReq.longitude,
             injectPositionReq.horUncCircular);

    LOC_SEND_SYNC_REQ(InjectPosition,
                      INJECT_POSITION,
                      injectPositionReq, LocApiV02::clientHandle);

    EXIT_LOG(%d, rv);
    return rv;
}

int LBSApiV02::injectWifiPosition(WifiLocation wifiInfo)
{
    qmiLocInjectWifiPositionReqMsgT_v02 injectWifiPositionReq;
    memset(&injectWifiPositionReq, 0, sizeof(injectWifiPositionReq));

    injectWifiPositionReq.wifiFixTime_valid = 1;
    injectWifiPositionReq.wifiFixTime.wifiPositionTime= 0xFFFFFFFF;

    injectWifiPositionReq.wifiFixPosition_valid = wifiInfo.positionValid;
    injectWifiPositionReq.wifiFixPosition.lat = wifiInfo.latitude;
    injectWifiPositionReq.wifiFixPosition.lon = wifiInfo.longitude;

    if(wifiInfo.accuracy < 1000)
    {
        wifiInfo.accuracy = 1000;
    }

    injectWifiPositionReq.wifiFixPosition.hepe =(uint16_t)wifiInfo.accuracy;
    injectWifiPositionReq.wifiFixPosition.numApsUsed =(uint8_t)wifiInfo.numApsUsed;

    if(wifiInfo.positionValid)
    {
        injectWifiPositionReq.wifiFixPosition.fixErrorCode =
            (qmiLocWifiFixErrorCodeEnumT_v02)wifiInfo.fixError;

        LOC_LOGD("%s:%d] Calling injectPosition", __func__, __LINE__);
        injectPosition(LAT_SCALE_DOWN(wifiInfo.latitude), LONG_SCALE_DOWN(wifiInfo.longitude), wifiInfo.accuracy);
    }
    else
    {
        injectWifiPositionReq.wifiFixPosition.fixErrorCode =
            eQMI_LOC_WIFI_FIX_ERROR_LOCATION_CANNOT_BE_DETERMINED_V02;
    }

    LOC_LOGV("%s:%d] wifiInfo.apInfoValid = %d,wifiInfo.numApsUsed = %d ...\n", __func__, __LINE__,
             wifiInfo.apInfoValid,wifiInfo.numApsUsed);

    if(wifiInfo.apInfoValid){
        injectWifiPositionReq.apInfo_valid = 1;
        injectWifiPositionReq.apInfo_len = wifiInfo.apInfo.apLen;
        LOC_LOGV("%s:%d] wifiInfo.apInfo.apLen = %d, ...\n", __func__, __LINE__,wifiInfo.apInfo.apLen);

        int k = 0;
        for(int i=0;i<MAX_REPORTED_APS;i++)
        {
            for(int j=0;j<MAC_ADDRESS_LENGTH;j++,k++)
                injectWifiPositionReq.apInfo[i].macAddr[j] = wifiInfo.apInfo.mac_address[k];

            injectWifiPositionReq.apInfo[i].rssi = wifiInfo.apInfo.rssi[i];
            injectWifiPositionReq.apInfo[i].channel = wifiInfo.apInfo.channel[i];
            injectWifiPositionReq.apInfo[i].apQualifier = 0;
            LOC_LOGV("%s:%d] mac address %d is  %X:%X:%X:%X:%X:%X rssi[%d] = %d and channel[%d] = %d ...\n"
                     , __func__, __LINE__,i,
                     injectWifiPositionReq.apInfo[i].macAddr[0],
                     injectWifiPositionReq.apInfo[i].macAddr[1],
                     injectWifiPositionReq.apInfo[i].macAddr[2],
                     injectWifiPositionReq.apInfo[i].macAddr[3],
                     injectWifiPositionReq.apInfo[i].macAddr[4],
                     injectWifiPositionReq.apInfo[i].macAddr[5],
                     i, injectWifiPositionReq.apInfo[i].rssi,
                     i, injectWifiPositionReq.apInfo[i].channel);
        }
    }

    LOC_LOGV("%s:%d] injecting wifi position ...\n", __func__, __LINE__);
    LOC_SEND_SYNC_REQ(InjectWifiPosition,
                      INJECT_WIFI_POSITION,
                      injectWifiPositionReq, LocApiV02::clientHandle);

    EXIT_LOG(%d, rv);
    return rv;
}

/* set the Wifi timeout value */
int LBSApiV02::setWifiWaitTimeoutValue(uint8_t timeout)
{

    qmiLocSetProtocolConfigParametersReqMsgT_v02 suplWifiConfigReq;
    memset(&suplWifiConfigReq, 0, sizeof(suplWifiConfigReq));

    LOC_LOGD("%s:%d]: WIFI timeout value = %d\n",  __func__, __LINE__, timeout);

    suplWifiConfigReq.wifiScanInjectTimeout_valid = 1;
    suplWifiConfigReq.wifiScanInjectTimeout = timeout;

    LOC_LOGV("%s:%d] injecting the wifi timeout value ...\n", __func__, __LINE__);
    LOC_SEND_SYNC_REQ(SetProtocolConfigParameters,
                      SET_PROTOCOL_CONFIG_PARAMETERS,
                      suplWifiConfigReq, LocApiV02::clientHandle);

    EXIT_LOG(%d, rv);
    return rv;
}

int LBSApiV02::injectWifiApInfo(WifiApInfo wifiApInfo)
{
    qmiLocInjectWifiApDataReqMsgT_v02 injectWifiApDataReq;
    memset(&injectWifiApDataReq, 0, sizeof(injectWifiApDataReq));

    injectWifiApDataReq.wifiApInfo_len = wifiApInfo.apLen;
    LOC_LOGV("%s:%d] injectWifiApDataReq.wifiApInfo_len = %d, ...\n", __func__, __LINE__,
             injectWifiApDataReq.wifiApInfo_len);

    int k = 0;
    for(uint32_t i=0;i<MAX_REPORTED_APS;i++)
    {
        for(int j=0;j<MAC_ADDRESS_LENGTH;j++,k++)
            injectWifiApDataReq.wifiApInfo[i].macAddress[j] = wifiApInfo.mac_address[k];

        injectWifiApDataReq.wifiApInfo[i].apRssi = wifiApInfo.rssi[i];
        injectWifiApDataReq.wifiApInfo[i].apChannel = (uint16_t)wifiApInfo.channel[i];
        injectWifiApDataReq.wifiApInfo[i].wifiApDataMask =
            WIFI_APDATA_MASK_AP_RSSI | WIFI_APDATA_MASK_AP_CHANNEL;

        LOC_LOGV("%s:%d] mac address %d is  %X:%X:%X:%X:%X:%X rssi[%d] = %d and channel[%d] = %d\n"
                 , __func__, __LINE__,i,
                 injectWifiApDataReq.wifiApInfo[i].macAddress[0],
                 injectWifiApDataReq.wifiApInfo[i].macAddress[1],
                 injectWifiApDataReq.wifiApInfo[i].macAddress[2],
                 injectWifiApDataReq.wifiApInfo[i].macAddress[3],
                 injectWifiApDataReq.wifiApInfo[i].macAddress[4],
                 injectWifiApDataReq.wifiApInfo[i].macAddress[5],
                 i, injectWifiApDataReq.wifiApInfo[i].apRssi,
                 i, injectWifiApDataReq.wifiApInfo[i].apChannel);
    }

    LOC_LOGV("%s:%d] injecting wifi ap info ...\n", __func__, __LINE__);
    LOC_SEND_SYNC_REQ(InjectWifiApData,
                      INJECT_WIFI_AP_DATA,
                      injectWifiApDataReq, LocApiV02::clientHandle);

    EXIT_LOG(%d, rv);
    return rv;
}


int LBSApiV02::cinfoInject (int cid, int lac, int mnc, int mcc, bool roaming)
{
    qmiLocInjectGSMCellInfoReqMsgT_v02 cinfoReq;
    cinfoReq.gsmCellId.MCC = mcc;
    cinfoReq.gsmCellId.MNC = mnc;
    cinfoReq.gsmCellId.LAC = lac;
    cinfoReq.gsmCellId.CID = cid;
    cinfoReq.roamingStatus = roaming;

    LOC_SEND_SYNC_REQ(InjectGSMCellInfo, INJECT_GSM_CELL_INFO,
                      cinfoReq, LocApiV02::clientHandle);

    EXIT_LOG(%d, rv);
    return rv;
}

int LBSApiV02::oosInform()
{
    void* oosReq = NULL;

    LOC_SEND_SYNC_REQ(WWANOutOfServiceNotification,
                      WWAN_OUT_OF_SERVICE_NOTIFICATION,
                      oosReq, LocApiV02::clientHandle);

    EXIT_LOG(%d, rv);
    return rv;
}

int LBSApiV02::niSuplInit(char* supl_init, int length)
{
    qmiLocInjectNetworkInitiatedMessageReqMsgT_v02 suplInitReq;
    memcpy(suplInitReq.injectedNIMessage, supl_init, length);
    suplInitReq.injectedNIMessage_len = length;
    suplInitReq.injectedNIMessageType = eQMI_LOC_INJECTED_NETWORK_INITIATED_MESSAGE_TYPE_SUPL_V02;

    LOC_SEND_SYNC_REQ(InjectNetworkInitiatedMessage,
                      INJECT_NETWORK_INITIATED_MESSAGE,
                      suplInitReq, LocApiV02::clientHandle);

    EXIT_LOG(%d, rv);
    return rv;
}

int LBSApiV02::chargerStatusInject(int status)
{
    qmiLocSetExternalPowerConfigReqMsgT_v02 chargerStatus;

    memset(&chargerStatus, 0, sizeof(chargerStatus));
    chargerStatus.externalPowerState = (qmiLocExternalPowerConfigEnumT_v02)status;

    LOC_SEND_SYNC_REQ(SetExternalPowerConfig,
                      SET_EXTERNAL_POWER_CONFIG,
                      chargerStatus, LocApiV02::clientHandle);

    EXIT_LOG(%d, rv);
    return rv;
}

qmiLocReliabilityEnumT_v02 LBSApiV02::mapReliabilityValue(ReliabilityValue reliabilityValue)
{
    switch (reliabilityValue) {
    case RELIABILITY_NOT_SET:
        return eQMI_LOC_RELIABILITY_NOT_SET_V02;
    case RELIABILITY_VERY_LOW:
        return eQMI_LOC_RELIABILITY_VERY_LOW_V02;
    case RELIABILITY_LOW:
        return eQMI_LOC_RELIABILITY_LOW_V02;
    case RELIABILITY_MEDIUM:
        return eQMI_LOC_RELIABILITY_MEDIUM_V02;
    case RELIABILITY_HIGH:
        return eQMI_LOC_RELIABILITY_HIGH_V02;
    default:
        return eQMI_LOC_RELIABILITY_NOT_SET_V02;
    }
}

qmiLocPositionSrcEnumT_v02 LBSApiV02::mapSourceType(PositionSourceType sourceType)
{
    switch (sourceType) {
    case GNSS:
        return eQMI_LOC_POSITION_SRC_GNSS_V02;
    case CELLID:
        return eQMI_LOC_POSITION_SRC_CELLID_V02;
    case ENH_CELLID:
        return eQMI_LOC_POSITION_SRC_ENH_CELLID_V02;
    case WIFI:
        return eQMI_LOC_POSITION_SRC_WIFI_V02;
    case TERRESTRIAL:
        return eQMI_LOC_POSITION_SRC_TERRESTRIAL_V02;
    case GNSS_TERRESTRIAL_HYBRID:
        return eQMI_LOC_POSITION_SRC_GNSS_TERRESTRIAL_HYBRID_V02;
    case OTHER:
        return eQMI_LOC_POSITION_SRC_OTHER_V02;
    default:
        return eQMI_LOC_POSITION_SRC_OTHER_V02;

    }
}

int LBSApiV02::shutdown()
{
    close();
    return true;
}

}; //namespace lbs_core
