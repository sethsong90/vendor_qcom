/**
@file
This file contains a single function, which returns a string indicating the
version number of the eSBC codec

@ingroup codec_internal
*/

/**
@addtogroup codec_internal
@{
*/

/**********************************************************************************
  $Revision: #1 $
  Copyright 2002 - 2004 Open Interface North America, Inc. All rights reserved.
***********************************************************************************/

#include "oi_stddefs.h"
#include "oi_codec_sbc_private.h"

/** Version string for the BLUEmagic 3.0 protocol stack and profiles */
PRIVATE OI_CHAR * const codecVersion = "v1.5"
#ifdef OI_SBC_EVAL
" (Evaluation version)"
#endif
;

/** This function returns the version string for the BLUEmagic 3.0 protocol stack
    and profiles */
OI_CHAR *OI_CODEC_Version(void) {
    return codecVersion;
}

/**********************************************************************************/

/**
@}
*/
