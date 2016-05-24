/* =======================================================================
                               oscl_file_io.cpp
DESCRIPTION
  Meaningful description of the definitions contained in this file.
  Description must specify if the module is portable specific, mobile
  specific, or common to both, and it should alert the reader if the
  module contains any conditional definitions which tailors the module to
  different targets.  Include any initialization and synchronizing
  requirements.

EXTERNALIZED FUNCTIONS
  List functions and a brief description that are exported from this file

INITIALIZATION AND SEQUENCING REQUIREMENTS
  Detail how to initialize and use this service.  The sequencing aspect
  is only needed if the order of operations is important.

Copyright 2011-2012 Qualcomm Technologies, Inc., All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/FileBaseLib/main/latest/src/oscl_file_io.cpp#56 $
$DateTime: 2012/06/20 22:55:22 $
$Change: 2522041 $


========================================================================== */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
/*
    This header file contains OS specific functions necessary for
    the File Format library to compile and run on different OS's.
*/

/* Includes custmp4.h. The following 2 includes must be the first includes in this file! */
#include "parserdatadef.h"
#include "parserinternaldefs.h"
#include "AEEStdDef.h"
#include "MMFile.h"
#include "oscl_file_io.h"
#include "qcplayer_oscl_utils.h"
#include "zrex_string.h"
#include "MMCriticalSection.h"
#include "MMMalloc.h"
#include "MMDebugMsg.h"
#ifdef FEATURE_FILESOURCE_DRM_DCF
 #include "IxStream.h"
#endif

/* ==========================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains definitions for constants, macros, types, variables
and other items needed by this module.

========================================================================== */

/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Local Object Definitions
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Forward Declarations
** ----------------------------------------------------------------------- */

static int OSCL_File_Read_Internal(MM_HANDLE handle, char *pBuffer, int nSize, int *pnBytesRead);

/* =======================================================================
**                            Function Definitions
** ======================================================================= */
#ifdef FEATURE_FILESOURCE_DRM_DCF
/* ======================================================================
FUNCTION
  OSCL_FileOpen

DESCRIPTION
  To wrap IxStream for supporting DCF playback.

DEPENDENCIES
 IxStream/DRM suite

INPUT PARAMETERS:
 ->inputStream:IxStream* for DCF media

RETURN VALUE
 <-OSCL_FILE*

SIDE EFFECTS

========================================================================== */
OSCL_FILE * OSCL_FileOpen(IxStream* inputStream)
{
   OSCL_FILE *fp = MM_New( OSCL_FILE );
   if (fp != NULL)
   {
      memset(fp, 0x0, sizeof(OSCL_FILE));

      //ToDo: add File-IO-Cache support for IxStream
      //Set Stream type to be DRM_IXSTREAM
      fp->StreamType  = DRM_IXSTREAM;
      fp->inputStream = inputStream;
      fp->videoHandle = FILE_HANDLE_INVALD;
   }

   return fp;
}
#endif

/* ======================================================================
FUNCTION
  OSCL_FileOpen

DESCRIPTION
  To open the file and set the read cache buffer size with the given value
  or with the default value.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
OSCL_FILE *OSCL_FileOpen (const FILESOURCE_STRING &filename, const OSCL_TCHAR *mode,
                          int32 bufferSize )
{
  OSCL_FILE     *fp   = NULL;
  MM_HANDLE  file = FILE_HANDLE_INVALD;
  if(!mode)
  {
    return NULL;
  }
  int namelength =  filename.get_size()+1;
#ifdef USE_PARSER_WCHAR_ROUTINES
  int modelength =  zrex_wcslen((wchar_t*)mode)+1;
#else
  int modelength =  std_wstrlen((AECHAR*)mode)+1;
#endif
  if( (!namelength) || (!modelength) )
  {
    return NULL;
  }
  char* pCharMode = (char*)MM_Malloc(modelength);

  if( (!pCharMode) )
  {
    if(pCharMode)
    {
      MM_Free(pCharMode);
    }
    return NULL;
  }
#ifdef USE_PARSER_WCHAR_ROUTINES
  WideCharToChar(mode,zrex_wcslen((wchar_t*)mode),pCharMode,modelength);
#else
  WideCharToChar(mode,std_wstrlen((AECHAR*)mode),pCharMode,modelength);
#endif

  /* mapping the mode "c" to "w", as EFs2 doesn't have case to handle "c"  mode */
  if (strcmp(pCharMode, "c") == 0)
  {
    if(MM_File_CreateW((MM_WCHAR*)(filename.get_cstr()), *pCharMode, &file) != 0)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"fileopen failed");
    }
  }
  else if ( (strcmp(pCharMode, "w") == 0) || (strcmp(pCharMode, "wb") == 0) )
  {
    if(MM_File_CreateW((MM_WCHAR*)(filename.get_cstr()), MM_FILE_CREATE_W, &file) != 0)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"fileopen failed");
    }
  }
  else if ( (strcmp(pCharMode, "w+") == 0) || (strcmp(pCharMode, "wb+") == 0) )
  {
    if(MM_File_CreateW((MM_WCHAR*)(filename.get_cstr()), MM_FILE_CREATE_W_PLUS, &file) != 0)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"fileopen failed");
    }
  }
  else if ( (strcmp(pCharMode, "r+") == 0) || (strcmp(pCharMode, "rb+") == 0) )
  {
    if(MM_File_CreateW((MM_WCHAR*)(filename.get_cstr()), MM_FILE_CREATE_R_PLUS, &file) != 0)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"fileopen failed");
    }
  }
  else if (strcmp(pCharMode, "a") == 0)
  {
    if(MM_File_CreateW((MM_WCHAR*)(filename.get_cstr()), MM_FILE_CREATE_A, &file) != 0)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"fileopen failed");
    }
  }
  else
  {
    if(MM_File_CreateW((MM_WCHAR*)(filename.get_cstr()), MM_FILE_CREATE_R, &file) != 0)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"fileopen failed");
    }
  }

  if (file == FILE_HANDLE_INVALD)
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"efs_open failed .Efs Error No");
  }
  else
  {
    fp = MM_New( OSCL_FILE );
    if (fp == NULL)
    {
      MM_MSG_PRIO(MM_GENERAL, MM_PRIO_ERROR, "OSCL_File Open failed" );
      MM_File_Release( file );
    }
    else
    {
      memset(fp,0,sizeof(OSCL_FILE));
      fp->videoHandle = file;
      fp->bBufValid = false;
      fp->bPullBufValid = false;
#ifdef FEATURE_FILESOURCE_FILE_IO_CACHING
      if(( strcmp(pCharMode, "r") == 0 ) || ( strcmp(pCharMode, "rb") == 0 ))
      {
        fp->cacheValid = 1;
        if (bufferSize > 0)
        {
          /* Allocate required buffer */
          fp->readBuffSize = bufferSize;
        }
        else
        {
          /* if the required size is less than zero allocate with default size */
          fp->readBuffSize = FILE_READ_BUFFER_DEFAULT_SIZE;
        }
        /* Allocate required buffer */
        fp->readBuffer = (uint8 *)MM_Malloc( fp->readBuffSize );
        if (fp->readBuffer == NULL)
        {
          MM_MSG_PRIO(MM_GENERAL, MM_PRIO_FATAL, "OSCL_FileOpen, no enough memory" );
          fp->cacheValid = 0;
        }
        fp->fileReadPos = -1;
      }
      fp->readBuffPos = -1;
      fp->fileSize = 0;
      //Determine the file-size
      fp->fileSize = OSCL_FileSize( filename );
#endif //FEATURE_FILESOURCE_FILE_IO_CACHING
    }
  }
  MM_Free(pCharMode);
  return fp;
}

