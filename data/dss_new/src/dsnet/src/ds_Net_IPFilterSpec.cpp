/*===========================================================================
  FILE: IPFilterSpec.cpp

  OVERVIEW: This file provides implementation of the IPFilterSpec class.

  DEPENDENCIES: None

  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_IPFilterSpec.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-08-10 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "AEEStdErr.h"
#include "ds_Net_IPFilterSpec.h"
#include "ds_Net_IIPFilterPriv.h"
#include "ds_Net_CreateInstance.h"
#include "ds_Net_Platform.h"

using namespace ds::Net;
using namespace ds::Error;
using namespace NetPlatform;

/*===========================================================================

                     PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  CONSTRUCTOR/DESTRUCTOR
---------------------------------------------------------------------------*/
IPFilterSpec::IPFilterSpec
(
  void
): mpIPFilterSpecClone(NULL),
   mpPSQoSSpecWrapper(NULL),
   refCnt(1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p", this, 0, 0);

  mpPSQoSSpecWrapper = new PSQoSSpecWrapper();
}

IPFilterSpec::~IPFilterSpec()
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

  DS_UTILS_RELEASEIF(mpIPFilterSpecClone);
  DS_UTILS_RELEASEIF(mpPSQoSSpecWrapper);
}

int IPFilterSpec::Clone
(
  IIPFilterPriv** filterObj
)
{
  int result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);

  if (NULL == filterObj)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  mpIPFilterSpecClone = new IPFilterSpec();
  if (NULL == mpIPFilterSpecClone)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  result = mpIPFilterSpecClone->mpPSQoSSpecWrapper->Clone(mpPSQoSSpecWrapper);
  if(AEE_SUCCESS != result)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  *filterObj = static_cast <IIPFilterPriv *> (mpIPFilterSpecClone);

  (void) (*filterObj)->AddRef();

  return AEE_SUCCESS;

bail:

  LOG_MSG_ERROR ("Clone failed, err %d", result, 0, 0);

  DS_UTILS_RELEASEIF(mpIPFilterSpecClone);

  return result;
}

