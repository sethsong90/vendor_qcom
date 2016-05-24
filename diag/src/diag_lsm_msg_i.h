#ifndef DIAG_LSM_MSG_I_H
#define DIAG_LSM_MSG_I_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

EXTENDED DIAGNOSTIC MESSAGE SERVICE LEGACY MAPPING
INTERNAL HEADER FILE

GENERAL DESCRIPTION
Internal header file

Copyright (c) 2007-2011, 2013 Qualcomm Technologies, Inc. All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================
                        EDIT HISTORY FOR FILE

$Header:

when       who     what, where, why
--------   ---     ----------------------------------------------------------
12/03/07   mad     Created File
===========================================================================*/

/* Initializes Mapping layer for message service*/
boolean Diag_LSM_Msg_Init (void);

/* clean up before exiting legacy service mapping layer.
Does nothing as of now, just returns TRUE. */
boolean Diag_LSM_Msg_DeInit (void);

/* updates the copy of the run-time masks for messages */
void msg_update_mask(unsigned char*, int);

#endif /* DIAG_LSM_MSG_I_H */
