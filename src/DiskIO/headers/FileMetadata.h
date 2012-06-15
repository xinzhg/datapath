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
// for Emacs -*- c++ -*-

#ifndef _FILEMETADATA_H
#define _FILEMETADATA_H

#include <vector>
#include <assert.h>
#include "Errors.h"
#include "Swap.h"
#include<stdio.h>

using namespace std;

/**
	The metadata is kept in the following relations:

	Relations -- info on relations such as name, etc
	relID:int, relName:text,  numColumns:int, freeChunkID:int
	Chunks -- info on chunks (what relation they belong to, etc.)
	chunkID:int, relID:int, numTuples:int
	Columns -- info on columns/chunk (storage)
	colNo:int, relID:int, chunkID:int, startPage:int, sizeInPages:int, columnType:int,
	varStartPage:int

	**/

// This is a class that encapsulates file metadata information. We
// decided to avoid the troublesome task of having this information in
// the start of the striped files. Instead, we will have them in a
// separate file, leaving the striped files with only data.

/**
	NOTE: the metadata file does not decide the layout of the data on the
	flat view of the relation representation, it only keeps track of the metadata
 */


/**
	This keeps fragment metadata information which is property of every column or chunk. Conceptually, every chunk
	consists of several fragments (some fixed constant). ChunkID is improved to keep range of fragments (IDs).
	Fragment ID is conceptually from 0 to some constant, hence vector index will suffice to point to it.
 */
class Fragments {

	public:
		friend class FileMetadata;
		friend class ChunkMetaD;
		friend class ColumnMetaData;

		// This will be filled while writing chunks as well as we will get back while reading them
		vector<int> startPositions;

	public:

		Fragments() {}
		Fragments (const Fragments& o) {
			startPositions = o.startPositions;
		}
		~Fragments () {startPositions.clear();}

		Fragments& operator=(const Fragments& o) {
			startPositions = o.startPositions;
			return *this;
		}

		// This will be used while writing of chunks to mark them
		void MarkFragment (int curPosInColumn);

		// swap
		void swap (Fragments& withMe);

		// Load from disk
	// method is incremental (all positions have to be inserted in order
		void Initialize(long int _startPos){	startPositions.push_back(_startPos); }

		// Get start position of column of the given fragment
		int GetStartPosition (int fragmentIndex);

		// Get end position of column of the given fragment
		int GetEndPosition (int fragmentIndex);

		bool IsValid() {return !startPositions.empty();}	

		int GetNumFragments() {return startPositions.size();}
};

class FragmentsTuples {

	public:
		friend class FileMetadata;
		friend class ChunkMetaD;
		friend class ColumnMetaData;

		// tuples count
		vector<int> tuplesCount;

		// This contains object length for bitstring
		__uint64_t header1;

		__uint64_t header2;

	public:

		FragmentsTuples() {}

		// Load from disk
	// incremental (counts provided in order)
		void Initialize(long int _tpCount){	tuplesCount.push_back(_tpCount); }

		FragmentsTuples (const FragmentsTuples& _t) {
			tuplesCount = _t.tuplesCount;
			header1 = _t.header1;
			header2 = _t.header2;
		}

		FragmentsTuples& operator=(const FragmentsTuples& _t) {
			tuplesCount = _t.tuplesCount;
			header1 = _t.header1;
			header2 = _t.header2;
		}

		// swap
		void swap (FragmentsTuples& withMe);

		// This will be used while writing of chunks to mark them
		void SetTuplesCount (int tuplesCount);

		void SetHeader1 (__uint64_t _header1);
		__uint64_t GetHeader1 ();

		void SetHeader2 (__uint64_t _header2);
		__uint64_t GetHeader2 ();

		int GetTupleCount (int fragmentIndex);

		int GetTupleCount (int fragmentIndexStart, int fragmentIndexEnd);

		int GetOverallTupleCount();

		void PrintTupleCount ();
};

class ColumnMetaData {

private:
	friend class FileMetadata;
	friend class ChunkMetaD;

	int startPage;
	int sizePages;
	int startPageCompr;
	int sizePagesCompr;
	int sizeBytes;
	int sizeBytesCompr;
	Fragments fragments;

public:

