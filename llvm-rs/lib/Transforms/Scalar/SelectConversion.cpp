// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
//

#define DEBUG_TYPE "selconv"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Function.h"
#include "llvm/GlobalVariable.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Support/NoFolder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/STLExtras.h"


#include <map>
#include <queue>
#include <set>
#include <vector>

#include "llvm/Support/Debug.h"

using namespace llvm;

static cl::opt<bool>
DisableSelConv("disable-sel-conv", cl::Hidden, cl::init(false),
    cl::desc("Disable Select Conversion"));


namespace {
  typedef std::pair<const BasicBlock*, const BasicBlock*> FlowGraphEdge;
  typedef std::set<Value*> ValueSet;
  typedef std::pair<const Value*, const Value*> ValuePair;
  typedef std::set<Instruction*> InstSet;
  typedef std::vector<Instruction*> InstVect;

  class SelectConversion : public FunctionPass {
  private:
    enum SplitKind { Triangle, Diamond };
    struct SplitInfo {
      BasicBlock *BB;
      SplitKind Kind;
      unsigned Bx;

      SplitInfo(BasicBlock *B, SplitKind K, unsigned X)
        : BB(B), Kind(K), Bx(X) {}
      SplitInfo(BasicBlock *B, SplitKind K)
        : BB(B), Kind(K), Bx(0) {}
    };
    typedef std::vector<SplitInfo> SplitVect;

  public:
    static char ID;

    SelectConversion() : FunctionPass(ID) {
      initializeSelectConversionPass(*PassRegistry::getPassRegistry());
    }

    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.addRequired<AliasAnalysis>();
      AU.addRequired<DominatorTree>();

      AU.addPreserved<AliasAnalysis>();
      AU.addPreserved<DominatorTree>();
    }

    virtual const char *getPassName() const {
      return "Selection Conversion";
    }

    virtual bool runOnFunction(Function &F);

  private:
    typedef std::set<FlowGraphEdge> FlowGraphEdgeSet;
    typedef std::vector<BasicBlock*> BlockVect;

    void collectSplitBlocks (SplitVect &SS);
    bool convertSplit (SplitInfo &S);
    bool isSpeculationCandidate(BasicBlock *BB);
    void expandDependencies(InstSet &Is, BasicBlock *BB);
    bool coversWholeBlock(InstSet &Is, BasicBlock *BB);
    bool addressMatchFuzzy(const Value *P, const Value *Q);
    bool pointersCoveredBy(const ValueSet &A, const ValueSet &B);
    void hoistInstructions(Instruction *Start, Instruction *End,
                           Instruction *Where);
    bool isDefinedInBlock(Value *V, BasicBlock *BB);
    void collectLoadStoreInfo(BasicBlock *BB, InstSet *Stores, ValueSet *Ptrs);

    // KP: hack
    void collectStoreOrder(BasicBlock *B, InstVect &Ins);

    AliasAnalysis *AA;
    DominatorTree *DT;
    Function      *Fn;

    FlowGraphEdgeSet BackEdges;
  };
}


char SelectConversion::ID = 0;


INITIALIZE_PASS_BEGIN(SelectConversion, "select-conversion",
                "Target Independent If Conversion", false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTree)
INITIALIZE_PASS_END(SelectConversion, "select-conversion",
                "Target Independent If Conversion", false, false)


static bool identicalInstructions(const Value *V1, const Value *V2);
static Type *advanceType(Type *Ty, const Value *X);



struct AddressEquivalence {
  AddressEquivalence(const ValueSet &AddrSet);
  bool Equ(const Value *A, const Value *B) const {
    if (A == B) return true;

    PtrToIntMap::const_iterator FA = EquMap.find(A);
    PtrToIntMap::const_iterator FB = EquMap.find(B);
    if (FA != EquMap.end() && FB != EquMap.end()) {
      return FA->second == FB->second;
    }
    assert (Full || "Comparing unknown values in a non-full equivalence");
    return false;
  }

private:
  typedef std::map<const Value*, uint32_t> PtrToIntMap;
  PtrToIntMap EquMap;
  bool Full;
};

