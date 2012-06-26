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
#include "LT_Join.h"
#include "AttributeManager.h"
#include <assert.h>

extern HashTable centralHashTable;

static string GetAllAttrAsString(const set<SlotID>& atts) {
        string rez;
        for (set<SlotID>::iterator it = atts.begin(); it != atts.end(); it++) {
                SlotID slot = *it;
                IDInfo l;
                (const_cast<SlotID&>(slot)).getInfo(l);
                rez += " ";
                rez += (l.getName()).c_str();
        }
        return rez;
}

bool LT_Join::GetConfig(WayPointConfigureData& where){

    // get the ID
    WayPointID joinID = GetId ();

    // get the two functions to send
    //WorkFunc tempFunc = JoinRHSFunc;
    WorkFunc tempFunc = NULL; // will be populated in code loader
    JoinRHSWorkFunc myJoinRHSFunc (tempFunc);
    //tempFunc = JoinLHSFunc;
    tempFunc = NULL;
    JoinLHSWorkFunc myJoinLHSFunc (tempFunc);
    tempFunc = NULL;
    JoinLHSHashWorkFunc myJoinLHSHashFunc (tempFunc);
    WorkFuncContainer myJoinWorkFuncs;
    myJoinWorkFuncs.Insert (myJoinRHSFunc);
    myJoinWorkFuncs.Insert (myJoinLHSFunc);
    myJoinWorkFuncs.Insert (myJoinLHSHashFunc);


    // this is the set of query exits that end at it, and flow through it
    QueryExitContainer myJoinEndingQueryExits;
    QueryExitContainer myJoinFlowThroughQueryExits;
    GetQueryExits (myJoinFlowThroughQueryExits, myJoinEndingQueryExits);
    PDEBUG("Printing query exits for JOIN WP ID = %s", joinID.getName().c_str());
#ifdef DEBUG
        cout << "\nFlow through query exits\n" << flush;
        myJoinFlowThroughQueryExits.MoveToStart();
        while (myJoinFlowThroughQueryExits.RightLength()) {
                (myJoinFlowThroughQueryExits.Current()).Print();
                myJoinFlowThroughQueryExits.Advance();
        }
        cout << "\nEnding query exits\n" << flush;
        myJoinEndingQueryExits.MoveToStart();
        while (myJoinEndingQueryExits.RightLength()) {
                (myJoinEndingQueryExits.Current()).Print();
                myJoinEndingQueryExits.Advance();
        }
        cout << endl;
#endif

    HashTable tempTable;
    tempTable.Clone (centralHashTable);

    // get the ID
    //string name = GetId().getName();
    //name += "_merge";
    //WayPointID joinmergeID(name.c_str());
    // get the ID
    string namew = GetId().getName();
    namew += "_writer";
    WayPointID joinwriterID(namew.c_str());

    QueryExitContainer myJoinEndingQueryExitsCopy;
    myJoinEndingQueryExitsCopy.copy (myJoinEndingQueryExits); // TBD, what these copies are, prob not copies !

    QueryExitContainer myJoinFlowThroughQueryExitsCopy;
    myJoinFlowThroughQueryExitsCopy.copy (myJoinFlowThroughQueryExits); // TBD, what these copies are

    // here is the waypoint configuration data
    JoinConfigureData joinConfigure (joinID, myJoinWorkFuncs,
                     myJoinEndingQueryExits, myJoinFlowThroughQueryExits, joinwriterID, cleanerID,
                     myJoinEndingQueryExitsCopy, myJoinFlowThroughQueryExitsCopy, tempTable);

    where.swap (joinConfigure);

    return true;

}

