/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/
#define LOG_NDDEBUG 0
#define LOG_TAG "LocSvc_LBSProxy"

#ifdef _HAS_LOC_V02_
#include <LBSApiV02.h>
#elif _HAS_LOC_RPC_
#include <LBSApiRpc.h>
#endif
#include <LBSProxy.h>
#include <log_util.h>

using namespace loc_core;
using namespace lbs_core;

namespace lbs_core {

pthread_mutex_t LBSProxy::mLock = PTHREAD_MUTEX_INITIALIZER;
UlpProxyBase* LBSProxy::mUlp = NULL;
LocAdapterBase* LBSProxy::mAdapter = NULL;
unsigned long LBSProxy::mCapabilities = 0;

LocApiBase* LBSProxy::getLocApi(const MsgTask* msgTask,
                                 LOC_API_ADAPTER_EVENT_MASK_T exMask) const {
#ifdef _HAS_LOC_V02_
    return new LBSApiV02(msgTask, exMask);
#elif _HAS_LOC_RPC_
    return new LBSApiRpc(msgTask, exMask);
#endif
    return NULL;
}

void LBSProxy::locRequestUlp(LocAdapterBase* adapter,
                              unsigned long capabilities) {
    pthread_mutex_lock(&LBSProxy::mLock);

    if (NULL == LBSProxy::mUlp) {
        LOC_LOGV("%s] %p", __func__, LBSProxy::mUlp);
    } else {
        LOC_LOGV("%s] ulp %p adapter %p", __func__,
                 LBSProxy::mUlp, adapter);
        LBSProxy::mUlp->setAdapter(adapter);
        LBSProxy::mUlp->setCapabilities(capabilities);
        adapter->setUlpProxy(LBSProxy::mUlp);
    }
    LBSProxy::mAdapter = adapter;
    LBSProxy::mCapabilities = capabilities;

    pthread_mutex_unlock(&LBSProxy::mLock);
}

void LBSProxy::ulpRequestLoc(UlpProxyBase* ulp) {
    pthread_mutex_lock(&LBSProxy::mLock);

    if (NULL == LBSProxy::mAdapter) {
        LOC_LOGV("%s] %p", __func__, LBSProxy::mAdapter);
    } else {
        LOC_LOGV("%s] ulp %p adapter %p", __func__, ulp,
                 LBSProxy::mAdapter);
        ulp->setAdapter(LBSProxy::mAdapter);
        ulp->setCapabilities(LBSProxy::mCapabilities);
        LBSProxy::mAdapter->setUlpProxy(ulp);
    }
    LBSProxy::mUlp = ulp;

    pthread_mutex_unlock(&LBSProxy::mLock);
}

bool LBSProxy :: reportPositionToUlp(UlpLocation &location,
                                     GpsLocationExtended &locationExtended,
                                     void* locationExt,
                                     enum loc_sess_status status,
                                     LocPosTechMask techMask)
{
    LOC_LOGD("%s:%d]: Enter", __func__, __LINE__);
    if(mUlp != NULL) {
        LOC_LOGD("%s:%d]: Exit. Sent to ulp", __func__, __LINE__);
        return mUlp->reportPosition(location, locationExtended, locationExt, status,
                                    techMask);
    }
    else {
        LOC_LOGD("%s:%d]: Exit", __func__, __LINE__);
        return false;
    }
}

}  // namespace lbs_core

extern "C" {
LBSProxyBase* getLBSProxy() {
    return new lbs_core::LBSProxy();
}
}