/* ======================================================================
FUNCTION
  OSCL_FileOpen

DESCRIPTION
  To open the file and set the read cache buffer size with the given value
  or with the default value.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
OSCL_FILE *OSCL_FileOpen (const char *filename, const char *mode,
                          int32 bufferSize )
{
  OSCL_FILE     *fp   = NULL;
  MM_HANDLE  file = FILE_HANDLE_INVALD;
  if(!mode)
  {
    return NULL;
  }
  int namelength =  std_strlen(filename);

  int modelength =  std_strlen(mode)+1;

  if( (!namelength) || (!modelength) )
  {
    return NULL;
  }


  /* mapping the mode "c" to "w", as EFs2 doesn't have case to handle "c"  mode */
  if (strcmp(mode, "c") == 0)
  {
    if(MM_File_Create(filename, *mode, &file) != 0)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"fileopen failed");
    }
  }
  else if ( (strcmp(mode, "w") == 0) || (strcmp(mode, "wb") == 0) )
  {
    if(MM_File_Create(filename, MM_FILE_CREATE_W, &file) != 0)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"fileopen failed");
    }
  }
  else if ( (strcmp(mode, "w+") == 0) || (strcmp(mode, "wb+") == 0) )
  {
    if(MM_File_Create(filename, MM_FILE_CREATE_W_PLUS, &file) != 0)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"fileopen failed");
    }
  }
  else if ( (strcmp(mode, "r+") == 0) || (strcmp(mode, "rb+") == 0) )
  {
    if(MM_File_Create(filename, MM_FILE_CREATE_R_PLUS, &file) != 0)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"fileopen failed");
    }
  }
  else if (strcmp(mode, "a") == 0)
  {
    if(MM_File_Create(filename, MM_FILE_CREATE_A, &file) != 0)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"fileopen failed");
    }
  }
  else
  {
    if(MM_File_Create(filename, MM_FILE_CREATE_R, &file) != 0)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"fileopen failed");
    }
  }

  if (file == FILE_HANDLE_INVALD)
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"efs_open failed .Efs Error No");
  }
  else
  {
    fp = MM_New( OSCL_FILE );
    if (fp == NULL)
    {
      MM_MSG_PRIO(MM_GENERAL, MM_PRIO_ERROR, "OSCL_File Open failed" );
      MM_File_Release( file );
    }
    else
    {
      memset(fp,0,sizeof(OSCL_FILE));
      fp->videoHandle = file;
      fp->bBufValid = false;
      fp->bPullBufValid = false;
#ifdef FEATURE_FILESOURCE_FILE_IO_CACHING
      if(( strcmp(mode, "r") == 0 ) || ( strcmp(mode, "rb") == 0 ))
      {
        fp->cacheValid = 1;
        if (bufferSize > 0)
        {
          /* Allocate required buffer */
          fp->readBuffSize = bufferSize;
        }
        else
        {
          /* if the required size is less than zero allocate with default size */
          fp->readBuffSize = FILE_READ_BUFFER_DEFAULT_SIZE;
        }
        /* Allocate required buffer */
        fp->readBuffer = (uint8 *)MM_Malloc( fp->readBuffSize );
        if (fp->readBuffer == NULL)
        {
          MM_MSG_PRIO(MM_GENERAL, MM_PRIO_FATAL, "OSCL_FileOpen, no enough memory" );
          fp->cacheValid = 0;
        }
        fp->fileReadPos = -1;
      }
      fp->readBuffPos = -1;
      fp->fileSize = 0;
      //Determine the file-size

      fp->fileSize = OSCL_FileSize( filename );
#endif //FEATURE_FILESOURCE_FILE_IO_CACHING
    }
  }

  return fp;
}


#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
/* ======================================================================
FUNCTION
  OSCL_FileOpen

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
OSCL_FILE *OSCL_FileOpen(video::iStreamPort* pPort)
{
  OSCL_FILE *fp = NULL;
  if(!pPort)
  {
     return fp;
  }
  fp = MM_New_Args(OSCL_FILE,());
  if(fp == NULL)
  {
    return NULL;
  }
  fp->StreamType = ISTREAM_PORT;
  fp->pStreamPort = pPort;
  fp->bBufValid = false;
  fp->bPullBufValid = false;
  fp->videoHandle = FILE_HANDLE_INVALD;
  //fp->pReadCriticalSection = ptrCS;
  fp->fileReadPos = -1;
  fp->readBuffPos = -1;
  return fp;
}
#endif
/* ======================================================================
FUNCTION
  OSCL_FileDelete

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
bool OSCL_FileDelete(const FILESOURCE_STRING &filename)
{
  bool bRet = false;
  int namelength =   filename.get_size()+1;
  char* pCharTempName = (char*)MM_Malloc(namelength);
  if(pCharTempName)
  {
    WideCharToChar(filename,filename.get_size(),pCharTempName,namelength);
  }
  if(pCharTempName)
  {
    if(MM_File_Delete(pCharTempName) != 0 )
    {
      MM_MSG_SPRINTF_PRIO_1( MM_GENERAL, MM_PRIO_ERROR,"Failed to Delete file --> %s",
                              pCharTempName);
      bRet = false;
    }
    else
    {
      MM_MSG_SPRINTF_PRIO_1( MM_GENERAL, MM_PRIO_HIGH,"Deleted file --> %s ",
                              pCharTempName);
      bRet = true;
    }
  }
    if(pCharTempName)
    {
      MM_Free(pCharTempName);
    }
  return bRet;
}

/* ======================================================================
FUNCTION
  OSCL_FileDelete

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
bool OSCL_FileDelete(const char *filename)
{
  bool bRet = false;

  if(filename)
  {
    if(MM_File_Delete(filename) != 0 )
    {
      MM_MSG_SPRINTF_PRIO_1( MM_GENERAL, MM_PRIO_ERROR,"Failed to Delete file --> %s",
                              filename);
      bRet = false;
    }
    else
    {
      MM_MSG_SPRINTF_PRIO_1( MM_GENERAL, MM_PRIO_HIGH,"Deleted file --> %s ",
                              filename);
      bRet = true;
    }
  }

  return bRet;
}



bool OSCL_FileRename (
    const char                  *old_filename,    /* Current name of file */
    const char                  *new_filename     /* New name of file */
)
{
  bool nRet = false;
  if(MM_File_Move(old_filename, new_filename) != 0)
  {
    MM_MSG_PRIO(MM_GENERAL, MM_PRIO_FATAL, "video_File_Rename failed" );
  }
  else
  {
    MM_MSG_PRIO(MM_GENERAL, MM_PRIO_FATAL, "video_File_Rename successful" );
    nRet = true;
  }
  return nRet;
}

