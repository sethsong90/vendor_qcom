/*===========================================================================
    FILE:           acdb_command.c

    OVERVIEW:       This file contains the implementaion of the helper methods
                    used to service ACDB ioctls.

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

    $Header: //source/qcom/qct/multimedia2/Audio/audcal4/acdb_sw/main/latest/acdb/src/acdb_command.c#2 $

    when        who     what, where, why
    ----------  ---     -----------------------------------------------------
    2013-08-07  avi     Support Fluence VP3 interfaces
    2013-05-23  mh      Updated software version minor from 5 to 6 (i.e. v2.6)
    2013-07-18  mh      Added new funstion to get device property and its size
    2013-06-07  avi     Support Voice Volume boost feature
    2013-06-07  mh      Corrected checkpatch errors
    2013-06-03  mh      Updated software version minor from 1 to 2 (i.e. v2.2)
    2013-05-23  mh      Updated software version minor from 0 to 1
    2010-11-16  ernanl  1. Added get/set calibration data of Afe table API
    2010-09-21  ernanl  1. Device Info Data Structure changes, update ACDB to main
                        backward comptible with new changes.
                        2. ADIE calibration support.
    2010-07-23  ernanl  Complete get/set function and introduce new
                        functions into command specific functions
    2010-07-01  vmn     Split apart the global functions into command
                        specific functions.
    2010-06-16  aas     Fixed the get table so that it returns module id,
                        param id and data length of each calibration unit
    2010-04-26  aas     Initial implementation of the acdb_ioctl API and
                        associated helper methods.
	2012-05-15 kpavan   1. New interface for APQ MDM device Mapping.
						2. New interface to get TX RX Device pair for Recording

========================================================================== */

/* ---------------------------------------------------------------------------
 * Include Files
 *--------------------------------------------------------------------------- */

#include "acdb_command.h"
#include "acdb_file_mgr.h"
#include "acdb_init.h"
#include "acdb_utility.h"
#include "acdb_data_mgr.h"
#include "acdb_translation.h"
#include "acdb_init_utility.h"

/* ---------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *--------------------------------------------------------------------------- */
#define ACDB_SOFTWARE_VERSION_MAJOR                      0x00000003
#define ACDB_SOFTWARE_VERSION_MINOR                      0x00000000
#define ACDB_SOFTWARE_VERSION_REVISION                      0x00000000

/* ---------------------------------------------------------------------------
 * Type Declarations
 *--------------------------------------------------------------------------- */

typedef struct _AcdbFileBufType{
	uint32_t nfile_size;
	uint8_t *pFileBuf;
}AcdbFileBufType;

typedef struct _AcdbCmdInitType {
   uint32_t nNoOfFiles;
   AcdbFileBufType acdbFileBufs[20];
} AcdbCmdInitType;

/* ---------------------------------------------------------------------------
 * Global Data Definitions
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Static Variable Definitions
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Static Function Declarations and Definitions
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Externalized Function Declarations and Definitions
 *--------------------------------------------------------------------------- */

int32_t AcdbCmdInitializeAcdb (AcdbInitCmdType *pInput)
{
   int32_t result = ACDB_SUCCESS;
   uint32_t i = 0;
   uint32_t avail_count = 0;
   AcdbCmdFileInfo *pCmdFileInfos = NULL;
   //AcdbCmdInitType initcmd;
   //initcmd.nNoOfFiles=0;
   if(pInput == NULL || pInput->nNoOfFiles > 20)
      return ACDB_BADPARM;

   pCmdFileInfos = (AcdbCmdFileInfo *)ACDB_MALLOC(sizeof(AcdbCmdFileInfo) * pInput->nNoOfFiles);
   if (NULL == pCmdFileInfos)
      return ACDB_INSUFFICIENTMEMORY;

   for(i=0;i<pInput->nNoOfFiles;i++)
   {
	   uint32_t maj;
	   uint32_t min;
           uint32_t revision;
	   int32_t initresult = acdb_init(pInput->acdbFiles[i].fileName,&pCmdFileInfos[i].pFileBuf,&pCmdFileInfos[i].nFileSize,&maj,&min,&revision);
	   if(initresult == ACDB_INIT_SUCCESS)
	   {
           if(!(maj == ACDB_SOFTWARE_VERSION_MAJOR && min == ACDB_SOFTWARE_VERSION_MINOR && revision == ACDB_SOFTWARE_VERSION_REVISION))
		   {
               ACDB_DEBUG_LOG("[ACDB Command]->SW Minor/Major/Revision version mismatch warning %s\n",pInput->acdbFiles[i].fileName);
               ACDB_DEBUG_LOG("[ACDB Command]->ACDB Sw Major = %d, ACDB Sw Minor = %d, ACDB Sw Revision = %d\n",ACDB_SOFTWARE_VERSION_MAJOR,ACDB_SOFTWARE_VERSION_MINOR,ACDB_SOFTWARE_VERSION_REVISION);
               ACDB_DEBUG_LOG("[ACDB Command]->ACDB File Major = %u, ACDB File Minor = %u, ACDB File Revision = %u\n",maj,min,revision);
                   }
	   }
	   if(ACDB_INIT_SUCCESS != initresult)
	   {
		   // If even one file is not valid the acdb_init should return failure.
		   // So we should free the memory allocated for any of the files;
		   uint8_t idx = 0;
		   for(idx = 0;idx<i;idx++)
		   {
			   AcdbFreeFileData((void *)pCmdFileInfos[idx].pFileBuf);
		   }
                   ACDB_MEM_FREE(pCmdFileInfos);
		   return ACDB_ERROR;
	   }
   }

   // Now that all the acbd files requested for initialization
   // are valid, check if acdb sw can host these man acdb files first
   // and then set the files info to the file mgr

   result = acdbdata_ioctl(ACDBDATACMD_GET_AVAILABLE_FILES_SLOT_COUNT,NULL,0,(uint8_t *)&avail_count,sizeof(uint32_t));
   if(ACDB_SUCCESS != result || avail_count < pInput->nNoOfFiles)
   {
	   uint8_t idx = 0;
	   for(idx = 0;idx<pInput->nNoOfFiles;idx++)
	   {
		   AcdbFreeFileData((void *)pCmdFileInfos[idx].pFileBuf);
	   }
           ACDB_MEM_FREE(pCmdFileInfos);
	   return ACDB_ERROR;
   }

   for(i=0;i<pInput->nNoOfFiles;i++)
   {
	   ACDB_MEM_CPY(&pCmdFileInfos[i].chFileName[0],sizeof(pCmdFileInfos[i].chFileName),&pInput->acdbFiles[i].fileName[0],sizeof(pInput->acdbFiles[i].fileName));
	   pCmdFileInfos[i].nFileNameLen = pInput->acdbFiles[i].fileNameLen;
	   if(ACDB_SUCCESS != acdbdata_ioctl(ACDBDATACMD_SET_ACDB_DATA,(uint8_t *)&pCmdFileInfos[i],sizeof(AcdbCmdFileInfo),NULL,0))
	   {
		   // This failure should never occur,
		   // if so free up the rest of the acdb files memory which was created for safety
		   while(i<pInput->nNoOfFiles)
		   {
			   AcdbFreeFileData((void *)pCmdFileInfos[i].pFileBuf);
			   i++;
		   }
                   ACDB_MEM_FREE(pCmdFileInfos);
		   return ACDB_ERROR;
	   }
   }
   ACDB_MEM_FREE(pCmdFileInfos);
   return result;
}

int32_t AcdbCmdSystemReset(void)
{
	int32_t result = ACDB_SUCCESS;
	result = acdbdata_ioctl(ACDBDATACMD_RESET,NULL,0,NULL,0);
	result = ACDBHeapReset();
	return result;
}

int32_t AcdbCmdGetAcdbSwVersion (AcdbModuleVersionType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   uint16_t major = ACDB_SOFTWARE_VERSION_MAJOR;
   uint16_t minor = ACDB_SOFTWARE_VERSION_REVISION;

   if (pOutput != NULL)
   {
      ACDB_MEM_CPY(&pOutput->major,sizeof(pOutput->major),&major,sizeof(major));
      ACDB_MEM_CPY(&pOutput->minor,sizeof(pOutput->minor),&minor,sizeof(minor));
   }
   else
   {
	   return ACDB_BADPARM;
   }
   return result;
}

int32_t AcdbCmdGetAcdbSwVersionV2 (AcdbModuleVersionTypeV2 *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   uint32_t major = ACDB_SOFTWARE_VERSION_MAJOR;
   uint32_t minor = ACDB_SOFTWARE_VERSION_MINOR;
   uint32_t revision = ACDB_SOFTWARE_VERSION_REVISION;

   if (pOutput != NULL)
   {
      ACDB_MEM_CPY(&pOutput->major,sizeof(pOutput->major),&major,sizeof(major));
      ACDB_MEM_CPY(&pOutput->minor,sizeof(pOutput->minor),&minor,sizeof(minor));
      ACDB_MEM_CPY(&pOutput->revision,sizeof(pOutput->revision),&revision,sizeof(revision));
   }
   else
   {
	   return ACDB_BADPARM;
   }
   return result;
}

int32_t AcdbCmdGetCmnDeviceInfo (AcdbDeviceInfoCmnCmdType* pInput,
                                 AcdbQueryResponseType* pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetCmnDeviceInfo]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetCmnDeviceInfo]->NULL pointer\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   AcdbDevPropInfo dpInfo = {0};
	   dpInfo.devId = pInput->nDeviceId;
	   dpInfo.pId = DEVICE_CMN_INFO;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&dpInfo, sizeof(AcdbDevPropInfo),
                              (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the common dev info property for devid %08X \n",pInput->nDeviceId);
		   return result;
	   }
	   if(pInput->nBufferLength < dpInfo.dataInfo.nDataLen)
	   {
	       ACDB_DEBUG_LOG("ACDB_COMMAND: Buffer size insufficient to copy the cmn device info data for devid %08X \n",pInput->nDeviceId);
		   return ACDB_INSUFFICIENTMEMORY;
	   }
	   ACDB_MEM_CPY(pInput->nBufferPointer,pInput->nBufferLength,dpInfo.dataInfo.pData,dpInfo.dataInfo.nDataLen);
	   pOutput->nBytesUsedInBuffer = dpInfo.dataInfo.nDataLen;
   }

   return result;
}

int32_t AcdbCmdGetTSDeviceInfo (AcdbDeviceInfoTargetSpecificCmdType* pInput,
                                AcdbQueryResponseType* pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetTSDeviceInfo]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->pBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetTSDeviceInfo]->NULL pointer\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   AcdbDevPropInfo dpInfo = {0};
	   dpInfo.devId = pInput->nDeviceId;
	   dpInfo.pId = DEVICE_SPEC_INFO;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&dpInfo, sizeof(AcdbDevPropInfo),
                              (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the common dev info property for devid %08X \n",pInput->nDeviceId);
		   return result;
	   }
	   if(pInput->nBufferLength < dpInfo.dataInfo.nDataLen)
	   {
	       ACDB_DEBUG_LOG("ACDB_COMMAND: Buffer size insufficient to copy the cmn device info data for devid %08X \n",pInput->nDeviceId);
		   return ACDB_INSUFFICIENTMEMORY;
	   }
	   ACDB_MEM_CPY(pInput->pBufferPointer,pInput->nBufferLength,dpInfo.dataInfo.pData,dpInfo.dataInfo.nDataLen);
	   pOutput->nBytesUsedInBuffer = dpInfo.dataInfo.nDataLen;
   }

   return result;
}

int32_t AcdbCmdGetDeviceCapabilities(AcdbDeviceCapabilitiesCmdType *pInput,
                                     AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceCapabilities]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceCapabilities]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t i = 0;
	   int32_t dstOffset = 0, srcOffset = 0;
	   AcdbDevices *pDevs = (AcdbDevices *)ACDB_MALLOC(sizeof(AcdbDevices));
	   if(pDevs == NULL)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbDevices\n");
		   return ACDB_INSUFFICIENTMEMORY;
	   }

	   memset((void*)pDevs, 0, sizeof(AcdbDevices));

	   if(ACDB_DEVICE_CAPABILITY_PARAM != pInput->nParamId)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Invalid param value %08X provided for GetDeviceCapabilities\n",pInput->nParamId);
		   ACDB_MEM_FREE(pDevs);
		   return ACDB_BADPARM;
	   }
	   result = acdbdata_ioctl (ACDBDATACMD_GET_DEVICE_LIST, (uint8_t *)pDevs, sizeof(AcdbDevices),
                              (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the devicelist to fill the device capabilities info\n");
		   ACDB_MEM_FREE(pDevs);
		   return result;
	   }

       if (pInput->nBufferLength < (sizeof (uint32_t) +
		   (pDevs->noOfDevs * sizeof (AcdbDeviceCapabilityType))))
       {
           result = ACDB_INSUFFICIENTMEMORY;
           ACDB_DEBUG_LOG("ACDB_COMMAND: InsufficientMemory to fulfill GetDeviceCapabilities request\n");
		   ACDB_MEM_FREE(pDevs);
		   return result;
       }
	   ACDB_MEM_CPY((void*)(pInput->nBufferPointer+dstOffset),sizeof(uint32_t),(void*)&pDevs->noOfDevs,sizeof(uint32_t));
       dstOffset += sizeof(uint32_t);
	   for(i=0;i<pDevs->noOfDevs;i++)
	   {
		   AcdbDevPropInfo dpCmnInfo = {0};
		   AcdbDevPropInfo dpSpecInfo = {0};
		   uint32_t nDeviceType;
		   dpCmnInfo.devId = pDevs->devList[i];
		   dpCmnInfo.pId = DEVICE_CMN_INFO;
		   result = acdbdata_ioctl (ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&dpCmnInfo, sizeof(AcdbDevPropInfo),
								  (uint8_t *)NULL, 0);
		   if(result != ACDB_SUCCESS)
		   {
			   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the device capabilities for devid %08X \n",dpCmnInfo.devId);
			   ACDB_MEM_FREE(pDevs);
			   return result;
		   }

		   dpSpecInfo.devId = pDevs->devList[i];
		   dpSpecInfo.pId = DEVICE_SPEC_INFO;
		   result = acdbdata_ioctl (ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&dpSpecInfo, sizeof(AcdbDevPropInfo),
								  (uint8_t *)NULL, 0);
		   if(result != ACDB_SUCCESS)
		   {
			   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the device capabilities for devid %08X \n",dpSpecInfo.devId);
			   ACDB_MEM_FREE(pDevs);
			   return result;
		   }

		   // Get DeviceId
		   ACDB_MEM_CPY((void*)(pInput->nBufferPointer+dstOffset),sizeof(uint32_t),(void*)&pDevs->devList[i],sizeof(uint32_t));
		   dstOffset += sizeof(uint32_t);

		   //Get DeviceType
		   ACDB_MEM_CPY((void*)&nDeviceType,sizeof(uint32_t),(void*)(dpCmnInfo.dataInfo.pData+sizeof(uint32_t)),sizeof(uint32_t));

           //Memcopy Sample rate mask
           srcOffset = acdb_devinfo_getSampleMaskOffset(nDeviceType);
           if (!srcOffset)
           {
              ACDB_DEBUG_LOG("ACDB_COMMAND: AcdbCmdGetDeviceCapabilities failed\n");
			  ACDB_MEM_FREE(pDevs);
			  return ACDB_ERROR;
           }

           ACDB_MEM_CPY((void*)(pInput->nBufferPointer+dstOffset),sizeof(uint32_t),(void*)(dpSpecInfo.dataInfo.pData+srcOffset),sizeof(uint32_t));
           dstOffset += sizeof(uint32_t);

           //Memcopy Byte per sample rate mask
           srcOffset = acdb_devinfo_getBytesPerSampleMaskOffset(nDeviceType);
           if (!srcOffset)
           {
              ACDB_DEBUG_LOG("ACDB_COMMAND: AcdbCmdGetDeviceCapabilities failed\n");
			  ACDB_MEM_FREE(pDevs);
			  return ACDB_ERROR;
           }
           ACDB_MEM_CPY((void*)(pInput->nBufferPointer+dstOffset),sizeof(uint32_t),(void*)(dpSpecInfo.dataInfo.pData+srcOffset),sizeof(uint32_t));
           dstOffset += sizeof(uint32_t);
	   }
	   pOutput->nBytesUsedInBuffer = dstOffset;
	   ACDB_MEM_FREE(pDevs);
   }

   return result;
}

