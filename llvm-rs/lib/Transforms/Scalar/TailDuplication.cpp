// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//===--------- TailDuplication.cpp - Tail duplication ---------------------===//
//
//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
//===----------------------------------------------------------------------===//
//
// Perform tail duplication on constant phi nodes.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "ir-tail-dup"

#include "llvm/Transforms/Scalar.h"

#include "llvm/Constants.h"
#include "llvm/Function.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"


using namespace llvm;

static cl::opt<bool> EnableIRTailDuplication("enable-ir-taildup",
                                             cl::init(true), cl::Hidden);
static
cl::opt<unsigned>IRTailDupHeurMinNumCalls("ir-taildup-heur-min-num-calls",
                                          cl::init(5), cl::Hidden);
namespace {
  /// TailDuplication Pass - Performs tail duplication on constant phi nodes.
  class TailDuplication : public FunctionPass {

    PostDominatorTree *PDT;

    SmallVector<BasicBlock *, 4> ExitingBBs;
    SmallPtrSet<CallInst *, 8> UsingCallSites;

    void findExitingBasicBlocks(Function &F);

    PHINode *findAllConstPhi(BasicBlock *BB);

    BasicBlock *getPostDominatingExitBlock(Instruction *I);

    void cloneTails(PHINode *StartPhi, BasicBlock *PostDom);

    void cloneTail(BasicBlock *CloneRegionEntry,
                   BasicBlock *CloneRegionEnd,
                   BasicBlock *CloneRegionPred,
                   unsigned CloneNum);

    void reset() {
      ExitingBBs.clear();
      UsingCallSites.clear();
    }

  public:
    static char ID;

    TailDuplication() : FunctionPass(ID) {
      initializeTailDuplicationPass(*PassRegistry::getPassRegistry());
    }

    bool runOnFunction(Function &F);

    // This transformation requires dominator and postdominator info.
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<PostDominatorTree>();
      // We change the CFG.
    }

  };

}

char TailDuplication::ID = 0;

FunctionPass *llvm::createTailDuplicationPass() {
  return new TailDuplication();
}

INITIALIZE_PASS_BEGIN(TailDuplication, "ir-taildup", "IR Tail Duplication",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTree)
INITIALIZE_PASS_END(TailDuplication, "ir-taildup", "IR Tail Duplication", false,
                    false)

void TailDuplication::findExitingBasicBlocks(Function &F) {
  for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
    BasicBlock *BB = &*FI;
    if (BB->getTerminator() && isa<ReturnInst>(BB->getTerminator()))
      ExitingBBs.push_back(BB);
  }
}

/// isBeneficialToTailDuplicate - Cost function to determine profitability of
/// tail duplicating.
static bool isBeneficialToTailDuplicate(PHINode *Phi,
                                 SmallPtrSet<CallInst *, 8> &UserCallSites) {
  unsigned NumCalls = 0;
  SmallVector<CallInst *, 8> TmpUserCallSites;
  for (Value::use_iterator UI = Phi->use_begin(), UE = Phi->use_end(); UI != UE;
       ++UI)
    if (CallInst *Call = dyn_cast<CallInst>(*UI)) {
      TmpUserCallSites.push_back(Call);
      ++NumCalls;
    }

  if (NumCalls > IRTailDupHeurMinNumCalls) {
    UserCallSites.insert(TmpUserCallSites.begin(), TmpUserCallSites.end());
    return true;
  }

  return false;
}

/// findAllConstPhi - Find a phi node whose inputs are all constant and which
/// looks like to be beneficial to replace by one constant by tail duplicating.
PHINode *TailDuplication::findAllConstPhi(BasicBlock *BB) {
  for (BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
    PHINode *Phi = dyn_cast<PHINode>(&*BI);
    if (!Phi)
      break;
    // Check that all incoming values are constants.
    bool IsAllConst = true;
    for (unsigned I = 0, E = Phi->getNumIncomingValues(); I != E; ++I) {
       if (!isa<ConstantInt>(Phi->getIncomingValue(I))) {
         IsAllConst = false;
         break;
       }
    }
    // If they are not all constant, look at the next phi node.
    if (!IsAllConst) continue;

    if (isBeneficialToTailDuplicate(Phi, UsingCallSites))
      return Phi;
  }
  return 0;
}

