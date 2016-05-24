// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//=- llvm/CodeGen/ScheduleMachineState.h - Scheduling Support -*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_SCHEDULE_MACHINE_MODEL_H
#define LLVM_CODEGEN_SCHEDULE_MACHINE_MODEL_H
#include "llvm/MC/MCInstrItineraries.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCInstrDesc.h"

namespace llvm {

typedef std::pair<unsigned, unsigned> UnsignPair;

class ScheduleMachineModel {
    const InstrItineraryData &InstrItins;
    int CurrentState;
    const int *DFAStateInputTable;
    const unsigned *DFAStateEntry;
    // CachedTable is a map from <FromState, Input> to ToState
    std::map<UnsignPair, unsigned> CachedTable;
    void ReadTable(unsigned int state) {
        unsigned ThisState = DFAStateEntry[state];
        unsigned NextStateInTable = DFAStateEntry[state+1];
        // Early exit in case CachedTable has already contains this state's transitions
        if (CachedTable.count(UnsignPair(state, DFAStateInputTable[ThisState]))) {
           return;
        }
        for (unsigned i = ThisState; i < NextStateInTable; i+=2) {
            CachedTable[UnsignPair(state, DFAStateInputTable[i])] = DFAStateInputTable[i+1];
        }
    }
    public:
    ScheduleMachineModel (const InstrItineraryData &I,
                          const int * const & dfasit,
                          const unsigned * const & dfase) :
        InstrItins(I), CurrentState(0), DFAStateInputTable(dfasit), DFAStateEntry(dfase) {}
    void clearResources() {
        CurrentState = 0;
    }

    bool canReserveResources(const llvm::MCInstrDesc* TID) {
        unsigned InsnClass = TID->getSchedClass();
        const llvm::InstrStage* IS = InstrItins.beginStage(InsnClass);
        unsigned FuncUnits = IS->getUnits();
        UnsignPair StateTrans = UnsignPair(CurrentState, FuncUnits);
        ReadTable(CurrentState);
        return (CachedTable.count(StateTrans) != 0);
    }

    bool reserveResources(const llvm::MCInstrDesc* TID) {
        unsigned InsnClass = TID->getSchedClass();
        const llvm::InstrStage* IS = InstrItins.beginStage(InsnClass);
        unsigned FuncUnits = IS->getUnits();
        UnsignPair StateTrans = UnsignPair(CurrentState, FuncUnits);
        ReadTable(CurrentState);
        assert(CachedTable.count(StateTrans) != 0);
        CurrentState = CachedTable[StateTrans];
        return true;
    }

    bool canReserveResources(llvm::MachineInstr* MI) {
        const llvm::MCInstrDesc& TID = MI->getDesc();
        return canReserveResources(&TID);
    }

    bool reserveResources(llvm::MachineInstr* MI) {
       const llvm::MCInstrDesc& TID = MI->getDesc();
       return reserveResources(&TID);
    }
};

}

#endif
