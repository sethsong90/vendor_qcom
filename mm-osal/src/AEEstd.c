/*===========================================================================
                          M M   W r a p p e r
                   f o r   C   STD LIB   R o u t i n e s

*//** @file  AEEstd.c
   This module defines routines to track and debug memory related usage

   Copyright (c) 2012 Qualcomm Technologies, Inc.
   All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*========================================================================*/

/* ==========================================================================
                     INCLUDE FILES FOR MODULE
========================================================================== */
#include "AEEstd.h"

/* =======================================================================

                        DATA DECLARATIONS

========================================================================== */

/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */

int  std_getversion(char *pcDst, int nDestSize)
{
   return std_strlcpy(pcDst, /*VERSION_STRING*/ "1.1", nDestSize);
}


char std_tolower(char c)
{
   if ((c >= 'A') && (c <= 'Z')) {
      c |= 32;
   }
   return c;
}

char std_toupper(char c)
{
   if ((c >= 'a') && (c <= 'z')) {
      c &= ~32;
   }
   return c;
}


static __inline int x_casecmp(unsigned char c1, unsigned char c2)
{
   int diff = c1 - c2;
   if (c1 >= 'A' && c1 <= 'Z') {
      diff += 32;
   }
   if (c2 >= 'A' && c2 <= 'Z') {
      diff -= 32;
   }
   return diff;
}


int std_strlen(const char* sStart)
{
   // To test 4 bytes at a time, evaluate (u - 0x01010101) & ~u &
   // 0x808080. It will be non-zero only when any byte in u is
   // zero.
   //
   // On RVCT3.0 ARM11, this inner loop is 6 instructions (per 4
   // bytes), and the function fits in two 32-byte cache lines.
   //
   const char *s = sStart;
   uint32 u;
   const uint32 *pu;

   // advance to alignment boundary
   while ( 0 != (3 & (size_t)s) ) {
      if (! *s) {
         return s - sStart;
      }
      ++s;
   }

   // Test four bytes at a time
   pu = (uint32*)s;
   while (u = *pu++, 0 == ((u - 0x01010101) & ~u & 0x80808080))
      ;

   s = (const char*)(pu - 1);

   while (*s) {
      ++s;
   }

   return s - sStart;
}

int std_strncmp(const char* s1, const char* s2, int n)
{
   if (n > 0) {
      int i;

      s1 += n;
      s2 += n;
      i = -n;
      do {
         unsigned char c1 = (unsigned char)s1[i];
         unsigned char c2 = (unsigned char)s2[i];
         int  diff = c1 - c2;

         if (diff) {
            return diff;
         }

         if ('\0' == c1) {
            break;
         }
      } while (++i);
   }

   return 0;
}

int std_strcmp(const char* s1, const char* s2)
{
   return std_strncmp(s1, s2, MAX_INT32);
}

int std_strnicmp(const char* s1, const char* s2, int n)
{
   if (n > 0) {
      int i = -n;

      s1 += n;
      s2 += n;

      do {
         unsigned char c1 = (unsigned char)s1[i];
         unsigned char c2 = (unsigned char)s2[i];

         int diff = x_casecmp(c1,c2);
         if (diff) {
            return diff;
         }
         if ('\0' == c1) {
            break;
         }
      } while (++i);
   }
   return 0;
}

int std_stricmp(const char* s1, const char* s2)
{
   return std_strnicmp(s1, s2, MAX_INT32);
}

int std_strlcpy(char* pcDst, const char* cpszSrc, int nDestSize)
{
   int nLen = std_strlen(cpszSrc);

   if (0 < nDestSize) {
      int n;

      n = STD_MIN(nLen, nDestSize - 1);
      (void)std_memmove(pcDst, cpszSrc, n);

      pcDst[n] = 0;
   }

   return nLen;
}

int std_strlcat(char* pcDst, const char* cpszSrc, int nDestSize)
{
   int nLen = 0;

   while ((nLen < nDestSize) && (0 != pcDst[nLen])) {
      ++nLen;
   }

   return nLen + std_strlcpy(pcDst+nLen, cpszSrc, nDestSize-nLen);
}