/* ======================================================================
FUNCTION
  OSCL_FileOpen

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
OSCL_FILE *OSCL_FileOpen(unsigned char *pBuf, uint64 bufSize,uint32 trackId)
{
  OSCL_FILE *fp = NULL;

  if ( (pBuf && bufSize ) )
  {
    fp = MM_New( OSCL_FILE );
    if(fp == NULL) {
       MM_MSG_PRIO(MM_GENERAL, MM_PRIO_FATAL, "OSCL_File Open failed" );
       return NULL;
    }
    memset( fp, 0, sizeof(OSCL_FILE) );
    fp->videoHandle = FILE_HANDLE_INVALD;
    fp->bBufValid = true;
    fp->bPullBufValid = false;
    fp->memBuf.pBuf = pBuf;
    fp->memBuf.bufSize = bufSize;
    fp->memBuf.curPos = 0;
  }
  return fp;
}
/* ======================================================================
FUNCTION
  OSCL_IsFileOpen

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
bool OSCL_IsFileOpen(OSCL_FILE *fp)
{
  bool retstatus = false;
  if( fp )
  {
    if( fp->bBufValid || fp->bPullBufValid)
    {
      retstatus = true;
    }
#ifdef FEATURE_FILESOURCE_DRM_DCF
    else
    {
      switch (fp->StreamType)
      {
      case DRM_IXSTREAM:
        //For IxStream input, it is always open!
        retstatus = true;
        break;
      }
    }
#endif
  }
  return retstatus;
}

/* ======================================================================
FUNCTION
  OSCL_FileRead

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
uint32  OSCL_FileRead (void *buffer, uint32 size,
                       uint32 numelements,  OSCL_FILE *fp)
{
  int numRead=0;

  /* sanity check */
  if (!fp || !buffer || !(size*numelements))
    return 0;

  if(fp->pCriticalSection)
    MM_CriticalSection_Enter(fp->pCriticalSection);

  if( fp->videoHandle != FILE_HANDLE_INVALD )
  {
    //fs_rsp_msg_type rsp;
#ifdef FEATURE_FILESOURCE_FILE_IO_CACHING
    if (fp->cacheValid)
    {
      int32 reqSizeToRead = size*numelements;

      /* sanity check on local read buffer */
      if (fp->readBuffer == NULL)
      {
        MM_MSG_PRIO(MM_GENERAL, MM_PRIO_FATAL, "OSCL_FileOpen, local read buffer is invalid" );
        if(fp->pCriticalSection)
          MM_CriticalSection_Leave(fp->pCriticalSection);
        return 0;
      }
      /* EOF check */
      if (fp->fileReadPos >= fp->fileSize)
      {
        MM_MSG_PRIO2(MM_GENERAL, MM_PRIO_FATAL, "OSCL_FileRead, Invalid fileReadPos fp->fileReadPos >= fp->fileSize fp->fileReadPos  %lld   fp->fileSize %lld", fp->fileReadPos, fp->fileSize);
        if(fp->pCriticalSection)
          MM_CriticalSection_Leave(fp->pCriticalSection);
        return 0;
      }
      if ((fp->fileReadPos + reqSizeToRead) >= fp->fileSize)
      {
        reqSizeToRead = (int32)fp->fileSize - (int32)fp->fileReadPos - 1;
      }

      /* if the read buffer is valid */
      if (fp->readBuffPos != -1)
      {
        //Read from the local buffer if the sufficient data is available.
        if ((fp->readBuffSize - fp->readBuffPos) >= reqSizeToRead)
        {
          //Read from the local buffer.
          memcpy(buffer, fp->readBuffer+fp->readBuffPos, reqSizeToRead);
          numRead = reqSizeToRead;
          fp->fileReadPos += reqSizeToRead;
          fp->readBuffPos += reqSizeToRead;
        }
        else
        {
          //Seek the original fptr to the correct location within the file.
          if( MM_File_SeekEx ( fp->videoHandle, fp->fileReadPos+1, SEEK_SET ) < 0 )
          {
            MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "MM_File_Seek failed. FS Status=");
          }
          else
          {
            if(!(OSCL_File_Read_Internal ( fp->videoHandle, (char*)buffer, reqSizeToRead, &numRead )))
            {
              fp->fileReadPos += numRead;
            }
          }
          //Declaring that the read-buffer is empty.
          fp->readBuffPos = -1;
        }
      }
      else
      {
        //Seek the original fptr to the correct location within the file.
        if( MM_File_SeekEx ( fp->videoHandle, fp->fileReadPos+1, SEEK_SET ) == 0 )
        {
          //Read into the buffer OR read directly into the supplied buff.
          if ((reqSizeToRead) > (fp->readBuffSize>>2))
          {
            if(!(OSCL_File_Read_Internal ( fp->videoHandle, (char*)buffer, reqSizeToRead, &numRead )))
            {
              fp->fileReadPos += numRead;
              //Declaring that the read-buffer is empty.
              fp->readBuffPos = -1;
            }
          }
          else
          {
            //Read directly from the file into Local buffer, filling it.
            OSCL_File_Read_Internal ( fp->videoHandle, (char*)fp->readBuffer, fp->readBuffSize, &numRead );
            if( numRead > 0 )
            {
                reqSizeToRead = FILESOURCE_MIN(reqSizeToRead,numRead);
                if(reqSizeToRead)
                {
                    memcpy(buffer, fp->readBuffer, reqSizeToRead);
                    numRead = reqSizeToRead;
                    fp->fileReadPos += numRead;
                    fp->readBuffPos += (reqSizeToRead+1);
                }
            }
          }
        }
        else
        {
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "MM_File_Seek failed. FS Status=");
        }
      }
    }
    else
#endif //FEATURE_FILESOURCE_FILE_IO_CACHING
    {
      OSCL_File_Read_Internal ( fp->readBuffer, (char*)buffer, fp->readBuffSize, &numRead );
    }
  }
  else if (fp->bBufValid)
  {
    int32 sizeToRead = (int32)FILESOURCE_MIN((fp->memBuf.bufSize-fp->memBuf.curPos), size*numelements);
    if (sizeToRead > 0)
    {
      memcpy(buffer, (fp->memBuf.pBuf+fp->memBuf.curPos), sizeToRead);
      fp->memBuf.curPos = fp->memBuf.curPos + sizeToRead;
      numRead = sizeToRead;
    }
  }
