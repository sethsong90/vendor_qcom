#ifndef DS_UTILS_DEBUG_MSG_H
#define DS_UTILS_DEBUG_MSG_H
/*===========================================================================
  @file ds_Utils_DebugMsg.h

  This header file defines macros which are used to log debug messages to the
  console. These macros translate to DIAG macros.

  DIAG team's MSG 2.0 API is used to send messages to DIAG.

  Copyright (c) 2008 - 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/inc/ds_Utils_DebugMsg.h#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "customer.h"
#include "comdef.h"
#include "msg.h"
#include "err.h"
#include "amssassert.h"

#ifdef FEATURE_DATA_PS

/*===========================================================================

                          MACRO DECLARATIONS

===========================================================================*/

      /*---------------------------------------------------------------------
        Message macros are mapped to DIAG msg levels
      ---------------------------------------------------------------------*/
      #define LOG_MSG_FUNCTION_ENTRY_LEVEL  MSG_LEGACY_MED
      #define LOG_MSG_FUNCTION_EXIT_LEVEL   MSG_LEGACY_MED
      #define LOG_MSG_INFO1_LEVEL           MSG_LEGACY_MED
      #define LOG_MSG_INFO2_LEVEL           MSG_LEGACY_MED
      #define LOG_MSG_INFO3_LEVEL           MSG_LEGACY_LOW
      #define LOG_MSG_INFO4_LEVEL           MSG_LEGACY_LOW
      #define LOG_MSG_STATE_CHANGE_LEVEL    MSG_LEGACY_HIGH
      #define LOG_MSG_INVALID_INPUT_LEVEL   MSG_LEGACY_HIGH
      #define LOG_MSG_ERROR_LEVEL           MSG_LEGACY_ERROR
      #define LOG_MSG_FATAL_ERROR_LEVEL     MSG_LEGACY_FATAL

      /*---------------------------------------------------------------------
        Printing function names is computationally expensive because it uses
        the slower MSG_SPRINTF to format the string instead of just packing
        the parameters off to DIAG. If you need it for debugging or bringup,
        enable FEATURE_DATACOMMON_VERBOSE_MSG.
      ---------------------------------------------------------------------*/
#ifdef FEATURE_DATACOMMON_PACKAGE_MODEM
  #ifdef FEATURE_DATACOMMON_VERBOSE_MSG
      #define PRINT_MSG( level, fmtString, x, y, z)                         \
        MSG_SPRINTF_4( MSG_SSID_LINUX_DATA, level, "%s(): " fmtString,      \
                       __FUNCTION__, x, y, z);

      #define PRINT_MSG_6( level, fmtString, a, b, c, d, e, f)              \
        MSG_SPRINTF_7( MSG_SSID_LINUX_DATA, level, "%s(): " fmtString,              \
                       __FUNCTION__, a, b, c, d, e, f);

  #else /* FEATURE_DATACOMMON_VERBOSE_MSG */
      #define PRINT_MSG( level, fmtString, x, y, z)                         \
        MSG_3( MSG_SSID_LINUX_DATA, level, fmtString, x, y, z);

      #define PRINT_MSG_6( level, fmtString, a, b, c, d, e, f)              \
        MSG_6( MSG_SSID_LINUX_DATA, level, fmtString, a, b, c, d, e, f);

  #endif /* FEATURE_DATACOMMON_VERBOSE_MSG */
#else
  #ifdef FEATURE_DATACOMMON_VERBOSE_MSG
      #define PRINT_MSG( level, fmtString, x, y, z)                         \
        MSG_SPRINTF_4( MSG_SSID_LINUX_DATA, level, "%s(): " fmtString,         \
                       __FUNCTION__, x, y, z);

      #define PRINT_MSG_6( level, fmtString, a, b, c, d, e, f)              \
        MSG_SPRINTF_7( MSG_SSID_LINUX_DATA, level, "%s(): " fmtString,         \
                       __FUNCTION__, a, b, c, d, e, f);

  #else /* FEATURE_DATACOMMON_VERBOSE_MSG */
      #define PRINT_MSG( level, fmtString, x, y, z)                         \
        MSG_3( MSG_SSID_LINUX_DATA, level, fmtString, x, y, z);

      #define PRINT_MSG_6( level, fmtString, a, b, c, d, e, f)              \
        MSG_6( MSG_SSID_LINUX_DATA, level, fmtString, a, b, c, d, e, f);

  #endif /* FEATURE_DATACOMMON_VERBOSE_MSG */
