// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
//


#include "llvm/Assembly/Writer.h"

#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/GlobalVariable.h"
#include "llvm/Pass.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Transforms/Scalar.h"

#include <map>
#include <queue>
#include <set>
#include <vector>

using namespace llvm;

static cl::opt<bool>
DisablePCLE("disable-pcle", cl::Hidden, cl::init(false),
      cl::desc("Disable Precomputing Loop Expressions"));

namespace {
  typedef int32_t Integer;
  #define BitSize(T) (8*sizeof(T))

  struct IVInfo {
    IVInfo() : L(0) {}
    IVInfo(Loop *Lp) : L(Lp) {}
    Integer Start, End, Bump;
    Loop *L;
  };

  typedef std::vector<Integer> IntVect;
  typedef std::map<Value*, IVInfo> IVInfoMap;
  typedef std::queue<Value*> ValueQueue;
  typedef std::set<Value*> ValueSet;
  typedef std::vector<Value*> ValueVect;


  class PrecomputeLoopExpressions : public FunctionPass {
  public:
    static char ID;
    static const unsigned MinCost = 8;

    PrecomputeLoopExpressions() : FunctionPass(ID) {
      initializePrecomputeLoopExpressionsPass(*PassRegistry::getPassRegistry());
    }

    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.addRequired<LoopInfo>();
      AU.addPreserved<LoopInfo>();
      AU.addRequired<ScalarEvolution>();
      AU.addPreserved<ScalarEvolution>();
    }

    virtual const char *getPassName() const {
      return "Precomputing Loop Expressions";
    }

    virtual bool runOnFunction(Function &Fn);

  private:
    bool isLoopValid(Loop *L);
    bool processLatchForIV(Instruction *TrIn, Value *&IV, IVInfo &IVI);
    bool processPHIForIV(Instruction *PIn, Value *IV, IVInfo &IVI);
    void collectInductionVariables();

    bool isAllowedOpcode(unsigned Opc);
    bool verifyExpressionNode(Value *Ex, ValueSet &Valid);
    bool verifyExpression(Value *Ex, ValueSet &Valid);
    void extendExpression(Value *Ex, ValueSet &Valid, ValueSet &New);
    unsigned computeExpressionCost(Value *V, ValueSet &Vs);
    void collectCandidateExpressions();

    void extractInductionVariables(Value *Ex, ValueVect &IVs);
    ArrayType *createTypeForArray(Type *ETy, ValueVect &IVs);
    Integer evaluateExpression(Value *Ex, ValueVect &IVs, IntVect &C);
    Constant *createInitializerForSlice (Value *Ex, unsigned Dim,
          ArrayType *ATy, ValueVect &IVs, bool Zero, IntVect &C,
          IntVect &Starts, IntVect &Ends, IntVect &Bumps);
    Constant *createInitializerForArray(Value *Ex, ArrayType *ATy,
          ValueVect &IVs);
    bool rewriteExpression (Value *Ex, ArrayType *ATy, ValueVect &IVs,
          GlobalVariable *GV);
    bool processCandidateExpressions();

    Function *F;
    LoopInfo *LI;
    ScalarEvolution *SE;

    IVInfoMap IVInfos;
    ValueSet IVEs;

    static unsigned Counter;
  };
}

char PrecomputeLoopExpressions::ID = 0;
unsigned PrecomputeLoopExpressions::Counter = 0;


INITIALIZE_PASS_BEGIN(PrecomputeLoopExpressions, "precompute-loop-expr",
                "Precomputing Loop Expressions", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfo)
INITIALIZE_PASS_DEPENDENCY(ScalarEvolution)
INITIALIZE_PASS_END(PrecomputeLoopExpressions, "precompute-loop-expr",
                "Precomputing Loop Expressions", false, false)


FunctionPass *llvm::createPrecomputeLoopExpressionsPass() {
  return new PrecomputeLoopExpressions();
}


bool PrecomputeLoopExpressions::isLoopValid(Loop *L) {
  BasicBlock *H = L->getHeader();
  if (!H) return false;
  BasicBlock *PH = L->getLoopPreheader();
  if (!PH) return false;
  BasicBlock *LT = L->getLoopLatch();
  if (!LT) return false;

  if (std::distance(pred_begin(H), pred_end(H)) != 2) return false;


  unsigned TC = SE->getSmallConstantTripCount(L, LT);
  if (TC == 0) return false;

  return true;
}


