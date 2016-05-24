/*===========================================================================

                     P S _ I F A C E _ D N S _ C A C H E _ C

DESCRIPTION

  The Data Services DNS Subsystem Cache manager module. Contains
  definitions of functions, variables, macros, structs and enums
  used by DNS Cache manager. This module is internal to the DNS subsystem.

EXTERNALIZED FUNCTIONS

  ps_iface_dns_cache_init()
    Initializes the cache structures.

  ps_iface_dns_cache_add_rr_q()
    Adds a Queue of RRs to the cache.

  ps_iface_dns_cache_find_rr_q()
    Finds a Queue of RRs corresponding to a query.

  ps_iface_dns_cache_flush()
    Deletes all RR Queues from the cache of the specified iface
    and frees memory associated with them.

  ps_iface_dns_cache_flush_entry()
    Deletes an entry in the cache of the specified iface
    and frees memory associated with it.

INTIALIZATIONS AND SEQUENCING REQUIREMENTS
  This module should be run only in PS task context.

Copyright (c) 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_iface_dns_cache.c#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
06/30/10   ea      Initial development. This file is based on
                   ps_dnsi_cache_mgr.c

===========================================================================*/


/*===========================================================================

                            INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS
#ifdef FEATURE_DATA_PS_DNS

#include "ps_dns_cache_config.h"
#include "ps_iface.h"
#include "ps_iface_dns_cache.h"
#include "ps_mem.h"
#include "ps_utils.h"
#include "ps_system_heap.h"
#include "AEEstd.h"
#include "ds_Utils_DebugMsg.h"

/*===========================================================================

                                DATA STRUCTURES

===========================================================================*/
/*---------------------------------------------------------------------------
  Tuning the number of ps dnsi cache entry buffers needed by this module
---------------------------------------------------------------------------*/
#define PS_DNSI_CACHE_ENTRY_BUF_SIZE  ((sizeof(ps_dnsi_cache_entry_type) + 3) & ~3)

#ifndef FEATURE_DATA_PS_LOW_MEM_CHIPSET

#define PS_DNSI_CACHE_ENTRY_BUF_NUM       10
#define PS_DNSI_CACHE_ENTRY_BUF_HIGH_WM    8
#define PS_DNSI_CACHE_ENTRY_BUF_LOW_WM     2

#else

#define PS_DNSI_CACHE_ENTRY_BUF_NUM       5
#define PS_DNSI_CACHE_ENTRY_BUF_HIGH_WM   5
#define PS_DNSI_CACHE_ENTRY_BUF_LOW_WM    1
#endif /* FEATURE_DATA_PS_LOW_MEM_CHIPSET */

/*----------------------------------------------------------------------------
  Allocate memory to hold ps_dnsi_cache_entry along with ps_mem header
----------------------------------------------------------------------------*/
static int ps_dnsi_cache_entry_buf_mem[PS_MEM_GET_TOT_SIZE_OPT
                                       (
                                         PS_DNSI_CACHE_ENTRY_BUF_NUM,
                                         PS_DNSI_CACHE_ENTRY_BUF_SIZE
                                       )] = {0};

#ifdef FEATURE_DATA_PS_MEM_DEBUG
/*----------------------------------------------------------------------------
  Array of pointers used to facilitate easy debugging.  The first one points
  to the ps_mem header and the latter points to ps_dnsi_cache_entry_buf
----------------------------------------------------------------------------*/
static ps_mem_buf_hdr_type      * ps_dnsi_cache_entry_buf_hdr[PS_DNSI_CACHE_ENTRY_BUF_NUM];
static ps_dnsi_cache_entry_type * ps_dnsi_cache_entry_buf_ptr[PS_DNSI_CACHE_ENTRY_BUF_NUM];
#endif /* FEATURE_DATA_PS_MEM_DEBUG */

/*---------------------------------------------------------------------------
  Tuning the number of cache rr buffers needed by this module
---------------------------------------------------------------------------*/
#define PS_DNSI_CACHE_RR_BUF_SIZE  ((sizeof(ps_dnsi_generic_rr_type) + 3) & ~3)

#ifndef FEATURE_DATA_PS_LOW_MEM_CHIPSET

#define PS_DNSI_CACHE_RR_BUF_NUM        30
#define PS_DNSI_CACHE_RR_BUF_HIGH_WM    25
#define PS_DNSI_CACHE_RR_BUF_LOW_WM      2

#else

#define PS_DNSI_CACHE_RR_BUF_NUM        5
#define PS_DNSI_CACHE_RR_BUF_HIGH_WM    5
#define PS_DNSI_CACHE_RR_BUF_LOW_WM     1
#endif /* FEATURE_DATA_PS_LOW_MEM_CHIPSET */

/*----------------------------------------------------------------------------
  Allocate memory to hold ps_dnsi_generic_rr along with ps_mem header
----------------------------------------------------------------------------*/
static int ps_dnsi_cache_rr_buf_mem[PS_MEM_GET_TOT_SIZE_OPT
                                      (
                                        PS_DNSI_CACHE_RR_BUF_NUM,
                                        PS_DNSI_CACHE_RR_BUF_SIZE
                                      )];

#ifdef FEATURE_DATA_PS_MEM_DEBUG
/*----------------------------------------------------------------------------
  Array of pointers used to facilitate easy debugging.  The first one points
  to the ps_mem header and the latter ponts to ps_dnsi_generic_rr_buf
----------------------------------------------------------------------------*/
static ps_mem_buf_hdr_type     * ps_dnsi_cache_rr_buf_hdr[PS_DNSI_CACHE_RR_BUF_NUM];
static ps_dnsi_generic_rr_type * ps_dnsi_cache_rr_buf_ptr[PS_DNSI_CACHE_RR_BUF_NUM];
#endif /* FEATURE_DATA_PS_MEM_DEBUG */

/*===========================================================================

                         INTERNAL FUNCTION DECLARATIONS

===========================================================================*/
/*===========================================================================
FUNCTION  PS_IFACE_DNS_CACHEI_MEM_ALLOC

DESCRIPTION
  The function is passed the amount of memory required.  If it finds a
  chunk of memory of suitable size to service the request it returns that
  otherwise it returns a NULL.

  This function may be called from tasks other then PS and therefore must
  be thread safe.

PARAMETERS
  size    - Size (in bytes) of the memory to be allocated.

RETURN VALUE
  Pointer to memory block if successful.
  NULL if could not get memory.

DEPENDENCIES
  None.

SIDE EFFECTS
  May allocate a large DSM item.  The DSM item is not freed until all memory
  allocated from it is freed.
===========================================================================*/
static void* ps_iface_dns_cachei_mem_alloc
(
  uint32 size
)
{
  uint32                     total_size;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Verify parameter
  -------------------------------------------------------------------------*/
  if( 0 == size || 0xfffffff0U < size )
  {
    LOG_MSG_ERROR( "Invalid size %d in ps_dnsi_memalloc()", size, 0, 0 );
    return NULL;
  }

  total_size = ((sizeof(int32) - 1) + size) & (~(sizeof(int32) - 1));

  /* Get from Modem heap */
  return ps_system_heap_mem_alloc( total_size );
} /* ps_iface_dns_cachei_mem_alloc() */

