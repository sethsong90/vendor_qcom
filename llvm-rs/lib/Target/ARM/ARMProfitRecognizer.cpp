// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//===-- ARMProfitRecognizer.cpp - ARM postra profit recognizer ------------===//
//
//                     The LLVM Compiler Infrastructure
//===----------------------------------------------------------------------===//

#include "ARMProfitRecognizer.h"
#include "ARMBaseInstrInfo.h"
#include "ARMSubtarget.h"
#include "MCTargetDesc/ARMMCTargetDesc.h"
#include "llvm/CodeGen/ScheduleDAG.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;


static unsigned int getKraitPipeCount(const ARMSubtarget &STI,
  unsigned Units)
{
  if (STI.isKrait2())
    return ARM_MC::getKrait2PipeCount(Units);
  if (STI.isKrait3())
    return ARM_MC::getKrait3PipeCount(Units);

  return 1;
}

static bool useKraitIntPipe(const ARMSubtarget &STI,
  unsigned Units)
{
  ARM_MC::PipeType PipeT;

  if (STI.isKrait2())
    PipeT = ARM_MC::getKrait2PipeType(Units);
  else if (STI.isKrait3())
    PipeT = ARM_MC::getKrait3PipeType(Units);
  else return false;

  switch (PipeT) {
    case ARM_MC::PipeType_Krait_X_Y_M_B_Z:
    case ARM_MC::PipeType_Krait_L:
    case ARM_MC::PipeType_Krait_S:
      return true;
  default:
    return false;
  }
}

// checks whether the instruction itineraries represented by left
// and right units result in an efficient pipeline mix
// according to Scorpion instruction mixing rules.
bool isEfficientKraitPipeMix(const ARMSubtarget &STI,
  unsigned LeftUnits, unsigned RightUnits)
{
  // Enforcing mixing rules for integer pipelines but not
  // for vector pipelines.
  // Krait efficient (balanced) integer pipeline mixing rules:
  // Left       Right
  // L         favor: X_Y_M_Z_B       avoid: L, S
  // S         favor: X_Y_M_Z_B       avoid: L, S
  // X_Y_M_Z_B any pipe is ok (most instructions execute
  // in more than one pipe so should be ok to allow back to back)

  // todo: enforce Krait instruction pairing rules.

  ARM_MC::PipeType LeftPT;
  ARM_MC::PipeType RightPT;

  if (STI.isKrait2()) {
    LeftPT = ARM_MC::getKrait2PipeType(LeftUnits);
    RightPT = ARM_MC::getKrait2PipeType(RightUnits);
  }
  else if (STI.isKrait3()) {
    LeftPT = ARM_MC::getKrait3PipeType(LeftUnits);
    RightPT = ARM_MC::getKrait3PipeType(RightUnits);
  }
  else
    return false;

  // Determine the integer pipeline mixing to avoid
  if (LeftPT == ARM_MC::PipeType_Krait_L &&
    (RightPT == ARM_MC::PipeType_Krait_L ||
    RightPT == ARM_MC::PipeType_Krait_S))
    return false;

  if (LeftPT == ARM_MC::PipeType_Krait_S &&
    (RightPT == ARM_MC::PipeType_Krait_S ||
    RightPT == ARM_MC::PipeType_Krait_L))
    return false;

  // all other pipeline mixing is ok
  return true;
}

static unsigned int getScorpionPipeCount(unsigned Units)
{
  return  ARM_MC::getScorpionPipeCount(Units);
}

// checks whether the instruction itinerary represented by units
// uses any of Scorpion Integer Pipelines
static bool useScorpionIntPipe(unsigned Units)
{
  ARM_MC::PipeType PipeT = ARM_MC::getScorpionPipeType(Units);

  switch (PipeT) {
    case ARM_MC::PipeType_Scorpion_S:
    case ARM_MC::PipeType_Scorpion_X:
    case ARM_MC::PipeType_Scorpion_X_Y:
      return true;
  default:
    return false;
  }
}

