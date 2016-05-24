// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//===- OptInitGlobals.cpp - Localize function static variables ------------===//
//
//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//

#define DEBUG_TYPE "localize-init-globals"
#include "llvm/Transforms/Scalar.h"
#include "llvm/BasicBlock.h"
#include "llvm/Constants.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/GlobalVariable.h"
#include "llvm/Module.h"
#include "llvm/Type.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Statistic.h"

#include <list>
#include <queue>
using namespace llvm;

static cl::opt<bool>
DisableInitGlobals("disable-localize-init-globals",
               cl::desc("Disable localize init global optimization"),
               cl::Hidden, cl::init(false));
namespace {
  //===--------------------------------------------------------------------===//
  // OptInitGlobals pass implementation
  //
  class OptInitGlobals : public FunctionPass {
  private:
    DominatorTree* DT;
    LoopInfo* LI;
    static const unsigned int MaxGVThreshold = 24;

  public:
    static char ID; // Pass identification, replacement for typeid
    OptInitGlobals() : FunctionPass(ID) {
      initializeOptInitGlobalsPass(*PassRegistry::getPassRegistry());
    }

    bool runOnFunction(Function &F);

    virtual const char *getPassName() const {
      return "Localize initialized globals";
    }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<DominatorTree>();
      AU.addRequired<LoopInfo>();
    }
  };
}

char OptInitGlobals::ID = 0;
INITIALIZE_PASS_BEGIN(OptInitGlobals,
                      "localize-init-globals", "Enable localize init globals",
                      false, false)
INITIALIZE_PASS_END(OptInitGlobals,
                    "localize-init-globals", "Enable localize init globals",
                    false, false)

Pass *llvm::createOptInitGlobalsPass() {
  return new OptInitGlobals();
}

namespace {
  class StaticGVUseInfo {
  public:
    GlobalVariable *GV;
    unsigned int Count;
    StaticGVUseInfo(GlobalVariable *GV, unsigned int Count) :
      GV(GV), Count(Count) {}
  };

  struct StaticGVUseInfoCompare {
    bool operator()(StaticGVUseInfo *A, StaticGVUseInfo *B) const {
      return A->Count < B->Count;
    }
  };
}

bool OptInitGlobals::runOnFunction(Function &F) {
  if (DisableInitGlobals)
    return false;

  DT = &getAnalysis<DominatorTree>();
  LI = &getAnalysis<LoopInfo>();

  // Go through all global variables in the current Module M.
  Module *M = F.getParent();
  // Remember all blocks where GV is used.
  std::list<BasicBlock*> UseBlocks;
  // Maintain a priority queue of GVs to be localized.
  std::priority_queue<StaticGVUseInfo*, std::vector<StaticGVUseInfo*>,
    StaticGVUseInfoCompare> GlobalVarQueue;

  // Loop through all GV in the Module.
  for (Module::global_iterator GVI = M->global_begin(), GVE = M->global_end();
       GVI != GVE; ) {
    GlobalVariable *GV = GVI++;

    // Safety.
    if (!GV->isSafeToLocalize())
      continue;

    bool FlagSafe = true;
    UseBlocks.clear();
    unsigned int UseCount = 0;
    // Go through all uses of GV.
    for (Value::use_iterator UI = GV->use_begin(), UE = GV->use_end(); UI != UE;
         ++UI) {
      User *U = *UI;
      if (Instruction *I = dyn_cast<Instruction>(U)) {
        BasicBlock *BB = I->getParent();
        // GV is used in function F.
        if (BB->getParent() == &F) {
          UseBlocks.push_back(BB);
          UseCount += 2* LI->getLoopDepth(BB);
        }
        else
          FlagSafe = false;
      } else
        FlagSafe = false;
    }

    if (FlagSafe) {
      BasicBlock *DominatorBlock = NULL;
      UseBlocks.unique();
      std::list<BasicBlock*>::iterator UseBlocksItrI = UseBlocks.begin();
      // Find a block that dominates all blocks in UseBlocks.
      for (; UseBlocksItrI != UseBlocks.end(); UseBlocksItrI++) {
        BasicBlock *TheBlockI = *UseBlocksItrI;
        std::list<BasicBlock*>::iterator UseBlocksItrJ = UseBlocks.begin();
        DominatorBlock = TheBlockI;
        for (; UseBlocksItrJ != UseBlocks.end(); UseBlocksItrJ++) {
          BasicBlock *TheBlockJ = *UseBlocksItrJ;

          if (UseBlocksItrI == UseBlocksItrJ)
            continue;

          if (!DT->dominates(TheBlockI, TheBlockJ)) {
            DominatorBlock = NULL;
            break;
          }
        }
        if (DominatorBlock)
          break;
      }

      // A dominator block is found.
      if (DominatorBlock) {
        // Look at all instructions in the block.
        for (BasicBlock::iterator I = DominatorBlock->begin(), E =
             DominatorBlock->end(); I != E; ++I) {
          bool FlagInit = false;
          bool FlagFound = false;
          for (unsigned Index = 0; Index < I->getNumOperands(); Index++) {
            Value *Op = I->getOperand(Index);
            if (GlobalVariable *GV2 = dyn_cast<GlobalVariable>(Op)) {
              // We found a use of GV in instruction I.
              if (GV == GV2) {
                FlagFound = true;
                // Storing into GV.
                if (StoreInst *ST = dyn_cast<StoreInst>(I)) {
                  Value *Val = ST->getOperand(0);
                  Value *Ptr = ST->getOperand(1);
                  if (Val != GV && Ptr == GV)
                    FlagInit = true;
                }
              }
            }
          }
          // Push GV in the queue if it is initialized before any use.
          if (FlagInit)
            GlobalVarQueue.push(new StaticGVUseInfo(GV, UseCount));
          if (FlagFound)
            break;
        }
      }
    }
  }

  // Time to fully localize GV in function F.
  for (unsigned int i = 0; i < MaxGVThreshold; i++) {
    if (GlobalVarQueue.empty())
      break;

    GlobalVariable *GV = GlobalVarQueue.top()->GV;
    GlobalVarQueue.pop();
    Instruction &FirstI = F.getEntryBlock().front();
    Type *ElemTy = GV->getType()->getElementType();
    AllocaInst *Alloca = new AllocaInst(ElemTy, NULL, GV->getName(), &FirstI);
    GV->replaceAllUsesWith(Alloca);
    GV->eraseFromParent();
  }

  return true;
}
