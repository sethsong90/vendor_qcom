#ifndef __AAC_ERRORS_H__
#define __AAC_ERRORS_H__

/*=======================================================================
                              aacErrors.h
DESCRIPTION
  
Copyright 2011 Qualcomm Technologies, Inc., All Rights Reserved

 ========================================================================*/

/*=======================================================================
                             PERFORCE HEADER
$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/AACParserLib/main/latest/inc/aacerrors.h#7 $
$DateTime: 2011/07/27 23:37:45 $
$Change: 1856265 $

==========================================================================*/

/*
 * Lists various error types used/reported by AAC parser. 

 * AAC_UNKNOWN_ERROR:Unknown error.

 * AAC_PARSE_ERROR:Error while parsing. 

 * AAC_FAILURE: Operation/API failed.

 * AAC_READ_FAILURE:  Failure to read data from the file.

 * AAC_CORRUPTED_FILE: Read in data is not same as being expected.

 * AAC_INVALID_USER_DATA:Parameter passed in to an API is illegal.

 * AAC_OUT_OF_MEMORY:Parser failed to allocate memory.

 * AAC_SUCCESS:Indicates successful operation/API call.

 * AAC_END_OF_FILE: Returned when retrieving sample info.
*/
/*

typedef enum aacError
{  
  AAC_SUCCESS,          
  AAC_UNKNOWN_ERROR,
  AAC_PARSE_ERROR,
  AAC_FAILURE,
  AAC_READ_FAILURE,    
  AAC_CORRUPTED_FILE,
  AAC_INVALID_USER_DATA,
  AAC_OUT_OF_MEMORY,  
  AAC_END_OF_FILE,
  AAC_INSUFFICIENT_BUFFER,
  AAC_INVALID_PARM,
  AAC_SEEK_FAILED,
  AAC_UNSUPPORTED,
  AAC_RESOURCENOTFOUND,
  AAC_DATA_UNDERRUN
}aacErrorType;*/
#endif