bool LT_Join::GetConfigs(WayPointConfigurationList& where){

    // Get the config for join waypoint
    WayPointConfigureData wpConfig;
    GetConfig(wpConfig);
    where.Insert(wpConfig);

/*
    // Get the config for join merge waypoint

    // get the ID
    string name = GetId().getName();
    name += "_merge";
    WayPointID joinmergeID(name.c_str());

    WorkFunc tempFunc = NULL; // will be populated in code loader
    JoinMergeWorkFunc myJoinMergeFunc (tempFunc);
    WorkFuncContainer myJoinMergeWorkFuncs;
    myJoinMergeWorkFuncs.Insert (myJoinMergeFunc);

    // this is the set of query exits that end at it, and flow through it
    QueryExitContainer myJoinMergeEndingQueryExits;
    QueryExitContainer myJoinMergeFlowThroughQueryExits;
    GetQueryExits (myJoinMergeFlowThroughQueryExits, myJoinMergeEndingQueryExits);

    // here is the waypoint configuration data
    JoinMergeConfigureData joinMergeConfigure (joinmergeID, myJoinMergeWorkFuncs,
            myJoinMergeEndingQueryExits, myJoinMergeFlowThroughQueryExits);

    where.Insert(joinMergeConfigure);
*/

    // Get the config for join writer waypoint
    // get the ID
    string namew = GetId().getName();
    namew += "_writer";
    WayPointID writer(namew.c_str());

    JoinMergeWorkFunc myWriterWorkFunc (NULL);
    WorkFuncContainer myWriterWorkFuncs;
    myWriterWorkFuncs.Insert (myWriterWorkFunc);

    // this is the set of query exits that end at it, and flow through it (none!)
    // note that by definition, the writer has a single query exit that flows
    // into it... the exit is the writer itelf (since chunks sent along that path
    // get written by the writer), and the query is a null, since there is not an
    // actual query associated with data sent along that path
    QueryExitContainer myWriterEndingQueryExits;
    QueryExitContainer myWriterFlowThroughQueryExits;
    GetQueryExits (myWriterFlowThroughQueryExits, myWriterEndingQueryExits);

    PDEBUG("Printing query exits for WRITER WP ID = %s", writer.getName().c_str());
#ifdef DEBUG
        cout << "\nFlow through query exits\n" << flush;
        myWriterFlowThroughQueryExits.MoveToStart();
        while (myWriterFlowThroughQueryExits.RightLength()) {
                (myWriterFlowThroughQueryExits.Current()).Print();
                myWriterFlowThroughQueryExits.Advance();
        }
        cout << "\nEnding query exits\n" << flush;
        myWriterEndingQueryExits.MoveToStart();
        while (myWriterEndingQueryExits.RightLength()) {
                (myWriterEndingQueryExits.Current()).Print();
                myWriterEndingQueryExits.Advance();
        }
        cout << endl;
#endif
    AttributeManager& am=AttributeManager::GetAttributeManager();

    set<SlotID> lhs;
    set<SlotID> rhs;
    GetAccumulatedLHSRHSAtts(lhs, rhs);

    // get lhs slot to slot mapping
    SlotPairContainer columnsSlotsPairLHS;
    SlotPair p1(BITSTRING_SLOT, 0);
    columnsSlotsPairLHS.Append (p1);
    SlotPair p2(HASH_SLOT, 1);
    columnsSlotsPairLHS.Append (p2);
    int count = 2;
    for (set<SlotID>::iterator it = lhs.begin(); it != lhs.end(); it++) {
        SlotPair p (*it, count++);
        columnsSlotsPairLHS.Append (p);
    }
    string relation_lhs = namew;
    relation_lhs += "_lhs";
/*
    am.GetColumnToSlotMapping(relation_lhs, columnsToSlotsMapLHS);
*/

    // get rhs slot to slot mapping
    SlotPairContainer columnsSlotsPairRHS;
    SlotPair p3(BITSTRING_SLOT, 0);
    columnsSlotsPairRHS.Append (p3);
    SlotPair p4(HASH_SLOT, 1);
    columnsSlotsPairRHS.Append (p4);
    count = 2;
    for (set<SlotID>::iterator it = rhs.begin(); it != rhs.end(); it++) {
        SlotPair p (*it, count++);
        columnsSlotsPairRHS.Append (p);
    }
    string relation_rhs = namew;
    relation_rhs += "_rhs";
/*
    am.GetColumnToSlotMapping(relation_rhs, columnsToSlotsMapRHS);
*/

    // query to slot map LHS
    QueryExitToSlotsMap queryColumnsMapLHS;
    GetQueryExitToSlotMapLHS(queryColumnsMapLHS);

    // query to slot map RHS
    QueryExitToSlotsMap queryColumnsMapRHS;
    GetQueryExitToSlotMapRHS(queryColumnsMapRHS);

    WayPointID associatedJoinID = GetId();
    // here is the waypoint configuration data
    TileJoinConfigureData writerConfigure (writer, myWriterWorkFuncs,
        myWriterEndingQueryExits, myWriterFlowThroughQueryExits,
        relation_lhs, relation_rhs, associatedJoinID, queryColumnsMapLHS, queryColumnsMapRHS, columnsSlotsPairLHS, columnsSlotsPairRHS);

    where.Insert(writerConfigure);

    return true;

}
void LT_Join::ReceiveAttributesTerminating(QueryToSlotSet& atts)
{
    PDEBUG("LT_Join::ReceiveAttributesTerminating(QueryToSlotSet& atts)");
#ifdef DEBUG
    PrintAllQueryAndAttributes(atts);
    printf("\n"); fflush(stdout);
#endif
    for (QueryToSlotSet::const_iterator iter = atts.begin();
                                                                            iter != atts.end();
                                                                            ++iter) {
        QueryID query = iter->first;
        if (DoIHaveQueries(query))    {
            SlotSet atts_s = iter->second;
            CheckQueryAndUpdate(query, atts_s, RHS_terminating);
        }
    }
#ifdef DEBUG
    printf("\nRHS_terminating inside LT_Join::ReceiveAttributesTerminating");
    PrintAllQueryAndAttributes(RHS_terminating);
    printf("\n"); fflush(stdout);
#endif
}

