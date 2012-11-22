#include "DataTypeManager.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <iostream>
#include <cstdio>
#include "ArgFormat.h"

using namespace std;

DataTypeManager* DataTypeManager::instance = NULL;

DataTypeManager::DataTypeManager() {}

DataTypeManager::~DataTypeManager() {

    Clear();
}

void DataTypeManager::Clear()
{
    TypeToInfoMap::const_iterator it = mType.begin();
    for (; it != mType.end(); ++it) {

        delete it->second;
    }

    mType.clear();

    FuncToInfoMap::const_iterator iter = mFunc.begin();
    for (; iter != mFunc.end(); ++iter) {
        set<FuncInfo*>::const_iterator iterest = iter->second.begin();
        for(; iterest != iter->second.end(); ++iterest )
        {
            delete *iterest;
        }
    }

    mFunc.clear();

    mTemp.clear();

    mSynonymToBase.clear();
}

bool DataTypeManager :: DoesTypeExist(string type) {

    TypeToInfoMap::const_iterator it = mType.find(type);
    if (it != mType.end()) {
        return true;
    }
    else {
        // Check synonyms
        map<string, string>::const_iterator iter = mSynonymToBase.find(type);
        if (iter != mSynonymToBase.end())
            return true;
    }
    return false;
}

void DataTypeManager :: AddBaseType(string type, string file ) {

    // First check if base type already exists
    TypeToInfoMap::const_iterator it = mType.find(type);
    if (it != mType.end()) {
        cout << "\nError: Base type already exists";
        return;
    }

    // Make the new base type also be a synonym for itself. This will make
    // base type lookups easier, as you won't have to handle the case where
    // the type you're asking about is already a base type.
    mSynonymToBase[type] = type;

    // Add it as base type
    TypeInfo* t = new TypeInfo;
    t->file = file;
    t->noExtract = false;
    mType[type] = t;
}

void DataTypeManager :: AddSynonymType(string baseType, string synonymType) {

    // Check if base type exist
    TypeToInfoMap::const_iterator it = mType.find(baseType);
    if (it == mType.end()) {
        cout << "\nError: Base type does not exist, please add it first";
        return;
    }
    // They should not be same
    if (baseType == synonymType) {
        cout << "\nError: Base type and synonym types are same";
        return;
    }
    // Does synonym already exists as base type?
    TypeToInfoMap::const_iterator iter = mType.find(synonymType);
    if (iter != mType.end()) {
        cout << "\nError: Synonym already exists as base type";
        return;
    }

    // Add into synonyms list
    mSynonymToBase[synonymType] = baseType;
}

string DataTypeManager :: GetBaseType(string synonymType) {

    // First check the synonym
    map<string, string>::const_iterator it = mSynonymToBase.find(synonymType);
    if (it != mSynonymToBase.end())
        return it->second;

    //    cout << "\nError: No base type exist for the asked synonym type : " << synonymType;

    // Otherwise, return nothing
    return string("");
}

bool DataTypeManager :: IsType( string& type ) {
    // Check synonym map
    map<string, string>::const_iterator it = mSynonymToBase.find(type);
    if( it != mSynonymToBase.end() ) {
        type = it->second;

        return true;
    }

    return false;
}

void DataTypeManager :: SetPossibleConversion(string fromType, string toType) {

    // Assumption is, all conversions should be recorded w.r.t base type, hence get basetype first
    if( !IsType(fromType) ) {
        cout << "\nError: type " << fromType << " not found";
        return;
    }

    if( !IsType(toType) ) {
        cout << "\nError: type " << toType << " not found";
        return;
    }

    // If base type found. Just insert.
    TypeToInfoMap::const_iterator it = mType.find(fromType);
    if (it != mType.end())
        (it->second)->convertibleTo.insert(toType);
}

bool DataTypeManager :: IsConversionPossible(string fromType, string toType) {

    // Assumption is, all conversions should be recorded w.r.t base type, hence get basetype first
    if( !IsType(fromType) ) {
        cout << "\nError: type " << fromType << " not found";
        return false;
    }

    if( !IsType(toType) ) {
        cout << "\nError: type " << toType << " not found";
        return false;
    }

    // If base type found. Just check
    TypeToInfoMap::const_iterator it = mType.find(fromType);
    if (it != mType.end()) {
        set<string>::const_iterator iter = (it->second)->convertibleTo.find(toType);
        if (iter != (it->second)->convertibleTo.end())
            return true;
    }
    return false;
}