int32_t AcdbCmdGetDevicePair (AcdbDevicePairType *pInput,
                              AcdbDevicePairingResponseType *pOutput)
{
   int32_t result = ACDB_ERROR;

   if (pInput == NULL || pOutput == NULL)
   {
	   ACDB_DEBUG_LOG("ACDB_Command: Provided invalid input\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   AcdbDevPropInfo devPropInfo = {0};
	   uint32_t i = 0;
	   uint32_t nNoOfRxDevs;
	   AcdbDevPairInfo *pRxDevs = NULL;
	   pOutput->ulIsDevicePairValid = 0;
	   devPropInfo.devId = pInput->nTxDeviceId;
	   devPropInfo.pId = DEVPAIR;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
							  (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the device pair info for devid %08X \n",pInput->nTxDeviceId);
		   return result;
	   }
	   nNoOfRxDevs = devPropInfo.dataInfo.nDataLen/sizeof(AcdbDevPairInfo);
	   pRxDevs = (AcdbDevPairInfo *)devPropInfo.dataInfo.pData;
	   if(nNoOfRxDevs == 0)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid devpair property for devid %08X \n",pInput->nTxDeviceId);
		   return ACDB_ERROR;
	   }
	   for(i=0;i<nNoOfRxDevs;i++)
	   {
		   if(pRxDevs[i].nRxDevId == pInput->nRxDeviceId)
		   {
			   pOutput->ulIsDevicePairValid = 1;
			   return ACDB_SUCCESS;
		   }
	   }
   }

   return ACDB_SUCCESS;
}

int32_t AcdbCmdGetVolTableStepSize (AcdbVolTblStepSizeRspType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pOutput == NULL)
   {
		ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
		result = ACDB_BADPARM;
   }
   else
   {
	   AcdbGlbalPropInfo glbPropInfo = {0};
	   glbPropInfo.pId = AUD_VOL_STEPS;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
							  (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n",glbPropInfo.pId);
		   return result;
	   }
	   if (NULL == glbPropInfo.dataInfo.pData)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n",glbPropInfo.pId);
		   return ACDB_ERROR;
	   }

	   pOutput->AudProcVolTblStepSize = *((uint32_t *)glbPropInfo.dataInfo.pData);

	   glbPropInfo.pId = VOC_VOL_STEPS;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
							  (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n",glbPropInfo.pId);
		   return result;
	   }
	   pOutput->VocProcVolTblStepSize = *((uint32_t *)glbPropInfo.dataInfo.pData);
   }
   return result;
}

int32_t AcdbCmdGetANCDevicePair (AcdbAncDevicePairCmdType *pInput,
                                 AcdbAncDevicePairRspType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbAncDevicePairCmdType]->System Erorr\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   AcdbDevPropInfo devPropInfo = {0};
	   uint32_t nNoOfTxDevs;
	   AcdbANCPairInfo *pTxDevs = NULL;
	   devPropInfo.devId = pInput->nRxDeviceId;
	   devPropInfo.pId = ANCDEVPAIR;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
							  (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the anc device pair info for devid %08X \n",pInput->nRxDeviceId);
		   return result;
	   }
	   nNoOfTxDevs = devPropInfo.dataInfo.nDataLen/sizeof(AcdbANCPairInfo);
	   pTxDevs = (AcdbANCPairInfo *)devPropInfo.dataInfo.pData;
	   if(nNoOfTxDevs == 0)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid anc devpair property for devid %08X \n",pInput->nRxDeviceId);
		   return ACDB_ERROR;
	   }
	   pOutput->nTxAncDeviceId = pTxDevs[0].nTxDevId;
   }

   return result;
}

int32_t AcdbCmdGetAudProcCmnTopId (AcdbGetAudProcTopIdCmdType *pInput,
                                   AcdbGetTopologyIdRspType *pOutput
                                   )
{
   int32_t result = ACDB_PARMNOTFOUND;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudProcCmnTopId]->System Erorr\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   AcdbDevPropInfo devPropInfo = {0};
	   uint32_t i = 0;
	   uint32_t nNoOfEntries;
	   AcdbAudprocTopInfo *pTopInfo = NULL;
	   devPropInfo.devId = pInput->nDeviceId;
	   devPropInfo.pId = AUDPROC_CMN_TOPID;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
							  (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the aud topo info for devid %08X \n",pInput->nDeviceId);
		   return ACDB_PARMNOTFOUND;
	   }
	   nNoOfEntries = devPropInfo.dataInfo.nDataLen/sizeof(AcdbAudprocTopInfo);
	   pTopInfo = (AcdbAudprocTopInfo *)devPropInfo.dataInfo.pData;
	   if(nNoOfEntries == 0)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid audtopo property for devid %08X \n",pInput->nDeviceId);
		   return ACDB_PARMNOTFOUND;
	   }
	   //reset the result
	   result = ACDB_PARMNOTFOUND;
	   for(i=0;i<nNoOfEntries;i++)
	   {
		   if(pTopInfo[i].nAppId == pInput->nApplicationType)
		   {
			   pOutput->nTopologyId = pTopInfo[i].nTopoId;
			   return ACDB_SUCCESS;
		   }
	   }
   }

   return result;
}

int32_t AcdbCmdGetVocProcCmnTopId (AcdbGetVocProcTopIdCmdType *pInput,
                                   AcdbGetTopologyIdRspType *pOutput
                                   )
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocProcCmnTopId]->System Erorr\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   AcdbDevPropInfo devPropInfo = {0};
	   uint32_t nNoOfEntries;
	   AcdbVocprocTopInfo *pTopInfo = NULL;
	   devPropInfo.devId = pInput->nDeviceId;
	   devPropInfo.pId = VOCPROC_CMN_TOPID;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
							  (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the voc topo info for devid %08X \n",pInput->nDeviceId);
		   return ACDB_PARMNOTFOUND;
	   }
	   nNoOfEntries = devPropInfo.dataInfo.nDataLen/sizeof(AcdbVocprocTopInfo);
	   pTopInfo = (AcdbVocprocTopInfo *)devPropInfo.dataInfo.pData;
	   if(nNoOfEntries == 0)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid voctopo property for devid %08X \n",pInput->nDeviceId);
		   return ACDB_PARMNOTFOUND;
	   }
	   pOutput->nTopologyId = pTopInfo[0].nTopoId;
   }

   return result;
}

int32_t AcdbCmdGetAudProcStrmTopId (AcdbGetAudProcStrmTopIdCmdType *pInput,
                                    AcdbGetTopologyIdRspType *pOutput
                                    )
{
   int32_t result = ACDB_ERROR;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudProcStrmTopId]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t i = 0;
	   uint32_t nNoOfEntries;
	   AcdbAudStrmTopInfo *pTopInfo = NULL;
	   AcdbGlbalPropInfo glbPropInfo = {0};
	   glbPropInfo.pId = AUDPROC_STRM_TOPID;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
							  (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n",glbPropInfo.pId);
		   return ACDB_PARMNOTFOUND;
	   }
	   nNoOfEntries = glbPropInfo.dataInfo.nDataLen/sizeof(AcdbAudStrmTopInfo);
	   pTopInfo = (AcdbAudStrmTopInfo *)glbPropInfo.dataInfo.pData;
	   if(nNoOfEntries == 0)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid vocstream property for appid %08X \n",pInput->nApplicationType);
		   return ACDB_PARMNOTFOUND;
	   }
	   for(i=0;i<nNoOfEntries;i++)
	   {
		   if(pTopInfo[i].nAppId == pInput->nApplicationType)
		   {
			   pOutput->nTopologyId = pTopInfo[i].nTopoId;
			   return ACDB_SUCCESS;
		   }
	   }
   }
   return ACDB_PARMNOTFOUND;
}

int32_t AcdbCmdGetCompRemoteDevId (AcdbGetRmtCompDevIdCmdType *pInput,
                                   AcdbGetRmtCompDevIdRspType *pOutput
                                   )
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetCompRemoteDevId]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   AcdbDevPropInfo devPropInfo = {0};
	   uint32_t nNoOfEntries;
	   AcdbCompRemoteDevIdInfo *pRemDevInfo = NULL;
	   devPropInfo.devId = pInput->nNativeDeviceId;
	   devPropInfo.pId = APQ_MDM_COMP_DEVID;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
							  (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the compatible remote devid for native devid %08X \n",pInput->nNativeDeviceId);
		   return ACDB_PARMNOTFOUND;
	   }
	   nNoOfEntries = devPropInfo.dataInfo.nDataLen/sizeof(AcdbCompRemoteDevIdInfo);
	   pRemDevInfo = (AcdbCompRemoteDevIdInfo *)devPropInfo.dataInfo.pData;
	   if(nNoOfEntries == 0)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid remotedevid property for native devid %08X \n",pInput->nNativeDeviceId);
		   return ACDB_PARMNOTFOUND;
	   }
	   pOutput->nRmtDeviceId= pRemDevInfo[0].nDeviceId;
   }

   return result;
}

int32_t AcdbCmdGetRecordRxDeviceList (AcdbAudioRecRxListCmdType *pInput,
                                    AcdbAudioRecRxListRspType *pOutput)

{
	int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetRecordRxDeviceList]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   AcdbDevPropInfo devPropInfo = {0};
	   uint32_t i = 0;
	   uint32_t offset = 0;
	   uint32_t nNoOfRxDevs;
	   AcdbECRecDevPairInfo *pRxDevs = NULL;
	   devPropInfo.devId = pInput->nTxDeviceId;
	   devPropInfo.pId = RECORDED_DEVICEPAIR;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
							  (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the ec record device pair info for devid %08X \n",pInput->nTxDeviceId);
		   return ACDB_PARMNOTFOUND;
	   }
	   nNoOfRxDevs = devPropInfo.dataInfo.nDataLen/sizeof(AcdbECRecDevPairInfo);
	   pRxDevs = (AcdbECRecDevPairInfo *)devPropInfo.dataInfo.pData;
	   if(nNoOfRxDevs == 0)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Contains invalid ec record devpair property for devid %08X \n",pInput->nTxDeviceId);
		   return ACDB_PARMNOTFOUND;
	   }
	   pOutput->nNoOfRxDevs = nNoOfRxDevs;
	   for(i=0;i<nNoOfRxDevs;i++)
	   {
		   ACDB_MEM_CPY( (uint8_t *)((uint8_t *)pOutput->pRxDevs + offset),sizeof(AcdbECRecDevPairInfo),(uint8_t *)&pRxDevs[i].nRxDeviceId,sizeof(AcdbECRecDevPairInfo));
		   offset += sizeof(AcdbECRecDevPairInfo);
	   }
   }
  return result;
}

int32_t AcdbCmdGetAudProcInfo(uint32_t queryType,uint8_t *pIn,uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudProcInfo]->Invalid NULL value parameters are provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   uint32_t tblId=AUDPROC_GAIN_INDP_TBL;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   AudProcCmnDataLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t nLookupKey;
	   AudProcCmnCmdLookupType audcmd;

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbAudProcTableCmdType *pInput = (AcdbAudProcTableCmdType *)pIn;
			   audcmd.nDeviceId = pInput->nDeviceId;
			   audcmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
			   audcmd.nApplicationType = pInput->nApplicationType;
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbAudProcTableSizeCmdType *pInput = (AcdbAudProcTableSizeCmdType *)pIn;
			   audcmd.nDeviceId = pInput->nDeviceId;
			   audcmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
			   audcmd.nApplicationType = pInput->nApplicationType;
		   }
		   break;
	   case DATA_CMD:
		   {
			   AcdbAudProcCmdType *pInput = (AcdbAudProcCmdType *)pIn;
			   audcmd.nDeviceId = pInput->nDeviceId;
			   audcmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
			   audcmd.nApplicationType = pInput->nApplicationType;
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }
	   //acdb_translate_sample_rate(audcmd.nDeviceSampleRateId,&audcmd.nDeviceSampleRateId);

	   cmd.devId = audcmd.nDeviceId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,audcmd.nDeviceId);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (AudProcCmnDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   AUDPROC_LUT_INDICES_COUNT,&audcmd,AUDPROC_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,audcmd.nDeviceId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   nLookupKey = (uint32_t)&lutTbl.pLut[index];
	   // Now get CDEF info
	   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset));
	   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

	   // Now get CDOT info
	   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset));
	   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));

	   if(cdefTbl.nLen != cdotTbl.nLen)
	   {
		   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n",audcmd.nDeviceId);
		   return ACDB_ERROR;
	   }

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbAudProcTableCmdType *pInput = (AcdbAudProcTableCmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
                           result = GetMidPidCalibTable(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,
				   pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
			   if(ACDB_SUCCESS != result)
			   {
				   return result;
			   }
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
			   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&pOutput->nSize))
			   {
				   return ACDB_ERROR;
			   }
		   }
		   break;
	   case DATA_CMD:
		   {
			   AcdbAudProcCmdType *pInput = (AcdbAudProcCmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
                           result = GetMidPidCalibData(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,pInput->nModuleId,
				   pInput->nParamId,pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
			   if(ACDB_SUCCESS != result)
			   {
				   return result;
			   }
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdSetAudProcData(AcdbAudProcCmdType *pInput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcData]->Invalid NULL value parameters are provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t tblId=AUDPROC_GAIN_INDP_TBL;
	   AudProcCmnCmdLookupType audcmd;
	   audcmd.nDeviceId = pInput->nDeviceId;
	   audcmd.nDeviceSampleRateId = pInput->nDeviceSampleRateId;
	   audcmd.nApplicationType = pInput->nApplicationType;
	   result = AcdbCmdSetOnlineData(tblId,(uint8_t *)&audcmd,AUDPROCTBL_INDICES_COUNT,pInput->nModuleId,pInput->nParamId,pInput->nBufferPointer,pInput->nBufferLength);
   }

   return result;
}

int32_t AcdbCmdGetAudProcGainDepStepInfo(uint32_t queryType,uint8_t *pIn,uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudProcGainDepStepInfo]->Invalid Null input provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   AudProcGainDepDataLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t nLookupKey;
	   AudProcGainDepCmdLookupType audcmd;
	   uint32_t tblId = AUDPROC_COPP_GAIN_DEP_TBL;

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbAudProcGainDepVolTblStepCmdType *pInput = (AcdbAudProcGainDepVolTblStepCmdType *)pIn;
			   audcmd.nDeviceId = pInput->nDeviceId;
			   audcmd.nApplicationType = pInput->nApplicationType;
			   audcmd.nVolIdx = pInput->nVolumeIndex;
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbAudProcGainDepVolTblStepSizeCmdType *pInput = (AcdbAudProcGainDepVolTblStepSizeCmdType *)pIn;
			   audcmd.nDeviceId = pInput->nDeviceId;
			   audcmd.nApplicationType = pInput->nApplicationType;
			   audcmd.nVolIdx = pInput->nVolumeIndex;
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   cmd.devId = audcmd.nDeviceId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,audcmd.nDeviceId);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (AudProcGainDepDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   AUDPROC_GAIN_DEP_LUT_INDICES_COUNT,&audcmd,AUDPROC_GAIN_DEP_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,audcmd.nDeviceId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }
	   // Now get CDEF info
	   nLookupKey = (uint32_t)&lutTbl.pLut[index];
	   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset));
	   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

	   // Now get CDOT info
	   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset));
	   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));

	   if(cdefTbl.nLen != cdotTbl.nLen)
	   {
		   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching \n",audcmd.nDeviceId);
		   return ACDB_ERROR;
	   }

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbAudProcGainDepVolTblStepCmdType *pInput = (AcdbAudProcGainDepVolTblStepCmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
                           result = GetMidPidCalibTable(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,
				   pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
			   if(ACDB_SUCCESS != result)
			   {
				   return result;
			   }
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
			   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&pOutput->nSize))
			   {
				   return ACDB_ERROR;
			   }
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAudProcVolStepInfo(uint32_t queryType,uint8_t *pIn,uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudProcVolStepInfo]->Invalid Null input param provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   AudProcGainDepDataLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t nLookupKey;
	   AudProcGainDepCmdLookupType audcmd;
	   uint32_t tblId = AUDPROC_AUD_VOL_TBL;

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbAudProcGainDepVolTblStepCmdType *pInput = (AcdbAudProcGainDepVolTblStepCmdType *)pIn;
			   audcmd.nDeviceId = pInput->nDeviceId;
			   audcmd.nApplicationType = pInput->nApplicationType;
			   audcmd.nVolIdx = pInput->nVolumeIndex;
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbAudProcGainDepVolTblStepSizeCmdType *pInput = (AcdbAudProcGainDepVolTblStepSizeCmdType *)pIn;
			   audcmd.nDeviceId = pInput->nDeviceId;
			   audcmd.nApplicationType = pInput->nApplicationType;
			   audcmd.nVolIdx = pInput->nVolumeIndex;
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   cmd.devId = audcmd.nDeviceId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,audcmd.nDeviceId);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (AudProcGainDepDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   AUDPROC_VOL_LUT_INDICES_COUNT,&audcmd,AUDPROC_VOL_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,audcmd.nDeviceId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   nLookupKey = (uint32_t)&lutTbl.pLut[index];
	   // Now get CDEF info
	   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset));
	   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

	   // Now get CDOT info
	   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset));
	   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));

	   if(cdefTbl.nLen != cdotTbl.nLen)
	   {
		   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n",audcmd.nDeviceId);
		   return ACDB_ERROR;
	   }

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbAudProcGainDepVolTblStepCmdType *pInput = (AcdbAudProcGainDepVolTblStepCmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
                           result = GetMidPidCalibTable(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,
				   pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
			   if(ACDB_SUCCESS != result)
			   {
				   return result;
			   }
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
			   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&pOutput->nSize))
			   {
				   return ACDB_ERROR;
			   }
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }
	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAudStreamInfo(uint32_t queryType,uint8_t *pIn,uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudStreamTableSize]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index=0;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   AudStreamDataLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t nLookupKey=0;
	   AudStreamCmdLookupType audstrmcmd;
	   uint32_t tblId = AUD_STREAM_TBL;

	   memset(&cmd,0,sizeof(AcdbTableCmd));
	   memset(&tblInfo,0,sizeof(AcdbTableInfo));
       memset(&lutTbl,0,sizeof(AudStreamDataLookupTblType));
       memset(&cdefTbl,0,sizeof(ContentDefTblType));
       memset(&cdotTbl,0,sizeof(ContentDataOffsetsTblType));
       memset(&audstrmcmd,0,sizeof(AudStreamCmdLookupType));

		switch(queryType)
		{
		case TABLE_CMD:
		   {
			   AcdbAudStrmTableV2CmdType *pInput = (AcdbAudStrmTableV2CmdType *)pIn;
			   audstrmcmd.nApplicationType = pInput->nApplicationTypeId;
		   }
		   break;
		case TABLE_SIZE_CMD:
		   {
			   AcdbAudStrmTblSizeCmdType *pInput = (AcdbAudStrmTblSizeCmdType *)pIn;
			   audstrmcmd.nApplicationType = pInput->nApplicationTypeId;
		   }
		   break;
		case DATA_CMD:
		   {
			   AcdbAudStrmV2CmdType *pInput = (AcdbAudStrmV2CmdType *)pIn;
			   audstrmcmd.nApplicationType = pInput->nApplicationTypeId;
		   }
		   break;
		default:
		   return  ACDB_ERROR;
		}

	   cmd.devId = 0;
	   cmd.tblId = tblId;

	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the appid %08X \n"
			   ,audstrmcmd.nApplicationType);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (AudStreamDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   AUDSTREAM_LUT_INDICES_COUNT,&audstrmcmd,AUDSTREAM_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the appid %08X \n"
			   ,audstrmcmd.nApplicationType);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   nLookupKey = (uint32_t)&lutTbl.pLut[index];
	   // Now get CDEF info
	   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset));
	   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

	   // Now get CDOT info
	   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset));
	   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));

	   if(cdefTbl.nLen != cdotTbl.nLen)
	   {
		   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for appid %08X \n",audstrmcmd.nApplicationType);
		   return ACDB_ERROR;
	   }

		switch(queryType)
		{
		case TABLE_CMD:
		   {
			   AcdbAudStrmTableV2CmdType *pInput = (AcdbAudStrmTableV2CmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
                           result = GetMidPidCalibTable(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,
				   pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
			   if(ACDB_SUCCESS != result)
			   {
				   return result;
			   }
		   }
		   break;
		case TABLE_SIZE_CMD:
		   {
			   AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
			   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&pOutput->nSize))
			   {
				   return ACDB_ERROR;
			   }
		   }
		   break;
		case DATA_CMD:
		   {
			   AcdbAudStrmV2CmdType *pInput = (AcdbAudStrmV2CmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
                           result = GetMidPidCalibData(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,pInput->nModuleId,
				   pInput->nParamId,pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
			   if(ACDB_SUCCESS != result)
			   {
				   return result;
			   }
		   }
		   break;
		default:
		   return  ACDB_ERROR;
		}
		result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdSetAudStreamData(AcdbAudStrmV2CmdType *pInput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudStreamData]->Invalid NULL value parameters are provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t tblId=AUD_STREAM_TBL;
	   AudStreamCmdLookupType audstrmcmd;
	   audstrmcmd.nApplicationType = pInput->nApplicationTypeId;
	   result = AcdbCmdSetOnlineData(tblId,(uint8_t *)&audstrmcmd,AUDSTREAMTBL_INDICES_COUNT,pInput->nModuleId,pInput->nParamId,pInput->nBufferPointer,pInput->nBufferLength);
   }

   return result;
}