bool LT_Join::AddBypass(QueryID query)
{
    bypassQueries.Union(query);
    queriesCovered.Union(query);
    return true;
}

void LT_Join::DeleteQuery(QueryID query)
{
    DeleteQueryCommon(query);
    RHS_terminating.erase(query);
    RHS.erase(query);
    LHS_copy.erase(query);
    RHS_copy.erase(query);
}

void LT_Join::ClearAllDataStructure() {
    ClearAll();
    LHS_atts.clear();
    RHS.clear();
    RHS_terminating.clear();
    LHS_copy.clear();
    RHS_copy.clear();
}

bool LT_Join::AddJoin(QueryID query, SlotSet& RHS_atts) {
    PDEBUG("LT_Join::AddJoin(QueryID query, SlotSet& RHS_atts)");
    CheckQueryAndUpdate(query, RHS_atts, newQueryToSlotSetMap);
    CheckQueryAndUpdate(query, RHS_atts, RHS);
    queriesCovered.Union(query);
#ifdef DEBUG
    PrintAllQueryAndAttributes(RHS);
    printf("\n"); fflush(stdout);
#endif
    return true;
}

/* Here is logic first for below two functions:
   propagated-up = LHS_copy U RHS_copy
   LHS_copy = (propagated-up) \intersect (LHS_all_attributes)
   RHS_copy = (propagated-up) \intersect (RHS_attributes for that query)
   propagated-down_LHS = LHS_all_attributes U LHS_copy
   propagated-down_RHS = RHS_all_attributes U RHS_copy */

// Implementation top -> down as follows per query:
// 1. used = used + new queries attributes added since last analysis
// 2. used should be subset of RHS_all (RHS_all from bottom up analysis)
// 3. LHS_copy = attributes from top \intersect LHS_all_atts (LHS_all from bottom up analysis)
// 4. RHS_copy = attributes from top \intersect RHS_attr for query
// 5. WARNING LHS_copy size + RHS_copy size == attr from top
// 6. result = LHS_all_atts U LHS_copy
//           = LHS_all_atts (because LHS_copy is subset of LHS_all_atts)
// Consider this as LHS
bool LT_Join::PropagateDown(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe)
{
    PDEBUG("LT_Join::PropagateDown(QueryID query = %s, const SlotSet& atts = %s, SlotSet& result, QueryExit qe)", query.ToString().c_str(), (GetAllAttrAsString(atts)).c_str());
    CheckQueryAndUpdate(newQueryToSlotSetMap, used);
#ifdef DEBUG
    printf("Used queries and attributes in LT_Join::PropagateDown");
    PrintAllQueryAndAttributes(used);
    printf("\n"); fflush(stdout);
    printf("RHS_terminating queries and attributes in LT_Join::PropagateDown");
    PrintAllQueryAndAttributes(RHS_terminating);
    printf("\n"); fflush(stdout);
#endif

    newQueryToSlotSetMap.clear();
    result.clear();

    // used (which is maintained for RHS attributes per query) should be subset of RHS_all
    if (!IsSubSet(used[query], RHS_terminating[query]))
    {
        PrintAttributeList(used[query], cout);
        PrintAttributeList(RHS_terminating[query], cout);
        cout << "Join WP : used RHS is not subset of RHS";
        return false;
    }

    SlotSet lhs_copy;
    set_intersection(atts.begin(), atts.end(), downAttributes[query].begin(), downAttributes[query].end(), inserter(lhs_copy, lhs_copy.begin()));
    CheckQueryAndUpdate(query, lhs_copy, LHS_copy);
#ifdef DEBUG
    printf("LHS_copy queries and attributes in LT_Join::PropagateDown");
    PrintAllQueryAndAttributes(LHS_copy);
    printf("\n"); fflush(stdout);
#endif

    set_union(lhs_copy.begin(), lhs_copy.end(), LHS_atts.begin(), LHS_atts.end(),
                        inserter(result, result.begin()));

    queryExit.Insert (qe);
    return true;
}

