// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//=- ScorpionSchedule.td - Scorpion Scheduling Definitions -*- tablegen -*-=//
//
//                     The LLVM Compiler Infrastructure
//
//===----------------------------------------------------------------------===//
//
// This file defines the itinerary class data for the Scorpion processors.
//
//===----------------------------------------------------------------------===//

//
// Scheduling information derived from 80-VB867-5 Rev. D "Scorpion Processor
// Family Programmer’s Reference Manual (PRM) and 80-VE670-2 Rev. E
// "Scorpion Code Generation Guidelines".
//
// Assumptions:
// - In scorpion all functional units are fully pipelined. 
// - Instructions can go down S, X, VS, or VX pipelines.
// - Simple arithmetic instructions (E pipe instructions) can execute 
//   on either S or X pipelines.
// - Vector load/store instructions use both S and VS pipelines.
// - Fetch width: 2 instructions per clock cycle
// - Issue width: (into Instruction Queue) is 2 instructions per clock cycle
// - Dispatch witdth: 4 instructions per clock cycle (2 integer + 2 NEON/VFP)
//   to keep the 3 pipelines (S/Y, X, VS/VX) full.
// - Instruction Queue size is 8 instructions.
// - Modeling the issue width as 2 instructions per clock cycle, 
//   and assuming this is the look-ahead capability of the processor.
//
// Functional units
def SCRPN_Issue0  : FuncUnit; // Issue 0
def SCRPN_Issue1  : FuncUnit; // Issue 1
def SCRPN_Branch  : FuncUnit; // Branch
def SCRPN_ALUY    : FuncUnit; // ALU pipeline Y
def SCRPN_ALUX    : FuncUnit; // ALU / MUL pipeline X
def SCRPN_AGU     : FuncUnit; // Address generation unit for ld / st
def SCRPN_VSUnit   : FuncUnit; // NEON/VFP VS pipeline
def SCRPN_VXUnit   : FuncUnit; // NEON/VFP VX pipeline
def SCRPN_MUX0    : FuncUnit; // Integer + NEON/FPU multiplexer
def SCRPN_LSUnit  : FuncUnit; // L/S Unit
def SCRPN_DRegsVFP: FuncUnit; // FP register set, VFP side
def SCRPN_DRegsN  : FuncUnit; // FP register set, NEON side

// Bypasses
def SCRPN_LdBypass : Bypass;