bool PrecomputeLoopExpressions::processLatchForIV(Instruction *TrIn,
      Value *&IV, IVInfo &IVI) {
  // Need a conditional branch.
  BranchInst *Br = dyn_cast<BranchInst>(TrIn);
  if (!Br || !Br->isConditional()) return false;

  // The branch condition needs to be an integer compare.
  Value *CV = Br->getCondition();
  Instruction *CIn = dyn_cast<Instruction>(CV);
  if (!CIn || CIn->getOpcode() != Instruction::ICmp) return false;

  // The comparison has to be less-than.
  ICmpInst *ICIn = cast<ICmpInst>(CIn);
  CmpInst::Predicate P = ICIn->getPredicate();
  if (P != CmpInst::ICMP_ULT && P != CmpInst::ICMP_SLT) return false;

  // Less-than a constant int to be exact.
  Value *CR = ICIn->getOperand(1);
  if (!isa<ConstantInt>(CR)) return false;

  // The int has to fit in 32 bits.
  const APInt &U = cast<ConstantInt>(CR)->getValue();
  if (!U.isSignedIntN(BitSize(Integer))) return false;

  // The value that is less-than the int needs to be an add.
  Value *VC = ICIn->getOperand(0);
  Instruction *VCIn = dyn_cast<Instruction>(VC);
  if (!VCIn || VCIn->getOpcode() != Instruction::Add) return false;

  // An add of a constant int.
  Value *ValA, *ValI;
  if (isa<ConstantInt>(VCIn->getOperand(1))) {
    ValA = VCIn->getOperand(0);
    ValI = VCIn->getOperand(1);
  } else {
    ValA = VCIn->getOperand(1);
    ValI = VCIn->getOperand(0);
  }
  if (!isa<ConstantInt>(ValI)) return false;

  // The added int has to fit in 32 bits.
  const APInt &B = cast<ConstantInt>(ValI)->getValue();
  if (!B.isSignedIntN(BitSize(Integer))) return false;


  // Done...
  IV = ValA;
  IVI.End = (Integer)U.getSExtValue();
  IVI.Bump = (Integer)B.getSExtValue();
  return true;
}


bool PrecomputeLoopExpressions::processPHIForIV(Instruction *PIn, Value *IV,
                                                IVInfo &IVI) {
  if (IV != PIn) return false;

  // The PHI must only have two incoming blocks.
  PHINode *P = cast<PHINode>(PIn);
  if (P->getNumIncomingValues() != 2) return false;

  // The blocks have to be preheader and loop latch.
  BasicBlock *PH = IVI.L->getLoopPreheader();
  BasicBlock *LT = IVI.L->getLoopLatch();

  if (P->getIncomingBlock(0) == PH) {
    if (P->getIncomingBlock(1) != LT) return false;
  } else if (P->getIncomingBlock(1) == PH) {
    if (P->getIncomingBlock(0) != LT) return false;
  } else {
    return false;
  }

  // The value coming from the preheader needs to be a constant int.
  Value *VPH = P->getIncomingValueForBlock(PH);
  if (!isa<ConstantInt>(VPH)) return false;

  // That int has to fit in 32 bits.
  const APInt &S = cast<ConstantInt>(VPH)->getValue();
  if (!S.isSignedIntN(BitSize(Integer))) return false;

  // All checks passed.
  IVI.Start = static_cast<Integer>(S.getSExtValue());
  return true;
}