void DataTypeManager :: DeleteType(string type) {

    // Remove from synonyms list
    map<string, string>::iterator iter = mSynonymToBase.find(type);
    if (iter != mSynonymToBase.end())
        mSynonymToBase.erase(iter);

    // If base, remove from base list
    TypeToInfoMap::iterator it = mType.find(type);
    if (it != mType.end()) {
        delete it->second;
        mType.erase(it);
    }

    // Now check function agruments, if found, just remove that function altogether
    FuncToInfoMap::iterator itf = mFunc.begin();
    for (; itf != mFunc.end(); ++itf) {
        set<FuncInfo*>::const_iterator fiter = itf->second.begin();
        for( ; fiter != itf->second.end(); ++fiter )
        {
            vector<string>::const_iterator iv = find(((*fiter)->args).begin(), ((*fiter)->args).end(), type);
            if ((*fiter)->type == type || (*fiter)->returnType == type || iv != ((*fiter)->args).end()) {
                delete *fiter;
                itf->second.erase(fiter);
            }
        }
    }
}

bool DataTypeManager :: ConvertFuncExists ( string source, string dest ) {
    if( !IsType(source) ) {
        cout << "\nError: type " << source << " not found";
        return false;
    }

    if( !IsType(dest) ) {
        cout << "\nError: type " << dest << " not found";
        return false;
    }

    string funcName = CONVERSION_PREFIX + dest;

    set<FuncInfo*> funcSet = mFunc[funcName];

    for( set<FuncInfo*>::const_iterator iter = funcSet.begin(); iter != funcSet.end(); ++iter ) {
        FuncInfo * curFunc = *iter;

        // Requirements for a function to be a proper conversion function from source to dest:
        // 1. It must have the proper name (already satisfied)

        // 2. It must be a function, not a method
        if( curFunc->type != string("") )
            continue;

        // 3. It must take exactly 1 argument
        if( curFunc->args.size() != 1 )
            continue;

        // 4. The sole argument must be of type source
        if( curFunc->args[0] != source )
            continue;

        // 5. The return type must be of type dest
        if( curFunc->returnType != dest )
            continue;

        // If we have gotten to this point, it is safe to call funcName( source ), and that
        // call will return type dest

        return true;
    }

    // Didn't find a matching function.
    return false;
}

void DataTypeManager :: AddFunc (string type, string fName, vector<string>& args, string returnType, string file, Associativity assoc, int priority, bool pure) {

    // Make sure we are dealing with base types.
    if( type != "" && !IsType(type) ) {
        cout << "\nError: type " << type << " not found";
        return;
    }

    if( returnType != "" && !IsType(returnType) ) {
        cout << "\nError: type " << returnType << " not found";
        return;
    }

    for( int i = 0; i < args.size(); ++i )
    {
        if( !IsType( args[i] ) ) {
            cout << "\nError: type " << args[i] << " not found";
            return;
        }
    }

    // Create a new struct
    FuncInfo* f = new FuncInfo;
    f->type = type;
    f->args = args;
    f->returnType = returnType;
    f->priority = priority;
    f->assoc = assoc;
    f->pure = pure;
    f->file = file;

    // Check if function already exists
    FuncToInfoMap::const_iterator it = mFunc.find(fName);
    if (it != mFunc.end()) {
        // Found a function with the same name, now check to see if parameter
        // list of any of the functions with that name is identical

        set<FuncInfo*>::const_iterator iter = it->second.begin();
        for(; iter != it->second.end(); ++iter)
        {
            if( **iter == *f )
            {
                // Identical type and parameter list found for this function
                // name already.
                cout << "\nError: Function/Operator " << f->toString( fName ) << " already exists";
                delete f;
                return;
            }
        }
    }



    // Add function name and function info into the map
    mFunc[fName].insert(f);
}