/// getPostDominatingExitBlock - Returns the basic block that exits the function
/// and post-dominates the instruction's \p Inst basic block.
BasicBlock * TailDuplication::getPostDominatingExitBlock(Instruction *Inst) {
  for (unsigned I = 0, E = ExitingBBs.size(); I != E; ++I)
    if (PDT->dominates(ExitingBBs[I], Inst->getParent()))
      return ExitingBBs[I];
  return 0;
}

/// isSafeToTailDuplicate - Returns false if there is branch to outside the
/// region from within the region.
bool isSafeToTailDuplicate(BasicBlock *RegionEntry, BasicBlock *RegionEnd) {
  if (RegionEntry == RegionEnd)
    return true;
  SmallPtrSet<BasicBlock *, 16> Visited;
  df_ext_iterator<BasicBlock *, SmallPtrSet<BasicBlock *, 16> >
    DFI = df_ext_begin(RegionEntry, Visited),
      E = df_ext_end(RegionEntry, Visited);
  // Skip over RegionEntry.
  ++DFI;
  // Remove the entry. We want to see whether we hit it again.
  Visited.erase(RegionEntry);
  while (DFI != E) {
    BasicBlock *Cur = *DFI;
    // Detected a cyle involving the RegionEntry.
    if (Cur == RegionEntry)
      return false;
    ++DFI;
  }
  return true;
}

bool TailDuplication::runOnFunction(Function &F) {
  if (!EnableIRTailDuplication)
    return false;

  bool HasChanged = false;
  PDT = &getAnalysis<PostDominatorTree>();

  reset();

  findExitingBasicBlocks(F);

  // Iterate depth-first over the basic blocks to find opportunity.
  for (df_iterator<BasicBlock*> DI = df_begin(&F.getEntryBlock()),
       DE = df_end(&F.getEntryBlock()); DI != DE; ++DI) {

    // Ignore basic blocks with a single predecessor.
    if ((*DI)->getSinglePredecessor())
      continue;

    PHINode *AllConstPhi = findAllConstPhi(*DI);
    if (!AllConstPhi)
      continue;

    BasicBlock *PostDom = getPostDominatingExitBlock(AllConstPhi);
    if (!PostDom)
      continue;

    // Check for cycles to outside the region.
    if (!isSafeToTailDuplicate(AllConstPhi->getParent(), PostDom))
      continue;

    HasChanged = true;

    // Inline the call sites that use the phi.
    for (SmallPtrSet<CallInst *, 8>::iterator CI = UsingCallSites.begin(),
         CE = UsingCallSites.end(); CI != CE; ++CI) {
      CallInst *Call = *CI;
      CallSite CS(Call);
      // Ignore recursive calls.
      if (!CS || CS.getCalledFunction() == &F)
        continue;

      DEBUG(errs() << "Trying " << *Call << "\n");
      InlineFunctionInfo IFI;
      bool HasInlined = InlineFunction(CS, IFI);
      (void)HasInlined;
      DEBUG(
        if (HasInlined)
          errs() << "Have inlined " <<
            CS.getCalledFunction()->getName() << "\n";
           );
    }
    cloneTails(AllConstPhi, PostDom);

    // FIXME: For now we stop once we encountered the first opportunity.
    break;
  }

  return HasChanged;
}

static void deleteIncomingEdgesFromOutsideTheRegion(BasicBlock *BB,
                                                    ValueToValueMapTy &VMap) {
  // An edge is from outside the region if it is not in VMap and is not the
  // EntryPred block.
  for (BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
    if (PHINode *Phi = dyn_cast<PHINode>(&*BI)) {
      SmallVector<BasicBlock *, 4> EdgeToDelete;
      for (unsigned I = 0, E = Phi->getNumIncomingValues(); I != E; ++I) {
        BasicBlock *IncomingBlock = Phi->getIncomingBlock(I);
        if (!VMap.count(IncomingBlock))
          EdgeToDelete.push_back(IncomingBlock);
      }
      for (unsigned I = 0, E = EdgeToDelete.size(); I != E; ++I)
        Phi->removeIncomingValue(EdgeToDelete[I]);
    } else
      // Saw the last phi node.
      return;
  }
}

