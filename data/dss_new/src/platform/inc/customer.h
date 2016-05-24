/******************************************************************************
 ----------------------------------------------------------------------------
 Copyright (c) 2007 Qualcomm Technologies, Inc.
 All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 ----------------------------------------------------------------------------
*******************************************************************************/

#ifndef CUSTOMER_H
#define CUSTOMER_H

/* Use BMP APPS feature set as base */
#define FEATURE_DATACOMMON_2H09_1_DUAL_PROC_BMP_APPS

#include "custdatacommon.h"

/* Tailor Linux feature set */
#undef  FEATURE_DATACOMMON_PACKAGE_BMP
#undef  FEATURE_DATACOMMON_PS_IFACE_IO
#undef  FEATURE_DATA_PS_IN_ALIASES
#undef  FEATURE_DATA_PS_PING
#undef  FEATURE_DATA_PS_DHCP
#undef  FEATURE_DATA_PS_DHCPV6
#undef  FEATURE_DATA_PS_MCAST
#undef  FEATURE_DATA_PS_MCAST_V6
#undef  FEATURE_DATA_RMNET_IPV6
#undef  FEATURE_DATA_PS_ADDR_MGMT
#undef  FEATURE_DS_SOCKETS


#define FEATURE_DATACOMMON_PACKAGE_LINUX_APPS
#define FEATURE_DATA_PS_SYSTEM_HEAP
#endif /* CUSTOMER_H */
