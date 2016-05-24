// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//=- llvm/CodeGen/ScheduleProfitRecognizer.h - Scheduling Support -*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
//===----------------------------------------------------------------------===//
//
// This file implements the ScheduleProfitRecognizer class, which implements
// instruction mixing profitability heuristics for scheduling.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_SCHEDULEPROFITRECOGNIZER_H
#define LLVM_CODEGEN_SCHEDULEPROFITRECOGNIZER_H

namespace llvm {

class SUnit;

/// ProfitRecognizer - Provides information on intruction mixing rules.
class ScheduleProfitRecognizer {

public:
  ScheduleProfitRecognizer() {}
  virtual ~ScheduleProfitRecognizer() {};

  /// reset - This callback is invoked when a new block of
  /// instructions is about to be scheduled. The profit state should be
  /// set to an initialized state.
  virtual void reset() {}

  /// addInstruction - This callback is invoked when an instruction is
  /// emitted to be scheduled, to record it as the last scheduled instruction.
  virtual void addInstruction(SUnit *) {}

  virtual bool isEfficientInstrMix(const SUnit *SU){
    // Default implementation - no information on whether
    // SU mixes well with previously issued instructions, so return false.
    return false;
  }

  virtual unsigned int getPipeCount(const SUnit *SU){
    // Default implementation - no information on whether
    // SU can be executed in more than one pipeline.
    return 1;
  }
};

}

#endif
