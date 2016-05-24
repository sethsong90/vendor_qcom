/*===========================================================================
                          M M   W r a p p e r
                        f o r   F i l e   S e r v i c e s

*//** @file MMFile.c
  This file implements the interfaces the support file operations like open,
  read, write, and seek.

Copyright (c) 2008 - 2009 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*========================================================================*/

/*===========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/platform/OSAbstraction/REX/main/latest/src/MMRexFile.c#6 $

when       who         what, where, why
--------   ---         -------------------------------------------------------
07/02/08   gkapalli    Created file.

============================================================================*/

/*===========================================================================
 Include Files
============================================================================*/
#include "MMFile.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <fcntl.h>
#include "MMMalloc.h"
#include "MMDebugMsg.h"
/* =======================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains definitions for constants, macros, types, variables
and other items needed by this module.

========================================================================== */

/*
 * Returns the length of a WIDE character string.
 *
 * @param[in] pWCharFilePath - Source WCHAR filepath
 *
 * @return length of the string.
 */

static int MM_WCHAR_Strlen
(
  MM_WCHAR *pWCharFilePath
)
{
  int nLen = 0;
  while((char)*pWCharFilePath++ != '\0')
  {
    pWCharFilePath++;
    nLen++;
  }

  //MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR,"MM_WCHAR_Strlen: String size %d",nLen );
  return nLen;
}

/*
 * Converts Wide character string to characater string
 *
 * @param[in] pWCharFilePath - Source WCHAR filepath
 * @param[in] pCharFilePath  -  Destination char filepath
 * @param[out] pHandle       - size of char destination filename array
 *
 * @return returns number of characters copied including NULL character.
 */

static int MM_WCHAR_ToChar
(
  MM_WCHAR *pWCharFilePath,
  char     *pCharFilePath,
  int       nSize
)
{
  int       nLen = nSize;
  MM_WCHAR *pSrc = pWCharFilePath;
  char     *pDst = pCharFilePath;

  if(!pSrc || !pDst || !nSize)
  {
    return 0;
  }
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"MM_WCHAR_ToChar");

  do
  {
    //MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR,"%c",*pSrc );
    *pDst++ = (char)*pSrc;
    pSrc++;
    pSrc++;
  }
  while(*pSrc != '\0' && --nLen);

  *pDst = '\0';
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR,"MM_WCHAR_ToChar: String %s",pCharFilePath );

  return nSize;
}


/*
 * Creates/Opens a file
 *
 * @param[in] pFilePath - Name and path of the file to act on
 * @param[in] nMode -  mode to be used to create a file (MM_FILE_CREATE_*)
 * @param[out] pHandle - returns a reference to the file handle
 *
 * @return return value 0 is success else failure
 */
int MM_File_Create
(
  const char* pFilePath,
  int   nMode,
  MM_HANDLE *pHandle
)
{
  int nResult = 1;
  MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_Create File Name %s , Mode %d",pFilePath, nMode );
  if ( pHandle )
  {
    int nOpenFlag;
    int nFileHandle;

    switch (nMode)
    {
      case MM_FILE_CREATE_R_PLUS:
        nOpenFlag = O_RDWR;
        break;
      case MM_FILE_CREATE_W:
        nOpenFlag = O_CREAT | O_TRUNC | O_WRONLY;
        break;
      case MM_FILE_CREATE_W_PLUS:
        nOpenFlag = O_CREAT | O_TRUNC | O_RDWR;
        break;
      case MM_FILE_CREATE_A:
        nOpenFlag = O_CREAT | O_APPEND | O_WRONLY;
        break;
      case MM_FILE_CREATE_R:
      default:
        nOpenFlag = O_RDONLY;
        break;
    }

    int mode = S_IRWXU | S_IRWXG | S_IRWXO ;

    nFileHandle = open(pFilePath, nOpenFlag, mode);
    *pHandle = (void *)nFileHandle;

    if ( nFileHandle >= 0 )
    {
      nResult = 0;
    }
    else
    {
      MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_Create failed .Efs Error No %d , File Name %s , Mode %d",nFileHandle, pFilePath, nMode );
    }
  }

  return nResult;

}

/*
 * Creates/Opens a file given in WCHAR format
 *
 * @param[in] pFilePath - Name and path of the file to act on
 * @param[in] nMode -  mode to be used to create a file (MM_FILE_CREATE_*)
 * @param[out] pHandle - returns a reference to the file handle
 *
 * @return return value 0 is success else failure
 */
