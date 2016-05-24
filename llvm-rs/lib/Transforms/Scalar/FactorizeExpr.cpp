// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//                     The LLVM Compiler Infrastructure
//

#include "llvm/Constants.h"
#include "llvm/Function.h"
#include "llvm/Pass.h"
#include "llvm/Type.h"

#include "llvm/ADT/APInt.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"

#include <map>
#include <queue>
#include <set>
#include <vector>

using namespace llvm;

namespace {
  typedef std::set<Instruction*> InstSet;
  typedef std::vector<Instruction*> InstVect;
  typedef std::set<Value*> ValueSet;
  typedef std::set<unsigned> UIntSet;
  typedef std::map<Value*, unsigned> ValueCountMap;

  struct Addend {
    Addend(Value *V, bool N = false) : Val(V), Neg(N) {}
    Value *Val;
    bool Neg;
  };

  typedef std::vector<Addend> AddendVect;


  class FactorizeExpressions : public FunctionPass {
  public:
    static char ID;

    FactorizeExpressions() : FunctionPass(ID) {
      initializeFactorizeExpressionsPass(*PassRegistry::getPassRegistry());
    }

    virtual const char *getPassName() const {
      return "Factorizing Expressions";
    }

    virtual bool runOnFunction(Function &F);

  private:
    bool visitBlock(BasicBlock *B);

//    void collectLargestSubtrees(BasicBlock *B, UIntSet &IOs, ValueSet &Trees);
    void collectInterestingRoots(Value *Top, UIntSet &IOs, ValueSet &Roots);
    void skipInterestingNodes(Value *N, UIntSet &IOs, ValueSet &Nodes);

    bool factorizeAddSub (InstSet &TopTrees, BasicBlock *B);
    bool factorizeAddSubExpression (Value *Node);
    Value *createSumFromAddends(AddendVect &Addends, Value *Factor,
                                IRBuilder<> &BL);
    Value *removeFactorFromMul(Value *M, Value *F);
    void propagateNegationUp(bool &Neg, Value *&Val);

    void simplifyBlock(BasicBlock *B);
    void removeDeadCode(BasicBlock *B);
  };

}

char FactorizeExpressions::ID = 0;


INITIALIZE_PASS_BEGIN(FactorizeExpressions, "factorize-expr",
                "Factorizing Expressions", false, false)
//INITIALIZE_PASS_DEPENDENCY(DominatorTree)
INITIALIZE_PASS_END(FactorizeExpressions, "factorize-expr",
                "Factorizing Expression", false, false)

FunctionPass *llvm::createFactorizeExpressionsPass() {
  return new FactorizeExpressions();
}



void FactorizeExpressions::collectInterestingRoots(Value *Top,
                                                   UIntSet &IOs,
                                                   ValueSet &Roots) {
  if (Instruction *In = dyn_cast<Instruction>(Top)) {
    unsigned Opc = In->getOpcode();
    if (Opc == Instruction::PHI) return;

    if (IOs.count(Opc)) {
      Roots.insert(Top);
      for (unsigned i = 0, n = In->getNumOperands(); i < n; ++i) {
        skipInterestingNodes(In->getOperand(i), IOs, Roots);
      }
    } else {
      for (unsigned i = 0, n = In->getNumOperands(); i < n; ++i) {
        collectInterestingRoots(In->getOperand(i), IOs, Roots);
      }
    }
  }
}

void FactorizeExpressions::skipInterestingNodes(Value *N,
                                                UIntSet &IOs,
                                                ValueSet &Nodes) {
  if (Instruction *In = dyn_cast<Instruction>(N)) {
    unsigned Opc = In->getOpcode();
    if (Opc == Instruction::PHI) return;

    if (IOs.count(Opc)) {
      for (unsigned i = 0, n = In->getNumOperands(); i < n; ++i) {
        skipInterestingNodes(In->getOperand(i), IOs, Nodes);
      }
    } else {
      collectInterestingRoots(N, IOs, Nodes);
    }
  }
}