char* std_strstr(const char* cpszHaystack, const char* cpszNeedle)
{
   /* Check the empty needle string as a special case */
   if ('\0' == *cpszNeedle ) {
      return (char*)cpszHaystack;
   }

   while ('\0' != *cpszHaystack) {
      /* Find the first character of the needle string in the haystack string */
      if (*cpszHaystack == *cpszNeedle) {
         /* check if the rest of the string matches */
         const char* pHaystack = cpszHaystack;
         const char* pNeedle = cpszNeedle;
         do {
            if ('\0' == *++pNeedle) {
               /* Found a match */
               return (char*)pHaystack;
            }
         } while (*++cpszHaystack == *pNeedle);
      }
      else {
         cpszHaystack++;
      }
   }

   return 0;
}


int std_memcmp(const void* p1, const void* p2, int length)
{
   const unsigned char *cpc1 = p1;
   const unsigned char *cpc2 = p2;

   while (length-- > 0) {
      int diff = *cpc1++ - *cpc2++;

      if (0 != diff) {
         return diff;
      }
   }
   return 0;
}


int std_wstrlen(const AECHAR* s)
{
   const AECHAR *sEnd = s;

   if (! *sEnd)
      return 0;

   do {
      ++sEnd;
   } while (*sEnd);

   return sEnd - s;
}

#if 0
int std_wstrlcpy(AECHAR* pwcDst, const AECHAR* cpwszSrc, int nDestSize)
{
   int nLen = std_wstrlen(cpwszSrc);

   if (0 < nDestSize) {
      int n;

      n = STD_MIN(nLen, nDestSize - 1);
      /* call memmove, in case n is larger than 1G */
      (void)memmove(pwcDst, cpwszSrc, ((size_t)n)*sizeof(AECHAR));

      pwcDst[n] = 0;
   }

   return nLen;
}

int std_wstrlcat(AECHAR* pwcDst, const AECHAR* cpwszSrc, int nDestSize)
{
   int nLen = 0;

   while ((nLen < nDestSize) && (0 != pwcDst[nLen])) {
      ++nLen;
   }

   return nLen + std_wstrlcpy(pwcDst+nLen, cpwszSrc, nDestSize-nLen);
}
#endif

char* std_strchrend(const char* cpsz, char c)
{
   while (*cpsz && *cpsz != c) {
      ++cpsz;
   }
   return (char*)cpsz;
}

char* std_strchr(const char* cpszSrch, int c)
{
   const char *pc = std_strchrend(cpszSrch, (char)c);

   return (*pc == c ? (char*)pc : 0);
}

void* std_memstr(const char* cpHaystack, const char* cpszNeedle,
                 int nHaystackLen)
{
   int nLen = 0;

   /* Handle empty needle string as a special case */
   if ('\0' == *cpszNeedle ) {
      return (char*)cpHaystack;
   }

   /* Find the first character of the needle string in the haystack string */
   while (nLen < nHaystackLen) {
      if (cpHaystack[nLen] == *cpszNeedle) {
         /* check if the rest of the string matches */
         const char* cpNeedle = cpszNeedle;
         int nRetIndex = nLen;
         do {
            if ('\0' == *++cpNeedle) {
               /* Found a match */
               return (void*)(cpHaystack + nRetIndex);
            }
            nLen++;
         } while(cpHaystack[nLen] == *cpNeedle);
      }
      else {
         nLen++;
      }
   }

   return 0;
}

void* std_memchrend(const void* p, int c, int nLen)
{
   const char* cpc = (const char*)p + nLen;
   int i = -nLen;

   if (nLen > 0) {
      do {
         if (cpc[i] == c) {
            break;
         }
      } while (++i);
   }
   return (void*) (cpc + i);
}

void* std_memchr(const void* s, int c, int n)
{
   const char *pEnd = (const char*)std_memchrend(s,c,n);
   int nEnd = pEnd - (const char*)s;

   if (nEnd < n) {
      return (void*)pEnd;
   }
   return 0;
}

void* std_memrchr(const void* p, int c, int nLen)
{
   const char* cpc = (const char*)p - 1;

   if (nLen > 0) {
      do {
         if (cpc[nLen] == c) {
            return (void*) (cpc + nLen);
         }
      } while (--nLen);
   }

   return 0;
}


