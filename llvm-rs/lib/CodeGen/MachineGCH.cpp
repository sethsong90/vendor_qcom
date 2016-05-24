// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//===----------------------------------------------------------------------===//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===-- MachineGCH.cpp - Machine Global Code Hoisting/Unification Pass ----===//
//
//
//===----------------------------------------------------------------------===//
//
// General idea behind this pass is to move an instruction to
// a parent block when all its children have the same instructions.
// This algorithm also works if the parent block doesn't dominate
// its children by inserting phi nodes into children's blocks.
// This algorithm is designed to reduce code size.
// In non-codesize optimization level, it works with more restrictions.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "machine-gch"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/ScopedHashTable.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/RecyclingAllocator.h"

using namespace llvm;

STATISTIC(NumHoisted, "Number of copies hoisted");

namespace {
  class MachineGCH : public MachineFunctionPass {
    const TargetInstrInfo *TII;
    const TargetRegisterInfo *TRI;
    AliasAnalysis *AA;
    MachineDominatorTree *DT;
    MachineRegisterInfo *MRI;
  public:
    static char ID; // Pass identification
    MachineGCH() : MachineFunctionPass(ID), LookAheadLimit(5) {
      initializeMachineGCHPass(*PassRegistry::getPassRegistry());
    }

    virtual bool runOnMachineFunction(MachineFunction &MF);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesCFG();
      MachineFunctionPass::getAnalysisUsage(AU);
      AU.addRequired<AliasAnalysis>();
      AU.addPreservedID(MachineLoopInfoID);
      AU.addRequired<MachineDominatorTree>();
      AU.addPreserved<MachineDominatorTree>();
    }

    virtual void releaseMemory() {
      VNTMap.clear();
      WorkList.clear();
    }

  private:
    const unsigned LookAheadLimit;
    typedef DenseMap<MachineInstr*, MachineInstr*,
        MachineInstrExpressionTrait> GCHHTType;
    DenseMap<MachineBasicBlock*, GCHHTType*> VNTMap;
    SmallVector<MachineBasicBlock*, 32> WorkList;

    bool hasLivePhysRegDefUses(const MachineInstr *MI,
                               const MachineBasicBlock *MBB,
                               SmallSet<unsigned,8> &PhysRefs,
                               SmallVector<unsigned,2> &PhysDefs) const;
    bool isGCHCandidate(MachineInstr *MI);

    bool isProfitableToGCH(MachineInstr*MI, MachineBasicBlock* ParentMBB);
    void AppendInstr(MachineInstr*MI, MachineBasicBlock* MBB);
    void Hoist(MachineInstr* MI, MachineBasicBlock* ParentMBB);
    bool ProcessInstruction(MachineInstr* MI, MachineBasicBlock* ParentMBB);
    bool ProcessBlock(MachineBasicBlock *MBB);
    bool PerformGCH();
    void CreateBlockVNT(MachineBasicBlock *MBB, GCHHTType* VNT);
    MachineInstr* DuplicateInstr(MachineFunction* F, MachineInstr* MI);
  };
} // end anonymous namespace

char MachineGCH::ID = 0;
char &llvm::MachineGCHID = MachineGCH::ID;
INITIALIZE_PASS_BEGIN(MachineGCH, "machine-gch",
                "Machine Global Code Hoisting/Unification", false, false)
INITIALIZE_PASS_DEPENDENCY(MachineDominatorTree)
INITIALIZE_AG_DEPENDENCY(AliasAnalysis)
INITIALIZE_PASS_END(MachineGCH, "machine-gch",
                "Machine Global Code Hoisting/Unification", false, false)

/// hasLivePhysRegDefUses - Return true if the specified instruction read/write
/// physical registers
bool MachineGCH::hasLivePhysRegDefUses(const MachineInstr *MI,
                                       const MachineBasicBlock *MBB,
                                       SmallSet<unsigned,8> &PhysRefs,
                                       SmallVector<unsigned,2> &PhysDefs) const{
  MachineBasicBlock::const_iterator I = MI; I = llvm::next(I);
  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);
    if (!MO.isReg())
      continue;
    unsigned Reg = MO.getReg();
    if (!Reg)
      continue;
    if (TargetRegisterInfo::isVirtualRegister(Reg))
      continue;
    PhysRefs.insert(Reg);
    if (MO.isDef())
      PhysDefs.push_back(Reg);
    for (const uint16_t *Alias = TRI->getAliasSet(Reg); *Alias; ++Alias)
      PhysRefs.insert(*Alias);
  }

  return !PhysRefs.empty();
}