int MM_File_CreateW
(
  MM_WCHAR* pFilePath,
  int   nMode,
  MM_HANDLE *pHandle
)
{
  int  nResult = 1;
  char *pCharName;
  int  nNameLength =  MM_WCHAR_Strlen(pFilePath)+1;

  pCharName = (char*)MM_Malloc(nNameLength);
  if(!pCharName)
  {
    return nResult;
  }
  //MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_CreateW: String size %d %d",nNameLength, nFileLength );

  nResult = MM_File_Create(pCharName,nMode,pHandle);
  if(nResult != 0)
  {
    if(MM_WCHAR_ToChar(pFilePath,pCharName,nNameLength) !=
      nNameLength)
    {
      MM_Free(pCharName);
      return nResult;
    }
    MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_Create Failed Once  %s , Mode %d",pCharName, nMode );
    nResult = MM_File_Create(pCharName,nMode,pHandle);
  }
  MM_Free(pCharName);
  return nResult;
}

/*
 * Releases the resources associated with the file handle
 *
 * @param[in] handle - the file handle
 *
 * @return zero value on success else failure
 */
int MM_File_Release
(
  MM_HANDLE handle
)
{
  int nResult = 1;
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_Release");
  if ((int)handle >= 0)
  {
    nResult = close( (int)handle );
  }
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_Release exit");
  return nResult;
}

/*
 * Reads data from a file into the buffer
 *
 * @param[in] handle - the file handle
 * @param[in] pBuffer - pointer to the buffer to which data may be copied
 * @param[in] nSize -  sizeof pBuffer
 * @param[out] pBytesRead -  number of bytes read into pBuffer
 *
 * @return return value 0 is success else failure
 */
int MM_File_Read
(
  MM_HANDLE handle,
  char *pBuffer,
  int nSize,
  int *pnBytesRead
)
{
  int nResult = 1;
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_Read");
  if ( (int)handle >= 0 && pnBytesRead )
  {
    ssize_t nBytesRead = read( (int)handle, pBuffer, (size_t)nSize);
    if (nBytesRead >= 0)
    {
      *pnBytesRead = (int)nBytesRead;
      nResult = 0;
    }
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_Read %d", *pnBytesRead);
  }
  return nResult;
}

/*
 * Writes data from the buffer into the file
 *
 * @param[in] handle - the file handle
 * @param[in] pBuffer - pointer to the buffer that contains the data
 * @param[in] nSize -  size of pBuffer that needs to be written
 * @param[out] pBytesWritten -  number of bytes written into the file
 *
 * @return return value 0 is success else failure
 */
int MM_File_Write
(
  MM_HANDLE handle,
  char *pBuffer,
  int nSize,
  int *pnBytesWritten
)
{
  int nResult = 1;
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR, "MM_File_Write Size %d", nSize);
  if ( (int)handle >= 0 && pnBytesWritten )
  {
    ssize_t nBytesRead = write((int)handle, pBuffer, (size_t)nSize);
    if (nBytesRead >= 0)
    {
      *pnBytesWritten = (int)nBytesRead;
      nResult = 0;
    }
  }

  return nResult;
}

/*
 * Reposition the file pointer in an open file
 *
 * @param[in] handle - the file handle
 * @param[in] nOffset - offset based on nWhence; may be negative
 * @param[in] nWhence -  flags to be used when acting on a file
 *
 * @return return value 0 is success else failure
 */
int MM_File_Seek
(
  MM_HANDLE handle,
  long nOffset,
  int nWhence
)
{
  int nResult = 1;
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_Seek");

  if ( (int)handle >= 0)
  {
    int efsWhence;

    switch (nWhence)
    {
      case MM_FILE_SEEK_BEG:
        efsWhence = SEEK_SET;
        break;
      case MM_FILE_SEEK_END:
        efsWhence = SEEK_END;
        break;
      case MM_FILE_SEEK_CUR:
      default:
        efsWhence = SEEK_CUR;
        break;
    }
    if ( lseek((int)handle, nOffset, efsWhence) == nOffset )
    {
      nResult = 0;
    }
  }

  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_Seek %d", nResult);
  return nResult;
}

/*
 * Reposition the file pointer in an open file
 *
 * @param[in] handle - the file handle
 * @param[in] nOffset - offset based on nWhence;
 * @param[in] nWhence -  flags to be used when acting on a file
 *
 * @return return value 0 is success else failure
 */