#endif

      /**
        This macro is used to log a message, when ever a function is invoked.
        Important input parameters are logged as well.
      */
      #define LOG_MSG_FUNCTION_ENTRY( fmtString, x, y, z)                   \
      {                                                                     \
        PRINT_MSG( LOG_MSG_FUNCTION_ENTRY_LEVEL, fmtString, x, y, z);       \
      }

      #define LOG_MSG_FUNCTION_ENTRY_6( fmtString, a, b, c, d, e, f)        \
      {                                                                     \
        PRINT_MSG_6( LOG_MSG_FUNCTION_ENTRY_LEVEL, fmtString,               \
                     a, b, c, d, e, f);                                     \
      }

      /**
        This macro is used to log a message, when ever a function returns.
        Return value and output parameters are logged as well.
      */
      #define LOG_MSG_FUNCTION_EXIT( fmtString, x, y, z)                    \
      {                                                                     \
        PRINT_MSG( LOG_MSG_FUNCTION_EXIT_LEVEL, fmtString, x, y, z);        \
      }

      #define LOG_MSG_FUNCTION_EXIT_6( fmtString, a, b, c, d, e, f)         \
      {                                                                     \
        PRINT_MSG_6( LOG_MSG_FUNCTION_EXIT_LEVEL, fmtString,                \
                     a, b, c, d, e, f);                                     \
      }

      /**
        This macro is used to log an informational message.
        For example "Routing look up returned iface" etc.
      */
      #define LOG_MSG_INFO1( fmtString, x, y, z)                            \
      {                                                                     \
        PRINT_MSG( LOG_MSG_INFO1_LEVEL, fmtString, x, y, z);                \
      }

      #define LOG_MSG_INFO1_6( fmtString, a, b, c, d, e, f)                \
      {                                                                     \
        PRINT_MSG_6 ( LOG_MSG_INFO1_LEVEL, fmtString, a, b, c, d, e, f);    \
      }

      /**
        This macro is used to log an informational message. This macro logs
        the message with a lower priority than LOG_MSG_INFO1. Typically,
        RTT calculation messages are logged using this macro.
      */
      #define LOG_MSG_INFO2( fmtString, x, y, z)                            \
      {                                                                     \
        PRINT_MSG( LOG_MSG_INFO2_LEVEL, fmtString, x, y, z);                \
      }

      #define LOG_MSG_INFO2_6( fmtString, a, b, c, d, e, f)                \
      {                                                                     \
        PRINT_MSG_6 ( LOG_MSG_INFO2_LEVEL, fmtString, a, b, c, d, e, f);    \
      }

      /**
        This macro is used to log an informational message. This macro logs
        the message with a lower priority than LOG_MSG_INFO2. Typically,
        stack unwindings are logged using this macro. Normally a function
        logs a message whenever that function fails and it is not typically
        necessary to log another message as stack unwinds. So a low priority
        message is logged as stack unwinds and this message level is turned
        on only in debug builds.
      */
      #define LOG_MSG_INFO3( fmtString, x, y, z)                            \
      {                                                                     \
        PRINT_MSG( LOG_MSG_INFO3_LEVEL, fmtString, x, y, z);                \
      }

      #define LOG_MSG_INFO3_6( fmtString, a, b, c, d, e, f)                \
      {                                                                     \
        PRINT_MSG_6 ( LOG_MSG_INFO3_LEVEL, fmtString, a, b, c, d, e, f);    \
      }

      /**
        This macro is used to log an informational message. This macro logs
        the message with a lower priority than LOG_MSG_INFO3. Typically,
        this macro is used to log in data path. Logging a message in data path
        results in too much noise in the F3 log and hence So a low priority
        message is logged to log data path. This message level is turned
        on only in debug builds.
      */
      #define LOG_MSG_INFO4( fmtString, x, y, z)                            \
      {                                                                     \
        PRINT_MSG( LOG_MSG_INFO4_LEVEL, fmtString, x, y, z);                \
      }

      #define LOG_MSG_INFO4_6( fmtString, a, b, c, d, e, f)                \
      {                                                                     \
        PRINT_MSG_6 ( LOG_MSG_INFO4_LEVEL, fmtString, a, b, c, d, e, f);    \
      }

      /**
        This macro is used to log state changes.
      */
      #define LOG_MSG_STATE_CHANGE( fmtString, x, y, z)                     \
      {                                                                     \
        PRINT_MSG( LOG_MSG_STATE_CHANGE_LEVEL, fmtString, x, y, z);         \
      }

      #define LOG_MSG_STATE_CHANGE_6( fmtString, a, b, c, d, e, f)          \
      {                                                                     \
        PRINT_MSG_6( LOG_MSG_STATE_CHANGE_LEVEL, fmtString,                 \
                     a, b, c, d, e, f);                                     \
      }

      /**
        This macro is used to log invalid user input. This macro is used
        only in the external API to applications when the input validation
        fails.
      */
      #define LOG_MSG_INVALID_INPUT( fmtString, x, y, z)                    \
      {                                                                     \
        PRINT_MSG( LOG_MSG_INVALID_INPUT_LEVEL, fmtString, x, y, z);        \
      }

      #define LOG_MSG_INVALID_INPUT_6( fmtString, a, b, c, d, e, f)         \
      {                                                                     \
        PRINT_MSG_6( LOG_MSG_INVALID_INPUT_LEVEL, fmtString,                \
                     a, b, c, d, e, f);                                     \
      }

      /**
        This macro is used to log recoverable errors with in Common Data code.
      */
      #define LOG_MSG_ERROR( fmtString, x, y, z)                            \
      {                                                                     \
        PRINT_MSG( LOG_MSG_ERROR_LEVEL, fmtString, x, y, z);                \
      }

      #define LOG_MSG_ERROR_6( fmtString, a, b, c, d, e, f)                 \
      {                                                                     \
        PRINT_MSG_6( LOG_MSG_ERROR_LEVEL, fmtString, a, b, c, d, e, f);     \
      }

      /**
        This macro is used to log unrecoverable errors with in Common Data
        code. This macro is followed by an ASSERT.
      */
      #define LOG_MSG_FATAL_ERROR( fmtString, x, y, z)                      \
      {                                                                     \
        PRINT_MSG( LOG_MSG_FATAL_ERROR_LEVEL, fmtString, x, y, z);          \
      }

      #define LOG_MSG_FATAL_ERROR_6( fmtString, a, b, c, d, e, f)           \
      {                                                                     \
        PRINT_MSG_6( LOG_MSG_FATAL_ERROR_LEVEL, fmtString,                  \
                     a, b, c, d, e, f);                                     \
      }

      /**
        This macro is used to log IPv4 addresses. The level can be specified
        using level parameter. Choose the level appropriately (INFO1,
        INFO2 etc.)
      */
      #define LOG_MSG_IPV4_ADDR(level, ip_addr)                             \
      {                                                                     \
        PRINT_MSG_6(level,                                                  \
                    "IPV4 Address is %d.%d.%d.%d",                          \
                    (unsigned char)(ip_addr),                               \
                    (unsigned char)(ip_addr >> 8),                          \
                    (unsigned char)(ip_addr >> 16) ,                        \
                    (unsigned char)(ip_addr >> 24),                         \
                    0,                                                      \
                    0);                                                     \
      }

      /**
        This macro is used to log IPv6 addresses. The level can be specified
        using level parameter. Choose the level appropriately (INFO1,
        INFO2 etc.)
      */
