#ifndef ACDB_LINKED_LIST_H
#define ACDB_LINKED_LIST_H
/*==============================================================================

FILE:      acdb_linked_list.h

DESCRIPTION: Functions and definitions to access the ACDB data structure.

PUBLIC CLASSES:  Not Applicable

INITIALIZATION AND SEQUENCING REQUIREMENTS:  N/A

        Copyright (c) 2010-2013 Qualcomm Technologies, Inc.
               All Rights Reserved.
            Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/
/* $Header: //source/qcom/qct/multimedia2/Audio/audcal4/acdb_sw/main/latest/acdb/src/acdb_linked_list.h#1 $ */
/*===========================================================================

EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.

when       who     what, where, why
--------   ---     ----------------------------------------------------------
07/23/10   ernanl  Introduce new heap optimization APIs
07/06/10   ernanl  Initial revision.

===========================================================================*/

/* ---------------------------------------------------------------------------
 * Include Files
 *--------------------------------------------------------------------------- */
#include "acdb_os_includes.h"

/* ---------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *--------------------------------------------------------------------------- */

#define ACDB_HEAP_FREE_NODE                  0x00000006
#define ACDB_HEAP_NOTFREE_NODE               0x00000007

/*------------------------------------------------------------------------------
Target specific definitions
------------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------
 * Type Declarations
 *--------------------------------------------------------------------------- */

/**
   @struct   AcdbDataLookupKeyType
   @brief The lookup key structure

   @param nLookupKey: An intermediate lookup key

   This structure provides an intermediate lookup key that is used
   to simplify looking up data between several calls to the data base.
*/
typedef struct _AcdbDataLookupKeyType {
   uint32_t nTableId;
   uint32_t nLookupKey;
} AcdbDataLookupKeyType;

/**
   @struct   AcdbDataUniqueDataNodeType
   @brief   This structure contains a pointer and length to a unique data entry.

   @param pUniqueData: A pointer to the unique data entry.
   @param ulDataLen: The size of the unique data in bytes.

   This structure contains a pointer and length to a unique data entry.
*/
typedef struct _AcdbDataUniqueDataNodeType{
   const uint8_t *pUniqueData;
   const uint32_t ulDataLen;
} AcdbDataUniqueDataNodeType;

/**
   @struct   AcdbDataTopologyType
   @brief Topology element description structure

   @param ulModuleId: The module id
   @param ulParamId: The parameter id
   @param ulMaxParamLen: The maximum length for the data of this node.

   The topology element description structure is used in parallel to the
   Calibration table to provide metadata for each table entry. This is
   separate from the table because it's common information and would add
   24 to 32 bytes for every calibration entry in the database (this is
   can cause the database to become very large!).
*/
typedef struct _AcdbDataTopologyType {
   const uint32_t ulModuleId;
   const uint32_t ulParamId;
   const uint32_t ulMaxParamLen;
} AcdbDataTopologyType;

/**
   @struct   AcdbDataGeneralInfoType
   @brief Response structure containing a byte buffer to be translated by
          the specific command it is used by.

   @param nBufferLength: The number of bytes provided in the buffer.
   @param pBuffer: A pointer to a buffer containing a payload.

   Response structure containing a byte buffer to be translated by the
   specific command it is used by. Please refer to the specific command
   for more information on the format of the payload.
*/
typedef struct _AcdbDataGeneralInfoType {
   uint32_t nBufferLength;
   uint8_t* pBuffer;
} AcdbDataGeneralInfoType;

//////////////////////////////////////////////////////
typedef struct AcdbDynamicUniqueDatastruct{
   uint32_t refcount;
   uint32_t ulParamId;
   uint8_t *ulDataBuf;
   uint32_t ulDataLen;
   struct AcdbDynamicUniqueDatastruct *pNext;
}AcdbDynamicUniqueDataType;

typedef struct AcdbDynamicDataNodeStruct{
   AcdbDynamicUniqueDataType *pDatalHead;
   AcdbDynamicUniqueDataType *pDatalTail;
}AcdbDynamicDataNodeType;

typedef struct AcdbParamIDStruct{
	uint32_t ulParamId;
	struct AcdbParamIDStruct *pNext;
}AcdbParamIDType;

typedef struct AcdbDynamicTopologyStruct{
   uint32_t ulModuleId;
   AcdbDynamicUniqueDataType *pDataNode;
   struct AcdbDynamicTopologyStruct *pNext;
}AcdbDynamicTopologyType;

typedef struct AcdbDynamicTopologyNodeStruct{
   AcdbDynamicTopologyType *pTopHead;
   AcdbDynamicTopologyType *pTopTail;
}AcdbDynamicTopologyNodeType;

typedef struct AcdbDynamicTblStruct{
   AcdbDataLookupKeyType *pKey;
   struct AcdbDynamicTopologyNodeStruct *pTopologyNode;
   struct AcdbDynamicTblStruct *pNext;
}AcdbDynamicTblType;

typedef struct AcdbDynamicTblNodeStruct{
   AcdbDynamicTblType *pTblHead;
   AcdbDynamicTblType *pTblTail;
}AcdbDynamicTblNodeType;