AddressEquivalence::AddressEquivalence(const ValueSet &AddrSet) {
  uint32_t C = 0;
  Full = false;

  for (ValueSet::const_iterator I = AddrSet.begin(), E = AddrSet.end(); I != E; ++I) {
    if (EquMap.count(*I)) continue;

    // Create a new equivalence class.
    if (!Full) {
      C++;
      if (C == 0) Full = true;
    }

    if (!Full) {
      EquMap.insert(std::make_pair(*I, C));
      for (ValueSet::const_iterator J = next(I); J != E; ++J) {
        if (identicalInstructions(*I, *J)) {
          EquMap.insert(std::make_pair(*J, C));
        }
      }
    }

  }
}


#if 0
struct ValueOrder : public std::set<ValuePair> {
private:
  struct MemRef {
    MemRef(const LoadInst *Ld, AliasAnalysis *AA)
        : Loc(AA->getLocation(Ld)), IsLoad(true) {}
    MemRef(const StoreInst *St, AliasAnalysis *AA)
        : Loc(AA->getLocation(St)), IsLoad(false) {}

    AliasAnalysis::Location Loc;
    bool IsLoad;
  };

public:
  ValueOrder(const BasicBlock *B, AliasAnalysis *AA);
  bool before(const Value *A, const Value *B) const {
    return count(std::make_pair(A, B)) > 0;
  }
  bool include(const ValueOrder &Ord);
};


ValueOrder::ValueOrder(const BasicBlock *B, AliasAnalysis *AA) {
  std::vector<MemRef> BBO;
  std::queue<ValuePair> NewPairs;

  for (BasicBlock::const_iterator I = B->begin(), E = B->end(); I != E; ++I) {
    if (const LoadInst *Ld = dyn_cast<const LoadInst>(I)) {
      BBO.push_back(MemRef(Ld, AA));
    } else if (const StoreInst *St = dyn_cast<const StoreInst>(I)) {
      BBO.push_back(MemRef(St, AA));
    }
  }

  unsigned n = BBO.size();
  for (unsigned i = 0; i < n; ++i) {
    const MemRef &Ri = BBO[i];
    for (unsigned j = i+1; j < n; ++j) {
      const MemRef &Rj = BBO[j];
      if (Ri.IsLoad && Rj.IsLoad) continue;
      if (AA->alias(Ri.Loc, Rj.Loc)) {
        insert(std::make_pair(Ri.Loc.Ptr, Rj.Loc.Ptr));
      }
    }
  }

}
#endif

bool SelectConversion::runOnFunction(Function &F) {
  if (DisableSelConv) return false;

  AA = &getAnalysis<AliasAnalysis>();
  DT = &getAnalysis<DominatorTree>();
  Fn = &F;

  {
    typedef SmallVector<FlowGraphEdge, 32> EdgeVect;
    EdgeVect ES;
    FindFunctionBackedges(F, ES);
    for (EdgeVect::iterator I = ES.begin(), E = ES.end(); I != E; ++I) {
      BackEdges.insert(*I);
    }
  }

  bool DidSomething = false;
  bool Changed;
  SplitVect SS;

  do {
    SS.clear();
    Changed = false;

    collectSplitBlocks(SS);
    for (SplitVect::iterator I = SS.begin(), E = SS.end(); I != E; ++I) {
      Changed |= convertSplit(*I);
    }
    DidSomething |= Changed;
  } while (Changed);

  return DidSomething;
}