int IPFilterSpec::GetOptionsInternal
(
  IPFilterIDType*  pOpts,
  int              optsLen,
  int *            pOptsLenReq,
  boolean          isErrMask
)
{
  int                             result;
  int                             index;
  uint32                          mask;
  uint8                           nextHdrProto;
  NetPlatform::PSFilterSpecType   localPSIPFilterSpec;

  /* Change the array size to current max num of options supported */
  IPFilterIDType   localOptsArr[20] = {0,};
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/


  LOG_MSG_INFO2 ("obj 0x%p", this, 0, 0);
  
  result = mpPSQoSSpecWrapper->GetPSIPFilterSpec(&localPSIPFilterSpec);

  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  nextHdrProto = (uint8) PS_NO_NEXT_HDR; 
  index = 0;

  if (IP_V4 == localPSIPFilterSpec.ip_vsn)
  {
    if ((uint32)IPFLTR_MASK_IP4_NEXT_HDR_PROT & 
        localPSIPFilterSpec.ip_hdr.v4.field_mask)
    {
      nextHdrProto = localPSIPFilterSpec.ip_hdr.v4.next_hdr_prot;
    }

    if (TRUE == isErrMask)
    {
      mask = localPSIPFilterSpec.ip_hdr.v4.err_mask;
    }
    else
    {
      mask = localPSIPFilterSpec.ip_hdr.v4.field_mask;
    }

    if ((uint32)IPFLTR_MASK_IP4_SRC_ADDR & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_SRC_ADDR;
    }

    if ((uint32)IPFLTR_MASK_IP4_DST_ADDR & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_DST_ADDR;
    }

    if ((uint32)IPFLTR_MASK_IP4_NEXT_HDR_PROT & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_NEXT_HDR_PROTO;
    }

    if ((uint32)IPFLTR_MASK_IP4_TOS & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_IPV4_TOS;
    }
  }
  else if (IP_V6 == localPSIPFilterSpec.ip_vsn)
  {
    if ((uint32)IPFLTR_MASK_IP6_NEXT_HDR_PROT & 
        localPSIPFilterSpec.ip_hdr.v6.field_mask)
    {
      nextHdrProto = localPSIPFilterSpec.ip_hdr.v6.next_hdr_prot;
    }

    if (TRUE == isErrMask)
    {
      mask = localPSIPFilterSpec.ip_hdr.v6.err_mask;
    }
    else
    {
      mask = localPSIPFilterSpec.ip_hdr.v6.field_mask;
    }

    if ((uint32)IPFLTR_MASK_IP6_SRC_ADDR & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_SRC_ADDR;
    }

    if ((uint32)IPFLTR_MASK_IP6_DST_ADDR & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_DST_ADDR;
    }

    if ((uint32)IPFLTR_MASK_IP6_NEXT_HDR_PROT & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_NEXT_HDR_PROTO;
    }

    if ((uint32)IPFLTR_MASK_IP6_TRAFFIC_CLASS & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_IPV6_TRF_CLASS;
    }

    if ((uint32)IPFLTR_MASK_IP6_FLOW_LABEL & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_IPV6_FLOW_LABEL;
    }
  }


  if ((uint8) PS_IPPROTO_TCP == nextHdrProto)
  {
    /* Return valid TCP options */
    if (TRUE == isErrMask)
    {
      mask = localPSIPFilterSpec.next_prot_hdr.tcp.err_mask;
    }
    else
    {
      mask = localPSIPFilterSpec.next_prot_hdr.tcp.field_mask;
    }

    if ((uint32)IPFLTR_MASK_TCP_SRC_PORT & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_TCP_SRC_PORT;
    }

    if ((uint32)IPFLTR_MASK_TCP_DST_PORT & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_TCP_DST_PORT;
    }
  }
  else if ((uint8) PS_IPPROTO_UDP == nextHdrProto)
  {
    /* Return valid UDP options */
    if (TRUE == isErrMask)
    {
      mask = localPSIPFilterSpec.next_prot_hdr.udp.err_mask;
    }
    else
    {
      mask = localPSIPFilterSpec.next_prot_hdr.udp.field_mask;
    }

    if ((uint32)IPFLTR_MASK_UDP_SRC_PORT & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_UDP_SRC_PORT;
    }

    if ((uint32)IPFLTR_MASK_UDP_DST_PORT & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_UDP_DST_PORT;
    }
  }
  else if ((uint8) PS_IPPROTO_ESP == nextHdrProto)
  {
    /* Return valid ESP options */
    if (TRUE == isErrMask)
    {
      mask = localPSIPFilterSpec.next_prot_hdr.esp.err_mask;
    }
    else
    {
      mask = localPSIPFilterSpec.next_prot_hdr.esp.field_mask;
    }

    if ((uint32)IPFLTR_MASK_ESP_SPI & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_ESP_SPI;
    }
  }
  else if ((uint8) PS_IPPROTO_ICMP == nextHdrProto)
  {
    /* Return valid ICMP options */
    if (TRUE == isErrMask)
    {
      mask = localPSIPFilterSpec.next_prot_hdr.icmp.err_mask;
    }
    else
    {
      mask = localPSIPFilterSpec.next_prot_hdr.icmp.field_mask;
    }

    if ((uint32)IPFLTR_MASK_ICMP_MSG_TYPE & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_ICMP_MTYPE;
    }

    if ((uint32)IPFLTR_MASK_ICMP_MSG_CODE & mask)
    {
      localOptsArr[index++] = IPFilterID::QDS_ICMP_MCODE;
    }
  }
  if(NULL != pOptsLenReq)
  {
      *pOptsLenReq = index;
  }
  if (NULL == pOpts)
  {
   if(0 != optsLen)
   {
      result = QDS_EFAULT;
      goto bail;
   }
   return AEE_SUCCESS;
  }
  if(0 == optsLen)
  {
     return AEE_SUCCESS;
  }
  if (optsLen < index)
  {
    (void) memcpy (pOpts,  
                   localOptsArr,
                   optsLen * sizeof (IPFilterIDType*));
  }
  else
  {
    (void) memcpy (pOpts,  
                   localOptsArr,
                   index * sizeof (IPFilterIDType*));
  }

  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("GetOptionsInternal failed, err %d", result, 0, 0);
  return result;


} /* GetOptionsInternal() */

int IPFilterSpec::GetValidOptions
(
  IPFilterIDType *pValidOptions,
  int             validOptionsLen,
  int             *pValidOptionsLenReq
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO1 ("GetValidOptions()", 0, 0, 0);

  return GetOptionsInternal (pValidOptions,
                             validOptionsLen,
                             pValidOptionsLenReq,
                             FALSE);

} /* GetValidOptions() */


int IPFilterSpec::GetErroneousOptions
(
  IPFilterIDType *pErrOpts,
  int            errOptsLen,
  int            *pErrOptsLenReq
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO1 ("GetErroneousOptions()", 0, 0, 0);

  return GetOptionsInternal (pErrOpts,
                             errOptsLen,
                             pErrOptsLenReq,
                             TRUE);

} /* GetErroneousOptions() */


int IPFilterSpec::GetIPVsn
(
  IPFilterIPVersionType *pIPVersion
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return mpPSQoSSpecWrapper->GetIPVsn((unsigned char *)pIPVersion);
}

