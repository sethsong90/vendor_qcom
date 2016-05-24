/*===========================================================================
    FILE:           acdb_override.c

    OVERVIEW:       This file contains the implementaion of the heap optimization
                    API and functions
    DEPENDENCIES:   None

                    Copyright (c) 2010-2013 Qualcomm Technologies, Inc.
                    All Rights Reserved.
                    Qualcomm Technologies Proprietary and Confidential.
========================================================================== */

/*===========================================================================
    EDIT HISTORY FOR MODULE

    This section contains comments describing changes made to the module.
    Notice that changes are listed in reverse chronological order. Please
    use ISO format for dates.

    $Header: //source/qcom/qct/multimedia2/Audio/audcal4/acdb_sw/main/latest/acdb/src/acdb_override.c#2 $

    when        who     what, where, why
    ----------  ---     -----------------------------------------------------
    2013-06-07  avi     Support Voice Volume boost feature
    2010-07-23  ernanl  Initial implementation of the Acdb_DM_Ioctl API and
                        associated helper methods.
========================================================================== */

#include "acdb.h"
#include "acdb_private.h"
#include "acdb_override.h"
#include "acdb_linked_list.h"
#include "acdb_datainfo.h"
/* ---------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Type Declarations
 *--------------------------------------------------------------------------- */

#define UNREFERENCED_VAR(param)

/* ---------------------------------------------------------------------------
 * Global Data Definitions
 *--------------------------------------------------------------------------- */
static AcdbDynamicTblNodeType *g_pTbl = NULL;
static AcdbDynamicDataNodeType *g_pData = NULL;
static AcdbDynamicAdieTblNodeType *g_pAdieTbl = NULL;

/* ---------------------------------------------------------------------------
 * Static Variable Definitions
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Static Function Declarations and Definitions
 *--------------------------------------------------------------------------- */

