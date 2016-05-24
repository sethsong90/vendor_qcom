// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//===- TypePromotion.cpp - Promote Types ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
//

#define DEBUG_TYPE "type-promotion"
#include "llvm/Transforms/Scalar.h"
#include "llvm/BasicBlock.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Type.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/CommandLine.h"
#include <list>
#include <map>
using namespace llvm;

cl::opt<bool> DisableTypePromotion("disable-idxvar-promotion",
              cl::Hidden, cl::init(false),
              cl::desc("Disable induction variable type promotion."));

namespace {
  class TypePromotion : public LoopPass {
  private:
    // List to hold values waiting to be promoted.
    std::list<Instruction*>PromoteList;
    // Map from old instructions to promoted instructions.
    std::map<Instruction*,Instruction*>PromoteMap;
    // Remember original phi node.
    PHINode *OriginalPhi;

  public:
    static char ID;
    TypePromotion() : LoopPass(ID) {
      initializeTypePromotionPass(*PassRegistry::getPassRegistry());
    }

    bool runOnLoop(Loop *L, LPPassManager &LPM);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<LoopInfo>();
    }

    // instructionPromotable - return true if instruction I can be promoted.
    bool instructionPromotable(Instruction *I);

    // dfsTypePromotion - recursively walk the use-def chain for type promotion.
    bool dfsTypePromotion(Loop *L, Instruction *I);

    // instructionOperandsPromotable - return true if all operands of
    // instruction I can be promoted.
    bool instructionOperandsPromotable(Loop *L, Instruction *I);

    // isAvailInPromoteList - return true if instruction I is found in
    // PromoteList.
    bool isAvailInPromoteList(Instruction *I);

    // Given a binary instructioin I, put operands 0 and 1 in W0 and W1
    // respectively.
    void PopulateBinaryOperands(Instruction *I, Loop *L, Value *&W0,
                                Value *&W1);

    // Insert a loop invariant sign-ext in the loop pre-header.
    Value *insertSignExtension(Value *V, Instruction *I, Loop *L,
                                   BasicBlock *E);

    // Check if given instruction I is present in the PromoteMap.
    Instruction *FindInPromoteMap(Instruction *I);

    // Replace all uses of I with J.
    void ReplaceAllUsesOfWith(Instruction *I, Instruction *J);
  };
}

char TypePromotion::ID = 0;
INITIALIZE_PASS_BEGIN(TypePromotion, "promote-type",
                      "type promotion", false,false)
INITIALIZE_PASS_DEPENDENCY(LoopInfo)
INITIALIZE_PASS_END(TypePromotion, "promote-type",
                    "type promotion", false, false)

Pass *llvm::createTypePromotionPass() {
  return new TypePromotion();
}

// instructionPromotable - return true if instruction I can be promoted safely.
bool TypePromotion::instructionPromotable(Instruction *I) {
  switch (I->getOpcode()) {
    default:
      break;
      // Simple binary operations are allowed.
    case Instruction::Add:
    case Instruction::Sub:
      return true;
    case Instruction::ICmp:
      // Only signed comparisons are allowed.
      if (ICmpInst *CmpI = dyn_cast<ICmpInst>(I))
        return CmpI->isSigned();
      return false;
    case Instruction::SExt:
      // Sign extensions are allowed.
      if (I->getType()->isIntegerTy(32))
        return true;
      break;
  }

  return false;
}

// isAvailInPromoteList - return true if instruction I is present in
// PromoteList.
bool TypePromotion::isAvailInPromoteList(Instruction *I) {
  std::list<Instruction*>::iterator Itr;
  Itr = std::find(PromoteList.begin(), PromoteList.end(), I);
  if (Itr != PromoteList.end())
    return true;

  return false;
}

// instructionOperandsPromotable - return true if all operands of instruction I
// can be safely promoted. For now, we handle the following cases:
//  1. operand is loop invariant.
//  2. operand is already promoted.
bool TypePromotion::instructionOperandsPromotable(Loop *L, Instruction *I) {
  // Process operands.
  for (unsigned i = 0; i < I->getNumOperands(); i++) {
    Value *V = I->getOperand(i);
    if (L->isLoopInvariant(V))
      continue;
    if (isAvailInPromoteList(dyn_cast<Instruction>(V)))
      continue;
    return false;
  }

  return true;
}