// All sanity checks are already done in above call, hence this can be simple
// result = RHS U RHS_copy
//        = RHS (because RHS_copy is subset of RHS)
bool LT_Join::PropagateDownTerminating(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe)
{
    result.clear();

    PDEBUG("LT_Join::PropagateDownTerminating(QueryID query = %s, const SlotSet& atts = %s, SlotSet& result, QueryExit qe)", query.ToString().c_str(), (GetAllAttrAsString(atts)).c_str());
    SlotSet rhs_copy;
    set_intersection(atts.begin(), atts.end(), RHS_terminating[query].begin(), RHS_terminating[query].end(), inserter(rhs_copy, rhs_copy.begin()));
    CheckQueryAndUpdate(query, rhs_copy, RHS_copy);
#ifdef DEBUG
    printf("RHS_copy queries and attributes in LT_Join::PropagateDownTerminating");
    PrintAllQueryAndAttributes(RHS_copy);
    printf("\n"); fflush(stdout);
#endif

    set_union(rhs_copy.begin(), rhs_copy.end(), RHS[query].begin(), RHS[query].end(),
                        inserter(result, result.begin()));

    //queryExit.Insert (qe);
    queryExitTerminating.Insert(qe);
    return true;
}

// Implementation bottom -> up as follows for all queries together:
// 1. used = used + new queries attributes added since last analysis
// 2. clear the new data
// 3. Correctness : used (RHS attr) is subset of RHS_all coming from below
// 4.               LHS all should be subset of LHS_all coming from below

bool LT_Join::PropagateUp(QueryToSlotSet& result)
{
    PDEBUG("LT_Join::PropagateUp(QueryToSlotSet& result)");
    CheckQueryAndUpdate(newQueryToSlotSetMap, used);
    newQueryToSlotSetMap.clear();
    result.clear();
#ifdef DEBUG
    printf("used queries and attributes in LT_Join::PropagateUp");
    PrintAllQueryAndAttributes(used);
    printf("\n"); fflush(stdout);
    printf("RHS_terminating queries and attributes in LT_Join::PropagateUp");
    PrintAllQueryAndAttributes(RHS_terminating);
    printf("\n"); fflush(stdout);
#endif

    // used (which is maintained for RHS attributes per query) should be subset of RHS_all
    if (!IsSubSet(used, RHS_terminating))
    {
        cout << "Attribute mismatch Join WP: used RHS is not subset of RHS";
        return false;
    }
    // LHS which is common for all queries should be subset of what is coming from LHS below
    if (!IsSubSet(LHS_atts, downAttributes))
    {
        cout << "Attribute mismatch Join WP: used LHS is not subset of LHS";
        return false;
    }

    //result up = LHS down + RHS down;
    CheckQueryAndUpdate(downAttributes, result);
    CheckQueryAndUpdate(RHS_terminating, result);
#ifdef DEBUG
    printf("result queries and attributes in LT_Join::PropagateUp");
    PrintAllQueryAndAttributes(result);
    printf("\n"); fflush(stdout);
#endif

    return true;
}

