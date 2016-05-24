// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//===- PostIncrement.cpp - Induction Variable Elimination ----------------===//
//
//                     The LLVM Compiler Infrastructure
//

#define DEBUG_TYPE "enable-base-offset"
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
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Transforms/Utils/Local.h"
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
  class EnableBaseOffset : public LoopPass {

  private:
    DominanceFrontier* DF;
    DominatorTree* DT;
    PostDominatorTree* PDT;

  public:
    static char ID; // Pass identification, replacement for typeid
    EnableBaseOffset() : LoopPass(ID) {}

    bool runOnLoop(Loop *L, LPPassManager &LPM);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<LoopInfo>();
      AU.addRequired<DominanceFrontier>();
      AU.addRequired<DominatorTree>();
      AU.addRequired<PostDominatorTree>();
    }
    
    // Insert Phi nodes if needed after incrementing the address
    void InsertPhiNodes(CastInst* PtrCast, PHINode* HdrPN, BasicBlock* LatchBlock);

  };
}

char EnableBaseOffset::ID = 0;


Pass *llvm::createEnableBaseOffsetPass() {
  return new EnableBaseOffset();
}

INITIALIZE_PASS_BEGIN(EnableBaseOffset, "enable-base-offset", 
                      "Simple strength reduction to enable base offset addressing", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfo)
INITIALIZE_PASS_DEPENDENCY(DominanceFrontier)
INITIALIZE_PASS_DEPENDENCY(DominatorTree)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTree)
INITIALIZE_PASS_END(EnableBaseOffset, "enable-base-offset", 
                    "Simple strength reduction to enable base offset addressing", false, false)


//
// Given an instruction that increments a base pointer -- PtrCast -- and
// a Phi Node inserted in the header, insert Phi nodes in the loop to
// propagate the incremented base pointer value through the loop
//
// Standard worklist based Phi node insertion algorithm:
//  For each definition, D in block BB
//    For each block DB in the iterated dominance frontier of BB
//      Insert a phi node
//
void EnableBaseOffset::InsertPhiNodes(CastInst* PtrCast, PHINode* HdrPN, BasicBlock* LatchBlock) {
  std::list<Instruction*> WorkList;
  WorkList.push_back(PtrCast);

  // Track whether a Phi has been inserted in a block
  std::map<BasicBlock*, PHINode*> BlockToPhis;
  BlockToPhis[HdrPN->getParent()] = HdrPN;

  std::list<Instruction*>::iterator ListIter = WorkList.begin();

  while (ListIter != WorkList.end()) {
    Instruction* CurInstr = *ListIter;
    BasicBlock* BB = CurInstr->getParent();
    DominanceFrontier::iterator DFI = DF->find(BB);

    // Iterate though blocks in the dominance frontier of BB
    if ( DFI != DF->end()) {
      DominanceFrontier::DomSetType S = DFI->second;
      for (DominanceFrontier::DomSetType::iterator I = S.begin(), E = S.end();
	   I != E; ++I) {
	BasicBlock *DB = *I;
	PHINode* PN = 0;

	// For some strange reason, DomFront(BB) can contain BB?! Filter
	// these out
	if (DB == BB) {
	  continue;
	}

	// Check if we have already inserted a PHI node for this block
	if (BlockToPhis.count(DB)) {
	  PN = BlockToPhis[DB];
	}
	else {
	  PN = PHINode::Create(CurInstr->getType(), 0, "phi.pst.cp", DB->begin());
	  BlockToPhis[DB] = PN;
	}
	
	// Iterate through the predecessors of the dominance frontier block DB
	// Insert an operand into the Phi node for the predecessor if we 
	// haven't already done so.
	bool Changed = false;
	for (pred_iterator PI = pred_begin(DB), E = pred_end(DB); PI != E; ++PI) {
	  BasicBlock *Pred = *PI;

	  // If a pred of DB is dominated by BB, then we want to add the incremented
	  // value along that path. Else add HdrPN (the old value)
	  if (DT->dominates(BB, Pred) && 
	      ((PN->getBasicBlockIndex(Pred) == -1) || 
	       PN->getIncomingValueForBlock(Pred) != CurInstr)) {
	    PN->addIncoming(CurInstr, Pred);
	    Changed = true;
	  }
	  else if (PN->getBasicBlockIndex(Pred) == -1) {
	    Changed = true;
	    PN->addIncoming(HdrPN, Pred);
	  }
	}

	if (Changed) {
	  WorkList.push_back(PN);
	}
      }
    }    
    ++ListIter;
  }


  // If at this point HdrPN does not have a value from LatchBlock,
  // then the increment instruction dominates the LatchBlock. Insert
  // the increment instruction into HdrPN as the value flowing in from 
  // the LatchBlock
  if (HdrPN->getBasicBlockIndex(LatchBlock) == -1) {
    HdrPN->addIncoming(PtrCast, LatchBlock);
  }
}


