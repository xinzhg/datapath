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
#ifndef _LT_GIST_H_
#define _LT_GIST_H_

#include "LT_Waypoint.h"
#include "GLAData.h"

class LT_GIST : public LT_Waypoint {

    typedef std::vector<WayPointID> StateSourceVec;

    struct GISTInfo {
        std::string name;
        std::string defs;
        std::string constructExp;
        StateSourceVec reqStates;
        bool retState;

        GISTInfo() {}
        GISTInfo( std::string& _name, std::string& _defs, std::string& _constructExp,
                StateSourceVec& _reqStates, bool _retState ) :
            name(_name), defs(_defs), constructExp(_constructExp), reqStates(_reqStates),
            retState(_retState) {
        }
    };

    // map from query to output attributes
    typedef EfficientMap<QueryID, SlotContainer> QueryToSlotContainer;
    QueryToSlotContainer gistAttribs;

    // info for each GIST
    typedef std::map<QueryID, GISTInfo> GISTInfoMap;
    GISTInfoMap gistInfoMap;

    QueryToSlotSet synthesized;

    void WriteM4DataStructures(ostream& out);

public:

    LT_GIST(WayPointID id) : LT_Waypoint(id)
    {}

    virtual WaypointType GetType() {return GISTWayPoint;}

    virtual void DeleteQuery(QueryID query);

    virtual void ClearAllDataStructure();

    // GIST, one per query basis
    virtual bool AddGIST(QueryID query,
            SlotContainer& resultAtts,
            std::string gistName,
            std::string gistDef,
            std::string constructorExp,
            StateSourceVec reqStates,
            bool retState);

    virtual bool PropagateDown(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe);

    virtual bool PropagateDownTerminating(QueryID query, const SlotSet& atts /*blank*/, SlotSet& result,
            QueryExit qe);

    virtual bool PropagateUp(QueryToSlotSet& result);

    virtual void WriteM4File(ostream& out);

    virtual bool GetConfig(WayPointConfigureData& where);
};

#endif // _LT_GIST_H_
