#ifndef PS_RULE_TABLE_H
#define PS_RULE_TABLE_H
/*===========================================================================
  @file 

  This file defines the class that implements the PS::Sock::Platform::ISocket
  interface.

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/inc/PS_Rule_Table.h#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "target.h"

#ifdef FEATURE_DATA_PS_SOFTAP

#include "ps_in.h"
#include "ps_nat.h"

namespace PS
{
  namespace RuleTable
  {
    //common defs for Rule Tables
  }
}

namespace PS
{
  namespace RuleTable
  {
    /**
      @brief Defines an interface for Translation Tables to be implemented.
    */
    class TranslationTable
    {
      public:
        /**
          Adds a new Rule in the Table.

          @param[in]      protocol        Transport layer protocol value.
          @param[in]      privateIp       Local LAN IP address.
          @param[in]      privatePort     Local LAN transport port number.
          @param[in]      globalIp        NAT Global IP address.
          @param[in]      globalPort      NAT Global transport port number.
          @param[in]      targetIp        Destination IP address.
          @param[in]      targetPort      Destination transport port number.
          @param[in]      timeout         Timeout related to Table entry.
          @param[out]     retEntryPtr     Ptr to added Table entry.
          @param[in]      dir             Direction of packet (inbound/outbound/
	                                  dont_care).
          
          @retval  0            Operation successful
          @retval  -1           Operation failed
        */

        virtual int32 Add
        (
          uint8   protocol, 
          uint32  privateIp,
          uint16  privatePort,
          uint32  globalIp,
          uint16  globalPort,
          uint32  targetIp, 
          uint16  targetPort,
          uint16  timeout,
          uint32 *retEntry,
          nat_pkt_direction_e dir
        ) = 0;

        /**
          Looks up a Rule in the Table.

          @param[in]      protocol        Transport layer protocol value.
          @param[in]      privateIp       Local LAN IP address.
          @param[in]      privatePort     Local LAN transport port number.
          @param[in]      globalIp        NAT Global IP address.
          @param[in]      globalPort      NAT Global transport port number.
          @param[in]      targetIp        Destination IP address.
          @param[in]      targetPort      Destination transport port number.
          @param[in]      dir             Direction of packet (inbound/outbound/
	                                  dont_care).
          
	  @retval  0            Operation failed
          @retval  "+ve value"  Operation success, entry looked returned.
        */
	
        virtual uint32 Lookup 
        (
          uint8   protocol, 
          uint32  privateIp,
          uint16  privatePort,
          uint32  globalIp,
          uint16  globalPort,
          uint32  targetIp, 
          uint16  targetPort,
          nat_pkt_direction_e dir	
        ) = 0;

        /**
          Deletes a Rule from the Table.

          @param[in]      entry        Table entry to be deleted.
          
	  @retval  0            Operation successful
          @retval  -1           Operation failed
        */

        virtual int32 Delete
        (
          void *entry
        ) = 0;

        /**
          @var maxNumEntries   Maximum entries Table can hold.
	  @var totalNumEntries Total entries populated in the Table.
        */

        uint32   maxNumEntries;
        uint32   totalNumEntries;
    }; //TranslationTable

  }//RuleTable

} //PS

#endif /* FEATURE_DATA_PS_SOFTAP */
#endif /* PS_RULE_TABLE_H */