#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
else if(fp->pStreamPort)
  {
    switch (fp->StreamType)
    {
      case ISTREAM_PORT:
        #ifdef FEATURE_FILESOURCE_FILE_IO_CACHING
          if(fp->cacheValid)
          {
            uint32 reqSizeToRead = size*numelements;
            /* EOF check */
            if((fp->fileReadPos + (int32)reqSizeToRead) >= fp->fileSize)
            {
              reqSizeToRead = (int32)fp->fileSize - (int32)fp->fileReadPos - 1;
            }
            /* if the read buffer is valid */
            if(fp->readBuffPos != -1)
            {
              //Read from the local buffer if the sufficient data is available.
              if((fp->readBuffSize - fp->readBuffPos) >= reqSizeToRead)
              {
                //Read from the local buffer.
                memcpy(buffer, fp->readBuffer+fp->readBuffPos, reqSizeToRead);
                numRead = reqSizeToRead;
                fp->fileReadPos += reqSizeToRead;
                fp->readBuffPos += reqSizeToRead;
              }
              else
              {
                //Seek the original fptr to the correct location within the file.
                int64 nRetOffset = 0;
                if(fp->pStreamPort->Seek((fp->fileReadPos+1),video::iStreamPort::DS_SEEK_SET,&nRetOffset)== video::iStreamPort::DS_SUCCESS)
                {
                  //Read directly from the file into target buffer.
                  {
                    fp->pStreamPort->Read((unsigned char*)buffer, reqSizeToRead, (int*)&numRead);
                  }
                  fp->fileReadPos += numRead;
                  if(!numRead)
                  {
                    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "fp->pFileReader->Read failed.");
                  }
                }
                else
                {
                  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "fp->pFileReader->Seek.");
                }

                //Declaring that the read-buffer is empty.
                fp->readBuffPos = -1;
              }
            }//if(fp->readBuffPos != -1)
            else
            {
              //Read into the buffer OR read directly into the supplied buff.
              if(((int32)reqSizeToRead) > (fp->readBuffSize>>2))
              {
                //Read directly from the file into target buffer.
                {
                  fp->pStreamPort->Read((unsigned char*)buffer, reqSizeToRead, (int*)&numRead);
                }
                if(numRead)
                {
                  fp->fileReadPos += numRead;
                  //Declaring that the read-buffer is empty.
                  fp->readBuffPos = -1;
                }
                else
                {
                  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "fp->pFileReader->Read failed.");
                }
              }
              else
              {
                //Read directly from the file into Local buffer, filling it.
                {
                  fp->pStreamPort->Read((byte*)fp->readBuffer, fp->readBuffSize, (int*)&numRead);
                }
                if(numRead)
                {
                  //Read from the local buffer for the first time (if we fetched some data).
                  reqSizeToRead = FILESOURCE_MIN((int32)reqSizeToRead,numRead);
                  if(reqSizeToRead)
                  {
                    memcpy(buffer, fp->readBuffer, reqSizeToRead);
                    numRead = reqSizeToRead;
                    fp->fileReadPos += reqSizeToRead;
                    fp->readBuffPos += (reqSizeToRead+1);
                  }
                }
                else
                {
                  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "fs_read failed");
                }
              }
            }
          }
          else
        #endif //FEATURE_FILESOURCE_FILE_IO_CACHING
          {
            //Read from FilePort
            uint32 reqSizeToRead = size*numelements;
            numRead = 0;
            fp->pStreamPort->Read((unsigned char*)buffer, reqSizeToRead, (int*)&numRead);
          }
        break;
      default:
        break;
    }
  }
#endif
#ifdef FEATURE_FILESOURCE_DRM_DCF
  else
  {
    switch (fp->StreamType)
    {
    case DRM_IXSTREAM:
      //Read from IxStream
      uint32 reqSizeToRead = size*numelements;
      numRead = 0;
      bool   isEndofStream = false;

      if (fp->inputStream != NULL)
      {
        IxErrnoType errorCode = E_FAILURE;
        if(fp->pCriticalSection)
        {
          MM_CriticalSection_Enter(fp->pCriticalSection);
        }
        errorCode =  ((IxStream*)fp->inputStream)->Read((byte*)buffer,
                                                        (uint32)reqSizeToRead,
                                                        (uint32*)&numRead,
                                                        &isEndofStream);
        if(fp->pCriticalSection)
        {
          MM_CriticalSection_Leave(fp->pCriticalSection);
        }

        if (isEndofStream == true)
        {
          MM_MSG_PRIO(MM_GENERAL, MM_PRIO_FATAL, "****$$$$OSCL_FileRead DETECTED isEndofStream TRUE$$$$$****");
        }

        if ((errorCode == E_SUCCESS) && (numRead == 0))
        {
          MM_MSG_PRIO(MM_GENERAL, MM_PRIO_HIGH, "??????OSCL_FileRead detected DATA_UNDERRUN");
        }
        else if (errorCode != E_SUCCESS)
        {
          //read failure,
          MM_MSG_PRIO1(MM_GENERAL, MM_PRIO_FATAL, "OSCL_FileRead failed..error code %d",errorCode );
        }
      }
      break;
    }
  }
#endif
  if(fp->pCriticalSection)
    MM_CriticalSection_Leave(fp->pCriticalSection);

  return(numRead);
}

/* ======================================================================
FUNCTION
  OSCL_FileSeekRead

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
uint32 OSCL_FileSeekRead ( void *buffer, uint32 size, uint32 numelements,
                          OSCL_FILE *fp,  uint64 offset, int32 origin )
{
  int numRead = 0;

  /* sanity check */
  if( !fp || !buffer || !(size*numelements) )
    return 0;

  if(fp->pCriticalSection)
    MM_CriticalSection_Enter(fp->pCriticalSection);

  if (fp->videoHandle != FILE_HANDLE_INVALD)
  {
#ifdef FEATURE_FILESOURCE_FILE_IO_CACHING
    if(fp->cacheValid)
    {
      switch ( origin )
      {
      case SEEK_SET:

        //SEEK_SET - Set to 'offset'
        /* if the read buffer is valid */
        if(fp->readBuffPos != -1)
        {
          /* if the offset is greater than the current file read position
          and less than the maximum valid read buffer */
          if(((int64)offset > fp->fileReadPos) && ((int64)offset<= (fp->fileReadPos + fp->readBuffSize - fp->readBuffPos)))
          {
            fp->readBuffPos += (offset - fp->fileReadPos -1);
          }
          /* if the offset less than/equal to file read position and also is greater
          than or equal to the minimum valid read buffer */
          else if(((int64)offset<= fp->fileReadPos) && ((int64)offset>((fp->fileReadPos - fp->readBuffPos) + 1 )))
          {
            fp->readBuffPos = offset - (( fp->fileReadPos - fp->readBuffPos )+ 1);
          }
          /* if the offset is not within the range of valid read buffer then flush the read buffer */
          else
          {
            fp->readBuffPos = -1;
          }
        }
        fp->fileReadPos = (int64)offset-1;
        break;

      case SEEK_CUR:

        //SEEK_CUR - Set to 'offset' + current position
        if(fp->readBuffPos != -1)
        {
          if((fp->readBuffPos + (int64)offset) >= (int64)fp->readBuffSize)
          {
            fp->readBuffPos = -1;
          }
          else
          {
            fp->readBuffPos += offset;
          }
        }
        fp->fileReadPos += offset;
        break;

      case SEEK_END:

        //SEEK_END - Set to 'offset' + file size
        fp->fileReadPos = fp->fileSize + offset -1;
        fp->readBuffPos = -1;
        break;

      default:
        /* This is an error*/
        if(fp->pCriticalSection)
          MM_CriticalSection_Leave(fp->pCriticalSection);
        return 0;
      }
      numRead = OSCL_FileRead(buffer, size, numelements, fp);
    }
    else
#endif /* FEATURE_FILESOURCE_FILE_IO_CACHING */
    {
      if( MM_File_SeekEx ( fp->videoHandle, offset, origin ) < 0 )
      {
        MM_MSG_PRIO( MM_FILE_OPS, MM_PRIO_ERROR, "MM_File_Seek failed. FS Status");
      }
      else
      {
        OSCL_File_Read_Internal(fp->videoHandle, (char*)buffer, size * numelements, &numRead);
      }
    }
  }
  else if ( fp->bBufValid )
  {
    int64 npos;
    switch ( origin )
    {
    case SEEK_CUR:
      npos = offset + fp->memBuf.curPos;
      break;

    case SEEK_END:
      npos = offset + fp->memBuf.bufSize;
      break;

    case SEEK_SET:
      npos = offset;
      break;

    default:
      npos = (-1); /* error */
      break;
    }
    if ( (npos>=0) && (npos<=(int32)fp->memBuf.bufSize) )
    {
      fp->memBuf.curPos = npos;
      /* now read from this location */
      int32 sizeToRead = (int32)FILESOURCE_MIN((fp->memBuf.bufSize-fp->memBuf.curPos), size*numelements);
      if ( sizeToRead > 0 )
      {
        memcpy(buffer, (fp->memBuf.pBuf+fp->memBuf.curPos), sizeToRead);
        fp->memBuf.curPos = fp->memBuf.curPos + sizeToRead;
        numRead = sizeToRead;
      }
    }
  }
