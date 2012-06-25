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
#include "BStringIterator.h" // has to be first since compilation
                             // braks otherwise. God knows why
#include "ExecEngine.h"
#include "DataPathGraph.h"
#include "WayPointConfigureData.h"
#include "CPUWorkerPool.h"
#include "Coordinator.h"
#include "Constants.h"
#include "Logging.h"
#include "Bitstring.h"

#include "WorkDescription.h"
#include "ExecEngineData.h"
#include "Column.h"
#include "ColumnIterator.cc"
#include "MMappedStorage.h"
#include "BString.h"
#include "BStringIterator.h"

#include <unistd.h>
#include <dlfcn.h>

//#include "JoinMergeWorkFuncs"

// all of the event global event processor classes go here... note that since I
// just have a "pretend" disk setup (where chunks are created entirely using CPU)
// I use CPU workers to handle the "disk"... obviously, this will change when the
// "real" disk is added in
ExecEngine executionEngine;
CPUWorkerPool myCPUWorkers (NUM_EXEC_ENGINE_THREADS);
CPUWorkerPool myDiskWorkers (16);

// global coordinator. Used by waypoints that produce external
// messages such as Print
/* 0 is a bogus parameter to avoid default constructor */

Coordinator globalCoordinator(0);

extern int JoinLHSFunc_WPJoin (WorkDescription &workDescription, ExecEngineData &result);
extern int JoinRHSFunc_WPJoin (WorkDescription &workDescription, ExecEngineData &result);

int main () {

	// Let's create LHS chunks where each chunk contains columns considering first column
	// for every chunk has hash value
		Chunk chunklhs;
{
		for (int j = 0; j < 3; j++) { // columns
			MMappedStorage myStore;
			Column myCol (myStore);
			ColumnIterator <int> myIter (myCol);
			for (int k = j*100, p = 0; p < 5; k++, p++) { // entries in a column
				myIter.Insert (k);
				if (p != 4)
					myIter.Advance ();
			}
			myIter.Done (myCol);
			chunklhs.SwapColumn (myCol, j); // column goes in
		}
		// put BitMap in
		MMappedStorage myStore;
		Column myCol (myStore);
		Bitstring pattern(0xffffffff, true);
		BStringIterator myIter (myCol, pattern, 5);
		//myIter.Done(myCol);
		myIter.Done();

		//chunk.SwapBitmap (myCol);
		chunklhs.SwapBitmap (myIter);
}

		Chunk chunkrhs;
	{
		for (int j = 0; j < 3; j++) { // columns
			MMappedStorage myStore;
			Column myCol (myStore);
			ColumnIterator <int> myIter (myCol);
			for (int k = (j)*100+3, p = 0; p < 5; k++, p++) { // entries in a column
				myIter.Insert (k);
				if (p != 4)
					myIter.Advance ();
			}
			myIter.Done (myCol);
			chunkrhs.SwapColumn (myCol, j); // column goes in
		}
		// put BitMap in
		MMappedStorage myStore;
		Column myCol (myStore);
		Bitstring pattern(0xffffffff, true);
		BStringIterator myIter (myCol, pattern, 5);
		//myIter.Done (myCol);
		myIter.Done ();

		//chunk.SwapBitmap (myCol);
		chunkrhs.SwapBitmap (myIter);
	}

/*
	int waypointID = 0;
	QueryExitContainer qe;
	JoinMergeWorkDescription workDesc (waypointID, qe, lhsChunks, rhsChunks);
	WorkFunc myFunc = GetWorkFunction (JoinMergeWorkFunc::type);
	WayPointID myID;
	HistoryList history;
	QueryExitContainer dest;
	ExecEngineData _data;
	HoppingDataMsg data (myID, dest, history, _data);
	myCPUWorkers.DoSomeWork (myID, data.get_lineage (), data.get_dest (), myToken, workDesc, myFunc);
*/


	int waypointID = 0;
	QueryID query(0xffffffff, true);
	WayPointID exit("joinwp_test");
	QueryExit qe(query, exit);
	QueryExitContainer qec;
	qec.Append(qe);
	HashTable centralHashTable;
	centralHashTable.Allocate (8);
	JoinRHSWorkDescription workDesc (waypointID, qec, chunkrhs, centralHashTable);
	ChunkContainer result;
	JoinRHSFunc_WPJoin (workDesc, result);

	int waypointID1 = 0;
	QueryID query1(0xffffffff, true);
	WayPointID exit1("joinwp1_test");
	QueryExit qe1(query1, exit1);
	QueryExitContainer qec1;
	qec1.Append(qe1);
	//ExecEngineData result1;
	ChunkContainer result1;
	JoinLHSWorkDescription workDesc1 (waypointID1, qec1, chunklhs, centralHashTable);
	//JoinMergeFunc (workDesc, result);
	JoinLHSFunc_WPJoin (workDesc1, result1);
//	((WorkFunc) dlsym(module, "JoinMergeFunc_WPJoin"))(workDesc, result);
	Chunk& rezChk = result1.get_myChunk();
	//Column bitMap;
	//rezChk.SwapBitmap(bitMap);
	//BStringIterator BIter(bitMap);
	BStringIterator BIter;
	rezChk.SwapBitmap(BIter);
	vector<ColumnIterator<int> > ColVec;
	ColVec.resize(rezChk.GetNumOfColumns());
	for (int i = 0; i < rezChk.GetNumOfColumns(); i++) {
		Column Col;
		rezChk.SwapColumn (Col, i);
		ColumnIterator<int> iter(Col);
		ColVec[i].swap(iter);
	}

	//while (!BIter.AtUnwrittenByte()) { // TBD fix
	while (!ColVec[0].AtUnwrittenByte()) {
		if (!(BIter.GetCurrent() == 0)) {
			for (int j = 0; j < rezChk.GetNumOfColumns(); j++) {
				cout << " " << ColVec[j].GetCurrent();
				ColVec[j].Advance();
			}
			cout << "\n";
		}
		BIter.Advance();
	}
	return 0;
}