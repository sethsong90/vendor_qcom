/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/

#ifndef LBS_ADAPTER_H
#define LBS_ADAPTER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <string.h>
#include <LBSAdapterBase.h>

using namespace loc_core;
using namespace lbs_core;

struct WiperSsrInform : public LocMsg {
    inline WiperSsrInform() : LocMsg() {}
    virtual void proc() const;
};

struct WiperRequest : public LocMsg {
    enum WifiRequestType mType;
    inline WiperRequest(enum WifiRequestType type) :
        LocMsg(), mType(type) {}
    virtual void proc() const;
};

struct WiperApDataRequest : public LocMsg {

    inline WiperApDataRequest() : LocMsg() {}
    virtual void proc() const;
};

class LBSCallback {
public:
    LBSCallback() {}
    inline virtual ~LBSCallback() {}
    virtual void locationReport(GpsLocation& location) {};
};

class LBSAdapter : public LBSAdapterBase {
    static LBSAdapter* mMe;
    LBSCallback *mLBSCallbacks;
    LBSAdapter(const LOC_API_ADAPTER_EVENT_MASK_T mask,
               MsgTask::tCreate tCreator, LBSCallback *callbacks);
    inline virtual ~LBSAdapter() {}
    virtual void getZppFixSync();
public:
    static LBSAdapter* get(const LOC_API_ADAPTER_EVENT_MASK_T mask,
                           MsgTask::tCreate tCreator, LBSCallback *callbacks);

    virtual bool requestWps(enum WifiRequestType type);
    virtual bool requestWifiApData();
    virtual void handleEngineUpEvent();

    int cinfoInject(int cid, int lac, int mnc, int mcc, bool roaming);
    int oosInform();
    int niSuplInit(char* supl_init, int length);
    int chargerStatusInject(int status);
    int wifiStatusInform();
    int injectCoarsePosition(CoarsePositionInfo cpInfo);
    int injectWifiPosition(WifiLocation wifiInfo);
    int injectWifiApInfo(WifiApInfo wifiApInfo);
    int setWifiWaitTimeoutValue(uint8_t timeout);

    // Zpp related
    int getZppFixRequest();

};

#ifdef __cplusplus
}
#endif

#endif /* LBS_ADAPTER_H */
