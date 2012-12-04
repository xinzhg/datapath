#ifndef _SUMPROBGLA_H_ 
#define _SUMPROBGLA_H_ 


/** Info for the meta-GLAs
 * GLA_DESC
 *
 * NAME(</SumProbGLA/>)
 * INPUTS(</(v, INT),(p, DOUBLE)/>)
 * OUTPUTS(</(confLow, INT), (confHigh, INT)/>)
 * RESULT_TYPE(</single/>)
 * LIBS(zPoly)
 *
 * END_DESC
 */

#include <map>
#include "base/Types.h"
#include "probdb/zPoly/zPoly.h"
#include "probdb/GLAs/CountProbGLA.h"

#define VALTYPE INT

class SumProbGLA {
  typedef std::map<VALTYPE, CountProbGLA*> Map;
  Map poly;
  CountProbGLA res;
  public: 
  
  //typedef struct {int a,b;} c_int;

  void AddItem(BIGINT v, DOUBLE p) { 
    Map::iterator it=poly.find(v);
    if ( it == poly.end() ){ // not in
      CountProbGLA* gla = new CountProbGLA;
      gla->AddItem(p);
      poly.insert(make_pair(v,gla));
    } else {
      it->second->AddItem(p);
    }

}

  void AddState(SumProbGLA& other){
    for(Map::iterator it = other.poly.begin(); it != other.poly.end(); it++){
      int v = it->first;
      Map::iterator it2=poly.find(v);
      if ( it2 == poly.end() ){ // not in
	CountProbGLA* gla = new CountProbGLA;
	poly.insert(make_pair(v,gla));
      } 
      
      poly[v]->AddState(*(it->second));
      
    }
  }
 
  void Finalize(){
    // go through partial PGFs
    for(Map::iterator it = poly.begin(); it != poly.end(); it++) 
    {
      it->second->spread(it->first); // spread the coeficients inserting zeros
      res.AddState(*(it->second)); // combine with current result
    }
  }

  float Equal(float a) { return res.Equal(a); }  

  float Greater(float a) { return res.Greater(a); }

  float GreaterEq(float a) { return res.GreaterEq(a); }

  template<typename PGF2> float Equal(PGF2& other) { return res.Equal(other); }

  template<typename PGF2> float Greater(PGF2& other) { return res.Greater(other); }

  template<typename PGF2> float GreaterEq(PGF2& other) { return res.GreaterEq(other); }

  CountProbGLA::c_int ConfidenceInterval(float conf) { return res.ConfidenceInterval (conf); }

  void GetResult(int &a, int &b) { Finalize(); return res.GetResult(a, b); }

}
; 

#endif //_SUMPROBGLA_H_
