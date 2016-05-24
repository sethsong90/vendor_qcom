#ifndef DS_NET_CNETWORKFACTORY_H
#define DS_NET_CNETWORKFACTORY_H

/*============================================================================
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#define ds_Net_AEECLSID_CNetworkFactory 0x1073e52
#else /* C++ */
#include "AEEStdDef.h"
namespace ds
{
   namespace Net
   {
      const ::AEECLSID AEECLSID_CNetworkFactory = 0x1073e52;
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
/*
  ================================================================================
  Class DOCUMENTATION
  ================================================================================
  
  AEECLSID_DSNetNetworkFactory
  
  Description:
  Allows user to create an instance of ds_Net_INetworkFactory interface
  object for creating ds_Net_INetwork object
  
  Default Interfaces:
  ds_Net_INetworkFactory
  
  Other Supported Interfaces (directly - via IQI):
  None.
  
  Other Supported Interfaces (indirectly):
  ds_Net_INetwork -  Can be obtained via INetworkFactory_CreateNetwork.
  Other Interfaces supported from ds_Net_INetwork (via IQI):
  ds_Net_INetworkExt - Other Interfaces supported from ds_Net_INetworkExt
  ds_Net_IBearerInfo - Can be obtained via INetworkExt_GetBearerInfo.
  ds_Net_IQoSManager - Can be obtained via INetworkExt_CreateQoSManager.
  Other Interfaces supported from ds_Net_IQoSManager:
  ds_Net_IQoSSecondary via RequestSecondary.
  Other Interfaces supported from ds_Net_IQoSSecondary:
  ds_Net_IQoS1x via GetTechObject.
  ds_Net_IPhysLink via GetTXPhysLink and GetRXPhysLink.
  ds_Net_IQoSSecondariesInput via CreateQoSSecondariesInput.
  ds_Net_IQoSSecondariesOutput via RequestBundle.
  ds_Net_IQoSDefault via GetQosDefault.
  Other Interfaces supported from ds_Net_IQoSDefault:
  ds_Net_IQoS1x via GetTechObject.
  ds_Net_IPhysLink via GetTXPhysLink and GetRXPhysLink.
  ds_Net_IQoSSpec via CreateQoSSpec.
  ds_Net_IQoSSpecsGroup via CreateQoSSpecsGroup.
  ds_Net_IMCastManager - Can be obtained via INetworkExt_CreateMCastManager.
  Other Interfaces supported from ds_Net_IMCastManager:
  ds_Net_IMCastSession via Join.
  ds_Net_IMCastManagerBCMCS via GetTechObject. 
  ds_Net_IMCastManagerExt - Can be obtained via INetworkExt_CreateMCastManager.
  Other Interfaces supported from ds_Net_IMCastManager:
  ds_Net_IMCastSession via Join.
  ds_Net_IMCastSessionsInput via CreateMCastSessionsInput.
  ds_Net_IMCastSessionsOutput via JoinBundle.
  ds_Net_IMCastManagerBCMCS via GetTechObject. 
  
  ds_Net_INetworkControl - Other Interfaces supported from ds_Net_INetworkControl:
  None.
  
  Other Interfaces supported from ds_Net_INetwork:
  ds_Net_INetwork1x via GetTechObject.
  ds_Net_INetworkUMTS via GetTechObject.
  ds_Net_INetworkIPv6 via GetTechObject.
  Other Interfaces supported from ds_Net_INetworkIPv6:
  ds_Net_IIPv6Address via GeneratePrivAddr and GetIPv6Address.
  ds_Net_IPhysLink via GetTXPhysLink and GetRXPhysLink.
  
  ds_Net_IPolicy -   Can be obtained via INetworkFactory_CreatePolicy.
  Other Interfaces supported from ds_Net_IPolicy (via 
  IQI):
  None.
  ds_Net_ITechUMTS - Can be obtained via INetworkFactory_CreateTechUMTS.
  Other Interfaces supported from ITechUMTS (via 
  IQI):
  None.
  
  Privileges for the default interface:
  AEEPRIVID_PNet          - CreateNetwork 
  with NetworkModeType_QDS_ACTIVE
  AEEPRIVID_PNetMonitored - CreateNetwork 
  with NetworkModeType_QDS_MONITORED
  AEEPRIVID_PNetPolicy    - CreatePolicy
  AEEPRIVID_PNet and
  AEEPRIVID_PMTPD         - CreateTechUMTS
  
  Privileges for Other (directly) Supported Interfaces:
  None
  
  Privileges for Other (indirectly) Supported Interfaces:   
  ds_Net_INetwork        Privileges:
  AEEPRIVID_PNet          - Stop API
  ds_Net_INetworkControl Privileges:
  AEEPRIVID_PGoNull       - GoNull API
  AEEPRIVID_PGoDormant    - GoDormant API
  AEEPRIVID_PNetOTA       - RefreshDHCPConfigInfo API
  AEEPRIVID_PNet          - OnStateChange(QDS_EV_EXTENDED_IP_CONFIG) API
  ds_Net_INetworkExt Privileges:
  AEEPRIVID_PQoS          - OnStateChange(QDS_EV_QOS_AWARENESS),
  QosAware
  GetQoSMode
  CreateQoSManager APIs
  AEEPRIVID_PMCastClient  - CreateMCastManager API
  ds_Net_INetwork1x Privileges:
  AEEPRIVID_PQoS          - QoSNAPriority
  AEEPRIVID_PTech1x       - EnableHoldDown
  SetMDR
  SetQoSNAPriority
  SetRLPAllCurrentNAK
  SetRLPDefCurrentNAK APIs
  ds_Net_INetworkIPv6 Privileges:
  AEEPRIVID_PPrivateAddr  - GeneratePrivAddr API
  ds_Net_IQoS1x Privileges:
  AEEPRIVID_PTech1x       - SetInactivityTimer API
  ds_Net_IPhysLink Privileges:
  AEEPRIVID_PGoDormant    - GoDormant and GoActive APIs
  ds_Net_IQoSDefault Privileges:
  AEEPRIVID_PNetSystem    - ModifyPrimary
  ds_Net_IQoSManager Privileges:
  AEEPRIVID_PNetSystem    - QosDefault
  
  See Also:
  None
  
  ================================================================================
  DOCUMENTATION for other supported classes
  ================================================================================
  
  ds_Net_IBearerInfo:
  
  In this implementation, only one subtechnology of BearerTechnologyCDMA will
  be selected at a time. Therefore the service options can be uniquely 
  identified even though some of the values are the same (for example,
  SERVICEOPTIONS_1X_IS95 == SERVICEOPTIONS_EVDO_DPA = 0x1).
  
  This is not the case for BearerTechnologyUMTS, where multiple subtechnologies
  can be set - see documentation of ds_Net_BearerTechUMTS in ds_Net_IBearerInfo.idl 
  (WCDMA and possibly HSDPA/HSUPA subtechnologies may be simultaneously set). 
  Therefore the values of all UMTS service options are distinct.
  
  ================================================================================
  
  ds_Net_IPolicy:
  
  IPolicy_ProfileList: It is currently reserved for future use and is not
  processed as part of the Policy specification.
  
  ================================================================================*/

#endif //DS_NET_CNETWORKFACTORY_H
