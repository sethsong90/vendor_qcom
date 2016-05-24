// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//===- LoopIndexOffset.cpp - Induction Variable Elimination ---------------===//
//
//                     The LLVM Compiler Infrastructure
//

#define DEBUG_TYPE "loop-index-offset"
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
#include "llvm/Analysis/PostDominators.h"
#include <list>
#include <map>
using namespace llvm;

namespace {
  class LoopIndexOffset : public LoopPass {

  private:

  public:
    static char ID;
    LoopIndexOffset() : LoopPass(ID) {
      initializeLoopIndexOffsetPass(*PassRegistry::getPassRegistry());
    }
    bool runOnLoop(Loop *L, LPPassManager &LPM);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<DominatorTree>();
      AU.addPreserved<DominatorTree>();
      
      AU.addRequired<LoopInfo>();
      AU.addPreserved<LoopInfo>();

    }
  };
}

char LoopIndexOffset::ID = 0;
INITIALIZE_PASS_BEGIN(LoopIndexOffset, "loop-index-offset", 
                      "Enable loop index offset optimization", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfo)
INITIALIZE_PASS_DEPENDENCY(DominatorTree)
INITIALIZE_PASS_END(LoopIndexOffset, "loop-index-offset", 
                    "Enable loop index offset optimization", false, false)




Pass *llvm::createLoopIndexOffsetPass() {
  return new LoopIndexOffset();
}

// LoopIndexOffset - Transform a loop with sgt compare into cmp eq. This transformation helps
// in identification of loop trip count for non standard loops such as:
//
// for (index = trip; index <= trip+5; index++)
//  {
//    buffer[index] = ret++;
//  }
//
// An example of LLVM IR before and after LoopIndexOffset transformation is shown below:

/***************************************************************************************************
*   define i32 @foo(i32 %index, i32 %trip, i32 %ret) nounwind {                                    *
*   bb.nph:                                                                                        *
*     %add = add nsw i32 %trip, 5           (AddValue)                                             *
*     %tmp13 = add i32 %trip, 1             (IncOp0)                                               *
*     %tmp16 = add i32 %ret, 1                                                                     *
*     br label %for.cond                                                                           *
*                                                                                                  *
*   for.cond:                                         ; preds = %for.cond, %bb.nph                 *
*     %indvar = phi i32 [ 0, %bb.nph ], [ %indvar.next, %for.cond ]                                *
*     ...                                                                                          *
*     %inc7 = add i32 %tmp13, %indvar       (IncValue)                                             *
*     %cmp = icmp sgt i32 %inc7, %add                                                              *
*     %indvar.next = add i32 %indvar, 1                                                            *
*     br i1 %cmp, label %for.end, label %for.cond                                                  *
*                                                                                                  *
*   for.end:                                          ; preds = %for.cond                          *
*     %inc = add i32 %tmp16, %indvar                                                               *
*     ret i32 %inc                                                                                 *
*   }                                                                                              *
***************************************************************************************************/

/***************************************************************************************************
*   define i32 @foo(i32 %index, i32 %trip, i32 %ret) nounwind {                                    *
*   bb.nph:                                                                                        *
*     %tmp16 = add i32 %ret, 1                                                                     *
*     br label %for.cond                                                                           *
*                                                                                                  *
*   for.cond:                                         ; preds = %for.cond, %bb.nph                 *
*     %indvar = phi i32 [ 0, %bb.nph ], [ %indvar.next, %for.cond ]                                *
*     %tmp = add i32 %indvar, %trip                                                                *
*     %ret.addr.011 = add i32 %indvar, %ret                                                        *
*     %tmp5 = load i32** @buffer, align 4                                                          *
*     %arrayidx = getelementptr i32* %tmp5, i32 %tmp                                               *
*     store i32 %ret.addr.011, i32* %arrayidx, align 4                                             *
*     %indvar.next = add i32 %indvar, 1                                                            *
*     %cmp-idx = icmp eq i32 %indvar.next, 6                                                       *
*     br i1 %cmp-idx, label %for.end, label %for.cond                                              *
*                                                                                                  *
*   for.end:                                          ; preds = %for.cond                          *
*     %inc = add i32 %tmp16, %indvar                                                               *
*     ret i32 %inc                                                                                 *
*   }                                                                                              *
***************************************************************************************************/

bool LoopIndexOffset::runOnLoop(Loop *L, LPPassManager &LPM) {
  PHINode *IV = L->getCanonicalInductionVariable();
  if (IV == 0 || IV->getNumIncomingValues() != 2)
    return false;

  bool P0InLoop = L->contains(IV->getIncomingBlock(0));

  // Get the incremented index
  Value *Inc = IV->getIncomingValue(!P0InLoop);
  // Get the Backedge
  BasicBlock *BackedgeBlock = IV->getIncomingBlock(!P0InLoop);

  if (BranchInst *BI = dyn_cast<BranchInst>(BackedgeBlock->getTerminator())) {
    if (BI->isConditional()) {
      if (ICmpInst *ICI = dyn_cast<ICmpInst>(BI->getCondition())) {
        // Handle sgt only
        if (ICI->getPredicate() != ICmpInst::ICMP_SGT)
          return false;

        Value *IncValue = ICI->getOperand(0);
        Value *AddValue = ICI->getOperand(1);

        // IncValue must be increment of loop iv and index var
        if (BinaryOperator *IncBO = dyn_cast<BinaryOperator>(IncValue)) {
          if (IncBO->getOpcode() != BinaryOperator::Add)
            return false;

          Value *IncOp0 = IncBO->getOperand(0);
          Value *IncOp1 = IncBO->getOperand(1);

          // Second operator to compare must be loop iv
          if (L->isLoopInvariant(AddValue)
              && L->isLoopInvariant (IncOp0) && IncOp1 == IV) {
            if (BinaryOperator *AddValueBO = dyn_cast<BinaryOperator>(AddValue)) {
              if (BinaryOperator *IncOp0BO = dyn_cast<BinaryOperator>(IncOp0)) {
                // We are handling only Add
                if (AddValueBO->getOpcode() == BinaryOperator::Add
                    &&IncOp0BO->getOpcode() == BinaryOperator::Add) {
                  // Get trip counts
                  Value *Trip1 = AddValueBO->getOperand(0);
                  Value *Trip2 = IncOp0BO->getOperand(0);

                  // The leaf trip count nodes must match
                  if (Trip1 != Trip2)
                    return false;

                  // Other operator to inc and add must be constants
                  if (ConstantInt *C1 = dyn_cast<ConstantInt>(AddValueBO->getOperand(1))) {
                    if (ConstantInt *C2 = dyn_cast<ConstantInt>(IncOp0BO->getOperand(1))) {
                      // Determine new loop count
                      int64_t NewLoopCount = C1->getSExtValue() - C2->getSExtValue() + 2;
                      assert (NewLoopCount >= 0);
                      Value *CValue = ConstantInt::get(C1->getType(), NewLoopCount);
                      // Generate the new cmp eq instruction
                      ICmpInst* Comp = new ICmpInst(BackedgeBlock->getTerminator(),
                                                    ICmpInst::ICMP_EQ, Inc, CValue, "cmp-idx");
                      // Replace BI's conditinal with Comp
                      BI->setCondition (Comp);
                      // DCE will take care of cleaning up
                      return true;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  return true;
}