// dfsTypePromotion - do a depth-first-search starting from instruction I and
// figure out if the entire def-use graph can be promoted.
bool TypePromotion::dfsTypePromotion(Loop *L, Instruction *I) {
  // For root node, break the cycle.
  if (I != OriginalPhi) {
    // Instruction I is promotable?
    if (!instructionPromotable(I))
      return false;

    // Operands of I are promotable.
    if (!instructionOperandsPromotable(L, I))
      return false;

    // Insert I in PromoteList.
    PromoteList.push_back(I);

    // ICmp and SExt instructions are leaf nodes.
    if (I->getOpcode() == Instruction::SExt)
      return true;

    if (I->getOpcode() == Instruction::ICmp)
      return true;
  }

  // Recursively check all uses of I.
  for (Value::use_iterator UI = I->use_begin(), UE = I->use_end(); UI != UE;) {
    Use &TheUse = UI.getUse();
    ++UI;
    if (Instruction *J = dyn_cast<Instruction>(TheUse.getUser())) {
      if (J == OriginalPhi)
        continue;
      if (!dfsTypePromotion(L, J))
        return false;
    }
    else
      return false;
  }

  return true;
}

// FindInPromoteMap - return true if I is found in the map.
Instruction *TypePromotion::FindInPromoteMap(Instruction *I) {
  std::map<Instruction*,Instruction*>::iterator Itr = PromoteMap.find(I);
  return (Itr == PromoteMap.end()) ? 0 : (*Itr).second;
}

// Given an instruction I, get its operand 0 and operand 1. Handle the following
// cases -
// a. operand is a constant.
// b. operand is loop invariant.
// c. operand is sign-extended.
void TypePromotion::PopulateBinaryOperands(Instruction *I, Loop *L, Value *&W0,
                                           Value *&W1) {
  Value *V0 = I->getOperand(0);
  Value *V1 = I->getOperand(1);
  Instruction *I0 = dyn_cast<Instruction>(V0);
  Instruction *I1 = dyn_cast<Instruction>(V1);

  Function *F = I->getParent()->getParent();
  BasicBlock *Entry = &(F->getEntryBlock());

  // Value is constant.
  if (ConstantInt *C0 = dyn_cast<ConstantInt>(V0)) {
    W0 =
      ConstantInt::get(Type::getInt32Ty(I->getContext()), C0->getSExtValue());
  }
  else {
    // Try to find in the promote map.
    W0 = FindInPromoteMap(I0);

    // If not found, must be loop invariant.
    if (W0 == 0) {
      W0 = insertSignExtension(V0, I0, L, Entry);
    }
  }

  if (ConstantInt *C1 = dyn_cast<ConstantInt>(V1)) {
    W1 =
      ConstantInt::get(Type::getInt32Ty(I->getContext()), C1->getSExtValue());
  }
  else {
    // Try to find in the promote map.
    W1 = FindInPromoteMap(I1);

    // If not found, must be loop invariant.
    if (W1 == 0) {
      W1 = insertSignExtension(V1, I1, L, Entry);
    }
  }
}

// insertSignExtension - Insert sext loop invariant value.
Value *TypePromotion::insertSignExtension(Value *V, Instruction *I, Loop *L,
                                       BasicBlock *E) {
  CastInst *NewSExt =
    CastInst::CreateSExtOrBitCast(V, Type::getInt32Ty(E->getContext()),
                                  V->getName() + ".pmt");
  // Insert NewSExt correctly.
  if (I) {
    if (PHINode *PH = dyn_cast<PHINode>(I)) {
      NewSExt->insertBefore(PH->getParent()->getFirstNonPHI());
    }
    else {
      NewSExt->insertAfter(I);
    }
  }
  else {
    NewSExt->insertBefore(E->getTerminator());
  }

  return NewSExt;
}

// ReplaceAllUsesOfWith - Replace all uses of I with J.
void TypePromotion::ReplaceAllUsesOfWith(Instruction *I, Instruction *J) {
  for (Value::use_iterator UI = I->use_begin(), UE = I->use_end(); UI != UE;) {
    Use &TheUse = UI.getUse();
    ++UI;
    if (Instruction *K = dyn_cast<Instruction>(TheUse.getUser())) {
      K->replaceUsesOfWith(I, J);
    }
  }
}