int32_t AcdbCmdGetVocProcInfo(uint32_t queryType,uint8_t *pIn,uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAudProcVolStepInfo]->Invalid Null input param provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   VocProcDataLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t offset = 0;
	   uint32_t nMemBytesLeft=0; // Used for Tbl cmd
	   uint32_t nMemBytesRequired=0; //Used for Size cmd
	   uint32_t nLookupKey;
	   VocProcCmdLookupType voccmd;
	   uint32_t tblId = VOCPROC_GAIN_INDP_TBL;
		switch(queryType)
		{
		case TABLE_CMD:
		   {
			   AcdbVocProcCmnTblCmdType *pInput = (AcdbVocProcCmnTblCmdType *)pIn;
			   voccmd.nTxDevId = pInput->nTxDeviceId;
			   voccmd.nRxDevId = pInput->nRxDeviceId;
			   voccmd.nTxAfeSr = pInput->nTxDeviceSampleRateId;
			   voccmd.nRxAfeSr = pInput->nRxDeviceSampleRateId;
		   }
		   break;
		case TABLE_SIZE_CMD:
		   {
			   AcdbVocProcCmnTblSizeCmdType *pInput = (AcdbVocProcCmnTblSizeCmdType *)pIn;
			   voccmd.nTxDevId = pInput->nTxDeviceId;
			   voccmd.nRxDevId = pInput->nRxDeviceId;
			   voccmd.nTxAfeSr = pInput->nTxDeviceSampleRateId;
			   voccmd.nRxAfeSr = pInput->nRxDeviceSampleRateId;
		   }
		   break;
		default:
		   return  ACDB_ERROR;
		}

		//acdb_translate_sample_rate(voccmd.nTxAfeSr,&voccmd.nTxAfeSr);
		//acdb_translate_sample_rate(voccmd.nRxAfeSr,&voccmd.nRxAfeSr);
		cmd.devId = voccmd.nTxDevId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,voccmd.nTxDevId);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (VocProcDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   VOCPROC_LUT_INDICES_COUNT,&voccmd,VOCPROC_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,voccmd.nTxDevId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   if(queryType == TABLE_CMD) //Iniitalize variables
	   {
		   AcdbVocProcCmnTblCmdType *pInput = (AcdbVocProcCmnTblCmdType *)pIn;
		   nMemBytesLeft = pInput->nBufferLength;
		   offset = 0;
	   }

	   do
	   {
		   nLookupKey = (uint32_t)&lutTbl.pLut[index];
		   // Now get CDEF info
		   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset));
		   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

		   // Now get CDOT info
		   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset));
		   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));

		   if(cdefTbl.nLen != cdotTbl.nLen)
		   {
			   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n",voccmd.nTxDevId);
			   return ACDB_ERROR;
		   }

		   switch(queryType)
		   {
		   case TABLE_CMD:
			   {
				   AcdbVocProcCmnTblCmdType *pInput = (AcdbVocProcCmnTblCmdType *)pIn;
				   uint32_t nMemBytesUsed=0;
				   VocProcCVDTblHdrFmtType hdr;
				   hdr.nNetworkId = lutTbl.pLut[index].nNetwork;
				   hdr.nTxVocSr = lutTbl.pLut[index].nTxVocSR;
				   hdr.nRxVocSr = lutTbl.pLut[index].nRxVocSR;
				   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&hdr.nCalDataSize))
				   {
					   return ACDB_ERROR;
				   }
				   if(nMemBytesLeft < (sizeof(hdr)+hdr.nCalDataSize))
				   {
					   ACDB_DEBUG_LOG("Insufficient memory to copy the vocproc table data\n");
					   return ACDB_INSUFFICIENTMEMORY;
				   }
				   //copy the vocproc table cvd header in to buffer
				   ACDB_MEM_CPY(pInput->nBufferPointer+offset,nMemBytesLeft,&hdr,sizeof(hdr));
				   nMemBytesLeft -= sizeof(hdr);
				   offset += sizeof(hdr);
				   //copy the caltable in to buffer
				   result = GetMidPidCalibTable(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,
					   pInput->nBufferPointer+offset,nMemBytesLeft,&nMemBytesUsed);
				   if(ACDB_SUCCESS != result)
				   {
					   return result;
				   }
				   nMemBytesLeft -= nMemBytesUsed;
				   offset += nMemBytesUsed;
				   if(nMemBytesUsed != hdr.nCalDataSize)
				   {
					   ACDB_DEBUG_LOG("Data size mismatch between getsize cmd and gettable cmd\n");
					   return ACDB_ERROR;
				   }
			   }
			   break;
		   case TABLE_SIZE_CMD:
			   {
				   uint32_t nBytesUsed=0;
				   nMemBytesRequired += sizeof(VocProcCVDTblHdrFmtType);
				   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&nBytesUsed))
				   {
					   return ACDB_ERROR;
				   }
				   nMemBytesRequired += nBytesUsed;
			   }
			   break;
		   default:
			   return  ACDB_ERROR;
		   }

		   //if(memcmp(&lutTbl.pLut[index+1],pIn,VOCPROC_CMD_INDICES_COUNT*sizeof(uint32_t))!=0)
		   if(memcmp(&lutTbl.pLut[index+1],&voccmd,sizeof(voccmd))!=0)
		   {
			   index = 0xFFFFFFFF;
			   break;
		   }
		   else
		   {
			   ++index;
		   }
	   }while(index<lutTbl.nLen);

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbVocProcCmnTblCmdType *pInput = (AcdbVocProcCmnTblCmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
			   pOutput->nBytesUsedInBuffer = pInput->nBufferLength - nMemBytesLeft;
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
			   pOutput->nSize = nMemBytesRequired;
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetVocProcVolInfo(uint32_t queryType,uint8_t *pIn,uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocProcVolInfo]->Invalid Null input param provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   VocProcVolDataLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t offset = 0;
	   uint32_t nMemBytesLeft=0; // Used for Tbl cmd
	   uint32_t nMemBytesRequired=0; //Used for Size cmd
	   uint32_t nLookupKey;
	   VocProcVolCmdLookupType voccmd;
	   uint32_t tblId = VOCPROC_COPP_GAIN_DEP_TBL;

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbVocProcGainDepVolTblCmdType *pInput = (AcdbVocProcGainDepVolTblCmdType *)pIn;
			   voccmd.nTxDevId = pInput->nTxDeviceId;
			   voccmd.nRxDevId = pInput->nRxDeviceId;
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbVocProcGainDepVolTblSizeCmdType *pInput = (AcdbVocProcGainDepVolTblSizeCmdType *)pIn;
			   voccmd.nTxDevId = pInput->nTxDeviceId;
			   voccmd.nRxDevId = pInput->nRxDeviceId;
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   cmd.devId = voccmd.nTxDevId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,voccmd.nTxDevId);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (VocProcVolDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   VOCPROC_VOL_LUT_INDICES_COUNT,&voccmd,VOCPROC_VOL_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,voccmd.nTxDevId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   if(queryType == TABLE_CMD) //Iniitalize variables
	   {
		   AcdbVocProcGainDepVolTblCmdType *pInput = (AcdbVocProcGainDepVolTblCmdType *)pIn;
		   nMemBytesLeft = pInput->nBufferLength;
		   offset = 0;
	   }

	   do
	   {
		   nLookupKey = (uint32_t)&lutTbl.pLut[index];
		   // Now get CDEF info
		   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset));
		   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

		   // Now get CDOT info
		   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset));
		   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));

		   if(cdefTbl.nLen != cdotTbl.nLen)
		   {
			   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n",voccmd.nTxDevId);
			   return ACDB_ERROR;
		   }

		   switch(queryType)
		   {
		   case TABLE_CMD:
			   {
				   AcdbVocProcGainDepVolTblCmdType *pInput = (AcdbVocProcGainDepVolTblCmdType *)pIn;
				   uint32_t nMemBytesUsed=0;
				   VocProcVolCVDTblHdrFmtType hdr;
				   hdr.nNetworkId = lutTbl.pLut[index].nNetwork;
				   hdr.nTxVocSr = lutTbl.pLut[index].nTxVocSR;
				   hdr.nRxVocSr = lutTbl.pLut[index].nRxVocSR;
				   hdr.nVolIdx = lutTbl.pLut[index].nVolStep;
				   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&hdr.nCalDataSize))
				   {
					   return ACDB_ERROR;
				   }
				   if(nMemBytesLeft < (sizeof(hdr)+hdr.nCalDataSize))
				   {
					   ACDB_DEBUG_LOG("Insufficient memory to copy the vocprocvol table data\n");
					   return ACDB_INSUFFICIENTMEMORY;
				   }
				   //copy the vocproc table cvd header in to buffer
				   ACDB_MEM_CPY(pInput->nBufferPointer+offset,nMemBytesLeft,&hdr,sizeof(hdr));
				   nMemBytesLeft -= sizeof(hdr);
				   offset += sizeof(hdr);
				   //copy the caltable in to buffer
				   result = GetMidPidCalibTable(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,
					   pInput->nBufferPointer+offset,nMemBytesLeft,&nMemBytesUsed);
				   if(ACDB_SUCCESS != result)
				   {
					   return result;
				   }
				   nMemBytesLeft -= nMemBytesUsed;
				   offset += nMemBytesUsed;
				   if(nMemBytesUsed != hdr.nCalDataSize)
				   {
					   ACDB_DEBUG_LOG("Data size mismatch between getsize cmd and gettable cmd\n");
					   return ACDB_ERROR;
				   }
			   }
			   break;
		   case TABLE_SIZE_CMD:
			   {
				   uint32_t nBytesUsed=0;
				   nMemBytesRequired += sizeof(VocProcVolCVDTblHdrFmtType);
				   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&nBytesUsed))
				   {
					   return ACDB_ERROR;
				   }
				   nMemBytesRequired += nBytesUsed;
			   }
			   break;
		   default:
			   return  ACDB_ERROR;
		   }

		   if(memcmp(&lutTbl.pLut[index+1],pIn,VOCPROC_VOL_CMD_INDICES_COUNT*sizeof(uint32_t))!=0)
		   {
			   index = 0xFFFFFFFF;
			   break;
		   }
		   else
		   {
			   ++index;
		   }
	   }while(index<lutTbl.nLen);

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbVocProcGainDepVolTblCmdType *pInput = (AcdbVocProcGainDepVolTblCmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
			   pOutput->nBytesUsedInBuffer = pInput->nBufferLength - nMemBytesLeft;
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
			   pOutput->nSize = nMemBytesRequired;
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetVocProcVolV2Info(uint32_t queryType,uint8_t *pIn,uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocProcVolV2Info]->Invalid Null input param provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   VocProcVolV2DataLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t offset = 0;
	   uint32_t nMemBytesLeft=0; // Used for Tbl cmd
	   uint32_t nMemBytesRequired=0; //Used for Size cmd
	   uint32_t nLookupKey;
	   VocProcVolV2CmdLookupType voccmd;
       uint32_t tblId = VOCPROC_COPP_GAIN_DEP_V2_TBL;

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbVocProcGainDepVolTblV2CmdType *pInput = (AcdbVocProcGainDepVolTblV2CmdType *)pIn;
			   voccmd.nTxDevId = pInput->nTxDeviceId;
			   voccmd.nRxDevId = pInput->nRxDeviceId;
                           voccmd.nFeatureId = pInput->nFeatureId;
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbVocProcGainDepVolTblSizeV2CmdType *pInput = (AcdbVocProcGainDepVolTblSizeV2CmdType *)pIn;
			   voccmd.nTxDevId = pInput->nTxDeviceId;
			   voccmd.nRxDevId = pInput->nRxDeviceId;
                           voccmd.nFeatureId = pInput->nFeatureId;
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   cmd.devId = voccmd.nTxDevId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));

	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,voccmd.nTxDevId);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (VocProcVolV2DataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   VOCPROC_VOL_V2_LUT_INDICES_COUNT,&voccmd,VOCPROC_VOL_V2_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,voccmd.nTxDevId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   if(queryType == TABLE_CMD) //Iniitalize variables
	   {
		   AcdbVocProcGainDepVolTblV2CmdType *pInput = (AcdbVocProcGainDepVolTblV2CmdType *)pIn;
		   nMemBytesLeft = pInput->nBufferLength;
		   offset = 0;
	   }

	   do
	   {
		   nLookupKey = (uint32_t)&lutTbl.pLut[index];
		   // Now get CDEF info
		   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset));
		   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

		   // Now get CDOT info
		   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset));
		   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));

		   if(cdefTbl.nLen != cdotTbl.nLen)
		   {
			   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n",voccmd.nTxDevId);
			   return ACDB_ERROR;
		   }

		   switch(queryType)
		   {
		   case TABLE_CMD:
			   {
				   AcdbVocProcGainDepVolTblV2CmdType *pInput = (AcdbVocProcGainDepVolTblV2CmdType *)pIn;
				   uint32_t nMemBytesUsed=0;
				   VocProcVolCVDTblHdrFmtType hdr;
				   hdr.nNetworkId = lutTbl.pLut[index].nNetwork;
				   hdr.nTxVocSr = lutTbl.pLut[index].nTxVocSR;
				   hdr.nRxVocSr = lutTbl.pLut[index].nRxVocSR;
				   hdr.nVolIdx = lutTbl.pLut[index].nVolStep;
				   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&hdr.nCalDataSize))
				   {
					   return ACDB_ERROR;
				   }
				   if(nMemBytesLeft < (sizeof(hdr)+hdr.nCalDataSize))
				   {
					   ACDB_DEBUG_LOG("Insufficient memory to copy the vocprocvol table data\n");
					   return ACDB_INSUFFICIENTMEMORY;
				   }
				   //copy the vocprocVolV2 table cvd header in to buffer
				   ACDB_MEM_CPY(pInput->nBufferPointer+offset,nMemBytesLeft,&hdr,sizeof(hdr));
				   nMemBytesLeft -= sizeof(hdr);
				   offset += sizeof(hdr);
				   //copy the caltable in to buffer
				   result = GetMidPidCalibTable(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,
					   pInput->nBufferPointer+offset,nMemBytesLeft,&nMemBytesUsed);
				   if(ACDB_SUCCESS != result)
				   {
					   return result;
				   }
				   nMemBytesLeft -= nMemBytesUsed;
				   offset += nMemBytesUsed;
				   if(nMemBytesUsed != hdr.nCalDataSize)
				   {
					   ACDB_DEBUG_LOG("Data size mismatch between getsize cmd and gettable cmd\n");
					   return ACDB_ERROR;
				   }
			   }
			   break;
		   case TABLE_SIZE_CMD:
			   {
				   uint32_t nBytesUsed=0;
				   nMemBytesRequired += sizeof(VocProcVolCVDTblHdrFmtType);
				   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&nBytesUsed))
				   {
					   return ACDB_ERROR;
				   }
				   nMemBytesRequired += nBytesUsed;
			   }
			   break;
		   default:
			   return  ACDB_ERROR;
		   }

		   if(memcmp(&lutTbl.pLut[index+1],pIn,VOCPROC_VOL_V2_CMD_INDICES_COUNT*sizeof(uint32_t))!=0)
		   {
			   index = 0xFFFFFFFF;
			   break;
		   }
		   else
		   {
			   ++index;
		   }
	   }while(index<lutTbl.nLen);

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbVocProcGainDepVolTblV2CmdType *pInput = (AcdbVocProcGainDepVolTblV2CmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
			   pOutput->nBytesUsedInBuffer = pInput->nBufferLength - nMemBytesLeft;
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
			   pOutput->nSize = nMemBytesRequired;
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetVocStreamInfo(uint32_t queryType,uint8_t *pIn,uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if ( (pOut == NULL) || (queryType == TABLE_CMD && pIn == NULL))
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocStreamInfo]->Invalid Null params provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   VocStreamDataLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t offset = 0;
	   uint32_t nMemBytesLeft=0; // Used for Tbl cmd
	   uint32_t nMemBytesRequired=0; //Used for Size cmd
	   uint32_t nLookupKey;
	   uint32_t tblId = VOC_STREAM_TBL;

	   cmd.devId = 0;  //Vocstream is not device based so make it zero
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the vocstream table\n");
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (VocStreamDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
	   if(lutTbl.nLen <= 0)
	   {
		   ACDB_DEBUG_LOG("Vocstream lookuptable empty\n");
		   return ACDB_ERROR;
	   }

	   if(queryType == TABLE_CMD) //Iniitalize variables
	   {
		   AcdbQueryCmdType *pInput = (AcdbQueryCmdType *)pIn;
		   nMemBytesLeft = pInput->nBufferLength;
		   offset = 0;
	   }

	   for(index=0;index<lutTbl.nLen;index++)
	   {
		   nLookupKey = (uint32_t)&lutTbl.pLut[index];
		   // Now get CDEF info
		   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset));
		   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

		   // Now get CDOT info
		   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset));
		   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));

		   if(cdefTbl.nLen != cdotTbl.nLen)
		   {
			   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables are not matching\n");
			   return ACDB_ERROR;
		   }

		   switch(queryType)
		   {
		   case TABLE_CMD:
			   {
				   AcdbQueryCmdType *pInput = (AcdbQueryCmdType *)pIn;
				   uint32_t nMemBytesUsed=0;
				   // The tbl header fmt of vocproctbl and vocstream tbl is same
				   VocProcCVDTblHdrFmtType hdr;
				   hdr.nNetworkId = lutTbl.pLut[index].nNetwork;
				   hdr.nTxVocSr = lutTbl.pLut[index].nTxVocSR;
				   hdr.nRxVocSr = lutTbl.pLut[index].nRxVocSR;
				   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&hdr.nCalDataSize))
				   {
					   return ACDB_ERROR;
				   }
				   if(nMemBytesLeft < (sizeof(hdr)+hdr.nCalDataSize))
				   {
					   ACDB_DEBUG_LOG("Insufficient memory to copy the vocprocvol table data\n");
					   return ACDB_INSUFFICIENTMEMORY;
				   }
				   //copy the vocproc table cvd header in to buffer
				   ACDB_MEM_CPY(pInput->pBufferPointer+offset,nMemBytesLeft,&hdr,sizeof(hdr));
				   nMemBytesLeft -= sizeof(hdr);
				   offset += sizeof(hdr);
				   //copy the caltable in to buffer
				   result = GetMidPidCalibTable(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,
					   pInput->pBufferPointer+offset,nMemBytesLeft,&nMemBytesUsed);
				   if(ACDB_SUCCESS != result)
				   {
					   return result;
				   }
				   nMemBytesLeft -= nMemBytesUsed;
				   offset += nMemBytesUsed;
				   if(nMemBytesUsed != hdr.nCalDataSize)
				   {
					   ACDB_DEBUG_LOG("Data size mismatch between getsize cmd and gettable cmd\n");
					   return ACDB_ERROR;
				   }
			   }
			   break;
		   case TABLE_SIZE_CMD:
			   {
				   uint32_t nBytesUsed=0;
				   nMemBytesRequired += sizeof(VocProcCVDTblHdrFmtType);
				   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&nBytesUsed))
				   {
					   return ACDB_ERROR;
				   }
				   nMemBytesRequired += nBytesUsed;
			   }
			   break;
		   default:
			   return  ACDB_ERROR;
		   }
	   }

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbQueryCmdType *pInput = (AcdbQueryCmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
			   pOutput->nBytesUsedInBuffer = pInput->nBufferLength - nMemBytesLeft;
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
			   pOutput->nSize = nMemBytesRequired;
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAdieANCDataTable (AcdbCodecANCSettingCmdType *pInput,
                                    AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAdieANCDataTable]->System Erorr\n");
	  result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAdieANCDataTable]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   ANCDataLookupTblType lutTbl;
	   uint32_t nLookupKey;
	   ANCCmdLookupType anccmd;
	   uint32_t tblId = ADIE_ANC_TBL;

	   anccmd.nDeviceId = pInput->nRxDeviceId;
	   anccmd.nPID = pInput->nParamId;

	   cmd.devId = anccmd.nDeviceId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,cmd.devId);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (ANCDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   ANC_LUT_INDICES_COUNT,&anccmd,ANC_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,cmd.devId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   nLookupKey = (uint32_t)&lutTbl.pLut[index];
	   result = GetCalibData(tblId,nLookupKey,lutTbl.pLut[index].nDataOffset,tblInfo.dataPoolChnk,
		   pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
	   if(ACDB_SUCCESS != result)
	   {
		   return result;
	   }
	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAdieProfileTable (AcdbAdiePathProfileV2CmdType *pInput,
                                    AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAdieProfileTable]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAdieProfileTable]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   ADIEDataLookupTblType lutTbl;
	   uint32_t nLookupKey;
	   ADIECmdLookupType adiecmd;
	   uint32_t tblId = ADIE_CODEC_TBL;

	   adiecmd.nCodecId = pInput->ulCodecPathId;
	   adiecmd.nPID = pInput->nParamId;

	   cmd.devId = 0;  //adie calibration is not device based, so make it zero
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the codecpathid %08X \n"
			   ,adiecmd.nCodecId);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (ADIEDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   ADIE_LUT_INDICES_COUNT,&adiecmd,ADIE_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the codecid %08X \n"
			   ,adiecmd.nCodecId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   nLookupKey = (uint32_t)&lutTbl.pLut[index];
           result = GetCalibData(tblId,nLookupKey,lutTbl.pLut[index].nDataOffset,tblInfo.dataPoolChnk,
		   pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
	   if(ACDB_SUCCESS != result)
	   {
		   return result;
	   }
	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetGlobalTable (AcdbGblTblCmdType *pInput,
                               AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetGlobalTable]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetGlobalTable]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   GlobalDataLookupTblType lutTbl;
	   uint32_t nLookupKey;
	   GlobalCmdLookupType gcmd;
	   uint32_t tblId = GLOBAL_DATA_TBL;

	   gcmd.nMid = pInput->nModuleId;
	   gcmd.nPid = pInput->nParamId;

	   cmd.devId = 0;  //adie calibration is not device based, so make it zero
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the globaldata table\n");
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (GlobalDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   GLOBAL_LUT_INDICES_COUNT,&gcmd,GLOBAL_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Couldnt find the mid %08X and pid %08X \n",gcmd.nMid,gcmd.nPid);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   nLookupKey = (uint32_t)&lutTbl.pLut[index];
           result = GetCalibData(tblId,nLookupKey,lutTbl.pLut[index].nDataOffset,tblInfo.dataPoolChnk,
		   pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
	   if(ACDB_SUCCESS != result)
	   {
		   return result;
	   }
	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAfeData (AcdbAfeDataCmdType *pInput,
                           AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAfeData]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAfeData]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   AfeDataLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t nLookupKey;
	   AfeCmdLookupType afecmd;
	   uint32_t tblId = AFE_TBL;

	   afecmd.nTxDevId = pInput->nTxDeviceId;
	   afecmd.nRxDevId = pInput->nRxDeviceId;

	   cmd.devId = afecmd.nTxDevId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,afecmd.nTxDevId);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (AfeDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   AFE_LUT_INDICES_COUNT,&afecmd,AFE_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,afecmd.nTxDevId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   nLookupKey = (uint32_t)&lutTbl.pLut[index];
	   // Now get CDEF info
	   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset));
	   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

	   // Now get CDOT info
	   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset));
	   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));

	   if(cdefTbl.nLen != cdotTbl.nLen)
	   {
		   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n",afecmd.nTxDevId);
		   return ACDB_ERROR;
	   }

           result = GetMidPidCalibData(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,pInput->nModuleId,
		   pInput->nParamId,pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
	   if(ACDB_SUCCESS != result)
	   {
		   return result;
	   }

	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAfeCmnInfo(uint32_t queryType,uint8_t *pIn,uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAfeCmnInfo]->Invalid NULL value parameters are provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   AfeCmnDataLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t nLookupKey;
	   AfeCmnCmdLookupType afecmncmd;
	   uint32_t tblId = AFE_CMN_TBL;

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbAfeCommonTableCmdType *pInput = (AcdbAfeCommonTableCmdType *)pIn;
			   afecmncmd.nDeviceId = pInput->nDeviceId;
			   afecmncmd.nDeviceSampleRateId = pInput->nSampleRateId;
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbAfeCommonTableSizeCmdType *pInput = (AcdbAfeCommonTableSizeCmdType *)pIn;
			   afecmncmd.nDeviceId = pInput->nDeviceId;
			   afecmncmd.nDeviceSampleRateId = pInput->nSampleRateId;
		   }
		   break;
	   case DATA_CMD:
		   {
			   AcdbAfeCmnDataCmdType *pInput = (AcdbAfeCmnDataCmdType *)pIn;
			   afecmncmd.nDeviceId = pInput->nDeviceId;
			   afecmncmd.nDeviceSampleRateId = pInput->nAfeSampleRateId;
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }
	   //acdb_translate_sample_rate(afecmncmd.nDeviceSampleRateId,&afecmncmd.nDeviceSampleRateId);

	   cmd.devId = afecmncmd.nDeviceId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,afecmncmd.nDeviceId);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (AfeCmnDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   AFECMN_LUT_INDICES_COUNT,&afecmncmd,AFECMN_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,afecmncmd.nDeviceId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   nLookupKey = (uint32_t)&lutTbl.pLut[index];
	   // Now get CDEF info
	   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset));
	   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

	   // Now get CDOT info
	   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset));
	   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));

	   if(cdefTbl.nLen != cdotTbl.nLen)
	   {
		   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n",afecmncmd.nDeviceId);
		   return ACDB_ERROR;
	   }

	   //if(ACDB_SUCCESS != GetMidPidCalibTableSize(cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&pOutput->nSize))
	   //{
		  // return ACDB_ERROR;
	   //}

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbAfeCommonTableCmdType *pInput = (AcdbAfeCommonTableCmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
			   result = GetMidPidCalibTable(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,
				   pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
			   if(ACDB_SUCCESS != result)
			   {
				   return result;
			   }
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
			   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&pOutput->nSize))
			   {
				   return ACDB_ERROR;
			   }
		   }
		   break;
	   case DATA_CMD:
		   {
			   AcdbAfeCmnDataCmdType *pInput = (AcdbAfeCmnDataCmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
                           result = GetMidPidCalibData(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,pInput->nModuleId,
				   pInput->nParamId,pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
			   if(ACDB_SUCCESS != result)
			   {
				   return result;
			   }
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetVocprocDevCfgInfo(uint32_t queryType,uint8_t *pIn,uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocprocDevCfgInfo]->Invalid NULL value parameters are provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   VocprocDevCfgDataLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t nLookupKey;
	   VocprocDevCfgCmdLookupType vpcfgcmd;
	   uint32_t tblId = VOCPROC_DEV_CFG_TBL;

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbVocProcDevCfgCmdType *pInput = (AcdbVocProcDevCfgCmdType *)pIn;
			   vpcfgcmd.nTxDevId = pInput->nTxDeviceId;
			   vpcfgcmd.nRxDevId = pInput->nRxDeviceId;
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbVocProcDevCfgSizeCmdType *pInput = (AcdbVocProcDevCfgSizeCmdType *)pIn;
			   vpcfgcmd.nTxDevId = pInput->nTxDeviceId;
			   vpcfgcmd.nRxDevId = pInput->nRxDeviceId;
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   cmd.devId = vpcfgcmd.nTxDevId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,vpcfgcmd.nTxDevId);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (VocprocDevCfgDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   VOCPROCDEVCFG_LUT_INDICES_COUNT,&vpcfgcmd,VOCPROCDEVCFG_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,vpcfgcmd.nTxDevId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   nLookupKey = (uint32_t)&lutTbl.pLut[index];
	   // Now get CDEF info
	   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset));
	   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

	   // Now get CDOT info
	   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset));
	   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));

	   if(cdefTbl.nLen != cdotTbl.nLen)
	   {
		   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n",vpcfgcmd.nTxDevId);
		   return ACDB_ERROR;
	   }

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbVocProcDevCfgCmdType *pInput = (AcdbVocProcDevCfgCmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
			   result = GetMidPidCalibTable(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,
				   pInput->pBuff,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
			   if(ACDB_SUCCESS != result)
			   {
				   return result;
			   }
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
			   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&pOutput->nSize))
			   {
				   return ACDB_ERROR;
			   }
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetVocColInfo(uint32_t queryType,uint8_t *pIn,uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVocColInfo]->Invalid NULL value parameters are provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t vocColInfo[] = {3, //No of columns
		ACDB_CAL_COLUMN_NETWORK,ACDB_CAL_COLUMN_TYPE_32_BIT,ACDB_CAL_NETWORK_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
		ACDB_CAL_COLUMN_TX_SAMPLING_RATE,ACDB_CAL_COLUMN_TYPE_32_BIT,ACDB_CAL_TX_SAMPLING_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
		ACDB_CAL_COLUMN_RX_SAMPLING_RATE,ACDB_CAL_COLUMN_TYPE_32_BIT,ACDB_CAL_RX_SAMPLING_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
		};
	   uint32_t vocVolColinfo[] = {4, //No of columns
		ACDB_CAL_COLUMN_NETWORK,ACDB_CAL_COLUMN_TYPE_32_BIT,ACDB_CAL_NETWORK_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
		ACDB_CAL_COLUMN_TX_SAMPLING_RATE,ACDB_CAL_COLUMN_TYPE_32_BIT,ACDB_CAL_TX_SAMPLING_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
		ACDB_CAL_COLUMN_RX_SAMPLING_RATE,ACDB_CAL_COLUMN_TYPE_32_BIT,ACDB_CAL_RX_SAMPLING_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
		ACDB_CAL_COLUMN_VOLUME_INDEX,ACDB_CAL_COLUMN_TYPE_32_BIT,ACDB_CAL_VOLUME_INDEX_ID_DUMMY_VALUE, //col id, colidtype, dummyvalue
		};
	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbVocColumnsInfoCmdType *pInput = (AcdbVocColumnsInfoCmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
			   if(pInput->nTableId == ACDB_VOC_PROC_TABLE)
			   {
				   if(pInput->nBufferLength < sizeof(vocColInfo))
				   {
					   ACDB_DEBUG_LOG("Memory not sufficeint to copy the vocproc col info\n");
					   return ACDB_INSUFFICIENTMEMORY;
				   }
				   ACDB_MEM_CPY((uint8_t *)pInput->pBuff,pInput->nBufferLength,(uint8_t *)&vocColInfo[0],sizeof(vocColInfo));
				   pOutput->nBytesUsedInBuffer = sizeof(vocColInfo);
			   }
			   else if(pInput->nTableId == ACDB_VOC_PROC_VOL_TABLE)
			   {
				   if(pInput->nBufferLength < sizeof(vocVolColinfo))
				   {
					   ACDB_DEBUG_LOG("Memory not sufficeint to copy the vocprocvol col info\n");
					   return ACDB_INSUFFICIENTMEMORY;
				   }
				   ACDB_MEM_CPY((uint8_t *)pInput->pBuff,pInput->nBufferLength,(uint8_t *)&vocVolColinfo[0],sizeof(vocVolColinfo));
				   pOutput->nBytesUsedInBuffer = sizeof(vocVolColinfo);
			   }
			   else if(pInput->nTableId == ACDB_VOC_STREAM_TABLE)
			   {
				   if(pInput->nBufferLength < sizeof(vocColInfo))
				   {
					   ACDB_DEBUG_LOG("Memory not sufficeint to copy the vocstream col info\n");
					   return ACDB_INSUFFICIENTMEMORY;
				   }
				   ACDB_MEM_CPY((uint8_t *)pInput->pBuff,pInput->nBufferLength,(uint8_t *)&vocColInfo[0],sizeof(vocColInfo));
				   pOutput->nBytesUsedInBuffer = sizeof(vocColInfo);
			   }
			   else
			   {
				   return ACDB_BADPARM;
			   }
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbVocColumnsInfoSizeCmdType *pInput = (AcdbVocColumnsInfoSizeCmdType *)pIn;
			   AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
			   if(pInput->nTableId == ACDB_VOC_PROC_TABLE)
			   {
				   pOutput->nSize = sizeof(vocColInfo);
			   }
			   else if(pInput->nTableId == ACDB_VOC_PROC_VOL_TABLE)
			   {
				   pOutput->nSize = sizeof(vocVolColinfo);
			   }
			   else if(pInput->nTableId == ACDB_VOC_STREAM_TABLE)
			   {
				   pOutput->nSize = sizeof(vocColInfo);
			   }
			   else
			   {
				   return ACDB_BADPARM;
			   }
		   }
		   break;
	   default:
		   return  ACDB_BADPARM;
	   }
	   result = ACDB_SUCCESS;
   }
   return result;
}

