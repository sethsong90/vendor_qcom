#ifndef DS_SOCK_SOCKETFACTORYPRIV_H
#define DS_SOCK_SOCKETFACTORYPRIV_H
/*===========================================================================
  @file DS_Sock_SocketFactoryPriv.h

  This file defines the class which extends SocketFactory class by supporting
  creation of ICMP sockets.

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_SocketFactoryPriv.h#1 $
  $DateTime: 2009/09/28 14:45:31 $ $Author: dmudgal $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "DS_Sock_SocketFactory.h"

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace DS
{
  namespace Sock
  {
    //TODO document class
    class SocketFactoryPriv : public SocketFactory            /*lint !e578 */
    {
      public:
        /**
          @brief Returns a SocketFactoryPriv object.

          Returns a SocketFactoryPriv object. Since this method
          implements Singleton pattern, an object is created only on the first
          invocation, and subsequent invocations return the same object.

          This instance supports ICMP sockets.

          To maintain the lifetime of singleton object, refCnt is maintained.
          It is incremented each time this method is called.

          @param None

          @retval address  Socket factory is created successfully
          @retval 0        Out of memory
        */                                                 /*lint -e{1511} */
        static SocketFactoryPriv * CreateInstance
        (
          void
        );

        /**
          @brief Indicates if this socket factory supports creation of
                 ICMP sockets.

          Socket factory checks if a class supports ICMP socket creation using
          this method. This class doesn't support them but derived classes
          can override this method.

          @param None

          @retval true  If ICMP sockets are supported
          @retval false If ICMP sockets are not supported
        */
        virtual bool IsICMPSupported
        (
          void
        );

        /*-------------------------------------------------------------------
          Defintions of IQI Methods
        -------------------------------------------------------------------*/
        /*-------------------------------------------------------------------
          Since there is no ISocketFactoryPrivileged class, QueryInterface()
          need not be overloaded
        -------------------------------------------------------------------*/

      private:
        /**
          @brief Overloaded operator new.

          Operator new is overloaded so that memory for SocketFactoryPriv is
          allocated from ps_mem instead of from the system heap.

          As the C Runtime Library throws a std::bad_alloc exception if the
          memory allocation fails, and since memory allocation could fail as
          memory is allocated from ps_mem, this method also handles the
          exception.

          @param[in] numBytes  The amount of memory to be allocated

          @retval address  Ptr to object if memory is allocated
          @retval 0        If memory couldn't be allocated
        */                                                 /*lint -e{1511} */
        static void * operator new
        (
          unsigned int numBytes
        ) throw();

        /**
          @brief Overloaded operator delete.

          Operator delete is overloaded so that memory for SocketFactoryPriv
          is de-allocated and is put back in to ps_mem

          @param[in]  bufPtr  The object to be deleted

          @retval None
        */                                                 /*lint -e{1511} */
        static void operator delete
        (
          void *  bufPtr
        );

        /**
          @brief Ptr to singleton instance of this class.

          Set to a non-NULL when CreateInstance(...) is called the first time.
          All subsequent calls to CreateInstance(...) get this instance.

          SocketFactory::sockFactoryPtr can't be used to implement factory
          pattern for SocketFactotyPrivileged class because static variables
          of base classes are derived as well and the same variable is shared
          by both the classes. So once SocketFactory class is instantiated,
          SocketFactoryPriv class will not instantiate itself as
          sockFactoryPtr is non-0.
        */
        static SocketFactoryPriv *  sockFactoryPrivPtr;

    }; /* class SocketFactoryPriv */
  } /* namespace Sock */
} /* namespace DS */

#endif /* DS_SOCK_SOCKETFACTORYPRIV_H */
