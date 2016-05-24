#ifndef DS_UTILS_CRIT_SECT_H
#define DS_UTILS_CRIT_SECT_H
/*=========================================================================*/
/*!
  @file
  ds_Utils_CritSect.h

  @brief
  This class provides critical section facility to objects in a platform
  independent manner. It implements the ds::Utils::ICritSect interface.

  @see AEEICritSect.h
  @see ds_Utils_ICritSect.h

  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_CritSect.h#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ps_crit_sect.h"
#include "ds_Utils_CSSupport.h"
#include "AEEICritSect.h"
#include "AEECCritSect.h"
#include "ds_Errors_Def.h"
#include "AEEStdErr.h"
#include "ps_mem_ext.h"
#include "ps_mem.h"


/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace ds
{
namespace Utils
{

/*lint -esym(1510, ICritSect) */
/*lint -esym(1510, IQI) */
class CritSect : public ICritSect
{
private:
  ps_crit_sect_type  mutex;

public:
  CritSect() throw();
  virtual ~CritSect() throw();

  DSIQI_IMPL_DEFAULTS(ICritSect)

  virtual void CDECL Enter() throw();
  virtual void CDECL Leave() throw();
  virtual int CDECL TryEnter() throw();

  /*-------------------------------------------------------------------------
    CreateInstance method for CritSects
  -------------------------------------------------------------------------*/
  static int CreateInstance (void* env,
                             AEECLSID clsid,
                             void* privset,
                             void** newObj)
  {
    (void) privset;
    (void) env;

    if (AEECLSID_CCritSect == clsid)
    {
      *newObj = (ICritSect *)(new CritSect());
      if (NULL == *newObj)
      {
        return AEE_ENOMEMORY;
      }
      return AEE_SUCCESS;
    }
    return AEE_ECLASSNOTSUPPORT;
  }

  /*-------------------------------------------------------------------------
    Overloaded new/delete methods.
  -------------------------------------------------------------------------*/
  /* Overloaded new operator */
  void* operator new (unsigned int num_bytes)
  throw()
  {
    (void) num_bytes;
    return ps_mem_get_buf (PS_MEM_DS_UTILS_CRIT_SECT);
  }

  /* Overloaded delete operator */
  void operator delete (void* objPtr)
  throw()
  {
    PS_MEM_FREE(objPtr);
  }


}; /* class CritSect */

}  /* namespace Utils */
}  /* namespace ds */

#endif /* DS_UTILS_CRIT_SECT_H */