bool MachineGCH::isGCHCandidate(MachineInstr *MI) {
  if (MI->isLabel() || MI->isPHI() || MI->isImplicitDef() ||
      MI->isKill() || MI->isInlineAsm() || MI->isDebugValue())
    return false;

  // Ignore copies.
  if (MI->isCopyLike())
    return false;

  // Ignore stuff that we obviously can't move.
  if (MI->mayStore() || MI->isCall() || MI->isTerminator() ||
      MI->hasUnmodeledSideEffects())
    return false;

  if (MI->mayLoad()) {
    // Okay, this instruction does a load. As a refinement, we allow the target
    // to decide whether the loaded value is actually a constant. If so, we can
    // actually use it as a load.
    if (!MI->isInvariantLoad(AA))
      // FIXME: we should be able to hoist loads with no other side effects if
      // there are no other instructions which can change memory in this loop.
      // This is a trivial form of alias analysis.
      return false;
  }

  return true;
}

bool MachineGCH::ProcessInstruction(MachineInstr* MI,
                                    MachineBasicBlock* ParentMBB)
{
  for (MachineBasicBlock::succ_iterator SI = ParentMBB->succ_begin(),
           SE = ParentMBB->succ_end(); SI != SE; ++SI) {
    MachineBasicBlock* MBB = *SI;
    GCHHTType* VNT = VNTMap[MBB];

    // check if there is a cycle
    for (MachineBasicBlock::succ_iterator SI2 = MBB->succ_begin(),
           SE2 = MBB->succ_end(); SI2 != SE2; ++SI2) {
       MachineBasicBlock* MBB2 = *SI2;
       if (MBB2 == ParentMBB) return false;
    }

    bool FoundGCH = VNT->count(MI);
    if (!FoundGCH) return false;
  }

  return true;
}

void MachineGCH::AppendInstr(MachineInstr* MI, MachineBasicBlock* MBB)
{
  // We have to insert before the branch instructions
  // Otherwise, the inserted instruction will become
  // unreachable
  MachineBasicBlock::iterator Pos = MBB->getFirstTerminator();
  MBB->insert(Pos, MI);
}

MachineInstr* MachineGCH::DuplicateInstr(MachineFunction* F, MachineInstr* MI)
{
  MachineInstr* NewMI = F->CloneMachineInstr(MI);

  unsigned NumDefs = NewMI->getDesc().getNumDefs();
  for (unsigned i = 0, e = NewMI->getNumOperands();
                            NumDefs && i != e; ++i) {
       MachineOperand &MO = NewMI->getOperand(i);
       if (!MO.isReg() || !MO.isDef())
          continue;
       unsigned OldReg = MO.getReg();
       unsigned NewReg = MRI->createVirtualRegister(MRI->getRegClass(OldReg));
       MO.setReg(NewReg);
       --NumDefs;
  }

  return NewMI;
}