char* std_strrchr(const char* cpsz, int c)
{
   return std_memrchr(cpsz, c, std_strlen(cpsz) + 1);
}


void* std_memrchrbegin(const void* p, int c, int n)
{
   void *pOut = std_memrchr(p, c, n);

   return (pOut ? pOut : (void*)p);
}


// x_scanbytes: internal function;  WARNING:  nLen must be >0
//
// cStop = character at which to stop (in addition to cpszChars[...])
//
// Using a bit mask provides a constant-time check for a terminating
// character: 10 instructions for inner loop on ADS12arm9.  Initialization
// overhead is increased, but this is quickly made up for as searching begins.
//
//
static char *x_scanbytes(const char *pcBuf, const char* cpszChars,
                         int nLen, unsigned char cStop, boolean bTestEqual)
{
   int n;
   unsigned a[8];

   // Initialize bit mask based on the input flag that specifies whether
   // we are looking for a character that matches "any" or "none"
   // of the characters in the search string

   #define ENTRY(c)   a[((c)&7)]   // c's bit lives here
   #define SHIFT(c)   ((c)>>3)     // c's bit is shifted by this much

   if (bTestEqual) {
      std_memset(a, 0, STD_SIZEOF(a));
      do {
         ENTRY(cStop) |= (0x80000000U >> SHIFT(cStop));
         cStop = (unsigned char)*cpszChars++;
      } while (cStop);
   }
   else {
      std_memset(a, 0xFF, STD_SIZEOF(a));

      while (0 != (cStop = (unsigned char)*cpszChars++)) {
         ENTRY(cStop) ^= (0x80000000U >> SHIFT(cStop));
      }
   }


   // Search buffer

   pcBuf += nLen;
   n = -nLen;
   do {
      unsigned char uc = (unsigned char)pcBuf[n];
      // testing for negative after shift is quicker than comparison
      if ( (int)(ENTRY(uc) << SHIFT(uc)) < 0) {
         break;
      }
   } while (++n);

   return (char*)(pcBuf+n);
}


void* std_memchrsend(const void* pBuf, const char* cpszChars, int nLen)
{
   if (nLen <= 0) {
      return (void*)pBuf;
   }
   if ('\0' == *cpszChars) {
      return (char*)pBuf + nLen;
   }

   return x_scanbytes((const char*)pBuf, cpszChars+1, nLen,
                      (unsigned char)*cpszChars, TRUE);
}


char* std_strchrsend(const char* cpszSrch, const char* cpszChars)
{
   return x_scanbytes(cpszSrch, cpszChars, MAX_INT32, '\0', TRUE);
}


char *std_strchrs(const char* cpszSrch, const char* cpszChars)
{
   const char *pc = std_strchrsend(cpszSrch, cpszChars);

   return (*pc ? (char*)pc : 0);
}


char* std_striends(const char* cpsz, const char* cpszSuffix)
{
   int nOffset = std_strlen(cpsz) - std_strlen(cpszSuffix);

   if ((0 <= nOffset) &&
       (0 == std_stricmp(cpsz+nOffset, cpszSuffix))) {

      return (char*)(cpsz+nOffset);
   }

   return 0;
}


char* std_strends(const char* cpsz, const char* cpszSuffix)
{
   int nOffset = std_strlen(cpsz) - std_strlen(cpszSuffix);

   if ((0 <= nOffset) &&
       (0 == std_strcmp(cpsz+nOffset, cpszSuffix))) {

      return (char*)(cpsz + nOffset);
   }

   return 0;
}

char* std_strbegins(const char* cpsz, const char* cpszPrefix)
{
   for (;;) {
      if ('\0' == *cpszPrefix) {
         return (char*)cpsz;
      }

      if (*cpszPrefix != *cpsz) {
         return 0;
      }

      ++cpszPrefix;
      ++cpsz;
   }
   // not reached
}

