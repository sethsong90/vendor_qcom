#ifndef LINUX_SOCKET_PLATFORM_EVENT_MANAGER_H
#define LINUX_SOCKET_PLATFORM_EVENT_MANAGER_H
/*===========================================================================
  @file LinuxPlatformSocketEventManager.h

  This file declares a method that initializes event manager module to handle
  socket platform events.

  Since PSStack, ps_mem, and DSM modules are implemented in C, the events are
  posted via callbacks. But socket platform is implemented in C++ and
  callbacks make C++ code look non-OOO. So this module acts as a bridge.
  It registers callbacks with PSStack, ps_mem, and DSM modules and translates
  the callbacks in to interface methods resulting in C++ code to not deal with
  callbacks but with cleaner interfaces.

  Copyright (c) 2008,2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header:$
  $DateTime:$

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  05/04/10   ar  Adapted for Linux platform
  05/02/08   hm  Created module.

===========================================================================*/

#include <sys/socket.h>
#include "ds_cmdq.h"
#include "LinuxPlatformSocket.h"



/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/

namespace PS
{
  namespace Sock
  {
    namespace Platform
    {
      /**
        @brief Defines event manager module for socket platform.

        Defines a module to listen to events from Linux network stack.

        @see PS::PS::Sock::PSStack::IEventListener,
             PS::Sock::PSStack::IGlobalEventListener
      */
      namespace EventManager
      {
        enum LinuxSocketOpEnum
        {
          LINUX_SOCKET_REG   = 1,
          LINUX_SOCKET_DEREG = 2
        };

        /**
          @brief Initializes the event manager module.

          Initializes the event manager module for socket platform. 

          @param None

          @retval None
        */
        void Init
        (
          void
        );

        /**
          @brief Operation to perform when reg/dereging socket.
         */
        int LinuxSocketEventManager
        (
          LinuxSocketOpEnum    operation, 
          LinuxSocket         *socket
        );

        /**
          @brief Operation to monitor for the specified event on the given socket.
         */
        int LinuxSocketEventManager_Monitor
        (
          LinuxSocket          *pSocket,
          EventType             event
        );

      } /* namespace EventManager */
    } /* namespace Platform */
  } /* namespace Sock */
} /* namespace PS */

#endif /* LINUX_SOCKET_PLATFORM_EVENT_MANAGER_H */
