#ifndef PS_FTP_ALG_CLIENT_H
#define PS_FTP_ALG_CLIENT_H
/*===========================================================================

                P S _ F T P _ A L G _ C L I E N T . H

DESCRIPTION
   This is NAT iface handler header file.

Copyright (c) 2010 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/inc/ps_ftp_alg_client.h#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "customer.h"
#include "comdef.h"

#ifdef FEATURE_DATA_PS_NAT_IFACE
#ifdef FEATURE_DATA_PS_FTP_ALG

#ifdef __cplusplus
extern "C"
{
#endif

/*===========================================================================

                            FUNCTION DECLARATIONS

===========================================================================*/
void ftp_alg_client_init ( void );

void ftp_alg_client_cleanup ( void );

#ifdef __cplusplus
}
#endif

#endif /* FEATURE_DATA_PS_FTP_ALG */
#endif /* FEATURE_DATA_PS_NAT_IFACE */
#endif /* PS_FTP_ALG_CLIENT_H */

