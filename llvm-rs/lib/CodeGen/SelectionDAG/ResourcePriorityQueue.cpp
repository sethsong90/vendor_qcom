//===- ResourcePriorityQueue.cpp - A DFA-oriented priority queue -*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the ResourcePriorityQueue class, which is a
// SchedulingPriorityQueue that prioritizes instructions using DFA state to
// reduce the length of the critical path through the basic block
// on VLIW platforms.
// The scheduler is basically a top-down adaptable list scheduler with DFA
// resource tracking and cluster computation added to the cost function.
// DFA is queried as a state machine to model "packets/bundles" during
// schedule. Currently packets/bundles are discarded at the end of
// scheduling, affecting only order of instructions.
// Clusters are employed to detect parallel data dependency chains
// (the sort one would see after aggressive loop unrolling) and
// serializing/overlapping them during scheduling.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "scheduler"
#include "llvm/CodeGen/ResourcePriorityQueue.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetLowering.h"

using namespace llvm;

static cl::opt<bool> DisableDFASched("disable-dfa-sched", cl::Hidden,
  cl::ZeroOrMore, cl::init(false),
  cl::desc("Disable use of DFA during scheduling"));

static cl::opt<unsigned> LateClustersThreshold(
  "dfa-sched-indepth-threshold", cl::Hidden, cl::ZeroOrMore, cl::init(25),
  cl::desc("Track reg pressure and switch strategy to in-depth"));

static cl::opt<signed> RegPressureThreshold(
  "dfa-sched-reg-pressure-threshold", cl::Hidden, cl::ZeroOrMore, cl::init(5),
  cl::desc("Track reg pressure and switch priority to in-depth"));

// Flag used for marking non-cluster use of
// a field in SUnit.
static const unsigned NonClusterUse = 0xffffffff;

ResourcePriorityQueue::ResourcePriorityQueue(SelectionDAGISel *IS) :
  Picker(this),
  InstrItins(IS->getTargetLowering().getTargetMachine().getInstrItineraryData())
{
   TII = IS->getTargetLowering().getTargetMachine().getInstrInfo();
   TRI = IS->getTargetLowering().getTargetMachine().getRegisterInfo();
   TLI = &IS->getTargetLowering();

   const TargetMachine &tm = (*IS->MF).getTarget();
   ResourcesModel = tm.getInstrInfo()->CreateTargetScheduleState(&tm,NULL);
   // This hard requirment could be relaxed, but for now
   // do not let it procede.
   assert (ResourcesModel && "Unimplemented CreateTargetScheduleState.");

   unsigned NumRC = TRI->getNumRegClasses();
   RegLimit.resize(NumRC);
   RegPressure.resize(NumRC);
   std::fill(RegLimit.begin(), RegLimit.end(), 0);
   std::fill(RegPressure.begin(), RegPressure.end(), 0);
   for (TargetRegisterInfo::regclass_iterator I = TRI->regclass_begin(),
        E = TRI->regclass_end(); I != E; ++I)
     RegLimit[(*I)->getID()] = TRI->getRegPressureLimit(*I, *IS->MF);

   ParallelLiveRanges = 0;
   HorizontalVerticalBalance = 0;
   CurrentCluster = NonClusterUse;
   DominatorChains = 1;
   ClusterScheduling = false;
}

bool ResourcePriorityQueue::SUContainsCall(SUnit *SU) {
  if (!SU)
    return false;

  for (SDNode *N = SU->getNode(); N; N = N->getGluedNode())
    if (N->isMachineOpcode()) {
      const MCInstrDesc &TID = TII->get(N->getMachineOpcode());
      if (TID.isCall())
        return true;
    }

  return false;
}

bool ResourcePriorityQueue::SUIsMachineOp(SUnit *SU) {
  if (!SU || !SU->getNode())
    return false;

  if (SUContainsCall(SU))
    return true;

  if (SU->getNode()->isMachineOpcode())
    return true;

  return false;
}

bool ResourcePriorityQueue::SUIsMachineTop(SUnit *SU) {
  // TODO See what to do about calls
  // If this node itself is a machine op?
  if (!SUIsMachineOp(SU))
    return false;

  // Do not include Branch here
  if (SU->getNode() && SU->getNode()->isMachineOpcode()) {
    const MCInstrDesc &TID = TII->get(SU->getNode()->getMachineOpcode());
    if (TID.isBranch())
      return false;
  }
  // Look through all its predecessors,
  // if none of them is machine op, all pseudos,
  // this is what we want?
  for (SUnit::pred_iterator I = SU->Preds.begin(), E = SU->Preds.end();
       I != E; ++I)
    if (SUIsMachineOp(I->getSUnit()))
      return false;

  return true;
}

static bool SUInChain(SUnit *SU, std::vector<SUnit *> &Chain) {
  for (unsigned i = 0, e = Chain.size(); i != e; ++i)
    if (SU == Chain[i])
      return true;

  return false;
}

unsigned ResourcePriorityQueue::getChainForSU(SUnit *SU) {
  for (unsigned i = 0, e = Dominators.size(); i != e; ++i)
    for (unsigned ii = 0, ee = Dominators[i].size(); ii != ee; ++ii)
      if (SU ==  Dominators[i][ii])
        return i+1;

  return 0;
}