void SelectConversion::collectSplitBlocks(SplitVect &SS) {
  for (Function::iterator I = Fn->begin(), E = Fn->end(); I != E; ++I) {
    BasicBlock *BB = &*I;
    TerminatorInst *TI = BB->getTerminator();
    if (TI->getNumSuccessors() != 2) continue;

    BasicBlock *S0 = TI->getSuccessor(0);
    BasicBlock *S1 = TI->getSuccessor(1);

    if (BackEdges.count(std::make_pair(BB, S0))) continue;
    if (BackEdges.count(std::make_pair(BB, S1))) continue;

    TerminatorInst *S0T = S0->getTerminator();
    TerminatorInst *S1T = S1->getTerminator();

    unsigned S0PredN = std::distance(pred_begin(S0), pred_end(S0));
    unsigned S0SuccN = S0T->getNumSuccessors();
    unsigned S1PredN = std::distance(pred_begin(S1), pred_end(S1));
    unsigned S1SuccN = S1T->getNumSuccessors();

    // Is this a triangle with S0 being the conditional block?
    if (S0SuccN == 1 && S0T->getSuccessor(0) == S1) {
      if (S0PredN == 1 && S1PredN == 2) {
        SS.push_back(SplitInfo(BB, Triangle, 0));
      }
      continue;
    }
    // Is this a triangle with S1 being the conditional block?
    if (S1SuccN == 1 && S1T->getSuccessor(0) == S0) {
      if (S1PredN == 1 && S0PredN == 2) {
        SS.push_back(SplitInfo(BB, Triangle, 1));
      }
      continue;
    }
    // Diamond?
    if (S0PredN == 1 && S1PredN == 1) {
      if (S0SuccN == 1 && S1SuccN == 1) {
        if (S0T->getSuccessor(0) == S1T->getSuccessor(0)) {
          SS.push_back(SplitInfo(BB, Diamond));
        }
      }
      continue;
    }
  }  // for I = block

}


bool SelectConversion::isSpeculationCandidate(BasicBlock *BB) {
  for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
    if (isa<DbgInfoIntrinsic>(I)) continue;
    if (isa<BranchInst>(I)) continue;
    if (isa<StoreInst>(I) || isa<LoadInst>(I)) continue;

    if (!isSafeToSpeculativelyExecute(I)) return false;
  }
  return true;
}


void SelectConversion::expandDependencies(InstSet &Is, BasicBlock *BB) {
  InstVect Iv;

  for (InstSet::iterator I = Is.begin(), E = Is.end(); I != E; ++I) {
    Instruction *In = *I;
    if (!isa<DbgInfoIntrinsic>(In) && !isa<BranchInst>(In)) {
      if (In->getParent() == BB) {
        Iv.push_back(In);
      }
    }
  }


  for (unsigned i = 0; i < Iv.size(); ++i) {
    Instruction *I = Iv[i];
    for (User::op_iterator PI = I->op_begin(), PE = I->op_end();
         PI != PE; ++PI) {
      Instruction *OpI = dyn_cast<Instruction>(*PI);
      if (OpI && OpI->getParent() == BB && !Is.count(OpI)) {
        Iv.push_back(OpI);
        Is.insert(OpI);
      }
    }
  }
}


bool SelectConversion::coversWholeBlock(InstSet &Is, BasicBlock *BB) {
  for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
    if (isa<DbgInfoIntrinsic>(I) || isa<BranchInst>(I)) continue;
    if (!Is.count(&*I)) return false;
  }
  return true;
}



static Type *advanceType(Type *Ty, const Value *X) {
  if (Ty->isArrayTy() || Ty->isPointerTy()) {
    Ty = dyn_cast<SequentialType>(Ty)->getElementType();
  } else if (Ty->isStructTy()) {
    Ty = dyn_cast<StructType>(Ty)->getTypeAtIndex(X);
  } else {
    llvm_unreachable("Unexpected type");
  }
  return Ty;
}