#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
  else if(fp->pStreamPort)
  {
    switch (fp->StreamType)
    {
    case ISTREAM_PORT:
#ifdef FEATURE_FILESOURCE_FILE_IO_CACHING
      if(fp->cacheValid)
      {
        int64 fpReadPos = fp->fileReadPos;
        int64 rdBuffPos = fp->readBuffPos;
        switch ( origin )
        {
        case SEEK_SET:
          //SEEK_SET - Set to 'offset'
          /* if the read buffer is valid */
          if(fp->readBuffPos != -1)
          {
            /* if the offset is greater than the current file read position
            and less than the maximum valid read buffer */
            if(((int64)offset > fp->fileReadPos) && ((int64)offset<= (fp->fileReadPos + fp->readBuffSize - fp->readBuffPos)))
            {
              fp->readBuffPos += (offset - fp->fileReadPos -1);
            }
            /* if the offset less than/equal to file read position and also is greater
            than or equal to the minimum valid read buffer */
            else if(((int64)offset<= fp->fileReadPos) && ((int64)offset>((fp->fileReadPos - fp->readBuffPos) + 1 )))
            {
              fp->readBuffPos = offset - (( fp->fileReadPos - fp->readBuffPos )+ 1);
            }
            /* if the offset is not within the range of valid read buffer then flush the read buffer */
            else
            {
              fp->readBuffPos = -1;
            }
          }
          fp->fileReadPos = (int64)offset-1;
          break;

        case SEEK_CUR:
          //SEEK_CUR - Set to 'offset' + current position
          if(fp->readBuffPos != -1)
          {
            if((fp->readBuffPos + (int64)offset) >= (int64)fp->readBuffSize)
            {
              fp->readBuffPos = -1;
            }
            else
            {
              fp->readBuffPos += offset;
            }
          }
          fp->fileReadPos += offset;
          break;

        case SEEK_END:
          //SEEK_END - Set to 'offset' + file size
          fp->fileReadPos = fp->fileSize + offset -1;
          fp->readBuffPos = -1;
          break;

        default:
          /* This is an error.*/
          if(fp->pCriticalSection)
          {
            MM_CriticalSection_Leave(fp->pCriticalSection);
          }
          return 0;
        }//switch ( origin )

        int64 nRetOffset = 0;
        if(fp->pStreamPort->Seek(offset,origin,&nRetOffset)== video::iStreamPort::DS_SUCCESS)
        {
          numRead = OSCL_FileRead(buffer, size, numelements, fp);
        }
        else
        {
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "fp->pFileReader->Seek failed..");
          fp->fileReadPos = fpReadPos;
          fp->readBuffPos = rdBuffPos;
        }
      }//if(fp->cacheValid)
      else
#endif /* FEATURE_FILESOURCE_FILE_IO_CACHING */
      {
        // Seek filePort then Read from FilePort
        int64 nRetOffset = 0;
        if(fp->pStreamPort->Seek(offset,origin,&nRetOffset)== video::iStreamPort::DS_SUCCESS)
        {
          uint32 reqSizeToRead = size*numelements;
          fp->pStreamPort->Read((unsigned char*)buffer, reqSizeToRead, (int*)&numRead);
        }
      }
      break;
    default:
      break;
    }//switch (fp->StreamType)
  }
#endif
#ifdef FEATURE_FILESOURCE_DRM_DCF
  else
  {
    switch (fp->StreamType)
    {
    case DRM_IXSTREAM:
      if (fp->inputStream != NULL)
      {
        // Seek IxStream then Read from IxStream
        IxStreamWhenceType fs_origin;
        switch ( origin )
        {
        case SEEK_SET:
          fs_origin = IX_STRM_SEEK_START;
          break;

        case SEEK_CUR:
          fs_origin = IX_STRM_SEEK_CURRENT;
          break;

        case SEEK_END:
          fs_origin = IX_STRM_SEEK_END;
          break;

        default:
          // This is an error
          if(fp->pCriticalSection)
          {
            MM_CriticalSection_Leave(fp->pCriticalSection);
          }
          return 0;
        }
        IxErrnoType errorCode = E_FAILURE;
        errorCode = ((IxStream*)fp->inputStream)->Seek(offset,fs_origin);
        if(errorCode == E_SUCCESS)
        {
          uint32 reqSizeToRead = size*numelements;
          numRead = 0;
          bool   isEndofStream = false;
          errorCode = E_FAILURE;
          errorCode =  ((IxStream*)fp->inputStream)->Read((byte*)buffer,
            (uint32)reqSizeToRead,
            (uint32*)&numRead,
            &isEndofStream);
          if( (errorCode == E_SUCCESS) && (numRead == 0) )
          {
            MM_MSG_PRIO(MM_GENERAL, MM_PRIO_HIGH, "OSCL_FileRead detected DATA_UNDERRUN");
          }
          else if(errorCode != E_SUCCESS)
          {
            //read failure,
            MM_MSG_PRIO1(MM_GENERAL, MM_PRIO_FATAL, "OSCL_FileRead failed..error code %d",errorCode );
          }
        }
        else
        {
          MM_MSG_PRIO1(MM_GENERAL, MM_PRIO_FATAL, "OSCL_FileSeekRead:IxStream->Seek failed..error code %d",errorCode );
        }
      }//if (fp->inputStream != NULL)
      break;
    }//switch (fp->streamType)
  }
#endif
  if(fp->pCriticalSection)
    MM_CriticalSection_Leave(fp->pCriticalSection);
  return(numRead);
}

