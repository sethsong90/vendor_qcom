//===-- CodePlacementOpt.cpp - Code Placement pass. -----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the pass that optimizes code placement and aligns loop
// headers to target-specific alignment boundaries.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "code-placement"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/Target/TargetLowering.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
using namespace llvm;

STATISTIC(NumLoopsAligned,  "Number of loops aligned");
STATISTIC(NumIntraElim,     "Number of intra loop branches eliminated");
STATISTIC(NumIntraMoved,    "Number of intra loop branches moved");

namespace {
  class CodePlacementOpt : public MachineFunctionPass {
    const MachineLoopInfo *MLI;
    const TargetInstrInfo *TII;
    const TargetLowering  *TLI;
    const TargetRegisterInfo *TRI;
    RegScavenger *RS;

  public:
    static char ID;
    CodePlacementOpt() : MachineFunctionPass(ID) {}

    virtual bool runOnMachineFunction(MachineFunction &MF);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<MachineLoopInfo>();
      AU.addPreservedID(MachineDominatorsID);
      MachineFunctionPass::getAnalysisUsage(AU);
    }

  private:
    bool HasFallthrough(MachineBasicBlock *MBB);
    bool HasAnalyzableTerminator(MachineBasicBlock *MBB);
    void Splice(MachineFunction &MF,
                MachineFunction::iterator InsertPt,
                MachineFunction::iterator Begin,
                MachineFunction::iterator End);
    bool EliminateUnconditionalJumpsToTop(MachineFunction &MF,
                                          MachineLoop *L);
    bool MoveDiscontiguousLoopBlocks(MachineFunction &MF,
                                     MachineLoop *L);
    bool OptimizeIntraLoopEdgesInLoopNest(MachineFunction &MF, MachineLoop *L);
    bool OptimizeIntraLoopEdges(MachineFunction &MF);
    bool AlignLoops(MachineFunction &MF);
    bool AlignLoop(MachineFunction &MF, MachineLoop *L, unsigned Align);
    bool IsBranchTarget(MachineBasicBlock &MBB);
    MachineBasicBlock *SplitBBAt(MachineFunction &MF, MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator I);
    bool SplitBB(MachineFunction &MF, MachineBasicBlock &MBB);
    bool AlignBranchTargets(MachineFunction &MF);
  };

  char CodePlacementOpt::ID = 0;
} // end anonymous namespace

char &llvm::CodePlacementOptID = CodePlacementOpt::ID;
INITIALIZE_PASS(CodePlacementOpt, "code-placement",
                "Code Placement Optimizer", false, false)

/// HasFallthrough - Test whether the given branch has a fallthrough, either as
/// a plain fallthrough or as a fallthrough case of a conditional branch.
///
bool CodePlacementOpt::HasFallthrough(MachineBasicBlock *MBB) {
  MachineBasicBlock *TBB = 0, *FBB = 0;
  SmallVector<MachineOperand, 4> Cond;
  if (TII->AnalyzeBranch(*MBB, TBB, FBB, Cond))
    return false;
  // This conditional branch has no fallthrough.
  if (FBB)
    return false;
  // An unconditional branch has no fallthrough.
  if (Cond.empty() && TBB)
    return false;
  // It has a fallthrough.
  return true;
}

/// HasAnalyzableTerminator - Test whether AnalyzeBranch will succeed on MBB.
/// This is called before major changes are begun to test whether it will be
/// possible to complete the changes.
///
/// Target-specific code is hereby encouraged to make AnalyzeBranch succeed
/// whenever possible.
///
bool CodePlacementOpt::HasAnalyzableTerminator(MachineBasicBlock *MBB) {
  // Conservatively ignore EH landing pads.
  if (MBB->isLandingPad()) return false;

  // Aggressively handle return blocks and similar constructs.
  if (MBB->succ_empty()) return true;

  // Ask the target's AnalyzeBranch if it can handle this block.
  MachineBasicBlock *TBB = 0, *FBB = 0;
  SmallVector<MachineOperand, 4> Cond;
  // Make sure the terminator is understood.
  if (TII->AnalyzeBranch(*MBB, TBB, FBB, Cond))
    return false;
   // Ignore blocks which look like they might have EH-related control flow.
   // AnalyzeBranch thinks it knows how to analyze such things, but it doesn't
   // recognize the possibility of a control transfer through an unwind.
   // Such blocks contain EH_LABEL instructions, however they may be in the
   // middle of the block. Instead of searching for them, just check to see
   // if the CFG disagrees with AnalyzeBranch.
  if (1u + !Cond.empty() != MBB->succ_size())
    return false;
  // Make sure we have the option of reversing the condition.
  if (!Cond.empty() && TII->ReverseBranchCondition(Cond))
    return false;
  return true;
}

