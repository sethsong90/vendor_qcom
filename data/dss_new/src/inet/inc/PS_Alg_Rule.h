#ifndef PS_ALG_RULE_H
#define PS_ALG_RULE_H
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

  $Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/inc/PS_Alg_Rule.h#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "target.h"

#ifdef FEATURE_DATA_PS_SOFTAP

#include "PS_Rule_Table.h"
#include "ps_alg_mgri.h"

#define ALG_HASH_SIZE 20
#define MAX_NAT_ASSOC_PER_ALG 10
#define ALG_MAX_ENTRIES 20

namespace PS
{

  namespace RuleTable
  {

    class AlgTable:public TranslationTable
    {
      /**
        @brief ALG association data struct.
               Holds NAT associations for each ALG entry. 
               NAT Global port is stored to retrieve association details.
      */
      typedef struct
      {
        uint16                      globalPort;
        uint32                      natHandle;
      } algAssocDataType;

      typedef struct algEntry
      {
        uint8                       protocol; /* TCP/UDP/ICMP */
        uint16                      sourcePort;        
        uint16                      destPort;
        uint32                      hashKey;

        struct algEntry            *nextEntryPtr;  
        struct algEntry            *prevEntryPtr;  
        struct algEntry            *nextFreeEntryPtr;  

        /* associated data */
        algAssocDataType            assocData[MAX_NAT_ASSOC_PER_ALG];
        nat_alg_read_cb_fn          ruleCb;
        uint32                      cbData;
      } algEntryType;
    
    public:
      /**
        @brief Gets the global ALG Table handle.

        @retval  0              Operation failed
        @retval  "handle > 0"   Operation successful, returns handle to ALG Table        
      */

      static AlgTable * GetHandle
      (
        void
      );
      
      /**
        @brief Creates the global ALG Table instance.

        @param[in]      maxNumEntries     Maximum number of entries ALG Table can hold.
          
        @retval  0              Operation failed
        @retval  "handle > 0"   Operation successful, returns handle to ALG Table        
      */

      static AlgTable * CreateInstance
      (
        int32 maxNumEntries
      );

      int32 Add 
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
      );

      uint32 Lookup 
      (
        uint8   protocol, 
        uint32  privateIp,
        uint16  privatePort,
        uint32  globalIp,
        uint16  globalPort,
        uint32  targetIp, 
        uint16  targetPort,
	nat_pkt_direction_e dir
      );

      /**
        @brief Adds a new Rule in the ALG Table.
               Overloads TranslationTable Add.
 
        If the operation fails, status1 is populated with the failure
        reason. Possible values are
            @li RULE_REG_EXISTS     Rule requested to be added already
                                    exists
            @li RULE_REG_NO_MEM     No memory left for Rule addtion


        @param[in]      protocol        Transport layer protocol value.
        @param[in]      srcPort         Source transport port number.
        @param[in]      dstPort         Destination transport port number.
        @param[in]      dir             Direction of packet (inbound/
                                        outbound/dont_care).
        @param[in]      cb              Rule hit cback to be called when Rule
                                        is matched.	  
        @param[in]      userData        User specified data to be passed with cb.
        @param[in]      status          errno value set when operation is unsuccessful.	  
        @param[out]     retEntryPtr     Ptr to added Table entry.
          
        @retval  0            Operation successful
        @retval  -1           Operation failed
      */

      int32 Add 
      (
        uint8               protocol, 
        uint16              srcPort,
        uint16              dstPort,
        nat_pkt_direction_e dir,
        nat_alg_read_cb_fn  cb,
	uint32              userData,
	int16              *status,
        uint32             *ret_entry_ptr
      );

      /**
        @brief Looks up a Rule in the ALG Table.
               Overloads TranslationTable Lookup.

        @param[in]      protocol        Transport layer protocol value.
        @param[in]      srcPort         Source transport port number.
        @param[in]      dstPort         Destination transport port number.
        @param[in]      dir             Direction of packet (inbound/outbound/
	                                  dont_care).
      
        @retval  0            Operation successful
        @retval  -1           Operation failed
      */

      uint32 Lookup 
      (
        uint8 protocol, 
        uint16 sourcePort,
        uint16 destPort,
        nat_pkt_direction_e dir
      );

      int32 Delete
      (
        void *entry
      ) throw();

      void * operator new
      (
        unsigned int sizeToAlloc
      ) throw();

      void operator delete
      (
        void * buf
      ) throw();

      virtual ~AlgTable
      (
        void
      );

      AlgTable
      (
        int32 maxNumEntries
      );

      /**
        @brief Sets ALG -> NAT association.
               One ALG entry can be associated to multiple NAT entries.

        @param[in]      natHandle       Handle to NAT entry which is to be 
                                        associated.
        @param[in]      port            NAT global port number.
        @param[in]      entry           Handle to ALG entry to be associated.
      
        @retval  0            Operation successful
        @retval  -1           Operation failed
      */

      int32 SetNatAssoc
      (
        uint32 natHandle,
        uint16 port,
        uint32 entry
      );

      /**
        @brief Gets ALG -> NAT association.
               This association brings down the lookup time for ALG packet
               transmissions ALG client -> ALG manager -> NAT iface Tx
               (WAN/LAN).

        @param[in]      port            NAT global port number.
        @param[in]      entry           Handle to ALG entry to be associated.
      
        @retval  "+ve value"  Operation successful, entry to NAT returned
        @retval  0            Operation failed
      */
      
      uint32 GetNatAssoc
      (
        uint16 port,
        uint32 entry
      );

      /**
        @brief Invokes Rule Cback associated with ALG entry.

        @param[in]      entry           Handle to ALG entry for which Rule matched.
        @param[in]      pktPtr          Ptr to packet which matched the Rule.
      
        @retval  "+ve value"  Operation successful, entry to NAT returned
        @retval  0            Operation failed
      */

      int InvokeCback
      (
        uint32           entry,
        dsm_item_type ** pktPtr
      );

      void Cleanup
      (
        void
      );

      int32 ClearNatAssoc
      (
        uint32 entry,
        uint16 port
      );


    private:

      algEntryType *GetNextFreeEntry
      (
        void
      );
      
      /* Header ptr for the hash bucket */
      algEntryType    *hashHeadPtr[ALG_HASH_SIZE]; 

      /* Free NAT entry header */
      algEntryType    *freeEntryHeadPtr;

      /* Free NAT entry pool allocated from heap */
      algEntryType    *freeEntryPoolPtr;
    };
  }

} //namespace PSNatRule

#endif /*FEATURE_DATA_PS_SOFTAP*/
#endif /* PS_ALG_RULE_H */