void PrecomputeLoopExpressions::collectInductionVariables() {
  IVInfos.clear();

  typedef std::queue<Loop*> LoopQueue;
  LoopQueue Work;

  for (LoopInfo::iterator I = LI->begin(), E = LI->end(); I != E; ++I) {
    Work.push(*I);
  }

  while (!Work.empty()) {
    Loop *L = Work.front();
    Work.pop();

    for (Loop::iterator I = L->begin(), E = L->end(); I != E; ++I) {
      Work.push(*I);
    }
    if (!isLoopValid(L)) continue;

    Value *IV;
    IVInfo IVI(L);
    Instruction *TrIn = L->getLoopLatch()->getTerminator();

    bool LatchOk = processLatchForIV(TrIn, IV, IVI);
    if (!LatchOk) continue;

    BasicBlock *H = L->getHeader();
    for (BasicBlock::iterator PI = H->begin(); isa<PHINode>(PI); ++PI) {
      if (PI == IV) {
        bool PHIOk = processPHIForIV(PI, IV, IVI);
        if (PHIOk) {
          IVInfos.insert(std::make_pair(IV, IVI));
        }
        break;
      }
    }

  }
}


bool PrecomputeLoopExpressions::isAllowedOpcode(unsigned Opc) {
  switch (Opc) {
    case Instruction::Add:
    case Instruction::Sub:
    case Instruction::Mul:
    case Instruction::UDiv:
    case Instruction::SDiv:
    case Instruction::URem:
    case Instruction::SRem:
    case Instruction::Shl:
    case Instruction::AShr:
    case Instruction::And:
    case Instruction::Or:
    case Instruction::Xor:
      return true;
  }
  return false;
}


bool PrecomputeLoopExpressions::verifyExpressionNode(Value *Ex,
      ValueSet &Valid) {
  Type *T = Ex->getType();
  if (!T->isIntegerTy()) return false;
  if (cast<IntegerType>(T)->getBitWidth() > BitSize(Integer)) return false;

  Instruction *In = dyn_cast<Instruction>(Ex);
  if (!In) return false;
  if (!isAllowedOpcode(In->getOpcode())) return false;

  return true;
}

bool PrecomputeLoopExpressions::verifyExpression(Value *Ex, ValueSet &Valid) {
  if (Valid.count(Ex)) return true;
  if (isa<ConstantInt>(Ex)) return true;

  if (!verifyExpressionNode(Ex, Valid)) return false;

  assert (isa<Instruction>(Ex) && "Should have checked for instruction");
  Instruction *In = cast<Instruction>(Ex);
  for (unsigned i = 0, n = In->getNumOperands(); i < n; ++i) {
    bool ValidOp = verifyExpression(In->getOperand(i), Valid);
    if (!ValidOp) return false;
  }
  return true;
}


void PrecomputeLoopExpressions::extendExpression(Value *Ex, ValueSet &Valid,
                                                 ValueSet &New) {
  for (Value::use_iterator I = Ex->use_begin(), E = Ex->use_end();
       I != E; ++I) {
    Value *U = *I;
    if (Valid.count(U)) continue;
    if (U->getType()->isVoidTy()) continue;

    bool BadUser = false;

    if (Instruction *In = dyn_cast<Instruction>(U)) {
      if (In->getOpcode() == Instruction::PHI) continue;
      if (!verifyExpressionNode(U, Valid)) continue;

      for (unsigned i = 0, n = In->getNumOperands(); i < n; ++i) {
        Value *Op = In->getOperand(i);
        if (Op != Ex && !verifyExpression(Op, Valid)) {
          BadUser = true;
          break;
        }
      }
    } else {
      BadUser = true;
    }
    if (BadUser) continue;

    New.insert(U);
  }
}


unsigned PrecomputeLoopExpressions::computeExpressionCost(Value *V,
                                                          ValueSet &Vs) {
  if (Vs.count(V)) return 0;
  Vs.insert(V);

  unsigned C = 0;
  if (Instruction *In = dyn_cast<Instruction>(V)) {
    switch (In->getOpcode()) {
      case Instruction::Add:
      case Instruction::Sub:
      case Instruction::Shl:
      case Instruction::AShr:
      case Instruction::And:
      case Instruction::Or:
      case Instruction::Xor:
        C = 1;
        break;
      case Instruction::Mul:
      case Instruction::UDiv:
      case Instruction::SDiv:
      case Instruction::URem:
      case Instruction::SRem:
        C = 3;
        break;
      case Instruction::PHI:
        return 0;
    }

    for (unsigned i = 0, n = In->getNumOperands(); i < n; ++i) {
      C += computeExpressionCost(In->getOperand(i), Vs);
    }
  }

  return C;
}


