dnl #
dnl #  Copyright 2012 Christopher Dudley
dnl #
dnl #  Licensed under the Apache License, Version 2.0 (the "License");
dnl #  you may not use this file except in compliance with the License.
dnl #  You may obtain a copy of the License at
dnl #
dnl #      http://www.apache.org/licenses/LICENSE-2.0
dnl #
dnl #  Unless required by applicable law or agreed to in writing, software
dnl #  distributed under the License is distributed on an "AS IS" BASIS,
dnl #  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
dnl #  See the License for the specific language governing permissions and
dnl #  limitations under the License.
dnl #
dnl # GLA for finding all tuples that have the most extreme (either maximum or
dnl # minimum) values for a set of attributes.
dnl
dnl # GLA Template arguments:
dnl # GLA_NAME: The name of the resulting GLA
dnl # MIN_ATTS: List of elements that are in the rank (var, type)...
dnl # EXT_ATTS: List of elements that are extra information (optional) (var, type)...

m4_redefine(</m4_fix_order/>, </dnl
<//>m4_if(m4_to_upper(</$1/>), </MIN/>, </MIN/>, </m4_if(m4_to_upper(</$1/>), </MAX/>, </MAX/>, </MIN/>)/>)<//>dnl
/>)dnl

m4_redefine(</ORDER/>,</m4_third($1)/>)

m4_define(</ExtremeTuplesGLA/>, </dnl
m4_redefine(</GLA_NAME/>, </$1/>)dnl
m4_redefine(</EXT_ATTS/>, </$3/>)dnl
dnl
m4_redefine(</__TEMP__/>,</$2/>)dnl
m4_foreach(</_A_/>,</__TEMP__/>,</dnl
<//>m4_redefine(</_TARG_/>, m4_quote(</(/>VAR(_A_)</, />TYPE(_A_)</, />m4_fix_order(ORDER(_A_))</)/>))
<//>m4_append(</MIN_ATTS/>, _TARG_, </</, />/>)
/>)dnl
dnl
m4_redefine(</MY_INPUT/>, m4_quote(GLUE_LISTS(</$2/>, </EXT_ATTS/>)))dnl
m4_redefine(</MY_OUTPUT/>, m4_defn(</MY_INPUT/>))dnl
m4_redefine(</MY_REZTYPE/>, </multi/>)dnl
dnl
#include "DataTypes.h"
#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <inttypes.h>

using namespace std;

/** Informatio for Meta-GLAs
    m4_qdefine(</GLA_NAME</_INPUT/>/>, </MY_INPUT/>)
    m4_qdefine(</GLA_NAME</_OUTPUT/>/>, </MY_OUTPUT/>)
    m4_qdefine(</</GLA_REZTYPE_/>GLA_NAME/>, </MY_REZTYPE/>)
 */

struct GLA_NAME<//>_Tuple {

m4_foreach(</_A_/>, </MY_INPUT/>, </dnl
    TYPE(_A_) VAR(_A_);
/>)dnl

    GLA_NAME<//>_Tuple() {
m4_foreach(</_A_/>, </MIN_ATTS/>, </dnl
        VAR(_A_) = TYPE(_A_)<//>();
/>)dnl
    }

    GLA_NAME<//>_Tuple(TYPED_REF_ARGS(MY_INPUT)) {
m4_foreach(</_A_/>, </MY_INPUT/>, </dnl
        this->VAR(_A_) = VAR(_A_);
/>)dnl
    }

    GLA_NAME<//>_Tuple & operator = ( const GLA_NAME<//>_Tuple & _other ) {
m4_foreach(</_A_/>, </MY_INPUT/>, </dnl
        VAR(_A_) = _other.VAR(_A_;)
/>)dnl
        return *this;
    }

};

// Auxiliary function to compare tuples
// Returns true if t1 is better than t2
inline bool BetterThan( const GLA_NAME<//>_Tuple & t1, const GLA_NAME<//>_Tuple & t2 ) {
m4_foreach(</_A_/>,</MIN_ATTS/>,</dnl
    if( (TYPE(_A_)) t1.VAR(_A_) m4_if(ORDER(_A_), </MIN/>, <, >) (TYPE(_A_)) t2.VAR(_A_) )
        return true;
    else if( (TYPE(_A_)) t1.VAR(_A_) m4_if(ORDER(_A_), </MIN/>, >, <) (TYPE(_A_)) t2.VAR(_A_) )
        return false;
/>)dnl
    return false;
}