/// Splice - Move the sequence of instructions [Begin,End) to just before
/// InsertPt. Update branch instructions as needed to account for broken
/// fallthrough edges and to take advantage of newly exposed fallthrough
/// opportunities.
///
void CodePlacementOpt::Splice(MachineFunction &MF,
                              MachineFunction::iterator InsertPt,
                              MachineFunction::iterator Begin,
                              MachineFunction::iterator End) {
  assert(Begin != MF.begin() && End != MF.begin() && InsertPt != MF.begin() &&
         "Splice can't change the entry block!");
  MachineFunction::iterator OldBeginPrior = prior(Begin);
  MachineFunction::iterator OldEndPrior = prior(End);

  MF.splice(InsertPt, Begin, End);

  prior(Begin)->updateTerminator();
  OldBeginPrior->updateTerminator();
  OldEndPrior->updateTerminator();
}

/// EliminateUnconditionalJumpsToTop - Move blocks which unconditionally jump
/// to the loop top to the top of the loop so that they have a fall through.
/// This can introduce a branch on entry to the loop, but it can eliminate a
/// branch within the loop. See the @simple case in
/// test/CodeGen/X86/loop_blocks.ll for an example of this.
bool CodePlacementOpt::EliminateUnconditionalJumpsToTop(MachineFunction &MF,
                                                        MachineLoop *L) {
  bool Changed = false;
  MachineBasicBlock *TopMBB = L->getTopBlock();

  bool BotHasFallthrough = HasFallthrough(L->getBottomBlock());

  if (TopMBB == MF.begin() ||
      HasAnalyzableTerminator(prior(MachineFunction::iterator(TopMBB)))) {
  new_top:
    for (MachineBasicBlock::pred_iterator PI = TopMBB->pred_begin(),
         PE = TopMBB->pred_end(); PI != PE; ++PI) {
      MachineBasicBlock *Pred = *PI;
      if (Pred == TopMBB) continue;
      if (HasFallthrough(Pred)) continue;
      if (!L->contains(Pred)) continue;

      // Verify that we can analyze all the loop entry edges before beginning
      // any changes which will require us to be able to analyze them.
      if (Pred == MF.begin())
        continue;
      if (!HasAnalyzableTerminator(Pred))
        continue;
      if (!HasAnalyzableTerminator(prior(MachineFunction::iterator(Pred))))
        continue;

      // Move the block.
      DEBUG(dbgs() << "CGP: Moving blocks starting at BB#" << Pred->getNumber()
                   << " to top of loop.\n");
      Changed = true;

      // Move it and all the blocks that can reach it via fallthrough edges
      // exclusively, to keep existing fallthrough edges intact.
      MachineFunction::iterator Begin = Pred;
      MachineFunction::iterator End = llvm::next(Begin);
      while (Begin != MF.begin()) {
        MachineFunction::iterator Prior = prior(Begin);
        if (Prior == MF.begin())
          break;
        // Stop when a non-fallthrough edge is found.
        if (!HasFallthrough(Prior))
          break;
        // Stop if a block which could fall-through out of the loop is found.
        if (Prior->isSuccessor(End))
          break;
        // If we've reached the top, stop scanning.
        if (Prior == MachineFunction::iterator(TopMBB)) {
          // We know top currently has a fall through (because we just checked
          // it) which would be lost if we do the transformation, so it isn't
          // worthwhile to do the transformation unless it would expose a new
          // fallthrough edge.
          if (!Prior->isSuccessor(End))
            goto next_pred;
          // Otherwise we can stop scanning and procede to move the blocks.
          break;
        }
        // If we hit a switch or something complicated, don't move anything
        // for this predecessor.
        if (!HasAnalyzableTerminator(prior(MachineFunction::iterator(Prior))))
          break;
        // Ok, the block prior to Begin will be moved along with the rest.
        // Extend the range to include it.
        Begin = Prior;
        ++NumIntraMoved;
      }

      // Move the blocks.
      Splice(MF, TopMBB, Begin, End);

      // Update TopMBB.
      TopMBB = L->getTopBlock();

      // We have a new loop top. Iterate on it. We shouldn't have to do this
      // too many times if BranchFolding has done a reasonable job.
      goto new_top;
    next_pred:;
    }
  }

  // If the loop previously didn't exit with a fall-through and it now does,
  // we eliminated a branch.
  if (Changed &&
      !BotHasFallthrough &&
      HasFallthrough(L->getBottomBlock())) {
    ++NumIntraElim;
  }

  return Changed;
}