void PrecomputeLoopExpressions::collectCandidateExpressions() {
  ValueQueue Work;

  IVEs.clear();

  for (IVInfoMap::iterator I = IVInfos.begin(), E = IVInfos.end();
       I != E; ++I) {
    IVEs.insert(I->first);
    Work.push(I->first);
  }

  ValueSet NewIVEs;
  while (!Work.empty()) {
    Value *V = Work.front();
    Work.pop();
    NewIVEs.clear();

    extendExpression(V, IVEs, NewIVEs);
    for (ValueSet::iterator I = NewIVEs.begin(), E = NewIVEs.end();
         I != E; ++I) {
      IVEs.insert(*I);
      Work.push(*I);
    }
  }

  // Remove IV expressions that are always subexpressions of another
  // IV expression.
  for (ValueSet::iterator I = IVEs.begin(), E = IVEs.end(); I != E; ) {
    ValueSet::iterator ThisI = I++;
    Value *V = *ThisI;

    bool Covered = true;
    for (Value::use_iterator UI = V->use_begin(), UE = V->use_end();
         UI != UE; ++UI) {
      Value *U = *UI;
      if (!IVEs.count(U)) {
        Covered = false;
        break;
      }
    }

    if (Covered) IVEs.erase(V);
  }

  // Remove IV expressions that are not complex enough.
  ValueSet Tmp;
  for (ValueSet::iterator I = IVEs.begin(), E = IVEs.end(); I != E; ) {
    ValueSet::iterator ThisI = I++;
    Value *V = *ThisI;

    Tmp.clear();
    unsigned C = computeExpressionCost(V, Tmp);
    if (C < MinCost) IVEs.erase(V);
  }

}


void PrecomputeLoopExpressions::extractInductionVariables(Value *Ex,
                                                          ValueVect &IVs) {
  ValueQueue Work;
  Work.push(Ex);

  ValueSet Memo;
  Memo.insert(Ex);

  while (!Work.empty()) {
    Value *V = Work.front();
    Work.pop();

    if (IVInfos.count(V)) {
      IVs.push_back(V);
    } else if (Instruction *In = dyn_cast<Instruction>(V)) {
      for (unsigned i = 0, n = In->getNumOperands(); i < n; ++i) {
        Value *Op = In->getOperand(i);
        if (!Memo.count(Op)) {
          Memo.insert(Op);
          Work.push(Op);
        }
      }
    }
  }
}


ArrayType *PrecomputeLoopExpressions::createTypeForArray(Type *ETy,
      ValueVect &IVs) {
  ArrayType *ATy = 0;

  for (ValueVect::iterator I = IVs.begin(), E = IVs.end(); I != E; ++I) {
    Value *V = *I;
    IVInfo &IVI = IVInfos[V];
    assert ((IVI.Start < IVI.End) && "Backward loop?");

    if (ATy) {
      ATy = ArrayType::get (ATy, IVI.End-IVI.Start);
    } else {
      ATy = ArrayType::get (ETy, IVI.End-IVI.Start);
    }
  }

  return ATy;
}