// runOnLoop - analyze loops for redundant sign-extensions. We start with the
// loop index variable and recursively look at its uses to see if it can be
// legally promoted. If the loop index variable is signed and set to 0 in the
// loop preheader, we can safely promote it since overflow of signed number is
// undefined in C.
bool TypePromotion::runOnLoop(Loop *L, LPPassManager &LPM) {

  if (DisableTypePromotion)
    return false;

  if (!L->isLoopSimplifyForm())
    return false;

  bool FlagCleanup = false;
  BasicBlock *Header = L->getHeader();
  BasicBlock *Latch = L->getLoopLatch();
  assert(Header &&  Latch && "Loop is not well structured!");

  // Handle all PHI nodes in the Header.
  for (BasicBlock::iterator II = Header->begin(), EE = Header->end(); II != EE;
       ++II) {
    // Look for PHI nodes in the Header block.
    if (PHINode *PhiVar = dyn_cast<PHINode>(II)) {
      // Look for simple loops.
      if (PhiVar->getNumIncomingValues() != 2)
        continue;
      // Promote i16 types only.
      if (!PhiVar->getType()->isIntegerTy(16))
        continue;

      // Initialize.
      PromoteList.clear();
      PromoteMap.clear();
      PromoteList.push_back(PhiVar);
      OriginalPhi = PhiVar;

      // Now recursively walk the use-def chain to see if type promotion is
      // valid.
      if (dfsTypePromotion(L, PhiVar)) {
        if (PromoteList.size() < 2)
          continue;

        // Go through all elements in the list and promote to i32.
        for (std::list<Instruction*>::iterator Curr = PromoteList.begin(),
             End = PromoteList.end(); Curr != End; ++Curr) {
          // Find the next instruction.
          Instruction *I = *Curr;
          Value *Op0 = NULL;
          Value *Op1 = NULL;
          switch (I->getOpcode()) {
            default:
              assert(true && "Unexpected opcode!");
              break;
            case Instruction::PHI: {
              PHINode *NewPN =
                PHINode::Create(Type::getInt32Ty(I->getContext()), 2,
                                I->getName() + ".pmt", I);
              PromoteMap[I] = NewPN;
              break;
            }
            case Instruction::Add:
            case Instruction::Sub: {
              BinaryOperator *BI = dyn_cast<BinaryOperator>(I);
              PopulateBinaryOperands(BI, L, Op0, Op1);
              BinaryOperator *NewBop =
                BinaryOperator::Create(BI->getOpcode(), Op0, Op1,
                                       BI->getName() + ".pmt", BI);
              PromoteMap[BI] = NewBop;
              break;
            }
            case Instruction::ICmp: {
              CmpInst *CI = dyn_cast<CmpInst>(I);
              PopulateBinaryOperands(CI, L, Op0, Op1);
              CmpInst *NewCmp =
                CmpInst::Create(CI->getOpcode(), CI->getPredicate(), Op0, Op1,
                                CI->getName() + ".pmt", I);
              ReplaceAllUsesOfWith(CI, NewCmp);
              PromoteMap[CI] = NewCmp;
              break;
            }
            case Instruction::SExt: {
              assert(I->getType()->isIntegerTy(32));
              Instruction *J = dyn_cast<Instruction>(I->getOperand(0));
              Instruction *K = FindInPromoteMap(J);
              assert(K);
              ReplaceAllUsesOfWith(I, K);
              break;
            }
          } // End switch.
        } // For all instructions in PromoteList.

        // Fix the original phi node.
        Value *Op0 = NULL;
        Value *Op1 = NULL;
        PopulateBinaryOperands(OriginalPhi, L, Op0, Op1);
        assert(Op0 && Op1);
        PHINode *NewPN = (PHINode *)PromoteMap[OriginalPhi];
        BasicBlock *BB = OriginalPhi->getIncomingBlock(0);
        NewPN->addIncoming(Op0, BB);
        BB = OriginalPhi->getIncomingBlock(1);
        NewPN->addIncoming(Op1, BB);
        FlagCleanup = true;
      } // If recursively use-def type promotion.
    } // If PHI node.
  } // For all instructions in BB.

  if (FlagCleanup) {
    Function *F = Header->getParent();
    FunctionPass *DCE = llvm::createDeadCodeEliminationPass();
    DCE->runOnFunction(*F);

    // Try to clean up cycles.
    for (BasicBlock::iterator II = Header->begin(), EE = Header->end();
         II != EE;) {
      if (PHINode *PhiVar = dyn_cast<PHINode>(II++)) {
        if (PhiVar->getNumIncomingValues() != 2)
          continue;
        if (!PhiVar->hasOneUse())
          continue;
        User *Use = PhiVar->use_back();
        if (!Use->hasOneUse())
          continue;
        if (Instruction *I = dyn_cast<Instruction>(Use)) {
          User *UseUse = Use->use_back();
          if (UseUse == PhiVar) {
            PhiVar->removeIncomingValue((unsigned)0, false);
            PhiVar->removeIncomingValue((unsigned)0, false);
            I->eraseFromParent();
            PhiVar->eraseFromParent();
          }
        }
      }
    }
  }

  return true;
}