void LT_Join::GetQueryExitToSlotMapLHS(QueryExitToSlotsMap& qe){

    AttributeManager& am = AttributeManager::GetAttributeManager();
    QueryExitToSlotsMap tmp;
    queryExit.MoveToStart();
    // iterate over queryExit and see it some if it matches with LHS queries
    while (queryExit.RightLength()) {
        SlotContainer slotIds;
        // Do it first for LHS_atts
        for (SlotSet::const_iterator iter = LHS_atts.begin(); iter != LHS_atts.end(); ++iter){
            SlotID s = *iter;
            slotIds.Insert(s);
        }
        // Now do it for LHS_copy
        QueryToSlotSet::const_iterator it = LHS_copy.find((queryExit.Current()).query);
        if (it != LHS_copy.end()){
            for (SlotSet::const_iterator iter = (it->second).begin(); iter != (it->second).end(); ++iter){
                SlotID s = *iter;
                slotIds.Insert(s);
            }
        }
        // Now insert queryExit to slot mapping
        QueryExit qeTemp = queryExit.Current();
        tmp.Insert(qeTemp, slotIds);
        // advance
        queryExit.Advance();
    }
    tmp.swap(qe);
}

void LT_Join::GetQueryExitToSlotMapRHS(QueryExitToSlotsMap& qe){

    AttributeManager& am = AttributeManager::GetAttributeManager();
    QueryExitToSlotsMap tmp;
    queryExit.MoveToStart();
    // iterate over queryExit and see it some if it matches with RHS queries
    while (queryExit.RightLength()) {
        SlotContainer slotIds;
        // do it for RHS first
        QueryToSlotSet::const_iterator it = RHS.find((queryExit.Current()).query);
        if (it != RHS.end()){
            for (SlotSet::const_iterator iter = (it->second).begin(); iter != (it->second).end(); ++iter){
                SlotID s = *iter;
                slotIds.Insert(s);
            }
        }
        // Now do it for RHS_copy
        QueryToSlotSet::const_iterator itr = RHS_copy.find((queryExit.Current()).query);
        if (it != RHS_copy.end()){
            for (SlotSet::const_iterator iter = (itr->second).begin(); iter != (itr->second).end(); ++iter){
                SlotID s = *iter;
                slotIds.Insert(s);
            }
        }
        // Now insert queryExit to slot mapping
        QueryExit qeTemp = queryExit.Current();
        tmp.Insert(qeTemp, slotIds);
        // advance
        queryExit.Advance();
    }
    tmp.swap(qe);
}

void LT_Join::GetAccumulatedLHSRHS(set<SlotID>& lhs, set<SlotID>& rhs, QueryIDSet& queries) {

/*
    lhs = LHS_atts;

    for (QueryToSlotSet::const_iterator iter = RHS.begin(); iter != RHS.end(); ++iter) {
        SlotSet atts_s = iter->second;
        SlotSet copy;
        set_union(rhs.begin(), rhs.end(), atts_s.begin(), atts_s.end(), inserter(copy, copy.begin()));
        rhs = copy;
    }
*/

    GetAccumulatedLHSRHSAtts (lhs, rhs);
    queries.copy(queriesCovered);
}

void LT_Join::GetAccumulatedLHSRHSAtts(set<SlotID>& lhs, set<SlotID>& rhs) {

    lhs = LHS_atts;

    for (QueryToSlotSet::const_iterator iter = LHS_copy.begin(); iter != LHS_copy.end(); ++iter) {
        SlotSet atts_s = iter->second;
        SlotSet copy;
        set_union(lhs.begin(), lhs.end(), atts_s.begin(), atts_s.end(), inserter(copy, copy.begin()));
        lhs = copy;
    }

    for (QueryToSlotSet::const_iterator iter = RHS.begin(); iter != RHS.end(); ++iter) {
        SlotSet atts_s = iter->second;
        SlotSet copy;
        set_union(rhs.begin(), rhs.end(), atts_s.begin(), atts_s.end(), inserter(copy, copy.begin()));
        rhs = copy;
    }

    for (QueryToSlotSet::const_iterator iter = RHS_copy.begin(); iter != RHS_copy.end(); ++iter) {
        SlotSet atts_s = iter->second;
        SlotSet copy;
        set_union(rhs.begin(), rhs.end(), atts_s.begin(), atts_s.end(), inserter(copy, copy.begin()));
        rhs = copy;
    }
}