/* ======================================================================
FUNCTION
  OSCL_FileSeekWrite

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
uint32 OSCL_FileSeekWrite (OSCL_FILE *fp, int32 origin, uint64 offset,
                           void *buffer, uint32 size, uint32 numelements)
{
   int numWritten = 0;
   // sanity check
   if( !fp || !buffer || !(size*numelements) )
     return 0;

  if( fp->videoHandle != FILE_HANDLE_INVALD)
  {
    if( !( MM_File_SeekEx ( fp->videoHandle, offset, origin ) < 0 ) )
    {
      MM_File_Write(fp->videoHandle, (char*)buffer, size*numelements, &numWritten );
#ifdef FEATURE_FILESOURCE_FILE_IO_CACHING
      fp->fileSize += numWritten;
#endif
    }
  }
  return numWritten;
}

/* ======================================================================
FUNCTION
  OSCL_FileWrite

DESCRIPTION
  Functions seeks to the file offset specified and write
  the data in the buffer starting from that offset.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
uint32  OSCL_FileWrite (void *buffer, uint32 size,
                        uint32 numelements,  OSCL_FILE *fp)
{
  int numWritten=0;

  /* sanity check */
  if (!fp || !buffer || !(size*numelements))
    return 0;

  if (fp->videoHandle != FILE_HANDLE_INVALD)
  {
    MM_File_Write(fp->videoHandle, (char*)buffer, size*numelements, &numWritten );
    if( numWritten == 0 )
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "MM_File_Write failed. FS Status");
    }
    else
      fp->fileSize += numWritten;
  }
  else if (fp->bBufValid)
  {
    int32 sizeToWrite = (int32)FILESOURCE_MIN((fp->memBuf.bufSize-fp->memBuf.curPos), size*numelements);
    if (sizeToWrite > 0)
    {
      memcpy((fp->memBuf.pBuf+fp->memBuf.curPos), buffer, sizeToWrite);
      fp->memBuf.curPos = fp->memBuf.curPos + sizeToWrite;
      numWritten = sizeToWrite;
    }
  }
  return(numWritten);
}

/* ======================================================================
FUNCTION
  OSCL_FileSeek

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
int64 OSCL_FileSeek (OSCL_FILE *fp,  uint64 offset, int32 origin)
{
  int32  nReturn = (-1);

  /* sanity check */
  if (!fp)
    return nReturn;

  if(fp->pCriticalSection)
    MM_CriticalSection_Enter(fp->pCriticalSection);

  if (fp->videoHandle != FILE_HANDLE_INVALD)
  {
#ifdef FEATURE_FILESOURCE_FILE_IO_CACHING
    if (fp->cacheValid)
    {
      int64 fpReadPos = fp->fileReadPos;
      int64 rdBuffPos = fp->readBuffPos;

      switch (origin)
      {
        case SEEK_SET:
        //SEEK_SET - Set to 'offset'
        /* if the read buffer is valid */
        if (fp->readBuffPos != -1)
        {
          /* if the offset is greater than the current file read position
           and less than the maximum valid read buffer */
          if (((int64)offset > fp->fileReadPos) && ((int64)offset<= (fp->fileReadPos + fp->readBuffSize - fp->readBuffPos)))
          {
            fp->readBuffPos += (offset - fp->fileReadPos -1);
          }
          /* if the offset less than/equal to file read position and also is greater
          than or equal to the minimum valid read buffer */
          else if (((int64)offset<= fp->fileReadPos) && ((int64)offset>((fp->fileReadPos - fp->readBuffPos) + 1 )))
          {
            fp->readBuffPos = offset - (( fp->fileReadPos - fp->readBuffPos )+ 1);
          }
          /* if the offset is not within the range of valid read buffer then flush the read buffer */
          else
          {
            fp->readBuffPos = -1;
          }
        }
        fp->fileReadPos = (int64)offset-1;
        break;

        case SEEK_CUR:
        //SEEK_CUR - Set to 'offset' + current position
        if (fp->readBuffPos != -1)
        {
          if ((fp->readBuffPos + (int64)offset) >= (int64)fp->readBuffSize)
          {
            fp->readBuffPos = -1;
          }
          else
          {
            fp->readBuffPos += offset;
          }
        }
        fp->fileReadPos += offset;
        break;

        case SEEK_END:
        //SEEK_END - Set to 'offset' + file size
        fp->fileReadPos = fp->fileSize + offset -1;
        fp->readBuffPos = -1;
        break;

        default:
        /* This is an error*/
        if(fp->pCriticalSection)
          MM_CriticalSection_Leave(fp->pCriticalSection);

        return -1;
      }

      if( MM_File_SeekEx ( fp->videoHandle, offset, origin ) < 0 )
      {
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "MM_File_Seek failed. FS Status");
        fp->fileReadPos = fpReadPos;
        fp->readBuffPos = rdBuffPos;
      }
      else
        nReturn = 0;
    }
    else
#endif  /*FEATURE_FILESOURCE_FILE_IO_CACHING */
    {
      switch (origin)
      {
        case SEEK_SET:
          break;

        case SEEK_CUR:
          break;

        case SEEK_END:
          break;

        default:
          /* This is an error*/
          if(fp->pCriticalSection)
            MM_CriticalSection_Leave(fp->pCriticalSection);
          return -1;
      }
      if( MM_File_SeekEx ( fp->videoHandle, offset, origin ) < 0 )
      {
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "MM_File_Seek failed. FS Status");
      }
      else
        nReturn = 0;
    }
  }
  else if (fp->bBufValid)
  {
    int64 npos;
    switch (origin)
    {
      case SEEK_CUR:
      npos = offset + fp->memBuf.curPos;
      break;

      case SEEK_END:
      npos = offset + fp->memBuf.bufSize;
      break;

      case SEEK_SET:
      npos = offset;
      break;

      default:
      npos = (-1); /* error */
      break;
    }
    if ((npos>=0) && (npos<=(int32)fp->memBuf.bufSize))
    {
      fp->memBuf.curPos = npos;
      nReturn = 0;
    }
  }
#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
  else if(fp->pStreamPort)
  {
    switch (fp->StreamType)
    {
      case ISTREAM_PORT:
        #ifdef FEATURE_FILESOURCE_FILE_IO_CACHING
          if(fp->cacheValid)
          {
            int64 fpReadPos = fp->fileReadPos;
            int64 rdBuffPos = fp->readBuffPos;

            switch ( origin )
            {
              case SEEK_SET:
              //SEEK_SET - Set to 'offset'
              /* if the read buffer is valid */
              if(fp->readBuffPos != -1)
              {
                /* if the offset is greater than the current file read position
                 and less than the maximum valid read buffer */
                if(((int64)offset > fp->fileReadPos) && ((int64)offset<= (fp->fileReadPos + fp->readBuffSize - fp->readBuffPos)))
                {
                  fp->readBuffPos += (offset - fp->fileReadPos -1);
                }
                /* if the offset less than/equal to file read position and also is greater
                than or equal to the minimum valid read buffer */
                else if(((int64)offset<= fp->fileReadPos) && ((int64)offset>((fp->fileReadPos - fp->readBuffPos) + 1 )))
                {
                  fp->readBuffPos = offset - (( fp->fileReadPos - fp->readBuffPos )+ 1);
                }
                /* if the offset is not within the range of valid read buffer then flush the read buffer */
                else
                {
                  fp->readBuffPos = -1;
                }
              }
              fp->fileReadPos = (int64)offset-1;
              break;

              case SEEK_CUR:
              //SEEK_CUR - Set to 'offset' + current position
              if(fp->readBuffPos != -1)
              {
                if((fp->readBuffPos + (int64)offset) >= (int64)fp->readBuffSize)
                {
                  fp->readBuffPos = -1;
                }
                else
                {
                  fp->readBuffPos += offset;
                }
              }
              fp->fileReadPos += offset;
              break;

              case SEEK_END:
              //SEEK_END - Set to 'offset' + file size
              fp->fileReadPos = fp->fileSize + offset -1;
              fp->readBuffPos = -1;
              break;

              default:
                /* This is an error*/
                if(fp->pCriticalSection)
                  MM_CriticalSection_Leave(fp->pCriticalSection);
                return -1;
            }
            int64 nRetOffset = 0;
            if (fp->pStreamPort->Seek(offset,origin,&nRetOffset) == video::iStreamPort::DS_SUCCESS)
            {
              nReturn = 0;
            }
            else
            {
              MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "fp->pFileReader->Seek");
              fp->fileReadPos = fpReadPos;
              fp->readBuffPos = rdBuffPos;
            }
          }
          else
        #endif  /*FEATURE_FILESOURCE_FILE_IO_CACHING */
          {
            int64 nRetOffset = 0;
            if( fp->pStreamPort->Seek(offset,origin,&nRetOffset) == video::iStreamPort::DS_SUCCESS )
            {
              nReturn = 0;
            }
            else
            {
              MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "fp->pFileReader->Seek");
            }
          }
          break;
      default:
        break;
    }//switch (fp->StreamType)
  }//if(fp->pFileReader)
