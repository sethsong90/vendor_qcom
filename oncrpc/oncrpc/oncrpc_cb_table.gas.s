@*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
@
@               O N C R P C   C A L L B A C K   S U P P O R T
@
@ GENERAL DESCRIPTION
@   This module provides the glue callback routines which are used for
@   remote callbacks.
@
@ INITIALIZATION AND SEQUENCING REQUIREMENTS
@   None.
@
@ Copyright (c) 2004-2006 Qualcomm Technologies, Inc.  All Rights Reserved.  
@ Qualcomm Technologies Proprietary and Confidential.
@
@*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
@============================================================================
@
@                        EDIT HISTORY FOR MODULE
@
@  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_cb_table.gas.s#3 $ 
@
@when        who    what, where, why
@--------    ---    ---------------------------------------------------------
@12/08/06    ptm    Increase the callback table size from 100 to 200.
@03/16/05    clp    Added header to source file.
@============================================================================
@============================================================================
@
@                             MODULE INCLUDES
@
@============================================================================

@============================================================================
@
@                             MODULE IMPORTS
@
@============================================================================
        .extern rpc_svc_callback_lookup

@============================================================================
@
@                             MODULE EXPORTS
@
@============================================================================
        .global oncrpc_cb_bridge_table

@============================================================================
@
@                             MODULE TEXT
@
@============================================================================
          
        .macro push regs
        stmfd   r13!, \regs
        .endm

        .macro pop regs
        ldmfd   r13!, \regs
        .endm


        .macro def_bridge idx, label
        .section .text
        .code 32
        .balign 8

        @ The next three lines preserve the stacks 8-byte alignment