int32_t AcdbCmdGetFilesInfo(AcdbQueryCmdType *pReq,AcdbQueryResponseType *pRsp)
{
   int32_t result = ACDB_SUCCESS;
   if(pReq == NULL || pRsp == NULL)
   {
	   ACDB_DEBUG_LOG("Invalid input provided to fetch the files info\n");
	   return ACDB_ERROR;
   }
   if(pReq->pBufferPointer == NULL || pReq->nBufferLength == 0)
   {
	   ACDB_DEBUG_LOG("Insufficient memory provided to fetch the files info\n");
	   return ACDB_INSUFFICIENTMEMORY;
   }
   result = acdbdata_ioctl(ACDBDATACMD_GET_LOADED_FILES_INFO,pReq->pBufferPointer,
	   pReq->nBufferLength,(uint8_t*)&pRsp->nBytesUsedInBuffer,sizeof(pRsp->nBytesUsedInBuffer));
   return result;
}

int32_t AcdbCmdGetFileData(AcdbCmdGetFileDataReq *pReq,AcdbCmdResp *pRsp)
{
   int32_t result = ACDB_SUCCESS;
   AcdbFileMgrGetFileDataReq *pFmReq = (AcdbFileMgrGetFileDataReq *)pReq;
   AcdbFileMgrResp *pFmResp = (AcdbFileMgrResp *)pRsp;
   if(pReq == NULL || pRsp == NULL)
   {
	   ACDB_DEBUG_LOG("Invalid input provided to fetch the file data\n");
	   return ACDB_ERROR;
   }
   if(pReq->pFileName == NULL || pReq->nfileNameLen == 0)
   {
	   ACDB_DEBUG_LOG("Invalid filename or filename len provided to fetch the file data\n");
	   return ACDB_ERROR;
   }
   if(pRsp->pRespBuff == NULL || pRsp->nresp_buff_len == 0)
   {
	   ACDB_DEBUG_LOG("Insufficient memory provided to fetch the file data\n");
	   return ACDB_INSUFFICIENTMEMORY;
   }
   result = acdbdata_ioctl(ACDBDATACMD_GET_FILE_DATA,(uint8_t*)pFmReq,
	   sizeof(AcdbFileMgrGetFileDataReq),(uint8_t*)pFmResp,sizeof(AcdbFileMgrResp));
   return result;
}