#endif
  else if (fp->bPullBufValid)
  {
    //seek locally and asuming origin is always SEEK_SET
    if (origin == SEEK_CUR)
    {
      fp->pullBuf.curPos += offset;
    }
    else if (origin == SEEK_SET)
    {
      fp->pullBuf.curPos = offset;
    }

    if (origin != SEEK_END)
    {
      nReturn = 0;
    }
    else
    {
      //SEEK_END is invalid as the buffer size is unknown.
      MM_ASSERT(0);
    }
  }
#ifdef FEATURE_FILESOURCE_DRM_DCF
  else
  {
    switch (fp->StreamType)
    {
      case DRM_IXSTREAM:
      IxStreamWhenceType fs_origin;
      switch (origin)
      {
        case SEEK_SET:
        fs_origin = IX_STRM_SEEK_START;
        break;

        case SEEK_CUR:
        fs_origin = IX_STRM_SEEK_CURRENT;
        break;

        case SEEK_END:
        fs_origin = IX_STRM_SEEK_END;
        break;

        default:
        // This is an error
        return 0;
      }
      if (fp->inputStream != NULL)
      {
        IxErrnoType errorCode = E_FAILURE;
        if(fp->pCriticalSection)
        {
           MM_CriticalSection_Enter(fp->pCriticalSection);
        }
        errorCode = ((IxStream*)fp->inputStream)->Seek(offset,fs_origin);
        if(fp->pCriticalSection)
        {
          MM_CriticalSection_Leave(fp->pCriticalSection);
        }
        if (errorCode != E_SUCCESS)
        {
          //seek failure,set error code to the error code returned from DRM suite.
          MM_MSG_PRIO1(MM_GENERAL, MM_PRIO_FATAL, "OSCL_FileSeek:IxStream->Seek failed..error code %d",errorCode );
        }
        else
        {
          nReturn = 0;
          //Seek successful.Debug purpose
          MM_MSG_PRIO1(MM_GENERAL, MM_PRIO_HIGH, "OSCL_FileSeek succeed..error code %d",errorCode );
        }
      }
      break;
    }//switch (fp->streamType)
  }
#endif
  if(fp->pCriticalSection)
    MM_CriticalSection_Leave(fp->pCriticalSection);

  return(nReturn);
}

/* ======================================================================
FUNCTION
  OSCL_FileTell

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
uint64 OSCL_FileTell (OSCL_FILE *fp,bool* bError)
{
  uint64  fpos = 0;
  /* sanity check */
  if(!fp)
  {
    return fpos;
  }
  if(bError)
  {
    *bError = false;
  }
  if (fp->videoHandle != FILE_HANDLE_INVALD)
  {
#ifdef FEATURE_FILESOURCE_FILE_IO_CACHING
    if(fp->readBuffPos != -1)
    {
      fpos = fp->fileReadPos+1;
    }
    else
    {
      if( MM_File_GetCurrentPosition(fp->videoHandle,(unsigned long*) &fpos) != 0)
      {
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "efs_ftell failed. FS Status");
        if(bError)
        {
          *bError = true;
        }
      }
    }
#else /* FEATURE_FILESOURCE_FILE_IO_CACHING */
    if( MM_File_GetCurrentPosition(fp->videoHandle, &fpos) != 0)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "efs_ftell failed. FS Status");
      if(bError)
      {
        *bError = true;
      }
    }
#endif /* FEATURE_FILESOURCE_FILE_IO_CACHING */
  }
  else if ( fp->bBufValid )
  {
    fpos = fp->memBuf.curPos;
  }
  else if( fp->bPullBufValid)
  {
    fpos = fp->pullBuf.curPos;
  }
#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
  else if (fp->pStreamPort)
  {
    #ifdef FEATURE_FILESOURCE_FILE_IO_CACHING
      if(fp->readBuffPos != -1)
      {
        fpos = fp->fileReadPos + 1;
      }
      else
      {
        int64 nRetOffset = 0;
          (void)fp->pStreamPort->Seek(0,video::iStreamPort::DS_SEEK_CUR,&nRetOffset);
        fpos = (int32)nRetOffset;
      }
    #else /* FEATURE_FILESOURCE_FILE_IO_CACHING */
      fp->pStreamPort->Seek(0,FS_SEEK_CUR);
    #endif /* FEATURE_FILESOURCE_FILE_IO_CACHING */
  }
#endif
#ifdef FEATURE_FILESOURCE_DRM_DCF
  else
  {
    switch (fp->StreamType)
    {
    case DRM_IXSTREAM:
       //FileTell on IxStream

       if(fp->inputStream != NULL)
       {
          IxErrnoType errorCode = E_FAILURE;
          errorCode = ((IxStream*)fp->inputStream)->Tell((uint32*)&fpos);
          if(errorCode != E_SUCCESS)
          {
             if(bError)
             {
                *bError = true;
             }
             //tell failure, set error code to the error code returned form DRM suite
             MM_MSG_PRIO1(MM_GENERAL, MM_PRIO_FATAL, "OSCL_FileTell:IxStream->Tell failed..error code %d",errorCode );
          }
       }
       break;
    }
  }
#endif

  return(fpos);
}

/* ======================================================================
FUNCTION
  OSCL_FileClose

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
int32 OSCL_FileClose (OSCL_FILE *fp)
{
  int32 nRetVal=0;

  if ( fp )
  {
    if (fp->videoHandle != FILE_HANDLE_INVALD)
    {
      if ( MM_File_Release ( fp->videoHandle ) != 0 )
      {
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "MM_File_Release failed. FS Status");
        nRetVal = EOF;
      }
      else
        fp->videoHandle = FILE_HANDLE_INVALD;
    }
#ifdef FEATURE_FILESOURCE_DRM_DCF
    else
    {
      switch (fp->StreamType)
      {
        case DRM_IXSTREAM:
        if(fp->inputStream != NULL)
        {
          /*
           * Should not do Close on IxStream here as if we do it here, then
           * when user tries to play the same clip without exiting QTV,
           * read fails on IxStream and thus playback erros out.
           * In this case, we need to do OPEN explicitly on IxStream.
           * To Do: Decide whether we need to call Close here or not.
           */
        }
        break;
      }
    }
#endif


#ifdef FEATURE_FILESOURCE_FILE_IO_CACHING
/* Freeing allocated read buffer here */
    if(fp->readBuffer)
    {
      MM_Free (fp->readBuffer);
    }
