#ifndef DS_UTILS_FULLPRIVSET_H 
#define DS_UTILS_FULLPRIVSET_H 

/*==========================================================================*/
/*!
  @file
  ds_Utils_FullPrivSet.h

  @details
  This file provides the ds::Utils::FullPrivSet class. This class implements
  the AEEIPrivSet interface. The following methods are exported.

  FullPrivSet()
    Constructor for the FullPrivSet object.

  ~FullPrivSet()
    Destructor for the FullPrivSet object.

  Set()
    Set the FullPrivSet. A FullPrivSet can only be set only when it is enabled.


                Copyright (c) 2010 Qualcomm Technologies, Inc.
                All Rights Reserved.
                Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/inc/ds_Utils_FullPrivSet.h#1 $
  $DateTime: 2011/06/17 12:02:33 $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-02-15 ts  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "AEEIPrivSet.h"
#include "ds_Utils_CSSupport.h"
#include "AEEStdErr.h"
#include "ds_Errors_Def.h"
/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace ds
{
  namespace Utils
  {

    /*lint -esym(1510, IPrivSet) */
    /*lint -esym(1510, IQI) */
    /*!
      @class
      ds::Utils::FullPrivSet

      @brief
      Implements the IPrivSet interface.

      @details
      The FullPrivSet implements the IPrivSet interface
      and returns AEE_SUCCESS on CheckPrivileges for any given privilege
      and therefore provides full privileges.
    */
    class FullPrivSet : public IPrivSet
    {
      public:
        /**
        @brief Returns a FullPrivSet object.

        Returns a FullPrivSet object. Since this method implements
        Singleton pattern, an object is created only on the first
        invocation, and subsequent invocations return the same object.

        @param None

        @retval address  FullPrivSet is created successfully
        @retval 0        Out of memory
        */
        static FullPrivSet * Instance
        (
          void
        ) 
        throw();

        /* Derived from IPrivSet interface */

        /*!
          @brief
          This method check if privilege exists

          @details
          For any specified privilege returns AEE_SUCCESS to indicate full privilege.
          
          @param      aprivs    - Privilege to check.
          @param      nPrivsLen - Privilege object length.
          @retval     AEE_SUCCESS Indicates privilege exists
          @see        IPrivSet::CheckPrivileges()
        */
        virtual ds::ErrorType CDECL CheckPrivileges
        (
          AEEPRIVID *aprivs,
          int nPrivsLen
        );
      
        /*!
          @brief
          This method returns the privilege

          @details
          Return the privset. This function is not supported and will always 
          return QDS_EOPNOTSUPP.

          @param      aprivs    - Privilege to check.
          @param      nPrivsLen - Privilege object length.
          @param      pnPrivsLenReq - Privilege object length required
          @retval     QDS_EOPNOTSUPP 
          @see        IPrivSet::GetPrivileges()
        */
        virtual ds::ErrorType CDECL GetPrivileges
        (
          AEEPRIVID *aprivs,
          int nPrivsLen,
          int* pnPrivsLenReq
        );

        /*!
          @brief
          This method create a privset subset

          @details
          Return the aprivSubset. This function is not supported and will always 
          return QDS_EOPNOTSUPP.

          @param      aprivSubset - Privilege subset.
          @param      nNumPrivs - Number of Privileges
          @param      ppipiSubset - Privilege Set 
          @retval     QDS_EOPNOTSUPP 
          @see        IPrivSet::CreateSubset()
        */
        virtual ds::ErrorType CDECL CreateSubset
        (
          const AEEPRIVID* aprivSubset,
          int nNumPrivs,
          IPrivSet** ppipiSubset
        );

        /**
          @brief IQI interface Methods
        */
        DSIQI_IMPL_DEFAULTS(IPrivSet)

    private:

        /**
          @brief Ptr to singleton instance of this class.

          Set to a non-NULL when CreateInstance(...) is called the first time.
          All subsequent calls to CreateInstance(...) get this instance.
        */
        static FullPrivSet * fullPrivSetPtr;

        /* Overloaded new operator */
        void* operator new (unsigned int num_bytes) 
        throw();

        /* Overloaded delete operator */
        void operator delete (void* objPtr) 
        throw();

        /*!
          @brief
          Constructor for a FullPrivSetFactory

          @param[in]  pCbackFcn - Registered callback function.
          @param[in]  pUserData - User data with the callback.

          @return     The constructed FullPrivSet object.
        */
        FullPrivSet
        (
        ) 
        throw();

        /*!
          @brief
          Destructor of the FullPrivSet object.

          @param      None.
          @return     None.
        */
        virtual ~FullPrivSet
        (
          void
        )
        throw();


    }; /* class FullPrivSet */

  } /* namespace Utils */
}/* namespace ds */

#endif /* DS_UTILS_FULLPRIVSET_H  */