int32_t Acdb_SetDataCal(AcdbDataLookupKeyType *pKey,
                        uint32_t *pModuleId,
                        uint32_t *pParamId,
						const uint8_t *pFileDataBuf,
						const uint32_t nFileDataLen,
                        uint8_t *pInputBufPtr,
                        uint32_t InputBufLen
                        )
{
   int32_t result = ACDB_SUCCESS;

   if (pKey != NULL && pModuleId != NULL && pParamId != NULL
       && pInputBufPtr != NULL && InputBufLen != 0
       && pFileDataBuf != NULL ) //nFileDataLen can be Zero for VP3
   {
	   if( (InputBufLen != nFileDataLen ) ||
		   (memcmp(pInputBufPtr,pFileDataBuf,nFileDataLen)))
	   {
		   result = ACDB_PARMNOTFOUND;
	   }
      if (result == ACDB_SUCCESS)
      {//If data node exist on default,check table node on heap
         AcdbDynamicTblType *pTblNode = NULL;
         result = FindTableNodeOnHeap((AcdbDataLookupKeyType*) pKey,
                                      (AcdbDynamicTblNodeType*) g_pTbl,
                                      (AcdbDynamicTblType**) &pTblNode
                                      );
         if (result == ACDB_SUCCESS)
         {
            AcdbDynamicTopologyType *pTopNode = NULL;
            result = FindTopologyNodeOnHeap((uint32_t*) pModuleId,
                                            (uint32_t*) pParamId,
                                            (AcdbDynamicTblType*) pTblNode,
                                            (AcdbDynamicTopologyType**) &pTopNode
                                            );
            if (result == ACDB_SUCCESS)
            {//Free Topology Node
               uint32_t fReeTblResult = ACDB_HEAP_NOTFREE_NODE;
               result = FreeTopologyNode((uint32_t*) pModuleId,
                                         (uint32_t*) pParamId,
                                         (AcdbDynamicTblType*) pTblNode,
                                         (uint32_t*) &fReeTblResult
                                         );
               if (fReeTblResult == ACDB_HEAP_FREE_NODE)
               {//Free Table Node if topology node no longer exist on table node
                  result = FreeTableNode((AcdbDataLookupKeyType*) pKey,
                                         (AcdbDynamicTblNodeType*) g_pTbl
                                         );
               }
            }
            if (result == ACDB_SUCCESS)
            {
               result = FreeDataNode((uint32_t*) pParamId,
                                     (AcdbDynamicDataNodeType*) g_pData
                                     );
            }
         }
         if (result == ACDB_PARMNOTFOUND)
         {
            result = ACDB_SUCCESS;
         }
      }
      else if (result == ACDB_PARMNOTFOUND)
      {//Data not member of static data
         AcdbDynamicUniqueDataType* pDataNode = NULL;
         uint32_t dataType = ACDB_HEAP_DATA_FOUND;

         result = IsDataOnHeap((uint32_t*) pParamId,
                               (uint8_t *) pInputBufPtr,
                               (uint32_t) InputBufLen,
                               (AcdbDynamicDataNodeType*) g_pData
                               );
         if (result == ACDB_PARMNOTFOUND)
         {
            result = CreateDataNodeOnHeap((uint32_t*) pParamId,
                                          (uint8_t *) pInputBufPtr,
                                          (uint32_t) InputBufLen,
                                          (AcdbDynamicDataNodeType*) g_pData,
                                          (AcdbDynamicUniqueDataType**) &pDataNode
                                          );
            dataType = ACDB_HEAP_DATA_NOT_FOUND;
         }//Data Node not found, create data node and return its address
         else if (result == ACDB_SUCCESS)
         {
            result = FindDataNodeOnHeap((uint32_t*) pParamId,
                                        (uint8_t *) pInputBufPtr,
                                        (uint32_t) InputBufLen,
                                        (AcdbDynamicDataNodeType*) g_pData,
                                        (AcdbDynamicUniqueDataType**) &pDataNode
                                        );
         }//if Data node found, find data node ptr address
         if (result == ACDB_SUCCESS)
         {
            AcdbDynamicTblType *pTblNode = NULL;
            result = FindTableNodeOnHeap((AcdbDataLookupKeyType*) pKey,
                                       (AcdbDynamicTblNodeType*) g_pTbl,
                                       (AcdbDynamicTblType**) &pTblNode
                                       );
            if (result == ACDB_PARMNOTFOUND)
            {
               result = CreateTableNodeOnHeap((AcdbDataLookupKeyType*) pKey,
                                              (AcdbDynamicTblNodeType*) g_pTbl,
                                              (AcdbDynamicTblType**) &pTblNode
                                              );
            }//table not created, create table node.
            if (result == ACDB_SUCCESS)
            {
               AcdbDynamicTopologyType *pTopNode = NULL;
               result = FindTopologyNodeOnHeap((uint32_t*) pModuleId,
                                               (uint32_t*) pParamId,
                                               (AcdbDynamicTblType*) pTblNode,
                                               (AcdbDynamicTopologyType**) &pTopNode
                                               );
               if (result == ACDB_SUCCESS)
               {
                  if (dataType == ACDB_HEAP_DATA_NOT_FOUND || pTopNode->pDataNode != pDataNode)
                  {
                     //Condition to decrement refcount
                     //1. check if found data node is different than what has already stored in the organization
                     //2. a new data node was created
                     //Decrease reference count from previous data node
                     pTopNode->pDataNode->refcount--;
                     //if data node reference = 0, free the data node
                     if (pTopNode->pDataNode->refcount == 0)
                     {
                        result = FreeDataNode((uint32_t*) pParamId,
                                              (AcdbDynamicDataNodeType*) g_pData
                                              );
                     }
                     //Link to new added data node
                     pTopNode->pDataNode = pDataNode;
                     pTopNode->pDataNode->refcount++;
                  }
               }
               else if (result == ACDB_PARMNOTFOUND)
               {
                  result = CreateTopologyNodeOnHeap((uint32_t*) pModuleId,
                                                    (uint32_t*) pParamId,
                                                    (AcdbDynamicUniqueDataType*) pDataNode,
                                                    (AcdbDynamicTblType*) pTblNode
                                                    );
               }//Create Topology node
            }//Create Table node
         }//Create Data node
      }
   }
   return result;
}

int32_t Acdb_GetDataCal(AcdbDataLookupKeyType *pKey,
                        uint32_t *pModuleId,
                        uint32_t *pParamId,
                        AcdbDynamicUniqueDataType **ppDataNode
                        )
{
   int32_t result = ACDB_BADPARM;

   if (pKey != NULL && pModuleId != NULL && pParamId && ppDataNode != NULL)
   {
      AcdbDynamicTblType *pTblNode = NULL;
      result = FindTableNodeOnHeap((AcdbDataLookupKeyType*) pKey,
                                   (AcdbDynamicTblNodeType*) g_pTbl,
                                   (AcdbDynamicTblType**) &pTblNode
                                   );
      if (result == ACDB_SUCCESS)
      {
         AcdbDynamicTopologyType *pTopNode = NULL;
         result = FindTopologyNodeOnHeap((uint32_t*) pModuleId,
                                         (uint32_t*) pParamId,
                                         (AcdbDynamicTblType*) pTblNode,
                                         (AcdbDynamicTopologyType**) &pTopNode
                                         );
         if (result == ACDB_SUCCESS)
         {
            *ppDataNode = pTopNode->pDataNode;
         }
      }
   }
   else
   {
      ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_GetDataCal]->NULL Input pointer");
   }
   return result;
}