char* std_stribegins(const char* cpsz, const char* cpszPrefix)
{
   for (;;) {
      if ('\0' == *cpszPrefix) {
         return (char*)cpsz;
      }

      if (x_casecmp((unsigned char)*cpszPrefix, (unsigned char)*cpsz)) {
         return 0;
      }

      ++cpszPrefix;
      ++cpsz;
   }
   // not reached
}

int std_strcspn(const char* cpszSrch, const char* cpszChars)
{
   const char *pc = x_scanbytes(cpszSrch, cpszChars, MAX_INT32, '\0', TRUE);

   return (pc - cpszSrch);
}

int std_strspn(const char* cpszSrch, const char* cpszChars)
{
   const char *pc = x_scanbytes(cpszSrch, cpszChars, MAX_INT32, '\0', FALSE);

   return (pc - cpszSrch);
}

#if 0
int std_wstrncmp(const AECHAR* s1, const AECHAR* s2, int nLen)
{
   if (nLen > 0) {
      int i;

      s1 += nLen;
      s2 += nLen;
      i = -nLen;
      do {
         AECHAR c1 = s1[i];
         AECHAR c2 = s2[i];
         int  diff = c1 - c2;

         if (diff) {
            return diff;
         }

         if ('\0' == c1) {
            break;
         }
      } while (++i);
   }

   return 0;
}

int std_wstrcmp(const AECHAR* s1, const AECHAR* s2)
{
   return std_wstrncmp(s1, s2, MAX_INT32);
}

AECHAR* std_wstrchr(const AECHAR* cpwszText, AECHAR ch)
{
   for (; ; cpwszText++) {
      AECHAR chn = *cpwszText;

      if (chn == ch) {
         return (AECHAR *)cpwszText;
      }
      else if ( chn == (AECHAR)0 ) {
         return 0;
      }
   }
}

AECHAR* std_wstrrchr(const AECHAR* cpwszText, AECHAR ch)
{
   const AECHAR* p = 0;

   do {
      if (*cpwszText == ch) {
         p = cpwszText;
      }
   } while (*cpwszText++ != (AECHAR)0);

   return (AECHAR*)p;
}
#endif

void* std_memset(void* p, int c, int nLen)
{
   if (nLen < 0) {
      return p;
   }
   return memset(p, c, (size_t)nLen);
}

void* std_memmove(void* pTo, const void* cpFrom, int nLen)
{
   if (nLen <= 0) {
      return pTo;
   }
   return memmove(pTo, cpFrom, (size_t)nLen);
}

#define IsAsciiDigit(c)   STD_BETWEEN(c, '0', '9'+1)
#define IsAsciiAlpha(c)   STD_BETWEEN( (c)|32, 'a', 'z'+1)
#define IsAsciiHexDigit(c) (STD_BETWEEN(c, '0', '9'+1) || STD_BETWEEN( (c)|32, 'a', 'f'+1))
#define AsciiToLower(c)   ((c)|32)