class GLA_NAME {
private:

    // Count number of tuples encountered
    uint64_t count;

    // Vector of tuples
    typedef vector<GLA_NAME<//>_Tuple> TupleVector;

    TupleVector tuples;

    // Position for output iterator
    TupleVector::size_type pos;

    // Current minimum
m4_foreach(</_A_/>, </MIN_ATTS/>, </dnl
    TYPE(_A_) VAR(_A_);
/>)dnl

    // Internal function to add tuple.
    void AddTupleInternal( GLA_NAME<//>_Tuple & tuple );

public:

    // Constructor and destructor
    GLA_NAME<//>() : count(0), pos(0) {}
    ~GLA_NAME<//>() {}

    // function to add an item
    void AddItem( TYPED_ARGS(MY_INPUT) );


    // take the state from other and incorporate it into this object.
    void AddState( GLA_NAME & other );

    // finalize the state and prepare for result extraction
    void Finalize();

    // get the result
    bool GetNextResult( TYPED_REF_ARGS(MY_OUTPUT) );
};

void GLA_NAME :: AddItem( TYPED_ARGS(MY_INPUT) ) {
    ++count;
    if( tuples.size() > 0<//>dnl
m4_foreach(</_A_/>, </MIN_ATTS/>, </dnl
 && (TYPE(_A_)) VAR(_A_) m4_if(ORDER(_A_), </MIN/>, >, <) (TYPE(_A_)) this->VAR(_A_)<//>dnl
/>)dnl
 ) { // fast path
        return;
    }
    else if( tuples.size() > 0<//>dnl
m4_foreach(</_A_/>, </MIN_ATTS/>, </dnl
 && (TYPE(_A_)) VAR(_A_) == (TYPE(_A_)) this->VAR(_A_)<//>dnl
/>)dnl
 ) { // add to tuples
        GLA_NAME<//>_Tuple tuple( ARGS(MY_INPUT) );
        AddTupleInternal( tuple );
    }
    else { // New extreme
        tuples.clear();
        GLA_NAME<//>_Tuple tuple( ARGS(MY_INPUT) );
        AddTupleInternal( tuple );

m4_foreach(</_A_/>, </MIN_ATTS/>, </dnl
        this->VAR(_A_) = VAR(_A_);
/>)dnl
    }
}

void GLA_NAME :: AddTupleInternal( GLA_NAME<//>_Tuple & tuple ) {
    tuples.push_back( tuple );
}

void GLA_NAME :: AddState( GLA_NAME & other ) {
    count += other.count;
    if( tuples.size() > 0<//>dnl
m4_foreach(</_A_/>, </MIN_ATTS/>, </dnl
 && (TYPE(_A_)) other.VAR(_A_) m4_if(ORDER(_A_), </MIN/>, >, <) (TYPE(_A_)) this->VAR(_A_)<//>dnl
/>)dnl
 ) // fast path
        return;
    else if( tuples.size() > 0<//>dnl
m4_foreach(</_A_/>, </MIN_ATTS/>, </dnl
 && (TYPE(_A_)) other.VAR(_A_) == (TYPE(_A_)) this->VAR(_A_)<//>dnl
/>)dnl
 ) { // add to tuples
        for( TupleVector::iterator it = other.tuples.begin(); it != other.tuples.end(); ++it )
            AddTupleInternal( *it );
    }
    else { // New minimum
        tuples.clear();
        for( TupleVector::iterator it = other.tuples.begin(); it != other.tuples.end(); ++it )
            AddTupleInternal( *it );

m4_foreach(</_A_/>, </MIN_ATTS/>, </dnl
        this->VAR(_A_) = other.VAR(_A_);
/>)dnl
    }
}

void GLA_NAME :: Finalize() {
    pos = 0;
    sort( tuples.begin(), tuples.end(), BetterThan );
}

bool GLA_NAME :: GetNextResult( TYPED_REF_ARGS(MY_OUTPUT) ) {
    if( pos == tuples.size() ) {
        return false;
    }
    else {
        GLA_NAME<//>_Tuple & tuple = tuples[pos++];

m4_foreach(</_A_/>, </MY_OUTPUT/>, </dnl
        VAR(_A_) = tuple.VAR(_A_);
/>)dnl

        return true;
    }
}
/>)
