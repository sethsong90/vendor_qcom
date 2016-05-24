// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//===---- LatencyPriorityQueue.cpp - A latency-oriented priority queue ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the LatencyProfitPriorityQueue class, which is a
// SchedulingPriorityQueue that schedules using latency information to
// reduce the length of the critical path through the basic block and
// pipeline instruction mixing profitability checks.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "scheduler"
#include "llvm/CodeGen/LatencyProfitPriorityQueue.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

bool latency_profit_sort::operator()(const SUnit *LHS, const SUnit *RHS) const {
   ScheduleProfitRecognizer *ProfitRec = PQ->getProfitRec();
   bool LHSProfit = ProfitRec->isEfficientInstrMix(LHS);
   bool RHSProfit = ProfitRec->isEfficientInstrMix(RHS);
   unsigned int LHSPipeCount = ProfitRec->getPipeCount(LHS);
   unsigned int RHSPipeCount = ProfitRec->getPipeCount(RHS);
   unsigned LHSCost;
   unsigned RHSCost;

   // The isScheduleHigh flag allows nodes with wraparound dependencies that
   // cannot easily be modeled as edges with latencies to be scheduled as
   // soon as possible in a top-down schedule.
   if (LHS->isScheduleHigh && !RHS->isScheduleHigh)
     return false;
   if (!LHS->isScheduleHigh && RHS->isScheduleHigh)
     return true;

   // After that, if two nodes have schedule urgency flags set,
   // look for the one which will result in better instruction mix.
   if(LHS->isScheduleHigh && RHS->isScheduleHigh) {
     LHSCost = 1;
     RHSCost = 1;

     if (LHSProfit) LHSCost <<= 2;
     if (RHSProfit) RHSCost <<= 2;

     if (LHSCost < RHSCost) return true; // RHS is better choice
     if (LHSCost > RHSCost) return false;

     // After that, favor instruction that executes in
     // specific pipes over more generic ones
     if (LHSPipeCount < RHSPipeCount) return false;
     if (LHSPipeCount > RHSPipeCount) return true; // RHS is better choice
   }

   unsigned LHSNum = LHS->NodeNum;
   unsigned RHSNum = RHS->NodeNum;

   // The most important heuristic is scheduling the critical path
   // so give a high weight for latency.
   LHSCost = (PQ->getLatency(LHSNum) * 10);
   RHSCost = (PQ->getLatency(RHSNum) * 10);

   // If instruction mixes well, multiply the chance of scheduling.
   if (LHSProfit) LHSCost <<= 2;
   if (RHSProfit) RHSCost <<= 2;

   if (LHSCost < RHSCost) return true; // RHS is better choice
   if (LHSCost > RHSCost) return false;

   // After that, favor instruction that executes in
   // specific pipes over more generic ones
   if (LHSPipeCount < RHSPipeCount) return false;
   if (LHSPipeCount > RHSPipeCount) return true; // RHS is better choice

   // After that, if two nodes have identical latencies,
   // look to see if one will unblock more other nodes than the other,
   // so give a high weight for num solely blocked nodes.
   LHSCost = (PQ->getNumSolelyBlockNodes(LHSNum) * 10);
   RHSCost = (PQ->getNumSolelyBlockNodes(RHSNum) * 10);

   // If instruction mixes well, multiply the chance of scheduling.
   if (LHSProfit) LHSCost <<= 2;
   if (RHSProfit) RHSCost <<= 2;

   if (LHSCost < RHSCost) return true;
   if (LHSCost > RHSCost) return false;

   // Favor instruction that executes in specific pipes over more generic ones
   if (LHSPipeCount < RHSPipeCount) return false;
   if (LHSPipeCount > RHSPipeCount) return true;

   // Finally, just to provide a stable ordering, use the node number as a
   // deciding factor.
   return RHSNum < LHSNum;
}

SUnit *LatencyProfitPriorityQueue::pop() {
  // Forward call to LatencyProfitPriorityQueue's base class
  // if ProfitRec not properly initialized.
  if (ProfitRec == NULL)
    return LatencyPriorityQueue::pop();

  if (empty()) return NULL;
  std::vector<SUnit *>::iterator Best = LatencyPriorityQueue::Queue.begin();
  for (std::vector<SUnit *>::iterator I = llvm::next(LatencyPriorityQueue::Queue.begin()),
       E = LatencyPriorityQueue::Queue.end(); I != E; ++I)
    if (ProfitPicker(*Best, *I))
      Best = I;

  SUnit *V = *Best;
  if (Best != prior(LatencyPriorityQueue::Queue.end()))
    std::swap(*Best, LatencyPriorityQueue::Queue.back());
  LatencyPriorityQueue::Queue.pop_back();
  return V;
}

void LatencyProfitPriorityQueue::push(SUnit *SU) {
  LatencyPriorityQueue::push(SU);
}


void LatencyProfitPriorityQueue::remove(SUnit *SU) {
  LatencyPriorityQueue::remove(SU);
}

#ifdef NDEBUG
void LatencyProfitPriorityQueue::dump(ScheduleDAG *DAG) const {}
#else
void LatencyProfitPriorityQueue::dump(ScheduleDAG *DAG) const {
  LatencyPriorityQueue::dump(DAG);
}
#endif
