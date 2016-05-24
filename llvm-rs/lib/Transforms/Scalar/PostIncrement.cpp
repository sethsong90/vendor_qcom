//===- PostIncrement.cpp - Post Increment Pass ----------------===//
//
// Copyright (c) 2011 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential.
//

#define DEBUG_TYPE "post-increment"
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

cl::opt<bool> DisablePostInc("disable-postinc",
                             cl::Hidden, cl::init(false),
                             cl::desc("Disable post increment."));

namespace {
  class PostIncrement : public LoopPass {

  private:
    DominanceFrontier* DF;
    DominatorTree* DT;
    PostDominatorTree* PDT;
    LoopInfo* LI;
    int32_t IncSize;
    Value *GEPIndex;
    bool NegIndex;

  public:
    static char ID;
    PostIncrement() : LoopPass(ID) {
      initializePostIncrementPass(*PassRegistry::getPassRegistry());
    }

    bool runOnLoop(Loop *L, LPPassManager &LPM);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<LoopInfo>();
      AU.addRequired<DominanceFrontier>();
      AU.addRequired<DominatorTree>();
      AU.addRequired<PostDominatorTree>();
    }

    // handleOffsetDefinition - Handle various GEP offsets for autoinc.
    bool handleOffsetDefinition(Value *Offset, Loop *L);

    // isIndVariableAndLoopInvariant - if OP0 is IV and OP1 is LI.
    bool isIndVariableAndLoopInvariant(Loop *L, Value *OP0, Value *OP1);

    // isIndVariableAndConstantValue - if OP0 is IV and OP1 is Constant.
    bool isIndVariableAndConstantValue(Loop *L, Value *OP0, Value *OP1);
  };
}

char PostIncrement::ID = 0;
INITIALIZE_PASS_BEGIN(PostIncrement, "post-inc", "Enable post increment", false,
                      false)
INITIALIZE_PASS_DEPENDENCY(LoopInfo)
INITIALIZE_PASS_DEPENDENCY(DominanceFrontier)
INITIALIZE_PASS_DEPENDENCY(DominatorTree)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTree)
INITIALIZE_PASS_END(PostIncrement, "post-inc", "Enable post increment", false,
                    false)

Pass *llvm::createPostIncrementPass() {
  return new PostIncrement();
}

bool PostIncrement::isIndVariableAndLoopInvariant(Loop *L, Value *IV,
                                                  Value *LI) {
  if (L->getCanonicalInductionVariable() == IV && L->isLoopInvariant(LI))
    return true;

  return false;
}

bool PostIncrement::isIndVariableAndConstantValue(Loop *L, Value *IV,
                                                  Value *C) {
  if (L->getCanonicalInductionVariable() == IV && isa<ConstantInt>(C))
    return true;

  return false;
}

