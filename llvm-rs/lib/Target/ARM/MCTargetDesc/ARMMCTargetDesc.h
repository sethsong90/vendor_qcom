//===-- ARMMCTargetDesc.h - ARM Target Descriptions -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides ARM specific target descriptions.
//
//===----------------------------------------------------------------------===//

#ifndef ARMMCTARGETDESC_H
#define ARMMCTARGETDESC_H

#include "llvm/Support/DataTypes.h"
#include <string>

namespace llvm {
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectWriter;
class MCSubtargetInfo;
class StringRef;
class Target;
class raw_ostream;

extern Target TheARMTarget, TheThumbTarget;

namespace ARM_MC {
  // Scorpion Integer and NEON/VFP Pipeline
  // X Integer Execute Pipeline
  // X_Y Integere Execute Pipeline
  // S Integer Load/Store Pipeline
  // SX either Integer Execute or Storage Pipelines
  // VS VFP/NEON Storage Pipeline
  // VX VFP/NEON Execute Pipeline

  // Krait Integer and NEON/VFP Pipelines
  // L Integer Load Pipeline
  // S Integer Store Pipeline
  // X_Y_M_B_Z: Integer Execute Pipelines
  // B Integer Execute Pipeline
  // Z Integer Execute Pipeline
  // VL VFP/NEON Load, Permute and MOV Pipeline
  // VS VFP/NEON Store and MOV Pipeline
  // VX VFP/NEON Execute Pipeline

  typedef enum {
    PipeType_Unknown,

    // Scorpion
    PipeType_Scorpion_S,
    PipeType_Scorpion_X,
    PipeType_Scorpion_X_Y,
    PipeType_Scorpion_VS,
    PipeType_Scorpion_VX,

    // Krait
    PipeType_Krait_L,
    PipeType_Krait_S,
    PipeType_Krait_X_Y_M_B_Z,
    PipeType_Krait_VL,
    PipeType_Krait_VS,
    PipeType_Krait_VX
  } PipeType;

  // Returns an instruction pipe affinity given the
  // instruction itinerary units in Scorpion.
  PipeType getScorpionPipeType(unsigned Units);

  // Returns an instruction pipe affinity given the
  // instruction itinerary units in Krait2.
  PipeType getKrait2PipeType(unsigned Units);

  // Returns an instruction pipe affinity given the
  // instruction itinerary units in Krait3.
  PipeType getKrait3PipeType(unsigned Units);

  // Returns the number of different execution pipelines
  // used by the instruction itinerary units in Scorpion.
  unsigned int getScorpionPipeCount(unsigned Units);

  // Returns the number of different execution pipelines
  // used by the instruction itinerary units in Krait2.
  unsigned int getKrait2PipeCount(unsigned Units);

  // Returns the number of different execution pipelines
  // used by the instruction itinerary units in Krait3
  unsigned int getKrait3PipeCount(unsigned Units);

  std::string ParseARMTriple(StringRef TT);

  /// createARMMCSubtargetInfo - Create a ARM MCSubtargetInfo instance.
  /// This is exposed so Asm parser, etc. do not need to go through
  /// TargetRegistry.
  MCSubtargetInfo *createARMMCSubtargetInfo(StringRef TT, StringRef CPU,
                                            StringRef FS);
}

MCCodeEmitter *createARMMCCodeEmitter(const MCInstrInfo &MCII,
                                      const MCSubtargetInfo &STI,
                                      MCContext &Ctx);

MCAsmBackend *createARMAsmBackend(const Target &T, StringRef TT);

/// createARMELFObjectWriter - Construct an ELF Mach-O object writer.
MCObjectWriter *createARMELFObjectWriter(raw_ostream &OS,
                                         uint8_t OSABI);

/// createARMMachObjectWriter - Construct an ARM Mach-O object writer.
MCObjectWriter *createARMMachObjectWriter(raw_ostream &OS,
                                          bool Is64Bit,
                                          uint32_t CPUType,
                                          uint32_t CPUSubtype);

} // End llvm namespace

// Defines symbolic names for ARM registers.  This defines a mapping from
// register name to register number.
//
#define GET_REGINFO_ENUM
#include "ARMGenRegisterInfo.inc"

// Defines symbolic names for the ARM instructions.
//
#define GET_INSTRINFO_ENUM
#include "ARMGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "ARMGenSubtargetInfo.inc"

#endif
