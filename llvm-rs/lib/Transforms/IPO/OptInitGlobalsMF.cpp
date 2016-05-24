// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//===- OptInitGlobalsMF.cpp - Localize static variables inside a function -===//
//
//                     The LLVM Compiler Infrastructure
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "localize-func-globals"
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
#include "llvm/Support/InstIterator.h"
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
DisableInitGlobals("disable-localize-func-globals",
               cl::desc("Disable localize global optimization"),
               cl::Hidden, cl::init(false));
namespace {
  //===--------------------------------------------------------------------===//
  // OptInitGlobalsMF pass implementation
  //
  class OptInitGlobalsMF : public FunctionPass {
  private:
    static const unsigned int MinUseThreshold = 4;
    static const unsigned int MaxCallThreshold = 2;

  public:
    static char ID; // Pass identification, replacement for typeid
    OptInitGlobalsMF() : FunctionPass(ID) {
      initializeOptInitGlobalsMFPass(*PassRegistry::getPassRegistry());
    }

    bool runOnFunction(Function &F);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    }
  };
}

char OptInitGlobalsMF::ID = 0;
INITIALIZE_PASS_BEGIN(OptInitGlobalsMF,
                      "localize-func-globals",
                      "Enable localize globals inside function",
                      false, false)
INITIALIZE_PASS_END(OptInitGlobalsMF,
                    "localize-func-globals",
                    "Enable localize globals inside function",
                    false, false)

Pass *llvm::createOptInitGlobalsMFPass() {
  return new OptInitGlobalsMF();
}

bool OptInitGlobalsMF::runOnFunction(Function &F) {
  if (DisableInitGlobals)
    return false;

  // Go through all GVs in module M.
  Module *M = F.getParent();
  // Remember where GV is used.
  std::list<Instruction *> UseInstrs;
  // Remember all call instructions in F.
  std::list<Instruction *> CallInstrs;
  // Remember all ret instructions in F.
  std::list<ReturnInst *> RetInstrs;

  // Loop through all GVs in M.
  for (Module::global_iterator GVI = M->global_begin(), GVE = M->global_end();
       GVI != GVE; ) {
    GlobalVariable *GV = GVI++;

    // Safety.
    if (!GV->isSafeToLocalizeMF())
      continue;

    bool FlagSafe = true;
    UseInstrs.clear();
    CallInstrs.clear();
    RetInstrs.clear();
    // Look at all uses of GV and populate UseInstrs.
    for (Value::use_iterator UI = GV->use_begin(), UE = GV->use_end(); UI != UE;
         ++UI) {
      User *U = *UI;
      if (Instruction *I = dyn_cast<Instruction>(U)) {
        BasicBlock *BB = I->getParent();
        if (BB->getParent() == &F)
          UseInstrs.push_back(I);
      } else
        FlagSafe = false;
    }

    if (FlagSafe && UseInstrs.size() >= MinUseThreshold) {
      // Collect all call and ret instructions in F.
      for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
        if (CallInst *CI = dyn_cast<CallInst>(I.getInstructionIterator()))
          CallInstrs.push_back(CI);
        if (ReturnInst *RI = dyn_cast<ReturnInst>(I.getInstructionIterator()))
          RetInstrs.push_back(RI);
      }

      // Use some heuristic for profitability.
      if (CallInstrs.size() <= MaxCallThreshold && RetInstrs.size() == 1) {
        // We have decided to localize GV.
        Type* ElemTy = GV->getType()->getElementType();
        BasicBlock &EntryBB = F.getEntryBlock();
        Instruction *InsertPos = EntryBB.getFirstInsertionPt();
        // Insert an alloca in the Entry block.
        AllocaInst* Alloca =
          new AllocaInst(ElemTy, NULL, GV->getName() + ".alloc", InsertPos);
        // Load from GV.
        LoadInst* LoadI =
          new LoadInst(GV, GV->getName() + ".load", InsertPos);
        // Store into alloca.
        new StoreInst(LoadI, Alloca, InsertPos);

        // Find all instructions using GV and replace with alloca.
        std::list<Instruction *>::iterator  UseItrB;
        std::list<Instruction *>::iterator  UseItrE;
        for (UseItrB = UseInstrs.begin(), UseItrE = UseInstrs.end();
             UseItrB != UseItrE; UseItrB++) {
          Instruction *I = *UseItrB;
          I->replaceUsesOfWith(GV, Alloca);
        }

        // Handle call and ret instructions.
        for (UseItrB = CallInstrs.begin(), UseItrE = CallInstrs.end();
             UseItrB != UseItrE; UseItrB++) {
          // For call instructions, store before and load after the call.
          Instruction *CI = *UseItrB;
          LoadI = new LoadInst(Alloca, Alloca->getName() + ".load", CI);
          new StoreInst(LoadI, GV, CI);

          LoadInst* LoadI = new LoadInst(GV, GV->getName() + ".load");
          LoadI->insertAfter(CI);
          StoreInst *StoreI = new StoreInst(LoadI, Alloca);
          StoreI->insertAfter(LoadI);
        }

        // Before ret, load from alloca and store into GV.
        ReturnInst *RI = RetInstrs.front();
        LoadI = new LoadInst(Alloca, Alloca->getName() + ".load", RI);
        new StoreInst(LoadI, GV, RI);
      }
    }
  }

  return true;
}
