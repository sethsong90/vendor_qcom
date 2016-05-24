#ifndef PS_DNS_PROXY_EXT_H
#define PS_DNS_PROXY_EXT_H
/*===========================================================================

                P S _ D N S _ P R O X Y _ E X T . H

DESCRIPTION
   This is DNS Proxy external header file.


  Copyright (c) 2010 by Qualcomm Technologies, Inc.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/inc/ps_dns_proxy_ext.h#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS
#include "ps_iface.h"

/*===========================================================================

                            DATA DECLARATIONS

===========================================================================*/

/*===========================================================================

                            FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION DNS_PROXY_ENABLE()

DESCRIPTION
  This function enables the DNS Proxy. It takes two iface pointers as input.
  The RM iface pointer is the iface where DNS proxy will receive requests.
  The UM iface pointer is the iface where DNS request will be forwarded to
  the actual DNS server that is configured on the UM iface.

  As of now only one DNS proxy can be run i.e. DNS proxy can enabled on
  only on one interface.

DEPENDENCIES

PARAMETERS
  rm_iface_ptr: Pointer to the iface where DNS proxy will receive request.
  um_iface_ptr: Pointer to the iface where DNS proxy will forward the
                request.
  ds_errno:     Error, if any.

RETURN VALUE
  Success  0
  Fail    -1
  

SIDE EFFECTS
  None
===========================================================================*/
int32
dns_proxy_enable
(
 ps_iface_type               *nat_iface_ptr,
 sint15                      *ds_errno 
);

/*===========================================================================
FUNCTION DNS_PROXY_DISABLE()

DESCRIPTION
  This function disables the DNS Proxy. Since we support only one DNS proxy
  in the system, the input rm_iface_ptr to this function should be the same
  as used when the DNS Proxy was enabled.

DEPENDENCIES

PARAMETERS
  rm_iface_ptr: The iface on which DNS proxy is running.
  ds_errno: Error number, if any.

RETURN VALUE
  Success  0
  Fail    -1
  

SIDE EFFECTS
  None
===========================================================================*/
int32
dns_proxy_disable
(
 ps_iface_type               *nat_iface_ptr,
 sint15                      *ds_errno 
);

#endif /* FEATURE_DATA_PS */
#endif /* PS_DNS_PROXY_EXT_H */