		ColumnMetaData (int _startPage = -1,
										int _sizePages = -1,
										int _startPageCompr = -1,
										int _sizePagesCompr = -1,
										int _sizeBytes = -1,
										int _sizeBytesCompr = -1) :
										startPage(_startPage),
									 	sizePages(_sizePages),
										startPageCompr(_startPageCompr),
										sizePagesCompr(_sizePagesCompr),
										sizeBytes(_sizeBytes),
										sizeBytesCompr(_sizeBytesCompr) {}

		ColumnMetaData (Fragments& _fragments, int _startPage = -1,
										int _sizePages = -1,
										int _startPageCompr = -1,
										int _sizePagesCompr = -1,
										int _sizeBytes = -1,
										int _sizeBytesCompr = -1) :
										startPage(_startPage),
									 	sizePages(_sizePages),
										startPageCompr(_startPageCompr),
										sizePagesCompr(_sizePagesCompr),
										sizeBytes(_sizeBytes),
										sizeBytesCompr(_sizeBytesCompr),
										fragments(_fragments) {}

		// Load from disk
		void Initialize(long int _startPage, long int _sizePages,
										long int _sizeBytes, long int _startPageCompr,
										long int _sizePagesCompr, long int _sizeBytesCompr);
			
		// Returns the starting page for a given chunk and column.
		off_t getStartPage();
		off_t getStartPageCompr();

		// Returns the size in pages for a given chunk and column.
		off_t getSizePages();
		off_t getSizePagesCompr();

		// Returns the size in bytes for a given chunk and column.
		off_t getSizeBytes();
		off_t getSizeBytesCompr();

		Fragments& getFragments();
};

class ChunkMetaD {
private:
	friend class FileMetadata;

	int numTuples;
	vector<ColumnMetaData> colMetaData;
	FragmentsTuples fragTuple;

public:

		// For STL
		ChunkMetaD (FragmentsTuples& f, int _numTuples = 0) : fragTuple(f), numTuples(_numTuples) {};
		ChunkMetaD (int _numTuples = 0) : numTuples(_numTuples) {}; // For STL resize

		// Load from disk
	void Initialize(int _numCols, long int _numTuples);

		// Returns the starting page for a given chunk and column.
		off_t getStartPage(unsigned long numCol);
		off_t getStartPageCompr(unsigned long numCol);

		// Returns the size in pages for a given chunk and column.
		off_t getSizePages(unsigned long numCol);
		off_t getSizePagesCompr(unsigned long numCol);

		// Returns the size in bytes for a given chunk and column.
		off_t getSizeBytes(unsigned long numCol);
		off_t getSizeBytesCompr(unsigned long numCol);

		int getNumTuples ();
		Fragments& getFragments(unsigned long numCol);
		FragmentsTuples& getFragmentsTuples() {return fragTuple;}

		// adds a column (must be done in order)
		void addColumn(
				off_t _startPage, 
				off_t _sizeBytes,
				off_t _sizePages,
				off_t _startPageCompr,
				off_t _sizeByesCompr,
				off_t _sizePagesCompr,
				Fragments& _fragments);
};

class FileMetadata {
	private:
		//relation name
		char *relName;

		// the relation ID. This ID has to be unique in the system
		int relID;
  
  // is the relation new?
  bool newRelation;
  

		// number of columns.`
		off_t numCols;

		// number of chunks.
		off_t numChunks;

		bool modified; // set true if we need to write the metadata

		// Chunk metadata, in order
		vector<ChunkMetaD> chunkMetaD;

		// what chunk is filled currently
		// if = -1, no chunk is
		long chkFilled;

		// how many columns have been filled
		long colsFilled;

	public:
		// constructor
		// if relation does not exist, it will be creaed
		// if it exists, we check that it has numCols columns
		// whoever calls this should know how many columns we are supposed
		// to have
		FileMetadata(const char* _relName, int _numCols);

		// Destructor.
		virtual ~FileMetadata();

		// returns the relationID. Unique throught the system
		int getRelID();

		// Returns the number of columns on each chunk.
		unsigned long getNumCols();

		// Returns the number of chunks stored on disk.
		off_t getNumChunks();

		// Returns the number of tuples for a given chunk.
		off_t getNumTuples(off_t numChunk);

		// Returns the starting page for a given chunk and column.
		off_t getStartPage(off_t numChunk, unsigned long numCol);
		off_t getStartPageCompr(off_t numChunk, unsigned long numCol);