int32_t Acdb_sys_reset(void)
{
   int32_t result = ACDB_SUCCESS;

   //Free general table on Heap
   if (g_pTbl != NULL)
   {
      AcdbDynamicTblType *pCur;
      pCur = g_pTbl->pTblHead;
      while (pCur != NULL)
      {
         AcdbDynamicTopologyType *pCurTop;
         if (pCur->pTopologyNode != NULL)
         {
            pCurTop = pCur->pTopologyNode->pTopHead;
            while (pCurTop)
            {
               pCur->pTopologyNode->pTopHead = pCurTop->pNext;
               ACDB_MEM_FREE(pCurTop);
               pCurTop = pCur->pTopologyNode->pTopHead;
            }
            if (pCurTop != NULL)
            {
               ACDB_MEM_FREE(pCur->pTopologyNode->pTopHead);
            }
         }
         g_pTbl->pTblHead = pCur->pNext;
         ACDB_MEM_FREE(pCur->pKey);
         ACDB_MEM_FREE(pCur->pTopologyNode);
         ACDB_MEM_FREE(pCur);
         pCur = g_pTbl->pTblHead;
         if (pCur == NULL)
         {
            g_pTbl->pTblTail = NULL;
         }
      }
      ACDB_MEM_FREE(g_pTbl);
      g_pTbl = NULL;
   }

   //Free Data on Heap
   if (g_pData != NULL)
   {
      AcdbDynamicUniqueDataType *pCurData = g_pData->pDatalHead;

      while (pCurData)
      {
         g_pData->pDatalHead = pCurData->pNext;
         ACDB_MEM_FREE(pCurData->ulDataBuf);
         ACDB_MEM_FREE(pCurData);
         pCurData = g_pData->pDatalHead;
      }
      if (pCurData != NULL)
      {
         ACDB_MEM_FREE(g_pData->pDatalHead);
      }
      ACDB_MEM_FREE(g_pData);
      g_pData = NULL;
   }

   //Free General Info on Heap

   //Free Adie Table on Heap
   if (g_pAdieTbl != NULL)
   {
      AcdbDynamicAdieTblType *pCurAdieTbl;
      pCurAdieTbl = g_pAdieTbl->pAdieTblHead;
      while (pCurAdieTbl)
      {
         g_pAdieTbl->pAdieTblHead = pCurAdieTbl->pNext;
         ACDB_MEM_FREE(pCurAdieTbl->pKey);
         ACDB_MEM_FREE(pCurAdieTbl);
         pCurAdieTbl = g_pAdieTbl->pAdieTblHead;
      }
      if (pCurAdieTbl != NULL)
      {
         ACDB_MEM_FREE(g_pAdieTbl->pAdieTblHead);
      }
      ACDB_MEM_FREE(g_pAdieTbl);
      g_pAdieTbl = NULL;
   }

   return result;
}

/* ---------------------------------------------------------------------------
 * Externalized Function Declarations and Definitions
 *--------------------------------------------------------------------------- */

int32_t Acdb_ChecktoFreeAdieTableCalOnHeap(AcdbDataLookupKeyType *pKey,
                                           uint8_t *pInputBufPtr,
                                           const uint32_t InputBufLen
                                           )
{
   int32_t result = ACDB_SUCCESS;

   if (pKey != NULL && pInputBufPtr != NULL && InputBufLen != 0)
   {
      if (g_pAdieTbl != NULL)
      {
         AcdbDynamicAdieTblType *pTblNode = NULL;
         result = FindAdieTableNodeOnHeap((AcdbDataLookupKeyType*) pKey,
                                          (AcdbDynamicAdieTblNodeType*) g_pAdieTbl,
                                          (AcdbDynamicAdieTblType**) &pTblNode
                                          );
         if (result == ACDB_SUCCESS)
         {
            result = FreeAdieTableNode((AcdbDataLookupKeyType*) pKey,
                                       (AcdbDynamicAdieTblNodeType*) g_pAdieTbl
                                       );
            if (result == ACDB_SUCCESS)
            {
               result = FreeDataNode((uint32_t*) &pKey->nLookupKey,
                                     (AcdbDynamicDataNodeType*) g_pData
                                     );
            }
         }
         if (result == ACDB_PARMNOTFOUND)
         {
            result = ACDB_SUCCESS;
         }
      }
   }
   else
   {
      result = ACDB_BADPARM;

      ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_ChecktoFreeAdieTableCalOnHeap]->NULL Input pointer");
   }//Null input

   return result;
}