Integer PrecomputeLoopExpressions::evaluateExpression(
      Value *Ex, ValueVect &IVs, IntVect &C) {

  if (ConstantInt *CI = dyn_cast<ConstantInt>(Ex)) {
    const APInt &A = CI->getValue();
    return A.getSExtValue();
  }

  if (Instruction *In = dyn_cast<Instruction>(Ex)) {
    switch (In->getOpcode()) {
      case Instruction::Add:
        return evaluateExpression(In->getOperand(0), IVs, C) +
               evaluateExpression(In->getOperand(1), IVs, C);
      case Instruction::Sub:
        return evaluateExpression(In->getOperand(0), IVs, C) -
               evaluateExpression(In->getOperand(1), IVs, C);
      case Instruction::Shl:
        return evaluateExpression(In->getOperand(0), IVs, C) <<
               evaluateExpression(In->getOperand(1), IVs, C);
      case Instruction::AShr:
        return evaluateExpression(In->getOperand(0), IVs, C) >>
               evaluateExpression(In->getOperand(1), IVs, C);
      case Instruction::Mul:
        return evaluateExpression(In->getOperand(0), IVs, C) *
               evaluateExpression(In->getOperand(1), IVs, C);
      case Instruction::UDiv:
      case Instruction::SDiv:
        return evaluateExpression(In->getOperand(0), IVs, C) /
               evaluateExpression(In->getOperand(1), IVs, C);
      case Instruction::URem:
      case Instruction::SRem:
        return evaluateExpression(In->getOperand(0), IVs, C) %
               evaluateExpression(In->getOperand(1), IVs, C);
      case Instruction::And:
        return evaluateExpression(In->getOperand(0), IVs, C) &
               evaluateExpression(In->getOperand(1), IVs, C);
      case Instruction::Or:
        return evaluateExpression(In->getOperand(0), IVs, C) |
               evaluateExpression(In->getOperand(1), IVs, C);
      case Instruction::Xor:
        return evaluateExpression(In->getOperand(0), IVs, C) ^
               evaluateExpression(In->getOperand(1), IVs, C);
      default:
        break;
    }
  }

  for (unsigned i = 0, n = IVs.size(); i < n; ++i) {
    if (Ex == IVs[i]) return C[i];
  }

  dbgs() << *Ex << "\n";
  llvm_unreachable ("Unexpected expression");
  return 0;
}


#if 0
static bool bump(IntVect &C, IntVect &Starts, IntVect &Ends, IntVect &Bumps) {
  unsigned N = C.size();
  unsigned i = 0;
  bool BumpNext;

  do {
    Integer I = C[i] + Bumps[i];
    if (I >= Ends[i]) {
      C[i] = Starts[i];
      BumpNext = true;
    } else {
      C[i] = I;
      BumpNext = false;
    }
    i++;
  } while (BumpNext || i >= N);

  return BumpNext;
}
#endif



Constant *PrecomputeLoopExpressions::createInitializerForSlice (
      Value *Ex, unsigned Dim, ArrayType *ATy, ValueVect &IVs, bool Zero,
      IntVect &C, IntVect &Starts, IntVect &Ends, IntVect &Bumps) {
  Integer S = Starts[Dim];
  Integer E = Ends[Dim]; 
  Integer B = Bumps[Dim];
  std::vector<Constant*> Init(E-S);

  Type *ETy = ATy->getElementType();

  for (unsigned i = 0; i <= Dim; ++i) {
    C[i] = Starts[i];
  }

  if (Dim > 0) {
    assert (ETy->isArrayTy() && "Expecting array type");
    ArrayType *AETy = cast<ArrayType>(ETy);

    Integer i = S;
    while (i < E) {
      Init[i-S] = createInitializerForSlice (Ex, Dim-1, AETy, IVs, Zero,
                                             C, Starts, Ends, Bumps);
      i++;
      for (Integer j = 0; j < B-1; ++j) {
        if (i >= E) break;
        Init[i-S] = createInitializerForSlice (Ex, Dim-1, AETy, IVs, true,
                                               C, Starts, Ends, Bumps);
        i++;
      }
      C[Dim] += B;
    }
  } else {
    assert (ETy->isIntegerTy() && "Expecting integer type");
    IntegerType *IETy = cast<IntegerType>(ETy);

    Integer i = S;
    while (i < E) {
      Integer A = Zero ? 0 : evaluateExpression(Ex, IVs, C);
      Init[i-S] = ConstantInt::getSigned(IETy, A);
      i++;
      for (Integer j = 0; j < B-1; ++j) {
        if (i >= E) break;
        Init[i-S] = ConstantInt::getSigned(IETy, 0);
        i++;
      }
      C[Dim] += B;
    }
  }

  ArrayRef<Constant*> AR(Init);
  return ConstantArray::get(ATy, AR);
}


Constant *PrecomputeLoopExpressions::createInitializerForArray(
      Value *Ex, ArrayType *ATy, ValueVect &IVs) {
  unsigned Dims = IVs.size();
  IntVect C(Dims), Starts(Dims), Ends(Dims), Bumps(Dims);

  for (unsigned i = 0; i < Dims; ++i) {
    IVInfo &IVI = IVInfos[IVs[i]];
    Starts[i] = IVI.Start;
    Ends[i] = IVI.End;
    Bumps[i] = IVI.Bump;
  }

  return createInitializerForSlice(Ex, Dims-1, ATy, IVs, false,
                                   C, Starts, Ends, Bumps);
}