uint64 std_scanux( const char *  pchBuf,
                   int           nRadix,
                   const char ** ppchEnd,
                   int *         pnError )
{
   int nError = 0;   // SUCCESS
   const unsigned char *pch = (const unsigned char*)pchBuf;
   uint64 ullVal = 0;
   uint64 ullOverflow = 0;
   int nLastDigMax = 0;
   unsigned char c;

   if (nRadix < 0 || nRadix > 36 || nRadix == 1) {
      nError = STD_BADPARAM;
      goto done;
   }

   // Skip whitespace

   while ( (c = *pch) == ' ' || c == '\t' ) {
      ++pch;
   }

   // Scan sign

   if (c == '-') {
      nError = STD_NEGATIVE;
      ++pch;
   } else if (c == '+') {
      ++pch;
   }

   // Read optional prefix
   if ((0 == nRadix || 16 == nRadix) &&
       '0' == pch[0] && 'x' == AsciiToLower(pch[1]) &&
       IsAsciiHexDigit(pch[2])) {
      pch += 2;
      nRadix = 16;
   }

   if (0 == nRadix && '0' == pch[0]) {
      nRadix = 8;
   }

   if (0 == nRadix) {
      nRadix = 10;
   }

   /*--------------------------------------------------------------------------
      Compute the overflow threshold, which is the value of ullVal that would
      lead to overflow if another digit were added to it.  Normally, overflow
      would occur if ullVal * nRadix + nDigit > ullMaxVal, but that can't be
      done with 64-bit arithmetic.  Instead we transform this to
            ullVal > (ullMaxVal - nDigit) / nRadix
        OR  ullVal > (ullMaxVal / nRadix) - (nDigit / nRadix))
        OR  ullVal > A - Bn
      where A = (ullMaxVal / nRadix) is a constant, and
            Bn = (nDigit / nRadix) depends on the current scanned digit.

      This means that we might overflow whenever ullVal is greater than A and
      we have scanned another digit. However, if ullVal is equal to A, then
      we may or may not overflow based on the digit scanned.

      Checking for overflow this way, we can avoid doing the 64-bit math for
      each digit by using this one-time computed value.
   --------------------------------------------------------------------------*/

   // Use the C preprocessor to compute the overflow values for common
   //  radices. Do (relatively expensive) division in code for the rest.
   if (10 == nRadix) {
      ullOverflow = MAX_UINT64/10;
      nLastDigMax = (int) (MAX_UINT64 - ((MAX_UINT64/10) * 10));
   } else if (16 == nRadix) {
      ullOverflow = MAX_UINT64/16;
      nLastDigMax = (int) (MAX_UINT64 - ((MAX_UINT64/16) * 16));
   } else if (8 == nRadix) {
      ullOverflow = MAX_UINT64/8;
      nLastDigMax = (int) (MAX_UINT64 - ((MAX_UINT64/8) * 8));
   } else {
      ullOverflow = MAX_UINT64/nRadix;
      nLastDigMax = (int) (MAX_UINT64 - (ullOverflow * nRadix));
   }


   // Read digits
   {
      const unsigned char* pchStartOfDigits = pch;

      for (;;) {
         int nDigit;

         c = *pch;

         if (IsAsciiDigit(c)) {
            nDigit = c - '0';
         } else if (IsAsciiAlpha(c)) {
            nDigit = (AsciiToLower(c) - 'a') + 10;
         } else {
            break;
         }

         if (nDigit >= nRadix) {
            break;
         }

         ++pch;

         if ((ullVal > ullOverflow) ||
             ((ullVal == ullOverflow) && (nDigit > nLastDigMax))) {
            ullVal = MAX_UINT64;
            nError = STD_OVERFLOW;
            break;
         }

         ullVal = ullVal * nRadix + nDigit;
      }

      if (pch == pchStartOfDigits) {
         pch = (const unsigned char*)pchBuf;
         nError = STD_NODIGITS;
      }
   }

 done:
   if (pnError) {
      *pnError = nError;
   }

   if (ppchEnd) {
      *ppchEnd = (const char*)pch;
   }

   return ullVal;
}

uint32 std_scanul( const char *  pchBuf,
                   int           nRadix,
                   const char ** ppchEnd,
                   int *         pnError )
{
   uint64 ullVal = 0;
   uint32 ulVal = 0;
   int nError = 0;

   ullVal = std_scanux(pchBuf, nRadix, ppchEnd, &nError);

   ulVal = (unsigned long) ullVal;

   if (ullVal > MAX_UINT32) {
      nError = STD_OVERFLOW;
      ulVal = MAX_UINT32;
   }
   else if (nError == STD_NEGATIVE) {
      ulVal = (uint32) -(int32)ulVal;
      if (ulVal == 0) {
         nError = 0; // SUCCESS
      }
   }

   if (pnError) {
      *pnError = nError;
   }

   return ulVal;
}

uint64 std_scanull( const char *  pchBuf,
                    int           nRadix,
                    const char ** ppchEnd,
                    int *         pnError )
{
   uint64 ullVal = 0;
   int nError = 0;

   ullVal = std_scanux(pchBuf, nRadix, ppchEnd, &nError);

   if (nError == STD_NEGATIVE) {
      ullVal = (uint64) -(int64)ullVal;
      if (ullVal == 0) {
         nError = 0; // SUCCESS
      }
   }

   if (pnError) {
      *pnError = nError;
   }

   return ullVal;
}