/// MoveDiscontiguousLoopBlocks - Move any loop blocks that are not in the
/// portion of the loop contiguous with the header. This usually makes the loop
/// contiguous, provided that AnalyzeBranch can handle all the relevant
/// branching. See the @cfg_islands case in test/CodeGen/X86/loop_blocks.ll
/// for an example of this.
bool CodePlacementOpt::MoveDiscontiguousLoopBlocks(MachineFunction &MF,
                                                   MachineLoop *L) {
  bool Changed = false;
  MachineBasicBlock *TopMBB = L->getTopBlock();
  MachineBasicBlock *BotMBB = L->getBottomBlock();

  // Determine a position to move orphaned loop blocks to. If TopMBB is not
  // entered via fallthrough and BotMBB is exited via fallthrough, prepend them
  // to the top of the loop to avoid losing that fallthrough. Otherwise append
  // them to the bottom, even if it previously had a fallthrough, on the theory
  // that it's worth an extra branch to keep the loop contiguous.
  MachineFunction::iterator InsertPt =
    llvm::next(MachineFunction::iterator(BotMBB));
  bool InsertAtTop = false;
  if (TopMBB != MF.begin() &&
      !HasFallthrough(prior(MachineFunction::iterator(TopMBB))) &&
      HasFallthrough(BotMBB)) {
    InsertPt = TopMBB;
    InsertAtTop = true;
  }

  // Keep a record of which blocks are in the portion of the loop contiguous
  // with the loop header.
  SmallPtrSet<MachineBasicBlock *, 8> ContiguousBlocks;
  for (MachineFunction::iterator I = TopMBB,
       E = llvm::next(MachineFunction::iterator(BotMBB)); I != E; ++I)
    ContiguousBlocks.insert(I);

  // Find non-contigous blocks and fix them.
  if (InsertPt != MF.begin() && HasAnalyzableTerminator(prior(InsertPt)))
    for (MachineLoop::block_iterator BI = L->block_begin(), BE = L->block_end();
         BI != BE; ++BI) {
      MachineBasicBlock *BB = *BI;

      // Verify that we can analyze all the loop entry edges before beginning
      // any changes which will require us to be able to analyze them.
      if (!HasAnalyzableTerminator(BB))
        continue;
      if (!HasAnalyzableTerminator(prior(MachineFunction::iterator(BB))))
        continue;

      // If the layout predecessor is part of the loop, this block will be
      // processed along with it. This keeps them in their relative order.
      if (BB != MF.begin() &&
          L->contains(prior(MachineFunction::iterator(BB))))
        continue;

      // Check to see if this block is already contiguous with the main
      // portion of the loop.
      if (!ContiguousBlocks.insert(BB))
        continue;

      // Move the block.
      DEBUG(dbgs() << "CGP: Moving blocks starting at BB#" << BB->getNumber()
                   << " to be contiguous with loop.\n");
      Changed = true;

      // Process this block and all loop blocks contiguous with it, to keep
      // them in their relative order.
      MachineFunction::iterator Begin = BB;
      MachineFunction::iterator End = llvm::next(MachineFunction::iterator(BB));
      for (; End != MF.end(); ++End) {
        if (!L->contains(End)) break;
        if (!HasAnalyzableTerminator(End)) break;
        ContiguousBlocks.insert(End);
        ++NumIntraMoved;
      }

      // If we're inserting at the bottom of the loop, and the code we're
      // moving originally had fall-through successors, bring the sucessors
      // up with the loop blocks to preserve the fall-through edges.
      if (!InsertAtTop)
        for (; End != MF.end(); ++End) {
          if (L->contains(End)) break;
          if (!HasAnalyzableTerminator(End)) break;
          if (!HasFallthrough(prior(End))) break;
        }

      // Move the blocks. This may invalidate TopMBB and/or BotMBB, but
      // we don't need them anymore at this point.
      Splice(MF, InsertPt, Begin, End);
    }

  return Changed;
}