int IPFilterSpec::SetIPVsn
(
  IPFilterIPVersionType ipVersion
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return mpPSQoSSpecWrapper->SetIPVsn(ipVersion);

}

int IPFilterSpec::GetNextHdrProt
(
  IPFilterIPNextProtocolType* pNextHdrProt
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  return mpPSQoSSpecWrapper->GetNextHdrProt((unsigned char *)pNextHdrProt);
}


int IPFilterSpec::SetNextHdrProt
(
  IPFilterIPNextProtocolType nextHdrProt
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  return mpPSQoSSpecWrapper->SetNextHdrProt(nextHdrProt);
}


int IPFilterSpec::GetSrcPort
(
  IPFilterPortType* pSrcPort
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  unsigned short int* localPort = (unsigned short int*)pSrcPort->port;
  return mpPSQoSSpecWrapper->GetSrcPort(localPort, &(pSrcPort->range));
}


int IPFilterSpec::SetSrcPort
(
  const IPFilterPortType* pSrcPort
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  return mpPSQoSSpecWrapper->SetSrcPort(*(unsigned short*)pSrcPort->port, pSrcPort->range);
}


int IPFilterSpec::GetDstPort
(
  IPFilterPortType* pDstPort
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  unsigned short int* localPort = (unsigned short int*)pDstPort->port;
  return mpPSQoSSpecWrapper->GetDstPort(localPort, &(pDstPort->range));
}

int IPFilterSpec::SetDstPort
(
  const IPFilterPortType* pDstPort
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  return mpPSQoSSpecWrapper->SetDstPort(*(unsigned short*)pDstPort->port, pDstPort->range);
  
}

int IPFilterSpec::GetSrcV4
(
  IPFilterIPv4AddrType* pSrcV4
)
{
  unsigned int addr = 0;
  unsigned int subnet = 0;
  int          res;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  res = mpPSQoSSpecWrapper->GetSrcV4(&addr, &subnet);
  if(AEE_SUCCESS != res)
  {
    return res;
  }

  memcpy((void *)pSrcV4->addr, (const void *)&addr, sizeof(INAddrType));

  memcpy((void *)pSrcV4->subnetMask, (const void *)&subnet, sizeof(INAddrType));
  
  return AEE_SUCCESS;

}

int IPFilterSpec::SetSrcV4
(
  const IPFilterIPv4AddrType* pSrcV4
)
{
  unsigned int addr = 0;
  unsigned int subnet = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  memcpy((void *)&addr, (const void *)pSrcV4->addr, sizeof(INAddrType));
  memcpy((void *)&subnet, (const void *)pSrcV4->subnetMask, sizeof(INAddrType));
  return mpPSQoSSpecWrapper->SetSrcV4(addr, subnet);

}

int IPFilterSpec::GetDstV4
(
  IPFilterIPv4AddrType* pDstV4
)
{
  unsigned int addr = 0;
  unsigned int subnet = 0;
  int          res;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  res = mpPSQoSSpecWrapper->GetDstV4(&addr, &subnet);
  if(AEE_SUCCESS != res)
  {
    return res;
  }

  memcpy((void *)pDstV4->addr, (const void *)&addr, sizeof(INAddrType));

  memcpy((void *)pDstV4->subnetMask, (const void *)&subnet, sizeof(INAddrType));
  
  return AEE_SUCCESS;
}

int IPFilterSpec::SetDstV4
(
  const IPFilterIPv4AddrType* pDstV4
)
{
  unsigned int addr = 0;
  unsigned int subnet = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  memcpy((void *)&addr, (const void *)pDstV4->addr, sizeof(INAddrType));
  memcpy((void *)&subnet, (const void *)pDstV4->subnetMask, sizeof(INAddrType));

  return mpPSQoSSpecWrapper->SetDstV4(addr, subnet);

}

int IPFilterSpec::GetTos
(
  IPFilterTOSType* pToS
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return mpPSQoSSpecWrapper->GetTos(&(pToS->val), &(pToS->mask));

}

int IPFilterSpec::SetTos
(
  const IPFilterTOSType* pToS
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  return mpPSQoSSpecWrapper->SetTos(pToS->val, pToS->mask);

}

int IPFilterSpec::GetFlowLabel
(
  IPFilterIPv6FlowLabelType* pFlowLabel
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return mpPSQoSSpecWrapper->GetFlowLabel((int *) pFlowLabel);

}

int IPFilterSpec::SetFlowLabel
(
  IPFilterIPv6FlowLabelType flowLabel
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return mpPSQoSSpecWrapper->SetFlowLabel(flowLabel);

}