bool PrecomputeLoopExpressions::rewriteExpression (Value *Ex, ArrayType *ATy,
      ValueVect &IVs, GlobalVariable *GV) {
  unsigned Dims = IVs.size();

  Instruction *In = dyn_cast<Instruction>(Ex);
  assert(In && "Expecting instruction");
  IRBuilder<> Builder(In);

  ValueVect Ops(Dims+1);

  Ops[0] = ConstantInt::get (IVs[0]->getType(), 0);

  for (unsigned i = 0; i < Dims; ++i) {
    unsigned IVx = Dims-i-1;
    IVInfo &IVI = IVInfos[IVs[IVx]];
    if (IVI.Start != 0) {
      Value *StartV = ConstantInt::get(Ex->getType(), IVI.Start);
      Ops[i+1] = Builder.CreateSub (IVs[i], StartV);
    } else {
      Ops[i+1] = IVs[IVx];
    }
  }

  ArrayRef<Value*> Idx(Ops);
  Value *GEP = Builder.CreateGEP (GV, Idx, "txgep");
  Value *Load = Builder.CreateLoad (GEP, "txld");
  Ex->replaceAllUsesWith(Load);
  return true;
}


struct LoopCompare {
  LoopCompare (IVInfoMap &M) : IVMap(M) {}
  bool operator() (Value *V, Value *W) {
    Loop *LV = IVMap[V].L;
    Loop *LW = IVMap[W].L;
    return LW->contains(LV);
  }

private:
  IVInfoMap &IVMap;
};


static bool IsSubexpression(Value *A, Value *B) {
  if (A == B) return true;
  if (!isa<Instruction>(B)) return false;

  Instruction *InB = cast<Instruction>(B);

  if (InB->getOpcode() == Instruction::PHI) return false;

  for (unsigned i = 0, n = InB->getNumOperands(); i < n; ++i) {
    if (A == InB->getOperand(i)) return true;
  }
  for (unsigned i = 0, n = InB->getNumOperands(); i < n; ++i) {
    if (IsSubexpression(A, InB->getOperand(i))) return true;
  }
  return false;
}


static bool ExprLessThan(Value *A, Value *B) {
  return (A == B) ? false : IsSubexpression(A, B);
}


bool PrecomputeLoopExpressions::processCandidateExpressions() {
  ValueVect IVs;
  LoopCompare LC(IVInfos);
  bool Changed = false;

  while (!IVEs.empty()) {
    ValueSet::iterator M = std::max_element(IVEs.begin(), IVEs.end(),
                                            ExprLessThan);
    Value *Ex = *M;

    IVs.clear();
    extractInductionVariables(Ex, IVs);
    // Sort from innermost to outermost.
    std::sort (IVs.begin(), IVs.end(), LC);

    ArrayType *ATy = createTypeForArray(Ex->getType(), IVs);

    GlobalVariable *GV = new GlobalVariable(*F->getParent(), ATy, true,
                                            GlobalValue::PrivateLinkage,
                                            0, Twine("tx")+Twine(Counter));
    Constant *Init = createInitializerForArray(Ex, ATy, IVs);
    GV->setInitializer(Init);

    Changed |= rewriteExpression (Ex, ATy, IVs, GV);

    IVEs.erase(Ex);
  }

  return Changed;
}


bool PrecomputeLoopExpressions::runOnFunction(Function &Fn) {
  if (DisablePCLE) return false;
//  dbgs() << "Before PCLE\n";
//  dbgs() << static_cast<Value&>(Fn);
  F = &Fn;
  LI = &getAnalysis<LoopInfo>();
  SE = &getAnalysis<ScalarEvolution>();

  collectInductionVariables();
  if (IVInfos.empty()) return false;

  collectCandidateExpressions();
  if (IVEs.empty()) return false;

  bool Changed = processCandidateExpressions();

//  dbgs() << "After PCLE\n";
//  dbgs() << static_cast<Value&>(Fn);
  return Changed;
}

