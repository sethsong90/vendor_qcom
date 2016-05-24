// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//===-- ARMProfitRecognizer.h - ARM Hazard Recognizers ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
//===----------------------------------------------------------------------===//
//
// This file defines instruction mix profit recognizers for
// scheduling ARM functions.
//
//===----------------------------------------------------------------------===//

#ifndef ARMPROFITRECOGNIZER_H
#define ARMPROFITRECOGNIZER_H

#include "llvm/CodeGen/ScheduleProfitRecognizer.h"

namespace llvm {

class InstrItineraryData;
class ARMBaseInstrInfo;
class ARMSubtarget;
class MachineInstr;

class ARMProfitRecognizer : public ScheduleProfitRecognizer {
  const InstrItineraryData *ItinData;
  const ARMSubtarget &STI;

  MachineInstr *LastMIIntPipe;

public:
  ARMProfitRecognizer(const InstrItineraryData *iiData,
                      const ARMBaseInstrInfo &tii,
                      const ARMSubtarget &sti) :
    ItinData(iiData), STI(sti), LastMIIntPipe(0) {}

  /// reset - This callback is invoked when a new block of
  /// instructions is about to be scheduled. The profit state should be
  /// set to an initialized state.
  virtual void reset();

  /// addInstruction - This callback is invoked when an instruction is
  /// emitted to be scheduled, to record it as the last scheduled instruction.
  virtual void addInstruction(SUnit *);

  // isEfficientInstrMix - This callback determines whether SU mixes
  // well with previously issued instructions.
  virtual bool isEfficientInstrMix(const SUnit *SU);

  // getPipeCount - This callback returns the number of different pipelines
  // this SU can be executed on.
  virtual unsigned int getPipeCount(const SUnit *SU);
};
} // end namespace llvm

#endif // ARMPROFITRECOGNIZER_H
