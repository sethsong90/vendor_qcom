#ifndef AEEDBG_H
#define AEEDBG_H
/*
=======================================================================

FILE:         AEEdbg.h

SERVICES:     dbg

DESCRIPTION:  Debug services

=======================================================================
        Copyright 2005 Qualcomm Technologies, Inc.
               All Rights Reserved.
            Qualcomm Technologies Proprietary and Confidential
=======================================================================
*/

//Priority levels for messages
#define DBG_MSG_LEVEL_LOW        0
#define DBG_MSG_LEVEL_MEDIUM     1
#define DBG_MSG_LEVEL_HIGH       2
#define DBG_MSG_LEVEL_ERROR      3
#define DBG_MSG_LEVEL_FATAL      4
#define DBG_MSG_LEVEL_RESERVED   0x7FFF



#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */


int dbg_MessageId(const char *lpszMessage, int nLevel, 
                     const char *lpszFileName, int nLine, int nSubSystemId);

int dbg_Message(const char *lpszMessage, int nLevel, 
                     const char *lpszFileName, int nLine);


int dbg_Event(unsigned int dwEvt, const void* pcvPayload, int nLen);

void dbg_Break(void);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

/*=====================================================================
INTERFACE DOCUMENTATION
=======================================================================
dbg Interface

Description:
  The dbg interface offers statically-linked debugging services for the components.

See Also:
   dbg_Break
   dbg_Event
   dbg_Message
   dbg_MessageId
   DBGEVENT


=======================================================================

dbg_MessageId()

Description:
   The dbg_MessageId() sends a null terminated message to a debug console. 
   This method sends callers subsystem Id, in contrast with the dbg_Message() which does not.

Prototype:

   int dbg_MessageId( const char *lpszMessage, int nLevel, 
                    const char *lpszFileName, int nLine, int nSubSystemId);

Parameters:
   lpszMessage: message buffer
   nLevel: describes the level or importance of the message
   lpszFileName: source file name
   nLine: line number in the file
   nSubSystemId: describes the subsystem id of the caller of the message

Return Value:
   None

Comments:
   None

Side Effects:
   None

See Also:
   dbg_Message

======================================================================= 
 
dbg_Message()

Description:
   Sends a NULL-terminated message to a debug console.

Prototype:
   int dbg_Message( const char *lpszMessage, int nLevel, 
                    const char *lpszFileName, int nLine );

Parameters:
   lpszMessage: The message buffer.
   nLevel: The level or importance of the message.
   lpszFileName: The source file name.
   nLine: The line number in the file.

Return Value:
   Undefined.

Comments:
   None.

Side Effects:
   None.

See Also:
   dbg_MessageId
 
======================================================================= 
 
dbg_Event()

Description:
   Sends an event to the debug console.

Prototype:
   int dbg_Event(unsigned int dwEvt, const void* pcvPayload, int nLen);

Parameters:
   dwEvt: A distinguishing event ID.
   pcvPayload: The payload buffer.
   nLen: The length in bytes of the payload buffer.

Return Value:
   Undefined.

Comments:
   None.

Side Effects:
   None.

See Also:
   dbg
   DBGEVENT

=======================================================================

dbg_Break()

Description:
   Passes control to debugger monitoring the process.

Prototype:
   void dbg_Break( void );
   
Parameters:
   None.

Return Value:
   None.

Comments:
   None.

Side Effects:
   If the debugger is attached to the target, the break point is hit.  If not then
   it just returns and continues.

See Also:
   dbg

=======================================================================*/

#endif /* #ifndef AEEDBG_H */


