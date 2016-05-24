#ifndef PS_UDP_HDR_H
#define PS_UDP_HDR_H
/*===========================================================================

                         P S _ U D P _ H D R . H
                   
DESCRIPTION
 The Data Services UDP protocol interface header file. This contains UDP
 header definition.
 

Copyright (c) 2008 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $PVCSPath: L:/src/asw/MM_DATA/vcs/ps_udp.h_v   1.0   08 Aug 2002 11:19:54   akhare  $
  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_udp.h#1 $ $DateTime: 2011/01/10 09:44:56 $ $Author: maldinge $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
12/14/08    pp     Created module as part of Common Modem Interface: 
                   Public/Private API split.
===========================================================================*/

/*===========================================================================

                        INCLUDE FILES FOR THE MODULE
                       
===========================================================================*/

/*===========================================================================

                          PUBLIC DATA DECLARATIONS

===========================================================================*/

/*---------------------------------------------------------------------------
  Definition of the maximum UDP payload length.  This is set to be 1472
  bytes, which accounts for the 20 byte IP header and 8 byte UDP header
  which are added to the payload to create a 1500 byte packet, which is the
  maximum Ethernet payload size.  The IP header does not allow 
  fragmentation (the DF bit is set), thus any payload greater than 1500 
  bytes will be discarded by Ethernet.  
---------------------------------------------------------------------------*/
#define UDP_MAX_PAYLOAD_LEN 1472

/*---------------------------------------------------------------------------
  Structure definition for the UDP header.
---------------------------------------------------------------------------*/
typedef struct
{
  uint16 source;                                            /* Source Port */
  uint16 dest;                                         /* Destination Port */
  uint16 length;                                      /* UDP packet length */
  uint16 chksum;                                    /* UDP packet checksum */
} udp_hdr_type;                                         /* UDP header type */


#endif /* PS_UDP_HDR_H */
