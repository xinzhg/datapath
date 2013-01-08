#include "stdint.h"
#include "time.h"
#include "sampling/GUSestimate/GeneralizedSamplingEstimate.h"
#include "base/Types/BIGINT.h"
#include "base/Types/DOUBLE.h"
using namespace std;
#ifndef _BernoulliGLA3limit_H_ 
#define _BernoulliGLA3limit_H_ 

#include "Errors.h"
using namespace std;

// PARAMETERS
#define LOWER_LIMIT 10000000
#define UPPER_LIMIT 4*LOWER_LIMIT

/** Info for the meta-GLAs
 * GLA_DESC
 *
 * NAME(</BernoulliGLA3limit/>)
 * INPUTS(</(id1, BIGINT), (id2, BIGINT), (id3, BIGINT), (agg, DOUBLE)/>)
 * OUTPUTS(</(variance, DOUBLE)/>)
 * RESULT_TYPE(</single/>)
 * END_DESC
 */

class BernoulliGLA3limit;

//Supriya's congruent hash
double congruenthash(double, double);
	
class BernoulliGLA3limit {
 public:

  static uint64_t myH_b; // my own seed for the CongruentHash
  static uint64_t RandomSeed(void){
    return ((uint64_t)lrand48() << 16) | lrand48();
  }

  /*Note: you cannot have different p's for different dimensions 
    once you filter one dimension with a certain p, there is no guarantee 
    that you will get the required number of tuples with the second dimension
    that satisfies the bernoulli condition for some other p. Since we care only about
    resizing to ease computation, we subsample all the relation s with the same p*/
  double p;
  int relations_no; // should be k
  int numtotaltuples;
	
  //max & min no. of samples allowed 
  size_t upper_limit;
  size_t lower_limit;

  /** maintain the datastructure that keeps the tuples */
  std::vector< mytuple<3> > V;

 public: 

 BernoulliGLA3limit(): p(1.0) {
    upper_limit = UPPER_LIMIT;
    lower_limit = LOWER_LIMIT;
    numtotaltuples = 0;
    //cout<<"Bernoulli constructor"<<endl;
  } 

 BernoulliGLA3limit(double _p): p(_p){
    upper_limit = 2000;
    lower_limit = 1000;
    //cout<<"Bernoulli constructor"<<endl
  } 
	
  void printtuples() {
    for(size_t i = 0; i < V.size(); i++) {
      cout << V[i] << endl;
    }  
  }

  bool deterministic_coin_flip(uint64_t id11, uint64_t id22, uint64_t id33, double p) {
    uint64_t ch[3];
    ch[0] = CongruentHashModified(id11, myH_b);
    ch[1] = CongruentHashModified(id22, myH_b);
    ch[2] = CongruentHashModified(id33, myH_b);
    if ((ch[0] < p*(1ULL<<61)) && (ch[1] < p*(1ULL<<61)) && (ch[2] < p*(1ULL<<61)))
      return true;
    else
      return false;
  } 
  
  void AddItem(const BIGINT& id11, const BIGINT& id22, const BIGINT& id33, const DOUBLE& agg){
    // determine if this tuple gets included
    numtotaltuples++;
    uint64_t ch[3];
    uint64_t id[3];
    id[0] = id11;
    id[1] = id22;
    id[2] = id33;
    //derive congruent hash values
    /*    ch[0] = congruenthash(11.0, id11);
	  ch[1]= congruenthash(13.0, id22);
	  ch[2] = congruenthash(17.0, id33);
    */
    ch[0] = CongruentHashModified(Hash(id11), myH_b);
    ch[1] = CongruentHashModified(Hash(id22), myH_b);
    ch[2] = CongruentHashModified(Hash(id33), myH_b);

    if (ch[0] > p*(1ULL<<61)) {
      return;
    }
    if (ch[1] > p*(1ULL<<61)) {
      return;
    }
    if (ch[2] > p*(1ULL<<61)) {
      return;
    }
    else {
      // form and include the tuple
      mytuple<3> T(id, agg);
      V.push_back(T);
      // cout <<T<<endl;
    }

    if (V.size() > upper_limit){
      resize();
    }
  }