bool DataTypeManager :: IsCorrectFunc (string type, string fName, vector<string>& args, string& returnType, string& file, Associativity assoc, int priority, bool& pure, vector<ArgFormat>& actualArgs) {

    // Make sure we are dealing with base types.
    if( type != "" && !IsType(type) ) {
        cout << "\nError: type " << type << " not found";
        return false;
    }

    for( int i = 0; i < args.size(); ++i )
    {
        if( !IsType( args[i] ) ) {
            cout << "\nError: type " << args[i] << " not found";
            return false;
        }
    }

    // Check if function exists
    FuncToInfoMap::const_iterator it = mFunc.find(fName);
    if (it == mFunc.end()) {
        return false;
    }

    // Need to keep track of any possible matches due to conversions as well as
    // if we have come across an exact match.
    bool exact_match = false;
    vector<FuncInfo*> matches;

    FuncInfo* f;
    set<FuncInfo*>::const_iterator iter = it->second.begin();
    for( ; !exact_match && iter != it->second.end(); ++iter )
    {
        // Check if all details match or are convertible (as applicable)
        exact_match = true;
        bool matchesSoFar = true;

        f = *iter;

        // Check if class type matches if function is method of class
        if (type != f->type)
        {
            exact_match = false;
            matchesSoFar = false;
        }

        // Check function signature

        // Check to see if functions have same number of parameters
        if( f->args.size() != args.size() )
        {
            matchesSoFar = false;
            exact_match = false;
        }

        // If same number of parameters, make sure that the parameters in order
        // are of the same type or of convertible types.
        vector<string>::const_iterator argIter = args.begin();
        vector<string>::const_iterator fArgIter = f->args.begin();
        for( ; matchesSoFar && argIter != args.end() && fArgIter != f->args.end(); ++argIter, ++fArgIter )
        {
            if( *argIter != *fArgIter )
            {
                exact_match = false;

                if( !IsConversionPossible( *argIter, *fArgIter ) &&
                        !ConvertFuncExists(*argIter, *fArgIter) ) {
                    matchesSoFar = false;
                }
            }
        }

        // Check associativity if needed
        if (assoc != NoAssoc) {
            if (f->assoc != assoc)
            {
                matchesSoFar = false;
                exact_match = false;
            }
        }

        // Check priority if needed
        if (priority != -1) {
            if (f->priority != priority)
            {
                matchesSoFar = false;
                exact_match = false;
            }
        }

        // If the function has not been ruled out so far, add it as a possible
        // match
        if( matchesSoFar ) {
            matches.push_back( f );

#ifdef DTM_DEBUG

            //cout << "\nAdded match " << f->toString( fName );

#endif
        }
    }

    if( matches.size() == 0 )
    {
        //printf("None\n");
        // No matches found, the requested function does not exist.
        cout << "\nNo matches found for function ";
        if( type != "" )
            cout << type << ".";

        cout << fName << "(";
        vector<string>::const_iterator argIter = args.begin();

        if( argIter != args.end() )
        {
            cout << *argIter;
            ++argIter;
        }

        for( ; argIter != args.end(); ++argIter )
        {
            cout << ", " << *argIter;
        }

        cout << ")";

        return false;
    }
    else if( exact_match )
    {

        // We found an exact match for the function.
        // The FuncInfo for the exact match will be the last element in the
        // matches vector.
        returnType = matches.back()->returnType;
        pure = matches.back()->pure;
        file = matches.back()->file;

        actualArgs.clear();

        FuncInfo * cur = matches.back();
        for( size_t i = 0; i < args.size(); ++i ) {
            if( args[i] == cur->args[i] || IsConversionPossible( args[i], cur->args[i] ) ) {
                TypeInfo * tInfo = mType[args[i]];
                bool noExtract = tInfo->noExtract;
                actualArgs.push_back( ArgFormat(cur->args[i], "%s", noExtract) );
            }
            else {
                string wrap;
                wrap += CONVERSION_PREFIX + cur->args[i];
                wrap += "(%s)";

                actualArgs.push_back( ArgFormat(cur->args[i], wrap, false ) );

            }
        }

#ifdef DTM_DEBUG
        printf("\n%s(", fName.c_str() );

        if( args.size() > 0 )
            printf("%s", args[0].c_str() );

        for( int x = 1; x < args.size(); ++x )
            printf(" %s", args[x].c_str() );

        printf(") -> %s", returnType.c_str());
#endif

        return true;
    }
    else if( matches.size() == 1 )
    {
        //printf("One\n");
        // We didn't find an exact match, but we found one possible match using
        // type conversions. Print a warning stating this.

        // Warning is of format:
        // "Exact match not found for function [type.]fName(param list),
        // using [f->type.]fName(f param list) instead."
        // The parts in brackets will only be printed if type is not empty.

        cout << "\nExact match not found for function ";

        if( type != "" )
            cout << type << ".";

        cout << fName << "(";
        vector<string>::const_iterator argIter = args.begin();

        if( argIter != args.end() )
        {
            cout << *argIter;
            ++argIter;
        }

        for( ; argIter != args.end(); ++argIter )
        {
            cout << ", " << *argIter;
        }

        cout << "), using function ";

        f = matches.back();

        cout << f->toString( fName );

        // Set return type and return
        returnType = f->returnType;
        pure = f->pure;
        file = f->file;

        actualArgs.clear();

        FuncInfo * cur = matches.back();
        for( size_t i = 0; i < args.size(); ++i ) {
            if( args[i] == cur->args[i] || IsConversionPossible( args[i], cur->args[i] ) ) {
                TypeInfo * tInfo = mType[args[i]];
                bool noExtract = tInfo->noExtract;
                actualArgs.push_back( ArgFormat(cur->args[i], "%s", noExtract) );
            }
            else {
                string wrap;
                wrap += CONVERSION_PREFIX + cur->args[i];
                wrap += "(%s)";

                actualArgs.push_back( ArgFormat(cur->args[i], wrap, false ) );

            }
        }

        return true;
    }
    else
    {
        //printf("Multiple\n");
        // We found multiple possible matches using type conversions, and thus
        // the function call is ambiguous.

        // Error message is of format:
        // "Exact match not found for [type.]fName(param list)"
        // "\nPossible matches:"
        // For each possible match f:
        // "\n[f->type.]fName(f param list)"

        cout << "\nExact match not found for function ";

        if( type != "" )
            cout << type << ".";

        cout << fName << "(";
        vector<string>::const_iterator argIter = args.begin();

        if( argIter != args.end() )
        {
            cout << *argIter;
            ++argIter;
        }

        for( ; argIter != args.end(); ++argIter )
        {
            cout << ", " << *argIter;
        }

        cout << ")";
        cout << "\nPossible matches:";

        vector<FuncInfo*>::const_iterator fIter = matches.begin();
        for( ; fIter != matches.end(); ++fIter ){
            f = *fIter;
            cout << "\n" << f->toString( fName );
        }

        return false;
    }
}