\label:  sub     r13, r13, #4    @ allocate space for callback addr
        push    {r0-r3}         @ save arguments
        push    {r14}           @ save return addr on stack
        mov     r0, #\idx       @ argument to rpc_svc_callback_lookup is
                                @ index of this routine                
        bl      rpc_svc_callback_lookup
        str     r0, [r13,#5*4]  @  write callback addr in space allocated above
        pop     {r0}            @ restore return address
        mov     r14,r0
        pop     {r0-r3}         @ restore arguments
        pop     {r15}           @ jump to callback handler

        .section .data
        .word     \label

        .endm

@============================================================================
@
@                             Callback Table area
@
@============================================================================
        @ This code must come before the definitions below so that the
        @ label "oncrpc_cb_bridge_table" is associated with first entry
        @ of the table. Each call to the macro "def_bridge" defined below
        @ creates an entry in the table.

        .section .data
        .balign 8
oncrpc_cb_bridge_table:
	
        @ The number of macro calls here must match the number of entries
        @ in the table 

                      def_bridge 0, oncrpc_cb_bridge0 
                      def_bridge 1, oncrpc_cb_bridge1 
                      def_bridge 2, oncrpc_cb_bridge2 
                      def_bridge 3, oncrpc_cb_bridge3 
                      def_bridge 4, oncrpc_cb_bridge4 
                      def_bridge 5, oncrpc_cb_bridge5 
                      def_bridge 6, oncrpc_cb_bridge6 
                      def_bridge 7, oncrpc_cb_bridge7 
                      def_bridge 8, oncrpc_cb_bridge8 
                      def_bridge 9, oncrpc_cb_bridge9 
	
                      def_bridge 10, oncrpc_cb_bridge10 
                      def_bridge 11, oncrpc_cb_bridge11 
                      def_bridge 12, oncrpc_cb_bridge12 
                      def_bridge 13, oncrpc_cb_bridge13 
                      def_bridge 14, oncrpc_cb_bridge14 
                      def_bridge 15, oncrpc_cb_bridge15 
                      def_bridge 16, oncrpc_cb_bridge16 
                      def_bridge 17, oncrpc_cb_bridge17 
                      def_bridge 18, oncrpc_cb_bridge18 
                      def_bridge 19, oncrpc_cb_bridge19 
	
                      def_bridge 20, oncrpc_cb_bridge20 
                      def_bridge 21, oncrpc_cb_bridge21 
                      def_bridge 22, oncrpc_cb_bridge22 
                      def_bridge 23, oncrpc_cb_bridge23 
                      def_bridge 24, oncrpc_cb_bridge24 
                      def_bridge 25, oncrpc_cb_bridge25 
                      def_bridge 26, oncrpc_cb_bridge26 
                      def_bridge 27, oncrpc_cb_bridge27 
                      def_bridge 28, oncrpc_cb_bridge28 
                      def_bridge 29, oncrpc_cb_bridge29 
	
                      def_bridge 30, oncrpc_cb_bridge30 
                      def_bridge 31, oncrpc_cb_bridge31 
                      def_bridge 32, oncrpc_cb_bridge32 
                      def_bridge 33, oncrpc_cb_bridge33 
                      def_bridge 34, oncrpc_cb_bridge34 
                      def_bridge 35, oncrpc_cb_bridge35 
                      def_bridge 36, oncrpc_cb_bridge36 
                      def_bridge 37, oncrpc_cb_bridge37 
                      def_bridge 38, oncrpc_cb_bridge38 
                      def_bridge 39, oncrpc_cb_bridge39 
	
                      def_bridge 40, oncrpc_cb_bridge40 
                      def_bridge 41, oncrpc_cb_bridge41 
                      def_bridge 42, oncrpc_cb_bridge42 
                      def_bridge 43, oncrpc_cb_bridge43 
                      def_bridge 44, oncrpc_cb_bridge44 
                      def_bridge 45, oncrpc_cb_bridge45 
                      def_bridge 46, oncrpc_cb_bridge46 
                      def_bridge 47, oncrpc_cb_bridge47 
                      def_bridge 48, oncrpc_cb_bridge48 
                      def_bridge 49, oncrpc_cb_bridge49 
	
                      def_bridge 50, oncrpc_cb_bridge50 
                      def_bridge 51, oncrpc_cb_bridge51 
                      def_bridge 52, oncrpc_cb_bridge52 
                      def_bridge 53, oncrpc_cb_bridge53 
                      def_bridge 54, oncrpc_cb_bridge54 
                      def_bridge 55, oncrpc_cb_bridge55 
                      def_bridge 56, oncrpc_cb_bridge56 
                      def_bridge 57, oncrpc_cb_bridge57 
                      def_bridge 58, oncrpc_cb_bridge58 
                      def_bridge 59, oncrpc_cb_bridge59 
	
                      def_bridge 60, oncrpc_cb_bridge60 
                      def_bridge 61, oncrpc_cb_bridge61 
                      def_bridge 62, oncrpc_cb_bridge62 
                      def_bridge 63, oncrpc_cb_bridge63 
                      def_bridge 64, oncrpc_cb_bridge64 
                      def_bridge 65, oncrpc_cb_bridge65 
                      def_bridge 66, oncrpc_cb_bridge66 
                      def_bridge 67, oncrpc_cb_bridge67 
                      def_bridge 68, oncrpc_cb_bridge68 
                      def_bridge 69, oncrpc_cb_bridge69 
	
                      def_bridge 70, oncrpc_cb_bridge70 
                      def_bridge 71, oncrpc_cb_bridge71 
                      def_bridge 72, oncrpc_cb_bridge72 
                      def_bridge 73, oncrpc_cb_bridge73 
                      def_bridge 74, oncrpc_cb_bridge74 
                      def_bridge 75, oncrpc_cb_bridge75 
                      def_bridge 76, oncrpc_cb_bridge76 
                      def_bridge 77, oncrpc_cb_bridge77 
                      def_bridge 78, oncrpc_cb_bridge78 
                      def_bridge 79, oncrpc_cb_bridge79 
	
                      def_bridge 80, oncrpc_cb_bridge80 
                      def_bridge 81, oncrpc_cb_bridge81 
                      def_bridge 82, oncrpc_cb_bridge82 
                      def_bridge 83, oncrpc_cb_bridge83 
                      def_bridge 84, oncrpc_cb_bridge84 
                      def_bridge 85, oncrpc_cb_bridge85 
                      def_bridge 86, oncrpc_cb_bridge86 
                      def_bridge 87, oncrpc_cb_bridge87 
                      def_bridge 88, oncrpc_cb_bridge88 
                      def_bridge 89, oncrpc_cb_bridge89 
	
                      def_bridge 90, oncrpc_cb_bridge90 
                      def_bridge 91, oncrpc_cb_bridge91 
                      def_bridge 92, oncrpc_cb_bridge92 
                      def_bridge 93, oncrpc_cb_bridge93 
                      def_bridge 94, oncrpc_cb_bridge94 
                      def_bridge 95, oncrpc_cb_bridge95 
                      def_bridge 96, oncrpc_cb_bridge96 
                      def_bridge 97, oncrpc_cb_bridge97 
                      def_bridge 98, oncrpc_cb_bridge98 
                      def_bridge 99, oncrpc_cb_bridge99 
	
                      def_bridge 100, oncrpc_cb_bridge100 
                      def_bridge 101, oncrpc_cb_bridge101 
                      def_bridge 102, oncrpc_cb_bridge102 
                      def_bridge 103, oncrpc_cb_bridge103 
                      def_bridge 104, oncrpc_cb_bridge104 
                      def_bridge 105, oncrpc_cb_bridge105 
                      def_bridge 106, oncrpc_cb_bridge106 
                      def_bridge 107, oncrpc_cb_bridge107 
                      def_bridge 108, oncrpc_cb_bridge108 
                      def_bridge 109, oncrpc_cb_bridge109 
	
                      def_bridge 110, oncrpc_cb_bridge110 
                      def_bridge 111, oncrpc_cb_bridge111 
                      def_bridge 112, oncrpc_cb_bridge112 
                      def_bridge 113, oncrpc_cb_bridge113 
                      def_bridge 114, oncrpc_cb_bridge114 
                      def_bridge 115, oncrpc_cb_bridge115 
                      def_bridge 116, oncrpc_cb_bridge116 
                      def_bridge 117, oncrpc_cb_bridge117 
                      def_bridge 118, oncrpc_cb_bridge118 
                      def_bridge 119, oncrpc_cb_bridge119 
	
                      def_bridge 120, oncrpc_cb_bridge120 
                      def_bridge 121, oncrpc_cb_bridge121 
                      def_bridge 122, oncrpc_cb_bridge122 
                      def_bridge 123, oncrpc_cb_bridge123 
                      def_bridge 124, oncrpc_cb_bridge124 
                      def_bridge 125, oncrpc_cb_bridge125 
                      def_bridge 126, oncrpc_cb_bridge126 
                      def_bridge 127, oncrpc_cb_bridge127 
                      def_bridge 128, oncrpc_cb_bridge128 
                      def_bridge 129, oncrpc_cb_bridge129 
	
                      def_bridge 130, oncrpc_cb_bridge130 
                      def_bridge 131, oncrpc_cb_bridge131 
                      def_bridge 132, oncrpc_cb_bridge132 
                      def_bridge 133, oncrpc_cb_bridge133 
                      def_bridge 134, oncrpc_cb_bridge134 
                      def_bridge 135, oncrpc_cb_bridge135 
                      def_bridge 136, oncrpc_cb_bridge136 
                      def_bridge 137, oncrpc_cb_bridge137 
                      def_bridge 138, oncrpc_cb_bridge138 
                      def_bridge 139, oncrpc_cb_bridge139 
	
                      def_bridge 140, oncrpc_cb_bridge140 
                      def_bridge 141, oncrpc_cb_bridge141 
                      def_bridge 142, oncrpc_cb_bridge142 
                      def_bridge 143, oncrpc_cb_bridge143 
                      def_bridge 144, oncrpc_cb_bridge144 
                      def_bridge 145, oncrpc_cb_bridge145 
                      def_bridge 146, oncrpc_cb_bridge146 
                      def_bridge 147, oncrpc_cb_bridge147 
                      def_bridge 148, oncrpc_cb_bridge148 
                      def_bridge 149, oncrpc_cb_bridge149 
	
                      def_bridge 150, oncrpc_cb_bridge150 
                      def_bridge 151, oncrpc_cb_bridge151 
                      def_bridge 152, oncrpc_cb_bridge152 
                      def_bridge 153, oncrpc_cb_bridge153 
                      def_bridge 154, oncrpc_cb_bridge154 
                      def_bridge 155, oncrpc_cb_bridge155 
                      def_bridge 156, oncrpc_cb_bridge156 
                      def_bridge 157, oncrpc_cb_bridge157 
                      def_bridge 158, oncrpc_cb_bridge158 
                      def_bridge 159, oncrpc_cb_bridge159 
	
                      def_bridge 160, oncrpc_cb_bridge160 
                      def_bridge 161, oncrpc_cb_bridge161 
                      def_bridge 162, oncrpc_cb_bridge162 
                      def_bridge 163, oncrpc_cb_bridge163 
                      def_bridge 164, oncrpc_cb_bridge164 
                      def_bridge 165, oncrpc_cb_bridge165 
                      def_bridge 166, oncrpc_cb_bridge166 
                      def_bridge 167, oncrpc_cb_bridge167 
                      def_bridge 168, oncrpc_cb_bridge168 
                      def_bridge 169, oncrpc_cb_bridge169 
	
                      def_bridge 170, oncrpc_cb_bridge170 
                      def_bridge 171, oncrpc_cb_bridge171 
                      def_bridge 172, oncrpc_cb_bridge172 
                      def_bridge 173, oncrpc_cb_bridge173 
                      def_bridge 174, oncrpc_cb_bridge174 
                      def_bridge 175, oncrpc_cb_bridge175 
                      def_bridge 176, oncrpc_cb_bridge176 
                      def_bridge 177, oncrpc_cb_bridge177 
                      def_bridge 178, oncrpc_cb_bridge178 
                      def_bridge 179, oncrpc_cb_bridge179 
	
                      def_bridge 180, oncrpc_cb_bridge180 
                      def_bridge 181, oncrpc_cb_bridge181 
                      def_bridge 182, oncrpc_cb_bridge182 
                      def_bridge 183, oncrpc_cb_bridge183 
                      def_bridge 184, oncrpc_cb_bridge184 
                      def_bridge 185, oncrpc_cb_bridge185 
                      def_bridge 186, oncrpc_cb_bridge186 
                      def_bridge 187, oncrpc_cb_bridge187 
                      def_bridge 188, oncrpc_cb_bridge188 
                      def_bridge 189, oncrpc_cb_bridge189 
	
                      def_bridge 190, oncrpc_cb_bridge190 
                      def_bridge 191, oncrpc_cb_bridge191 
                      def_bridge 192, oncrpc_cb_bridge192 
                      def_bridge 193, oncrpc_cb_bridge193 
                      def_bridge 194, oncrpc_cb_bridge194 
                      def_bridge 195, oncrpc_cb_bridge195 
                      def_bridge 196, oncrpc_cb_bridge196 
                      def_bridge 197, oncrpc_cb_bridge197 
                      def_bridge 198, oncrpc_cb_bridge198 
                      def_bridge 199, oncrpc_cb_bridge199 
	
        .end