int MM_File_SeekEx
(
  MM_HANDLE handle,
  long long nOffset,
  int nWhence
)
{
  int nResult = 1;
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_SeekEx");

  if ( (int)handle >= 0)
  {
    int efsWhence;

    switch (nWhence)
    {
      case MM_FILE_SEEK_BEG:
        efsWhence = SEEK_SET;
        break;
      case MM_FILE_SEEK_END:
        efsWhence = SEEK_END;
        break;
      case MM_FILE_SEEK_CUR:
      default:
        efsWhence = SEEK_CUR;
        break;
    }
    if ( lseek64((int)handle, nOffset, efsWhence) == nOffset )
    {
      nResult = 0;
    }
  }

  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_SeekEx %d", nResult);
  return nResult;
}

/*
 * Returns the current file size
 *
 * @param[in] handle - the file handle
 * @param[out] dwSize - returns the file size on success
 *
 * @return return value 0 is success else failure
 */
int MM_File_GetSize
(
  MM_HANDLE handle,
  unsigned long *pnSize
)
{
  int nResult = 1;
  if ( (int)handle >= 0 && pnSize )
  {
    struct stat fsStat;
    if ( fstat((int)handle, &fsStat) == 0 )
    {
      *pnSize = fsStat.st_size;
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_GetSize: Filesize  %lu",*pnSize);
      nResult = 0;
    }
  }

  return nResult;
}

/*
 * Truncates the file to the length specified
 *
 * @param[in] pFile - reference to the file handle
 * @param[in] nLength - the length to which the file needs to be truncated
 *
 * @return return value 0 is success else failure
 */
int MM_File_Truncate
(
  MM_HANDLE handle,
  long nLength
)
{
  return ftruncate( (int)handle, nLength);
}

/*
 * Copies an existing file to a new file, fails if there is file by same
 * name at the new location.
 *
 * @param[in] pExistingFile - Pointer to a null-terminated string that
 *                            specifies the name of an existing file
 * @param[in] pNewFilePath - Pointer to a null-terminated string that specifies
 *                           the name of the new file.
 *
 * @return return value 0 is success else failure
 */
int MM_File_Copy
(
  const char* pExistingFile,
  const char *pNewFile
)
{
  int nResult = 1;
  return nResult;
}

/*
 * Moves an existing file to a new file/location, fails if there is file by same
 * name at the new location.
 *
 * @param[in] pExistingFile - Pointer to a null-terminated string that
 *                            specifies the name of an existing file
 * @param[in] pNewFilePath - Pointer to a null-terminated string that specifies
 *                           the name of the new file.
 *
 * @return return value 0 is success else failure
 */
int MM_File_Move
(
  const char* pExistingFile,
  const char *pNewFile
)
{
  return rename(pExistingFile,pNewFile);
}

/*
 * Deletes an existing file.
 *
 * @param[in] pFile - Pointer to a null-terminated string that specifies the
 *                    name of the file file
 *
 * @return return value 0 is success else failure
 */
int MM_File_Delete
(
  const char* pFile
)
{
  return unlink(pFile);
}

/*
 * Gets the free space in the disk pointed by the path
 *
 * @param[in] pPath - Pointer to a null-terminated string that specifies the
 *                    path
 * @param[out] pFreeSpace - updates the value with the avaliable free space
 *
 * @return return value 0 is success else failure
 */
int MM_File_GetFreeSpace
(
  const char* pPath,
  unsigned long long *pFreeSpace
)
{
  int nResult = 1;
  if ( pFreeSpace != NULL && pPath != NULL)
  {
    struct statfs statvfs;
    if (statfs ( pPath, &statvfs) == 0 )
    {
      *pFreeSpace =  statvfs.f_bsize * statvfs.f_bavail;
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_GetFreeSpace: Free Space  for path %s", pPath);
      nResult = 0;
    }
  }
  return nResult;
}

/*
 * Get the current file position
 *
 * @param[in] handle - Reference to the file handle
 * @param[out] filePos - Pointer to the file position
 *
 * @return return value 0 is success else failure
 */

int MM_File_GetCurrentPosition
(
  MM_HANDLE handle,
  unsigned long* filePos
)
{
  int result = 1;

  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_GetCurrentPosition");
  if ( (int)handle >= 0 && filePos )
  {
    if( ( *filePos = (int)lseek ( (int)handle, 0, SEEK_CUR) ) > 0 )
    {
      result = 0;
    }
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR,"MM_File_GetCurrentPosition %lu", *filePos);
  }
  return result;
}