bool FactorizeExpressions::factorizeAddSub (InstSet &TopTrees, BasicBlock *B) {
  UIntSet InterestingOpcodes;
  InterestingOpcodes.insert (Instruction::Add);
  InterestingOpcodes.insert (Instruction::Sub);

  ValueSet Roots;
  bool Changed = false;

  for (InstSet::iterator I = TopTrees.begin(), E = TopTrees.end();
       I != E; ++I) {
    Roots.clear();
    collectInterestingRoots (*I, InterestingOpcodes, Roots);

    for (ValueSet::iterator J = Roots.begin(), F = Roots.end(); J != F; ++J) {
      Changed |= factorizeAddSubExpression(*J);
    }
  }

  return Changed;
}


struct MulWithFactor {
  MulWithFactor (Value *F) : Fact(F) {}
  bool operator() (const Addend &A) {
    if (Instruction *In = dyn_cast<Instruction>(A.Val)) {
      if (In->getOpcode() == Instruction::Mul) {
        for (unsigned i = 0, n = In->getNumOperands(); i < n; ++i) {
          if (In->getOperand(i) == Fact) return true;
        }
      }
    }
    return false;
  }
private:
  Value *Fact;
};


struct MCItCmp {
  bool operator() (const ValueCountMap::iterator &I1,
                   const ValueCountMap::iterator &I2) {
    // Descending order of counts.
    if (I1->second > I2->second) return true;
    if (I1->second < I2->second) return false;

    // If counts are equal, prefer instructions to other values.
    if (isa<Instruction>(I1->first)) return true;
    if (isa<Instruction>(I2->first)) return false;

    // Tie-breaker.  Here the order doesn't really matter, but need to
    // return consistent answers.
    uintptr_t P1 = reinterpret_cast<uintptr_t>(I1->first);
    uintptr_t P2 = reinterpret_cast<uintptr_t>(I2->first);
    return P1 > P2;
  }
};


