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
#ifndef QUERYMANAGER_H
#define QUERYMANAGER_H

#include <map>
#include <set>
#include <pthread.h>
#include <string>
#include <fstream>

#include "ID.h"

using namespace std;


class QueryManager{

#include "QueryManagerPrivate.h"

public:
    // This is the class that manages the query and the mapping.
    // For each new query inserted, the query manager assign a unique bit for this query
    // This information is prepared for other components to ask.
    // This is the only class that creates unique QueryID's for queries in the system.

    // Returns the single instance of QueryManager.
    static QueryManager &GetQueryManager();

    // Empty  destructor
    virtual ~QueryManager();


    // Add new query into the system, input is the queryname and
    // returns the queryid in returnMe
    // fails if the query was deleted before
    bool AddNewQuery(string queryName, QueryID& returnMe);

    // Removes query from system
    bool RemoveQuery(string queryName);

    // returns true if query is valid. It returns false for all deleted queries
    bool IsValid(string queryName);

    // Ask which bit a query maps to . the bit position is returned if QID is in the system,
    // otherwise an error messenge will be printed and the program will exit.
    int QueryBitIndex(QueryID queryMe);

    // get the query ID from name
    // if the query is not registered returns false
    // otherwise fillMe is set an true is returned
    // if the query is not found the code should FAIL
    QueryID GetQueryID(string queryName);
    QueryID GetQueryID(const char* name);
    // the oposite question
    // return false if not a valid QID
    bool GetQueryName(QueryID query, string& fillMe);

    // This method writes all the query information in an M4 macro file
    // for the code generateor to use. It is supposed to be called only by
    // the code generator, who knows exactly the output filename.
    void GenerateM4Files(string fullFileName);
};

#endif