// handleOffsetDefinition - Handle GEP offset definition to see if it can be
// converted into autoinc addressing.
//   offset = is a function Fn of IV and (LI or Const).
//   ptr = gep base, offset (base is LI)
//   val = load ptr
// where function Fn is one of the following:
//   PHI, ADD, SUB, MUL, SHIFT
bool PostIncrement::handleOffsetDefinition(Value *Offset, Loop *L) {
  NegIndex = false;
  GEPIndex = NULL;
  IncSize = 0;

  if (Instruction *I = dyn_cast<Instruction>(Offset)) {
    Value *OP0;
    Value *OP1;
    Value *IV = L->getCanonicalInductionVariable();

    // Handle PHI, ADD, SUB, MUL, and SHIFT.
    switch (I->getOpcode()) {
      case Instruction::PHI:
        // x = array[i]
        if (Offset == IV) {
          IncSize = 1;
          return true;
        }
        break;
      case Instruction::Add:
        // x = array[i+loopinv]
        OP0 = I->getOperand(0);
        OP1 = I->getOperand(1);

        if (isIndVariableAndLoopInvariant(L, OP0, OP1)) {
          IncSize = 1;
          GEPIndex = OP1;
          return true;
        } else if (isIndVariableAndLoopInvariant(L, OP1, OP0)) {
          IncSize = 1;
          GEPIndex = OP0;
          return true;
        }
        break;
      case Instruction::Sub:
        // x = array[i-loopinv]
        // x = array[loopinv-i]
        OP0 = I->getOperand(0);
        OP1 = I->getOperand(1);

        if (isIndVariableAndLoopInvariant(L, OP0, OP1)) {
          // x = array[i-loopinv]
          IncSize = 1;
          GEPIndex = OP1;
          NegIndex = true;
          return true;
        } else if (isIndVariableAndLoopInvariant(L, OP1, OP0)) {
          // x = array[loopinv-i]
          IncSize = -1;
          GEPIndex = OP0;
          return true;
        }
        break;
      case Instruction::Mul:
        // x = array[i*const]
        OP0 = I->getOperand(0);
        OP1 = I->getOperand(1);

        if (isIndVariableAndConstantValue(L, OP0, OP1)) {
          // x = array[i*const]
          ConstantInt *C = dyn_cast<ConstantInt>(OP1);
          IncSize = C->getSExtValue();
          return true;
        } else if (isIndVariableAndConstantValue(L, OP1, OP0)) {
          // x = array[const*i]
          ConstantInt *C = dyn_cast<ConstantInt>(OP0);
          IncSize = C->getSExtValue();
          return true;
        }
        break;
      case Instruction::Shl:
        // x = array[i<<const]
        OP0 = I->getOperand(0);
        OP1 = I->getOperand(1);

        if (isIndVariableAndConstantValue(L, OP0, OP1)) {
          ConstantInt *C = dyn_cast<ConstantInt>(OP1);
          IncSize = 1 << C->getSExtValue();
          return true;
        }
        break;
      case Instruction::SExt:
        // x = array[(sext)i]
        OP0 = I->getOperand(0);

        if (OP0 == IV) {
          IncSize = 1;
          return true;
        }
        break;
      default:
        break;
    }
  }

  return false;
}

