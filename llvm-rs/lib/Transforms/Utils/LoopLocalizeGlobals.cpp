//===- LoopLocalizeGlobals.cpp - Localize globals ----------------===//
//
// Copyright (c) 2011 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential.
//

#define DEBUG_TYPE "localize-globals"
#include "llvm/Transforms/Scalar.h"
#include "llvm/BasicBlock.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/GlobalVariable.h"
#include "llvm/Type.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Statistic.h"
using namespace llvm;

static cl::opt<bool>
DisableLocalizeGlobals("disable-localize-globals",
               cl::desc("Disable localize global optimization"),
               cl::Hidden, cl::init(false));
namespace {
  //===--------------------------------------------------------------------===//
  // LoopLocalizeGlobals pass implementation
  //
  class LoopLocalizeGlobals : public LoopPass {
  public:
    static char ID; // Pass identification, replacement for typeid
    LoopLocalizeGlobals() : LoopPass(ID) {
      initializeLoopLocalizeGlobalsPass(*PassRegistry::getPassRegistry());
    }

    bool safeToLocalizeInBB(GlobalVariable *GV, BasicBlock *BB);
    bool runOnLoop(Loop *L, LPPassManager &LPM);

    virtual const char *getPassName() const {
      return "Localize globals";
    }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<LoopInfo>();
    }
  };
}

char LoopLocalizeGlobals::ID = 0;
INITIALIZE_PASS_BEGIN(LoopLocalizeGlobals,
                      "localize-globals", "Enable localize globals",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfo)
INITIALIZE_PASS_END(LoopLocalizeGlobals,
                    "localize-globals", "Enable localize globals",
                    false, false)

Pass *llvm::createLoopLocalizeGlobalsPass() {
  return new LoopLocalizeGlobals();
}

bool LoopLocalizeGlobals::safeToLocalizeInBB(GlobalVariable *GV,
                                             BasicBlock *BB) {
  assert (GV && BB);
  // All uses are load and stores
  for (Value::use_iterator UI = GV->use_begin(), UE = GV->use_end();
       UI != UE; ++UI) {
    User *Usr = *UI;
    if (Instruction *II = dyn_cast<Instruction>(Usr)) {
      if (II->getParent() == BB) {
        // Handle only load / store inst
        if (isa<LoadInst>(II) || isa<StoreInst>(II)) {
          // GV should be the ptr and not the value
          if (StoreInst *ST = dyn_cast<StoreInst>(II)) {
            Value *Val = ST->getOperand(0);
            Value *Ptr = ST->getOperand(1);
            if (Val == GV)
              return false;
            assert (Ptr == GV);
          }
          else if (LoadInst *LD = dyn_cast<LoadInst>(II)) {
            Value *Ptr = LD->getOperand(0);
            assert (Ptr == GV);
          }
        } else {
          return false;
        }
      } else if (II->getParent()->getParent() != BB->getParent()) {
        // Probably got inlined after GlobalOpt
        return false;
      }
    }
  }

  return GV->isSafeToLocalize() &&
  (GV->getType()->getElementType()->isIntegerTy() ||
   GV->getType()->getElementType()->isFloatTy() ||
   GV->getType()->getElementType()->isDoubleTy());
}

bool LoopLocalizeGlobals::runOnLoop(Loop *L, LPPassManager &LPM) {
  if (DisableLocalizeGlobals)
    return false;

  if (!L->isLoopSimplifyForm())
    return false;

  if (L->getNumBlocks() != 1)
    return false;

  BasicBlock *Header = L->getHeader();
  BasicBlock *PreHeader = L->getLoopPreheader();
  assert(Header && PreHeader && "Loop is not well structured");

  BasicBlock *Exit = L->getExitBlock();
  // Not handling more than one exit blocks
  if (!Exit)
    return false;

  // Loop through all instructions in the basic block
  SmallPtrSet<GlobalVariable*, 8> GlobalVars;
  for (BasicBlock::iterator I = Header->begin(), E = Header->end();
       I != E; ++I) {
    // If BB contains calls, we can't optimize it. This also takes care of
    // inline assembly since inline asm block are represented as pseudo call
    // instructions.
    if (isa<CallInst>(I)) {
      return false;
    } else if (Instruction *II = dyn_cast<Instruction>(I)) {
      // Find any globals used as operands
      for (unsigned Index = 0; Index < II->getNumOperands(); Index++) {
        Value *Op = II->getOperand(Index);
        if (GlobalVariable *GV = dyn_cast<GlobalVariable>(Op)) {
          GlobalVars.insert(GV);
        }
      } // all operands
    } // if instruction
  } // all inst in bb

  // Go through all global variables and attempt to localize them
  for (SmallPtrSet<GlobalVariable*, 8>::iterator I = GlobalVars.begin(),
       E = GlobalVars.end(); I != E; ++I) {
    GlobalVariable *GV = *I;
    // Safety
    if (!safeToLocalizeInBB(GV, Header))
      continue;

    // Replace global with alloca
    // Create alloca in Entry Block so that Mem2Reg can remove it
    Type* ElemTy = GV->getType()->getElementType();
    BasicBlock &EntryBB = Header->getParent()->getEntryBlock();
    Instruction *InsertPos = EntryBB.getFirstInsertionPt();
    AllocaInst* Alloca = new AllocaInst(ElemTy, NULL, GV->getName() + ".alloc",
                                        InsertPos);

    // Load from GV and store into alloca in PreHeader
    TerminatorInst *TermI = PreHeader->getTerminator();
    LoadInst* LoadI = new LoadInst(GV, GV->getName() + ".load", TermI);
    new StoreInst(LoadI, Alloca, TermI);

    // Insert position in Exit
    InsertPos = Exit->getFirstInsertionPt();
    // Load from alloca
    LoadI = new LoadInst(Alloca, GV->getName() + ".load.alloc", InsertPos);
    // Store into GV
    new StoreInst(LoadI, GV, InsertPos);

    // Replace all uses of GV inside BB with alloca
    DenseMap<Instruction*, AllocaInst*>InstrMap;
    InstrMap.clear();
    for (Value::use_iterator UI = GV->use_begin(), UE = GV->use_end();
         UI != UE; ++UI) {
      User *Usr = *UI;
      if (Instruction *II = dyn_cast<Instruction>(Usr)) {
        if (II->getParent() == Header) {
          // Put it in a map to be replaced later
          InstrMap.insert(std::make_pair(II, Alloca));
        }
      }
    } // find all uses of GV in Header

    // Replace GV with Allocs
    for (DenseMap<Instruction*, AllocaInst*>::iterator ItrB = InstrMap.begin(),
         ItrE = InstrMap.end(); ItrB != ItrE; ++ItrB) {
      Instruction *I = ItrB->first;
      AllocaInst *A = ItrB->second;
      I->replaceUsesOfWith (GV, A);
    }
  } // all globals

  return true;
}