/*===========================================================================
FUNCTION  PS_IFACE_DNS_CACHEI_DUP_RR_Q()

DESCRIPTION
  Creates a duplicates Q from a given Q of RRs.

PARAMETERS
  dup_rr_q  - Duplicate Q, output
  rr_q      - Input RR Q.
              Can be NULL, in which case NULL will be returned and errno
              would be DSS_SUCCESS.
  ps_errno  - Error code in case of error.

RETURN VALUE
  None.

  errno values
  ------------
  DS_ENOMEM     - No memory for allocating any RR

DEPENDENCIES
  None

SIDE EFFECTS
  Memory will be allocated from PS Mem for the RR nodes, and from heap
  for the rdata section of the RR node.
===========================================================================*/
static int16 ps_iface_dns_cachei_dup_rr_q
(
  q_type      * dup_rr_q,
  q_type      * rr_q,
  int16       * ps_errno
)
{
  ps_dnsi_generic_rr_type   * rr_node     = NULL;
  ps_dnsi_generic_rr_type   * dup_rr_node = NULL;
  int16                       ret_val     = DSS_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ASSERT( NULL != dup_rr_q && NULL != ps_errno );

  /*-------------------------------------------------------------------------
    A NULL rr_q is valid in case of negative caching
  -------------------------------------------------------------------------*/
  if( NULL == rr_q )
  {
    dup_rr_q = NULL;
    return DSS_SUCCESS;
  }

  /*-------------------------------------------------------------------------
    Copy nodes from rr_q in to dup_rr_q
  -------------------------------------------------------------------------*/
  rr_node = q_check( rr_q );
  while( rr_node != NULL )
  {
    dup_rr_node =
      (ps_dnsi_generic_rr_type *) ps_mem_get_buf(PS_MEM_DNSI_GENERIC_RR_TYPE);
    if( NULL == dup_rr_node )
    {
      LOG_MSG_ERROR("Cant get PS mem for generic RR", 0, 0, 0);
      *ps_errno = DS_ENOMEM;
      ret_val = DSS_ERROR;
      break;
    }

    if( 0 < rr_node->rdata_len )
    {
      dup_rr_node->rdata = ps_iface_dns_cachei_mem_alloc( (uint32)rr_node->rdata_len );
      if( NULL == dup_rr_node->rdata )
      {
        ps_mem_free( (void **) &dup_rr_node );
        LOG_MSG_ERROR("Cant get memory for RDATA section", 0, 0, 0);

        *ps_errno = DS_ENOMEM;
        ret_val = DSS_ERROR;
        break;
      }
    }

    /*-----------------------------------------------------------------------
      Copy RR contents.
    -----------------------------------------------------------------------*/
    (void) q_link( dup_rr_node, &(dup_rr_node->link) );

    dup_rr_node->rr_type    = rr_node->rr_type;
    dup_rr_node->rr_class   = rr_node->rr_class;
    dup_rr_node->ttl        = rr_node->ttl;
    dup_rr_node->rdata_len  = rr_node->rdata_len;

    (void) std_strlcpy( dup_rr_node->domain_name,
                        rr_node->domain_name,
                        PS_DNSI_MAX_DOMAIN_NAME_LEN );
    if( 0 < rr_node->rdata_len )
    {
      memcpy( dup_rr_node->rdata,
              rr_node->rdata,
              (uint32)rr_node->rdata_len );
    }

    q_put( dup_rr_q, &dup_rr_node->link );

    rr_node = q_next( rr_q, &(rr_node->link) );
  }

  /*-------------------------------------------------------------------------
    Free all the nodes in dup_rr_q if all the nodes in rr_q couldn't be
    duplicated
  -------------------------------------------------------------------------*/
  if( ret_val == DSS_ERROR )
  {
    while( ( dup_rr_node = q_get( dup_rr_q ) ) != NULL )
    {
      if( 0 < dup_rr_node->rdata_len )
      {
        PS_SYSTEM_HEAP_MEM_FREE(dup_rr_node->rdata);
      }
      ps_mem_free( (void **) &dup_rr_node );
    }

    dup_rr_q = NULL;
  }

  return ret_val;

} /* ps_iface_dns_cachei_dup_rr_q() */