void LT_Join::WriteM4File(ostream& out) {
    IDInfo info;
    GetId().getInfo(info);
    string wpname = info.getName();

    out << "M4_JOIN_MODULE(" << wpname << ", ";
    //out << "\t";

    // -----------------------------------------------------------------------------------
    // outout left hash attributes
    out << "</";
    out << "(";
    PrintAttributeList(LHS_atts, out);
    //out << "),\t</";
    out << ")";
    out << "/>,";
    // -----------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------
    // Now print all attributes accessed for LHS
    QueryToSlotSet allLHS;
    QueryIDSet qrys=queriesCovered;
    while( !qrys.IsEmpty()){
      QueryID q = qrys.GetFirst(); // net the next element in the set
      CheckQueryAndUpdate(q, LHS_atts, allLHS);
    }

    // Get all LHS copy queries and attributes
    CheckQueryAndUpdate(LHS_copy, allLHS);
    // Now add LHS_atts (hash attributes) to all queries we have in copy
    //for (QueryToSlotSet::iterator it=LHS_copy.begin(); it!=LHS_copy.end(); it++){
//        QueryID query = it->first;
//        CheckQueryAndUpdate(query, LHS_atts, allLHS);
//    }
    {
        // Now do the printing
        SlotToQuerySet reverse;
        AttributesToQuerySet(allLHS, reverse);
        out << "(";
        PrintAttToQuerySets(reverse, out);
        out << ")";
        out << ",";
    }
    // -----------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------
    {
        // LHS_copy
        SlotToQuerySet reverse;
        AttributesToQuerySet(LHS_copy, reverse);
        out << "(";
        PrintAttToQuerySets(reverse, out);
        out << ")";
        out << ",";
    }
    // -----------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------
    // Query class
    QueryToSlotSet allRHSqc;
    //CheckQueryAndUpdate(RHS_copy, allRHSqc);
    CheckQueryAndUpdate(RHS, allRHSqc);
    SlotToQuerySet rh;
    AttributesToQuerySet(allRHSqc, rh); // This give back attr to queryIDSet mapping
    // Now consolidate all attributes which have same querySet to create query class
    QueryToSlotSet qClassToSlotsqc; // This is actually query class to all slots with that query class
    for (SlotToQuerySet::iterator it = rh.begin(); it != rh.end(); it++) {
        SlotSet s;
        s.insert(it->first);
        CheckQueryAndUpdate(it->second, s, qClassToSlotsqc);
    }
    out << "</";
    out << "(";
    for (QueryToSlotSet::iterator it = qClassToSlotsqc.begin(); it != qClassToSlotsqc.end(); it++) {
        out << (const_cast<QueryIDSet&>(it->first)).ToString(); // Write the queryIDSet as a numeric value
        out << ",";
        out << "(";
        bool firstTime = true;
        for(SlotSet::iterator iter = (it->second).begin(); iter != (it->second).end(); iter++) {
            if (!firstTime) {
                out << ",";
            }
            firstTime = false;
            AttributeManager& am = AttributeManager::GetAttributeManager();
            out << am.GetAttributeName(*iter);
        }
        out << ")";
        out << ",";
        // Now print all (attr, qSet) which need to be put into hash table
        SlotToQuerySet reverse;
        QueryToSlotSet allRHS;
        CheckQueryAndUpdate(RHS_copy, allRHS);
        CheckQueryAndUpdate(RHS, allRHS);
        AttributesToQuerySet(allRHS, reverse);
        PrintAttToQuerySetsJoin(reverse, out);
    }
    out << ")";
    out << "/>,";
    // -----------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------
    // Now print all attributes accessed for RHS
    QueryToSlotSet allRHS;
    QueryToSlotSet hashRHS;
    CheckQueryAndUpdate(RHS_copy, allRHS);
    CheckQueryAndUpdate(RHS, allRHS);
    CheckQueryAndUpdate(RHS, hashRHS);
    SlotToQuerySet allRHSreverse;
    SlotToQuerySet hashRHSreverse;
    AttributesToQuerySet(hashRHS, hashRHSreverse);
    {
        // Now do the printing
        SlotToQuerySet reverse;
        AttributesToQuerySet(allRHS, reverse);
        allRHSreverse = reverse;
        out << "(";
        PrintAttToQuerySets(reverse, out);
        out << ")";
        out << ",";
    }
    // -----------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------
    {
        // RHS_copy
        SlotToQuerySet reverse;
        AttributesToQuerySet(RHS_copy, reverse);
        out << "(";
        PrintAttToQuerySets(reverse, out);
        out << ")";
        out << ",";
    }
    // -----------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------
    set<SlotID> s;
    for (SlotToQuerySet::iterator it = allRHSreverse.begin(); it != allRHSreverse.end(); it++) {
        s.insert(it->first);
    }
    out << "</";
    out << "(";
    bool first = true;
    for (set<SlotID>::iterator i = s.begin(); i != s.end(); i++) {
        if (!first) {
            out << ",";
        }
        first = false;
        AttributeManager& am = AttributeManager::GetAttributeManager();
        out << am.GetAttributeName(*i);
    }
    out << ")";
    out << "/>,";
    // -----------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------
    // M4_Queries_Attribute_Comparision
    // We write the pairs of lhs and rhs attributes per query class
    vector<SlotID> v;
    for (SlotSet::iterator ite = LHS_atts.begin(); ite != LHS_atts.end(); ite++) {
        v.push_back(*ite);
    }
    out << "</";
    out << "(";
    //out << "(";
    QueryToSlotSet qClassToSlots; // This is actually query class to all slots with that query class
    int count = 0;
    for (SlotToQuerySet::iterator it = hashRHSreverse.begin(); it != hashRHSreverse.end(); it++) {
        SlotSet s;
        s.insert(it->first);
        CheckQueryAndUpdate(it->second, s, qClassToSlots);
    }
    for (QueryToSlotSet::iterator it = qClassToSlots.begin(); it != qClassToSlots.end(); it++) {
        out << (const_cast<QueryIDSet&>(it->first)).ToString(); // Write the queryIDSet as a numeric value
        out << ",";
        out << "(";
        bool firstTime = true;
        for(SlotSet::iterator iter = (it->second).begin(); iter != (it->second).end(); iter++) {
            if (!firstTime) {
                out << ",";
            }
            out << "(";
            firstTime = false;
            AttributeManager& am = AttributeManager::GetAttributeManager();
            //cout << "\nUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU\n";
            //cout << am.GetAttributeName(*iter); cout << flush;
            //cout << ",";
            //cout << am.GetAttributeName(v[count]);
            assert(count < v.size());
            out << am.GetAttributeName(v[count++]);
            out << ",";
            out << am.GetAttributeName(*iter);
            out << ")";
        }
        out << ")";
    }
    out << ")";
    out << "/>";
    // -----------------------------------------------------------------------------------
    out << ")";
    // -----------------------------------------------------------------------------------
    out << "\n\n";

    // Now write the data for hashing LHS attributes
    WriteM4LHSHashFile(out);
    out << "\n\n";
    // Now write the data for Join merger waypoint
    WriteM4JoinMergeFile(out);
}