bool FactorizeExpressions::factorizeAddSubExpression (Value *Node) {
  Instruction *RootIn = dyn_cast<Instruction>(Node);
  assert (RootIn && "Expecting instruction");

  AddendVect Addends;

  std::queue<Addend> Work;
  Work.push(Addend(RootIn));

  while (!Work.empty()) {
    Addend A = Work.front();
    Work.pop();

    if (Instruction *In = dyn_cast<Instruction>(A.Val)) {
      unsigned nop = In->getNumOperands();

      switch (In->getOpcode()) {
        case Instruction::Add:
          for (unsigned i = 0; i < nop; ++i) {
            Work.push (Addend(In->getOperand(i), A.Neg));
          }
          break;
        case Instruction::Sub:
          Work.push (Addend(In->getOperand(0), A.Neg));
          for (unsigned i = 1; i < nop; ++i) {
            Work.push (Addend(In->getOperand(i), !A.Neg));
          }
          break;
        default:
          Addends.push_back(A);
          break;
      }
    } else {
      Addends.push_back(A);
    }
  } // while Work


#if 1
  dbgs() << __func__ << ": breakdown of node: " << *Node << "\n";
  for (AddendVect::iterator I = Addends.begin(), E = Addends.end();
       I != E; ++I) {
    dbgs() << "  " << (I->Neg ? "-" : " ") << *I->Val << "\n";
  }
#endif


  ValueCountMap MCs;  // Multiplier counts.
  bool AllUnique = true;

  for (AddendVect::iterator I = Addends.begin(), E = Addends.end();
       I != E; ++I) {
    if (Instruction *In = dyn_cast<Instruction>(I->Val)) {
      if (In->getOpcode() == Instruction::Mul) {
        // Avoid duplicates coming from the same instruction.
        ValueSet Vs;
        for (unsigned i = 0, n = In->getNumOperands(); i < n; ++i) {
          Vs.insert(In->getOperand(i));
        }
        // Insert/bump count for each unique operand.
        for (ValueSet::iterator VI = Vs.begin(), VE = Vs.end();
             VI != VE; ++VI) {
          ValueCountMap::iterator FindIt = MCs.find(*VI);
          unsigned C = (FindIt != MCs.end()) ? FindIt->second : 0;
          if (C) {
            MCs.erase(FindIt);
            AllUnique = false;
          }
          MCs.insert(std::make_pair(*VI, C+1));
        } // for unique multiplier
      } // if mul
    } // if instruction
  } // for addend

//  dbgs() << "all unique? " << (AllUnique ? "yes" : "no") << "\n";
  if (AllUnique) return false;


  // Traverse the count map in the order of decreasing counts.
  std::vector<ValueCountMap::iterator> ItList;
  for (ValueCountMap::iterator I = MCs.begin(), E = MCs.end(); I != E; ++I) {
    ItList.push_back(I);
  }
  std::sort(ItList.begin(), ItList.end(), MCItCmp());

  typedef std::vector<ValueCountMap::iterator>::iterator Iter_Iterator;
#if 1
  for (Iter_Iterator I = ItList.begin(), E = ItList.end(); I != E; ++I) {
    ValueCountMap::iterator J = *I;
    dbgs() << "  count:" << J->second << "  " << *J->first << "\n";
  }
#endif

  AddendVect Factorized;
  // XXX
  IRBuilder<> BL(dyn_cast<Instruction>(Node));

  for (Iter_Iterator I = ItList.begin(), E = ItList.end(); I != E; ++I) {
    ValueCountMap::iterator J = *I;
    if (J->second < 2) break;  // It will be less than 2 from now on.

    Value *M = J->first;
    AddendVect::iterator AI = Addends.begin();
    AddendVect AMs;

    // Collect all addend multiplications that use M as a factor.
    for (unsigned i = 0; i < J->second; ++i) {
      AI = std::find_if(AI, Addends.end(), MulWithFactor(M));
      if (AI == Addends.end()) break;  // Addends can share factors.
      AMs.push_back(*AI);
      Addends.erase(AI);
      ++AI;
    }


    if (AMs.empty()) continue;

    assert (M == J->first && "Unexpected factor");

    Value *Sum = createSumFromAddends(AMs, M, BL);
    Value *Mul = BL.CreateMul (M, Sum);
    if (Instruction *In = dyn_cast<Instruction>(Mul)) {
      Value *Simple = SimplifyInstruction(In);
      if (Simple) Mul = Simple;
    }
#if 0
    dbgs() << "factor: " << *M << "  count=" << J->second << "\n";
    for (AddendVect::iterator BI = AMs.begin(), BE = AMs.end();
         BI != BE; ++BI) {
      dbgs() << "    " << (BI->Neg ? "-" : " ") << *BI->Val << "\n";
    }
    dbgs() << "  created mul: " << *Mul << "\n";
#endif

    bool isZero = false;
    if (Constant *C = dyn_cast<Constant>(Mul)) {
      isZero = C->isNullValue();
    }
    if (!isZero) {
      Factorized.push_back(Addend(Mul, false));
    }
  }

  // Append the rest of the addends (the unfactorized ones).
  for (AddendVect::iterator I = Addends.begin(), E = Addends.end();
       I != E; ++I) {
    Factorized.push_back(*I);
  }

  Value *New = createSumFromAddends(Factorized, 0, BL);
  Node->replaceAllUsesWith(New);

  return true;
}
 