bool PostIncrement::runOnLoop(Loop *L, LPPassManager &LPM) {
  DT = &getAnalysis<DominatorTree>();
  DF = &getAnalysis<DominanceFrontier>();
  PDT = &getAnalysis<PostDominatorTree>();
  LI = &getAnalysis<LoopInfo>();

  if (DisablePostInc)
    return false;

  if (!L->isLoopSimplifyForm())
    return false;

  BasicBlock *LatchBlock = L->getLoopLatch();
  BasicBlock *PreHeader = L->getLoopPreheader();
  assert(PreHeader && LatchBlock && "Loop is not well structured!");

  // Loop through all basic blocks in the loop.
  for (Loop::block_iterator I = L->block_begin(), E = L->block_end();
       I != E; ++I) {
    // Get the basic block.
    BasicBlock *BB = *I;

    if (!DT->dominates(BB, LatchBlock))
      continue;

    // Loop though all instructions in the basic block.
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
      // Handle only GEP instructions.
      if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(I)) {
        // Handle GEP with only 2 operands.
        if (GEP->getNumOperands() != 2)
          continue;

        // Get base and offset fields.
        Value *Base = GEP->getOperand(0);
        Value *Offset = GEP->getOperand(1);

        // Make sure Base is loop invariant.
        if (!L->isLoopInvariant(Base))
          continue;

        // Make sure offset is Ind variable.
        if (!handleOffsetDefinition(Offset, L))
          continue;

        std::list<BasicBlock*> UseBlocks;
        bool AutoIdxUnsafe = false;

        // Make sure all uses of GEP are within the loop.
        for (Value::use_iterator UI = GEP->use_begin(), UE = GEP->use_end();
             UI != UE; ) {
          Use &TheUse = UI.getUse();
          ++UI;
          if (Instruction *I = dyn_cast<Instruction>(TheUse.getUser())) {
            BasicBlock *Parent = I->getParent();
            if (!L->contains(Parent)) {
              AutoIdxUnsafe = true;
              break;
            }
            // Push Parent in use list.
            UseBlocks.push_back(Parent);
          }
        }

        // Try next if unsafe.
        if (AutoIdxUnsafe)
          continue;

        // Nothing to do if UseBlocks is empty.
        if (UseBlocks.empty())
          continue;

        // Now go through the use list and find the bb that post dominates all
        // other uses.
        bool FoundPDomUse = false;
        std::list<BasicBlock*>::iterator UseBlocksItrI = UseBlocks.begin();
        for (; UseBlocksItrI != UseBlocks.end(); UseBlocksItrI++) {
          BasicBlock *TheBlockI = *UseBlocksItrI;
          std::list<BasicBlock*>::iterator UseBlocksItrJ = UseBlocks.begin();
          FoundPDomUse = true;
          for (; UseBlocksItrJ != UseBlocks.end(); UseBlocksItrJ++) {
            BasicBlock *TheBlockJ = *UseBlocksItrJ;

            if (UseBlocksItrI == UseBlocksItrJ)
              continue;

            if (!PDT->dominates(TheBlockI, TheBlockJ)) {
              FoundPDomUse = false;
              break;
            }
          }
          if (FoundPDomUse)
            break;
        }

        // Try next if we cannot find a use that post dominates all other uses.
        if (FoundPDomUse == false)
          continue;

        // Get the basic block that post dominates all other uses.
        BasicBlock *PostDomBlock = *UseBlocksItrI;

        // PostDomBlock loop depth = GEP loop depth.
        if (LI->getLoopDepth(PostDomBlock) != LI->getLoopDepth(BB))
          continue;

        // PostDomBlock must dominate the Latch block.
        if (!DT->dominates(PostDomBlock, LatchBlock))
          continue;

        // All systems go.
        BasicBlock *PreheaderBlock = L->getLoopPreheader();

        // Negate GEPIndex if needed.
        Value *GEPOffset = NULL;
        if (GEPIndex) {
          GEPOffset = GEPIndex;
          if (NegIndex) {
            Value *Zero = Constant::getNullValue(GEPIndex->getType());
            GEPOffset = BinaryOperator::Create(Instruction::Sub, Zero,
                                               GEPIndex,
                                               GEP->getName() + ".neg",
                                               PreheaderBlock->getTerminator());
          }
        } else
          GEPOffset = Constant::getNullValue(Offset->getType());

        // Insert the new gep instruction in the pre-header
        PHINode *Phi = L->getCanonicalInductionVariable();
        GetElementPtrInst *NewGEP =
          GetElementPtrInst::Create(Base, GEPOffset, GEP->getName() + ".gep",
                                    PreheaderBlock->getTerminator());

        // Create a new PHI instruction in the Header.
        PHINode *NewPN = PHINode::Create(NewGEP->getType(), 2, GEP->getName() +
                                         ".phi", Phi);

        // Create a new Inc instruction in the PostDomBlock.
        Instruction *IncGEP = GetElementPtrInst::Create(
            NewPN, Constant::getIntegerValue(Phi->getType(),
                                             APInt(32, IncSize)),
            GEP->getName()+".inc", PostDomBlock->getTerminator());

        // Add Phi node values.
        NewPN->addIncoming(NewGEP, PreHeader);
        NewPN->addIncoming(IncGEP, LatchBlock);

        SSAUpdater SSA;
        SSA.Initialize(NewGEP->getType(), NewGEP->getName());
        SSA.AddAvailableValue(PreheaderBlock, NewGEP);
        SSA.AddAvailableValue(PostDomBlock, IncGEP);

        // Replace original GEP with NewPN.
        for (Value::use_iterator UI = GEP->use_begin(), UE = GEP->use_end();
             UI != UE; ) {
          Use &TheUse = UI.getUse();
          ++UI;
          if (Instruction *I = dyn_cast<Instruction>(TheUse.getUser())) {
            I->replaceUsesOfWith(GEP, NewPN);
          }
        }

        // Recompute the PHI nodes for NewGEP.
        for (Value::use_iterator UI = NewGEP->use_begin(),
                                 UE = NewGEP->use_end(); UI != UE; ) {
          Use &TheUse = UI.getUse();
          ++UI;
          SSA.RewriteUse(TheUse);
        }
      } // Handle GEP.
    } // For all inst in bb.
  } // For all bb in loop.
  return true;
}