bool DataTypeManager :: IsCorrectFuncExact (string type, string fName, vector<string>& args, string& returnType, string& file, Associativity assoc, int priority, bool& pure) {

    // Make sure we are dealing with base types.
    if( type != "" && !IsType(type) ) {
        cout << "\nError: type " << type << " not found";
        return false;
    }

    for( int i = 0; i < args.size(); ++i )
    {
        if( !IsType( args[i] ) ) {
            cout << "\nError: type " << args[i] << " not found";
            return false;
        }
    }

    // Check if function exists
    FuncToInfoMap::const_iterator it = mFunc.find(fName);
    if (it == mFunc.end()) {
        return false;
    }

    FuncInfo* f;
    set<FuncInfo*>::const_iterator iter = it->second.begin();
    set<FuncInfo*> possibleMatches;
    for( ; iter != it->second.end(); ++iter )
    {
        bool matchesSoFar = true;
        f = *iter;
        possibleMatches.insert(f);

        // Check if class type matches if function is method of class
        if (type != f->type)
        {
            matchesSoFar = false;
            continue;
        }

        // Check function signature

        // Check to see if functions have same number of parameters
        if( f->args.size() != args.size() )
        {
            matchesSoFar = false;
            continue;
        }

        // If same number of parameters, make sure that the parameters in order
        // are of the same type or of convertible types.
        vector<string>::const_iterator argIter = args.begin();
        vector<string>::const_iterator fArgIter = f->args.begin();
        for( ; matchesSoFar && argIter != args.end() && fArgIter != f->args.end(); ++argIter, ++fArgIter )
        {
            if( *argIter != *fArgIter )
            {
                matchesSoFar = false;
            }
        }

        // Check associativity if needed
        if (assoc != NoAssoc) {
            if (f->assoc != assoc)
            {
                matchesSoFar = false;
                continue;
            }
        }

        // Check priority if needed
        if (priority != -1) {
            if (f->priority != priority)
            {
                matchesSoFar = false;
                continue;
            }
        }

        // If the function has not been ruled out so far, add it as a possible
        // match
        if( matchesSoFar ) {
            return true;
        }
    }

    cout << "\nExact match not found for function ";

    if( type != "" )
        cout << type << ".";

    cout << fName << "(";
    vector<string>::const_iterator argIter = args.begin();

    if( argIter != args.end() )
    {
        cout << *argIter;
        ++argIter;
    }

    for( ; argIter != args.end(); ++argIter )
    {
        cout << ", " << *argIter;
    }

    cout << "). Exact match required.";
    
    if ( possibleMatches.size() == 0 ) {
        cout << endl << "  No possible matches found.";
    }
    else {
        cout << endl << "  Possible matches:";
        for( set<FuncInfo*>::const_iterator iter = possibleMatches.begin(); iter != possibleMatches.end(); ++iter ) {
            FuncInfo* fInf = *iter;
           cout << endl << "    " << fInf->toString(fName); 
        }
    }

    return false;
}