int32_t Acdb_SetAdieTableCal(AcdbDataLookupKeyType *pKey,
                             uint8_t *pInputBufPtr,
                             const uint32_t InputBufLen
                             )
{
   int32_t result = ACDB_BADPARM;

   if (pKey != NULL && pInputBufPtr != NULL && InputBufLen != 0)
   {
      AcdbDynamicUniqueDataType *pDataNode = NULL;
      AcdbDynamicAdieTblType *pTblNode = NULL;

      uint32_t dataType = ACDB_HEAP_DATA_FOUND;

      result = IsDataOnHeap((uint32_t*) &pKey->nLookupKey,
                            (uint8_t*) pInputBufPtr,
                            (uint32_t) InputBufLen,
                            (AcdbDynamicDataNodeType*) g_pData
                            ); //check if data is on heap
      if (result == ACDB_PARMNOTFOUND)
      {
         result = CreateDataNodeOnHeap((uint32_t*) &pKey->nLookupKey,
                                       (uint8_t*) pInputBufPtr,
                                       (uint32_t) InputBufLen,
                                       (AcdbDynamicDataNodeType*) g_pData,
                                       (AcdbDynamicUniqueDataType**) &pDataNode
                                       ); //if not on heap, create data node
         dataType = ACDB_HEAP_DATA_NOT_FOUND;
      }
      else if (result == ACDB_SUCCESS)
      {
         result = FindDataNodeOnHeap((uint32_t*) &pKey->nLookupKey,
                                     (uint8_t*) pInputBufPtr,
                                     (uint32_t) InputBufLen,
                                     (AcdbDynamicDataNodeType*) g_pData,
                                     (AcdbDynamicUniqueDataType**) &pDataNode
                                      ); //if yes, find the data node
      }
      result = FindAdieTableNodeOnHeap((AcdbDataLookupKeyType*) pKey,
                                       (AcdbDynamicAdieTblNodeType*) g_pAdieTbl,
                                       (AcdbDynamicAdieTblType**) &pTblNode
                                       ); // check if volume table node is on heap
      if (result == ACDB_SUCCESS)
      {
         //Both pDataNode and PTblNode should be filled
         if (pDataNode != NULL && pTblNode != NULL)
         {
            if (dataType == ACDB_HEAP_DATA_NOT_FOUND || pDataNode != pTblNode->pDataNode)
            {
               //Condition to decrement refcount
               //1. check if found data node is different than what has already stored in the organization
               //2. a new data node was creatd
               //Decrease previous data node count
               pTblNode->pDataNode->refcount--;
               if (pTblNode->pDataNode->refcount == 0)
               {
                  result = FreeDataNode((uint32_t*) &pKey->nLookupKey,
                                        (AcdbDynamicDataNodeType*) g_pData
                                        );
               }
               //Assign new data node
               pTblNode->pDataNode = pDataNode;
               pTblNode->pDataNode->refcount++;
            }
         }
      }
      else if (result == ACDB_PARMNOTFOUND)
      {
         result = CreateAdieTableNodeOnHeap((AcdbDataLookupKeyType*) pKey,
                                            (AcdbDynamicUniqueDataType*) pDataNode,
                                            (AcdbDynamicAdieTblNodeType*) g_pAdieTbl
                                            ); //if not, create the volume table node
      }
   }
   else
   {
      ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_SetAdieTableCal]->NULL Input pointer");
   }//Null input

   return result;
}

int32_t Acdb_GetAdieTableCal(AcdbDataLookupKeyType *pKey,
                             AcdbDynamicUniqueDataType **ppDataNode
                             )
{
   int32_t result = ACDB_BADPARM;

   if (pKey != NULL && ppDataNode != NULL)
   {
      AcdbDynamicAdieTblType *pTblNode = NULL;

      result = IsDataNodeOnHeap((uint32_t*) &pKey->nLookupKey,
                                (AcdbDynamicDataNodeType*) g_pData
                                );

      if (result == ACDB_SUCCESS)
      {
         result = FindAdieTableNodeOnHeap((AcdbDataLookupKeyType*) pKey,
                                          (AcdbDynamicAdieTblNodeType*) g_pAdieTbl,
                                          (AcdbDynamicAdieTblType**) &pTblNode
                                          );
         if (result == ACDB_SUCCESS && pTblNode != NULL)
         {
            *ppDataNode = pTblNode->pDataNode;
         }
      }
   }
   else
   {
      ACDB_DEBUG_LOG("[ACDB Override ERROR]->[Acdb_GetAdieTableCal]->NULL Input pointer");
   }//Null input
   return result;
}

