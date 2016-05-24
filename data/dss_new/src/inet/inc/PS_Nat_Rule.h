#ifndef PS_NAT_RULE_H
#define PS_NAT_RULE_H
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

  $Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/inc/PS_Nat_Rule.h#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "target.h"

#ifdef FEATURE_DATA_PS_SOFTAP

#ifdef __cplusplus
#include "PS_Rule_Table.h"
#include "ps_nat.h"
#include "dsm.h"

namespace PS
{
  namespace RuleTable
  {
    class NatTable:public TranslationTable
    {
      typedef struct natEntry
      {
        nat_state_e                   state;
        uint8                         protocol; /* TCP/UDP/ICMP */

        /* Public IP address in network order */
        uint32                        globalIpAddr;     
        /* Priavte IP address in network order */      
        uint32                        privateIpAddr; 
        /* Public Port in network order */
        uint16                        globalPort;        
        /* Private Port in network order */          
        uint16                        privatePort;

        /* Peer IP address in network order */
        uint32                        targetIpAddr;
        /* Peer Port in network order */
        uint16                        targetPort;

        /* Hash Key targetIp ^ targetPort ^ globalIp */
        uint32                        hashKey;

        /* Pointer to the next NAT entry belong to the same HASH key */
        struct natEntry             * nextEntryPtr;  

        /* Pointer to the previous NAT entry belong to the same HASH key */
        struct natEntry             * prevEntryPtr;  

        /* Pointer to the next free NAT entry */
        struct natEntry             * nextFreeEntryPtr;  

        /* Config table entry timeout in sec */
        uint16                        entryTimeout;
        /* Time remaining on table entry timeout in sec */
        uint16                        entryTimeoutLeft;

        /* associated in/outbound ALG handles */
        uint32                        inAlgHandle;
        uint32                        outAlgHandle;

        /* associated DMZ handle*/
        uint32                        dmzHandle;
      } natEntryType;
    	    
      public:
        /**
          @brief Creates a NAT Table instance.

          @param[in]      maxNumEntries     Maximum number of entries ALG Table can hold.
          
          @retval  0              Operation failed
          @retval  "handle > 0"   Operation successful, returns handle to NAT Table        
        */
        static NatTable * CreateInstance
        (
          int32 maxNumEntries
        );

        virtual ~NatTable
        (
          void
        ) throw();

        void * operator new
        (
          unsigned int sizeToAlloc
        ) throw();

        void operator delete
        (
          void * buf
        ) throw();

        NatTable
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

        int32 Delete
        (
          void *entry
        ) throw();

        /**
          @brief Deletes ALG -> NAT association.
                 ALG maintains NAT association for maintaining translational
                 lookups. If a dynamic NAT entry times out, it needs to be 
                 deleted at ALG level too. 
  
          @param[in]      natEntry        Handle to NAT entry for which ALG 
                                          association is to be deleted.
          @param[in]      algEntry        Handle to ALG entry.
        
          @retval  0            Operation successful
          @retval  -1           Operation failed
        */

        static int32 DeleteAlgAssoc
        (
          uint32 natEntry,
          uint32 algEntry
        );

        /**
          @brief Processes NAT entry timeouts.
                 Goes through all NAT entries decreasing their leftover times.

          @param[in]      interval        unit of timeout to be decremented.
        
          @retval  none
        */
        void ProcessTimeout
        (
          uint16 interval
        );

        uint32 GetNumStaticEntries
        (
          void
        );	 

        uint32 GetNumEntries
        (
          void
        );	 

        int32 GetNextStaticEntry
        (
          uint8  *protocol,
          uint32 *privateIp,
          uint16 *privatePort,
          uint16 *globalPort 
        );

        void SetEntryTimeout
        (
          uint32 entry,
          uint16 timeoutVal
        );

        void GetEntryVals
        (
          uint32   entry,
          uint32  *privateIpAddr,
          uint16  *privatePort,
          uint32  *globalIpAddr,
          uint16  *globalPort,
          uint32  *targetIpAddr,
          uint16  *targetPort, 
          uint16  *timeoutVal 
        );

        void CleanUp
        (
          void
        );

        /**
          @brief Invokes the ALG entry corresponding to a dynamic NAT entry.
                 If an ALG handle was associated with this NAT entry, an
                 ALG client would be consuming this in the end. 
  
          @param[in]      entry           Handle to NAT entry for which ALG 
                                          association is to be invoked.
          @param[in]      pktPtr          Ptr to packet which matched Rule.
          @param[in]      dir             Packet direction when this was 
                                          invoked (in/out/dont_care).
        
          @retval  0            Operation successful
          @retval  -1           Operation failed
        */
        int32 NatInvokeAlg
        (
          uint32           entry,
          dsm_item_type ** pktPtr,
          nat_pkt_direction_e  dir
        );

        int32 SetNatDMZAssoc
        (
          uint32 natEntry,
          uint32 dmzEntry
        );

#ifdef TEST_FRAMEWORK
       uint32 GetHashHeadEntry
       (
         uint32 hash
       );

       uint32 GetHashTailEntry
       (
         uint32 hash
       );

       uint32 GetNextEntry
       (
         uint32 entry
       );

       uint32 GetPrevEntry
       (
         uint32 entry
       );

       uint32 GetNextFreeEntry
       (
         uint32 entry
       );

       uint32 GetFreeEntryPoolPtr
       (
         void
       );

       uint32 GetFreeEntryHeadPtr
       (
         void
       );
#endif /*TEST_FRAMEWORK*/
      private:

        natEntryType *GetNextFreeEntry
        (
          void
        );

        uint32 GetHashKey
        (
          uint32 targetIp,
          uint16 targetPort,
          uint32 globalIp
        );

        /* Header ptr for the hash bucket */
        natEntryType    *hashHeadPtr[NAT_HASH_SIZE]; 

        /* Tail ptr for the hash bucket */
        natEntryType    *hashTailPtr[NAT_HASH_SIZE]; 

        /* Total number of static entries in the table */
        uint32          totalNumStaticEntries;
        /* Last IOCTL GET static entry ptr */
        natEntryType    *lastQueryStaticEntryPtr;

        /* Free NAT entry header */
        natEntryType    *freeEntryHeadPtr;
  
        /* Free NAT entry pool allocated from heap */
        natEntryType    *freeEntryPoolPtr;
    };
  }
} //namespace PSNatRule


#else //C++

void NatCleanup
(
  void    *natTableHandle
);

void ProcessTimeout
(
  uint1    timeoutVal,
  void    *natTableHandle
);

#endif // C++

#endif /* FEATURE_DATA_PS_SOFTAP */
#endif /* PS_NAT_RULE_H */