int32_t AcdbCmdGetOnlineData(uint32_t tblId,uint8_t *pIndices,uint32_t nIdxCount,uint32_t mid,uint32_t pid,
						  uint8_t *pBuff, uint32_t nBuffLen,uint32_t *pBuffBytesFilled)
{
   int32_t result = ACDB_SUCCESS;

   if (pBuff == NULL || nBuffLen == 0)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->Invalid NULL value parameters are provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index=0;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   uint32_t devId=0;
	   uint32_t noOfLUTEntries = 0;
	   uint8_t *pLut;
	   uint32_t noOfLutIndices=0;
	   uint32_t noOfTableIndices = 0;
	   //AudProcCmnDataLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t nLookupKey=0;
	   uint32_t cdefoffset=0,cdotoffset=0;
	   uint32_t cdefoffsetval=0,cdotoffsetval=0;
	   uint8_t nonModuleTblFound = 0;

	   switch(tblId)
	   {
	   case AUDPROC_GAIN_INDP_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=AUDPROC_LUT_INDICES_COUNT;
		   noOfTableIndices = AUDPROCTBL_INDICES_COUNT;
		   break;
	   case AUDPROC_COPP_GAIN_DEP_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=AUDPROC_GAIN_DEP_LUT_INDICES_COUNT;
		   noOfTableIndices = AUDPROCTBL_GAIN_DEP_INDICES_COUNT;
		   break;
	   case AUDPROC_AUD_VOL_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=AUDPROC_VOL_LUT_INDICES_COUNT;
		   noOfTableIndices = AUDPROCTBL_VOL_INDICES_COUNT;
		   break;
	   case AUD_STREAM_TBL:
		   devId = 0;
		   noOfLutIndices=AUDSTREAM_LUT_INDICES_COUNT;
		   noOfTableIndices = AUDSTREAMTBL_INDICES_COUNT;
		   break;
	   case VOCPROC_GAIN_INDP_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=VOCPROC_LUT_INDICES_COUNT;
		   noOfTableIndices = VOCPROCTBL_INDICES_COUNT;
		   break;
	   case VOCPROC_COPP_GAIN_DEP_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=VOCPROC_VOL_LUT_INDICES_COUNT;
		   noOfTableIndices = VOCPROCTBL_VOL_INDICES_COUNT;
		   break;
	   case VOC_STREAM_TBL:
		   devId = 0;
		   noOfLutIndices=VOCSTREAM_LUT_INDICES_COUNT;
		   noOfTableIndices = VOCSTREAMTBL_INDICES_COUNT;
		   break;
	   case AFE_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=AFE_LUT_INDICES_COUNT;
		   noOfTableIndices = AFETBL_INDICES_COUNT;
		   break;
	   case AFE_CMN_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=AFECMN_LUT_INDICES_COUNT;
		   noOfTableIndices = AFECMNTBL_INDICES_COUNT;
		   break;
	   case VOCPROC_DEV_CFG_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=VOCPROCDEVCFG_LUT_INDICES_COUNT;
		   noOfTableIndices = VOCPROCDEVCFGTBL_INDICES_COUNT;
		   break;
	   case LSM_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=LSM_LUT_INDICES_COUNT;
		   noOfTableIndices = LSM_INDICES_COUNT;
		   break;
	   case ADIE_SIDETONE_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=ADST_LUT_INDICES_COUNT;
		   noOfTableIndices = ADST_INDICES_COUNT;
		   break;
	   case AANC_CFG_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=AANC_CFG_LUT_INDICES_COUNT;
		   noOfTableIndices = AANC_CFG_TBL_INDICES_COUNT;
		   break;
	   case ADIE_ANC_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=ANC_LUT_INDICES_COUNT;
		   noOfTableIndices = ANCTBL_INDICES_COUNT;
		   nonModuleTblFound = 1;
		   break;
	   case ADIE_CODEC_TBL:
		   devId = 0;
		   noOfLutIndices=ANC_LUT_INDICES_COUNT;
		   noOfTableIndices = ANCTBL_INDICES_COUNT;
		   nonModuleTblFound = 1;
		   break;
	   case GLOBAL_DATA_TBL:
		   devId = 0;
		   noOfLutIndices=GLOBAL_LUT_INDICES_COUNT;
		   noOfTableIndices = GLOBALTBL_INDICES_COUNT;
		   nonModuleTblFound = 1;
		   break;
	   case CDC_FEATURES_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=CDC_FEATURES_DATA_LUT_INDICES_COUNT;
		   noOfTableIndices = CDC_FEATURES_DATA_INDICES_COUNT;
		   nonModuleTblFound = 1;
		   break;
       case VOCPROC_COPP_GAIN_DEP_V2_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=VOCPROC_VOL_V2_LUT_INDICES_COUNT;
		   noOfTableIndices = VOCPROCTBL_VOL_V2_INDICES_COUNT;
		   break;
       case VOICE_VP3_TBL:
           ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=VOICE_VP3_LUT_INDICES_COUNT;
		   noOfTableIndices = VOICE_VP3TBL_INDICES_COUNT;
           break;
       case AUDIO_REC_VP3_TBL:
           ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=AUDREC_VP3_LUT_INDICES_COUNT;
		   noOfTableIndices = AUDREC_VP3TBL_INDICES_COUNT;
           break;
       case AUDIO_REC_EC_VP3_TBL:
           ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=AUDREC_EC_VP3_LUT_INDICES_COUNT;
		   noOfTableIndices = AUDREC_EC_VP3TBL_INDICES_COUNT;
           break;
	   default:
		   ACDB_DEBUG_LOG("Invalid table passed to get the acdb data\n");
		   return ACDB_ERROR;
	   }
	   if(nIdxCount != noOfTableIndices)
	   {
		   ACDB_DEBUG_LOG("Invalid indices passed to get the acdb data\n");
		   return ACDB_ERROR;
	   }
	   cmd.devId = devId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,devId);
		   return result;
	   }
	   ACDB_MEM_CPY(&noOfLUTEntries,sizeof(uint32_t),tblInfo.tblLutChnk.pData,sizeof(uint32_t));
	   pLut = (uint8_t *)(tblInfo.tblLutChnk.pData + sizeof(noOfLUTEntries));
	   //lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   //lutTbl.pLut = (AudProcCmnDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

	   result = AcdbDataBinarySearch((void *)pLut,noOfLUTEntries,
		   noOfLutIndices,pIndices,noOfTableIndices,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,devId);
		   return ACDB_ERROR;
	   }

       //nLookupKey = (uint32_t)&lutTbl.pLut[index];
	   nLookupKey = (uint32_t)(pLut+(noOfLutIndices*index*sizeof(uint32_t)));

	   if(nonModuleTblFound == 0)
	   {
		   // Now get CDEF info
		   //cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset));
		   //cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));
		   cdefoffset = (noOfLutIndices*index*sizeof(uint32_t)) + (noOfTableIndices*sizeof(uint32_t));
		   cdefoffsetval = *((uint32_t *)(pLut + cdefoffset));
		   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + cdefoffsetval));
		   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + cdefoffsetval+ sizeof(cdefTbl.nLen));

		   // Now get CDOT info
		   //cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset));
		   //cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));
		   cdotoffset = cdefoffset + sizeof(uint32_t);
		   cdotoffsetval = *((uint32_t *)(pLut + cdotoffset));
		   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + cdotoffsetval));
		   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + cdotoffsetval+ sizeof(cdotTbl.nLen));

		   if(cdefTbl.nLen != cdotTbl.nLen)
		   {
			   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n",devId);
			   return ACDB_ERROR;
		   }

		   if(ACDB_SUCCESS != GetMidPidCalibData(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,mid,
			   pid,pBuff,nBuffLen,pBuffBytesFilled))
		   {
			   return ACDB_ERROR;
		   }
	   }
	   else
	   {
		   uint32_t dataoffset = *((uint32_t *)(pLut + (noOfLutIndices*index*sizeof(uint32_t)) + (noOfTableIndices*sizeof(uint32_t))));
		   if(ACDB_SUCCESS != GetCalibData(tblId,nLookupKey,dataoffset,tblInfo.dataPoolChnk,
			   pBuff,nBuffLen,pBuffBytesFilled))
		   {
			   return ACDB_ERROR;
		   }
	   }

	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdSetOnlineData(uint32_t tblId,uint8_t *pIndices,uint32_t nIdxCount,uint32_t mid,uint32_t pid,
						  uint8_t *pInBuff, uint32_t nInBuffLen)
{
   int32_t result = ACDB_SUCCESS;

   if (pInBuff == NULL || nInBuffLen == 0)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->Invalid NULL value parameters are provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   uint32_t devId;
	   uint32_t noOfLUTEntries = 0;
	   uint8_t *pLut;
	   uint32_t noOfLutIndices=0;
	   uint32_t noOfTableIndices = 0;
	   //AudProcCmnDataLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t nLookupKey;
	   uint32_t cdefoffset,cdotoffset;
	   uint32_t cdefoffsetval,cdotoffsetval;
	   uint8_t nonModuleTblFound = 0;
           uint8_t dependencyTblPresent = 0;

	   switch(tblId)
	   {
	   case AUDPROC_GAIN_INDP_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=AUDPROC_LUT_INDICES_COUNT;
		   noOfTableIndices = AUDPROCTBL_INDICES_COUNT;
		   break;
	   case AUDPROC_COPP_GAIN_DEP_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=AUDPROC_GAIN_DEP_LUT_INDICES_COUNT;
		   noOfTableIndices = AUDPROCTBL_GAIN_DEP_INDICES_COUNT;
		   break;
	   case AUDPROC_AUD_VOL_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=AUDPROC_VOL_LUT_INDICES_COUNT;
		   noOfTableIndices = AUDPROCTBL_VOL_INDICES_COUNT;
		   break;
	   case AUD_STREAM_TBL:
		   devId = 0;
		   noOfLutIndices=AUDSTREAM_LUT_INDICES_COUNT;
		   noOfTableIndices = AUDSTREAMTBL_INDICES_COUNT;
		   break;
	   case VOCPROC_GAIN_INDP_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=VOCPROC_LUT_INDICES_COUNT;
		   noOfTableIndices = VOCPROCTBL_INDICES_COUNT;
		   break;
	   case VOCPROC_COPP_GAIN_DEP_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=VOCPROC_VOL_LUT_INDICES_COUNT;
		   noOfTableIndices = VOCPROCTBL_VOL_INDICES_COUNT;
		   break;
	   case VOC_STREAM_TBL:
		   devId = 0;
		   noOfLutIndices=VOCSTREAM_LUT_INDICES_COUNT;
		   noOfTableIndices = VOCSTREAMTBL_INDICES_COUNT;
		   break;
	   case AFE_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=AFE_LUT_INDICES_COUNT;
		   noOfTableIndices = AFETBL_INDICES_COUNT;
		   break;
	   case AFE_CMN_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=AFECMN_LUT_INDICES_COUNT;
		   noOfTableIndices = AFECMNTBL_INDICES_COUNT;
		   break;
	   case VOCPROC_DEV_CFG_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=VOCPROCDEVCFG_LUT_INDICES_COUNT;
		   noOfTableIndices = VOCPROCDEVCFGTBL_INDICES_COUNT;
		   break;
	   case LSM_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=LSM_LUT_INDICES_COUNT;
		   noOfTableIndices = LSM_INDICES_COUNT;
		   break;
	   case ADIE_SIDETONE_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=ADST_LUT_INDICES_COUNT;
		   noOfTableIndices = ADST_INDICES_COUNT;
		   break;
	   case AANC_CFG_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=AANC_CFG_LUT_INDICES_COUNT;
		   noOfTableIndices = AANC_CFG_TBL_INDICES_COUNT;
		   break;
	   case ADIE_ANC_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=ANC_LUT_INDICES_COUNT;
		   noOfTableIndices = ANCTBL_INDICES_COUNT;
		   nonModuleTblFound = 1;
		   break;
	   case ADIE_CODEC_TBL:
		   devId = 0;
		   noOfLutIndices=ANC_LUT_INDICES_COUNT;
		   noOfTableIndices = ANCTBL_INDICES_COUNT;
		   nonModuleTblFound = 1;
		   break;
	   case GLOBAL_DATA_TBL:
		   devId = 0;
		   noOfLutIndices=GLOBAL_LUT_INDICES_COUNT;
		   noOfTableIndices = GLOBALTBL_INDICES_COUNT;
		   nonModuleTblFound = 1;
		   break;
	   case CDC_FEATURES_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=CDC_FEATURES_DATA_LUT_INDICES_COUNT;
		   noOfTableIndices = CDC_FEATURES_DATA_INDICES_COUNT;
		   nonModuleTblFound = 1;
		   break;
       case VOCPROC_COPP_GAIN_DEP_V2_TBL:
		   ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
		   noOfLutIndices=VOCPROC_VOL_V2_LUT_INDICES_COUNT;
		   noOfTableIndices = VOCPROCTBL_VOL_V2_INDICES_COUNT;
                   dependencyTblPresent = 1;
		   break;
       case VOICE_VP3_TBL:
           ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
           noOfLutIndices=VOICE_VP3_LUT_INDICES_COUNT;
           noOfTableIndices = VOICE_VP3TBL_INDICES_COUNT;
           break;
       case AUDIO_REC_VP3_TBL:
           ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
           noOfLutIndices=AUDREC_VP3_LUT_INDICES_COUNT;
           noOfTableIndices = AUDREC_VP3TBL_INDICES_COUNT;
           break;
       case AUDIO_REC_EC_VP3_TBL:
           ACDB_MEM_CPY((void *)&devId,sizeof(uint32_t),(void *)pIndices,sizeof(uint32_t));
           noOfLutIndices=AUDREC_EC_VP3_LUT_INDICES_COUNT;
           noOfTableIndices = AUDREC_EC_VP3TBL_INDICES_COUNT;
           break;
	   default:
		   ACDB_DEBUG_LOG("Invalid table passed to get the acdb data\n");
		   return ACDB_ERROR;
	   }
	   if(nIdxCount != noOfTableIndices)
	   {
		   ACDB_DEBUG_LOG("Invalid indices passed to get the acdb data\n");
		   return ACDB_ERROR;
	   }
	   cmd.devId = devId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,devId);
		   return result;
	   }
	   ACDB_MEM_CPY(&noOfLUTEntries,sizeof(uint32_t),tblInfo.tblLutChnk.pData,sizeof(uint32_t));
	   pLut = (uint8_t *)(tblInfo.tblLutChnk.pData + sizeof(noOfLUTEntries));

	   result = AcdbDataBinarySearch((void *)pLut,noOfLUTEntries,
		   noOfLutIndices,pIndices,noOfTableIndices,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,devId);
		   return ACDB_ERROR;
	   }

	   nLookupKey = (uint32_t)(pLut+(noOfLutIndices*index*sizeof(uint32_t)));

	   if(nonModuleTblFound == 0)
	   {
		   // Now get CDEF info
		   cdefoffset = (noOfLutIndices*index*sizeof(uint32_t)) + (noOfTableIndices*sizeof(uint32_t));
		   cdefoffsetval = *((uint32_t *)(pLut + cdefoffset));
		   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + cdefoffsetval));
		   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + cdefoffsetval+ sizeof(cdefTbl.nLen));

		   // Now get CDOT info
		   cdotoffset = cdefoffset + sizeof(uint32_t);
		   cdotoffsetval = *((uint32_t *)(pLut + cdotoffset));
		   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + cdotoffsetval));
		   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + cdotoffsetval+ sizeof(cdotTbl.nLen));

		   if(cdefTbl.nLen != cdotTbl.nLen)
		   {
			   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n",devId);
			   return ACDB_ERROR;
		   }

		   if(ACDB_SUCCESS != SetMidPidCalibData(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,mid,
			   pid,pInBuff,nInBuffLen))
		   {
			   return ACDB_ERROR;
		   }
	   }
	   else
	   {
		   uint32_t dataoffset = *((uint32_t *)(pLut + (noOfLutIndices*index*sizeof(uint32_t)) + (noOfTableIndices*sizeof(uint32_t))));
		   if(ACDB_SUCCESS != SetCalibData(tblId,nLookupKey,dataoffset,tblInfo.dataPoolChnk,
			   pInBuff,nInBuffLen))
		   {
			   return ACDB_ERROR;
		   }
	   }

       if(dependencyTblPresent == 1)
       {
           uint32_t dependencyTblId = 0;
           uint8_t *pDepIndices = NULL;
           uint32_t nDepIdxCount = 0;
           uint32_t fsid = 0;
           switch(tblId)
           {
           case VOCPROC_COPP_GAIN_DEP_V2_TBL:
               {
                   dependencyTblId = VOCPROC_COPP_GAIN_DEP_TBL;
                   nDepIdxCount = nIdxCount - 1; // vocVolFeatId is not present in vocProcVol table, so we need to remove that.
                   ACDB_MEM_CPY((void *)&fsid,sizeof(uint32_t),(void *)(pIndices + 8),sizeof(uint32_t));

                   if(fsid == ACDB_VOCVOL_FID_DEFAULT) // only need to set to vocProcVol if it is Default ID.
                   {
                       // featId is 3rd idx, so, copy 1st two, ignore 3rd idx and copy rest.
                       pDepIndices = (uint8_t *)ACDB_MALLOC((noOfTableIndices - 1) * sizeof(uint32_t));
					   if(pDepIndices == NULL)
					   {
						   return ACDB_ERROR;
					   }
                       ACDB_MEM_CPY((void *)pDepIndices,sizeof(uint32_t)*2,(void *)pIndices, sizeof(uint32_t)*2);
                       ACDB_MEM_CPY((void *)(pDepIndices + sizeof(uint32_t)*2),sizeof(uint32_t)*(noOfTableIndices - 3),(void *)(pIndices + sizeof(uint32_t)*3),sizeof(uint32_t)*(noOfTableIndices - 3));

                       result = AcdbCmdSetOnlineData(dependencyTblId, pDepIndices, nDepIdxCount,mid,pid,pInBuff, nInBuffLen);
                       ACDB_MEM_FREE(pDepIndices);
                       pDepIndices = NULL;
                   }
               }
               break;
           default:
               ACDB_DEBUG_LOG("Dependent table not found to set!\n");
               return ACDB_ERROR;
           }
       }

	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetNoOfTblEntriesOnHeap(uint8_t *pCmd,uint32_t nCmdSize ,uint8_t *pRsp,uint32_t nRspSizse)
{
   int32_t result = ACDB_SUCCESS;
   if (pCmd == NULL || pRsp == NULL || nCmdSize == 0 || nRspSizse == 0)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->Invalid NULL value parameters are provided to get no of tbl entries\n");
	  return ACDB_ERROR;
   }
   result = GetNoOfTblEntriesOnHeap(pCmd,nCmdSize,pRsp,nRspSizse);

   return result;
}