unsigned
ResourcePriorityQueue::numberRCValPredInSU(SUnit *SU, unsigned RCId) {
  unsigned NumberDeps = 0;
  for (SUnit::pred_iterator I = SU->Preds.begin(), E = SU->Preds.end();
       I != E; ++I) {
    if (I->isCtrl())
      continue;

    SUnit *PredSU = I->getSUnit();
    const SDNode *ScegN = PredSU->getNode();

    if (!ScegN)
      continue;

    // If value is passed to CopyToReg, it is probably
    // live outside BB.
    switch (ScegN->getOpcode()) {
      default:  break;
      case ISD::TokenFactor:    break;
      case ISD::CopyFromReg:    NumberDeps++;  break;
      case ISD::CopyToReg:      break;
      case ISD::INLINEASM:      break;
    }
    if (!ScegN->isMachineOpcode())
      continue;

    for (unsigned i = 0, e = ScegN->getNumValues(); i != e; ++i) {
      EVT VT = ScegN->getValueType(i);
      if (TLI->isTypeLegal(VT)
         && (TLI->getRegClassFor(VT)->getID() == RCId)) {
        NumberDeps++;
        break;
      }
    }
  }
  return NumberDeps;
}

unsigned ResourcePriorityQueue::numberRCValSuccInSU(SUnit *SU,
                                                    unsigned RCId) {
  unsigned NumberDeps = 0;
  for (SUnit::const_succ_iterator I = SU->Succs.begin(), E = SU->Succs.end();
       I != E; ++I) {
    if (I->isCtrl())
      continue;

    SUnit *SuccSU = I->getSUnit();
    const SDNode *ScegN = SuccSU->getNode();
    if (!ScegN)
      continue;

    // If value is passed to CopyToReg, it is probably
    // live outside BB.
    switch (ScegN->getOpcode()) {
      default:  break;
      case ISD::TokenFactor:    break;
      case ISD::CopyFromReg:    break;
      case ISD::CopyToReg:      NumberDeps++;  break;
      case ISD::INLINEASM:      break;
    }
    if (!ScegN->isMachineOpcode())
      continue;

    for (unsigned i = 0, e = ScegN->getNumOperands(); i != e; ++i) {
      const SDValue &Op = ScegN->getOperand(i);
      EVT VT = Op.getNode()->getValueType(Op.getResNo());
      if (TLI->isTypeLegal(VT)
         && (TLI->getRegClassFor(VT)->getID() == RCId)) {
        NumberDeps++;
        break;
      }
    }
  }
  return NumberDeps;
}

static unsigned numberCtrlDepsInSU(SUnit *SU) {
  unsigned NumberDeps = 0;
  for (SUnit::const_succ_iterator I = SU->Succs.begin(), E = SU->Succs.end();
       I != E; ++I)
    if (I->isCtrl())
      NumberDeps++;

  return NumberDeps;
}

static unsigned numberCtrlPredInSU(SUnit *SU) {
  unsigned NumberDeps = 0;
  for (SUnit::pred_iterator I = SU->Preds.begin(), E = SU->Preds.end();
       I != E; ++I)
    if (I->isCtrl())
      NumberDeps++;

  return NumberDeps;
}

unsigned ResourcePriorityQueue::iterateChainPred(
                           SUnit *SU, unsigned *Depth,
                           unsigned ClusterNumber,
                           bool IncludePseudo,
                           std::vector<SUnit *> &DepList) {
  if (!SU || SU->isScheduled)
    return *Depth;

  // Same as "visited" marker.
  // If it is not called from growth,
  // use SU->NodeQueueId.
  if (getChainForSU(SU) == ClusterNumber)
    return *Depth;

  // On the way up we should not be able to encounter a branch.
  if (getChainForSU(SU) && (ClusterNumber !=getChainForSU(SU)))
    return *Depth;

  if (!getChainForSU(SU)) {
    DepList.push_back(SU);
    SU->NodeQueueId = ClusterNumber;
    (*Depth)++;
    for (SUnit::pred_iterator I = SU->Preds.begin(), E = SU->Preds.end();
         I != E; ++I)
      iterateChainPred (&(*I->getSUnit()),
                      Depth,ClusterNumber,IncludePseudo,DepList);
    return *Depth;
  }
  return ++(*Depth);
}

unsigned ResourcePriorityQueue::iterateDominatingCluster(
                           SUnit *SU, unsigned *Depth,
                           unsigned ClusterNumber,
                           bool IncludePseudo,
                           std::vector<SUnit *> &DepList) {
  if (!SU || SU->isScheduled || SU->NodeQueueId)
    return *Depth;

  // Do not want it.
  if (!IncludePseudo && !SUIsMachineOp(SU))
    return *Depth;

  // If all successors of this SU belong to the current cluster,
  // include it as well.
  // If it belongs to another cluster, or not assigned yet,
  // do no touch it.
  for (SUnit::const_succ_iterator I = SU->Succs.begin(), E = SU->Succs.end();
       I != E; ++I)
    if (I->getSUnit()->NodeQueueId != ClusterNumber)
      return *Depth;

  DepList.push_back(SU);
  SU->NodeQueueId = ClusterNumber;
  (*Depth)++;
  for (SUnit::pred_iterator I = SU->Preds.begin(), E = SU->Preds.end();
       I != E; ++I)
    iterateDominatingCluster (&(*I->getSUnit()),
                      Depth,ClusterNumber,IncludePseudo,DepList);
  return *Depth;

}

