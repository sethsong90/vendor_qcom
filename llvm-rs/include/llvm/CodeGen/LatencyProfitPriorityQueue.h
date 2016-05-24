// Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//
//===----------------------------------------------------------------------===//
//
// This file declares the LatencyProfitPriorityQueue class, which is a
// SchedulingPriorityQueue that schedules using latency information to
// reduce the length of the critical path through the basic block and
// pipeline instruction mixing profitability checks.
//
//===----------------------------------------------------------------------===//

#ifndef LATENCY_PROFIT_PRIORITY_QUEUE_H
#define LATENCY_PROFIT_PRIORITY_QUEUE_H

#include "llvm/CodeGen/LatencyPriorityQueue.h"
#include "llvm/CodeGen/ScheduleProfitRecognizer.h"
#include "llvm/CodeGen/ScheduleDAG.h"

namespace llvm {
  class LatencyProfitPriorityQueue;

  /// Sorting functions for the Available queue.
  struct latency_profit_sort : public std::binary_function<SUnit*, SUnit*, bool> {
    LatencyProfitPriorityQueue *PQ;
    explicit latency_profit_sort(LatencyProfitPriorityQueue *pq) : PQ(pq) {}

    bool operator()(const SUnit* left, const SUnit* right) const;
  };

  class LatencyProfitPriorityQueue : public LatencyPriorityQueue {
    latency_profit_sort ProfitPicker;

    // ProfitRec - The profit recognizer to use.
    ScheduleProfitRecognizer *ProfitRec;

  public:
    LatencyProfitPriorityQueue() : ProfitPicker(this), ProfitRec(0) {
    }

    virtual void push(SUnit *U);

    virtual SUnit *pop();

    virtual void remove(SUnit *SU);

    virtual void dump(ScheduleDAG* DAG) const;

    ScheduleProfitRecognizer *getProfitRec(){
      return ProfitRec;
    }
    void setProfitRec(ScheduleProfitRecognizer* PR){
      ProfitRec = PR;
    }
  };
}

#endif