void DataTypeManager :: AddTemplate( string name, TemplateType type, string file ) {
    TempToInfoMap :: iterator it = mTemp.find(name);

    if( it != mTemp.end() ) {
        TemplateInfo& info = it->second;

        if( info.type != type ) {
            cout << "\nError adding template " << name
                << ", a template of a different type already exists with that name." << endl;

            return;
        }

        if( info.file != file ) {
            cout << "\nError adding template " << name
                << ", a template of the same type is already defined in a different file" << endl;
        }
    }
    else {
        mTemp[name] = TemplateInfo( type, file );
    }
}

bool DataTypeManager :: TemplateExists( string name, TemplateType type, string& file ) {
    TempToInfoMap :: iterator it = mTemp.find( name );

    if( it != mTemp.end() ) {
        TemplateInfo & info = it->second;

        if( info.type != type )
            return false;

        file = info.file;

        return true;
    }

    return false;
}

void DataTypeManager :: AddUnaryOperator(string op, string typearg, string rettype, string file, int priority, bool pure) {

    vector<string> vec;
    vec.push_back(typearg);
    string type;
    AddFunc (type, op, vec, rettype, file, NoAssoc, priority, pure);
}

bool DataTypeManager :: IsUnary(string op, string typearg, string &rettype, string& file, bool& pure,
        vector<ArgFormat>& actualArgs) {

    vector<string> vec;
    vec.push_back(typearg);
    string type;
    return IsCorrectFunc (type, op, vec, rettype, file, NoAssoc, -1, pure, actualArgs);
}

void DataTypeManager :: AddBinaryOperator(string op, string tp1, string tp2, string rettype, string file, Associativity assoc, int priority, bool pure) {

    vector<string> vec;
    vec.push_back(tp1);
    vec.push_back(tp2);
    string type;
    AddFunc (type, op, vec, rettype, file, assoc, priority, pure);
}

bool DataTypeManager :: IsBinary(string op, string tp1, string tp2, string &rettype, string& file, bool& pure,
        vector<ArgFormat>& actualArgs) {

    vector<string> vec;
    vec.push_back(tp1);
    vec.push_back(tp2);
    string type;
    return IsCorrectFunc (type, op, vec, rettype, file, NoAssoc, -1, pure, actualArgs);
}

void DataTypeManager :: AddFunctions(string fName, vector<string>& types, string rettype, string file, bool pure) {

    vector<string> vec(types);
    string type;
    AddFunc (type, fName, vec, rettype, file, NoAssoc, -1, pure);
}

void DataTypeManager :: AddFunctionTemplate( string fName, string file ) {
    AddTemplate( fName, T_FUNC, file );
}

bool DataTypeManager :: IsFunction(string fName, vector<string>& types, string& rettype, string& file, bool& pure,
        vector<ArgFormat>& actualArgs) {

    vector<string> vec(types);
    string type;
    return IsCorrectFunc (type, fName, vec, rettype, file, NoAssoc, -1, pure, actualArgs);
}