unsigned ResourcePriorityQueue::iterateChainSucc(SUnit *SU, unsigned *Depth,
                           unsigned ClusterNumber,
                           bool IncludePseudo,
                           std::vector<SUnit *> &DepList) {
  if (!SU || SU->isScheduled)
    return *Depth;

  // Same as "visited" marker.
  if (SU->NodeQueueId)
    return *Depth;

  if (!IncludePseudo && !SUIsMachineOp(SU))
    return *Depth;

  // If I want to include it, make sure it has not been yet
  // included to some other chain.
  if (!SUIsMachineOp(SU) && getChainForSU(SU))
    return *Depth;

  // Try not to include jmp into any clusters.
  for (SDNode *N = SU->getNode(); N; N = N->getGluedNode())
    if (N->isMachineOpcode()) {
      const MCInstrDesc &TID = TII->get(N->getMachineOpcode());
      // Do not include jumps.
      if (TID.isBranch()) return *Depth;
    }

  DepList.push_back(SU);
  SU->NodeQueueId = ClusterNumber;
  (*Depth)++;

  for (SUnit::const_succ_iterator I = SU->Succs.begin(), E = SU->Succs.end();
       I != E; ++I)
    iterateChainSucc (&(*I->getSUnit()),
                      Depth,ClusterNumber,IncludePseudo,DepList);

  if (!SUIsMachineOp(SU))
    return *Depth;
  for (SUnit::pred_iterator I = SU->Preds.begin(), E = SU->Preds.end();
       I != E; ++I)
    iterateDominatingCluster (&(*I->getSUnit()),
                      Depth,ClusterNumber,IncludePseudo,DepList);

  return *Depth;
}

/// This function detects clusters in BBs.
/// It attempts to detect independent groups
/// of machine instructions joined by
/// dataflow.
unsigned ResourcePriorityQueue::iterateChain(SUnit *SU, unsigned *Depth,
                       unsigned ClusterNumber,
                       bool IncludePseudo,
                       std::vector<SUnit *> &DepList) {
  if (!SU)
    return *Depth;

  DepList.push_back(SU);
  SU->NodeQueueId = ClusterNumber;
  unsigned AccumulatedDepth = 0;
  unsigned ChainDept = 1;
  (*Depth)++;
  // TODO: Better detection of terminator node.
  if (SU->NodeNum <= 0)
    return *Depth;

  for (SUnit::const_succ_iterator I = SU->Succs.begin(), E = SU->Succs.end();
                                  I != E; ++I) {
    ChainDept = 1;
    iterateChainSucc(&(*I->getSUnit()),
                     &ChainDept,ClusterNumber,IncludePseudo,DepList);
    if (ChainDept > AccumulatedDepth) AccumulatedDepth = ChainDept;
  }
  // Now go up a bit, and if pred node dominates only this cluster,
  // add it in here as well.
  for (SUnit::pred_iterator I = SU->Preds.begin(), E = SU->Preds.end();
       I != E; ++I)
    iterateDominatingCluster (&(*I->getSUnit()),
                      Depth,ClusterNumber,IncludePseudo,DepList);

  return AccumulatedDepth;
}

///
/// Grow thin clusters.
///
unsigned ResourcePriorityQueue::growClusters() {
  unsigned Depth = 0;

  if (Dominators.size() < 2)
    return 0;

  // Include non-shared SUs into existing clusters.
  for (unsigned i = 0, e = Dominators.size(); i != e; ++i)
    for (unsigned ii = 0, ee = Dominators[i].size(); ii != ee; ++ii) {
      SUnit *SU =    Dominators[i][ii];
      for (SUnit::pred_iterator I = SU->Preds.begin(), E = SU->Preds.end();
                            I != E; ++I)
        iterateChainPred(&(*I->getSUnit()),
                          &Depth,(i+1),true,Dominators[i]);
    }

  // Try to merge some clusters.
  // If a cluster is dominated by another cluster - merge them.
  for (unsigned i = 0, e = Dominators.size(); i != e; ++i) {
    unsigned Dominator = 0;
    bool HasMultiplePreds = false;

    for (unsigned ii = 0, ee = Dominators[i].size(); ii != ee; ++ii) {
      SUnit *SU =    Dominators[i][ii];
      // If any predecessors of this SU belongs to another cluster...
      for (SUnit::pred_iterator I = SU->Preds.begin(), E = SU->Preds.end();
           I != E; ++I) {
        if (getChainForSU(I->getSUnit()) != (i+1)) {
          if (Dominator && (Dominator != getChainForSU(I->getSUnit()))) {
            // Multiple predecessors.
            HasMultiplePreds = true;
            break;
          }
          else Dominator = getChainForSU(I->getSUnit());
        }
      }
    }
    // Ok, there is only one dominating cluster - merge them.
    if (!HasMultiplePreds && Dominator) {
      for (unsigned ii = 0, ee = Dominators[i].size(); ii != ee; ++ii) {
        SUnit *SU =    Dominators[i][ii];
        Dominators[Dominator-1].push_back(SU);
        SU->NodeQueueId = Dominator;
      }
      // Remove this SU from original cluster.
      for (unsigned ii = 0, ee = Dominators[i].size(); ii != ee; ++ii)
        Dominators[i].pop_back();
    }
  }

  return Depth;
}

