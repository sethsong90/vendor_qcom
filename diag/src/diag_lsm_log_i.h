#ifndef DIAG_LSM_LOG_I_H
#define DIAG_LSM_LOG_I_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

EXTENDED DIAGNOSTIC LOG LEGACY SERVICE MAPPING HEADER FILE
(INTERNAL ONLY)

GENERAL DESCRIPTION

  All the declarations and definitions necessary to support the reporting
  of messages.  This includes support for the
  extended capabilities as well as the legacy messaging scheme.

Copyright (c) 2007-2011 Qualcomm Technologies, Inc. All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================
                        EDIT HISTORY FOR FILE

$Header:

when       who     what, where, why
--------   ---     ----------------------------------------------------------
11/26/07   JV      Created File
===========================================================================*/

/* Initializes legacy service mapping for Diag log service */
boolean Diag_LSM_Log_Init(void);

/* updates the copy of log_mask */
void log_update_mask(unsigned char*);

/* updates the copy of the dci log_mask */
void log_update_dci_mask(unsigned char*);

#endif /* DIAG_LSM_LOG_I_H */