		// Returns the size in pages for a given chunk and column.
		off_t getSizePages(off_t numChunk, unsigned long numCol);
		off_t getSizePagesCompr(off_t numChunk, unsigned long numCol);

		// Returns the size in bytes for a given chunk and column.
		off_t getSizeBytes(off_t numChunk, unsigned long numCol);
		off_t getSizeBytesCompr(off_t numChunk, unsigned long numCol);

		Fragments& getFragments(off_t numChunk, unsigned long numCol);
		FragmentsTuples& getFragmentsTuples(off_t numChunk);

		/** Methods to add a new chunk

			In order to avoid exposing the internal structure of the metadata file,
			we provide methods to add parts of a chunk incrementaly.

			Chunks are added at the end. Only the metadata file knows where
			the end is.

			Columns have to be added in order.

		 */
		// need to call this before info is put in the new chunk
		off_t startNewChunk(off_t _numTuples, off_t _numColumns, FragmentsTuples& f);

		// call this when the chunk is finished
		void finishedChunk(void);

		// set the number of tuples for the chunk constructed
		void setNumTuples(off_t _numTuples);

		// adds a column (must be done in order)
		void addColumn(off_t _startPage, 
				off_t _sizeBytes,
				off_t _sizePages,
				off_t _startPageCompr,
				off_t _sizeByesCompr,
				off_t _sizePagesCompr,
				Fragments& _fragments);

		// reserve pages in the storage; the return is the index of the first page
		// in the sequence
		// this method needs to be called before inserting any column in a chunk
		off_t allocatePages(off_t _noPages);

		// saving the content of the metadata file
		// if the file specified exists, it is erased
		void Flush(void);

		void Print ();

};

// INLINED METHODS!

// ===========================INLINE methods for Fragments =================
inline
void Fragments :: MarkFragment (int curPosInColumn) {

	for (int i = 0; i < startPositions.size(); i++) {
		//printf("\n curPosInColumn = %d, startPositions[%d] = %d", curPosInColumn, i, startPositions[i]); fflush(stdout);
		assert(curPosInColumn >= startPositions[i]);
	}
	startPositions.push_back(curPosInColumn);
}

inline
void Fragments :: swap (Fragments& withMe) {

	startPositions.swap (withMe.startPositions);
}

inline
int Fragments :: GetStartPosition (int fragmentIndex) {

  FATALIF(!(fragmentIndex >= 0 && fragmentIndex < startPositions.size()), "FragmentIndex=%d out of range", fragmentIndex);

	return startPositions[fragmentIndex];
}

inline
int Fragments :: GetEndPosition (int fragmentIndex) {

	assert(0); // function not needed for now
	assert(fragmentIndex >= 0);
	return startPositions[fragmentIndex];
}

// ===========================INLINE methods for FragmentsTuples =================
inline
void FragmentsTuples :: SetTuplesCount (int _tuplesCount) {

	tuplesCount.push_back(_tuplesCount);
}
inline
void FragmentsTuples :: swap (FragmentsTuples& withMe) {

	tuplesCount.swap (withMe.tuplesCount);
	SWAP(header1, withMe.header1);
	SWAP(header2, withMe.header2);
}

inline
void FragmentsTuples :: SetHeader1 (__uint64_t _header1) {
	header1 = _header1;
}

inline
__uint64_t FragmentsTuples :: GetHeader1 () {
	return header1;
}

inline
__uint64_t FragmentsTuples :: GetHeader2 () {
	return header2;
}

inline
void FragmentsTuples :: SetHeader2 (__uint64_t _header2) {
	_header2 = _header2;
}

inline
int FragmentsTuples :: GetTupleCount (int fragmentIndex) {
	assert(fragmentIndex >= 0);
	assert(fragmentIndex < tuplesCount.size());

	if (fragmentIndex == 0)
		return tuplesCount[0];
	else
		return tuplesCount[fragmentIndex] - tuplesCount[fragmentIndex-1];
}

inline
int FragmentsTuples :: GetTupleCount (int fragmentIndexStart, int fragmentIndexEnd) {
	assert(fragmentIndexStart >= 0);
	assert(fragmentIndexEnd >= 0);
	assert(fragmentIndexEnd >= fragmentIndexStart);
	if (fragmentIndexEnd > tuplesCount.size()-1)
		fragmentIndexEnd = tuplesCount.size()-1;

	int cnt = 0;
	for (int i = fragmentIndexStart; i <= fragmentIndexEnd; i++) {
		cnt += tuplesCount[i];
	}
	return cnt;
}