struct CustomSU {
    SUnit *SU;
    CustomSU (SUnit *su) : SU(su) {}
};

struct CustomSUCmp {
  bool operator () (const CustomSU &SU1, const CustomSU &SU2) {
    return SU1.SU->getHeight() > SU2.SU->getHeight();
  }
};

///
/// Initialize nodes. Detect clusters.
///
void ResourcePriorityQueue::initNodes(std::vector<SUnit> &sunits) {
  SUnits = &sunits;
  NumNodesSolelyBlocking.resize(SUnits->size(), 0);
  // Find nodes that are likely chain starters
  // and sort them.
  std::vector<CustomSU> ChainTops;

  for (unsigned i = 0, e = SUnits->size(); i != e; ++i) {
    SUnit *SU = &(*SUnits)[i];
    initNumRegDefsLeft(SU);
    if (SUIsMachineTop(SU))
        ChainTops.push_back(CustomSU(SU));
    // This means unassigned node.
    SU->NodeQueueId = 0;
  }
  std::sort(ChainTops.begin(), ChainTops.end(), CustomSUCmp());

  for (unsigned i = 0, e = ChainTops.size(); i != e; ++i) {
    SUnit *SU = ChainTops[i].SU;
    std::vector<SUnit*> DominatorChain;
    unsigned Depth = 0;
    bool SUAlreadyInChain = false;

    for (unsigned i = 0, e = Dominators.size(); i != e; ++i) {
        if (SUInChain(SU,Dominators[i])) {
            SUAlreadyInChain = true;
            break;
        }
    }
    if  (!SUAlreadyInChain) {
        // TODO: This is the location of adaptive scheduling
        // decision making prior to scheduling initiation.
        // In the future an early decision on starting cluster
        // could be performed at this time.
        Depth = iterateChain(SU,&Depth,DominatorChains++,true,
                             DominatorChain);
        assert(DominatorChain.size() && "Empty cluster");
        Dominators.push_back(DominatorChain);
    }
  }
  // Now grow cluster to unclude non shared pseudos.
  growClusters();
  UnscheduledClusterMembers.resize(Dominators.size());

  for (unsigned i = 0, e = Dominators.size(); i != e; ++i)
    UnscheduledClusterMembers[i] = Dominators[i].size();

}

/// This heuristic is used if DFA scheduling is not desired
/// for some VLIW platform.
bool resource_sort::operator()(const SUnit *LHS, const SUnit *RHS) const {
  // The isScheduleHigh flag allows nodes with wraparound dependencies that
  // cannot easily be modeled as edges with latencies to be scheduled as
  // soon as possible in a top-down schedule.
  if (LHS->isScheduleHigh && !RHS->isScheduleHigh)
    return false;

  if (!LHS->isScheduleHigh && RHS->isScheduleHigh)
    return true;

  unsigned LHSNum = LHS->NodeNum;
  unsigned RHSNum = RHS->NodeNum;

  // The most important heuristic is scheduling the critical path.
  unsigned LHSLatency = PQ->getLatency(LHSNum);
  unsigned RHSLatency = PQ->getLatency(RHSNum);
  if (LHSLatency < RHSLatency) return true;
  if (LHSLatency > RHSLatency) return false;

  // After that, if two nodes have identical latencies, look to see if one will
  // unblock more other nodes than the other.
  unsigned LHSBlocked = PQ->getNumSolelyBlockNodes(LHSNum);
  unsigned RHSBlocked = PQ->getNumSolelyBlockNodes(RHSNum);
  if (LHSBlocked < RHSBlocked) return true;
  if (LHSBlocked > RHSBlocked) return false;

  // Finally, just to provide a stable ordering, use the node number as a
  // deciding factor.
  return LHSNum < RHSNum;
}


/// getSingleUnscheduledPred - If there is exactly one unscheduled predecessor
/// of SU, return it, otherwise return null.
SUnit *ResourcePriorityQueue::getSingleUnscheduledPred(SUnit *SU) {
  SUnit *OnlyAvailablePred = 0;
  for (SUnit::const_pred_iterator I = SU->Preds.begin(), E = SU->Preds.end();
       I != E; ++I) {
    SUnit &Pred = *I->getSUnit();
    if (!Pred.isScheduled) {
      // We found an available, but not scheduled, predecessor.  If it's the
      // only one we have found, keep track of it... otherwise give up.
      if (OnlyAvailablePred && OnlyAvailablePred != &Pred)
        return 0;
      OnlyAvailablePred = &Pred;
    }
  }
  return OnlyAvailablePred;
}