int32_t Acdb_GetNoOfTblEntries(uint8_t *pCmd,uint32_t nCmdSize ,uint8_t *pRsp,uint32_t nRspSizse)
{
	int32_t result = ACDB_SUCCESS;
	AcdbQueryNoOfTblEntriesCmdType *pInput = (AcdbQueryNoOfTblEntriesCmdType *)pCmd;
	AcdbRespNoOfTblEntriesCmdType *pOut = (AcdbRespNoOfTblEntriesCmdType *) pRsp;
	uint32_t nonModuleTbl = 0;

	if(pCmd == NULL || nCmdSize != sizeof(AcdbQueryNoOfTblEntriesCmdType) ||
		pRsp == NULL || nRspSizse != sizeof(AcdbRespNoOfTblEntriesCmdType))
	{
		ACDB_DEBUG_LOG("Invalid input params provided to retrieve the data");
		return ACDB_ERROR;
	}
	switch(pInput->nTblId)
	{
	   case AUDPROC_GAIN_INDP_TBL:
	   case AUDPROC_COPP_GAIN_DEP_TBL:
	   case AUDPROC_AUD_VOL_TBL:
	   case AUD_STREAM_TBL:
	   case VOCPROC_GAIN_INDP_TBL:
	   case VOCPROC_COPP_GAIN_DEP_TBL:
	   case VOC_STREAM_TBL:
	   case AFE_TBL:
	   case AFE_CMN_TBL:
	   case VOCPROC_DEV_CFG_TBL:
	   case LSM_TBL:
	   case ADIE_SIDETONE_TBL:
	   case AANC_CFG_TBL:
       case VOCPROC_COPP_GAIN_DEP_V2_TBL:
       case VOICE_VP3_TBL:
       case AUDIO_REC_VP3_TBL:
       case AUDIO_REC_EC_VP3_TBL:
		   nonModuleTbl = 0;
		   break;
	   case ADIE_ANC_TBL:
	   case ADIE_CODEC_TBL:
	   case GLOBAL_DATA_TBL:
	   case CDC_FEATURES_TBL:
		   nonModuleTbl = 1;
		   break;
	   default:
		ACDB_DEBUG_LOG("Provided invalid tableid");
        return ACDB_ERROR;
	}
	pOut->nNoOfEntries = 0;
	if(nonModuleTbl == 0)
	{
		AcdbDynamicTblType *pCur;

		if(g_pTbl == NULL || g_pTbl->pTblHead == NULL)
		{
			pOut->nNoOfEntries = 0;
			return ACDB_SUCCESS;
		}
		pCur = g_pTbl->pTblHead;
		while(pCur != NULL)
		{
			if(pCur->pKey->nTableId == pInput->nTblId)
			{
				AcdbDynamicTopologyType *pCurTop = pCur->pTopologyNode->pTopHead;
				while(pCurTop != NULL)
				{
					pOut->nNoOfEntries++;
					//AcdbDynamicUniqueDataType *pCurData = pCurTop->pDataNode;
					//while(pCurData != NULL)
					//{
					//	pOut->nNoOfEntries++;
					//	pCurData = pCurData->pNext;
					//}
					pCurTop = pCurTop->pNext;
				}
			}
			pCur = pCur->pNext;
		}
	}
	else
	{
		AcdbDynamicAdieTblType *pCur;
		if(g_pAdieTbl == NULL || g_pAdieTbl->pAdieTblHead == NULL)
		{
			pOut->nNoOfEntries = 0;
			return ACDB_SUCCESS;
		}
		pCur = g_pAdieTbl->pAdieTblHead;
		while(pCur != NULL)
		{
			if(pCur->pKey->nTableId == pInput->nTblId)
			{
				pOut->nNoOfEntries++;
				//AcdbDynamicUniqueDataType *pCurData = pCur->pDataNode;
				//while(pCurData != NULL)
				//{
				//
				//	pCurData = pCurData->pNext;
				//}
			}
			pCur = pCur->pNext;
		}
	}
	return result;
}