int32_t AcdbCmdGetTblEntriesOnHeap(uint8_t *pCmd,uint32_t nCmdSize ,uint8_t *pRsp,uint32_t nRspSizse)
{
   int32_t result = ACDB_SUCCESS;
   if (pCmd == NULL || pRsp == NULL || nCmdSize == 0 || nRspSizse == 0)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->Invalid NULL value parameters are provided to get tbl entries\n");
	  return ACDB_ERROR;
   }
   result = GetTblEntriesOnHeap(pCmd,nCmdSize,pRsp,nRspSizse);

   return result;
}

int32_t AcdbCmdGetFeatureSupportedDevList(AcdbFeatureSupportedDevListCmdType *pCmd,AcdbQueryResponseType *pResp)
{
   int32_t result = ACDB_SUCCESS;
   uint32_t i = 0;
   int32_t offset = 0;
   uint32_t noOfDevs = 0;
   uint32_t noOfBytesRemaining = 0;
   uint32_t nParamId = 0;
   AcdbDevices *pDevs = NULL;
   if (pCmd == NULL || pResp == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->Invalid NULL value parameters are provided to get supported device list for a given feature\n");
	  return ACDB_BADPARM;
   }

   noOfBytesRemaining = pCmd->nBufferLength;

   switch(pCmd->nFeatureID)
   {
   case ACDB_FEEDBACK_SPEAKERPROTECTION_RX:
	   nParamId = IS_RX_DEV_FB_SPEAKER_PROT_ENABLED;
	   break;
   case ACDB_FEEDBACK_SPEAKERPROTECTION_TX:
	   nParamId = IS_TX_DEV_FB_SPEAKER_PROT_ENABLED;
	   break;
   case ACDB_ADAPTIVE_ANC_RX:
	   nParamId = IS_RX_DEV_ADAPTIVE_ANC_ENABLED;
	   break;
   default:
	   ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid feature id to get the feature supported device list\n");
	   return ACDB_BADPARM;
   }

   pDevs = (AcdbDevices *)ACDB_MALLOC(sizeof(AcdbDevices));
   if(pDevs == NULL)
   {
	   ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to allocate memory for AcdbDevices\n");
	   return ACDB_INSUFFICIENTMEMORY;
   }
   // Minimum provided buffer size should be 4 to atleast holds the no of devices value as zero
   // if none of the device support the given feature.
   if(noOfBytesRemaining < sizeof(noOfDevs))
   {
	   ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to get the list of supported devices for given feature\n");
	   ACDB_MEM_FREE(pDevs);
	   return ACDB_INSUFFICIENTMEMORY;
   }

   memset((void*)pDevs, 0, sizeof(AcdbDevices));

   result = acdbdata_ioctl (ACDBDATACMD_GET_DEVICE_LIST, (uint8_t *)pDevs, sizeof(AcdbDevices),
                          (uint8_t *)NULL, 0);
   if(result != ACDB_SUCCESS)
   {
	   ACDB_DEBUG_LOG("ACDB_COMMAND: No devices found.Please check if the correct set of acdb files are loaded.\n");
	   ACDB_MEM_FREE(pDevs);
	   return result;
   }
   offset += sizeof(noOfDevs);
   noOfBytesRemaining -= sizeof(noOfDevs);

   for(i=0;i<pDevs->noOfDevs;i++)
   {
	   AcdbDevPropInfo devPropInfo = {0};
	   uint32_t *pIsValid = NULL;
	   devPropInfo.devId = pDevs->devList[i];
	   devPropInfo.pId = nParamId;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
							  (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   continue;
	   }
	   if(devPropInfo.dataInfo.nDataLen != sizeof(uint32_t))
	   {
		   ACDB_DEBUG_LOG("The property %08X contains invalid data len for device %08X",nParamId,pDevs->devList[i]);
		   ACDB_MEM_FREE(pDevs);
		   return ACDB_BADPARM;
	   }
	   pIsValid = (uint32_t *)devPropInfo.dataInfo.pData;
	   if(*pIsValid != TRUE)
	   {
		   continue;
	   }
	   if(noOfBytesRemaining < sizeof(pDevs->devList[i]))
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to get the list of supported devices for given feature\n");
		   ACDB_MEM_FREE(pDevs);
		   return ACDB_INSUFFICIENTMEMORY;
	   }
	   noOfDevs += 1;
	   ACDB_MEM_CPY((void *)(pCmd->pBufferPointer+offset),noOfBytesRemaining,(void *)(&pDevs->devList[i]),sizeof(pDevs->devList[i]));
	   offset += sizeof(pDevs->devList[i]);
	   noOfBytesRemaining -= sizeof(pDevs->devList[i]);
   }
   if(noOfDevs == 0)
   {
	   ACDB_DEBUG_LOG("ACDB_COMMAND: No devices found with the requested feature enabled\n");
	   ACDB_MEM_FREE(pDevs);
	   return ACDB_ERROR;
   }
   else
   {
	   result = ACDB_SUCCESS;
   }
   ACDB_MEM_CPY((void *)(pCmd->pBufferPointer),sizeof(noOfDevs),(void *)(&noOfDevs),sizeof(noOfDevs));
   pResp->nBytesUsedInBuffer = offset;

   ACDB_MEM_FREE(pDevs);
   return result;
}

int32_t AcdbCmdGetDevPairList(AcdbDevicePairListCmdType *pCmd,AcdbQueryResponseType *pResp)
{
   int32_t result = ACDB_SUCCESS;
   int32_t offset = 0;
   uint32_t noOfDevs = 0;
   uint32_t noOfBytesRemaining = 0;
   uint32_t nParamId = 0;
   AcdbDevPropInfo devPropInfo = {0};
   uint32_t *pDevList = NULL;

   if (pCmd == NULL || pResp == NULL)
   {
        ACDB_DEBUG_LOG("[ACDB Command]->Invalid NULL value parameters are provided to get supported device list for a given feature\n");
	return ACDB_BADPARM;
   }

   noOfBytesRemaining = pCmd->nBufferLength;

   switch(pCmd->nDevPairType)
   {
   case ACDB_FEEDBACK_SPEAKERPROTECTION_RX2TX_LIST:
	   nParamId = FB_SPEAKER_PROT_RX_TX_DEV_PAIR;
	   break;
   case ACDB_VOICE_TX2RX_LIST:
	   nParamId = DEVPAIR;
	   break;
   case ACDB_ANC_RX2TX_LIST:
	   nParamId = ANCDEVPAIR;
	   break;
   case ACDB_NATIVE_REMOTE_MAP_LIST:
	   nParamId = APQ_MDM_COMP_DEVID;
	   break;
   case ACDB_AUDIO_RECORD_TX2RX_LIST:
	   nParamId = RECORDED_DEVICEPAIR;
	   break;
   case ACDB_ADAPTIVE_ANC_RX2TX_LIST:
	   nParamId = ADAPTIVE_ANCDEVPAIR;
	   break;
   default:
	   ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid devpair type to get the device pair list\n");
	   return ACDB_BADPARM;
   }

   // Minimum provided buffer size should be 4 to atleast holds the no of devices value as zero
   // if none of the device support the given feature.
   if(noOfBytesRemaining < sizeof(noOfDevs))
   {
	   ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to get the list of supported devices for given feature\n");
	   return ACDB_INSUFFICIENTMEMORY;
   }

   devPropInfo.devId = pCmd->nDeviceID;
   devPropInfo.pId = nParamId;
   result = acdbdata_ioctl (ACDBDATACMD_GET_DEVICE_PROP, (uint8_t *)&devPropInfo, sizeof(AcdbDevPropInfo),
						  (uint8_t *)NULL, 0);
   if(result != ACDB_SUCCESS)
   {
	   ACDB_DEBUG_LOG("Requested Device Pair not found");
	   return ACDB_ERROR;
   }
   if((devPropInfo.dataInfo.nDataLen == 0) || ((devPropInfo.dataInfo.nDataLen % 4)!=0))
   {
	   ACDB_DEBUG_LOG("Contains Invalid data for the device pair for the given device id %08X", pCmd->nDeviceID);
	   return ACDB_ERROR;
   }
   noOfDevs = devPropInfo.dataInfo.nDataLen/4;
   pDevList = (uint32_t *)devPropInfo.dataInfo.pData;

   if(noOfBytesRemaining < (devPropInfo.dataInfo.nDataLen + sizeof(noOfDevs)))
   {
	   ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to get the dev pair data\n");
	   return ACDB_INSUFFICIENTMEMORY;
   }

   ACDB_MEM_CPY((void *)(pCmd->pBufferPointer+offset),noOfBytesRemaining,(void *)(&noOfDevs),sizeof(noOfDevs));
   offset += sizeof(noOfDevs);
   ACDB_MEM_CPY((void *)(pCmd->pBufferPointer+offset),(noOfBytesRemaining-sizeof(noOfDevs)),(void *)(pDevList),devPropInfo.dataInfo.nDataLen);
   offset += devPropInfo.dataInfo.nDataLen;

   pResp->nBytesUsedInBuffer = offset;

   return result;
}