void ResourcePriorityQueue::push(SUnit *SU) {
  // Look at all of the successors of this node.  Count the number of nodes that
  // this node is the sole unscheduled node for.
  unsigned NumNodesBlocking = 0;
  for (SUnit::const_succ_iterator I = SU->Succs.begin(), E = SU->Succs.end();
       I != E; ++I)
    if (getSingleUnscheduledPred(I->getSUnit()) == SU)
      ++NumNodesBlocking;

  NumNodesSolelyBlocking[SU->NodeNum] = NumNodesBlocking;
  Queue.push_back(SU);
}

/// Check if scheduling of this SU is possible
/// in the current packet.
bool ResourcePriorityQueue::isResourceAvailable(SUnit *SU) {
  if (!SU || !SU->getNode())
    return false;

  // If this is a compound instruction,
  // it is likely to be a call. Do not delay it.
  if (SU->getNode()->getGluedNode())
    return true;

  // First see if the pipeline could receive this instruction
  // in the current cycle.
  if (SU->getNode()->isMachineOpcode())
    switch (SU->getNode()->getMachineOpcode()) {
    default:
      if (!ResourcesModel->canReserveResources(&TII->get(
          SU->getNode()->getMachineOpcode())))
           return false;
    case TargetOpcode::EXTRACT_SUBREG:
    case TargetOpcode::INSERT_SUBREG:
    case TargetOpcode::SUBREG_TO_REG:
    case TargetOpcode::REG_SEQUENCE:
    case TargetOpcode::IMPLICIT_DEF:
        break;
    }

  // Now see if there are no other dependencies
  // to instructions alredy in the packet.
  for (unsigned i = 0, e = Packet.size(); i != e; ++i)
    for (SUnit::const_succ_iterator I = Packet[i]->Succs.begin(),
         E = Packet[i]->Succs.end(); I != E; ++I) {
      // Since we do not add pseudos to packets, might as well
      // ignor order deps.
      if (I->isCtrl())
        continue;

      if (I->getSUnit() == SU)
        return false;
    }

  return true;
}

/// Keep track of available resources.
void ResourcePriorityQueue::reserveResources(SUnit *SU) {
  // If this SU does not fit in the packet
  // start a new one.
  if (!isResourceAvailable(SU) || SU->getNode()->getGluedNode()) {
    ResourcesModel->clearResources();
    Packet.clear();
  }

  if (SU->getNode() && SU->getNode()->isMachineOpcode()) {
    switch (SU->getNode()->getMachineOpcode()) {
    default:
      ResourcesModel->reserveResources(&TII->get(
        SU->getNode()->getMachineOpcode()));
      break;
    case TargetOpcode::EXTRACT_SUBREG:
    case TargetOpcode::INSERT_SUBREG:
    case TargetOpcode::SUBREG_TO_REG:
    case TargetOpcode::REG_SEQUENCE:
    case TargetOpcode::IMPLICIT_DEF:
      break;
    }
    Packet.push_back(SU);
  }
  // Forcefully end packet for PseudoOps.
  else {
    ResourcesModel->clearResources();
    Packet.clear();
  }

  // If packet is now full, reset the state so in the next cycle
  // we start fresh.
  if (Packet.size() >= InstrItins->IssueWidth) {
    ResourcesModel->clearResources();
    Packet.clear();
  }
}

signed ResourcePriorityQueue::rawRegPressureDelta(SUnit *SU, unsigned RCId) {
  signed RegBalance    = 0;

  if (!SU || !SU->getNode() || !SU->getNode()->isMachineOpcode())
    return RegBalance;

  // Gen estimate.
  for (unsigned i = 0, e = SU->getNode()->getNumValues(); i != e; ++i) {
      EVT VT = SU->getNode()->getValueType(i);
      if (TLI->isTypeLegal(VT)
          && TLI->getRegClassFor(VT)
          && TLI->getRegClassFor(VT)->getID() == RCId)
        RegBalance += numberRCValSuccInSU(SU, RCId);
  }
  // Kill estimate.
  for (unsigned i = 0, e = SU->getNode()->getNumOperands(); i != e; ++i) {
      const SDValue &Op = SU->getNode()->getOperand(i);
      EVT VT = Op.getNode()->getValueType(Op.getResNo());
      if (isa<ConstantSDNode>(Op.getNode()))
        continue;

      if (TLI->isTypeLegal(VT) && TLI->getRegClassFor(VT)
          && TLI->getRegClassFor(VT)->getID() == RCId)
        RegBalance -= numberRCValPredInSU(SU, RCId);
  }
  return RegBalance;
}

