
#ifndef PS_EAPI_METHODS_H
#define PS_EAPI_METHODS_H


/*===========================================================================


   E X T E N S I B L E   A U T H E N T I C A T I O N   P R O T O C O L (EAP)
         I N T E R N A L  M E T H O D S  A P I   H E A D E R   F I L E
                   
DESCRIPTION


 The EAP INTERNAL METHODS API header file.


Copyright (c) 2006-2009 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE


  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


 when        who    what, where, why
--------    ---    ----------------------------------------------------------
03/06/06    lti     Created module.

===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "ps_eap.h"

/*===========================================================================

                          PUBLIC DATA DECLARATIONS

===========================================================================*/

#define EAPI_METHODS_PASSWORD_MAX_CHAR 255

/*---------------------------------------------------------------------------
  EAP Password Structure
---------------------------------------------------------------------------*/
typedef struct
{ 
  /* length of password          */
  uint16  len;                     

  /* Password                    */
  uint8   password[EAPI_METHODS_PASSWORD_MAX_CHAR];

}eapi_methods_password_type;

/*---------------------------------------------------------------------------
  Structure used for storing EAP MD5 Credentials
---------------------------------------------------------------------------*/
typedef struct
{
  /* User Id               */
  eap_identity_type              user_id;   

  /* Password              */
  eapi_methods_password_type     pass;  

}eapi_methods_md5_credentials_type;

/*---------------------------------------------------------------------------
  Structure used for storing EAP OTP Credentials
---------------------------------------------------------------------------*/
typedef eapi_methods_md5_credentials_type eapi_methods_otp_credentials_type;

/*---------------------------------------------------------------------------
  Structure used for storing EAP GTC Credentials
---------------------------------------------------------------------------*/
typedef eapi_methods_md5_credentials_type eapi_methods_gtc_credentials_type;

/*---------------------------------------------------------------------------
  Union used for storing EAP Internal Methods Credentials
---------------------------------------------------------------------------*/
typedef union
{
  /* MD5 credentials */
  eapi_methods_md5_credentials_type  md5;

  /* OTP credentials */
  eapi_methods_otp_credentials_type  otp;

  /* GTC credentials */
  eapi_methods_gtc_credentials_type  gtc;

}eapi_methods_credentials_type;

/*---------------------------------------------------------------------------
  Prototype for the get credentials callback function that the lower layer 
  registers to allow EAP to retrieve the inner methods data
---------------------------------------------------------------------------*/
typedef boolean (*eapi_methods_get_credentials_cback_type)
(
  eap_method_enum_type            type,

  eapi_methods_credentials_type*  eap_credentials
);

/*---------------------------------------------------------------------------
  Authentication manager structure
---------------------------------------------------------------------------*/
typedef struct
{
  eapi_methods_get_credentials_cback_type  eapi_methods_get_credentials_f_ptr;

}eapi_methods_auth_mgr_type;

#endif /* PS_EAPI_METHODS_H */