/// OptimizeIntraLoopEdgesInLoopNest - Reposition loop blocks to minimize
/// intra-loop branching and to form contiguous loops.
///
/// This code takes the approach of making minor changes to the existing
/// layout to fix specific loop-oriented problems. Also, it depends on
/// AnalyzeBranch, which can't understand complex control instructions.
///
bool CodePlacementOpt::OptimizeIntraLoopEdgesInLoopNest(MachineFunction &MF,
                                                        MachineLoop *L) {
  bool Changed = false;

  // Do optimization for nested loops.
  for (MachineLoop::iterator I = L->begin(), E = L->end(); I != E; ++I)
    Changed |= OptimizeIntraLoopEdgesInLoopNest(MF, *I);

  // Do optimization for this loop.
  Changed |= EliminateUnconditionalJumpsToTop(MF, L);
  Changed |= MoveDiscontiguousLoopBlocks(MF, L);

  return Changed;
}

/// OptimizeIntraLoopEdges - Reposition loop blocks to minimize
/// intra-loop branching and to form contiguous loops.
///
bool CodePlacementOpt::OptimizeIntraLoopEdges(MachineFunction &MF) {
  bool Changed = false;

  if (!TLI->shouldOptimizeCodePlacement())
    return Changed;

  // Do optimization for each loop in the function.
  for (MachineLoopInfo::iterator I = MLI->begin(), E = MLI->end();
       I != E; ++I)
    if (!(*I)->getParentLoop())
      Changed |= OptimizeIntraLoopEdgesInLoopNest(MF, *I);

  return Changed;
}

/// AlignLoops - Align loop headers to target preferred alignments.
///
bool CodePlacementOpt::AlignLoops(MachineFunction &MF) {
  const Function *F = MF.getFunction();
  if (F->hasFnAttr(Attribute::OptimizeForSize))
    return false;

  unsigned Align = TLI->getPrefLoopAlignment();
  if (!Align)
    return false;  // Don't care about loop alignment.

  bool Changed = false;

  for (MachineLoopInfo::iterator I = MLI->begin(), E = MLI->end();
       I != E; ++I)
    Changed |= AlignLoop(MF, *I, Align);

  return Changed;
}

/// AlignLoop - Align loop headers to target preferred alignments.
///
bool CodePlacementOpt::AlignLoop(MachineFunction &MF, MachineLoop *L,
                                 unsigned Align) {
  bool Changed = false;

  // Do alignment for nested loops.
  for (MachineLoop::iterator I = L->begin(), E = L->end(); I != E; ++I)
    Changed |= AlignLoop(MF, *I, Align);

  L->getTopBlock()->setAlignment(Align);
  Changed = true;
  ++NumLoopsAligned;

  return Changed;
}

/// IsBranchTarget - Check if the basic block is a branch target.
///
bool CodePlacementOpt::IsBranchTarget(MachineBasicBlock &MBB) {
  if (MBB.hasAddressTaken()) return true;

  unsigned PredNum = MBB.pred_size();
  if (PredNum > 1)  return true;
  if (PredNum == 0) return false;  // Dead?

  // Single predecessor block.  Check for branch at the end.
  MachineBasicBlock *TBB = 0, *FBB = 0;
  SmallVector<MachineOperand, 4> Cond;
  MachineBasicBlock &Pred = **MBB.pred_begin();

  // Check if it's a return point.
  const MachineInstr &LastI = Pred.back();
  const MCInstrDesc &MCID = LastI.getDesc();
  if (MCID.isCall()) return true;

  if (!TII->AnalyzeBranch(Pred, TBB, FBB, Cond)) {
    // If succeeded in analyzing branch, check the target. &MBB is never NULL.
    return TBB == &MBB || FBB == &MBB;
  } else {
    // Failed to analyze branch.  Safe to assume there isn't one.
    return false;
  }
}