int32_t Acdb_GetTblEntries(uint8_t *pCmd,uint32_t nCmdSize ,uint8_t *pRsp,uint32_t nRspSizse)
{
	int32_t result = ACDB_SUCCESS;
	AcdbQueryTblEntriesCmdType *pInput = (AcdbQueryTblEntriesCmdType *)pCmd;
	AcdbQueryResponseType *pOut = (AcdbQueryResponseType *) pRsp;
	uint32_t nonModuleTbl = 0;
	uint32_t noOfTableIndices=0;
	uint32_t curNoOfEntriesOffset=0;
	uint32_t noOfEntriesCopied=0;
	//uint32_t noOfEntriesRemaining=0;
	//uint32_t noOfBytesRemaining=pInput->nBuffSize;
	uint32_t noOfBytesReqPerEntry=0;
	uint32_t offset = 0;
	uint8_t nMemExhausted = 0;
	//uint32_t nActualRemainingEntries=0;

	if(pCmd == NULL || nCmdSize != sizeof(AcdbQueryTblEntriesCmdType) ||
		pRsp == NULL || nRspSizse != sizeof(AcdbQueryResponseType) ||
		pInput->pBuff == NULL || pInput->nBuffSize < 4)
	{
		ACDB_DEBUG_LOG("Invalid input params provided to retrieve the data");
		return ACDB_ERROR;
	}
	switch(pInput->nTblId)
	{
	   case AUDPROC_GAIN_INDP_TBL:
		   noOfTableIndices = AUDPROCTBL_INDICES_COUNT;
		   break;
	   case AUDPROC_COPP_GAIN_DEP_TBL:
		   noOfTableIndices = AUDPROCTBL_GAIN_DEP_INDICES_COUNT;
		   break;
	   case AUDPROC_AUD_VOL_TBL:
		   noOfTableIndices = AUDPROCTBL_VOL_INDICES_COUNT;
		   break;
	   case AUD_STREAM_TBL:
		   noOfTableIndices = AUDSTREAMTBL_INDICES_COUNT;
		   break;
	   case VOCPROC_GAIN_INDP_TBL:
		   noOfTableIndices = VOCPROCTBL_INDICES_COUNT;
		   break;
	   case VOCPROC_COPP_GAIN_DEP_TBL:
		   noOfTableIndices = VOCPROCTBL_VOL_INDICES_COUNT;
		   break;
           case VOCPROC_COPP_GAIN_DEP_V2_TBL:
		   noOfTableIndices = VOCPROCTBL_VOL_V2_INDICES_COUNT;
		   break;
	   case VOC_STREAM_TBL:
		   noOfTableIndices = VOCSTREAMTBL_INDICES_COUNT;
		   break;
	   case AFE_TBL:
		   noOfTableIndices = AFETBL_INDICES_COUNT;
		   break;
	   case AFE_CMN_TBL:
		   noOfTableIndices = AFECMNTBL_INDICES_COUNT;
		   break;
	   case VOCPROC_DEV_CFG_TBL:
		   noOfTableIndices = VOCPROCDEVCFGTBL_INDICES_COUNT;
		   break;
	   case LSM_TBL:
		   noOfTableIndices = LSM_INDICES_COUNT;
		   break;
	   case ADIE_SIDETONE_TBL:
		   noOfTableIndices = ADST_INDICES_COUNT;
		   break;
	   case AANC_CFG_TBL:
		   noOfTableIndices = AANC_CFG_TBL_INDICES_COUNT;
		   break;
	   case ADIE_ANC_TBL:
		   noOfTableIndices = ANCTBL_INDICES_COUNT;
		   nonModuleTbl = 1;
		   break;
	   case ADIE_CODEC_TBL:
		   noOfTableIndices = ANCTBL_INDICES_COUNT;
		   nonModuleTbl = 1;
		   break;
	   case GLOBAL_DATA_TBL:
		   noOfTableIndices = GLOBALTBL_INDICES_COUNT;
		   nonModuleTbl = 1;
		   break;
	   case CDC_FEATURES_TBL:
		   noOfTableIndices = CDC_FEATURES_DATA_INDICES_COUNT;
		   nonModuleTbl = 1;
		   break;
       case VOICE_VP3_TBL:
           noOfTableIndices = VOICE_VP3TBL_INDICES_COUNT;
           break;
       case AUDIO_REC_VP3_TBL:
           noOfTableIndices = AUDREC_VP3TBL_INDICES_COUNT;
           break;
       case AUDIO_REC_EC_VP3_TBL:
           noOfTableIndices = AUDREC_EC_VP3TBL_INDICES_COUNT;
           break;
	   default:
		ACDB_DEBUG_LOG("Provided invalid tableid");
        return ACDB_ERROR;
	}
	offset = sizeof(noOfEntriesCopied) ;
	if(nonModuleTbl == 0)
	{
		AcdbDynamicTblType *pCur;
		noOfBytesReqPerEntry = ((noOfTableIndices+2) * sizeof(uint32_t)); //2 is added to include mid and pid as well

		if(g_pTbl == NULL || g_pTbl->pTblHead == NULL)
		{
			ACDB_DEBUG_LOG("No entries are there on heap\n");
			return ACDB_ERROR;
		}
		pCur = g_pTbl->pTblHead;
		while(pCur != NULL)
		{
			if(pCur->pKey->nTableId == pInput->nTblId)
			{
				AcdbDynamicTopologyType *pCurTop = pCur->pTopologyNode->pTopHead;
				while(pCurTop != NULL)
				{
					if( pInput->nTblEntriesOffset <= curNoOfEntriesOffset)
					{
						if(((pInput->nBuffSize-offset) >= noOfBytesReqPerEntry))
					{
						uint8_t* pLut = (uint8_t *)pCur->pKey->nLookupKey;
						ACDB_MEM_CPY(pInput->pBuff+offset,(noOfTableIndices*sizeof(uint32_t)),pLut,(noOfTableIndices*sizeof(uint32_t)));
						offset += (noOfTableIndices*sizeof(uint32_t));
						ACDB_MEM_CPY(pInput->pBuff+offset,sizeof(uint32_t),&pCurTop->ulModuleId,sizeof(uint32_t));
						offset += sizeof(uint32_t);
						ACDB_MEM_CPY(pInput->pBuff+offset,sizeof(uint32_t),&pCurTop->pDataNode->ulParamId,sizeof(uint32_t));
						offset += sizeof(uint32_t);
						++noOfEntriesCopied;
					}
						else
						{
							nMemExhausted = 1;
							break;
						}
					}
					pCurTop = pCurTop->pNext;
					++curNoOfEntriesOffset;
				}
			}
			if(nMemExhausted == 1)
				break;
			pCur = pCur->pNext;
		}
	}
	else
	{
		AcdbDynamicAdieTblType *pCur;
		noOfBytesReqPerEntry = (noOfTableIndices * sizeof(uint32_t)); //1 is added to include pid as well
		if(g_pAdieTbl == NULL || g_pAdieTbl->pAdieTblHead == NULL)
		{
			ACDB_DEBUG_LOG("No entries are there on heap\n");
			return ACDB_ERROR;
		}
		pCur = g_pAdieTbl->pAdieTblHead;
		while(pCur != NULL)
		{
			if(pCur->pKey->nTableId == pInput->nTblId)
			{
				//AcdbDynamicUniqueDataType *pCurData = pCur->pDataNode;
				//while(pCurData != NULL)
				//{
				//	pCurData = pCurData->pNext;
				//}
				if(pInput->nTblEntriesOffset <= curNoOfEntriesOffset)
				{
					if((pInput->nBuffSize-offset) >= noOfBytesReqPerEntry)
				{
					uint8_t* pLut = (uint8_t *)pCur->pKey->nLookupKey;
					ACDB_MEM_CPY(pInput->pBuff+offset,(noOfTableIndices*sizeof(uint32_t)),pLut,(noOfTableIndices*sizeof(uint32_t)));
					offset += (noOfTableIndices*sizeof(uint32_t));
					//ACDB_MEM_CPY(pInput->pBuff+offset,&pCurTop->ulModuleId,sizeof(uint32_t));
					//offset += sizeof(uint32_t);
					//ACDB_MEM_CPY(pInput->pBuff+offset,&pCurTop->pDataNode->ulParamId,noOfTableIndices*sizeof(uint32_t));
					//offset += sizeof(uint32_t);
					++noOfEntriesCopied;
				}
					else
					{
						nMemExhausted = 1;
						break;
					}
				}
				//pCurTop = pCurTop->pNext;
				++curNoOfEntriesOffset;
			}
			pCur = pCur->pNext;
		}
	}

	//if(curNoOfEntriesOffset <= pInput->nTblEntriesOffset)
	//{
	//	ACDB_DEBUG_LOG("Invalid offset request is provided to retrieve the table entries\n");
	//	return ACDB_ERROR;
	//}
	//nActualRemainingEntries = curNoOfEntriesOffset - pInput->nTblEntriesOffset;
	//nActualRemainingEntries = (curNoOfEntriesOffset>pInput->nRequiredNoOfTblEntries)?pInput->nRequiredNoOfTblEntries:curNoOfEntriesOffset;
	//noOfEntriesRemaining = 0;
	//if(nActualRemainingEntries <= pInput->nRequiredNoOfTblEntries)
	//	noOfEntriesRemaining = 0;
	//else
	//	noOfEntriesRemaining = nActualRemainingEntries - pInput->nRequiredNoOfTblEntries;
	ACDB_MEM_CPY(pInput->pBuff,sizeof(noOfEntriesCopied),&noOfEntriesCopied,sizeof(noOfEntriesCopied));
	//ACDB_MEM_CPY(pInput->pBuff+sizeof(uint32_t),&noOfEntriesRemaining,sizeof(noOfEntriesRemaining));
	pOut->nBytesUsedInBuffer = offset;
	return result;
}
// Externalized Function
int32_t Acdb_DM_Ioctl(uint32_t Acdb_DM_CMD_Id,
                      AcdbDataLookupKeyType *pKey,
                      uint32_t *pModuleId,
                      uint32_t *pParamId,
                      uint8_t *pInputBuf,
                      const uint32_t InputBufLen,
                      const uint8_t *pFileDataBuf,
                      const uint32_t nFileDataLen,
					  uint8_t *pOutBuff,
					  const uint32_t nOutBuffLen,
					  uint32_t *nOutBuffBytesUsed,
                      AcdbDynamicUniqueDataType **ppDataNode
                      )
{
   int32_t result = ACDB_SUCCESS;
   //UNREFERENCED_VAR(nOutBuffBytesUsed);
   if(nOutBuffBytesUsed != NULL)
	*nOutBuffBytesUsed = 0;

   //Global Variable Initialization
   if (g_pTbl == NULL)
   {
      g_pTbl = (AcdbDynamicTblNodeType*)ACDB_MALLOC(sizeof(AcdbDynamicTblNodeType));
      if (g_pTbl != NULL)
      {
         g_pTbl->pTblHead = NULL;
         g_pTbl->pTblTail = NULL;
      }
      else
      {
         result = ACDB_BADSTATE;
      }
   }
   if (g_pData == NULL)
   {
      g_pData = (AcdbDynamicDataNodeType*)ACDB_MALLOC(sizeof(AcdbDynamicDataNodeType));
      if (g_pData != NULL)
      {
         g_pData->pDatalHead = NULL;
         g_pData->pDatalTail = NULL;
      }
      else
      {
         result = ACDB_BADSTATE;
      }
   }
   if (g_pAdieTbl == NULL)
   {
      g_pAdieTbl = (AcdbDynamicAdieTblNodeType*)ACDB_MALLOC(sizeof(AcdbDynamicAdieTblNodeType));
      if (g_pAdieTbl != NULL)
      {
         g_pAdieTbl->pAdieTblHead = NULL;
         g_pAdieTbl->pAdieTblTail = NULL;
      }
      else
      {
         result = ACDB_BADSTATE;
      }
   }

   switch (Acdb_DM_CMD_Id)
   {
   case ACDB_GET_DATA:

      result = Acdb_GetDataCal((AcdbDataLookupKeyType*) pKey,
                               (uint32_t*) pModuleId,
                               (uint32_t*) pParamId,
                               (AcdbDynamicUniqueDataType**) &(*ppDataNode)
                               );
      break;

   case ACDB_SET_DATA:

      result = Acdb_SetDataCal((AcdbDataLookupKeyType*) pKey,
                               (uint32_t*) pModuleId,
                               (uint32_t*) pParamId,
							   (const uint8_t*) pFileDataBuf,
							   nFileDataLen,
                               (uint8_t*) pInputBuf,
                               (uint32_t) InputBufLen
                               );

      break;

   case ACDB_GET_ADIE_TABLE:

      result = Acdb_GetAdieTableCal((AcdbDataLookupKeyType*) pKey,
                                   (AcdbDynamicUniqueDataType**) &(*ppDataNode)
                                   );
      break;

   case ACDB_SET_ADIE_TABLE:

      result = Acdb_SetAdieTableCal((AcdbDataLookupKeyType*) pKey,
                                   (uint8_t*) pInputBuf,
                                   (uint32_t) InputBufLen
                                   );
      break;

   case ACDB_GET_NO_OF_TBL_ENTRIES_ON_HEAP:
      result = Acdb_GetNoOfTblEntries(pInputBuf,InputBufLen,pOutBuff,nOutBuffLen);
	   break;
   case ACDB_GET_TBL_ENTRIES_ON_HEAP:
      result = Acdb_GetTblEntries(pInputBuf,InputBufLen,pOutBuff,nOutBuffLen);
	   break;

   case ACDB_SYS_RESET:

      result = Acdb_sys_reset();

      break;
   }

   return result;
}