inline
int FragmentsTuples :: GetOverallTupleCount () {
	int total = 0;
	for (int i = 0; i < tuplesCount.size(); i++) {
		//printf("\n %d",tuplesCount[i]);
		total += tuplesCount[i];
	}
	return total;
}

inline
void FragmentsTuples :: PrintTupleCount() {
	printf("(((");
	for (int i = 0; i < tuplesCount.size(); i++) {
		printf("%d, ",tuplesCount[i]);
	}
	printf(")))");
}


// ===========================INLINE methods for ColumnMetaData =================
inline
void ColumnMetaData::Initialize(long int _startPage, long int _sizePages,
																long int _sizeBytes, long int _startPageCompr,
																long int _sizePagesCompr, long int _sizeBytesCompr){

			startPage = _startPage;
			startPageCompr = _startPageCompr;
			sizePages = _sizePages;
			sizePagesCompr = _sizePagesCompr;
			sizeBytes = _sizeBytes;
			sizeBytesCompr = _sizeBytesCompr;
}

inline
off_t ColumnMetaData::getStartPage() {
	return startPage;
}

inline
off_t ColumnMetaData::getStartPageCompr() {
	return startPageCompr;
}

inline
off_t ColumnMetaData::getSizePages() {
	return sizePages;
}

inline
off_t ColumnMetaData::getSizePagesCompr() {
	return sizePagesCompr;
}

inline
off_t ColumnMetaData::getSizeBytes() {
	return sizeBytes;
}

inline
off_t ColumnMetaData::getSizeBytesCompr() {
	return sizeBytesCompr;
}

inline
Fragments& ColumnMetaData::getFragments(){
	return fragments;
}
// ===========================INLINE methods for ChunkMetaData =================
inline 
void ChunkMetaD :: Initialize (int _numCols, long int _numTuples) {
	colMetaData.resize(_numCols);
	numTuples=_numTuples;
}


inline off_t ChunkMetaD::getStartPage(unsigned long numCol) {
#ifdef DEBUG
	assert(numCol < colMetaData.size());
#endif
	return colMetaData[numCol].getStartPage();
}

inline off_t ChunkMetaD::getStartPageCompr(unsigned long numCol) {
#ifdef DEBUG
	assert(numCol < colMetaData.size());
#endif
	return colMetaData[numCol].getStartPageCompr();
}

inline off_t ChunkMetaD::getSizePages(unsigned long numCol) {
#ifdef DEBUG
	assert(numCol < colMetaData.size());
#endif
	return colMetaData[numCol].getSizePages();
}

inline off_t ChunkMetaD::getSizePagesCompr(unsigned long numCol) {
#ifdef DEBUG
	assert(numCol < colMetaData.size());
#endif
	return colMetaData[numCol].getSizePagesCompr();
}

inline off_t ChunkMetaD::getSizeBytes(unsigned long numCol) {
#ifdef DEBUG
	assert(numCol < colMetaData.size());
#endif
	return colMetaData[numCol].getSizeBytes();
}

inline off_t ChunkMetaD::getSizeBytesCompr(unsigned long numCol) {
#ifdef DEBUG
	assert(numCol < colMetaData.size());
#endif
	return colMetaData[numCol].getSizeBytesCompr();
}

inline Fragments& ChunkMetaD::getFragments(unsigned long numCol) {
#ifdef DEBUG
	assert(numCol < colMetaData.size());
#endif
	return colMetaData[numCol].getFragments();
}

inline int ChunkMetaD::getNumTuples () {
	return numTuples;
}

inline void ChunkMetaD::addColumn(
				off_t _startPage, 
				off_t _sizeBytes,
				off_t _sizePages,
				off_t _startPageCompr,
				off_t _sizeBytesCompr,
				off_t _sizePagesCompr,
				Fragments& _fragments) {

	ColumnMetaData col (_fragments, _startPage, _sizePages, _startPageCompr, _sizePagesCompr, _sizeBytes, _sizeBytesCompr);
	colMetaData.push_back(col);
}

// ===========================INLINE methods for FileMetaData =================
inline int FileMetadata::getRelID(void){ return relID; }