void LT_Join::WriteM4LHSHashFile(ostream& out) {

    IDInfo info;
    GetId().getInfo(info);
    string wpname = info.getName();

    out << "M4_JOINLHSHASH_MODULE(" << wpname << ", ";

    // -----------------------------------------------------------------------------------
    // Now print all attributes accessed for LHS
    QueryToSlotSet allLHS;
    QueryIDSet qrys=queriesCovered;
    while( !qrys.IsEmpty()){
      QueryID q = qrys.GetFirst(); // net the next element in the set
      CheckQueryAndUpdate(q, LHS_atts, allLHS);
    }

    // Get all LHS copy queries and attributes
    CheckQueryAndUpdate(LHS_copy, allLHS);
    {
        // Now do the printing
        SlotToQuerySet reverse;
        AttributesToQuerySet(allLHS, reverse);
        out << "(";
        PrintAttToQuerySets(reverse, out);
        out << ")";
        out << ",";
    }
    // -----------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------
    // output left hash attributes
    out << "</";
    out << "(";
    PrintAttributeList(LHS_atts, out);
    //out << "),\t</";
    out << ")";
    out << "/>,";
    // -----------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------
    {
        // LHS_copy
        SlotToQuerySet reverse;
        AttributesToQuerySet(LHS_copy, reverse);
        out << "(";
        PrintAttToQuerySets(reverse, out);
        out << ")";
    }
    // -----------------------------------------------------------------------------------
    out << ")";
}