// checks whether the instruction itineraries represented by left
// and right units result in an efficient pipeline mix
// according to Scorpion instruction mixing rules.
bool isEfficientScorpionPipeMix(unsigned LeftUnits,
   unsigned RightUnits)
{
  // Enforcing mixing rules for integer pipelines but not
  // for vector pipelines.
  // Scorpion efficient (balanced) integer pipeline mixing rules:
  // Left       Right
  // S          favor: X, X_Y            avoid: S
  // X          favor: S, X_Y,           avoid: X
  // X_Y        any pipe is ok

  ARM_MC::PipeType LeftPT = ARM_MC::getScorpionPipeType(LeftUnits);
  ARM_MC::PipeType RightPT = ARM_MC::getScorpionPipeType(RightUnits);

  // Determine the integer pipeline mixing to avoid
  if (LeftPT == ARM_MC::PipeType_Scorpion_S &&
    RightPT == ARM_MC::PipeType_Scorpion_S)
    return false;

  if (LeftPT == ARM_MC::PipeType_Scorpion_X &&
    RightPT == ARM_MC::PipeType_Scorpion_X)
    return false;

  // all other pipeline mixing is ok
  return true;
}

static unsigned getUnits(const InstrItineraryData *ItinData,
  unsigned ItinClassIndx) {

  // target doesn't provide itinerary information or
  // a dummy (Generic) itinerary which should be handled as if its
  // itinerary is empty
  if (ItinData->isEmpty() ||
    ItinData->Itineraries[ItinClassIndx].FirstStage == 0)
    return 0;

  // Get all FUs used by instruction class
  unsigned Units = 0;
  for (const InstrStage *IS = ItinData->beginStage(ItinClassIndx),
    *E = ItinData->endStage(ItinClassIndx); IS != E; ++IS) {
    Units |= IS->getUnits();
  }

  return Units;
}

void ARMProfitRecognizer::reset() {
  LastMIIntPipe = 0;
}

void ARMProfitRecognizer::addInstruction(SUnit *SU) {
  MachineInstr *MI = SU->getInstr();

  if (MI->isDebugValue())
    return;

  const MCInstrDesc &MCID = MI->getDesc();
  unsigned Idx = MCID.getSchedClass();
  unsigned Units = getUnits(ItinData, Idx);

  if (STI.isScorpion()) {
    // just keep track of emitted integer pipe instructions
    if (useScorpionIntPipe(Units))
      LastMIIntPipe = MI;
  }
  else if (STI.isKrait()) {
    // just keep track of emitted integer pipe instructions
    if (useKraitIntPipe(STI, Units))
      LastMIIntPipe = MI;
  }
}

bool ARMProfitRecognizer::isEfficientInstrMix(const SUnit * SU) {

  // disregard subtarget with no information on instruction mix
  if (!(STI.isScorpion() || STI.isKrait())) {
     return false;
  }

  // no previous instr in the integer pipe to check for mixing rules
  if (!LastMIIntPipe)
    return true;

  MachineInstr *MI = SU->getInstr();
  const MCInstrDesc &LastMCID = LastMIIntPipe->getDesc();
  const MCInstrDesc &CurrMCID = MI->getDesc();
  unsigned LastIdx = LastMCID.getSchedClass();
  unsigned CurrIdx = CurrMCID.getSchedClass();

  bool PipeMix;
  if (STI.isScorpion())
    PipeMix = isEfficientScorpionPipeMix(getUnits(ItinData, LastIdx),
      getUnits(ItinData, CurrIdx));
  else if (STI.isKrait())
    PipeMix = isEfficientKraitPipeMix(STI, getUnits(ItinData, LastIdx),
      getUnits(ItinData, CurrIdx));

   DEBUG(errs() << "LastMIIntPipe=" << *LastMIIntPipe
   << " CurrMI=" << *MI
   << " PipeMix=" << PipeMix <<"\n");

  return PipeMix;
}

unsigned int ARMProfitRecognizer::getPipeCount(const SUnit * SU) {
  // disregard subtarget with no information on pipe count
  if (!(STI.isScorpion() || STI.isKrait())) {
     return 1;
  }

  MachineInstr *MI = SU->getInstr();
  const MCInstrDesc &CurrMCID = MI->getDesc();
  unsigned CurrIdx = CurrMCID.getSchedClass();

  unsigned int PipeCount;
  if (STI.isScorpion())
    PipeCount = getScorpionPipeCount(getUnits(ItinData, CurrIdx));
  else if (STI.isKrait())
    PipeCount = getKraitPipeCount(STI, getUnits(ItinData, CurrIdx));

   DEBUG(errs() << " CurrMI=" << *MI
   << " PipeCount=" << PipeCount <<"\n");

  return PipeCount;
}