/// SplitBBAt - Splits given block at a given position
///
MachineBasicBlock *CodePlacementOpt::SplitBBAt(MachineFunction &MF,
                                               MachineBasicBlock &MBB,
                                               MachineBasicBlock::iterator I) {
  const BasicBlock *IR_Block = MBB.getBasicBlock();
  MachineBasicBlock *NewBlock = MF.CreateMachineBasicBlock(IR_Block);
  MachineFunction::iterator MBBI = &MBB;
  MF.insert(++MBBI, NewBlock);

  NewBlock->transferSuccessors(&MBB);
  MBB.addSuccessor(NewBlock);
  NewBlock->splice(NewBlock->end(), &MBB, I, MBB.end());

  // Update live-in information in the new block.
  if (RS) {
    RS->enterBasicBlock(&MBB);
    if (!MBB.empty()) RS->forward(prior(MBB.end()));
    BitVector RegsLiveAtExit(TRI->getNumRegs());
    RS->getRegsUsed(RegsLiveAtExit, false);
    for (unsigned int i = 0, e = TRI->getNumRegs(); i != e; i++) {
      if (RegsLiveAtExit[i]) NewBlock->addLiveIn(i);
    }
  }

  return NewBlock;
}

/// SplitBB - Break up the block at points that indicate possible branch
/// targets that are not directly reflected in the CFG.  For example,
/// return points from call instructions.
bool CodePlacementOpt::SplitBB(MachineFunction &MF, MachineBasicBlock &MBB) {
  bool Changed = false;
  MachineBasicBlock *Block = &MBB;

  typedef std::vector<MachineInstr*> MIVector;
  MIVector MIs;
  for (MachineBasicBlock::instr_iterator I = MBB.instr_begin(),
                                         E = MBB.instr_end();
       I != E; ++I) {
    MIs.push_back(&*I);
  }

  for (MIVector::iterator I = MIs.begin(), E = MIs.end(); I != E; ++I) {
    MachineInstr *MI = *I;
    const MCInstrDesc &MCID = MI->getDesc();
    if (MCID.isCall()) {
      MachineBasicBlock::instr_iterator It = MI;
      ++It;  // Split the block (i.e. start a new one) at the next instruction.
      if (It != Block->instr_end() && TII->isLegalToSplitMBBAt(*Block, It)) {
        Block = SplitBBAt(MF, *Block, It);
        Changed = true;
      }
    } // if (call)
  }

  return Changed;
}


/// AlignBranchTargets - Align branch targets to target preferred alignments.
///
bool CodePlacementOpt::AlignBranchTargets(MachineFunction &MF) {
  unsigned Align = TLI->getPrefBranchTargetAlignment();
  if (Align == 0) return false;  // Nothing to do.

  if (MF.getFunction()->hasFnAttr(Attribute::OptimizeForSize))
    return false;

  bool Changed = false;
  RS = TRI->requiresRegisterScavenging(MF) ? new RegScavenger() : NULL;

  typedef std::vector<MachineBasicBlock*> MBBVector;
  MBBVector MBBs;
  for (MachineFunction::iterator I = MF.begin(), E = MF.end();
       I != E; ++I) {
    MBBs.push_back(&*I);
  }

  for (MBBVector::iterator I = MBBs.begin(), E = MBBs.end(); I != E; ++I) {
    MachineBasicBlock *MBB = *I;
    Changed |= SplitBB(MF, *MBB);
  }

  for (MachineFunction::iterator I = MF.begin(), E = MF.end(); I != E; ++I) {
    MachineBasicBlock &MBB = *I;
    if (MBB.getAlignment() < Align) {
      if (IsBranchTarget(MBB)) {
        MBB.setAlignment(Align);
        Changed = true;
      }
    }
  }

  delete RS;
  return Changed;
}


bool CodePlacementOpt::runOnMachineFunction(MachineFunction &MF) {
  TLI = MF.getTarget().getTargetLowering();
  TII = MF.getTarget().getInstrInfo();
  TRI = MF.getTarget().getRegisterInfo();

  bool Changed = false;

  MLI = &getAnalysis<MachineLoopInfo>();
  if (!MLI->empty()) {
    Changed |= OptimizeIntraLoopEdges(MF);
    Changed |= AlignLoops(MF);
  }

  Changed |= AlignBranchTargets(MF);

  return Changed;
}