/// Estimates change in reg pressure from this SU.
/// It is acheived by trivial tracking of defined
/// and used vregs in dependent instructions.
/// The RawPressure flag makes this function to ignore
/// existing reg file sizes, and report raw def/use
/// balance.
signed ResourcePriorityQueue::regPressureDelta(SUnit *SU, bool RawPressure) {
  signed RegBalance    = 0;

  if (!SU || !SU->getNode() || !SU->getNode()->isMachineOpcode())
    return RegBalance;

  if (RawPressure) {
    for (TargetRegisterInfo::regclass_iterator I = TRI->regclass_begin(),
             E = TRI->regclass_end(); I != E; ++I) {
      const TargetRegisterClass *RC = *I;
      RegBalance += rawRegPressureDelta(SU, RC->getID());
    }
  }
  else {
    for (TargetRegisterInfo::regclass_iterator I = TRI->regclass_begin(),
         E = TRI->regclass_end(); I != E; ++I) {
      const TargetRegisterClass *RC = *I;
      if ((RegPressure[RC->getID()] +
           rawRegPressureDelta(SU, RC->getID()) > 0) &&
          (RegPressure[RC->getID()] +
           rawRegPressureDelta(SU, RC->getID())  >= RegLimit[RC->getID()]))
        RegBalance += rawRegPressureDelta(SU, RC->getID());
    }
  }

  return RegBalance;
}

// Constants used to denote relative importance of
// heuristic components for cost computation.
static const unsigned PriorityOne = 200;
static const unsigned PriorityTwo = 100;
static const unsigned PriorityThree = 50;
static const unsigned PriorityFour = 15;
static const unsigned PriorityFive = 5;
static const unsigned ScaleOne = 20;
static const unsigned ScaleTwo = 10;
static const unsigned ScaleThree = 5;
static const unsigned FactorOne = 2;

/// Returns single number reflecting benefit of scheduling SU
/// in the current cycle.
signed ResourcePriorityQueue::SUSchedulingCost(SUnit *SU) {
  // Initial trivial priority.
  signed ResCount = 1;

  // Do not waste time on a node that is already scheduled.
  if (SU->isScheduled)
    return ResCount;

  // Forced priority is high.
  if (SU->isScheduleHigh)
    ResCount += PriorityOne;

  // Adaptable scheduling has three broad scenarios
  // Cluster scheduling for wide, crowded regions
  // with clusters of dependencies, largely
  // independent from each other.
  if (ClusterScheduling) {
    // Heavily prioratize current cluster
    // and allow other started clusters to rise.
    for (unsigned i = 0, e = StartedCluster.size(); i != e; ++i)
      if (StartedCluster[i] == (SU->NodeQueueId - 1)) {
        ResCount += PriorityTwo;
        break;
      }

    if (CurrentCluster == (SU->NodeQueueId - 1))
      ResCount += PriorityTwo;

    // Critical path first.
    ResCount += (SU->getHeight() * ScaleTwo);
    // Now see how many instructions is blocked by this SU.
    ResCount += (NumNodesSolelyBlocking[SU->NodeNum] * ScaleTwo);
    // If resources are available for it, multiply the
    // chance of scheduling.
    if (isResourceAvailable(SU))
      ResCount <<= FactorOne;

    // Adjust for potential reg pressure delta.
    ResCount -= (regPressureDelta(SU) * ScaleTwo);
  }
  // A non-cluster, small, but very parallel
  // region, where reg pressure is an issue,
  // but no clusters could be identified.
  else if (HorizontalVerticalBalance > RegPressureThreshold) {
    // Critical path first
    ResCount += (SU->getHeight() * ScaleTwo);
    // If resources are available for it, multiply the
    // chance of scheduling.
    if (isResourceAvailable(SU))
      ResCount <<= FactorOne;

    // Consider change to reg pressure from scheduling
    // this SU.
    ResCount -= (regPressureDelta(SU,true) * ScaleOne);
  }
  // Default heuristic, greeady and
  // critical path driven.
  else {
    // Critical path first.
    ResCount += (SU->getHeight() * ScaleTwo);
    // Now see how many instructions is blocked by this SU.
    ResCount += (NumNodesSolelyBlocking[SU->NodeNum] * ScaleTwo);
    // If resources are available for it, multiply the
    // chance of scheduling.
    if (isResourceAvailable(SU))
      ResCount <<= FactorOne;

    ResCount -= (regPressureDelta(SU) * ScaleTwo);
  }

  // These are platform specific things.
  // Will need to go into the back end
  // and accessed from here via a hook.
  for (SDNode *N = SU->getNode(); N; N = N->getGluedNode()) {
    if (N->isMachineOpcode()) {
      const MCInstrDesc &TID = TII->get(N->getMachineOpcode());
      if (TID.isCall())
        ResCount += (PriorityThree + (ScaleThree*N->getNumValues()));
    }
    else
      switch (N->getOpcode()) {
      default:  break;
      case ISD::TokenFactor:
      case ISD::CopyFromReg:
      case ISD::CopyToReg:
        ResCount += PriorityFive;
        break;

      case ISD::INLINEASM:
        ResCount += PriorityFour;
        break;
      }
  }
  return ResCount;
}