int IPFilterSpec::GetSrcV6
(
  IPFilterIPv6AddrType* pSrcV6
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  INAddr6Type* localAddr = (INAddr6Type *)(pSrcV6->addr);
  return mpPSQoSSpecWrapper->GetSrcV6(localAddr, &(pSrcV6->prefixLen));

}

int IPFilterSpec::SetSrcV6
(
  const IPFilterIPv6AddrType* pSrcV6
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return mpPSQoSSpecWrapper->SetSrcV6(pSrcV6->addr, pSrcV6->prefixLen);

}

int IPFilterSpec::GetDstV6
(
  IPFilterIPv6AddrType* pDstV6
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  INAddr6Type* localAddr = (INAddr6Type*)(pDstV6->addr);
  return mpPSQoSSpecWrapper->GetDstV6(localAddr, &(pDstV6->prefixLen));

}

int IPFilterSpec::SetDstV6
(
  const IPFilterIPv6AddrType* pDstV6
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return mpPSQoSSpecWrapper->SetDstV6(pDstV6->addr, pDstV6->prefixLen);

}


int IPFilterSpec::GetTrafficClass
(
  IPFilterIPv6TrafficClassType* pTrafficClass
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 
  return mpPSQoSSpecWrapper->GetTrafficClass(&(pTrafficClass->val), &(pTrafficClass->mask));

}

int IPFilterSpec::SetTrafficClass
(
  const IPFilterIPv6TrafficClassType* pTrafficClass
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  return mpPSQoSSpecWrapper->SetTrafficClass(pTrafficClass->val, pTrafficClass->mask);
}

int IPFilterSpec::GetAuxInfo
(
  IPFilterAuxInfoType* pAuxInfo
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return mpPSQoSSpecWrapper->GetAuxInfo(&(pAuxInfo->fi_id), &(pAuxInfo->fi_precedence));

}

ds::ErrorType CDECL IPFilterSpec::GetEspSpi 
(
  int* pEspSpi
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  return mpPSQoSSpecWrapper->GetEspSpi(pEspSpi);
  
} /* GetEspSpi() */



ds::ErrorType CDECL IPFilterSpec::GetICMPType 
(
  unsigned char* ICMPType
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return mpPSQoSSpecWrapper->GetICMPType(ICMPType);
  
} /* GetICMPType() */



ds::ErrorType CDECL IPFilterSpec::GetICMPCode
(
  unsigned char* ICMPCode
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return mpPSQoSSpecWrapper->GetICMPCode(ICMPCode);
  
} /* GetICMPCode() */



ds::ErrorType CDECL IPFilterSpec::SetEspSpi 
(
 int espSpi
)
{
  
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return mpPSQoSSpecWrapper->SetEspSpi(espSpi);
  
} /* SetEspSpi() */




ds::ErrorType CDECL IPFilterSpec::SetICMPType
( 
  unsigned char ICMPType
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  return mpPSQoSSpecWrapper->SetICMPType(ICMPType);
  
} /* SetICMPType() */



ds::ErrorType CDECL IPFilterSpec::SetICMPCode
( 
  unsigned char ICMPCode
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return mpPSQoSSpecWrapper->SetICMPCode(ICMPCode);
  
} /* SetICMPCode() */




ds::ErrorType IPFilterSpec::GetPSIPFilterSpec
(
  NetPlatform::PSFilterSpecType *pPSIPFilterSpec
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == pPSIPFilterSpec)
  {
    return QDS_EFAULT;
  }

  return mpPSQoSSpecWrapper->GetPSIPFilterSpec(pPSIPFilterSpec);

} /* GetPSIPFilterSpec() */

ds::ErrorType IPFilterSpec::UpdateSpec
(
  NetPlatform::PSFilterSpecType *pPSIPFilterSpec
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == pPSIPFilterSpec)
  {
    return QDS_EFAULT;
  }
  
  return mpPSQoSSpecWrapper->UpdatePSIPFilterSpec(pPSIPFilterSpec);

} /* UpdateSpec() */

ds::ErrorType IPFilterSpec::QueryInterface
(
  AEEIID iid,
  void **ppo
)
{
  int result = AEE_SUCCESS;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1  ("Obj 0x%p, ref cnt %d, iid 0x%x", this, refCnt, iid);

  if (NULL == ppo)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *ppo = NULL;

  switch (iid)
  {
    case AEEIID_IIPFilterPriv:
      *ppo = static_cast <IIPFilterPriv *>(this);
      (void) AddRef(); 
      break;

    case AEEIID_IQI:
      *ppo = reinterpret_cast <IQI *> (this);
      (void) AddRef();
      break;

    default:
      return AEE_ECLASSNOTSUPPORT;
   }

   return result;
}