bool DataTypeManager :: IsFunctionTemplate( string fName, string& file ) {
    return TemplateExists( fName, T_FUNC, file );
}

void DataTypeManager :: AddMethod(string type, string mName, vector<string>& types, string rettype, string file, bool pure) {

    vector<string> vec(types);
    AddFunc (type, mName, vec, rettype, file, NoAssoc, -1, pure);
}

bool DataTypeManager :: IsMethod(string type, string mName, vector<string>& types, string& rettype, string& file, bool& pure,
        vector<ArgFormat>& actualArgs) {

    vector<string> vec(types);
    return IsCorrectFunc (type, mName, vec, rettype, file, NoAssoc, -1, pure, actualArgs);
}

void DataTypeManager :: AddFilter(string fName, vector<string>& typeargs, string file, bool pure) {

    vector<string> vec(typeargs);
    string ret;
    string type;
    AddFunc (type, fName, vec, ret, file, NoAssoc, -1, pure);
}

bool DataTypeManager :: IsFilter(string fName, vector<string>& typeargs, string& file, bool& pure,
        vector<ArgFormat>& actualArgs) {

    vector<string> vec(typeargs);
    string ret;
    string type;
    return IsCorrectFunc (type, fName, vec, ret, file, NoAssoc, -1, pure, actualArgs);
}

void DataTypeManager :: AddGLA(string glaName, vector<string>& typeargs,
        vector<string>& typeret, string fileName, string_vector& reqStates ) {
    if( !DoesTypeExist( glaName ) ) {
        AddBaseType( glaName, fileName );
    }
    else {
        // Duplicate definition.
        cout << "\nWarning: Attempting to add a duplicate GLA " << glaName;
        return;
    }

    string fName_add = "AddItem_" + glaName;
    string fName_ret = "GetResult_" + glaName;
    string fName_req = "Requires_" + glaName;

    string ret = "";

    vector<string> args_vec(typeargs);
    vector<string> ret_vec(typeret);
    vector<string> req_vec(reqStates);

    AddFunc( glaName, fName_add, args_vec, ret, fileName, NoAssoc, -1, false );
    AddFunc( glaName, fName_ret, ret_vec, ret, fileName, NoAssoc, -1, false );
    AddFunc( glaName, fName_req, req_vec, ret, fileName, NoAssoc, -1, false );
}

void DataTypeManager :: AddGLATemplate( string glaName, string filename ) {
    AddTemplate( glaName, T_GLA, filename );
}

bool DataTypeManager :: IsGLATemplate( string glaName, string& filename ) {
    return TemplateExists( glaName, T_GLA, filename );
}

bool DataTypeManager ::  IsGLA( string& glaName, vector<string>& typeargs,
        vector<string>& typeret, string& file, vector<string>& reqStates,
        vector<ArgFormat>& /* out param */ actualArgs ) {

    vector<string> arg_vec(typeargs);
    vector<string> ret_vec(typeret);
    vector<string> req_vec(reqStates);
    string ret("");

    if( !IsType( glaName ) ) {
        cout << "\nError: No GLA " << glaName << " known to system!";
        return false;
    }

    string fName_add = "AddItem_" + glaName;
    string fName_ret = "GetResult_" + glaName;
    string fName_req = "Requires_" + glaName;

    bool pure;

    // Check the arguments and get any formatting necessary.
    bool correct = IsCorrectFunc( glaName, fName_add, arg_vec, ret, file, NoAssoc, -1, pure, actualArgs );

    // If we didn't get a match for a concrete GLA, see if a template with this
    // name exists.
    if( !correct ) {
        return false;
    }

    // Check to make sure the required states are the same
    correct = IsCorrectFuncExact( glaName, fName_req, req_vec, ret, file, NoAssoc, -1, pure );
    if( !correct ) {
        return false;
    }

    // If ret_vec is blank, then we don't care about what the output is.
    if( ret_vec.size() == 0 )
        return true;

    return IsCorrectFuncExact( glaName, fName_ret, ret_vec, ret, file, NoAssoc, -1, pure );
}