/// Main resource tracking point.
void ResourcePriorityQueue::scheduledNode(SUnit *SU) {
  // Use NULL entry as an event marker to reset
  // the DFA state.
  if (!SU) {
    ResourcesModel->clearResources();
    Packet.clear();
    return;
  }

  const SDNode *ScegN = SU->getNode();
  // Update reg pressure tracking.
  // First update current node.
  if (ScegN->isMachineOpcode()) {
    // Estimate generated regs.
    for (unsigned i = 0, e = ScegN->getNumValues(); i != e; ++i) {
      EVT VT = ScegN->getValueType(i);

      if (TLI->isTypeLegal(VT)) {
        const TargetRegisterClass *RC = TLI->getRegClassFor(VT);
        if (RC)
          RegPressure[RC->getID()] += numberRCValSuccInSU(SU, RC->getID());
      }
    }
    // Estimate killed regs.
    for (unsigned i = 0, e = ScegN->getNumOperands(); i != e; ++i) {
      const SDValue &Op = ScegN->getOperand(i);
      EVT VT = Op.getNode()->getValueType(Op.getResNo());

      if (TLI->isTypeLegal(VT)) {
        const TargetRegisterClass *RC = TLI->getRegClassFor(VT);
        if (RC) {
          if (RegPressure[RC->getID()] >
            (numberRCValPredInSU(SU, RC->getID())))
            RegPressure[RC->getID()] -= numberRCValPredInSU(SU, RC->getID());
          else RegPressure[RC->getID()] = 0;
        }
      }
    }
    for (SUnit::pred_iterator I = SU->Preds.begin(), E = SU->Preds.end();
                              I != E; ++I) {
      if (I->isCtrl() || (I->getSUnit()->NumRegDefsLeft == 0))
        continue;
      --I->getSUnit()->NumRegDefsLeft;
    }
  }

  // If the cluster scheduling is already underway
  // see when do we want to overlap next cluster with
  // the current one.
  // For this we need to track local reg pressure
  // and detect when it peaks and starts going down
  if (ClusterScheduling) {
    // This threshold determines when to allow next cluster
    // to start. It is largely platform independent, and
    // mainly a function of cluster size.
    // Empirical value of 15 seems to work well for
    // a variety of scenarious. If in the future this would
    // be proven to be false, this might receive its own
    // computational method.
    static const unsigned OverlapThreshold = 15;
    // Keep track of number scheduled cluster members.
    if ((SU->NodeQueueId != 0) && (SU->NodeQueueId != NonClusterUse))
        UnscheduledClusterMembers[SU->NodeQueueId - 1]--;

    // If we are almost done with the current cluster, allow
    // the next one start.
    if ((SU->NodeQueueId != 0)
         && (SU->NodeQueueId != NonClusterUse)
         && (CurrentCluster != (SU->NodeQueueId - 1))
         && (UnscheduledClusterMembers[CurrentCluster] < OverlapThreshold))
      StartedCluster.push_back((SU->NodeQueueId - 1));
  }

  // If this node has no predecessors, see which cluster it belongs to.
  // It might be starting a new cluster, and we would want to note this event.
  // TODO: This is foundation of BB characterisation for improved
  // adaptive scheduling in the future.
  if (!ClusterScheduling
      && (Dominators.size() > 1)
      && SU->NodeQueueId != 0
      && SU->NodeQueueId != NonClusterUse) {
    ClusterScheduling = true;
    // Cluster # and cluster ID are off by one.
    CurrentCluster = SU->NodeQueueId - 1;
    // This SU is already scheduled
    UnscheduledClusterMembers[CurrentCluster]--;
    // All clusters "in flight"
    StartedCluster.push_back(CurrentCluster);

    // After initial cluster formation some none machine
    // SUs could belong to multiple clusters, here we need to
    // include them all in the cluster currently being selected
    // for scheduling.
    for (unsigned ii = 0, ee = Dominators[CurrentCluster].size();
         ii != ee; ++ii)
      Dominators[CurrentCluster][ii]->NodeQueueId = CurrentCluster + 1;
  }

  // Reserve resources for this SU.
  reserveResources(SU);

  // Detect trailing clusters.
  // If we have began with no clusters, or situation has
  // dynamically changed, see if we can detect a subcluster
  // chain, and finish it first, in the hope to reduce number of
  // parallel live ranges.
  if (!ClusterScheduling
      && (ParallelLiveRanges > LateClustersThreshold)
      && !SU->isScheduleHigh) {
    std::vector<SUnit*> DominatorChain;
    unsigned Depth = 0;
    // If this is an unscheduled node, set cluster name correctly.
    unsigned TrailingChains = 1;

    // First throw away current cluster assignment (if any).
    for (unsigned i = 0, e = SUnits->size(); i != e; ++i) {
      SUnit *SU = &(*SUnits)[i];
      SU->NodeQueueId = 0;
    }
    // Reconstruct partial clusters.
    Depth = iterateChain(SU, &Depth, TrailingChains, false,
                             DominatorChain);

    // Mark the sub-cluster for priority scheduling.
    // Note the different way to mark SUs.
    for (unsigned i = 0, e = DominatorChain.size(); i != e; ++i) {
        DominatorChain[i]->isScheduleHigh = true;
        // Reset this to look unclustered, so we do not
        // interfere with early clustering.
        DominatorChain[i]->NodeQueueId = 0;
    }
  }


  // If this cluster is fully scheduled, and another has not started yet,
  // reset to the initial state.
  if (ClusterScheduling && !UnscheduledClusterMembers[CurrentCluster]) {
    ClusterScheduling = false;
    CurrentCluster = NonClusterUse;
  }

  // Adjust number of parallel live ranges.
  // Heuristic is simple - node with no data successors reduces
  // number of live ranges. All others, increase it.
  unsigned NumberNonControlDeps = 0;

  for (SUnit::const_succ_iterator I = SU->Succs.begin(), E = SU->Succs.end();
                                  I != E; ++I) {
    adjustPriorityOfUnscheduledPreds(I->getSUnit());
    if (!I->isCtrl())
      NumberNonControlDeps++;
  }

  if (!NumberNonControlDeps) {
    if (ParallelLiveRanges >= SU->NumPreds)
      ParallelLiveRanges -= SU->NumPreds;
    else
      ParallelLiveRanges = 0;

  }
  else
    ParallelLiveRanges += SU->NumRegDefsLeft;

  // Track parallel live chains.
  HorizontalVerticalBalance += (SU->Succs.size() - numberCtrlDepsInSU(SU));
  HorizontalVerticalBalance -= (SU->Preds.size() - numberCtrlPredInSU(SU));
}