bool SelectConversion::addressMatchFuzzy(const Value *P, const Value *Q) {
  if (P == Q) return true;

  // Check for an identical address first.
  const Instruction *PI = dyn_cast<const Instruction>(P);
  if (!PI) return false;

  if (const Instruction *QI = dyn_cast<const Instruction>(Q)) {
    if (identicalInstructions(PI, QI)) return true;
  }

  // Exact match not found.  Look for same base pointer with constant
  // indices.
  const GetElementPtrInst *PGep = dyn_cast<const GetElementPtrInst>(P);
  if (!PGep) return false;
  const Instruction *PBase =
        dyn_cast<const Instruction>(PGep->getPointerOperand());
  if (!PBase) return false;

  const GetElementPtrInst *QGep = dyn_cast<const GetElementPtrInst>(Q);
  if (!QGep) return false;
  const Instruction *QBase =
        dyn_cast<const Instruction>(QGep->getPointerOperand());

  if (!PBase || !QBase) return false;
  if (!identicalInstructions(PBase, QBase)) return false;


  if (PGep->getNumOperands() != QGep->getNumOperands()) return false;

  bool SameSoFar = true;
  Type *TyP = PBase->getType();
  Type *TyQ = QBase->getType();
  GetElementPtrInst::const_op_iterator PX = PGep->idx_begin(),
                                       PE = PGep->idx_end();
  GetElementPtrInst::const_op_iterator QX = QGep->idx_begin(),
                                       QE = QGep->idx_end();

  while (PX != PE) {
    bool AnyPointer = TyP->isPointerTy() || TyQ->isPointerTy();
    if (!SameSoFar && AnyPointer) return false;

    ConstantInt *PI = dyn_cast<ConstantInt>(*PX);
    if (!PI) return false;
    ConstantInt *QI = dyn_cast<ConstantInt>(*QX);
    if (!QI) return false;

    APInt PV = PI->getValue();
    APInt QV = QI->getValue();
    if (PV.getBitWidth() != QV.getBitWidth() || (PV != QV)) {
      if (AnyPointer) return false;
      SameSoFar = false;
    }

    TyP = advanceType(TyP, PI);
    TyQ = advanceType(TyQ, QI);

    ++PX; ++QX;
    assert ((PX == PE) == (QX == QE));
  }

  return true;
}


bool SelectConversion::pointersCoveredBy(const ValueSet &A,
                                         const ValueSet &B) {
  for (ValueSet::const_iterator I = A.begin(), E = A.end(); I != E; ++I) {
    bool FoundMatch = false;
    for (ValueSet::const_iterator J = B.begin(), F = B.end(); J != F; ++J) {
      if (addressMatchFuzzy(*I, *J)) {
        FoundMatch = true;
        break;
      }
    }
    if (!FoundMatch) return false;
  }
  return true;
}


void SelectConversion::hoistInstructions(Instruction *Start, Instruction *End,
                                         Instruction *Where) {
  if (Start == End) return;

  typedef BasicBlock::InstListType InstListType;

  InstListType &BBIs = Start->getParent()->getInstList();
  InstListType &BPIs = Where->getParent()->getInstList();

  BasicBlock::iterator S = Start, E = End, W = Where;
  BPIs.splice(W, BBIs, S, E);
}


static bool opcodesDifferent(unsigned Opc1, unsigned Opc2) {
  if (Opc1 != Opc2) return true;
  switch (Opc1) {
    case Instruction::Alloca:
    case Instruction::AtomicRMW:
    case Instruction::AtomicCmpXchg:
    case Instruction::Call:
    case Instruction::Fence:
    case Instruction::UserOp1:
    case Instruction::UserOp2:
      return true;
  }
  return false;
}


static bool isVolatile(const Instruction *In) {
  if (const LoadInst *Load = dyn_cast<const LoadInst>(In)) {
    return Load->isVolatile();
  } else if (const StoreInst *Store = dyn_cast<const StoreInst>(In)) {
    return Store->isVolatile();
  }
  return false;
}


static bool identicalInstructions(const Value *V1, const Value *V2) {
  if (V1 == V2) return true;

  if (const Instruction *IV1 = dyn_cast<const Instruction>(V1)) {
    if (const Instruction *IV2 = dyn_cast<const Instruction>(V2)) {
      if (opcodesDifferent(IV1->getOpcode(), IV2->getOpcode())) return false;
      if (IV1->getNumOperands() != IV2->getNumOperands()) return false;
      if (IV1->getType() != IV2->getType())               return false;
      if (isVolatile(IV1) || isVolatile(IV2))             return false;
      for (unsigned i = 0, n = IV1->getNumOperands(); i < n; ++i) {
        if (!identicalInstructions(IV1->getOperand(i), IV2->getOperand(i)))
          return false;
      }
      return true;
    }
  }
  return false;
}