/*===========================================================================
FUNCTION  PS_IFACE_DNS_CACHEI_FREE_ENTRY()

DESCRIPTION
  Delete a cache entry from the cache and free memory associated with it.

  The specific cache is determined based on the iface_ptr member
  of cache_entry_data

PARAMETERS
  cache_entry_data   -  cache entry to be freed (Can be NULL).

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  None.
===========================================================================*/
static void ps_iface_dns_cachei_free_entry
(
  void  * cache_entry_data
)
{
  ps_dnsi_generic_rr_type       * rr_node = NULL;
  ps_dnsi_cache_entry_type      * cache_entry;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO2("Freeing entry 0x%p from cache for iface %d", cache_entry_data, 0, 0 );


  if( NULL == cache_entry_data )
  {
     return; //Nothing to free
  }

  cache_entry = ( ps_dnsi_cache_entry_type *) cache_entry_data;
  LOG_MSG_INFO2("Freeing entry 0x%p from cache for iface %d", cache_entry_data,
          cache_entry->iface_ptr->iface_private.iface_index, 0 );

  if (!PS_IFACE_IS_VALID(cache_entry->iface_ptr))
  {
     LOG_MSG_ERROR("Invalid iface, 0x%p, is passed", cache_entry->iface_ptr, 0, 0);
     ASSERT(0);
     return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-----------------------------------------------------------------------
    If pstimer is allocated for this entry, free it
  -----------------------------------------------------------------------*/
  if( PS_TIMER_INVALID_HANDLE != cache_entry->pstimer )
  {
    if( PS_TIMER_SUCCESS != ps_timer_free( cache_entry->pstimer ) )
    {
      LOG_MSG_ERROR("Cant free timer 0x%x", cache_entry->pstimer, 0, 0);
      ASSERT(0);
    }
  }

  /*-----------------------------------------------------------------------
    Free the RR queue associated with this cache entry.
  -----------------------------------------------------------------------*/
  while( NULL != (rr_node = q_get( &cache_entry->rr_q )) )
  {
     if ( 0 < rr_node->rdata_len )
     {
       PS_SYSTEM_HEAP_MEM_FREE(rr_node->rdata);
     }
     PS_MEM_FREE(rr_node);
  }
#ifdef FEATURE_Q_NO_SELF_QPTR
  q_delete( &( cache_entry->iface_ptr->iface_private.dns_cache_q ),
            &( cache_entry->link ) );
#else
  q_delete( &( cache_entry->link ) );
#endif

  q_destroy(&cache_entry->rr_q);

  PS_MEM_FREE(cache_entry);

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_dns_cachei_free_entry() */

/*===========================================================================
FUNCTION  PS_IFACE_DNS_CACHEI_PTON4()

DESCRIPTION
  Converts an IPV4 presentation (ASCII or printable) format address into its
  corresponding network (binary) format version.  The converted address is
  returned in the dst argument which should be large enough for the IPv4
  binary format address

DEPENDENCIES
  dst argument should have enough memory to contain an IPv4 binary format
  address

RETURN VALUE
  DSS_SUCCESS in case of success with the binary format address returned in
              the dst argument
  DSS_ERROR   in case of error with the error code returned in the dss_errno
              argument

SIDE EFFECTS
  Returns the converted printable format IPv6 address in the dst argument.
  Any errors are returned in dss_errno argument.
===========================================================================*/
static int ps_iface_dns_cachei_pton4
(
  const char    *src,
  unsigned char *dst,
  int16         *dss_errno
)
{
  const char digits[] = "0123456789";
  int saw_digit = FALSE;
  int octets = 0;
  unsigned char ch = 0;
  unsigned char tmp[sizeof(struct ps_in_addr)];
  unsigned char *tp = tmp;
  const char *pch;
  uint32 new_digit;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  *tp = 0;

  while ((ch = *src++) != '\0')
  {
    if ((pch = strchr(digits, ch)) != NULL)
    {
      new_digit = (uint32)(*tp * 10 + (pch - digits));

      if (saw_digit && *tp == 0)
      {
        LOG_MSG_INFO2("ps_iface_dns_cachei_pton4(): For octet %d value started with 0.  "
                "Return DS_NAMEERR", octets, 0, 0 );
        *dss_errno = DS_NAMEERR;
        return DSS_ERROR;
      }
      if (new_digit > 255)
      {
        LOG_MSG_INFO2("ps_iface_dns_cachei_pton4(): Value %d greater then 0xff in octet %d.  "
                "Return DS_NAMEERR", new_digit, octets, 0 );
        *dss_errno = DS_NAMEERR;
        return DSS_ERROR;
      }
      *tp = (unsigned char)new_digit;
      if (!saw_digit)
      {
        if (++octets > 4)
        {
          LOG_MSG_INFO2("ps_iface_dns_cachei_pton4(): Number of octets %d greater then 4.  "
                  "Return DS_NAMEERR", octets, 0, 0 );
          *dss_errno = DS_NAMEERR;
          return DSS_ERROR;
        }
        saw_digit = 1;
      }
    }
    else
    {
      if (ch == '.' && saw_digit)
      {
        if (octets == 4)
        {
          LOG_MSG_INFO2("ps_iface_dns_cachei_pton4(): '.' found after 4 octets.  Return "
                  "DS_NAMEERR", 0, 0, 0 );
          *dss_errno = DS_NAMEERR;
          return DSS_ERROR;
        }
        *++tp = 0;
        saw_digit = 0;
      }
      else
      {
        LOG_MSG_INFO2("ps_iface_dns_cachei_pton4(): %c found in octet %d no digits seen.  "
                "Return DS_NAMEERR", ch, octets, 0 );
        *dss_errno = DS_NAMEERR;
        return DSS_ERROR;
      }
    }
  }

  if (octets < 4)
  {
    LOG_MSG_INFO2("ps_iface_dns_cachei_pton4(): Only %d octets found. Return DS_NAMEERR",
            octets, 0, 0 );
    *dss_errno = DS_NAMEERR;
    return DSS_ERROR;
  }

  memcpy(dst, tmp, sizeof(struct ps_in_addr));
  return DSS_SUCCESS;
} /* ps_iface_dns_cachei_pton4() */

/*===========================================================================
FUNCTION  PS_IFACE_DNS_CACHEI_PTON6()

DESCRIPTION
  This function is called to convert an IPv6 presentation (ASCII) format
  address into its corresponding network (binary) format address.

DEPENDENCIES
  The dst argument should be large enough for an IPv6 address in network
  address format.

RETURN VALUE
  DSS_SUCCESS in case of success with the address returned in the dst
              argument
  DSS_ERROR   in case of error with the error code returned in the dss_errno
              argument

SIDE EFFECTS
  Returns the converted printable format IPv6 address in the dst argument.
  Any errors are returned in dss_errno argument.
 (1) does not touch `dst' unless it's returning 1.
 (2) :: in a full address is silently ignored.
===========================================================================*/
static int ps_iface_dns_cachei_pton6
(
  const char    *src,
  unsigned char *dst,
  int16         *dss_errno
)
{
  static const char xdigits_l[] = "0123456789abcdef",
                    xdigits_u[] = "0123456789ABCDEF";
  unsigned char tmp[sizeof(struct ps_in6_addr)], *tp, *endp, *colonp;
  const char *xdigits, *curtok;
  int ch, seen_xdigits;
  unsigned int val;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  memset((tp = tmp), '\0', sizeof(struct ps_in6_addr));
  endp = tp + sizeof(struct ps_in6_addr);
  colonp = NULL;

  /*------------------------------------------------------------------------
    Leading :: requires some special handling.
  ------------------------------------------------------------------------*/
  if (*src == ':')
  {
    if (*++src != ':')
    {
      LOG_MSG_INFO2("ps_iface_dns_cachei_pton6(): Address begins with single ':'.  Return "
              "DS_NAMEERR", 0, 0, 0 );
      *dss_errno = DS_NAMEERR;
      return DSS_ERROR;
    }
  }

  curtok = src;
  seen_xdigits = 0;
  val = 0;

  while ((ch = *src++) != '\0')
  {
    const char *pch;

    if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
    {
      pch = strchr((xdigits = xdigits_u), ch);
    }

    if (pch != NULL)
    {
      val <<= 4;
      val |= (unsigned int)(pch - xdigits);
      if (++seen_xdigits > 4)
      {
        LOG_MSG_INFO2("ps_iface_dns_cachei_pton6(): %d digits seen.  Return DS_NAMEERR",
                seen_xdigits, 0, 0 );
        *dss_errno = DS_NAMEERR;
        return DSS_ERROR;
      }
      continue;
    }

    if (ch == ':')
    {
      curtok = src;
      if (!seen_xdigits)
      {
        if (colonp)
        {
          LOG_MSG_INFO2("ps_iface_dns_cachei_pton6(): Duplicate :: found.  Return DS_NAMEERR",
                  0, 0, 0 );
          *dss_errno = DS_NAMEERR;
          return DSS_ERROR;
        }
        colonp = tp;
        continue;
      }
      else
      {
        if (*src == '\0')
        {
          LOG_MSG_INFO2("ps_iface_dns_cachei_pton6(): ':' found at end of address.  "
                  "Return DS_NAMEERR", 0, 0, 0 );
          *dss_errno = DS_NAMEERR;
          return DSS_ERROR;
        }
      }
      if (tp + sizeof(int16) > endp)
      {
        LOG_MSG_INFO2("ps_iface_dns_cachei_pton6(): Insufficient memory.  Need %d octets, "
                "have %d octets.  Return DS_EMSGTRUNC",
                sizeof(int16), endp - tp, 0 );
        *dss_errno = DS_EMSGTRUNC;
        return DSS_ERROR;
      }

      *tp++ = (unsigned char) (val >> 8) & 0xff;
      *tp++ = (unsigned char) val & 0xff;
      seen_xdigits = 0;
      val = 0;
      continue;
    }
    if (ch == '.' && ((tp + sizeof(struct ps_in_addr)) <= endp) &&
        ps_iface_dns_cachei_pton4(curtok, tp, dss_errno) == DSS_SUCCESS)
    {
      tp += sizeof(struct ps_in_addr);
      seen_xdigits = 0;
      break;  /* '\0' was seen by inet_pton4(). */
    }

    LOG_MSG_INFO2("dnsi_inet_pton4():  Found ch %c, need %d octets, have %d "
            "octets.  Return DS_NAMEERR",
            ch, sizeof(struct ps_in_addr), endp - tp );
    *dss_errno = DS_NAMEERR;
    return DSS_ERROR;
  }

  if (seen_xdigits)
  {
    if (tp + sizeof(int16) > endp)
    {
      LOG_MSG_INFO2("ps_iface_dns_cachei_pton6(): Insufficient memory.  Need %d octets, "
              "have %d octets.  Return DS_EMSGTRUNC",
              sizeof(int16), endp - tp, 0 );
      *dss_errno = DS_EMSGTRUNC;
      return DSS_ERROR;
    }

    *tp++ = (unsigned char) (val >> 8) & 0xff;
    *tp++ = (unsigned char) val & 0xff;
  }

  if (colonp != NULL)
  {
    /*----------------------------------------------------------------------
      Since some memmove()'s erroneously fail to handle overlapping
      regions, we'll do the shift by hand.
    ----------------------------------------------------------------------*/
    const int n = tp - colonp;
    int i;

    if (tp == endp)
    {
      LOG_MSG_INFO2("ps_iface_dns_cachei_pton6(): Insufficient memory.  Return DS_EMSGTRUNC",
              0, 0, 0 );
      *dss_errno = DS_EMSGTRUNC;
      return DSS_ERROR;
    }

    for (i = 1; i <= n; i++)
    {
      endp[- i] = colonp[n - i];
      colonp[n - i] = 0;
    }
    tp = endp;
  }

  if (tp != endp)
  {
    LOG_MSG_INFO2("ps_iface_dns_cachei_pton6(): Out of %d octets, processed %d octets.  "
            "Return DS_NAMEERR", endp - tmp, tp - tmp, 0 );
    *dss_errno = DS_NAMEERR;
    return DSS_ERROR;
  }

  memcpy(dst, tmp, sizeof(struct ps_in6_addr));
  return DSS_SUCCESS;
} /* ps_iface_dns_cachei_pton6() */

/*===========================================================================
FUNCTION  PS_IFACE_DNS_CACHEI_PTON()

DESCRIPTION
  This function is called with a presentation (printable or ASCII) format
  address to be converted to its network address (binary) format.  The af
  argument can be either DSS_AF_INET if the address to be converted is an IPv4
  address or DSS_AF_INET6 if the address is an IPv6 address.  In case of error
  the error code is returned in the dss_errno argument.

DEPENDENCIES
  The dst argument should have sufficient memory for the network address
  of the appropriate family.  For IPv4 it should be at least
  sizeof(struct ps_in_addr) while for IPv6 it should be at least
  sizeof(struct ps_in6_addr).

RETURN VALUE
  DSS_SUCCESS in case of success with the network format address
              returned in the dst argument.
  DSS_ERROR   in case of error with the error code returned in dss_errno
              argument.

              dss_errno values returned:
              DS_EFAULT         invalid arguments passed to function
              DS_EAFNOSUPPORT   invalid value for the address family
                                argument
              DS_NAMEERR        Malformed address passed to be converted
              DS_EMSGTRUNC      Insufficient buffer space in return argument

SIDE EFFECTS
  Returns the converted printable format IPv6 address in the dst argument.
  Any errors are returned in dss_errno argument.
===========================================================================*/
static int32 ps_iface_dns_cachei_pton
(
  const char *src,       /* String containing presentation form IP address */
  int32       af,        /* Address family of address in src argument      */
  void       *dst,       /* Memory for returning address in network format */
  uint32      dst_size,  /* Size of memory passed in dst argument          */
  int16      *dss_errno  /* Error code returned in case of DSS_ERROR return*/
)
{
  int32 retval = DSS_ERROR;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (dss_errno == NULL)
  {
    LOG_MSG_INFO2( "ps_iface_dns_cachei_pton(): Called with dss_errno NULL", 0, 0, 0 );
    return DSS_ERROR;
  }

  if (src == NULL || dst == NULL)
  {
    LOG_MSG_INFO2( "ps_iface_dns_cachei_pton(): Called with src %p, dst %p", src, dst, 0 );
    *dss_errno = DS_EFAULT;
    return DSS_ERROR;
  }

  if ((af == DSS_AF_INET && dst_size < sizeof(struct ps_in_addr)) ||
      (af == DSS_AF_INET6 && dst_size < sizeof(struct ps_in6_addr)))
  {
    LOG_MSG_INFO2( "ps_iface_dns_cachei_pton(): dst_size %d too small for af %d",
             dst_size, af, 0 );
    *dss_errno = DS_EFAULT;
    return DSS_ERROR;
  }

  switch (af)
  {
  case DSS_AF_INET:
    retval = ps_iface_dns_cachei_pton4(src, dst, dss_errno);
    break;

  case DSS_AF_INET6:
    retval = ps_iface_dns_cachei_pton6(src, dst, dss_errno);
    break;

  default:
    *dss_errno = DS_EAFNOSUPPORT;
    break;
  }

  return retval;
} /* ps_iface_dns_cachei_pton() */

// TODO: for now I duplicated the following function; also true for the 3 dss_inet_pton() functions; maybe there is a better solution
/*===========================================================================
FUNCTION PS_IFACE_DNS_CACHEI_ADD_TRAILING_DOT_TO_HOSTNAME

DESCRIPTION
  The function adds a trailing dot to a hostname string unless the hostname
  matches one of the following conditions:

  1. is an IPv4 or IPv6 numerical address

  2. already has a trailing dot

DEPENDENCIES
  hostname must not be NULL.
  hostname must not be of length PS_DNSI_MAX_DOMAIN_NAME_LEN (validate using,
    for example, dss_dnsi_validate_hostname_query())
  hostname must be allocated with space for an extra character - change is
    done in place.

PARAMETERS
  hostname - the hostname string to check.

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void ps_iface_dns_cachei_add_trailing_dot_to_hostname
(
  char *hostname
)
{
  struct ps_in_addr  temp_in_addr;
  struct ps_in6_addr temp_in6_addr;
  int32              retval = DSS_ERROR;
  int16              dss_errno = 0;
  int32              query_len = 0;

  retval = ps_iface_dns_cachei_pton( hostname,
                                     DSS_AF_INET,
                                     &temp_in_addr,
                                     sizeof(struct ps_in_addr),
                                     &dss_errno );
  if( DSS_ERROR == retval )
  {
    retval = ps_iface_dns_cachei_pton( hostname,
                                       DSS_AF_INET6,
                                       &temp_in6_addr,
                                       sizeof(struct ps_in6_addr),
                                       &dss_errno );
  }

  if( DSS_ERROR == retval )
  {
    query_len = std_strlen(hostname) ;
    if( '.' != hostname[ query_len - 1 ] )
    {
      hostname[ query_len ] = '.';
      hostname[ query_len + 1 ] = '\0';
    }
  }
} /* ps_iface_dns_cachei_add_trailing_dot_to_hostname() */

/*===========================================================================

                         EXTERNAL FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION  PS_IFACE_DNS_CACHE_INIT()

DESCRIPTION
  Initializes the cache Q during powerup.

PARAMETERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void ps_iface_dns_cache_init
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*-------------------------------------------------------------------------
    Initialize Pool
  -------------------------------------------------------------------------*/
  if (PS_MEM_POOL_INIT_OPT(PS_MEM_DNSI_CACHE_ENTRY_TYPE,
                           ps_dnsi_cache_entry_buf_mem,
                           PS_DNSI_CACHE_ENTRY_BUF_SIZE,
                           PS_DNSI_CACHE_ENTRY_BUF_NUM,
                           PS_DNSI_CACHE_ENTRY_BUF_HIGH_WM,
                           PS_DNSI_CACHE_ENTRY_BUF_LOW_WM,
                           NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                           (int *) ps_dnsi_cache_entry_buf_hdr,
                           (int *) ps_dnsi_cache_entry_buf_ptr
#else
                           NULL,
                           NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    LOG_MSG_FATAL_ERROR("Can't init the module", 0, 0, 0);
  }

  if (PS_MEM_POOL_INIT_OPT(PS_MEM_DNSI_GENERIC_RR_TYPE,
                           ps_dnsi_cache_rr_buf_mem,
                           PS_DNSI_CACHE_RR_BUF_SIZE,
                           PS_DNSI_CACHE_RR_BUF_NUM,
                           PS_DNSI_CACHE_RR_BUF_HIGH_WM,
                           PS_DNSI_CACHE_RR_BUF_LOW_WM,
                           NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                           (int *) ps_dnsi_cache_rr_buf_hdr,
                           (int *) ps_dnsi_cache_rr_buf_ptr
#else
                           NULL,
                           NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    LOG_MSG_FATAL_ERROR("Can't init the module", 0, 0, 0);
  }
} /* ps_dnsi_cache_mgr_init() */

/*===========================================================================
FUNCTION  PS_IFACE_DNS_CACHE_ADD_RR_Q()

DESCRIPTION
  Adds a Q of RRs as a single cache entry to the cache. If the entire
  Q cannot be added, then truncated flag will be set to true in the
  cache entry.

PARAMETERS
  iface_ptr       - Pointer to the interface of the cache queue
  query_type      - Type of query
  query_class     - Class of query
  query_data_ptr  - Query data
  rr_q            - Q of RRs to be added to cache. All the nodes must
                    be of type ps_dnsi_generic_rr_type.
                    A NULL rr_q specifies that this is a negative cache.
  ps_errno        - Error number in case of error

RETURN VALUE
  Returns DSS_SUCCESS in case of success.
  Returns DSS_ERROR on error and sets errno to the error code.

  errno values
  ------------
  DS_EFAULT - Invalid arguments.
  DS_ENOMEM - If not enough memory to add into cache.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
int16 ps_iface_dns_cache_add_rr_q
(
  ps_iface_type                  * iface_ptr,
  ps_dnsi_query_type_enum_type     query_type,
  ps_dnsi_query_class_enum_type    query_class,
  char                           * query_data_ptr,
  q_type                         * rr_q,
  int16                          * ps_errno
)
{
  ps_dnsi_cache_entry_type    * cache_entry        = NULL;
  ps_dnsi_generic_rr_type     * rr_node            = NULL;
  uint32                        min_ttl            = 0;
  int64                         ttl_in_ms;
  int32                         cache_max_entries  = 0;
  uint32                        cache_max_ttl      = 0;
  uint32                        cache_min_ttl      = 0;
  int32                         cache_negative_ttl = 0;
  int16                         result             = DSS_SUCCESS;
  boolean                       in_cache           = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2("Adding qtype %d, qclass %d, qdata 0x%p to cache",
          query_type, query_class, query_data_ptr);

  LOG_MSG_INFO2("Adding qtype %d, qclass %d, to cache for iface %d",
          query_type, query_class, iface_ptr->iface_private.iface_index);


  if( NULL == ps_errno )
  {
    LOG_MSG_ERROR("NULL errno", 0, 0, 0);
    ASSERT(0);
    return DSS_ERROR;
  }

  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface, 0x%p, is passed", iface_ptr, 0, 0);
    ASSERT(0);
    *ps_errno = DS_EFAULT;
    return DSS_ERROR;
  }

  if( PS_DNSI_QUERY_TYPE_MIN  >  query_type   ||
      PS_DNSI_QUERY_TYPE_MAX  <= query_type   ||
      PS_DNSI_QUERY_CLASS_MIN >  query_class  ||
      PS_DNSI_QUERY_CLASS_MAX <= query_class  ||
      NULL == query_data_ptr )
  {
    LOG_MSG_ERROR("Invalid arguments", 0, 0, 0);
    ASSERT(0);
    *ps_errno = DS_EFAULT;
    return DSS_ERROR;
  }

  if( DSS_ERROR == ps_dns_cache_getconst( PS_DNS_CACHE_CONST_MAX_ENTRIES,
                                          &cache_max_entries,
                                          sizeof(int32) ) )
  {
    LOG_MSG_ERROR("Can't get const for max cache entries", 0, 0, 0);
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return DSS_ERROR;
  }
  /*-------------------------------------------------------------------------
  Look into the correct queue based on the iface_id
  -------------------------------------------------------------------------*/

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    Check if the entries are already present in the cache.
  -------------------------------------------------------------------------*/
  cache_entry = q_check( &iface_ptr->iface_private.dns_cache_q );
  while( NULL != cache_entry )
  {
    if( cache_entry->query_type  == (uint16)query_type      &&
        cache_entry->query_class == (uint16)query_class     &&
        ((0 == std_strnicmp( cache_entry->query_data,
                             query_data_ptr,
                             PS_DNSI_MAX_DOMAIN_NAME_LEN )) ||
         (0 == std_strnicmp( cache_entry->cname,
                             query_data_ptr,
                             PS_DNSI_MAX_DOMAIN_NAME_LEN ))) )
    {
      LOG_MSG_INFO2("Found a cache match", 0, 0, 0);
      in_cache = TRUE;
      break;
    } /* if */

    cache_entry = q_next( &iface_ptr->iface_private.dns_cache_q,
                          &(cache_entry->link) );
  }

  if (TRUE == in_cache)
  {
    /*-------------------------------------------------------------------------
      Results are already present in the cache. Update the TTL value.
    -------------------------------------------------------------------------*/
    rr_node = q_check( &cache_entry->rr_q );
    while( NULL != rr_node )
    {
      if( cache_entry->min_ttl > rr_node->ttl )
      {
        cache_entry->min_ttl = rr_node->ttl;
      }
      rr_node = q_next( &cache_entry->rr_q, &(rr_node->link) );
    } /* while */

    /*-------------------------------------------------------------------------
      Restart the Timer.
    -------------------------------------------------------------------------*/
    ttl_in_ms = ((int64) cache_entry->min_ttl) * 1000;
    if (PS_TIMER_FAILURE == ps_timer_start(cache_entry->pstimer, ttl_in_ms))
    {
      LOG_MSG_FATAL_ERROR( "Timer %d start failed", cache_entry->pstimer, 0, 0 );
      *ps_errno = DS_ESYSTEM;
      result = DSS_ERROR;
    }

    // In order to preserve the queue to be managed using LRU, this cache entry
    // should be moved to the end of the queue.
#ifdef FEATURE_Q_NO_SELF_QPTR
    q_delete( &iface_ptr->iface_private.dns_cache_q, &( cache_entry->link ) );
#else
    q_delete( &( cache_entry->link ) );
#endif

    q_put( &iface_ptr->iface_private.dns_cache_q, &( cache_entry->link ) );

    goto bail;
  }

  if( NULL != rr_q ) {

    if( DSS_ERROR == ps_dns_cache_getconst( PS_DNS_CACHE_CONST_MAX_TTL,
                                            &cache_max_ttl,
                                            sizeof(uint32) ) ) {
      LOG_MSG_ERROR("Can't get const for max cache TTL", 0, 0, 0);
      ASSERT(0);
      ps_iface_dns_cachei_free_entry( cache_entry );
      result = DSS_ERROR;
      goto bail;
    }

    if( DSS_ERROR == ps_dns_cache_getconst( PS_DNS_CACHE_CONST_MIN_TTL,
                                            &cache_min_ttl,
                                            sizeof(uint32) ) ) {
      LOG_MSG_ERROR("Can't get const for min cache TTL", 0, 0, 0);
      ASSERT(0);
      ps_iface_dns_cachei_free_entry( cache_entry );
      result = DSS_ERROR;
      goto bail;
    }

    // Calculate the minimum TTL

    min_ttl = cache_max_ttl;

    rr_node = q_check( rr_q );
    while( NULL != rr_node ) {
      if( min_ttl > rr_node->ttl ) {
        min_ttl = rr_node->ttl;
      }

      if( min_ttl < cache_min_ttl ) {
        LOG_MSG_INFO2("The TTL is smaller than the minimum allowed; will not create a cache entry for the response", 0, 0, 0);
        goto bail;
      }
      rr_node = q_next( rr_q, &(rr_node->link) );
    }
  }

  if( cache_max_entries <= q_cnt( &iface_ptr->iface_private.dns_cache_q ) )
  {
     LOG_MSG_INFO1("Cache on iface %x full, oldest entry removed",
              iface_ptr->iface_private.iface_index, 0, 0);

     // Get the first cache entry from the queue and remove it.
     ps_iface_dns_cachei_free_entry( q_check( &iface_ptr->iface_private.dns_cache_q ) );
  }

  /*-------------------------------------------------------------------------
    Get memory for Cache entry and initialize it
  -------------------------------------------------------------------------*/
  cache_entry = ps_mem_get_buf(PS_MEM_DNSI_CACHE_ENTRY_TYPE);
  if( NULL == cache_entry )
  {
    LOG_MSG_ERROR("Can't get memory for cache entry", 0, 0, 0);
    *ps_errno = DS_ENOMEM;
    result = DSS_ERROR;
    goto bail;
  }

  memset( cache_entry, 0, sizeof( ps_dnsi_cache_entry_type ) );
  (void) q_link( cache_entry, &cache_entry->link );
  memset( &cache_entry->rr_q, 0, sizeof(q_type) );
  (void) q_init( &cache_entry->rr_q );

  /*-------------------------------------------------------------------------
    Indicate the iface ID so when the timer callback calls
    ps_iface_dns_cachei_free_entry the specific cache can be
    determined (there is a cache per iface_id).
    -------------------------------------------------------------------------*/
  cache_entry->iface_ptr = iface_ptr;
  /*-------------------------------------------------------------------------
    As ps_iface_dns_cachei_free_entry() dequeues a node from ps_dnsi_cache_q,
    cache_entry is enqueued before hand so that ps_iface_dns_cachei_free_entry()
    finds the entry in the cache even if that function is called because
    one of the following operations failed
  -------------------------------------------------------------------------*/
  q_put( &iface_ptr->iface_private.dns_cache_q, &( cache_entry->link ) );

  /*-------------------------------------------------------------------------
    Allocate timer for cleaning up the cache entry.
  -------------------------------------------------------------------------*/
  cache_entry->pstimer = ps_timer_alloc( ps_iface_dns_cachei_free_entry,
                                         (void *) cache_entry );

  if( (uint32)PS_TIMER_INVALID_HANDLE == cache_entry->pstimer )
  {
    LOG_MSG_ERROR( "Failed to acquire timer", 0, 0, 0 );
    *ps_errno = DS_ESYSTEM;
    ps_iface_dns_cachei_free_entry( cache_entry );
    result = DSS_ERROR;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Fill up the cache entry.
  -------------------------------------------------------------------------*/
  (void) std_strlcpy( cache_entry->query_data,
                      query_data_ptr,
                      PS_DNSI_MAX_DOMAIN_NAME_LEN );
  cache_entry->query_type  = (uint16)query_type;
  cache_entry->query_class = (uint16)query_class;

  /*-------------------------------------------------------------------------
    DNS server responds with zero RRs in case of negative caching
  -------------------------------------------------------------------------*/
  if( NULL == rr_q)
  {
    if( DSS_ERROR == ps_dns_cache_getconst( PS_DNS_CACHE_CONST_NEGATIVE_TTL,
                                            &cache_negative_ttl,
                                            sizeof(int32) ) )
    {
      LOG_MSG_ERROR("Can't get const for negative cache TTL", 0, 0, 0);
      ASSERT(0);
      ps_iface_dns_cachei_free_entry( cache_entry );
      result = DSS_ERROR;
      goto bail;
    }

    cache_entry->min_ttl = (uint32)cache_negative_ttl;
  }
  else
  {
    /*-----------------------------------------------------------------------
      Copy RRs in to the cache
    -----------------------------------------------------------------------*/
    if( DSS_ERROR ==
          ps_iface_dns_cachei_dup_rr_q( &(cache_entry->rr_q), rr_q, ps_errno ) )
    {
      LOG_MSG_ERROR("Couldn't dup rr_q", 0, 0, 0);
      ps_iface_dns_cachei_free_entry( cache_entry );
      result = DSS_ERROR;
      goto bail;
    }

    /*-----------------------------------------------------------------------
      Update cache entry's TTL with the minimum TTL among all RRs. Once
      this TTL elapses, all RRs become invalid
    -----------------------------------------------------------------------*/
    cache_entry->min_ttl = min_ttl;

    /*-----------------------------------------------------------------------
      If any of the RRs is a CNAME RR, store the alias in the cache entry.
    -----------------------------------------------------------------------*/
    rr_node = q_check( &cache_entry->rr_q );
    while( NULL != rr_node )
    {
      if( PS_DNSI_QUERY_TYPE_CNAME == rr_node->rr_type )
      {
        (void) std_strlcpy( cache_entry->cname,
                            rr_node->domain_name,
                            PS_DNSI_MAX_DOMAIN_NAME_LEN );
      }

      rr_node = q_next( &cache_entry->rr_q, &(rr_node->link) );
    } /* while */
  }

  /*-------------------------------------------------------------------------
    Start the timer corresponding to TTL in the cache entry
  -------------------------------------------------------------------------*/
  ttl_in_ms = ((int64) cache_entry->min_ttl) * 1000;
  if ( PS_TIMER_FAILURE == ps_timer_start( cache_entry->pstimer, ttl_in_ms ) )
  {
    LOG_MSG_FATAL_ERROR( "Timer %d start failed", cache_entry->pstimer, 0, 0 );
    ps_iface_dns_cachei_free_entry( cache_entry );
    *ps_errno = DS_ESYSTEM;
    result = DSS_ERROR;
    goto bail;
  }

bail:
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return result;

} /* ps_iface_dns_cache_add_rr_q() */

/*===========================================================================
FUNCTION  PS_IFACE_DNS_CACHE_FIND_RR_Q()

DESCRIPTION
  Finds a Q of RRs corresponding to a query.

PARAMETERS
  iface_ptr       - Pointer to the interface of the cache queue
  query_type      - Type of query
  query_class     - Class of query
  query_data_ptr  - Query data
  rr_q            - Resultant RR Q.
  ps_errno        - Error code in case of error.

RETURN VALUE
  Returns the Q of RRs if found in cache. Each node of the Queue can be
  cast to ps_dnsi_generic_rr_type.
  Returns NULL if query results are not found in cache or its a
  negative cache hit.

  errno values
  ------------
  DS_ENOTFOUND    - Cache results are not found
  DS_EFAULT       - Invalid arguments
  DS_ENOMEM       - No memory for the RR q.
  DSS_SUCCESS     - On Negative or positive cache hit.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
int16 ps_iface_dns_cache_find_rr_q
(
  ps_iface_type                  * iface_ptr,
  ps_dnsi_query_type_enum_type     query_type,
  ps_dnsi_query_class_enum_type    query_class,
  char                           * query_data_ptr,
  q_type                         * rr_q,
  int16                          * ps_errno
)
{
  ps_dnsi_cache_entry_type      * cache_entry;
  int16                           result = DSS_SUCCESS;
  boolean                         found_flag = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2("Finding qtype %d, qclass %d, qdata 0x%p in cache",
          query_type, query_class, query_data_ptr);
  LOG_MSG_INFO2("Finding qtype %d, qclass %d, in cache %d",
     query_type, query_class, iface_ptr->iface_private.iface_index);

  if( NULL == ps_errno )
  {
    LOG_MSG_ERROR("NULL errno", 0, 0, 0);
    ASSERT(0);
    return DSS_ERROR;
  }

  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface, 0x%p, is passed", iface_ptr, 0, 0);
    ASSERT(0);
    *ps_errno = DS_EFAULT;
    return DSS_ERROR;
  }

  if( NULL == rr_q )
  {
    LOG_MSG_ERROR("NULL rr_q", 0, 0, 0);
    ASSERT(0);
    *ps_errno = DS_EFAULT;
    return DSS_ERROR;
  }

  if( PS_DNSI_QUERY_TYPE_MIN  >  query_type   ||
      PS_DNSI_QUERY_TYPE_MAX  <= query_type   ||
      PS_DNSI_QUERY_CLASS_MIN >  query_class  ||
      PS_DNSI_QUERY_CLASS_MAX <= query_class  ||
      NULL == query_data_ptr )
  {
    LOG_MSG_ERROR("Invalid arguments", 0, 0, 0);
    ASSERT(0);
    *ps_errno = DS_EFAULT;
    return DSS_ERROR;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    Traverse the cache to check if there is an entry with the same
    query type, class, and data. Also check if any alias matches the query data

    No need to enter critical section as ps_dnsi_cache_q is accessed only in
    PS context
  -------------------------------------------------------------------------*/
  cache_entry = q_check( &iface_ptr->iface_private.dns_cache_q );
  while( NULL != cache_entry )
  {
    if( cache_entry->query_type  == (uint16)query_type              &&
        cache_entry->query_class == (uint16)query_class             &&
        ((0 == std_strnicmp( cache_entry->query_data,
                             query_data_ptr,
                             PS_DNSI_MAX_DOMAIN_NAME_LEN )) ||
         (0 == std_strnicmp( cache_entry->cname,
                             query_data_ptr,
                             PS_DNSI_MAX_DOMAIN_NAME_LEN ))) )
    {
      LOG_MSG_INFO2("Found a cache match", 0, 0, 0);
      if( DSS_SUCCESS !=
            ps_iface_dns_cachei_dup_rr_q( rr_q, &cache_entry->rr_q, ps_errno ) )
      {
        LOG_MSG_ERROR("Cant dup RR list, error %d", *ps_errno, 0, 0);
        result = DSS_ERROR;
        goto bail;
      }

      found_flag = TRUE;
      break;
    }

    cache_entry = q_next( &iface_ptr->iface_private.dns_cache_q, &(cache_entry->link) );
  }

  if (FALSE == found_flag)
  {
    *ps_errno = DS_EHOSTNOTFOUND;
    result = DSS_ERROR;
  }

bail:
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return result;

} /* ps_iface_dns_cache_find_rr_q() */

/*===========================================================================
FUNCTION  PS_IFACE_DNS_CACHE_FLUSH()

DESCRIPTION
  Deletes all entries in the cache of the specified iface_ptr and frees memory
  associated with them.

PARAMETERS
  iface_ptr       - Pointer to the interface of the cache queue
  ps_errno        - Error number in case of error

RETURN VALUE
  Returns DSS_SUCCESS in case of success.
  Returns DSS_ERROR on error and sets errno to the error code.

  errno values
  ------------
  DS_EFAULT - Invalid arguments.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
int16 ps_iface_dns_cache_flush
(
  ps_iface_type                  * iface_ptr,
  int16                          * ps_errno
)
{
  ps_dnsi_cache_entry_type      * cache_entry;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1("Flushing the cache for iface index %d", iface_ptr->iface_private.iface_index, 0, 0);

  if( NULL == ps_errno )
  {
     LOG_MSG_ERROR("NULL errno", 0, 0, 0);
     ASSERT(0);
     return DSS_ERROR;
  }

  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
     LOG_MSG_ERROR("Invalid iface, 0x%p, is passed", iface_ptr, 0, 0);
     ASSERT(0);
     *ps_errno = DS_EFAULT;
     return DSS_ERROR;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  while( NULL != (cache_entry = q_get( &iface_ptr->iface_private.dns_cache_q ) ) )
  {
    ps_iface_dns_cachei_free_entry( cache_entry );
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return DSS_SUCCESS;
} /* ps_iface_dns_cache_flush() */

/*===========================================================================
FUNCTION  PS_IFACE_DNS_CACHE_FLUSH_ENTRY()

DESCRIPTION
  Deletes an entry in the cache of the specified iface_ptr.
  The entry is specified by host name.
  Entries with query type PS_DNSI_QUERY_TYPE_A or PS_DNSI_QUERY_TYPE_AAAA
  or PS_DNSI_QUERY_TYPE_CNAME with the specified host name are deleted (flushed)

PARAMETERS
  iface_ptr       - Pointer to the interface of the cache queue
  hostname_ptr    - Name of the host for which entries should be flushed
  ps_errno        - Error number in case of error

RETURN VALUE
  Returns DSS_SUCCESS in case of success.
  Returns DSS_ERROR on error and sets errno to the error code.

  errno values
  ------------
  DS_EFAULT - Invalid arguments.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
int16 ps_iface_dns_cache_flush_entry
(
  ps_iface_type                  * iface_ptr,
  const char                     * hostname_ptr,
  int16                          * ps_errno
)
{
  ps_dnsi_cache_entry_type      * cache_entry;
  ps_dnsi_cache_entry_type      * next_cache_entry;
  char                            hostname[PS_DNSI_MAX_DOMAIN_NAME_LEN];
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2("ps_iface_dns_cache_flush_entry iface %d",
          iface_ptr->iface_private.iface_index, 0, 0);

  if( NULL == ps_errno )
  {
    LOG_MSG_ERROR("NULL errno", 0, 0, 0);
    ASSERT(0);
    return DSS_ERROR;
  }

  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface, 0x%p, is passed", iface_ptr, 0, 0);
    ASSERT(0);
    *ps_errno = DS_EFAULT;
    return DSS_ERROR;
  }

  if( NULL == hostname_ptr )
  {
    LOG_MSG_ERROR("NULL hostname_ptr", 0, 0, 0);
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return DSS_ERROR;
  }

  (void) std_strlcpy( hostname, hostname_ptr, PS_DNSI_MAX_DOMAIN_NAME_LEN );

  ps_iface_dns_cachei_add_trailing_dot_to_hostname(hostname);

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    Traverse the cache to check if there is an entry with the same
    query type, class, and data. Also check if any alias matches the query data

    No need to enter critical section as ps_dnsi_cache_q is accessed only in
    PS context
  -------------------------------------------------------------------------*/
  cache_entry = q_check( &iface_ptr->iface_private.dns_cache_q );
  while( NULL != cache_entry )
  {
    next_cache_entry = q_next( &iface_ptr->iface_private.dns_cache_q,
                               &(cache_entry->link) );

    if(
       /*if query type is PS_DNSI_QUERY_TYPE_A or PS_DNSI_QUERY_TYPE_AAAA
       then need to compare hostname to query_data*/
       ((
         ( (int)PS_DNSI_QUERY_TYPE_A == cache_entry->query_type ) ||
         ( (int)PS_DNSI_QUERY_TYPE_AAAA == cache_entry->query_type )
        )&&
        ( 0 == std_strnicmp( cache_entry->query_data,
                             hostname,
                             PS_DNSI_MAX_DOMAIN_NAME_LEN )
        )) ||
       /*if query type is PS_DNSI_QUERY_TYPE_CNAME
       then need to compare hostname to cname*/
       (
         ( (int)PS_DNSI_QUERY_TYPE_CNAME == cache_entry->query_type ) &&
         (
          0 == std_strnicmp( cache_entry->cname,
                             hostname,
                             PS_DNSI_MAX_DOMAIN_NAME_LEN )
         )
        )
       )
    {/*The entry match so free it*/
      LOG_MSG_INFO2("Found a match", 0, 0, 0);
      ps_iface_dns_cachei_free_entry( cache_entry );
      /* Don't quit the while since there might be more entries for this
         hostname - for example, when both A and CNAME results are in cache */
    }

    cache_entry = next_cache_entry;
  }/*  while( NULL != cache_entry ) */
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return DSS_SUCCESS;
} /* ps_iface_dns_cache_flush_entry() */

#endif  /* FEATURE_DATA_PS_DNS */
#endif  /* FEATURE_DATA_PS */