inline unsigned long FileMetadata::getNumCols() {
	return(numCols);
}

inline off_t FileMetadata::getNumChunks() {
	return(numChunks);
}

inline off_t FileMetadata::getNumTuples(off_t numChunk) {
#ifdef DEBUG
	assert(numChunk < chunkMetaD.size());
#endif
	return chunkMetaD[numChunk].getNumTuples();
}

inline off_t FileMetadata::getStartPage(off_t numChunk, unsigned long numCol) {
#ifdef DEBUG
	assert(numChunk < chunkMetaD.size());
#endif
	return chunkMetaD[numChunk].getStartPage(numCol);
}

inline off_t FileMetadata::getStartPageCompr(off_t numChunk, unsigned long numCol) {
#ifdef DEBUG
	assert(numChunk < chunkMetaD.size());
#endif
	return chunkMetaD[numChunk].getStartPageCompr(numCol);
}

inline off_t FileMetadata::getSizePages(off_t numChunk, unsigned long numCol) {
#ifdef DEBUG
	assert(numChunk < chunkMetaD.size());
#endif
	return chunkMetaD[numChunk].getSizePages(numCol);
}

inline off_t FileMetadata::getSizePagesCompr(off_t numChunk, unsigned long numCol) {
#ifdef DEBUG
	assert(numChunk < chunkMetaD.size());
#endif
	return chunkMetaD[numChunk].getSizePagesCompr(numCol);
}

inline off_t FileMetadata::getSizeBytes(off_t numChunk, unsigned long numCol) {
#ifdef DEBUG
	assert(numChunk < chunkMetaD.size());
#endif
	return chunkMetaD[numChunk].getSizeBytes(numCol);
}

inline off_t FileMetadata::getSizeBytesCompr(off_t numChunk, unsigned long numCol) {
#ifdef DEBUG
	assert(numChunk < chunkMetaD.size());
#endif
	return chunkMetaD[numChunk].getSizeBytesCompr(numCol);
}

inline Fragments& FileMetadata::getFragments(off_t numChunk, unsigned long numCol) {
#ifdef DEBUG
	assert(numChunk < chunkMetaD.size());
#endif
	return chunkMetaD[numChunk].getFragments(numCol);
}

inline FragmentsTuples& FileMetadata::getFragmentsTuples(off_t numChunk) {
#ifdef DEBUG
	assert(numChunk < chunkMetaD.size());
#endif
/*
	size_t total = 0;
	for (int i = 0; i < chunkMetaD.size(); i++) {
		total = total + (chunkMetaD[i].getFragmentsTuples().GetOverallTupleCount());
	}
	printf("\n ---------     %ld", total); fflush(stdout);
*/
	return chunkMetaD[numChunk].getFragmentsTuples();
}

inline off_t FileMetadata::startNewChunk(off_t _numTuples, off_t _numColumns, FragmentsTuples& f){

	assert (chkFilled == -1);
	assert (numChunks == chunkMetaD.size());

	chkFilled = numChunks; // Keep it to old value for now, increment it once we finish adding
	colsFilled=0; // We still need to start adding columns

	// Add new chunk
	//ChunkMetaD chunk(numChunks + 1);
	ChunkMetaD chunk(f, _numTuples);
	chunkMetaD.push_back (chunk);

	modified = true; // to make sure we write the metadata
	numCols = _numColumns;

	return chkFilled;
}

inline void FileMetadata::finishedChunk(void){
	// Make sure we added all columns as received in FileMetaData constructor, i.e. addColumn
	// is called that many number of times
	assert (chkFilled == numChunks && colsFilled == numCols);
	numChunks++;
	chkFilled=-1;
}

inline void FileMetadata::addColumn(off_t _startPage, 
		off_t _sizeBytes,
		off_t _sizePages,
		off_t _startPageCompr,
		off_t _sizeBytesCompr,
		off_t _sizePagesCompr,
		Fragments& _fragments) {

	assert (chkFilled == numChunks && colsFilled < numCols);

	// We add columns to last newly added chunk
	chunkMetaD[chunkMetaD.size()-1].addColumn (_startPage, _sizeBytes, _sizePages, _startPageCompr, _sizeBytesCompr, _sizePagesCompr, _fragments);

	colsFilled++;
}

#endif // _FILEMEDATATA_H
