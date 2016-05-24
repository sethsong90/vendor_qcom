#ifndef QDI_DEBUG_H
#define QDI_DEBUG_H

#ifdef FEATURE_DATA_LOG_QXDM
  #include "msg.h"
#endif

#include "ds_string.h"


#ifdef QDI_OFFTARGET
  #define SA_FAMILY(addr)         (addr).sa_family
  #define SA_DATA(addr)           (addr).sa_data
  #define SASTORAGE_FAMILY(addr)  (addr).ss_family
  #define SASTORAGE_DATA(addr)    (addr).__ss_padding
#else
  #define SA_FAMILY(addr)         (addr).sa_family
  #define SA_DATA(addr)           (addr).sa_data
  #define SASTORAGE_FAMILY(addr)  (addr).ss_family
  #define SASTORAGE_DATA(addr)    (addr).__data
#endif

/* define to enable function level trace logging */
#define QDI_TRACE

#ifdef FEATURE_DATA_LOG_QXDM

#define QDI_MAX_DIAG_LOG_MSG_SIZE 512

#define QDI_QXDM_LOG(level, ...)                                 \
  do {                                                           \
    char buf[QDI_MAX_DIAG_LOG_MSG_SIZE];                         \
    std_strlprintf(buf, QDI_MAX_DIAG_LOG_MSG_SIZE, __VA_ARGS__); \
    MSG_SPRINTF_1(MSG_SSID_LINUX_DATA, level, "%s", buf);        \
  } while (0)

  #define QDI_LOG_LOW(...)    QDI_QXDM_LOG(MSG_LEGACY_LOW, __VA_ARGS__)
  #define QDI_LOG_MED(...)    QDI_QXDM_LOG(MSG_LEGACY_MED, __VA_ARGS__)
  #define QDI_LOG_HIGH(...)   QDI_QXDM_LOG(MSG_LEGACY_HIGH, __VA_ARGS__)
  #define QDI_LOG_ERROR(...)  QDI_QXDM_LOG(MSG_LEGACY_ERROR, __VA_ARGS__)
  #define QDI_LOG_FATAL(...)  QDI_QXDM_LOG(MSG_LEGACY_FATAL, __VA_ARGS__)

#elif defined(FEATURE_DATA_LOG_STDERR)

  #define QDI_LOG_LOW(...)    fprintf(stderr, "%s %d:", __FILE__, __LINE__);\
                              fprintf(stderr, __VA_ARGS__)
  #define QDI_LOG_MED(...)    fprintf(stderr, "%s %d:", __FILE__, __LINE__);\
                              fprintf(stderr, __VA_ARGS__)
  #define QDI_LOG_HIGH(...)   fprintf(stderr, "%s %d:", __FILE__, __LINE__);\
                              fprintf(stderr, __VA_ARGS__)
  #define QDI_LOG_ERROR(...)  fprintf(stderr, "%s %d:", __FILE__, __LINE__);\
                              fprintf(stderr, __VA_ARGS__)
  #define QDI_LOG_FATAL(...)  fprintf(stderr, "%s %d:", __FILE__, __LINE__);\
                              fprintf(stderr, __VA_ARGS__)
#else

  #define QDI_LOG_LOW(...)
  #define QDI_LOG_MED(...)
  #define QDI_LOG_HIGH(...)
  #define QDI_LOG_ERROR(...)
  #define QDI_LOG_FATAL(...)

#endif

#ifdef QDI_TRACE
  #define QDI_LOG_TRACE_ENTRY            QDI_LOG_LOW("%s: enter", __FUNCTION__)
  #define QDI_LOG_TRACE_EXIT             QDI_LOG_LOW("%s: exit", __FUNCTION__)
  #define QDI_LOG_TRACE_RETURN(ret)      QDI_LOG_LOW("%s: exit %s=%d", __FUNCTION__, #ret, ret)
  #define QDI_LOG_TRACE_RETURN_HEX(ret)  QDI_LOG_LOW("%s: exit %s=0x%x", __FUNCTION__, #ret, ret)
  #define QDI_LOG_TRACE_RETURN_PTR(ret)  QDI_LOG_LOW("%s: exit %s=%p", __FUNCTION__, #ret, ret)
#else
  #define QDI_LOG_TRACE_ENTRY
  #define QDI_LOG_TRACE_EXIT
  #define QDI_LOG_TRACE_RETURN(ret)
  #define QDI_LOG_TRACE_RETURN_HEX(ret)
  #define QDI_LOG_TRACE_RETURN_PTR(ret)
#endif /*QDI_TRACE*/

#endif /*QDI_DEBUG_H*/