class StoreCompare {
  StoreInst *Base;
  Value *Ptr;
  Instruction *PtrIn;
  Type *Ty;
  AddressEquivalence &AE;

public:
  StoreCompare(StoreInst *B, AddressEquivalence &Eq) : Base(B), AE(Eq) {
    Ptr = B->getPointerOperand();
    PtrIn = dyn_cast<Instruction>(Ptr);
    Ty = B->getValueOperand()->getType();
  }
  bool operator() (const Instruction *In) {
    if (const StoreInst *St = dyn_cast<const StoreInst>(In)) {
      if (!AE.Equ(St->getPointerOperand(), PtrIn)) return false;
      if (Ty != St->getValueOperand()->getType())  return false;
      return true;
    }
    return false;
  }
};


bool SelectConversion::isDefinedInBlock(Value *V, BasicBlock *BB) {
  if (Instruction *In = dyn_cast<Instruction>(V)) {
    if (In->getParent() == BB) return true;
  }
  return false;
}


void SelectConversion::collectLoadStoreInfo(BasicBlock *BB,
                                            InstSet *Stores,
                                            ValueSet *Ptrs) {
  if (!Stores && !Ptrs) return;

  for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
    if (StoreInst *St = dyn_cast<StoreInst>(I)) {
      if (Stores) Stores->insert(St);
      if (Ptrs) {
        Value *P = St->getPointerOperand();
        if (isDefinedInBlock(P, BB)) {
          Ptrs->insert(P);
        }
      }
      continue;
    }

    if (Ptrs) {
      if (LoadInst *Ld = dyn_cast<LoadInst>(I)) {
        Value *P = Ld->getPointerOperand();
        if (isDefinedInBlock(P, BB)) {
          Ptrs->insert(P);
        }
      }
    }

  }  // for

}


void SelectConversion::collectStoreOrder(BasicBlock *B, InstVect &Sts) {
  for (BasicBlock::iterator I = B->begin(), E = B->end(); I != E; ++I) {
    if (isa<StoreInst>(I)) Sts.push_back(I);
  }
}


// Union of sets.
template <typename T>
inline static std::set<T> U(const std::set<T> A, const std::set<T> B) {
  std::set<T> R = A;
  for (typename std::set<T>::const_iterator I = B.begin(), E = B.end(); I != E; ++I)
    R.insert(*I);
  return R;
}


static inline void eraseFromParent(Instruction *In) {
  In->eraseFromParent();
}