#ifdef FEATURE_DATACOMMON_PACKAGE_MODEM
      #define LOG_MSG_IPV6_ADDR(level, ip_addr)                             \
      {                                                                     \
        MSG_8( MSG_SSID_LINUX_DATA,                                                 \
               level,                                                       \
               "IPV6 Address %x:%x:%x:%x:%x:%x:%x:%x",                      \
               (uint16)(ps_ntohs(ip_addr[0])),                              \
               (uint16)(ps_ntohs(ip_addr[0] >> 16)),                        \
               (uint16)(ps_ntohs(ip_addr[0] >> 32)) ,                       \
               (uint16)(ps_ntohs(ip_addr[0] >> 48)),                        \
               (uint16)(ps_ntohs(ip_addr[1])),                              \
               (uint16)(ps_ntohs(ip_addr[1] >> 16)),                        \
               (uint16)(ps_ntohs(ip_addr[1] >> 32)) ,                       \
               (uint16)(ps_ntohs(ip_addr[1] >> 48)));                       \
      }
#else
      #define LOG_MSG_IPV6_ADDR(level, ip_addr)                             \
      {                                                                     \
        MSG_8( MSG_SSID_LINUX_DATA,                                            \
               level,                                                       \
               "IPV6 Address %x:%x:%x:%x:%x:%x:%x:%x",                      \
               (uint16)(ps_ntohs(ip_addr[0])),                              \
               (uint16)(ps_ntohs(ip_addr[0] >> 16)),                        \
               (uint16)(ps_ntohs(ip_addr[0] >> 32)) ,                       \
               (uint16)(ps_ntohs(ip_addr[0] >> 48)),                        \
               (uint16)(ps_ntohs(ip_addr[1])),                              \
               (uint16)(ps_ntohs(ip_addr[1] >> 16)),                        \
               (uint16)(ps_ntohs(ip_addr[1] >> 32)) ,                       \
               (uint16)(ps_ntohs(ip_addr[1] >> 48)));                       \
      }
#endif


#endif /* FEATURE_DATA_PS */

#endif /* DS_UTILS_DEBUG_MSG_H */