/// cloneTail - Clone the region starting at \p CloneRegionEntry and ending at
/// \p CloneRegionEnd to \p CloneRegionPred.
void TailDuplication::cloneTail(BasicBlock *CloneRegionEntry,
                                BasicBlock *CloneRegionEnd,
                                BasicBlock *CloneRegionPred,
                                unsigned CloneNum) {
  ValueToValueMapTy VMap;

  // Add predecessor block to map.
  VMap[CloneRegionPred] = CloneRegionPred;

  // Clone basic blocks.
  SmallVector<BasicBlock *, 32> ClonedBlocks;
  for (df_iterator<BasicBlock*> DI = df_begin(CloneRegionEntry),
       DE = df_end(CloneRegionEntry); DI != DE; ++DI) {
    BasicBlock *BB = *DI;

    // Create a new basic block and copy instructions into it!
    BasicBlock *CBB = CloneBasicBlock(BB, VMap, ".taildup." +
                                      Twine(CloneNum));
    ClonedBlocks.push_back(CBB);
    Function::iterator InsertPt = CloneRegionEntry;
    BB->getParent()->getBasicBlockList().insert(InsertPt, CBB);

    // Add basic block mapping.
    VMap[BB] = CBB;
  }

  // Fix phi nodes with incoming values from basic blocks outside of the cloned
  // region.
  for (unsigned I = 0, E = ClonedBlocks.size(); I != E; ++I)
    for (BasicBlock::iterator BI = ClonedBlocks[I]->begin(),
         BE = ClonedBlocks[I]->end(); BI != BE; ++BI)
    deleteIncomingEdgesFromOutsideTheRegion(ClonedBlocks[I], VMap);


  // Remap instruction in cloned region.
  for (unsigned I = 0, E = ClonedBlocks.size(); I != E; ++I)
    for (BasicBlock::iterator BI = ClonedBlocks[I]->begin(),
         BE = ClonedBlocks[I]->end(); BI != BE; ++BI)
    RemapInstruction(&*BI, VMap,
                     RF_NoModuleLevelChanges | RF_IgnoreMissingEntries);

  // Link region's predecessor to newly created region.
  BranchInst *Br = dyn_cast<BranchInst>(CloneRegionPred->getTerminator());
  assert(Br && "Need a branch instruction");
  for (unsigned I = 0, E = Br->getNumSuccessors(); I != E; ++I) {
    if (Br->getSuccessor(I) == CloneRegionEntry) {
      Value *V = VMap[CloneRegionEntry];
      assert(dyn_cast<BasicBlock>(V) && "Block must be mapped to a block");
      Br->setSuccessor(I, dyn_cast<BasicBlock>(V));
    }
  }
}

/// cloneTails - Clones the tail starting at ConstPhi's basic block up to
/// PostDom into ConstPhi's basic block predecessors.
void TailDuplication::cloneTails(PHINode *ConstPhi, BasicBlock *PostDom) {
  // Clone the tail into the first N-1 predecessors.
  unsigned LastIncomingValIndex = ConstPhi->getNumIncomingValues() - 1;
  SmallVector<BasicBlock*, 8> ReroutedBlocks;
  for (unsigned PI = 0; PI != LastIncomingValIndex; ++PI) {
    ReroutedBlocks.push_back(ConstPhi->getIncomingBlock(PI));
    cloneTail(ConstPhi->getParent(), PostDom, ConstPhi->getIncomingBlock(PI),
              PI);
  }
  assert(ReroutedBlocks.size() == LastIncomingValIndex);

  // Fixup the phis in the entry block of the region.
  BasicBlock *Entry = ConstPhi->getParent();
  for (BasicBlock::iterator BI = Entry->begin(), BE = Entry->end(); BI != BE;
       ++BI) {
    if (PHINode *Phi = dyn_cast<PHINode>(&*BI)) {
      for (unsigned PI = 0; PI != LastIncomingValIndex; ++PI)
        Phi->removeIncomingValue(ReroutedBlocks[PI]);
    } else
      break;
  }
}