void MachineGCH::Hoist(MachineInstr* MI, MachineBasicBlock* ParentMBB)
{
  SmallVector<std::pair<unsigned, unsigned>, 8> GCHPairs;
  GCHHTType* ParentVNT = VNTMap[ParentMBB];

  // we need create a new MI if this block doesn't dominate any
  MachineFunction* F = ParentMBB->getParent();
  MachineInstr* MainMI = DuplicateInstr(F, MI);

  AppendInstr(MainMI, ParentMBB);
  (*ParentVNT)[MainMI] = MainMI;

  // Process the rest of children
  for (MachineBasicBlock::succ_iterator SI = ParentMBB->succ_begin(),
           SE = ParentMBB->succ_end(); SI != SE; ++SI) {
      MachineBasicBlock* MBB = *SI;
      if (MBB == ParentMBB) continue;

      GCHHTType* VNT = VNTMap[MBB];
      MachineInstr* oldMI = VNT->lookup(MainMI);
      VNT->erase(MainMI);

      bool Dominated = DT->dominates(ParentMBB, MBB);
      if (Dominated) {
        // Use main MI to replace the old MI
        // Create def-use replacement pairs
        unsigned NumDefs = oldMI->getDesc().getNumDefs();
        for (unsigned i = 0, e = oldMI->getNumOperands();
                            NumDefs && i != e; ++i) {
          MachineOperand &MO = oldMI->getOperand(i);
          if (!MO.isReg() || !MO.isDef())
            continue;
          unsigned OldReg = MO.getReg();
          unsigned NewReg = MainMI->getOperand(i).getReg();
          if (OldReg == NewReg)
            continue;

          GCHPairs.push_back(std::make_pair(OldReg, NewReg));
          --NumDefs;
        }
        oldMI->eraseFromParent();
      } else {
        MachineBasicBlock* PredMBB = NULL;
        for (MachineBasicBlock::pred_iterator PI = MBB->pred_begin(),
           PE = MBB->pred_end(); PI != PE; ++PI) {
           PredMBB = *PI;
           if (PredMBB != ParentMBB) break;
        }

        GCHHTType* PredVNT = VNTMap[PredMBB];
        bool FoundGCH = PredVNT->count(MainMI);
        MachineInstr* PredMI;
        if (FoundGCH == false) {
          // create a new one
          PredMI = DuplicateInstr(F, oldMI);
          AppendInstr(PredMI, PredMBB);
          (*PredVNT)[PredMI] = PredMI;
         }else{
          // reuse
          PredMI = (*PredVNT)[MainMI];
        }

        // create phi and def-use replacement pair
        unsigned NumDefs = oldMI->getDesc().getNumDefs();
        for (unsigned i = 0, e = oldMI->getNumOperands();
                            NumDefs && i != e; ++i) {
          MachineOperand &MO = oldMI->getOperand(i);
          if (!MO.isReg() || !MO.isDef())
            continue;
          unsigned OldReg = MO.getReg();
          unsigned NewReg = MainMI->getOperand(i).getReg();
          unsigned PredReg = PredMI->getOperand(i).getReg();

          // Create Phi
          MachineBasicBlock::iterator Loc = MBB->begin();
          unsigned PhiReg =
                          MRI->createVirtualRegister(MRI->getRegClass(OldReg));
          MachineInstr *InsertedPHI = BuildMI(*MBB, Loc, DebugLoc(),
                                           TII->get(TargetOpcode::PHI), PhiReg);
          MachineInstrBuilder MIB(InsertedPHI);
          MIB.addReg(PredReg).addMBB(PredMBB);
          MIB.addReg(NewReg).addMBB(ParentMBB);

          // Create replacement pair
          GCHPairs.push_back(std::make_pair(OldReg, PhiReg));
          --NumDefs;
        }

        // remove the old one
        oldMI->eraseFromParent();
      }

  }

  NumHoisted++;

  // Do Replacement
  for (unsigned i = 0, e = GCHPairs.size(); i != e; ++i) {
     MRI->replaceRegWith(GCHPairs[i].first, GCHPairs[i].second);
     MRI->clearKillFlags(GCHPairs[i].second);
  }
}

bool MachineGCH::isProfitableToGCH(MachineInstr*MI, MachineBasicBlock* ParentMBB)
{
  // profitable, if the parent MBB dominating all this children
  // if not dominating, only consider a child with two pred blocks
  // For code size optimization:
  // profitable, if the whole number instructions after hoisting is reduced
  // For general optimization:
  // Don't do if we need move an instruction to a flow that does not have
  // to execute this instruction before.
  int domCount = 0;
  int removeCount = 0;
  int costCount = 0;
  for (MachineBasicBlock::succ_iterator SI = ParentMBB->succ_begin(),
           SE = ParentMBB->succ_end(); SI != SE; ++SI) {
     MachineBasicBlock* MBB = *SI;
     if (DT->dominates( ParentMBB, MBB )) {
       domCount ++;
     }else{
       // if not dominate, currently,the only case supported is
       // only two pred blocks.
       if (MBB->pred_size() != 2) return false;

       MachineBasicBlock* PredMBB = NULL;
       for (MachineBasicBlock::pred_iterator PI = MBB->pred_begin(),
           PE = MBB->pred_end(); PI != PE; ++PI) {
           PredMBB = *PI;
           if (PredMBB != ParentMBB) break;
       }

       GCHHTType* PredVNT = VNTMap[PredMBB];
       bool FoundGCH = PredVNT->count(MI);
       if (FoundGCH) removeCount ++;
          else if (PredMBB->succ_size() >= 2) costCount ++;
     }
  }

  if (!ParentMBB->getParent()->getFunction()
                                    ->hasFnAttr(Attribute::OptimizeForSize)) {
    if (costCount > 0) return false;
  }

  if (domCount == 0) {
    // no domination, a new instruction is need
    // we have to see if it is benefitical
    if (removeCount == 0) return false;
      return true;
  }else return true;
}