void ResourcePriorityQueue::initNumRegDefsLeft(SUnit *SU) {
  unsigned  NodeNumDefs = 0;
  for (SDNode *N = SU->getNode(); N; N = N->getGluedNode())
    if (N->isMachineOpcode()) {
      const MCInstrDesc &TID = TII->get(N->getMachineOpcode());
      // No register need be allocated for this.
      if (N->getMachineOpcode() == TargetOpcode::IMPLICIT_DEF) {
        NodeNumDefs = 0;
        break;
      }
      NodeNumDefs = std::min(N->getNumValues(), TID.getNumDefs());
    }
    else
      switch(N->getOpcode()) {
        default:     break;
        case ISD::CopyFromReg:
          NodeNumDefs++;
          break;
        case ISD::INLINEASM:
          NodeNumDefs++;
          break;
      }

  SU->NumRegDefsLeft = NodeNumDefs;
}

/// adjustPriorityOfUnscheduledPreds - One of the predecessors of SU was just
/// scheduled.  If SU is not itself available, then there is at least one
/// predecessor node that has not been scheduled yet.  If SU has exactly ONE
/// unscheduled predecessor, we want to increase its priority: it getting
/// scheduled will make this node available, so it is better than some other
/// node of the same priority that will not make a node available.
void ResourcePriorityQueue::adjustPriorityOfUnscheduledPreds(SUnit *SU) {
  if (SU->isAvailable) return;  // All preds scheduled.

  SUnit *OnlyAvailablePred = getSingleUnscheduledPred(SU);
  if (OnlyAvailablePred == 0 || !OnlyAvailablePred->isAvailable)
    return;

  // Okay, we found a single predecessor that is available, but not scheduled.
  // Since it is available, it must be in the priority queue.  First remove it.
  remove(OnlyAvailablePred);

  // Reinsert the node into the priority queue, which recomputes its
  // NumNodesSolelyBlocking value.
  push(OnlyAvailablePred);
}


/// Main access point - returns next instructions
/// to be placed in scheduling sequence.
SUnit *ResourcePriorityQueue::pop() {
  if (empty())
    return 0;

  std::vector<SUnit *>::iterator Best = Queue.begin();
  if (!DisableDFASched) {
    signed BestCost = SUSchedulingCost(*Best);
    for (std::vector<SUnit *>::iterator I = Queue.begin(),
           E = Queue.end(); I != E; ++I) {
      if (*I == *Best)
        continue;

      if (SUSchedulingCost(*I) > BestCost) {
        BestCost = SUSchedulingCost(*I);
        Best = I;
      }
    }
  }
  // Use default TD scheduling mechanism.
  else {
    for (std::vector<SUnit *>::iterator I = llvm::next(Queue.begin()),
       E = Queue.end(); I != E; ++I)
      if (Picker(*Best, *I))
        Best = I;
  }

  SUnit *V = *Best;
  if (Best != prior(Queue.end()))
    std::swap(*Best, Queue.back());

  Queue.pop_back();

  return V;
}


void ResourcePriorityQueue::remove(SUnit *SU) {
  assert(!Queue.empty() && "Queue is empty!");
  std::vector<SUnit *>::iterator I = std::find(Queue.begin(), Queue.end(), SU);
  if (I != prior(Queue.end()))
    std::swap(*I, Queue.back());

  Queue.pop_back();
}


#ifdef NDEBUG
void ResourcePriorityQueue::dump(ScheduleDAG *DAG) const {}
#else
void ResourcePriorityQueue::dump(ScheduleDAG *DAG) const {
  ResourcePriorityQueue q = *this;
  while (!q.empty()) {
    SUnit *su = q.pop();
    dbgs() << "Height " << su->getHeight() << ": ";
    su->dump(DAG);
  }
}
#endif