  /* int resize() - checks if the total number of tuples is between upper and lower. If not, it approximately halves the sample
     untill the total number of samples is between lower and upper. It returns the final number of samples and modifies p1, p2 
     and p3 accordingly.
  */ 
  void resize() {
    while (V.size() > upper_limit) {
      std::vector< mytuple <3> > Vcopy(V);
      Vcopy.reserve(V.size());
      Vcopy.clear();
      p = p/1.25992105;
      for (size_t i=0; i < V.size(); i++) {
	if (deterministic_coin_flip(V[i].ch[0], V[i].ch[1], V[i].ch[2], p)) {
	  Vcopy.push_back(V[i]);
	}
      }
      cout<<"V was too big = "<<V.size()<<" After cutting down, p ="<< p<<" and Vcopy.size() = "<<Vcopy.size()<<endl;

      while (Vcopy.size() < lower_limit) {
	//bump up the p's and add tuples to Vcopy
	p = p* 1.14471424;
	Vcopy.clear();
	for (size_t i = 0; i < V.size(); i++) {
	  if(deterministic_coin_flip(V[i].ch[0], V[i].ch[1], V[i].ch[2], p)) {
	    Vcopy.push_back(V[i]);
	  }
	}
	cout<<"V was too small. After bumping up p ="<< p<<" and Vcopy.size() = "<<Vcopy.size()<<endl;
      }
  
      //swap Vcopy and 
      V.swap(Vcopy);
      Vcopy.clear();
    }
    cout <<"p after resize() = " << p <<" V.size after resize() = "<<V.size()<< endl;
  }


  
  void AddState(BernoulliGLA3limit& other){
    FATALIF(this == &other, "Why are the two objects identical?");

    // put the two sets together
    cout<<"In AddState"<< endl;
    if (p <= other.p) {
      cout<<"p <= other.p "<<"V.size() = "<< V.size()<< endl;
      for (size_t i = 0; i <  other.V.size(); i++) {
	if (deterministic_coin_flip(other.V[i].ch[0], other.V[i].ch[1], other.V[i].ch[2], p)) {
	  V.push_back(other.V[i]);
	}
      }
      cout<<"V.size() after adding other = "<<V.size()<< endl;
    }
    else {
      p = other.p;
      for (size_t i = 0; i < V.size(); i++) {
	if (!(deterministic_coin_flip(V[i].ch[0], V[i].ch[1], V[i].ch[2], p))) {
	  V.erase(V.begin() + i );
	}
      }
      for (size_t i = 0; i < other.V.size(); i++) {
	V.push_back(other.V[i]);
      }
    }
    cout<<" In AddState, numtotaltuples = "<< numtotaltuples<< "other.numtotaltuples = "<< other.numtotaltuples<< endl; 
    numtotaltuples += other.numtotaltuples;
    cout<<"modified count = "<< numtotaltuples<< "V.size() before resize() = " << V.size()<<endl;
    resize();
  }
  
  /* std::vector<tuple <3>> printtuples() { */
  /*   return V; */
  /* } */
  
  void GetResult(double& variance){
    /*    
	  AddItem(1,12,100,1);
	  AddItem(2,12,101,1);
	  AddItem(3,11,101,1);
	  AddItem(1,11,102,1);
	  AddItem(2,10,102,1);
	    AddItem(3,10,102,1);
      */
      cout<<"Total num of input tuples " << numtotaltuples << endl;
      cout<<"num of sampled tuples = " << V.size() << endl;
      cout<<"p = "<<p;
      /* TB sample from PB*/
      double a = 1.0e-9;
      double b[] = {9.999999999999999e-19, 1.0e-15, 1.0e-15, 1.0000000000000002e-12, 1.0e-15, 1.0000000000000002e-12, 1.0e-12, 1.0e-9};
    

      /*Tb sample from 10TB
	double a = 0.0010000000000000002;
	double b[] = {1.0000000000000002e-6, 1.0e-5, 1.0e-5, 0.0001, 1.0e-5, 0.0001, 0.00010000000000000002, 0.0010000000000000002};
      */

      /* 1Tb sample from 1Tb
	 double a = 1.0;
	 double b[] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
      */
      clock_t t1, t2;
      t1 = clock();
      GeneralizedSamplingData G(3, a, b, p, V);
      G.GenerateStatistics();
      G.ComputeUnbiasedCoefs();
      variance =  G.ComputeVariance();
      t2 = clock();
      float diff ((float)t2 - (float)t1);
      float diffsec = diff/CLOCKS_PER_SEC;
      cout<<"Time taken for analysis = "<< diffsec<<endl;
    }
   
  }; 

  double congruenthash(double seed, double id){
    double bucket;
    bucket = fmod(id,seed) /seed;
    return bucket;
  };


uint64_t BernoulliGLA3limit::myH_b=BernoulliGLA3limit::RandomSeed();

#endif //_BernoulliGLA3limit_H_
