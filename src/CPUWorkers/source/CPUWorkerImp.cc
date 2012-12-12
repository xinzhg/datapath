//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#include <vector>

#include "CPUWorkerImp.h"
#include "CPUWorkerPool.h"
#include "EEExternMessages.h"
#include "ExecEngine.h"
#include "ExecEngineImp.h"
#include "Profiling.h"
#include "Logging.h"
#include "Diagnose.h"

/** How oftern the system should have context swithes? Need this to determine if we have
  too many context switches thus we should be worried */
#ifndef HZ // linux defines it in asm/param.h
#define HZ 100
#endif

/** If the list of performance counters to watch changes, mofify this

  Make sure the list size is set correctly as well
  */

#ifdef PER_CPU_PROFILE
static const PerfCounter::EventType eventsPC[] = {
    PerfCounter::Cycles,
    PerfCounter::Instructions,
    PerfCounter::Branch_Instructions,
    PerfCounter::Branch_Misses,
    PerfCounter::Cache_References,
    PerfCounter::Cache_Misses,
    PerfCounter::Context_Switches,
    PerfCounter::Task_Clock
};

static const size_t eventsPC_size = 8;
#endif // PER_CPU_PROFILE

///////////////////// NO SYSTEM HEADERS SHOULD BE INCLUDED BEYOND THIS POINT ////////////////////

void CPUWorkerImp :: GetCopyOf (EventProcessor &myParent){
    me.copy (myParent);
}

CPUWorkerImp :: CPUWorkerImp ()
{

    // register the DoSomeWork method
    RegisterMessageProcessor (WorkRequestMsg :: type, &DoSomeWork, 1);
}

CPUWorkerImp :: ~CPUWorkerImp () {}

MESSAGE_HANDLER_DEFINITION_BEGIN(CPUWorkerImp, DoSomeWork, WorkRequestMsg) {

    // this is where the result of the computation will go
    ExecEngineData computationResult;

    LOG_ENTRY_P(1, " Function of waypoint %s started\n", msg.currentPos.getName().c_str());
    DIAG_ID dID = DIAGNOSE_ENTRY("CPUWORKER", msg.currentPos.getName().c_str(), "CPUWORK");
    uint64_t effort = PREFERED_TUPLES_PER_CHUNK; // function fills in the effort

#ifdef PER_CPU_PROFILE
    // Start performance counters
    std::vector<PerfCounter> counters(eventsPC_size);
    for( size_t i = 0; i < eventsPC_size; ++i ) {
        PerfCounter cnt(eventsPC[i]);
        counters[i].Swap(cnt);
    }
    PROFILING2_START;
#endif // PER_CPU_PROFILE

    // now, call the work function to actually produce the output data
    int returnVal = msg.myFunc (msg.workDescription, computationResult);

#ifdef PER_CPU_PROFILE
    PROFILING2_END;
    // Read performance counters
    PCounterList waypointList;
    const string globalGroup("perf global");
    const string waypointGroup = "perf " + msg.currentPos.getName();
    for( size_t i = 0; i < eventsPC_size; ++i ) {
        // Create one counter for global aggregation and one for waypoint aggregation
        std::string name = PerfCounter::names[eventsPC[i]];
        int64_t count = counters[i].GetCount();
        PCounter globalCnt(name, count, globalGroup);
        PCounter waypointCnt(name, count, waypointGroup);

        waypointList.Append(globalCnt);
        waypointList.Append(waypointCnt);
    }

    PROFILING2_SET(waypointList);
#endif // PER_CPU_PROFILE

    DIAGNOSE_EXIT(dID);

    // and finally, store outselves in the queue for future use
    CPUWorker me;
    me.copy(evProc.me);
    if (CHECK_DATA_TYPE(msg.token, CPUWorkToken)) {
        myCPUWorkers.AddWorker (me);
    } else if (CHECK_DATA_TYPE(msg.token, DiskWorkToken)) {
        myDiskWorkers.AddWorker (me);
    } else
        FATAL ("Strange work token type!\n");

    // now, send the result back
    // first, create the object that will have the result
    HoppingDataMsg result (msg.currentPos, msg.dest, msg.lineage, computationResult);

    //printf("\nData msg from MESSAGE_HANDLER_DEFINITION_BEGIN(CPUWorkerImp, DoSomeWork, WorkRequestMsg) sent");
    // and send it
    HoppingDataMsgMessage_Factory (executionEngine, returnVal, msg.token, result);


}MESSAGE_HANDLER_DEFINITION_END