int32_t AcdbCmdGetLSMTblInfo(uint32_t queryType,uint8_t *pIn,uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetLSMTblInfo]->Invalid NULL value parameters are provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   LSMTblLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t nLookupKey;
	   LSMTblCmdLookupType lsmtblcmd;
	   uint32_t tblId = LSM_TBL;

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbLsmTableCmdType *pInput = (AcdbLsmTableCmdType *)pIn;
			   lsmtblcmd.nDevId = pInput->nDeviceId;
			   lsmtblcmd.nLSMAppTypeId = pInput->nMadApplicationType;
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbLsmTableSizeCmdType *pInput = (AcdbLsmTableSizeCmdType *)pIn;
			   lsmtblcmd.nDevId = pInput->nDeviceId;
			   lsmtblcmd.nLSMAppTypeId = pInput->nMadApplicationType;
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   cmd.devId = lsmtblcmd.nDevId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,lsmtblcmd.nDevId);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (LSMTblLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   LSM_LUT_INDICES_COUNT,&lsmtblcmd,LSM_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,lsmtblcmd.nDevId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   nLookupKey = (uint32_t)&lutTbl.pLut[index];
	   // Now get CDEF info
	   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset));
	   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

	   // Now get CDOT info
	   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset));
	   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));

	   if(cdefTbl.nLen != cdotTbl.nLen)
	   {
		   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n",lsmtblcmd.nDevId);
		   return ACDB_ERROR;
	   }

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbLsmTableCmdType *pInput = (AcdbLsmTableCmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
			   result = GetMidPidCalibTable(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,
				   pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
			   if(ACDB_SUCCESS != result)
			   {
				   return result;
			   }
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
			   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&pOutput->nSize))
			   {
				   return ACDB_ERROR;
			   }
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetCodecFeaturesData(AcdbCodecCalDataCmdType *pInput,
                                    AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetCodecFeaturesData]->System Erorr\n");
	  result = ACDB_BADPARM;
   }
   else if (pInput->pBufferPointer== NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetCodecFeaturesData]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   CDCFeaturesDataLookupTblType lutTbl;
	   uint32_t nLookupKey;
	   CDCFeaturesDataCmdLookupType cdcfdatacmd;
	   uint32_t tblId = CDC_FEATURES_TBL;

	   cdcfdatacmd.nDeviceId = pInput->nDeviceID;
	   cdcfdatacmd.nPID = pInput->nCodecFeatureType;

	   cmd.devId = cdcfdatacmd.nDeviceId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,cmd.devId);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (CDCFeaturesDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));
	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   CDC_FEATURES_DATA_LUT_INDICES_COUNT,&cdcfdatacmd,CDC_FEATURES_DATA_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,cmd.devId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   nLookupKey = (uint32_t)&lutTbl.pLut[index];
	   result = GetCalibData(tblId,nLookupKey,lutTbl.pLut[index].nDataOffset,tblInfo.dataPoolChnk,
		   pInput->pBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
	   if(ACDB_SUCCESS != result)
	   {
		   return result;
	   }
	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAdieSidetoneTblInfo(uint32_t queryType,uint8_t *pIn,uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAdieSidetoneTblInfo]->Invalid NULL value parameters are provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   AdieSidetoneLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t nLookupKey;
	   AdieSidetoneCmdLookupType lsmtblcmd;
	   uint32_t tblId = ADIE_SIDETONE_TBL;

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbAdieSidetoneTableCmdType *pInput = (AcdbAdieSidetoneTableCmdType *)pIn;
			   lsmtblcmd.nTxDevId = pInput->nTxDeviceId;
			   lsmtblcmd.nRxDevId = pInput->nRxDeviceId;
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   cmd.devId = lsmtblcmd.nTxDevId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,lsmtblcmd.nTxDevId);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (AdieSidetoneLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   ADST_LUT_INDICES_COUNT,&lsmtblcmd,ADST_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,lsmtblcmd.nTxDevId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   nLookupKey = (uint32_t)&lutTbl.pLut[index];
	   // Now get CDEF info
	   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset));
	   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

	   // Now get CDOT info
	   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset));
	   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));

	   if(cdefTbl.nLen != cdotTbl.nLen)
	   {
		   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n",lsmtblcmd.nTxDevId);
		   return ACDB_ERROR;
	   }

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbAdieSidetoneTableCmdType *pInput = (AcdbAdieSidetoneTableCmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
			   result = GetMidPidCalibTable(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,
				   pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
			   if(ACDB_SUCCESS != result)
			   {
				   return result;
			   }
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetAudioCOPPTopologyData(AcdbQueryCmdType *pInput,AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
		ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
		result = ACDB_BADPARM;
   }
   else
   {
	   AcdbGlbalPropInfo glbPropInfo = {0};
	   glbPropInfo.pId = AUDIO_COPP_TOPO_INFO_ID;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
							  (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n",glbPropInfo.pId);
		   return ACDB_DATA_NOT_FOUND;
	   }
	   if (NULL == glbPropInfo.dataInfo.pData)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n",glbPropInfo.pId);
		   return ACDB_DATA_NOT_FOUND;
	   }

	   if(pInput->nBufferLength < glbPropInfo.dataInfo.nDataLen)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to fill the audio copp topology info for pid %08X \n",glbPropInfo.pId);
		   return ACDB_INSUFFICIENTMEMORY;
	   }

	   ACDB_MEM_CPY(pInput->pBufferPointer,pInput->nBufferLength,glbPropInfo.dataInfo.pData,glbPropInfo.dataInfo.nDataLen);
	   pOutput->nBytesUsedInBuffer = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetAudioCOPPTopologyDataSize(AcdbSizeResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pOutput == NULL)
   {
		ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
		result = ACDB_BADPARM;
   }
   else
   {
	   AcdbGlbalPropInfo glbPropInfo = {0};
	   glbPropInfo.pId = AUDIO_COPP_TOPO_INFO_ID;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
							  (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n",glbPropInfo.pId);
		   return ACDB_ERROR;
	   }
	   if (NULL == glbPropInfo.dataInfo.pData)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n",glbPropInfo.pId);
		   return ACDB_ERROR;
	   }

	   pOutput->nSize = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetAudioPOPPTopologyData(AcdbQueryCmdType *pInput,AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
		ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
		result = ACDB_BADPARM;
   }
   else
   {
	   AcdbGlbalPropInfo glbPropInfo = {0};
	   glbPropInfo.pId = AUDIO_POPP_TOPO_INFO_ID;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
							  (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n",glbPropInfo.pId);
		   return ACDB_DATA_NOT_FOUND;
	   }
	   if (NULL == glbPropInfo.dataInfo.pData)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n",glbPropInfo.pId);
		   return ACDB_DATA_NOT_FOUND;
	   }

	   if(pInput->nBufferLength < glbPropInfo.dataInfo.nDataLen)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Insufficient memory provided to fill the audio copp topology info for pid %08X \n",glbPropInfo.pId);
		   return ACDB_INSUFFICIENTMEMORY;
	   }

	   ACDB_MEM_CPY(pInput->pBufferPointer,pInput->nBufferLength,glbPropInfo.dataInfo.pData,glbPropInfo.dataInfo.nDataLen);
	   pOutput->nBytesUsedInBuffer = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetAudioPOPPTopologyDataSize(AcdbSizeResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pOutput == NULL)
   {
		ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
		result = ACDB_BADPARM;
   }
   else
   {
	   AcdbGlbalPropInfo glbPropInfo = {0};
	   glbPropInfo.pId = AUDIO_POPP_TOPO_INFO_ID;
	   result = acdbdata_ioctl (ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
							  (uint8_t *)NULL, 0);
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n",glbPropInfo.pId);
		   return ACDB_ERROR;
	   }
	   if (NULL == glbPropInfo.dataInfo.pData)
	   {
		   ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n",glbPropInfo.pId);
		   return ACDB_ERROR;
	   }

	   pOutput->nSize = glbPropInfo.dataInfo.nDataLen;
   }
   return result;
}

int32_t AcdbCmdGetAANCTblCmnInfo(uint32_t queryType,uint8_t *pIn,uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetAANCTblCmnInfo]->Invalid NULL value parameters are provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;
	   AANCDataLookupTblType lutTbl;
	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t nLookupKey;
	   AANCCmdLookupType aanccfgcmd;
	   uint32_t tblId = AANC_CFG_TBL;

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbAANCConfigTableCmdType *pInput = (AcdbAANCConfigTableCmdType *)pIn;
			   aanccfgcmd.nDeviceId = pInput->nTxDeviceId;
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbAANCConfigTableSizeCmdType *pInput = (AcdbAANCConfigTableSizeCmdType *)pIn;
			   aanccfgcmd.nDeviceId = pInput->nTxDeviceId;
		   }
		   break;
	   case DATA_CMD:
		   {
			   AcdbAANCConfigDataCmdType *pInput = (AcdbAANCConfigDataCmdType *)pIn;
			   aanccfgcmd.nDeviceId = pInput->nTxDeviceId;
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   cmd.devId = aanccfgcmd.nDeviceId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,aanccfgcmd.nDeviceId);
		   return result;
	   }
	   lutTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
	   lutTbl.pLut = (AANCDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutTbl.nLen));

	   result = AcdbDataBinarySearch((void *)lutTbl.pLut,lutTbl.nLen,
		   AANC_CFG_LUT_INDICES_COUNT,&aanccfgcmd,AANC_CFG_CMD_INDICES_COUNT,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
			   ,aanccfgcmd.nDeviceId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

	   nLookupKey = (uint32_t)&lutTbl.pLut[index];
	   // Now get CDEF info
	   cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset));
	   cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

	   // Now get CDOT info
	   cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset));
	   cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));

	   if(cdefTbl.nLen != cdotTbl.nLen)
	   {
		   ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n",aanccfgcmd.nDeviceId);
		   return ACDB_ERROR;
	   }

	   switch(queryType)
	   {
	   case TABLE_CMD:
		   {
			   AcdbAANCConfigTableCmdType *pInput = (AcdbAANCConfigTableCmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
			   result = GetMidPidCalibTable(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,
				   pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
			   if(ACDB_SUCCESS != result)
			   {
				   return result;
			   }
		   }
		   break;
	   case TABLE_SIZE_CMD:
		   {
			   AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
			   if(ACDB_SUCCESS != GetMidPidCalibTableSize(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,&pOutput->nSize))
			   {
				   return ACDB_ERROR;
			   }
		   }
		   break;
	   case DATA_CMD:
		   {
			   AcdbAANCConfigDataCmdType *pInput = (AcdbAANCConfigDataCmdType *)pIn;
			   AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;
                           result = GetMidPidCalibData(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,pInput->nModuleId,
				   pInput->nParamId,pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
			   if(ACDB_SUCCESS != result)
			   {
				   return result;
			   }
		   }
		   break;
	   default:
		   return  ACDB_ERROR;
	   }

	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdGetDeviceProperty(uint32_t queryType,uint8_t *pIn,uint8_t *pOut)
{
   int32_t result = ACDB_SUCCESS;

   if (pIn == NULL || pOut == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceProperty]->Invalid NULL value parameters are provided\n");
      result = ACDB_BADPARM;
   }
   else
   {
      uint8_t *pOutBufPtr = NULL;
      uint32_t nOutBufLength = 0;
      //uint32_t nDeviceId = 0;
      AcdbDevPropertyType devPropID;
      AcdbDevPropInfo devPropInfo = {0};

      switch(queryType)
      {
      case DATA_CMD:
         {
            AcdbDevPropCmdType *pInput = (AcdbDevPropCmdType *)pIn;
            devPropInfo.devId = pInput->nDeviceId;
            devPropID = (AcdbDevPropertyType) pInput->nPropID;
            nOutBufLength = pInput->nBufferLength;
            pOutBufPtr = pInput->pBufferPointer;

            if(pOutBufPtr == NULL)
            {
               ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceProperty]->Payload buffer pointer is NULL\n");
              return ACDB_BADPARM;
            }
         }
         break;
      case DATA_SIZE_CMD:
         {
            AcdbDevPropSizeCmdType *pInput = (AcdbDevPropSizeCmdType *)pIn;
            devPropInfo.devId = pInput->nDeviceId;
            devPropID = (AcdbDevPropertyType)pInput->nPropID;
         }
         break;
      default:
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceProperty]->Invalid query %d\n",
            queryType);
            return  ACDB_ERROR;
         }
      }

      switch(devPropID)
      {
      case ACDB_AVSYNC_INFO:
         devPropInfo.pId = AV_SYNC_DELAY;
         break;
      default:
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceProperty]->Invalid device property ID %d\n",
            devPropID);
            return ACDB_ERROR;
         }
      }

      result = acdbdata_ioctl(
         ACDBDATACMD_GET_DEVICE_PROP,
         (uint8_t *)&devPropInfo,
         sizeof(AcdbDevPropInfo),
         (uint8_t *)NULL,
         0
      );
      if(result != ACDB_SUCCESS)
      {
         ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceProperty]->Failed to fetch the device property of the device %08X\n",
         devPropInfo.devId);
         return result;
      }

      switch(queryType)
      {
      case DATA_CMD:
         {
            AcdbQueryResponseType *pOutput = (AcdbQueryResponseType *)pOut;

            if (nOutBufLength < devPropInfo.dataInfo.nDataLen)
            {
               ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceProperty]->Insufficient buffer size to copy the device property data for device %08X\n",
               devPropInfo.devId);
               return ACDB_INSUFFICIENTMEMORY;
            }

            ACDB_MEM_CPY(pOutBufPtr,nOutBufLength,devPropInfo.dataInfo.pData,devPropInfo.dataInfo.nDataLen);
            pOutput->nBytesUsedInBuffer = devPropInfo.dataInfo.nDataLen;
         }
         break;
      case DATA_SIZE_CMD:
         {
            AcdbSizeResponseType *pOutput = (AcdbSizeResponseType *)pOut;
            pOutput->nSize = devPropInfo.dataInfo.nDataLen;
         }
         break;
      default:
         {
            ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetDeviceProperty]->Invalid query %d\n",queryType);
            result = ACDB_ERROR;
         }
      }
   }

   return result;
}

int32_t GetVP3InfoFromUseCaseId(int32_t useCaseId, uint32_t *tblId, int32_t *lutIndicesCount, int32_t *cmdIndicesCount, int32_t *tblIndicesCount)
{
    int32_t result = ACDB_SUCCESS;
    switch(useCaseId)
    {
    case ACDB_VP3_VOICE_USECASE:
        *tblId = VOICE_VP3_TBL;
        *lutIndicesCount = VOICE_VP3_LUT_INDICES_COUNT;
        *cmdIndicesCount = VOICE_VP3_CMD_INDICES_COUNT;
        *tblIndicesCount = VOICE_VP3TBL_INDICES_COUNT;
        break;
    case ACDB_VP3_AUDIO_REC_USECASE:
        *tblId = AUDIO_REC_VP3_TBL;
        *lutIndicesCount = AUDREC_VP3_LUT_INDICES_COUNT;
        *cmdIndicesCount = AUDREC_VP3_CMD_INDICES_COUNT;
        *tblIndicesCount = AUDREC_VP3TBL_INDICES_COUNT;
        break;
    case ACDB_VP3_AUDIO_WITH_EC_USECASE:
        *tblId = AUDIO_REC_EC_VP3_TBL;
        *lutIndicesCount = AUDREC_EC_VP3_LUT_INDICES_COUNT;
        *cmdIndicesCount = AUDREC_EC_VP3_CMD_INDICES_COUNT;
        *tblIndicesCount = AUDREC_EC_VP3TBL_INDICES_COUNT;
        break;
    default:
        result = ACDB_ERROR;
        break;
    }

    return result;
}

