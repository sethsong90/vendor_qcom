// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//===- PartialLICM.cpp - Partial Loop Invariant Code Motion ---------------===//
//
//                     The LLVM Compiler Infrastructure
//

#define DEBUG_TYPE "partial-licm"
#include "llvm/Transforms/Scalar.h"
#include "llvm/BasicBlock.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Type.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/PostDominators.h"
#include <list>
#include <map>
using namespace llvm;

namespace {
  class PartialLICM : public LoopPass {

  private:
    DominanceFrontier* DF;
    DominatorTree* DT;
    PostDominatorTree* PDT;
    LoopInfo* LI;

  public:
    static char ID; // Pass identification, replacement for typeid
    PartialLICM() : LoopPass(ID) {
      initializePartialLICMPass(*PassRegistry::getPassRegistry());
    }

    bool runOnLoop(Loop *L, LPPassManager &LPM);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<LoopInfo>();
      AU.addRequired<DominanceFrontier>();
      AU.addRequired<DominatorTree>();
      AU.addRequired<PostDominatorTree>();
    }

  };
}

char PartialLICM::ID = 0;
INITIALIZE_PASS_BEGIN(PartialLICM, "partial-licm", "Partial LICM", false,
                      false)
INITIALIZE_PASS_DEPENDENCY(LoopInfo)
INITIALIZE_PASS_DEPENDENCY(DominanceFrontier)
INITIALIZE_PASS_DEPENDENCY(DominatorTree)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTree)
INITIALIZE_PASS_END(PartialLICM, "Partial-licm", "partial LICM", false,
                    false)

Pass *llvm::createPartialLICMPass() {
  return new PartialLICM();
}

//
// PartialLICM - attempt to perform partial LICM based on
// associativity property of input operands
// Example-
// 
// Linv1 = computation
// Linv2 = compuation
//
// Loop1:
//  LoopAdd = Linv1 + IndVar
//  LoopAdd1 = LoopAdd + Linv2
// 
// - into -
// 
// Linv1 = computation
// Linv2 = compuation
// Linv3 = Linv1 + Linv2
//
// Loop1:
//  LoopAdd1 = Linv3 + IndVar
//

bool PartialLICM::runOnLoop(Loop *L, LPPassManager &LPM) {

  DT = &getAnalysis<DominatorTree>();
  DF = &getAnalysis<DominanceFrontier>();
  PDT = &getAnalysis<PostDominatorTree>();
  LI = &getAnalysis<LoopInfo>();

  if (!L->isLoopSimplifyForm())
    return false;

  // loop thru all basic blocks in the loop
  for (Loop::block_iterator I = L->block_begin(), E = L->block_end(); I != E; ++I) {
    BasicBlock *BB = *I;

    if (BB->size() > 25)
      continue;

    // loop thru all instructions in the basic block
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
      // check if getelementptr instruction
      if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(I)) {
        if (GEP->getNumOperands() == 2) {
          // get base and offset
          Value *Base = GEP->getOperand(0);
          Value *Offset = GEP->getOperand(1);

          if (!L->isLoopInvariant(Base))
            continue;

          // make sure Offset is i32 type
          if (!Offset->getType()->isIntegerTy(32))
            continue;

          if (isa<Instruction>(Offset)) {
            Instruction *OffsetI = dyn_cast<Instruction>(Offset);
            if (OffsetI->getOpcode() == Instruction::Add) {
              Value * AddOp0 = OffsetI->getOperand(0);
              Value * AddOp1 = OffsetI->getOperand(1);

              if (!L->isLoopInvariant(AddOp0))
                continue;

              if (L->isLoopInvariant(AddOp1))
                continue;

              if (BinaryOperator *BO = dyn_cast<BinaryOperator>(AddOp1)) {
                Value *BinOp0 = BO->getOperand(0);
                Value *BinOp1 = BO->getOperand(1);

                if (BinOp0 != L->getCanonicalInductionVariable())
                  continue;

                if (isa<ConstantInt>(BinOp1)) {
                  BasicBlock *PreheaderBlock = L->getLoopPreheader();
                  GetElementPtrInst *BaseOffsetGEP = GetElementPtrInst::Create(Base, AddOp0, Base->getName()+".pst",
                                                                        PreheaderBlock->getTerminator());

                  GetElementPtrInst *NewGEP = GetElementPtrInst::Create(BaseOffsetGEP,
                                                                        AddOp1, GEP->getName()+".pst", GEP);

                  for (Value::use_iterator UI = GEP->use_begin(), UE = GEP->use_end(); UI != UE;) {
                    Use &TheUse = UI.getUse();
                    ++UI;
                    if (Instruction *I = dyn_cast<Instruction>(TheUse.getUser())) {
                      I->replaceUsesOfWith(GEP, NewGEP);
                    }
                  }
                }
              } // Binary op
            } // Add
          } // is Instruction
        } // gep operands == 2
      } // gep instruction
    } // for all instructions in bb
  } // for all loops

  return true;
}