bool EnableBaseOffset::runOnLoop(Loop *L, LPPassManager &LPM) {

  DT = &getAnalysis<DominatorTree>();
  DF = &getAnalysis<DominanceFrontier>();
  PDT = &getAnalysis<PostDominatorTree>();

  // Loop through the phi nodes in the header
  // Identify Induction Variables
  BasicBlock* Hdr = L->getHeader();

  for (BasicBlock::iterator PNI = Hdr->begin(); 
       (PNI != Hdr->end()) && isa<PHINode>(PNI); ++PNI) {
    PHINode* PN = cast<PHINode>(PNI);

    // Only consider simple loops where header has 2 predecessors
    if (PN->getNumIncomingValues() != 2) {
      break;
    }

    BasicBlock *LatchBlock = L->getLoopLatch(); 
    Instruction *IncrementCand = dyn_cast<Instruction>(PN->getIncomingValueForBlock(LatchBlock));
    if (!IncrementCand) {
      continue;
    }
    BinaryOperator* BO = dyn_cast<BinaryOperator>(IncrementCand);
    if (!BO) {
      continue;
    }

    ConstantInt* IncrementVal = NULL;
    int BBIndexLB = PN->getBasicBlockIndex(LatchBlock);
    assert(BBIndexLB == 1 || BBIndexLB == 0);
    int BBIndex = (BBIndexLB == 1) ? 0 : 1;
    BasicBlock *NonLatchBB = PN->getIncomingBlock(BBIndex);

    Value* InitialVal = PN->getIncomingValue(BBIndex);
    if (BO && (BO->getOpcode() == Instruction::Add ||
	       BO->getOpcode() == Instruction::Sub) &&
	( (IncrementVal = dyn_cast<ConstantInt>(BO->getOperand(1))))) {
      
      // Identify Strength Reduce candidate
      for (Value::use_iterator UI = PN->use_begin(), UE = PN->use_end(); UI != UE; ++UI) {
	Use &TheUse = UI.getUse();
	Instruction* UseInst = dyn_cast<Instruction>(TheUse.getUser());
	assert(UseInst);
	BinaryOperator* SRCand = NULL;
	if ( (SRCand = dyn_cast<BinaryOperator>(UseInst)) &&
	     (SRCand->getOpcode() == Instruction::Add ||
	      SRCand->getOpcode() == Instruction::Sub)) {

	  Value *Op0 = SRCand->getOperand(0);
	  Value *Op1 = SRCand->getOperand(1);

	  Value* InvariantOpCand = (Op0 == PN) ? Op1 : Op0;
	  // Ensure that InvariantOpCand is Loop Invariant but not a constant
	  if (!isa<ConstantInt>(InvariantOpCand) && L->isLoopInvariant(InvariantOpCand)) {
	  
	    if (SRCand->hasOneUse()) {
	      Instruction* Use = cast<Instruction>(* SRCand->use_begin());
	      if ( (Use->getOpcode() == Instruction::IntToPtr) && 
		   (Use->hasOneUse())) {
		Instruction* PtrUse = dyn_cast<Instruction>(* Use->use_begin());
		// We only consider cases where PtrUse dominates the latch block.
		// For all other cases, we can strength reduce the address increment but
		// we will have to place the increment in the latch block which disallows
		// post-increment recognition. Currently post-increment recognition occurs only
		// if the address increment and the memory op is in the same basic block
		if (DT->dominates(PtrUse->getParent(), LatchBlock) && 
		    (isa<LoadInst>(PtrUse) || isa<StoreInst>(PtrUse))) {
		  // Construct the loop invariant part of the address
		  Instruction* LoopInvariant = BinaryOperator::Create(SRCand->getOpcode(), InvariantOpCand,
								      InitialVal, "enable.binop", 
								      NonLatchBB->getTerminator());
		  
		  // Construct the PHI node
		  PHINode* SRPhi = PHINode::Create(LoopInvariant->getType(),
						   PN->getNumIncomingValues(),
						   "enable.base",
						   Hdr->begin());
		  
		  // Construct the incremented address
		  Instruction* AddressIncrement = BinaryOperator::Create(BO->getOpcode(), SRPhi, IncrementVal, 
									 "address.inc");
		  AddressIncrement->insertAfter(PtrUse);
		  
		  // Populate the PHI node
		  SRPhi->addIncoming(LoopInvariant, NonLatchBB);
		  SRPhi->addIncoming(AddressIncrement, LatchBlock);
		  
		  // Modify uses of SRCand
		  SRCand->replaceAllUsesWith(SRPhi);
		  
		  DEBUG(dbgs() << "EnableBaseOffset: Changing value \n");
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