int32_t GetMaxLenPrpty(MaxLenDefPrptyType *maxLenPrpty)
{
    int32_t result = ACDB_SUCCESS;
    AcdbGlbalPropInfo glbPropInfo = {0};

    glbPropInfo.pId = MID_PID_MAX_LEN;

    result = acdbdata_ioctl (ACDBDATACMD_GET_GLOBAL_PROP, (uint8_t *)&glbPropInfo, sizeof(AcdbGlbalPropInfo),
        (uint8_t *)NULL, 0);
    if(result != ACDB_SUCCESS)
    {
        ACDB_DEBUG_LOG("ACDB_COMMAND: Failed to fetch the property info for pid %08X \n",glbPropInfo.pId);
        return ACDB_DATA_NOT_FOUND;
    }
    if (NULL == glbPropInfo.dataInfo.pData)
    {
        ACDB_DEBUG_LOG("ACDB_COMMAND: pData NULL on fetch the property info for pid %08X \n",glbPropInfo.pId);
        return ACDB_DATA_NOT_FOUND;
    }

    maxLenPrpty->nLen = *((uint32_t *)(glbPropInfo.dataInfo.pData));
    maxLenPrpty->pCntDef = (MaxLenDefType *)(glbPropInfo.dataInfo.pData + sizeof(maxLenPrpty->nLen));

    return result;
}

int32_t AcdbCmdGetVP3MidPidList(AcdbVP3MidPidListCmdType *pInput,
                                     AcdbQueryResponseType *pOutput)
{
    int32_t result = ACDB_SUCCESS;
    uint32_t i=0;
    uint32_t j=0;

    if (pInput == NULL || pOutput == NULL)
    {
        ACDB_DEBUG_LOG("ACDB_COMMAND: Provided invalid param\n");
        result = ACDB_BADPARM;
    }
    else
    {
        uint32_t index;
        uint32_t tblId = 0;
        int32_t lutIndicesCount = 0;
        int32_t cmdIndicesCount = 0;
        int32_t tblIndicesCount = 0;
        uint32_t dataOffset = 0;
        uint32_t copiedMaxLen = 0;
        MaxLenDefPrptyType maxLenPrpty;
        ContentDefTblType cdefTbl;
        AcdbTableCmd cmd;

        AcdbTableInfo tblInfo;
        int32_t tblQueryResult = ACDB_ERROR;

        result = GetMaxLenPrpty(&maxLenPrpty);
        if(result != ACDB_SUCCESS)
        {
            ACDB_DEBUG_LOG("ACDB_COMMAND: Could not get maxLen property\n");
            return result;
        }

        tblQueryResult = GetVP3InfoFromUseCaseId(pInput->nUseCaseId, &tblId, &lutIndicesCount, &cmdIndicesCount, &tblIndicesCount);

        if(tblQueryResult != ACDB_SUCCESS)
        {
            ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to find tblId for useCaseId %08X \n",pInput->nUseCaseId);
            return ACDB_INPUT_PARAMS_NOT_FOUND;
        }

        if(pInput->nUseCaseId == ACDB_VP3_AUDIO_REC_USECASE)
        {
            VP3DevDataLookupTblType lutDevTbl;
            VP3DevCmdLookupType vp3Devcmd;

            vp3Devcmd.nTxDevId = pInput->nTxDeviceId;

            cmd.devId = pInput->nTxDeviceId;
        cmd.tblId = tblId;
        result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
            (uint8_t *)&tblInfo,sizeof(tblInfo));
        if(result != ACDB_SUCCESS)
        {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
                    ,pInput->nTxDeviceId);
            return result;
        }
            lutDevTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
            lutDevTbl.pLut = (VP3DevDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutDevTbl.nLen));

            result = AcdbDataBinarySearch((void *)lutDevTbl.pLut,lutDevTbl.nLen,
                lutIndicesCount,&vp3Devcmd,cmdIndicesCount,&index);
        if(result != SEARCH_SUCCESS)
        {
            ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
                    ,pInput->nTxDeviceId);
            return ACDB_INPUT_PARAMS_NOT_FOUND;
        }

        // Now get CDEF info
            cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutDevTbl.pLut[index].nCDEFTblOffset));
            cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutDevTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));
        }
        else
        {
            VP3DevPairDataLookupTblType lutDevPairTbl;
            VP3DevPairCmdLookupType vp3DevPaircmd;

            vp3DevPaircmd.nRxDevId = pInput->nRxDeviceId;
            vp3DevPaircmd.nTxDevId = pInput->nTxDeviceId;

            cmd.devId = pInput->nTxDeviceId;
            cmd.tblId = tblId;
            result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
                (uint8_t *)&tblInfo,sizeof(tblInfo));
            if(result != ACDB_SUCCESS)
            {
                ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
                    ,pInput->nTxDeviceId);
                return result;
            }
            lutDevPairTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
            lutDevPairTbl.pLut = (VP3DevPairDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutDevPairTbl.nLen));

            result = AcdbDataBinarySearch((void *)lutDevPairTbl.pLut,lutDevPairTbl.nLen,
                lutIndicesCount,&vp3DevPaircmd,cmdIndicesCount,&index);
            if(result != SEARCH_SUCCESS)
            {
                ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
                    ,pInput->nTxDeviceId);
                return ACDB_INPUT_PARAMS_NOT_FOUND;
            }

            // Now get CDEF info
            cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutDevPairTbl.pLut[index].nCDEFTblOffset));
            cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutDevPairTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));
        }

        if(pInput->nBufferLength < cdefTbl.nLen * sizeof(MaxLenDefType))
        {
            ACDB_DEBUG_LOG("ACDB_COMMAND:[AcdbCmdGetVP3MidPidList]: Insufficient buffer size to copy VP3 MID PID data\n");
            return ACDB_INSUFFICIENTMEMORY;
        }

        dataOffset = 0;
        copiedMaxLen = 0;
        ACDB_MEM_CPY(pInput->nBufferPointer + dataOffset,pInput->nBufferLength - dataOffset,&cdefTbl.nLen,sizeof(cdefTbl.nLen));
        dataOffset += sizeof(cdefTbl.nLen);

        for(i=0;i<cdefTbl.nLen;i++)
        {
            for(j=0;j<maxLenPrpty.nLen;j++)
            {
                if(cdefTbl.pCntDef[i].nMid == maxLenPrpty.pCntDef[j].nMid &&
                    cdefTbl.pCntDef[i].nPid == maxLenPrpty.pCntDef[j].nPid )
                {
                    ACDB_MEM_CPY(pInput->nBufferPointer + dataOffset,pInput->nBufferLength - dataOffset,&maxLenPrpty.pCntDef[j].nMid,sizeof(maxLenPrpty.pCntDef[j].nMid));
                    dataOffset += sizeof(maxLenPrpty.pCntDef[j].nMid);

                    ACDB_MEM_CPY(pInput->nBufferPointer + dataOffset,pInput->nBufferLength - dataOffset,&maxLenPrpty.pCntDef[j].nPid,sizeof(maxLenPrpty.pCntDef[j].nPid));
                    dataOffset += sizeof(maxLenPrpty.pCntDef[j].nPid);

                    ACDB_MEM_CPY(pInput->nBufferPointer + dataOffset,pInput->nBufferLength - dataOffset,&maxLenPrpty.pCntDef[j].nMaxLen,sizeof(maxLenPrpty.pCntDef[j].nMaxLen));
                    dataOffset += sizeof(maxLenPrpty.pCntDef[j].nMaxLen);

                    copiedMaxLen++;
                }
            }
        }

        if(cdefTbl.nLen != copiedMaxLen)
        {
            ACDB_DEBUG_LOG("ACDB_COMMAND:[AcdbCmdGetVP3MidPidList]: Insufficient buffer size to copy VP3 MID PID data\n");
            return ACDB_ERROR;
        }

        pOutput->nBytesUsedInBuffer = dataOffset;
    }
    return result;
}

int32_t AcdbCmdGetVP3Data(AcdbVP3CmdType *pInput,AcdbQueryResponseType *pOutput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL || pOutput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVP3Data]->System Erorr\n");
      result = ACDB_BADPARM;
   }
   else if (pInput->nBufferPointer == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdGetVP3Data]->NULL pointer\n");
      result = ACDB_BADPARM;
   }
   else
   {
	   uint32_t index;
	   AcdbTableCmd cmd;
	   AcdbTableInfo tblInfo;

	   ContentDefTblType cdefTbl;
	   ContentDataOffsetsTblType cdotTbl;
	   uint32_t nLookupKey;

	   uint32_t tblId = 0;
       int32_t lutIndicesCount = 0;
       int32_t cmdIndicesCount = 0;
       int32_t tblIndicesCount = 0;
       int32_t tblQueryResult = ACDB_ERROR;

       tblQueryResult = GetVP3InfoFromUseCaseId(pInput->nUseCaseId, &tblId, &lutIndicesCount, &cmdIndicesCount, &tblIndicesCount);

       if(tblQueryResult != ACDB_SUCCESS)
       {
           ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to find tblId for useCaseId %08X \n",pInput->nUseCaseId);
           return ACDB_INPUT_PARAMS_NOT_FOUND;
       }

       if(pInput->nUseCaseId == ACDB_VP3_AUDIO_REC_USECASE)
       {
           VP3DevDataLookupTblType lutDevTbl;
           VP3DevCmdLookupType vp3Devcmd;

           vp3Devcmd.nTxDevId = pInput->nTxDeviceId;

           cmd.devId = vp3Devcmd.nTxDevId;
	   cmd.tblId = tblId;
	   result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
		   (uint8_t *)&tblInfo,sizeof(tblInfo));
	   if(result != ACDB_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
                   ,vp3Devcmd.nTxDevId);
		   return result;
	   }
           lutDevTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
           lutDevTbl.pLut = (VP3DevDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutDevTbl.nLen));

           result = AcdbDataBinarySearch((void *)lutDevTbl.pLut,lutDevTbl.nLen,
               lutIndicesCount,&vp3Devcmd,cmdIndicesCount,&index);
	   if(result != SEARCH_SUCCESS)
	   {
		   ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
                   ,vp3Devcmd.nTxDevId);
		   return ACDB_INPUT_PARAMS_NOT_FOUND;
	   }

           nLookupKey = (uint32_t)&lutDevTbl.pLut[index];
	   // Now get CDEF info
           cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutDevTbl.pLut[index].nCDEFTblOffset));
           cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutDevTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

	   // Now get CDOT info
           cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutDevTbl.pLut[index].nCDOTTblOffset));
           cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutDevTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));
       }
       else
       {
           VP3DevPairDataLookupTblType lutDevPairTbl;
           VP3DevPairCmdLookupType vp3DevPaircmd;

           vp3DevPaircmd.nTxDevId = pInput->nTxDeviceId;
           vp3DevPaircmd.nRxDevId = pInput->nRxDeviceId;

           cmd.devId = vp3DevPaircmd.nTxDevId;
           cmd.tblId = tblId;
           result = acdbdata_ioctl(ACDBDATACMD_GET_TABLE_INFO,(uint8_t *)&cmd,sizeof(cmd),
               (uint8_t *)&tblInfo,sizeof(tblInfo));
           if(result != ACDB_SUCCESS)
           {
               ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
                   ,vp3DevPaircmd.nTxDevId);
               return result;
           }
           lutDevPairTbl.nLen = *((uint32_t *)tblInfo.tblLutChnk.pData);
           lutDevPairTbl.pLut = (VP3DevPairDataLookupType *)(tblInfo.tblLutChnk.pData + sizeof(lutDevPairTbl.nLen));

           result = AcdbDataBinarySearch((void *)lutDevPairTbl.pLut,lutDevPairTbl.nLen,
               lutIndicesCount,&vp3DevPaircmd,cmdIndicesCount,&index);
           if(result != SEARCH_SUCCESS)
           {
               ACDB_DEBUG_LOG("Failed to fetch the lookup information of the device %08X \n"
                   ,vp3DevPaircmd.nTxDevId);
               return ACDB_INPUT_PARAMS_NOT_FOUND;
           }

           nLookupKey = (uint32_t)&lutDevPairTbl.pLut[index];
           // Now get CDEF info
           cdefTbl.nLen = *((uint32_t *)(tblInfo.tblCdftChnk.pData + lutDevPairTbl.pLut[index].nCDEFTblOffset));
           cdefTbl.pCntDef = (ContentDefType *)(tblInfo.tblCdftChnk.pData + lutDevPairTbl.pLut[index].nCDEFTblOffset + sizeof(cdefTbl.nLen));

           // Now get CDOT info
           cdotTbl.nLen = *((uint32_t *)(tblInfo.tblCdotChnk.pData + lutDevPairTbl.pLut[index].nCDOTTblOffset));
           cdotTbl.pDataOffsets = (uint32_t *)(tblInfo.tblCdotChnk.pData + lutDevPairTbl.pLut[index].nCDOTTblOffset + sizeof(cdotTbl.nLen));
       }

	   if(cdefTbl.nLen != cdotTbl.nLen)
	   {
           ACDB_DEBUG_LOG("The no of entries in CDEF and CDOT tables for devid %08X not matching\n",pInput->nTxDeviceId);
		   return ACDB_ERROR;
	   }

           result = GetMidPidCalibData(tblId,nLookupKey,cdefTbl,cdotTbl,tblInfo.dataPoolChnk,pInput->nModuleId,
		   pInput->nParamId,pInput->nBufferPointer,pInput->nBufferLength,&pOutput->nBytesUsedInBuffer);
	   if(ACDB_SUCCESS != result)
	   {
		   return result;
	   }

	   result = ACDB_SUCCESS;
   }

   return result;
}

int32_t AcdbCmdSetVP3Data(AcdbVP3CmdType *pInput)
{
   int32_t result = ACDB_SUCCESS;

   if (pInput == NULL)
   {
      ACDB_DEBUG_LOG("[ACDB Command]->[AcdbCmdSetAudProcData]->Invalid NULL value parameters are provided\n");
	  result = ACDB_BADPARM;
   }
   else
   {
       uint32_t j=0;
       uint32_t tblId=0;
       int32_t lutIndicesCount = 0;
       int32_t cmdIndicesCount = 0;
       int32_t tblIndicesCount = 0;
       int32_t tblQueryResult = ACDB_ERROR;
       MaxLenDefPrptyType maxLenPrpty;
       tblQueryResult = GetVP3InfoFromUseCaseId(pInput->nUseCaseId, &tblId, &lutIndicesCount, &cmdIndicesCount, &tblIndicesCount);

       if(tblQueryResult != ACDB_SUCCESS)
       {
           ACDB_DEBUG_LOG("ACDB_COMMAND: Unable to find tblId for useCaseId %08X \n",pInput->nUseCaseId);
           return ACDB_INPUT_PARAMS_NOT_FOUND;
       }

       result = GetMaxLenPrpty(&maxLenPrpty);
        if(result != ACDB_SUCCESS)
        {
            ACDB_DEBUG_LOG("ACDB_COMMAND: Could not get maxLen property\n");
            return result;
        }

        for(j=0;j<maxLenPrpty.nLen;j++)
        {
            if(pInput->nModuleId == maxLenPrpty.pCntDef[j].nMid &&
                pInput->nParamId == maxLenPrpty.pCntDef[j].nPid )
            {
                if(pInput->nBufferLength > maxLenPrpty.pCntDef[j].nMaxLen)
                {
                    return ACDB_BADPARM;
                }
                break;
            }
        }
       if(pInput->nUseCaseId == ACDB_VP3_AUDIO_REC_USECASE)
        {
           VP3DevCmdLookupType vp3Devcmd;
           vp3Devcmd.nTxDevId = pInput->nTxDeviceId;
           result = AcdbCmdSetOnlineData(tblId,(uint8_t *)&vp3Devcmd,tblIndicesCount,pInput->nModuleId,pInput->nParamId,pInput->nBufferPointer,pInput->nBufferLength);
        }
       else
           {
           VP3DevPairCmdLookupType vp3DevPaircmd;
           vp3DevPaircmd.nTxDevId = pInput->nTxDeviceId;
           vp3DevPaircmd.nRxDevId = pInput->nRxDeviceId;
           result = AcdbCmdSetOnlineData(tblId,(uint8_t *)&vp3DevPaircmd,tblIndicesCount,pInput->nModuleId,pInput->nParamId,pInput->nBufferPointer,pInput->nBufferLength);
       }
   }

   return result;
}