bool SelectConversion::convertSplit (SplitInfo &S) {
  if (S.Kind != Diamond) return false;

  TerminatorInst *TI = S.BB->getTerminator();
  BranchInst *BI = dyn_cast<BranchInst>(TI);
  if (!BI) return false;

  // Handle diamonds only for now.
  BasicBlock *B0 = TI->getSuccessor(0);
  BasicBlock *B1 = TI->getSuccessor(1);
  if (!isSpeculationCandidate(B0)) return false;
  if (!isSpeculationCandidate(B1)) return false;

#if 0
  ValueSet Ptrs0, Ptrs1, PtrsBB;

  collectLoadStoreInfo(B0, 0, &Ptrs0);
  collectLoadStoreInfo(B1, 0, &Ptrs1);
  collectLoadStoreInfo(S.BB, 0, &PtrsBB);

  AddressEquivalence AddrEqu(U(PtrsBB, U(Ptrs0, Ptrs0)));
#endif

  // Pair up store instructions.
  InstVect StV0, StV1;
  collectStoreOrder(B0, StV0);
  collectStoreOrder(B1, StV1);

  if (StV0.size() == 0) return false;
  if (StV0.size() != StV1.size()) return false;

  for (unsigned i = 0, n = StV0.size(); i < n; ++i) {
    StoreInst *S0 = dyn_cast<StoreInst>(StV0[i]);
    StoreInst *S1 = dyn_cast<StoreInst>(StV1[i]);
    assert (S0 && S1);
    bool AddrEqu = identicalInstructions(S0->getPointerOperand(),
                                         S1->getPointerOperand());
    if (!AddrEqu) return false;
  }

#if 0
  typedef std::pair<StoreInst*, StoreInst*> StorePair;
  typedef std::set<StorePair> StorePairSet;

  InstSet Work0 = Stores0, Work1 = Stores1;

  // Stores with a common address (i.e. storing to the same object).
  StorePairSet CommonAddr;

  for (InstSet::iterator I = Work0.begin(), E = Work0.end(); I != E; ) {
    InstSet::iterator N = I;
    ++N;

    StoreInst *St = cast<StoreInst>(*I);

    InstSet::iterator F = std::find_if(Work1.begin(), Work1.end(),
                                       StoreCompare(St, AddrEqu));
    if (F != Work1.end()) {
      CommonAddr.insert(std::make_pair(St, dyn_cast<StoreInst>(*F)));
      Work0.erase(I);
      Work1.erase(F);
    }
    I = N;
  }

  // Work0 and Work1 contain unpaired stores.
  if (Work0.size() != 0 || Work1.size()) return false;

  // Nothing to do.
  if (CommonAddr.empty()) return false;
#endif


  // We only want to do something if the control flow will be simplified,
  // i.e. both blocks, B0 and B1 will be merged.  Check right hand sides
  // to see if the stores, the values stores and the dependent values
  // cover the entire block.
  ValueSet Ptrs0, Ptrs1, PtrsBB;
  InstSet Work0, Work1;

  collectLoadStoreInfo(B0, &Work0, &Ptrs0);
  collectLoadStoreInfo(B1, &Work1, &Ptrs1);
  collectLoadStoreInfo(S.BB, 0, &PtrsBB);

  expandDependencies(Work0, B0);
  if (!coversWholeBlock(Work0, B0)) return false;
  expandDependencies(Work1, B1);
  if (!coversWholeBlock(Work1, B1)) return false;

  // For each indirect memory reference, check if this reference would
  // have occured in both paths.  Cannot speculate an indirect reference
  // if it's not executed on each path, since it may not be safe to do so.
  if (!pointersCoveredBy(Ptrs0, U(PtrsBB, Ptrs1))) return false;
  if (!pointersCoveredBy(Ptrs1, U(PtrsBB, Ptrs0))) return false;

#if 0
  dbgs() << "Common Address Stores:\n";
  for (StorePairSet::iterator I = CommonAddr.begin(), E = CommonAddr.end();
       I != E; ++I) {
    dbgs() << " 1:" << *I->first << "\n";
    dbgs() << " 2:" << *I->second << "\n";
  }

  // Check ordering of memory references in each block and make sure that
  // the store pairing will not violate the original memory access order.
#endif

  Value *SelV;
  Value *BrCond = BI->getCondition();

  IRBuilder<true, NoFolder> Builder(BI);

  BasicBlock::iterator I0 = B0->begin(), I1 = B1->begin();
  for (unsigned i = 0, n = StV0.size(); i < n; ++i) {
    StoreInst *St0 = dyn_cast<StoreInst>(StV0[i]);
    StoreInst *St1 = dyn_cast<StoreInst>(StV1[i]);
    hoistInstructions(I0, St0, TI);
    hoistInstructions(I1, St1, TI);

    Value *A = St0->getPointerOperand();
    Value *V0 = St0->getValueOperand();
    Value *V1 = St1->getValueOperand();

    SelV = Builder.CreateSelect(BrCond, V0, V1, "selv");
    Builder.CreateStore(SelV, A);

    I0 = St0; I0++;
    I1 = St1; I1++;
  }

  std::for_each(StV0.begin(), StV0.end(), eraseFromParent);
  std::for_each(StV1.begin(), StV1.end(), eraseFromParent);

  SimplifyCFG(B0);
  SimplifyCFG(B1);
  SimplifyCFG(S.BB);
  return true;
}


FunctionPass *llvm::createSelectConversionPass() {
  return new SelectConversion();
}