bool DataTypeManager :: GLAExists( string& glaName, string& file ) {
    if( !IsType( glaName ) )
        return false;

    file = GetTypeFile( glaName );

    return true;
}

void DataTypeManager :: AddGT(string gtName, vector<string>& typeargs,
        vector<string>& typeret, string fileName, vector<string>& reqStates ) {
    if( !DoesTypeExist( gtName ) ) {
        AddBaseType( gtName, fileName );
    }
    else {
        // Duplicate definition.
        cout << "\nWarning: Attempting to add a duplicate GT " << gtName;
        return;
    }

    string fName_add = "AddItem_" + gtName;
    string fName_ret = "GetResult_" + gtName;
    string fName_req = "Requires_" + gtName;

    string ret = "";

    vector<string> args_vec(typeargs);
    vector<string> ret_vec(typeret);
    vector<string> req_vec(reqStates);

    AddFunc( gtName, fName_add, args_vec, ret, fileName, NoAssoc, -1, false );
    AddFunc( gtName, fName_ret, ret_vec, ret, fileName, NoAssoc, -1, false );
    AddFunc( gtName, fName_req, req_vec, ret, fileName, NoAssoc, -1, false );
}

void DataTypeManager :: AddGTTemplate( string gtName, string filename ) {
    AddTemplate( gtName, T_GT, filename );
}

bool DataTypeManager :: IsGTTemplate( string gtName, string& filename ) {
    return TemplateExists( gtName, T_GT, filename );
}

bool DataTypeManager ::  IsGT( string& gtName, vector<string>& typeargs,
        vector<string>& typeret, string& file, vector<string>& reqStates,
        vector<ArgFormat>& /* out param */ actualArgs ) {

    vector<string> arg_vec(typeargs);
    vector<string> ret_vec(typeret);
    vector<string> req_vec(reqStates);
    string ret("");

    if( !IsType( gtName ) ) {
        cout << "\nError: No GT " << gtName << " known to system!";
        return false;
    }

    string fName_add = "AddItem_" + gtName;
    string fName_ret = "GetResult_" + gtName;
    string fName_req = "Requires_" + gtName;

    bool pure;

    // Check the arguments and get any formatting necessary.
    bool correct = IsCorrectFunc( gtName, fName_add, arg_vec, ret, file, NoAssoc, -1, pure, actualArgs );

    // If we didn't get a match for a concrete GT, see if a template with this
    // name exists.
    if( !correct ) {
        return false;
    }

    // Check to make sure the required states are the same.
    correct = IsCorrectFuncExact( gtName, fName_req, req_vec, ret, file, NoAssoc, -1, pure );
    if( !correct )
        return false;

    return IsCorrectFuncExact( gtName, fName_ret, ret_vec, ret, file, NoAssoc, -1, pure );
}

bool DataTypeManager :: GTExists( string& gtName, string& file ) {
    if( !IsType( gtName ) )
        return false;

    file = GetTypeFile( gtName );

    return true;
}

void DataTypeManager :: AddGF(string gfName, vector<string>& typeargs, string fileName,
        vector<string>& reqStates ) {
    if( !DoesTypeExist( gfName ) ) {
        AddBaseType( gfName, fileName );
    }
    else {
        // Duplicate definition.
        cout << "\nWarning: Attempting to add a duplicate GF " << gfName;
        return;
    }

    string fName = "Filter_" + gfName;
    string fName_req = "Requires_" + gfName;

    string ret = "bool";
    string ret_req = "";

    vector<string> args_vec(typeargs);
    vector<string> req_vec(reqStates);

    AddFunc( gfName, fName, args_vec, ret, fileName, NoAssoc, -1, false );
    AddFunc( gfName, fName_req, req_vec, ret_req, fileName, NoAssoc, -1, false );
}

void DataTypeManager :: AddGFTemplate( string gfName, string filename ) {
    AddTemplate( gfName, T_GF, filename );
}

bool DataTypeManager :: IsGFTemplate( string gfName, string& filename ) {
    return TemplateExists( gfName, T_GF, filename );
}