Value *FactorizeExpressions::createSumFromAddends(AddendVect &Addends,
                                                  Value *Factor,
                                                  IRBuilder<> &BL) {
  std::queue<Addend> Work;

  // Copy to the work queue.
  for (AddendVect::iterator I = Addends.begin(), E = Addends.end();
       I != E; ++I) {
    Addend A = *I;
    if (Instruction *In = dyn_cast<Instruction>(A.Val)) {
      Value *Op0 = In->getOperand(0);
      Value *Op1 = In->getOperand(1);
      propagateNegationUp(A.Neg, Op0);
      propagateNegationUp(A.Neg, Op1);
      A.Val = BL.CreateMul(Op0, Op1);
    }
    propagateNegationUp(A.Neg, A.Val);
    Work.push(A);
  }

  Value *VA, *VB, *NewV;

  while (Work.size() > 1) {
    Addend A = Work.front();
    Work.pop();
    Addend B = Work.front();
    Work.pop();

    if (Factor) {
      VA = removeFactorFromMul(A.Val, Factor);
      VB = removeFactorFromMul(B.Val, Factor);
    } else {
      VA = A.Val;
      VB = B.Val;
    }

    bool ANeg = A.Neg;
    bool BNeg = B.Neg;
    propagateNegationUp (ANeg, VA);
    propagateNegationUp (BNeg, VB);

    if (ANeg ^ BNeg) {
      // Different negations.
      if (ANeg) NewV = BL.CreateSub(VB, VA);
      else      NewV = BL.CreateSub(VA, VB);
      Work.push (Addend(NewV, false));
    } else {
      // Same negations.
      NewV = BL.CreateAdd(VA, VB);
      Work.push (Addend(NewV, ANeg));
    }
  }

  Value *Final;

  if (Work.front().Neg) {
    // Need to negate the result.  Create "0 - val".
    Value *V = Work.front().Val;
    Type *T = V->getType();
    if (T->isIntegerTy()) {
      Final = BL.CreateSub(ConstantInt::get(T, 0), V);
    } else {
      llvm_unreachable ("Unexpected type");
    }
  } else {
    Final = Work.front().Val;
  }

  return Final;
}


void FactorizeExpressions::propagateNegationUp(bool &Neg, Value *&Val) {
  if (isa<ConstantInt>(Val)) {
    ConstantInt *CI = cast<ConstantInt>(Val);
    APInt V = CI->getValue();
    if (V.isNegative()) {
      Val = ConstantInt::get(Val->getType(), -V);
      Neg = !Neg;
    }
  }
}


Value *FactorizeExpressions::removeFactorFromMul(Value *M, Value *F) {
  Instruction *In = dyn_cast<Instruction>(M);
  assert (In && "Expecting instruction");

  // XXX assumes 2-ary
  return (In->getOperand(0) == F) ? In->getOperand(1) : In->getOperand(0);
}


bool FactorizeExpressions::visitBlock(BasicBlock *B) {
  InstSet TopTrees;

  for (BasicBlock::iterator I = B->begin(), E = B->end(); I != E; ++I) {
    for (User::op_iterator J = I->op_begin(), F = I->op_end(); J != F; ++J) {
      if (Instruction *OpI = dyn_cast<Instruction>(J)) {
        if (TopTrees.count(OpI)) TopTrees.erase(OpI);
      }
    }
    TopTrees.insert(I);
  }


#if 0
  dbgs() << "Block " << B->getName() << "\n";
  for (InstSet::iterator I = TopTrees.begin(), E = TopTrees.end();
       I != E; ++I) {
    dbgs() << **I << "\n";
  }
#endif
  bool Changed = false;

  Changed |= factorizeAddSub(TopTrees, B);

  if (Changed) {
    simplifyBlock(B);
    removeDeadCode(B);
  }

  return Changed;
}


void FactorizeExpressions::simplifyBlock(BasicBlock *B) {
  for (BasicBlock::iterator I = B->begin(), E = B->end(); I != E; ++I) {
    Instruction *In = &*I;
    if (!In->getType()->isVoidTy()) {
      Value *Simple = SimplifyInstruction(In);
      if (Simple) In->replaceAllUsesWith(Simple);
    }
  }
}


void FactorizeExpressions::removeDeadCode(BasicBlock *B) {
  InstVect Is;
  for (BasicBlock::iterator I = B->begin(), E = B->end(); I != E; ++I) {
    Is.push_back(&*I);
  }
  std::reverse(Is.begin(), Is.end());

  for (InstVect::iterator I = Is.begin(), E = Is.end(); I != E; ++I) {
    Instruction *In = *I;
    if (In->getType()->isVoidTy()) continue;
    if (In->getNumUses() == 0) In->eraseFromParent();
  }
}


bool FactorizeExpressions::runOnFunction(Function &F) {
  bool Changed = false;

  return false;

  for (Function::iterator I = F.begin(), E = F.end(); I != E; ++I) {
    Changed |= visitBlock(&*I);
  }

  return Changed;
}