void LT_Join::WriteM4JoinMergeFile(ostream& out) {

    IDInfo info;
    GetId().getInfo(info);
    string wpname = info.getName();
    //wpname += "_merge";
    wpname += "_writer";

    out << "M4_JOINMERGE_MODULE(" << wpname << ", ";

    // -----------------------------------------------------------------------------------
    // Now print all attributes accessed for LHS
    QueryToSlotSet allLHS;
    QueryIDSet qrys=queriesCovered;
    while( !qrys.IsEmpty()){
      QueryID q = qrys.GetFirst(); // net the next element in the set
      CheckQueryAndUpdate(q, LHS_atts, allLHS);
    }


    // Get all LHS copy queries and attributes
    CheckQueryAndUpdate(LHS_copy, allLHS);
    {
        // Now do the printing
        SlotToQuerySet reverse;
        AttributesToQuerySet(allLHS, reverse);
        //out << "(";
        PrintAttToQuerySets(reverse, out);
        //out << ")";
        out << ",";
    }
    // -----------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------
    // Now print all attributes accessed for RHS
    QueryToSlotSet allRHS;
    QueryToSlotSet hashRHS;
    CheckQueryAndUpdate(RHS_copy, allRHS);
    CheckQueryAndUpdate(RHS, allRHS);
    CheckQueryAndUpdate(RHS, hashRHS);
    SlotToQuerySet allRHSreverse;
    SlotToQuerySet hashRHSreverse;
    AttributesToQuerySet(hashRHS, hashRHSreverse);
    {
        // Now do the printing
        SlotToQuerySet reverse;
        AttributesToQuerySet(allRHS, reverse);
        allRHSreverse = reverse;
        //out << "(";
        PrintAttToQuerySets(reverse, out);
        //out << ")";
        out << ",";
    }
    // -----------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------
    {
        // LHS_copy
        SlotToQuerySet reverse;
        AttributesToQuerySet(LHS_copy, reverse);
        //out << "(";
        PrintAttToQuerySets(reverse, out);
        //out << ")";
        out << ",";
    }
    // -----------------------------------------------------------------------------------
    //
    // -----------------------------------------------------------------------------------
    {
        // RHS_copy
        SlotToQuerySet reverse;
        AttributesToQuerySet(RHS_copy, reverse);
        //out << "(";
        PrintAttToQuerySets(reverse, out);
        //out << ")";
        out << ",";
    }
    // -----------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------
    // M4_Queries_Attribute_Comparision
    // We write the pairs of lhs and rhs attributes per query class
    vector<SlotID> v;
    for (SlotSet::iterator ite = LHS_atts.begin(); ite != LHS_atts.end(); ite++) {
        v.push_back(*ite);
    }
    out << "</";
    out << "(";
    //out << "(";
    QueryToSlotSet qClassToSlots; // This is actually query class to all slots with that query class
    int count = 0;
    for (SlotToQuerySet::iterator it = hashRHSreverse.begin(); it != hashRHSreverse.end(); it++) {
        SlotSet s;
        s.insert(it->first);
        CheckQueryAndUpdate(it->second, s, qClassToSlots);
    }
    for (QueryToSlotSet::iterator it = qClassToSlots.begin(); it != qClassToSlots.end(); it++) {
        out << (const_cast<QueryIDSet&>(it->first)).ToString(); // Write the queryIDSet as a numeric value
        out << ",";
        out << "(";
        bool firstTime = true;
        for(SlotSet::iterator iter = (it->second).begin(); iter != (it->second).end(); iter++) {
            if (!firstTime) {
                out << ",";
            }
            out << "(";
            firstTime = false;
            AttributeManager& am = AttributeManager::GetAttributeManager();
            assert(count < v.size());
            out << am.GetAttributeName(v[count++]);
            out << ",";
            out << am.GetAttributeName(*iter);
            out << ")";
        }
        out << ")";
    }
    out << ")";
    out << "/>";
    // -----------------------------------------------------------------------------------
    out << ")";
}