bool DataTypeManager ::  IsGF( string& gfName, vector<string>& typeargs,
        string& file, vector<string>& reqStates,
        vector<ArgFormat>& /* out param */ actualArgs ) {

    vector<string> arg_vec(typeargs);
    vector<string> req_vec(reqStates);
    string ret("bool");

    if( !IsType( gfName ) ) {
        cout << "\nError: No GF " << gfName << " known to system!";
        return false;
    }

    string fName = "Filter_" + gfName;
    string fName_req = "Requires_" + gfName;

    bool pure;

    // Check to make sure that the required states are correct.
    string ret_req = "";
    bool correct = IsCorrectFuncExact( gfName, fName_req, req_vec, ret_req, file, NoAssoc, -1, pure );
    if( !correct )
        return false;

    // Check the arguments and get any formatting necessary.
    return IsCorrectFunc( gfName, fName, arg_vec, ret, file, NoAssoc, -1, pure, actualArgs );
}

bool DataTypeManager :: GFExists( string& gfName, string& file ) {
    if( !IsType( gfName ) )
        return false;

    file = GetTypeFile( gfName );

    return true;
}

void DataTypeManager :: AddGIST(string gistName, vector<string>& stateargs,
        vector<string>& typeret, string fileName ) {
    if( !DoesTypeExist( gistName ) ) {
        AddBaseType( gistName, fileName );
    }
    else {
        // Duplicate definition.
        cout << "\nWarning: Attempting to add a duplicate GIST " << gistName;
        return;
    }

    string fName_req = "Requires_" + gistName;
    string fName_ret = "GetResult_" + gistName;

    string ret = "";

    vector<string> req_vec(stateargs);
    vector<string> ret_vec(typeret);

    AddFunc( gistName, fName_req, req_vec, ret, fileName, NoAssoc, -1, false );
    AddFunc( gistName, fName_ret, ret_vec, ret, fileName, NoAssoc, -1, false );
}

void DataTypeManager :: AddGISTTemplate( string gistName, string filename ) {
    AddTemplate( gistName, T_GIST, filename );
}

bool DataTypeManager :: IsGISTTemplate( string gistName, string& filename ) {
    return TemplateExists( gistName, T_GIST, filename );
}

bool DataTypeManager ::  IsGIST( string& gistName, vector<string>& stateargs,
        vector<string>& typeret, string& file ) {

    vector<string> req_vec(stateargs);
    vector<string> ret_vec(typeret);
    string ret("");

    if( !IsType( gistName ) ) {
        cout << "\nError: No GIST " << gistName << " known to system!";
        return false;
    }

    string fName_req = "Requires_" + gistName;
    string fName_ret = "GetResult_" + gistName;

    bool pure;

    // Check the arguments and get any formatting necessary.
    bool correct = IsCorrectFuncExact( gistName, fName_req, req_vec, ret, file, NoAssoc, -1, pure );

    // If we didn't get a match for a concrete GT, see if a template with this
    // name exists.
    if( !correct ) {
        return false;
    }

    // If ret_vec is blank, then we don't care about what the output is.
    if( ret_vec.size() == 0 )
        return true;

    return IsCorrectFuncExact( gistName, fName_ret, ret_vec, ret, file, NoAssoc, -1, pure );
}

bool DataTypeManager :: GISTExists( string& gistName, string& file ) {
    if( !IsType( gistName ) )
        return false;

    file = GetTypeFile( gistName );

    return true;
}

void DataTypeManager :: GenerateIncludes(vector<string>& types, string& result) {

    for (int i = 0; i < types.size(); i++) {
        IsType(types[i]);
        string& baseType = types[i];
        TypeToInfoMap::const_iterator iter = mType.find(baseType);
        if (iter != mType.end()) {
            result += "#include " + (iter->second)->file;
            result += "\n";
        } else {
            cout << "\nError : No entry exists for type " << (types[i]).c_str();
        }
    }
}

string DataTypeManager :: GetTypeFile(string type) {
    if( !IsType(type) ) {
        cout << "\nError: attempting to get source file for unknown type " << type;
        return string("");
    }

    TypeToInfoMap::const_iterator it = mType.find(type);
    return (it->second)->file;
}

DataTypeManager& DataTypeManager::GetDataTypeManager() {
    if (instance == NULL) {
        instance = new DataTypeManager();
        //instance->Load();
    }
    return *instance;
}