bool MachineGCH::ProcessBlock(MachineBasicBlock *MBB)
{
  bool Changed = false;
  bool Movable;

  MachineBasicBlock *firstMBB = *MBB->succ_begin();

  for (MachineBasicBlock::iterator I = firstMBB->begin(),
       E = firstMBB->end(); I != E;) {
    MachineInstr *MI = &*I;
    ++I;

    // It is movable only if this instruction is in all children of MBB
    Movable = ProcessInstruction(MI, MBB);

    if (!Movable)
      continue;

    if (isProfitableToGCH(MI,MBB)) {
        Changed = true;
        // Hoist this instruction to MBB
        Hoist(MI, MBB);
    } else {
        // remove it from a map and make it not available anymore
        GCHHTType* VNT = VNTMap[firstMBB];
        VNT->erase(MI);
    }
  }

  return Changed;
}

void MachineGCH::CreateBlockVNT(MachineBasicBlock *MBB, GCHHTType* VNT)
{
  // Scan instructions top-down
  for (MachineBasicBlock::iterator I = MBB->begin(), E = MBB->end(); I != E;) {
    MachineInstr *MI = &*I;
    ++I;

    if (!isGCHCandidate(MI))
      continue;

    if (VNT->count(MI))
      continue;

    // Assume it's not safe if the instruction uses physical registers.
    SmallSet<unsigned,8> PhysRefs;
    SmallVector<unsigned, 2> PhysDefs;
    if (hasLivePhysRegDefUses(MI, MBB, PhysRefs, PhysDefs))
      continue;

    // add into VNT, only add the first one, the rest will be removed by CSE
    (*VNT)[MI] = MI;
  }
}

bool MachineGCH::PerformGCH()
{
  bool Changed = false;
  for (unsigned i = 0, e = WorkList.size(); i != e; ++i) {
    MachineBasicBlock* MBB = WorkList[i];
    Changed |= ProcessBlock(MBB);
  }

  return Changed;
}

bool MachineGCH::runOnMachineFunction(MachineFunction &MF)
{
  TII = MF.getTarget().getInstrInfo();
  TRI = MF.getTarget().getRegisterInfo();
  MRI = &MF.getRegInfo();
  AA = &getAnalysis<AliasAnalysis>();
  DT = &getAnalysis<MachineDominatorTree>();

  GCHHTType* VNT;
  // Create Variable Name Table for each block
  for (MachineFunction::iterator I = MF.begin(), E = MF.end(); I != E;) {
    MachineBasicBlock *MBB = I++;
    VNT = new GCHHTType();
    CreateBlockVNT(MBB, VNT);
    VNTMap[MBB] = VNT;
  }

  // Create worklist for blocks with more than on successors
  for (MachineFunction::iterator I = MF.begin(), E = MF.end(); I != E;) {
    MachineBasicBlock *MBB = I++;
    if (MBB->succ_size() > 1) {
       WorkList.push_back(MBB);
    }
  }

  bool Changed;
  bool Modified = false;
  do {
    Changed = PerformGCH();
    if (Changed) Modified = true;
  } while (Changed);

  // Release VNT
  for (MachineFunction::iterator I = MF.begin(), E = MF.end(); I != E;) {
    MachineBasicBlock *MBB = I++;
    GCHHTType* VNT = VNTMap[MBB];
    delete VNT;
  }

  return Modified;
}
