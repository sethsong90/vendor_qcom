#ifndef AAC_HDR_DATA_TYPES_H
#define AAC_HDR_DATA_TYPES_H

/* =======================================================================
                              aacdatatypes.h
DESCRIPTION
  
Copyright 2011 Qualcomm Technologies, Inc., All Rights Reserved
========================================================================== */

/* =======================================================================
                             PERFORCE HEADER
$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/AACParserLib/main/latest/inc/aacdatatypes.h#5 $
========================================================================== */

//Data types used
typedef unsigned int        aac_uint32;
typedef int                 aac_int32;
typedef short               aac_int16;
typedef unsigned short      aac_uint16;
typedef unsigned char       aac_uint8;
typedef char                aac_int8;
typedef signed long long    aac_int64;      
typedef unsigned long long  aac_uint64;     
typedef aac_uint64          AAC_FILE_OFFSET;
typedef unsigned char       aac_boolean;

//MP3 Parser states.
typedef enum aact_parserState
{
  //Initial Parser state when parser handle is created.
  AAC_PARSER_IDLE,
  //Parser state when parsing begins.
  AAC_PARSER_INIT,
  //Parser state when parsing is successful.
  AAC_PARSER_READY,
  //Parser is seeking to a given timestamp.
  AAC_PARSER_SEEK,
  //Read failed
  AAC_PARSER_READ_FAILED,
  //Data being read is corrupted.
  AAC_PARSER_FAILED_CORRUPTED_FILE,
  //Parser state it fails to allocate memory,
  //that is, when malloc/new fails.
  AAC_PARSER_NO_MEMORY, 
  //Parser state if requested amount of data is 
  // not downloaded in PD or PS cases
  AAC_PARSER_UNDERRUN
}aacParserState;


#endif