#endif /* FEATURE_FILESOURCE_FILE_IO_CACHING */
    MM_Delete(fp);/*lint !e449 */
  }
  return(nRetVal);
}

/* ======================================================================
FUNCTION
  OSCL_FileFlush

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
int32 OSCL_FileFlush(OSCL_FILE*)
{
  /* just return 0 for now */
  return 0;
}


/* ======================================================================
FUNCTION
  OSCL_CheckEndOfFile

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
int32 OSCL_CheckEndOfFile (OSCL_FILE *fp)
{
  int32 nRetVal = 0;
  unsigned long filesize = 0;
  uint32 filePos = 0;

  /* sanity check */
  if(!fp)
    return 0;

  if (fp->videoHandle != FILE_HANDLE_INVALD)
  {
    if(!(MM_File_GetSize(fp->videoHandle, &filesize)))
    {
      MM_MSG_PRIO( MM_FILE_OPS, MM_PRIO_ERROR, "MM_File_GetSize failed. FS Status");
    }
    else
    {
      if ((MM_File_GetCurrentPosition(fp->videoHandle, &filePos) == 0) && ((unsigned long)filePos > filesize) )
      {
        nRetVal = 1;
      }
    }
  }
  else if ( fp->bBufValid )
  {
    nRetVal = (fp->memBuf.curPos==fp->memBuf.bufSize);
  }
  return(nRetVal);
}

/* ======================================================================
FUNCTION
  OSCL_GetFileError

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
int32 OSCL_GetFileError (OSCL_FILE*)
{
  return 0; /* no error */
}
/* ======================================================================
FUNCTION
  OSCL_GetEFSFileSysFreeSpace

DESCRIPTION
  Find if a file with given (full path) name already exists

DEPENDENCIES
  NONE

RETURN VALUE
  TRUE if file already exists.

SIDE EFFECTS
  NONE

========================================================================== */
uint64 OSCL_GetEFSFileSysFreeSpace (void)
{
  //return fs_space_available();
  return 0;
}

/* ======================================================================
FUNCTION
  OSCL_FileExists

DESCRIPTION
  Find if a file with given (full path) name already exists

DEPENDENCIES
  NONE

RETURN VALUE
  TRUE if file already exists.

SIDE EFFECTS
  NONE

========================================================================== */
bool OSCL_FileExists (const FILESOURCE_STRING &filename)
{
  MM_HANDLE file = NULL;

  if(MM_File_CreateW((MM_WCHAR*)(filename.get_cstr()),
                                    MM_FILE_CREATE_R , &file) != 0)
  {
     return false;
  }

  (void)MM_File_Release(file);
  return true;
}

/* ======================================================================
FUNCTION
  OSCL_GetFileSysFreeSpace

DESCRIPTION
  Checks the available space in the file system
  path provided and returns that value

DEPENDENCIES
  NONE

RETURN VALUE
  TRUE if file already exists.

SIDE EFFECTS
  NONE

========================================================================== */
int64 OSCL_GetFileSysFreeSpace (const FILESOURCE_STRING &filePath)
{
  int64 freeSpace = 0;
  int namelength = filePath.get_size()+1;
  char* pCharTempName = (char*)MM_Malloc(namelength);
  if(!pCharTempName)
  {
    return freeSpace;
  }
  WideCharToChar(filePath,namelength,pCharTempName,namelength);

  (void)MM_File_GetFreeSpace(pCharTempName, (uint64*)(&freeSpace));

  MM_Free(pCharTempName);
  pCharTempName = NULL;
  return freeSpace;
}

/* ======================================================================
FUNCTION
  OSCL_GetFileSysFreeSpace

DESCRIPTION
  Checks the available space in the file system
  path provided and returns that value

DEPENDENCIES
  NONE

RETURN VALUE
  TRUE if file already exists.

SIDE EFFECTS
  NONE

========================================================================== */
int64 OSCL_GetFileSysFreeSpace (const char *filePath)
{
  int64 freeSpace = 0;

  (void)MM_File_GetFreeSpace(filePath, (uint64*)(&freeSpace));

  return freeSpace;
}

/* ======================================================================
FUNCTION
  OSCL_FileList

DESCRIPTION
  Lists all the files in a given directory on EFS

DEPENDENCIES
  NONE

RETURN VALUE
  TRUE if file list found.

SIDE EFFECTS
  NONE

========================================================================== */
bool OSCL_FileList(void *buffer,unsigned char *dirName,unsigned long bufSize)
{
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "Function not implemented yet (TODO)" );
  return false;
}

/* ======================================================================
FUNCTION
  OSCL_NumFiles

DESCRIPTION
  Returns number of files in a given directory on the EFS

DEPENDENCIES
  NONE

RETURN VALUE
  TRUE if number of files found

SIDE EFFECTS
  NONE

========================================================================== */
bool OSCL_NumFiles(const char *dirName,int32 *numFiles)
{
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR, "Function not implemented yet (TODO)" );
  return false;
}

/* ======================================================================
FUNCTION
  OSCL_FileSize

DESCRIPTION
  Returns the size of the file

DEPENDENCIES
  NONE

RETURN VALUE
  The Size of the file

SIDE EFFECTS
  NONE

========================================================================== */
uint64 OSCL_FileSize( const FILESOURCE_STRING &filename )
{
  unsigned long size = 0;
  MM_HANDLE file = NULL;

  if(MM_File_CreateW((MM_WCHAR*)(filename.get_cstr()), MM_FILE_CREATE_R , &file) != 0)
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_Create failed");
    return size;
  }
  (void)MM_File_GetSize(file,&size);

  (void)MM_File_Release(file);
  return size;
}

/* ======================================================================
FUNCTION
  OSCL_FileSize

DESCRIPTION
  Returns the size of the file

DEPENDENCIES
  NONE

RETURN VALUE
  The Size of the file

SIDE EFFECTS
  NONE

========================================================================== */
uint64 OSCL_FileSize( const char *filename )
{
  unsigned long size = 0;
  MM_HANDLE file = NULL;

  if(MM_File_Create(filename, MM_FILE_CREATE_R , &file) != 0)
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_Create failed");
    return size;
  }
  (void)MM_File_GetSize(file,&size);

  (void)MM_File_Release(file);
  return size;
}


/* ======================================================================
FUNCTION
  OSCL_File_Read_Internal

DESCRIPTION
  Tries to read all requested data from file in single call

DEPENDENCIES
  NONE

RETURN VALUE
  pBytesRead -  number of bytes read into pBuffer
  return value 0 is success else failure

SIDE EFFECTS
  NONE

========================================================================== */

int OSCL_File_Read_Internal(MM_HANDLE handle, char *pBuffer, int nSize, int *pnBytesRead)
{
  int reqSizeToRead = nSize;
  int BytesRead;
  int nRet = 1; /* default to error */
  *pnBytesRead = 0;

  do
  {
    BytesRead = 0;
    nRet = MM_File_Read(handle, pBuffer+*pnBytesRead, reqSizeToRead, &BytesRead);
    reqSizeToRead = reqSizeToRead - BytesRead;
    *pnBytesRead = *pnBytesRead + BytesRead;
  }
  while(reqSizeToRead && BytesRead && (!nRet));

  if(nSize == *pnBytesRead)
  {
    nRet = 0;
  }

  return nRet;
}