def ScorpionItineraries : ProcessorItineraries<
  [SCRPN_Issue0, SCRPN_Issue1,
   SCRPN_Branch, SCRPN_ALUX, SCRPN_ALUY, SCRPN_AGU,
   SCRPN_VSUnit,SCRPN_VXUnit, SCRPN_MUX0,
   SCRPN_LSUnit, SCRPN_DRegsVFP, SCRPN_DRegsN],
  [SCRPN_LdBypass], [
  // Two fully-pipelined integer ALU pipelines

  //
  // Move instructions, unconditional
  // Note: This is an E pipe instruction
  InstrItinData<IIC_iMOVi   , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>], [1]>,
  InstrItinData<IIC_iMOVr   , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>], [1, 1]>,
  InstrItinData<IIC_iMOVsi  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>], [1, 1]>,
  InstrItinData<IIC_iMOVsr  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<2, [SCRPN_ALUX, SCRPN_ALUY]>], [2, 1, 1]>,
  InstrItinData<IIC_iMOVix2 , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>], [2]>,
  InstrItinData<IIC_iMOVix2addpc,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                  InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>,
                                  InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>,
                                  InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>], [3]>,
  InstrItinData<IIC_iMOVix2ld,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_AGU], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>], [5]>,
  //
  // MVN instructions
  // Note: This is an E pipe instruction
  InstrItinData<IIC_iMVNi   , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>],
                              [1]>,
  InstrItinData<IIC_iMVNr   , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>],
                              [1, 1], [NoBypass, SCRPN_LdBypass]>,
  InstrItinData<IIC_iMVNsi  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<2, [SCRPN_ALUX, SCRPN_ALUY]>],
                              [2, 1]>,
  InstrItinData<IIC_iMVNsr  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<3, [SCRPN_ALUX, SCRPN_ALUY]>],
                              [3, 1, 1]>,
  //
  // No operand cycles
  // Note (todo): Assuming all ALU classes are E pipe instructions
  InstrItinData<IIC_iALUx   , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>]>,
  //
  // Binary Instructions that produce a result
  // Note (todo): Assuming all ALU classes are E pipe instructions
  InstrItinData<IIC_iALUi , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                             InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>],
                            [1, 1], [NoBypass, SCRPN_LdBypass]>,
  InstrItinData<IIC_iALUr , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                             InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>],
                            [1, 1, 1], [NoBypass, SCRPN_LdBypass, SCRPN_LdBypass]>,
  InstrItinData<IIC_iALUsi, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                             InstrStage<2, [SCRPN_ALUX, SCRPN_ALUY]>],
                            [2, 1, 1], [NoBypass, SCRPN_LdBypass, NoBypass]>,
  InstrItinData<IIC_iALUsir,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                             InstrStage<2, [SCRPN_ALUX, SCRPN_ALUY]>],
                            [2, 1, 1], [NoBypass, NoBypass, SCRPN_LdBypass]>,
  InstrItinData<IIC_iALUsr, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                             InstrStage<3, [SCRPN_ALUX, SCRPN_ALUY]>],
                            [3, 1, 1, 1],
                            [NoBypass, SCRPN_LdBypass, NoBypass, NoBypass]>,
  //
  // Bitwise Instructions that produce a result
  // Note (todo): Assuming all BIT classes are E pipe instructions
  InstrItinData<IIC_iBITi , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                             InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>], [1, 1]>,
  InstrItinData<IIC_iBITr , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                             InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>], [1, 1, 1]>,
  InstrItinData<IIC_iBITsi, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                             InstrStage<2, [SCRPN_ALUX, SCRPN_ALUY]>], [2, 1, 1]>,
  InstrItinData<IIC_iBITsr, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                             InstrStage<3, [SCRPN_ALUX, SCRPN_ALUY]>], [3, 1, 1, 1]>,
  //
  // Unary Instructions that produce a result

  // CLZ, RBIT, etc.
  // Note (todo): Assuming all uNA classes are E pipe instructions
  InstrItinData<IIC_iUNAr , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                             InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>], [1, 1]>,

  // BFC, BFI, UBFX, SBFX
  // Note (todo): Assuming all uNA classes are E pipe instructions
  InstrItinData<IIC_iUNAsi, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                             InstrStage<2, [SCRPN_ALUX, SCRPN_ALUY]>], [2, 1]>,

  //
  // Zero and sign extension instructions
  // Note: EXT classes are E pipe instructions
  InstrItinData<IIC_iEXTr , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                             InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>], [2, 1]>,
  InstrItinData<IIC_iEXTAr, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                             InstrStage<2, [SCRPN_ALUX, SCRPN_ALUY]>], [3, 1, 1]>,
  InstrItinData<IIC_iEXTAsr,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                             InstrStage<3, [SCRPN_ALUX, SCRPN_ALUY]>], [3, 1, 1, 1]>,
  //
  // Compare instructions
  // Note: CMP classes are E pipe instructions
  InstrItinData<IIC_iCMPi   , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>],
                               [1], [SCRPN_LdBypass]>,
  InstrItinData<IIC_iCMPr   , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>],
                               [1, 1], [SCRPN_LdBypass, SCRPN_LdBypass]>,
  InstrItinData<IIC_iCMPsi  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<2, [SCRPN_ALUX, SCRPN_ALUY]>],
                                [1, 1], [SCRPN_LdBypass, NoBypass]>,
  InstrItinData<IIC_iCMPsr  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<3, [SCRPN_ALUX, SCRPN_ALUY]>],
                              [1, 1, 1], [SCRPN_LdBypass, NoBypass, NoBypass]>,
  //
  // Test instructions
  // Note: TST classes are E pipe instructions
  InstrItinData<IIC_iTSTi   , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>], [1]>,
  InstrItinData<IIC_iTSTr   , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>], [1, 1]>,
  InstrItinData<IIC_iTSTsi  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<2, [SCRPN_ALUX, SCRPN_ALUY]>], [1, 1]>,
  InstrItinData<IIC_iTSTsr  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<3, [SCRPN_ALUX, SCRPN_ALUY]>], [1, 1, 1]>,
  //
  // Move instructions, conditional
  // FIXME: Correctly model the extra input dep on the destination.
  // Note (todo): Assuming condional MOV classes are E pipe instructions
  InstrItinData<IIC_iCMOVi  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>], [1]>,
  InstrItinData<IIC_iCMOVr  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>], [1, 1]>,
  InstrItinData<IIC_iCMOVsi , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>], [1, 1]>,
  InstrItinData<IIC_iCMOVsr , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<2, [SCRPN_ALUX, SCRPN_ALUY]>], [2, 1, 1]>,
  InstrItinData<IIC_iCMOVix2, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>,
                               InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>], [2]>,

  // Integer multiply pipeline
  //
  // Note: MUL classes are X pipe instructions
  InstrItinData<IIC_iMUL16  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<2, [SCRPN_ALUX]>], [3, 1, 1]>,
  InstrItinData<IIC_iMAC16  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<2, [SCRPN_ALUX]>],
                              [3, 1, 1, 1]>,
  InstrItinData<IIC_iMUL32  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<2, [SCRPN_ALUX]>], [4, 1, 1]>,
  InstrItinData<IIC_iMAC32  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<2, [SCRPN_ALUX]>],
                              [4, 1, 1, 1]>,
  InstrItinData<IIC_iMUL64  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<3, [SCRPN_ALUX]>], [4, 5, 1, 1]>,
  InstrItinData<IIC_iMAC64  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<3, [SCRPN_ALUX]>],
                              [4, 5, 1, 1]>,

  // Integer parallel add and sub IIC_iALUpr for now modeled as NoItinerary

  // todo: IIC_iDML32: MAC dual for now defined as IIC_iMAC32
  // SMLAD,DX; SMLSD,DX;
  InstrItinData<IIC_iDML32  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                              InstrStage<2, [SCRPN_ALUX]>],
                              [4, 1, 1, 1]>,

  // todo: IIC_iDML64: MAC long dual for now defined as IIC_iMAC64
  // SMLALD,LDX; SMLSLD,LDX
  InstrItinData<IIC_iDML64  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                              InstrStage<3, [SCRPN_ALUX]>],
                              [4, 5, 1, 1]>,

  // todo: IIC_iMUAD32: Dual multiply add for now defined as IIC_iMAC32
  // SMUAD,DX, SMUSD,DX
  InstrItinData<IIC_iMUAD32  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<2, [SCRPN_ALUX]>],
                               [4, 1, 1]>,

  // todo: IIC_iDIV32: integer division for now defined as IIC_iALUr
  // min latency=3 min throughput=2; max latency=36 throughput=34
  // SDIV, UDIV
  InstrItinData<IIC_iDIV32 , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                             InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>],
                             [1, 1, 1]>,

  // Integer load pipeline
  // FIXME: The timings are some rough approximations
  //
  // Immediate offset
  InstrItinData<IIC_iLoad_i   , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<1, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit]>],
                                [3, 1], [SCRPN_LdBypass]>,
  InstrItinData<IIC_iLoad_bh_i, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<2, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit]>],
                                [4, 1], [SCRPN_LdBypass]>,
  // FIXME: If address is 64-bit aligned, AGU cycles is 1.
  InstrItinData<IIC_iLoad_d_i , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<2, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit]>],
                                [3, 3, 1], [SCRPN_LdBypass]>,
  //
  // Register offset
  InstrItinData<IIC_iLoad_r   , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<1, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit]>],
                                [3, 1, 1], [SCRPN_LdBypass]>,
  InstrItinData<IIC_iLoad_bh_r, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<2, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit]>],
                                [4, 1, 1], [SCRPN_LdBypass]>,
  InstrItinData<IIC_iLoad_d_r , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<2, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit]>],
                                [3, 3, 1, 1], [SCRPN_LdBypass]>,
  //
  // Scaled register offset
  InstrItinData<IIC_iLoad_si  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<1, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit], 0>],
                                [4, 1, 1], [SCRPN_LdBypass]>,
  InstrItinData<IIC_iLoad_bh_si,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<2, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit]>],
                                [5, 1, 1], [SCRPN_LdBypass]>,
  //
  // Immediate offset with update
  // Note (todo): Load with base update use ALUY but not modeling
  // this detail since the hardware supports single issue from SRSV
  InstrItinData<IIC_iLoad_iu  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<1, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit]>],
                                [3, 2, 1], [SCRPN_LdBypass]>,
  InstrItinData<IIC_iLoad_bh_iu,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<2, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit]>],
                                [4, 3, 1], [SCRPN_LdBypass]>,
  //
  // Register offset with update
  InstrItinData<IIC_iLoad_ru  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<1, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit]>],
                                [3, 2, 1, 1], [SCRPN_LdBypass]>,
  InstrItinData<IIC_iLoad_bh_ru,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<2, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit]>],
                                [4, 3, 1, 1], [SCRPN_LdBypass]>,
  InstrItinData<IIC_iLoad_d_ru, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<2, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit]>],
                                [3, 3, 1, 1], [SCRPN_LdBypass]>,
  //
  // Scaled register offset with update
  InstrItinData<IIC_iLoad_siu , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<1, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit]>],
                                [4, 3, 1, 1], [SCRPN_LdBypass]>,
  InstrItinData<IIC_iLoad_bh_siu,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                  InstrStage<1, [SCRPN_MUX0], 0>,
                                  InstrStage<2, [SCRPN_AGU], 0>,
                                  InstrStage<1, [SCRPN_LSUnit]>],
                                 [5, 4, 1, 1], [SCRPN_LdBypass]>,
  //
  // Load multiple, def is the 5th operand.
  // FIXME: This assumes 3 to 4 registers.
  InstrItinData<IIC_iLoad_m  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                InstrStage<1, [SCRPN_MUX0], 0>,
                                InstrStage<2, [SCRPN_AGU], 1>,
                                InstrStage<2, [SCRPN_LSUnit]>],
                               [1, 1, 1, 1, 3],
                         [NoBypass, NoBypass, NoBypass, NoBypass, SCRPN_LdBypass]>,
  //
  // Load multiple + update, defs are the 1st and 5th operands.
  InstrItinData<IIC_iLoad_mu , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                InstrStage<1, [SCRPN_MUX0], 0>,
                                InstrStage<2, [SCRPN_AGU], 1>,
                                InstrStage<2, [SCRPN_LSUnit]>],
                               [2, 1, 1, 1, 3],
                         [NoBypass, NoBypass, NoBypass, NoBypass, SCRPN_LdBypass]>,
  //
  // Load multiple plus branch
  InstrItinData<IIC_iLoad_mBr, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                InstrStage<1, [SCRPN_MUX0], 0>,
                                InstrStage<1, [SCRPN_AGU], 1>,
                                InstrStage<2, [SCRPN_LSUnit]>,
                                InstrStage<1, [SCRPN_Branch]>],
                               [1, 2, 1, 1, 3],
                         [NoBypass, NoBypass, NoBypass, NoBypass, SCRPN_LdBypass]>,
  //
  // Pop, def is the 3rd operand.
  InstrItinData<IIC_iPop  ,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                InstrStage<1, [SCRPN_MUX0], 0>,
                                InstrStage<2, [SCRPN_AGU], 1>,
                                InstrStage<2, [SCRPN_LSUnit]>],
                               [1, 1, 3],
                               [NoBypass, NoBypass, SCRPN_LdBypass]>,
  //
  // Pop + branch, def is the 3rd operand.
  InstrItinData<IIC_iPop_Br,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                InstrStage<1, [SCRPN_MUX0], 0>,
                                InstrStage<2, [SCRPN_AGU], 1>,
                                InstrStage<2, [SCRPN_LSUnit]>,
                                InstrStage<1, [SCRPN_Branch]>],
                               [1, 1, 3],
                               [NoBypass, NoBypass, SCRPN_LdBypass]>,

  //
  // iLoadi + iALUr for t2LDRpci_pic.
  InstrItinData<IIC_iLoadiALU, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                InstrStage<1, [SCRPN_MUX0], 0>,
                                InstrStage<1, [SCRPN_AGU], 0>,
                                InstrStage<1, [SCRPN_LSUnit]>,
                                InstrStage<1, [SCRPN_ALUX, SCRPN_ALUY]>],
                               [2, 1]>,

  // Integer store pipeline
  ///
  // Immediate offset
  InstrItinData<IIC_iStore_i  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<1, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit]>], [1, 1]>,
  InstrItinData<IIC_iStore_bh_i,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<2, [SCRPN_AGU], 1>,
                                 InstrStage<1, [SCRPN_LSUnit]>], [1, 1]>,
  // FIXME: If address is 64-bit aligned, AGU cycles is 1.
  InstrItinData<IIC_iStore_d_i, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<2, [SCRPN_AGU], 1>,
                                 InstrStage<1, [SCRPN_LSUnit]>], [1, 1]>,
  //
  // Register offset
  InstrItinData<IIC_iStore_r  , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<1, [SCRPN_AGU], 0>,
                                 InstrStage<1, [SCRPN_LSUnit]>], [1, 1, 1]>,
  InstrItinData<IIC_iStore_bh_r,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<2, [SCRPN_AGU], 1>,
                                 InstrStage<1, [SCRPN_LSUnit]>], [1, 1, 1]>,
  InstrItinData<IIC_iStore_d_r, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                 InstrStage<1, [SCRPN_MUX0], 0>,
                                 InstrStage<2, [SCRPN_AGU], 1>,
                                 InstrStage<1, [SCRPN_LSUnit]>], [1, 1, 1]>,
  //
  // Scaled register offset
  InstrItinData<IIC_iStore_si ,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                  InstrStage<1, [SCRPN_MUX0], 0>,
                                  InstrStage<1, [SCRPN_AGU], 0>,
                                  InstrStage<1, [SCRPN_LSUnit]>], [1, 1, 1]>,
  InstrItinData<IIC_iStore_bh_si,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                  InstrStage<1, [SCRPN_MUX0], 0>,
                                  InstrStage<2, [SCRPN_AGU], 1>,
                                  InstrStage<1, [SCRPN_LSUnit]>], [1, 1, 1]>,
  //
  // Immediate offset with update
  // Note (todo): Store with base update use ALUY but not modeling
  // this detail since the hardware supports single issue from SRSV
  InstrItinData<IIC_iStore_iu ,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                  InstrStage<1, [SCRPN_MUX0], 0>,
                                  InstrStage<1, [SCRPN_AGU], 0>,
                                  InstrStage<1, [SCRPN_LSUnit]>], [2, 1, 1]>,
  InstrItinData<IIC_iStore_bh_iu,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                  InstrStage<1, [SCRPN_MUX0], 0>,
                                  InstrStage<2, [SCRPN_AGU], 1>,
                                  InstrStage<1, [SCRPN_LSUnit]>], [3, 1, 1]>,
  //
  // Register offset with update
  InstrItinData<IIC_iStore_ru ,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                  InstrStage<1, [SCRPN_MUX0], 0>,
                                  InstrStage<1, [SCRPN_AGU], 0>,
                                  InstrStage<1, [SCRPN_LSUnit]>],
                                 [2, 1, 1, 1]>,
  InstrItinData<IIC_iStore_bh_ru,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                  InstrStage<1, [SCRPN_MUX0], 0>,
                                  InstrStage<2, [SCRPN_AGU], 1>,
                                  InstrStage<1, [SCRPN_LSUnit]>],
                                 [3, 1, 1, 1]>,
  InstrItinData<IIC_iStore_d_ru, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                  InstrStage<1, [SCRPN_MUX0], 0>,
                                  InstrStage<2, [SCRPN_AGU], 1>,
                                  InstrStage<1, [SCRPN_LSUnit]>],
                                 [3, 1, 1, 1]>,
  //
  // Scaled register offset with update
  InstrItinData<IIC_iStore_siu,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                    InstrStage<1, [SCRPN_MUX0], 0>,
                                    InstrStage<1, [SCRPN_AGU], 0>,
                                    InstrStage<1, [SCRPN_LSUnit]>],
                                   [2, 1, 1, 1]>,
  InstrItinData<IIC_iStore_bh_siu, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                    InstrStage<1, [SCRPN_MUX0], 0>,
                                    InstrStage<2, [SCRPN_AGU], 1>,
                                    InstrStage<1, [SCRPN_LSUnit]>],
                                   [3, 1, 1, 1]>,
  //
  // Store multiple
  InstrItinData<IIC_iStore_m , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                InstrStage<1, [SCRPN_MUX0], 0>,
                                InstrStage<1, [SCRPN_AGU], 0>,
                                InstrStage<2, [SCRPN_LSUnit]>]>,
  //
  // Store multiple + update
  InstrItinData<IIC_iStore_mu, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                InstrStage<1, [SCRPN_MUX0], 0>,
                                InstrStage<1, [SCRPN_AGU], 0>,
                                InstrStage<2, [SCRPN_LSUnit]>], [2]>,

  //
  // Preload
  InstrItinData<IIC_Preload,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1]>], [1, 1]>,

  // Branch
  //
  // no delay slots, so the latency of a branch is unimportant
  InstrItinData<IIC_Br       , [InstrStage<1, [SCRPN_Issue0], 0>,
                                InstrStage<1, [SCRPN_Issue1], 0>,
                                InstrStage<1, [SCRPN_Branch]>]>,

  // VFP and NEON shares the same register file. This means that every VFP
  // instruction should wait for full completion of the consecutive NEON
  // instruction and vice-versa. We model this behavior with two artificial FUs:
  // DRegsVFP and DRegsVFP.
  //
  // Every VFP instruction:
  //  - Acquires DRegsVFP resource for 1 cycle
  //  - Reserves DRegsN resource for the whole duration (including time to
  //    register file writeback!).
  // Every NEON instruction does the same but with FUs swapped.
  //
  // Since the reserved FU cannot be acquired, this models precisely
  // "cross-domain" stalls.

  // VFP
  // Issue through integer pipeline, and execute in NEON unit.
  // Note: fpSTAT is a VS Pipe instruction
  // FP Special Register to Integer Register File Move
  InstrItinData<IIC_fpSTAT , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                              InstrStage<1, [SCRPN_MUX0], 0>,
                              InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                              InstrStage<2, [SCRPN_DRegsN],   0, Reserved>,
                              InstrStage<1, [SCRPN_VSUnit]>],
                             [1]>,
  //
  // Single-precision FP Unary
  // Note (todo): Assuming fpUNA class is a VX Pipe instruction
  // Note (todo): Scorpion wbck cycles not known
  InstrItinData<IIC_fpUNA32 , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               // Extra latency cycles since wbck is 2 cycles
                               InstrStage<3, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [2, 1]>,
  //
  // Double-precision FP Unary
  // Note (todo): Assuming dpUNA class is a VX Pipe instruction
  // Note (todo): Scorpion wbck cycles not known
  InstrItinData<IIC_fpUNA64 , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               // Extra latency cycles since wbck is 2 cycles
                               InstrStage<3, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [2, 1]>,

  //
  // Single-precision FP Compare
  // Note (todo): Assuming fpCMP class is a VX Pipe instruction
  // Note (todo): Scorpion wbck cycles not known
  InstrItinData<IIC_fpCMP32 , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               // Extra latency cycles since wbck is 4 cycles
                               InstrStage<5, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [1, 1]>,
  //
  // Double-precision FP Compare
  // Note (todo): Assuming fpCMP class is a VX Pipe instruction
  // Note (todo): Scorpion wbck cycles not known
  InstrItinData<IIC_fpCMP64 , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               // Extra latency cycles since wbck is 4 cycles
                               InstrStage<5, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [1, 1]>,
  //
  // Single to Double FP Convert
  // Note (todo): Assuming fpCVT class is a VX Pipe instruction
  InstrItinData<IIC_fpCVTSD , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<5, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [4, 1]>,
  //
  // Double to Single FP Convert
  // Note (todo): Assuming fpCVT class is a VX Pipe instruction
  InstrItinData<IIC_fpCVTDS , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<5, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [4, 1]>,

  //
  // Single to Half FP Convert
  // Note (todo): Assuming fpCVTSH class is a VX Pipe instruction
  InstrItinData<IIC_fpCVTSH , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<5, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [4, 1]>,
  //
  // Half to Single FP Convert
  // Note (todo): Assuming fpCVTHS class is a VX Pipe instruction
  InstrItinData<IIC_fpCVTHS , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<3, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [2, 1]>,

  //
  // Single-Precision FP to Integer Convert
  // Note (todo): Assuming fpCVTSI class is a VX Pipe instruction
  InstrItinData<IIC_fpCVTSI , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<8, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [7, 1]>,
  //
  // Double-Precision FP to Integer Convert
  // Note (todo): Assuming fpCVTDI class is a VX Pipe instruction
  InstrItinData<IIC_fpCVTDI , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<8, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [7, 1]>,
  //
  // Integer to Single-Precision FP Convert
  // Note (todo): Assuming fpCVTIS class is a VX Pipe instruction
  InstrItinData<IIC_fpCVTIS , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<11, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [10, 1]>,
  //
  // Integer to Double-Precision FP Convert
  // Note (todo): Assuming fpCVTID class is a VX Pipe instruction
  InstrItinData<IIC_fpCVTID , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<11, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [10, 1]>,
  //
  // Single-precision FP ALU
  // Note (todo): Assuming fpALU class is a VX Pipe instruction
  InstrItinData<IIC_fpALU32 , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<8, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [7, 1, 1]>,
  //
  // Double-precision FP ALU
  // Note:  Assuming fpALU class is a VX Pipe instruction
  InstrItinData<IIC_fpALU64 , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<8, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [7, 1, 1]>,
  //
  // Single-precision FP Multiply
  // Note: fpALU class is a VX Pipe instruction
  // Note (todo): Assuming execute 32-bit MUL each cycle
  InstrItinData<IIC_fpMUL32 , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<8, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [7, 1, 1]>,
  //
  // Double-precision FP Multiply
  // Note: fpMUL class is a VX Pipe instruction
  // Note (todo): Assuming execute 64-bit MUL each 2 cycles
  InstrItinData<IIC_fpMUL64 , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<9, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [8, 1, 1]>,
  //
  // Single-precision FP MAC
  // Note (todo): Assuming execute 32-bit MAC each cycle
  InstrItinData<IIC_fpMAC32 , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<11, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [10, 1, 1, 1]>,
  //
  // Double-precision FP MAC
  // Note: fpMAC class is a VX Pipe instruction
  // Note (todo): Assuming execute 64-bit MAC each 2 cycles
  InstrItinData<IIC_fpMAC64 , [InstrStage<1,  [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1,  [SCRPN_MUX0], 0>,
                               InstrStage<1,  [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<12, [SCRPN_DRegsN],  0, Reserved>,
                               InstrStage<2,  [SCRPN_VXUnit]>],
                              [11, 1, 1, 1]>,
  //
  // Single-precision FP DIV
  // Note: fpDIV class is a VX Pipe instruction
  // Note (todo): Assuming execute 32-bit DIV  each 10 cycles
  InstrItinData<IIC_fpDIV32 , [InstrStage<1,  [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1,  [SCRPN_MUX0], 0>,
                               InstrStage<1,  [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<28, [SCRPN_DRegsN],  0, Reserved>,
                               InstrStage<10, [SCRPN_VXUnit]>],
                              [27, 1, 1]>,
  //
  // Double-precision FP DIV
  // Note: fpDIV class is a VX Pipe instruction
  // Note (todo): Assuming execute 64-bit DIV  each 20 cycles
  InstrItinData<IIC_fpDIV64 , [InstrStage<1,  [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1,  [SCRPN_MUX0], 0>,
                               InstrStage<1,  [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<58, [SCRPN_DRegsN],  0, Reserved>,
                               InstrStage<20, [SCRPN_VXUnit]>],
                              [57, 1, 1]>,
  //
  // Single-precision FP SQRT
  // Note: fpSQRT class is a VX Pipe instruction
  // Note (todo): Assuming execute 32-bit SQRT  each 13 cycles
  InstrItinData<IIC_fpSQRT32, [InstrStage<1,  [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1,  [SCRPN_MUX0], 0>,
                               InstrStage<1,  [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<28, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<13, [SCRPN_VXUnit]>],
                              [27, 1]>,
  //
  // Double-precision FP SQRT
  // Note: fpSQRT class is a VX Pipe instruction
  // Note (todo): Assuming execute 32-bit SQRT  each 28 cycles
  InstrItinData<IIC_fpSQRT64, [InstrStage<1,  [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1,  [SCRPN_MUX0], 0>,
                               InstrStage<1,  [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<58, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<28, [SCRPN_VXUnit]>],
                              [57, 1]>,

  //
  // Integer to Single-precision Move
  // Note: fpMOV class is a VS Pipe instruction
  // Note (todo): Scorpion wbck latency not known
  InstrItinData<IIC_fpMOVIS,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               // Extra 1 latency cycle since wbck is 2 cycles
                               InstrStage<3, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit]>],
                              [2, 1]>,
  //
  // Integer to Double-precision Move
  // Note: fpMOVID class is a VS Pipe instruction
  // Note (todo): Scorpion wbck latency not known
  InstrItinData<IIC_fpMOVID,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               // Extra 1 latency cycle since wbck is 2 cycles
                               InstrStage<3, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit]>],
                              [2, 1, 1]>,
  //
  // Single-precision to Integer Move
  //
  // Note (todo): On A9/Scorpion move-from-VFP is free to issue with no stall if other VFP
  // operations are in flight. I assume it still can't dual-issue though.
  // Note: fpMOVSI class is a VS Pipe instruction
  InstrItinData<IIC_fpMOVSI,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>],
                              [2, 1]>,
  //
  // Double-precision to Integer Move
  //
  // Note (todo): On A9/Scorpion move-from-VFP is free to issue with no stall if other VFP
  // operations are in flight. I assume it still can't dual-issue though.
  // Note: fpMOVDI class is a VS Pipe instruction
  InstrItinData<IIC_fpMOVDI,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>],
                              [2, 1, 1]>,
  //
  // Single-precision FP Load
  // Note (todo): fpLoad class also uses S pipe (uses LSUnit/ALUY)
  InstrItinData<IIC_fpLoad32, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<2, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [1, 1]>,
  //
  // Double-precision FP Load
  // FIXME: Result latency is 1 if address is 64-bit aligned.
  InstrItinData<IIC_fpLoad64, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<2, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [2, 1]>,
  //
  // FP Load Multiple
  // FIXME: assumes 2 doubles which requires 2 LS cycles.
  InstrItinData<IIC_fpLoad_m, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<2, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>], [1, 1, 1, 1]>,
  //
  // FP Load Multiple + update
  // FIXME: assumes 2 doubles which requires 2 LS cycles.
  InstrItinData<IIC_fpLoad_mu,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<2, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>], [2, 1, 1, 1]>,
  //
  // Single-precision FP Store
  InstrItinData<IIC_fpStore32,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<2, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [1, 1]>,
  //
  // Double-precision FP Store
  InstrItinData<IIC_fpStore64,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<2, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [1, 1]>,
  //
  // FP Store Multiple
  // FIXME: assumes 2 doubles which requires 2 LS cycles.
  InstrItinData<IIC_fpStore_m,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                               InstrStage<2, [SCRPN_DRegsN],   0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>], [1, 1, 1, 1]>,
  //
  // FP Store Multiple + update
  // FIXME: assumes 2 doubles which requires 2 LS cycles.
  InstrItinData<IIC_fpStore_mu,[InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                                InstrStage<1, [SCRPN_MUX0], 0>,
                                InstrStage<1, [SCRPN_DRegsVFP], 0, Required>,
                                InstrStage<2, [SCRPN_DRegsN],   0, Reserved>,
                                InstrStage<1, [SCRPN_VSUnit], 0>,
                                InstrStage<2, [SCRPN_LSUnit]>], [2, 1, 1, 1]>,
  // NEON
  // VLD1
  InstrItinData<IIC_VLD1,     [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [1, 1]>,
  // VLD1x2
  InstrItinData<IIC_VLD1x2,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [1, 1, 1]>,
  // VLD1x3
  InstrItinData<IIC_VLD1x3,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [1, 1, 2, 1]>,
  // VLD1x4
  InstrItinData<IIC_VLD1x4,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [1, 1, 2, 2, 1]>,
  // VLD1u
  InstrItinData<IIC_VLD1u,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [1, 2, 1]>,
  // VLD1x2u
  InstrItinData<IIC_VLD1x2u,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [1, 1, 2, 1]>,
  // VLD1x3u
  InstrItinData<IIC_VLD1x3u,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [1, 1, 2, 2, 1]>,
  // VLD1x4u
  InstrItinData<IIC_VLD1x4u,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [1, 1, 2, 2, 2, 1]>,
  //
  // VLD1ln
  InstrItinData<IIC_VLD1ln,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [3, 1, 1, 1]>,
  //
  // VLD1lnu
  InstrItinData<IIC_VLD1lnu,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [3, 2, 1, 1, 1, 1]>,
  //
  // VLD1dup
  InstrItinData<IIC_VLD1dup,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [2, 1]>,
  //
  // VLD1dupu
  InstrItinData<IIC_VLD1dupu, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [2, 2, 1, 1]>,
  //
  // VLD2
  InstrItinData<IIC_VLD2,     [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [2, 2, 1]>,
  //
  // VLD2x2
  InstrItinData<IIC_VLD2x2,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [2, 3, 2, 3, 1]>,
  //
  // VLD2ln
  InstrItinData<IIC_VLD2ln,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [3, 3, 1, 1, 1, 1]>,
  //
  // VLD2u
  InstrItinData<IIC_VLD2u,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [2, 2, 2, 1, 1, 1]>,
  //
  // VLD2x2u
  InstrItinData<IIC_VLD2x2u,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [2, 3, 2, 3, 2, 1]>,
  //
  // VLD2lnu
  InstrItinData<IIC_VLD2lnu,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [3, 3, 2, 1, 1, 1, 1, 1]>,
  //
  // VLD2dup
  InstrItinData<IIC_VLD2dup,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [2, 2, 1]>,
  //
  // VLD2dupu
  InstrItinData<IIC_VLD2dupu, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [2, 2, 2, 1, 1]>,
  //
  // VLD3
  InstrItinData<IIC_VLD3,     [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<9,[SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<3, [SCRPN_VSUnit], 0>,
                               InstrStage<3, [SCRPN_LSUnit]>],
                              [3, 3, 4, 1]>,
  //
  // VLD3ln
  InstrItinData<IIC_VLD3ln,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<11,[SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<5, [SCRPN_VSUnit], 0>,
                               InstrStage<5, [SCRPN_LSUnit]>],
                              [5, 5, 6, 1, 1, 1, 1, 2]>,
  //
  // VLD3u
  InstrItinData<IIC_VLD3u,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<9,[SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<3, [SCRPN_VSUnit], 0>,
                               InstrStage<3, [SCRPN_LSUnit]>],
                              [3, 3, 4, 2, 1]>,
  //
  // VLD3lnu
  InstrItinData<IIC_VLD3lnu,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<11,[SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<5, [SCRPN_VSUnit], 0>,
                               InstrStage<5, [SCRPN_LSUnit]>],
                              [5, 5, 6, 2, 1, 1, 1, 1, 1, 2]>,
  //
  // VLD3dup
  InstrItinData<IIC_VLD3dup,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<9, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<3, [SCRPN_VSUnit], 0>,
                               InstrStage<3, [SCRPN_LSUnit]>],
                              [3, 3, 4, 1]>,
  //
  // VLD3dupu
  InstrItinData<IIC_VLD3dupu, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<9, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<3, [SCRPN_VSUnit], 0>,
                               InstrStage<3, [SCRPN_LSUnit]>],
                              [3, 3, 4, 2, 1, 1]>,
  //
  // VLD4
  InstrItinData<IIC_VLD4,     [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<9,[SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<3, [SCRPN_VSUnit], 0>,
                               InstrStage<3, [SCRPN_LSUnit]>],
                              [3, 3, 4, 4, 1]>,
  //
  // VLD4ln
  InstrItinData<IIC_VLD4ln,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<10,[SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<4, [SCRPN_VSUnit], 0>,
                               InstrStage<4, [SCRPN_LSUnit]>],
                              [4, 4, 5, 5, 1, 1, 1, 1, 2, 2]>,
  //
  // VLD4u
  InstrItinData<IIC_VLD4u,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<9,[SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<3, [SCRPN_VSUnit], 0>,
                               InstrStage<3, [SCRPN_LSUnit]>],
                              [3, 3, 4, 4, 2, 1]>,
  //
  // VLD4lnu
  InstrItinData<IIC_VLD4lnu,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<10,[SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<4, [SCRPN_VSUnit], 0>,
                               InstrStage<4, [SCRPN_LSUnit]>],
                              [4, 4, 5, 5, 2, 1, 1, 1, 1, 1, 2, 2]>,
  //
  // VLD4dup
  InstrItinData<IIC_VLD4dup,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [2, 2, 3, 3, 1]>,
  //
  // VLD4dupu
  InstrItinData<IIC_VLD4dupu, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [2, 2, 3, 3, 2, 1, 1]>,
  //
  // VST1
  // Note: fpStore class is a S (uses LSUnit/ALUY) and S Pipe instruction
  InstrItinData<IIC_VST1,     [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [1, 1, 1]>,
  //
  // VST1x2
  InstrItinData<IIC_VST1x2,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [1, 1, 1, 1]>,
  //
  // VST1x3
  InstrItinData<IIC_VST1x3,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<2, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [1, 1, 1, 1, 2]>,
  //
  // VST1x4
  InstrItinData<IIC_VST1x4,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<2, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [1, 1, 1, 1, 2, 2]>,
  //
  // VST1u
  InstrItinData<IIC_VST1u,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [2, 1, 1, 1, 1]>,
  //
  // VST1x2u
  InstrItinData<IIC_VST1x2u,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [2, 1, 1, 1, 1, 1]>,
  //
  // VST1x3u
  InstrItinData<IIC_VST1x3u,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<2, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [2, 1, 1, 1, 1, 1, 2]>,
  //
  // VST1x4u
  InstrItinData<IIC_VST1x4u,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<2, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [2, 1, 1, 1, 1, 1, 2, 2]>,
  //
  // VST1ln
  InstrItinData<IIC_VST1ln,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [1, 1, 1]>,
  //
  // VST1lnu
  InstrItinData<IIC_VST1lnu,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [2, 1, 1, 1, 1]>,
  //
  // VST2
  InstrItinData<IIC_VST2,     [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [1, 1, 1, 1]>,
  //
  // VST2x2
  InstrItinData<IIC_VST2x2,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<3, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<3, [SCRPN_VSUnit], 0>,
                               InstrStage<3, [SCRPN_LSUnit]>],
                              [1, 1, 1, 1, 2, 2]>,
  //
  // VST2u
  InstrItinData<IIC_VST2u,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [2, 1, 1, 1, 1, 1]>,
  //
  // VST2x2u
  InstrItinData<IIC_VST2x2u,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<3, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<3, [SCRPN_VSUnit], 0>,
                               InstrStage<3, [SCRPN_LSUnit]>],
                              [2, 1, 1, 1, 1, 1, 2, 2]>,
  //
  // VST2ln
  InstrItinData<IIC_VST2ln,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [1, 1, 1, 1]>,
  //
  // VST2lnu
  InstrItinData<IIC_VST2lnu,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit], 0>,
                               InstrStage<1, [SCRPN_LSUnit]>],
                              [2, 1, 1, 1, 1, 1]>,
  //
  // VST3
  InstrItinData<IIC_VST3,     [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<2, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [1, 1, 1, 1, 2]>,
  //
  // VST3u
  InstrItinData<IIC_VST3u,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<2, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [2, 1, 1, 1, 1, 1, 2]>,
  //
  // VST3ln
  InstrItinData<IIC_VST3ln,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<3, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<3, [SCRPN_VSUnit], 0>,
                               InstrStage<3, [SCRPN_LSUnit]>],
                              [1, 1, 1, 1, 2]>,
  //
  // VST3lnu
  InstrItinData<IIC_VST3lnu,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<3, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<3, [SCRPN_VSUnit], 0>,
                               InstrStage<3, [SCRPN_LSUnit]>],
                              [2, 1, 1, 1, 1, 1, 2]>,
  //
  // VST4
  InstrItinData<IIC_VST4,     [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<2, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [1, 1, 1, 1, 2, 2]>,
  //
  // VST4u
  InstrItinData<IIC_VST4u,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<2, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [2, 1, 1, 1, 1, 1, 2, 2]>,
  //
  // VST4ln
  InstrItinData<IIC_VST4ln,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<2, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [1, 1, 1, 1, 2, 2]>,
  //
  // VST4lnu
  InstrItinData<IIC_VST4lnu,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<2, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit], 0>,
                               InstrStage<2, [SCRPN_LSUnit]>],
                              [2, 1, 1, 1, 1, 1, 2, 2]>,

  //
  // Double-register Integer Unary
  // Note (todo): Assuming VUNAiD class is a VX pipe instruction
  InstrItinData<IIC_VUNAiD,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [4, 2]>,
  //
  // Quad-register Integer Unary
  // Note (todo): Assuming VUNAiQ class is a VX pipe instruction
  InstrItinData<IIC_VUNAiQ,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [4, 2]>,
  //
  // Double-register Integer Q-Unary
  // Note (todo): Assuming VQUNAiD class is a VX pipe instruction
  InstrItinData<IIC_VQUNAiD,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [4, 1]>,
  //
  // Quad-register Integer CountQ-Unary
  // Note (todo): Assuming VQUNAiQ class is a VX pipe instruction
  InstrItinData<IIC_VQUNAiQ,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [4, 1]>,
  //
  // Double-register Integer Binary
  // Note (todo): Assuming VBINiD class is a VX pipe instruction
  InstrItinData<IIC_VBINiD,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [3, 2, 2]>,
  //
  // Quad-register Integer Binary
  // Note (todo): Assuming VBINiQ class is a VX pipe instruction
  InstrItinData<IIC_VBINiQ,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [3, 2, 2]>,
  //
  // Double-register Integer Subtract
  // Note: VSUBiD class is a VX pipe instruction
  InstrItinData<IIC_VSUBiD,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [3, 2, 1]>,
  //
  // Quad-register Integer Subtract
  // Note: VSUBiQ class is a VX pipe instruction
  InstrItinData<IIC_VSUBiQ,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [3, 2, 1]>,
  //
  // Double-register Integer Shift
  // Note: VSHLiD class is a VX pipe instruction
  InstrItinData<IIC_VSHLiD,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [3, 1, 1]>,
  //
  // Quad-register Integer Shift
  // Note: VSHLiQ class is a VX pipe instruction
  InstrItinData<IIC_VSHLiQ,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [3, 1, 1]>,
  //
  // Double-register Integer Shift (4 cycle)
  // Note: VSHLi4D class is a VX pipe instruction
  InstrItinData<IIC_VSHLi4D,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [4, 1, 1]>,
  //
  // Quad-register Integer Shift (4 cycle)
  // Note: VSHLi4Q class is a VX pipe instruction
  InstrItinData<IIC_VSHLi4Q,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [4, 1, 1]>,
  //
  // Double-register Integer Binary (4 cycle)
  // Note (todo): Assuming VBINi4D class is a VX pipe instruction
  InstrItinData<IIC_VBINi4D,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [4, 2, 2]>,
  //
  // Quad-register Integer Binary (4 cycle)
  // Note (todo): Assuming VBINi4Q class is a VX pipe instruction
  InstrItinData<IIC_VBINi4Q,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [4, 2, 2]>,
  //
  // Double-register Integer Subtract (4 cycle)
  // Note: VSUBi4D class is a VX pipe instruction
  InstrItinData<IIC_VSUBi4D,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [4, 2, 1]>,
  //
  // Quad-register Integer Subtract (4 cycle)
  // Note: VSUBi4Q class is a VX pipe instruction
  InstrItinData<IIC_VSUBi4Q,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [4, 2, 1]>,

  //
  // Double-register Integer Count
  // Note: VCNTiD class is a VX pipe instruction
  InstrItinData<IIC_VCNTiD,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [3, 2, 2]>,
  //
  // Quad-register Integer Count
  // Result written in N3, but that is relative to the last cycle of multicycle,
  // so we use 4 for those cases
  // Note: VCNTiQ class is a VX pipe instruction
  InstrItinData<IIC_VCNTiQ,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [4, 2, 2]>,
  //
  // Double-register Absolute Difference and Accumulate
  // Note: VABAD class is a VX pipe instruction
  InstrItinData<IIC_VABAD,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [6, 3, 2, 1]>,
  //
  // Quad-register Absolute Difference and Accumulate
  // Note: VABAQ class is a VX pipe instruction
  InstrItinData<IIC_VABAQ,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [6, 3, 2, 1]>,
  //
  // Double-register Integer Pair Add Long
  // Note: VPALiD class is a VX pipe instruction
  InstrItinData<IIC_VPALiD,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [6, 3, 1]>,
  //
  // Quad-register Integer Pair Add Long
  // Note: VPALiQ class is a VX pipe instruction
  InstrItinData<IIC_VPALiQ,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [6, 3, 1]>,

  //
  // Double-register Integer Multiply (.8, .16)
  // Note: VMULi16D class is a VX pipe instruction
  InstrItinData<IIC_VMULi16D, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [6, 2, 2]>,
  //
  // Quad-register Integer Multiply (.8, .16)
  // Note: VMULi16Q class is a VX pipe instruction
  InstrItinData<IIC_VMULi16Q, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [7, 2, 2]>,

  //
  // Double-register Integer Multiply (.32)
  // Note: VMULi32D class is a VX pipe instruction
  InstrItinData<IIC_VMULi32D, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [7, 2, 1]>,
  //
  // Quad-register Integer Multiply (.32)
  // Note: VMULi32Q class is a VX pipe instruction
  InstrItinData<IIC_VMULi32Q, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 9 cycles
                               InstrStage<10, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<4, [SCRPN_VXUnit]>],
                              [9, 2, 1]>,
  //
  // Double-register Integer Multiply-Accumulate (.8, .16)
  // Note: VMACi16D class is a VX pipe instruction
  InstrItinData<IIC_VMACi16D, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [6, 3, 2, 2]>,
  //
  // Double-register Integer Multiply-Accumulate (.32)
  // Note: VMACi32D class is a VX pipe instruction
  InstrItinData<IIC_VMACi32D, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [7, 3, 2, 1]>,
  //
  // Quad-register Integer Multiply-Accumulate (.8, .16)
  // Note: VMACi16Q class is a VX pipe instruction
  InstrItinData<IIC_VMACi16Q, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [7, 3, 2, 2]>,
  //
  // Quad-register Integer Multiply-Accumulate (.32)
  // Note: VMACi32Q class is a VX pipe instruction
  InstrItinData<IIC_VMACi32Q, [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 9 cycles
                               InstrStage<10, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<4, [SCRPN_VXUnit]>],
                              [9, 3, 2, 1]>,

  //
  // Move
  // Note: VMOV class is a VS pipe instruction
  InstrItinData<IIC_VMOV,     [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<1, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit]>],
                              [1,1]>,
  //
  // Move Immediate
  // Note: VMOVImm class is a VS pipe instruction
  InstrItinData<IIC_VMOVImm,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit]>],
                              [3]>,
  //
  // Double-register Permute Move
  // Note: VMOVD class is a VS pipe instruction
  InstrItinData<IIC_VMOVD,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit]>],
                              [2, 1]>,
  //
  // Quad-register Permute Move
  // Note: VMOVQ class is a VS pipe instruction
  InstrItinData<IIC_VMOVQ,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit]>],
                              [2, 1]>,
  //
  // Integer to Single-precision Move
  // Note: VMOVIS class is a VS pipe instruction
  InstrItinData<IIC_VMOVIS ,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<3, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit]>],
                              [1, 1]>,
  //
  // Integer to Double-precision Move
  // Note: VMOVID class is a VS pipe instruction
  InstrItinData<IIC_VMOVID ,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<3, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit]>],
                              [1, 1, 1]>,
  //
  // Single-precision to Integer Move
  // Note: VMOVSI class is a VS pipe instruction
  InstrItinData<IIC_VMOVSI ,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<3, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit]>],
                              [2, 1]>,
  //
  // Double-precision to Integer Move
  // Note: VMOVSI class is a VS pipe instruction
  InstrItinData<IIC_VMOVDI ,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<3, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit]>],
                              [2, 2, 1]>,
  //
  // Integer to Lane Move
  // Note (todo): Assuming VMOVISL class is a VS pipe instruction
  InstrItinData<IIC_VMOVISL , [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               InstrStage<4, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [3, 1, 1]>,

  //
  // Vector narrow move
  // Note (todo): Assuming VMOVN class is a VX pipe instruction
  InstrItinData<IIC_VMOVN,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [3, 1]>,
  //
  // Double-register FP Unary
  // Note (todo): Assuming VUNAD class is a VX pipe instruction
  InstrItinData<IIC_VUNAD,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [5, 2]>,
  //
  // Quad-register FP Unary
  // Result written in N5, but that is relative to the last cycle of multicycle,
  // so we use 6 for those cases
  // Note (todo): Assuming VUNAQ class is a VX pipe instruction
  InstrItinData<IIC_VUNAQ,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [6, 2]>,
  //
  // Double-register FP Binary
  // FIXME: We're using this itin for many instructions and [2, 2] here is too
  // optimistic.
  // Note:  VBIND class is a VX pipe instruction
  InstrItinData<IIC_VBIND,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [5, 2, 2]>,

  //
  // VPADD, etc.
  // Note:  VPBIND class is a VX pipe instruction
  InstrItinData<IIC_VPBIND,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [5, 1, 1]>,
  //
  // Double-register FP VMUL
  // Note:  VFMULD class is a VX pipe instruction
  InstrItinData<IIC_VFMULD,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [5, 2, 1]>,
  //
  // Quad-register FP Binary
  // Result written in N5, but that is relative to the last cycle of multicycle,
  // so we use 6 for those cases
  // FIXME: We're using this itin for many instructions and [2, 2] here is too
  // optimistic.
  // Note:  VBINQ class is a VX pipe instruction
  InstrItinData<IIC_VBINQ,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [6, 2, 2]>,
  //
  // Quad-register FP VMUL
  // Note:  VFMULQ class is a VX pipe instruction
  InstrItinData<IIC_VFMULQ,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [6, 2, 1]>,
  //
  // Double-register FP Multiple-Accumulate
  // Note:  VMACD class is a VX pipe instruction
  InstrItinData<IIC_VMACD,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [6, 3, 2, 1]>,
  //
  // Quad-register FP Multiple-Accumulate
  // Result written in N9, but that is relative to the last cycle of multicycle,
  // so we use 10 for those cases
  // Note:  VMACQ class is a VX pipe instruction
  InstrItinData<IIC_VMACQ,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 9 cycles
                               InstrStage<10, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<4, [SCRPN_VXUnit]>],
                              [8, 4, 2, 1]>,
  //
  // Double-register Reciprical Step
  // Note:  VRECSD class is a VX pipe instruction
  InstrItinData<IIC_VRECSD,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 10 cycles
                               InstrStage<11, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [9, 2, 2]>,
  //
  // Quad-register Reciprical Step
  // Note:  VRECSQ class is a VX pipe instruction
  InstrItinData<IIC_VRECSQ,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 11 cycles
                               InstrStage<12, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [10, 2, 2]>,
  //
  // Double-register Permute
  // Note:  VPERMD class is a VX pipe instruction
  InstrItinData<IIC_VPERMD,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [2, 2, 1, 1]>,
  //
  // Quad-register Permute
  // Result written in N2, but that is relative to the last cycle of multicycle,
  // so we use 3 for those cases
  // Note:  VPERMQ class is a VX pipe instruction
  InstrItinData<IIC_VPERMQ,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [3, 3, 1, 1]>,
  //
  // Quad-register Permute (3 cycle issue)
  // Result written in N2, but that is relative to the last cycle of multicycle,
  // so we use 4 for those cases
  // Note:  VPERMQ3 class is a VX pipe instruction
  InstrItinData<IIC_VPERMQ3,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 8 cycles
                               InstrStage<9, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<3, [SCRPN_VXUnit]>],
                              [4, 4, 1, 1]>,

  //
  // Double-register VEXT
  // Note:  VEXTD class is a VS pipe instruction
  InstrItinData<IIC_VEXTD,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit]>],
                              [2, 1, 1]>,
  //
  // Quad-register VEXT
  // Note:  VEXTQ class is a VS pipe instruction
  InstrItinData<IIC_VEXTQ,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit]>],
                              [3, 1, 2]>,
  //
  // VTB
  // Note:  VTB class is a VS pipe instruction
  InstrItinData<IIC_VTB1,     [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit]>],
                              [3, 2, 1]>,
  InstrItinData<IIC_VTB2,     [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<2, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit]>],
                              [3, 2, 2, 1]>,
  InstrItinData<IIC_VTB3,     [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<2, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 8 cycles
                               InstrStage<9, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<3, [SCRPN_VSUnit]>],
                              [4, 2, 2, 3, 1]>,
  InstrItinData<IIC_VTB4,     [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 8 cycles
                               InstrStage<9, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<3, [SCRPN_VSUnit]>],
                              [4, 2, 2, 3, 3, 1]>,
  //
  // VTBX
  // Note:  VTBX class is a VS pipe instruction
  InstrItinData<IIC_VTBX1,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit]>],
                              [3, 1, 2, 1]>,
  InstrItinData<IIC_VTBX2,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit]>],
                              [3, 1, 2, 2, 1]>,
  InstrItinData<IIC_VTBX3,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 8 cycles
                               InstrStage<9, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<3, [SCRPN_VSUnit]>],
                              [4, 1, 2, 2, 3, 1]>,
  InstrItinData<IIC_VTBX4,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 8 cycles
                               InstrStage<9, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VSUnit]>],
                              [4, 1, 2, 2, 3, 3, 1]>,

  //
  // Double-register Integer Add IIC_VADDiD for now defined as IIC_VBINiD
  // and reusing same latency info
  InstrItinData<IIC_VADDiD,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [3, 2, 2]>,

  //
  // Quad-register Integer Add IIC_VADDiQ for now defined as IIC_VBINiQ
  // and reusing same latency info
  InstrItinData<IIC_VADDiQ,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [3, 2, 2]>,

  //
  // Double-register FP Add (in SP) IIC_VADDD for now defined as IIC_VBIND
  // and reusing same latency info
  InstrItinData<IIC_VADDD,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [5, 2, 2]>,

  //
  // Quad-register FP Add (in SP) IIC_VADDQ for now defined as IIC_VBINQ
  // and reusing same latency info
  InstrItinData<IIC_VADDQ,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [6, 2, 2]>,

  //
  // Double-register Integer Binary IIC_VBINi2D for now defined as IIC_VSUBi4D
  // and reusing same latency info
  InstrItinData<IIC_VBINi2D,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [4, 2, 1]>,

  //
  // Quad-register Integer Binary IIC_VBINi2Q for now defined as IIC_VSUBi4Q
  // and reusing same latency info
  InstrItinData<IIC_VBINi2Q,  [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [4, 2, 1]>,

  //
  // Double-register FP Binary IIC_VBIN2D for now defined as IIC_VBIND
  // and reusing same latency info
  InstrItinData<IIC_VBIN2D,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [5, 2, 2]>,

  //
  // Quad-register FP Binary IIC_VBIN2Q for now defined as IIC_VBINQ
  // and reusing same latency info
  InstrItinData<IIC_VBIN2Q,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [6, 2, 2]>,

  // Integer Pairwise IIC_VPBIN2D for now defined as IIC_VPBIND
  // and reusing same latency info
  InstrItinData<IIC_VPBIN2D,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [5, 1, 1]>,

  //
  // Double-register Integer Count IIC_VCNTi2D for now defined as IIC_VCNTiD
  // and reusing same latency info
  InstrItinData<IIC_VCNTi2D,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [3, 2, 2]>,

  //
  // Quad-register Integer Count IIC_VCNTi2Q for now defined as IIC_VCNTiQ
  // and reusing same latency info
  InstrItinData<IIC_VCNTi2Q,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [4, 2, 2]>,

  //
  // Double-register Integer Shift IIC_VSHLi4D for now defined as IIC_VSHLiD
  // and reusing same latency info
  InstrItinData<IIC_VSHLi4D,   [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [3, 1, 1]>,

  //
  // Double-register FP Unary IIC_VUNA1D for now defined as IIC_VUNAD
  // and reusing same latency info
  InstrItinData<IIC_VUNA1D,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VXUnit]>],
                              [5, 2]>,

  //
  // Quad-register FP Unary IIC_VUNA1Q for now defined as IIC_VUNAQ
  // and reusing same latency info
  InstrItinData<IIC_VUNA1Q,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 7 cycles
                               InstrStage<8, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<2, [SCRPN_VXUnit]>],
                              [6, 2]>,

  //
  // Double-register Reverse IIC_VREVD for now defined as IIC_VMOVD
  InstrItinData<IIC_VREVD,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit]>],
                              [2, 1]>,

  //
  // Quad-register Reverse IIC_VREVQ for now defined as IIC_VMOVQ
  InstrItinData<IIC_VREVQ,    [InstrStage<1, [SCRPN_Issue0, SCRPN_Issue1], 0>,
                               InstrStage<1, [SCRPN_MUX0], 0>,
                               InstrStage<1, [SCRPN_DRegsN],   0, Required>,
                               // Extra latency cycles since wbck is 6 cycles
                               InstrStage<7, [SCRPN_DRegsVFP], 0, Reserved>,
                               InstrStage<1, [SCRPN_VSUnit]>],
                              [2, 1]>

]>;