typedef struct AcdbDynamicAdieTblStruct{
   AcdbDataLookupKeyType *pKey;
   AcdbDynamicUniqueDataType *pDataNode;
   struct AcdbDynamicAdieTblStruct *pNext;
}AcdbDynamicAdieTblType;

typedef struct AcdbDynamicAdieTblNodeStruct{
   AcdbDynamicAdieTblType *pAdieTblHead;
   AcdbDynamicAdieTblType *pAdieTblTail;
}AcdbDynamicAdieTblNodeType;

/* ---------------------------------------------------------------------------
 * Function Declarations and Documentation
 *--------------------------------------------------------------------------- */

int32_t IsDataNodeOnHeap(uint32_t *pParamId,
                         AcdbDynamicDataNodeType *pUniqDataOnHeap
                         );

int32_t IsDataOnHeap(uint32_t *ulParamId,
                     uint8_t *pUniqueData,
                     const uint32_t ulDataLen,
                     AcdbDynamicDataNodeType *pUniqDataOnHeap
                     );

int32_t FindTableNodeOnHeap(AcdbDataLookupKeyType *pKey,
                            AcdbDynamicTblNodeType *pTblOnHeap,
                            AcdbDynamicTblType **ppTblNode
                            );

int32_t FindTopologyNodeOnHeap(uint32_t *pModuleId,
                               uint32_t *pParamId,
                               AcdbDynamicTblType *pTblOnHeap,
                               AcdbDynamicTopologyType **ppTblNode
                               );

int32_t FindDataNodeOnHeap(uint32_t *pParamId,
                           uint8_t *pInData,
                           const uint32_t InDataLen,
                           AcdbDynamicDataNodeType *pUniqDataOnHeap,
                           AcdbDynamicUniqueDataType **ppDataNode
                           );

int32_t CreateDataNodeOnHeap(uint32_t *pParamId,
                             uint8_t *pInData,
                             const uint32_t InDataLen,
                             AcdbDynamicDataNodeType *pUniqDataNode,
                             AcdbDynamicUniqueDataType **pDataNode
                             );

int32_t CreateTableNodeOnHeap(AcdbDataLookupKeyType *pKey,
                              AcdbDynamicTblNodeType *pTblOnHeap,
                              AcdbDynamicTblType **pTblNode
                              );

int32_t CreateTopologyNodeOnHeap(uint32_t *pModuleId,
                                 uint32_t *pParamId,
                                 AcdbDynamicUniqueDataType *pDataNode,
                                 AcdbDynamicTblType *pTbNodelOnHeap
                                 );

int32_t FreeTopologyNode(uint32_t *pModuleId,
                         uint32_t *pParamId,
                         AcdbDynamicTblType *pTblNode,
                         uint32_t *fReeTblResult
                         );

int32_t FreeTableNode(AcdbDataLookupKeyType *pKey,
                      AcdbDynamicTblNodeType *pTblOnHeap
                      );

int32_t FreeDataNode(uint32_t *pParamId,
                     AcdbDynamicDataNodeType *pDataOnHeap
                     );

int32_t CompareStaticData(uint32_t *pModuleId,
                          uint32_t *pParamId,
                          AcdbDataUniqueDataNodeType **ppCalTbl,
                          AcdbDataTopologyType *pTopology,
                          const uint32_t nTopologyEntries,
                          uint8_t *pInBufPtr,
                          const uint32_t InBufLen
                          );

int32_t GetDataCal(AcdbDataLookupKeyType *pKey,
                   uint32_t *pModuleId,
                   uint32_t *pParamId,
                   AcdbDynamicTblNodeType *pTbl,
                   AcdbDynamicUniqueDataType **ppDataNode
                   );

int32_t IsInfoDataOnHeap(AcdbDataGeneralInfoType* pInput,
                         AcdbDataGeneralInfoType* pInfoData
                         );

int32_t GetInfoDataNodeOnHeap(AcdbDataGeneralInfoType* pInput,
                              AcdbDataGeneralInfoType* pInfoData
                              );

int32_t CreateInfoDataNodeOnHeap(AcdbDataGeneralInfoType* pInput,
                                 AcdbDataGeneralInfoType** ppInfoData
                                 );

int32_t FreeInfoDataNodeOnHeap(AcdbDataGeneralInfoType** ppInfoData
                               );

int32_t FindAdieTableNodeOnHeap(AcdbDataLookupKeyType *pKey,
                                AcdbDynamicAdieTblNodeType *pTblOnHeap,
                                AcdbDynamicAdieTblType **ppTblNode
                                );

int32_t FreeAdieTableNode(AcdbDataLookupKeyType *pKey,
                          AcdbDynamicAdieTblNodeType *pTblOnHeap
                         );

int32_t CreateAdieTableNodeOnHeap(AcdbDataLookupKeyType *pKey,
                                  AcdbDynamicUniqueDataType *pDataNode,
                                  AcdbDynamicAdieTblNodeType *pTblOnHeap
                                  );

int32_t GetDataNodeOnHeap(uint32_t *pParamId,
                         AcdbDynamicDataNodeType *pUniqDataOnHeap,
						 AcdbDynamicUniqueDataType **ppDataNode
                         );

#endif//ACDB_LINKED_LIST_H
