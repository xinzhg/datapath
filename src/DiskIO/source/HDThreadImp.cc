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
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h> /* usleep() */
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "Errors.h"
#include "HDThread.h"
#include "DistributedCounter.h"
#include "Timer.h"
#include "DiskArray.h"
#include "MmapAllocator.h"
#include "Profiling.h"

#ifndef O_DIRECT
# define O_LARGEFILE 0100000
# define O_DIRECT 040000 /* Direct disk access.  */
#endif

using namespace std;


HDThreadImp::HDThreadImp(char *_fileName, int _diskNo, EventProcessor &_dispatcher,
                         int _frequencyUpdate, bool _isReadOnly) :
#ifdef DEBUG_EVPROC
	EventProcessorImp(true, _fileName),
#endif
	diskArray(DiskArray::GetDiskArray()),
	alpha(1.0/_frequencyUpdate)
{
	diskNo = _diskNo;
	fileName = strdup(_fileName);
	diskArray.copy(_dispatcher);

	isReadOnly=_isReadOnly;

	// statistics
	frequencyUpdate = _frequencyUpdate;
	exp = 0.0;
	exp2 = 0.0;
	counter = 0;

	int options = isReadOnly ? (O_RDONLY | O_CREAT | O_LARGEFILE) 
		:(O_RDWR | O_CREAT | O_LARGEFILE);

#ifdef MMAP_IS_MALLOC // unoptimized IO to allow missaligned malloc pages
	fileDescriptor = open (fileName,  options, S_IRUSR | S_IWUSR);
#warning "Using UNOPTIMIZED disk read"
#else // normal, optimized IO
	fileDescriptor = open (fileName,  options | O_DIRECT , S_IRUSR | S_IWUSR);
#endif MMAP_IS_MALLOC

	if (fileDescriptor == -1){
		perror("HDThread:");
		FATAL("Error in HDThreads(%s)\n", _fileName);
	}

	RegisterMessageProcessor(MegaJob::type, &HDThreadImp::ExecuteJob, 1);
}


HDThreadImp::~HDThreadImp() {
  // WARNING("You should not shut down the disk array");

	free(fileName);
	close(fileDescriptor);
}

void HDThreadImp::UpdateStatistics(double time){
	// we got the time for a page
	exp = time*alpha+exp*(1-alpha);
	exp2 = time*alpha+exp2*(1-alpha);
	counter++;

	if (counter % frequencyUpdate == 0){
		// send a message to the dispatcher
		DiskStatistics_Factory(diskArray, diskNo, exp, exp2-exp*exp);
	}
}

//thread for each HD
//the parameter is a HDThreadParam struct
MESSAGE_HANDLER_DEFINITION_BEGIN(HDThreadImp, ExecuteJob, MegaJob){

	FATALIF(!msg.requestor.IsValid(), "Requestor passed in DiskArray is not valid");

	// main loop over the pages in the request
	for(msg.requests.MoveToStart(); !msg.requests.AtEnd(); msg.requests.Advance()){
		DiskRequestData& request = msg.requests.Current();
		off_t page = request.get_startPage();
		off_t numPG = request.get_sizePages();
		void* where = request.get_memLoc();

//		cout << "Disk " << evProc.fileName << " ID=" << msg.requestId << " accesses page:" << page << " numPG:" << numPG << " pointer:" << where << endl;

		// we do a lock on the dir here to make sure no other thread
		// accesses the same disk

		// pthread_mutex_lock(evProc.myDirLock);
		// printf("Locked %p in %s\n", evProc.myDirLock, evProc.fileName);

		Timer clock;
		clock.Restart();

		//first go to the correct location
		lseek (evProc.fileDescriptor, PAGES_TO_BYTES(page), SEEK_SET);

		// now perform the operation
		if (msg.operation == WRITE) {
			if (write (evProc.fileDescriptor, where, PAGES_TO_BYTES(numPG) ) == -1){
				perror("HDThread:");
				FATAL("Writting of file %s at position %ld of size %ld for job %d failed. Mem: %lx", 
							evProc.fileName, page, PAGES_TO_BYTES(numPG),  (int)msg.requestId, where);
			} else {
			  PROFILING2("diskW", PAGES_TO_BYTES(numPG));
			}
		}
		else  if (msg.operation == READ) {
			if (read (evProc.fileDescriptor, where, PAGES_TO_BYTES(numPG)) == -1) {
				perror("HDThread:");
				FATAL("Reading of file %s at position %d of size %d for job %d failed. Mem: %lx", evProc.fileName, (int)page, PAGES_TO_BYTES(numPG), (int)msg.requestId, where);
			} else {
			  PROFILING2("diskR", PAGES_TO_BYTES(numPG));
			}

		}
		else {
			FATAL("Invalid operation type(%d) specified\n",msg.operation);
		}

		evProc.UpdateStatistics(clock.GetTime()/numPG);
	}

	PROFILING2_FLUSH;

	//signal the calling thread if these are the last pages to read/write
	if (msg.counter->Decrement(1) == 0) { // decrease the number of threads that finished
		// last piece, signal ChunkReaderWriter
		MegaJobFinished_Factory(msg.requestor, msg.requestId, msg.operation, msg.counter);
	}
}MESSAGE_HANDLER_DEFINITION_END
