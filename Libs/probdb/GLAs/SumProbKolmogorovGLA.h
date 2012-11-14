#ifndef _SUMPROBKOLMOGOROVGLA_H_ 
#define _SUMPROBKOLMOGOROVGLA_H_ 

#include <math.h>
#include "CountProbGLA.h"
#define O 6
#define B 10 //half of the support of distribution in standard deviations
#define N 100 // number of bins for distribution

#define STIRLING   \
  c[0][0][0] = -1; \
  c[1][0][0] = -2; \
  c[1][0][1] = -1; \
  c[1][1][1] = 2;  \
  c[2][0][0] = -3; \
  c[2][0][1] = -3; \
  c[2][0][2] = -1; \
  c[2][1][1] = 6;  \
  c[2][1][2] = 6;   \
  c[2][2][2] = -6;  \
  c[3][0][0] = -4;  \
  c[3][0][1] = -6;  \
  c[3][0][2] = -4;  \
  c[3][0][3] = -1;  \
  c[3][1][1] = 12;  \
  c[3][1][2] = 24;  \
  c[3][1][3] = 14;  \
  c[3][2][2] = -24; \
  c[3][2][3] = -36; \
  c[3][3][3] = 24;  \
  c[4][0][0] = -5;  \
  c[4][0][1] = -10; \
  c[4][0][2] = -10; \
  c[4][0][3] = -5;  \
  c[4][0][4] = -1;  \
  c[4][1][1] = 20;  \
  c[4][1][2] = 60;  \
  c[4][1][3] = 70;  \
  c[4][1][4] = 30;  \
  c[4][2][2] = -60; \
  c[4][2][3] = -180;\
  c[4][2][4] = -150;\
  c[4][3][3] = 120; \
  c[4][3][4] = 240; \
  c[4][4][4] = -120;\
  c[5][0][0] = -6;  \
  c[5][0][1] = -15; \
  c[5][0][2] = -20; \
  c[5][0][3] = -15; \
  c[5][0][4] = -6;  \
  c[5][0][5] = -1;  \
  c[5][1][1] = 30;  \
  c[5][1][2] = 120; \
  c[5][1][3] = 210; \
  c[5][1][4] = 180; \
  c[5][1][5] = 62;  \
  c[5][2][2] = -120;\
  c[5][2][3] = -540;\
  c[5][2][4] = -900;\
  c[5][2][5] = -540;\
  c[5][3][3] = 360; \
  c[5][3][4] = 1440;\
  c[5][3][5] = 1560;\
  c[5][4][4] = -720;\
  c[5][4][5] = -1800;\
  c[5][5][5] = 720;

class SumProbKolmogorovGLA {
  // cumulants of the true distribution
  long double cm[O];
  // moments of the true distribution 
  double nu[O]; 
  // moments of the initial approximation
  double bm[O];   
  //approximating distribution
  double p[N]; 
  //differences
  double d[N], d2[N]; 
  double a[O];
  double x[N];
  double sumDet;
  int n;

  typedef struct { double a,b; } c_dbl;
 public:
  SumProbKolmogorovGLA(void) { for(int i = 0; i < O; i++) cm[i] = 0.0; sumDet = 0.0; n = 0;}

  //update cumulants of the true distribution 
  //with the new data point
  void AddItem(DOUBLE x, DOUBLE p){
    //cout <<x<<"\t"<<p<< endl;
    if (p<0.0 || p>1.0)
      return;
    //cout << x<<"\t"<<p<< endl;
    cm[0] += p*x;
    cm[1] += x*x * p*(1.0-p);
    cm[2] += x*x*x * p*(1.0-p)*(1.0-2.0*p);
    cm[3] += x*x*x*x * p*(1.0-p)*(1.0-6.0*p*(1.0-p));
    cm[4] += x*x*x*x*x * p*(1.0-p)*(1.0-2.0*p)*(1.0-12.0*p*(1.0-p));
    cm[5] += x*x*x*x*x*x * p*(1.0-p)*( 1.0+p*(1.0-p)*(120.0*p*(1.0-p)-30.0));
    sumDet += x;
    n++;
  }

  void AddState(SumProbKolmogorovGLA& o) { for (int i = 0; i < O; i++) cm[i] += o.cm[i]; sumDet += o.sumDet;}
  
  void Finalize(void){
    int c[O][O][O];
    STIRLING
      //  cout << "cumulants\t" << cm[0] << "\t"<< cm[1] << "\t"<< cm[2] << "\t"<< cm[3] << "\t"<< cm[4] << "\t"<< cm[5] << endl;

    //compute moments of true distribution
      /*    nu[0] = cm[0];
	    nu[1] = cm[1] + cm[0]*cm[0];
	    nu[2] = cm[2] + 3*cm[1]*cm[0] + cm[0]*cm[0]*cm[0];
	    nu[3] = cm[3] + 4*cm[2]*cm[0] + 3*cm[1]*cm[1] + 6*cm[1]*cm[0]*cm[0] + cm[0]*cm[0]*cm[0]*cm[0];
	    nu[4] = cm[4] + 5*cm[3]*cm[0] + 10*cm[2]*cm[1] + 10*cm[2]*cm[0]*cm[0] + 15*cm[1]*cm[1]*cm[0] + 10*cm[1]*cm[0]*cm[0]*cm[0] + cm[0]*cm[0]*cm[0]*cm[0]*cm[0];
	    nu[5] = cm[5] + 6*cm[4]*cm[0] + 15*cm[3]*cm[1] + 15*cm[3]*cm[0]*cm[0] + 10*cm[2]*cm[2]*cm[2] + 60*cm[2]*cm[1]*cm[0] + 20*cm[2]*cm[0]*cm[0]*cm[0] + 15*cm[1]*cm[1]*cm[1] + 45*cm[1]*cm[1]*cm[0]*cm[0] + 15*cm[1]*cm[0]*cm[0]*cm[0]*cm[0] + cm[0]*cm[0]*cm[0]*cm[0]*cm[0]*cm[0];
	    
      */
    
      nu[0] = 0.0;
    
    nu[1] = cm[1];
    nu[2] = cm[2];
    nu[3] = cm[3]+3*cm[1]*cm[1];
    nu[4] = cm[4]+10*cm[2]*cm[1];
    nu[5] = cm[5]+15*cm[3]*cm[1]+10*cm[2]*cm[2]+15*cm[1]*cm[1]*cm[1];

    cout << "n=" << n << endl;
    cout << "moments\t" << nu[0] << "\t" << nu[1] << "\t" << nu[2] << "\t" << nu[3] << "\t" << nu[4] << "\t" << nu[5] << endl;

    //compute initial approximation
    double cdf = 0.0, newcdf = 0.0, sigma = pow(cm[1], 0.5);

    // the width of the histogram of the approximation
    const double delta = 2.0 * (double)B * sigma / ((double)N - 2.0);
    const double start = cm[0] -(double)B*sigma;
    for(int i = 0; i < N - 1; i++){
      x[i] = start + delta*i;
      cout << x[i] << endl;
      newcdf = 0.5 * ( 1.0 + erf((x[i] - cm[0]) /  (pow(2.0, 0.5) * sigma)));
      p[i] = newcdf - cdf;
      cdf = newcdf;
      //cout << nu[0] << "\t"<<nu[1]<< endl;
      //cout << i << " ";
      //FATALIF(!isfinite(p[i]), "NORMAL APPROXIMATION FAILED");
    }
    p[N - 1] = 1.0 - cdf;


    cout << "p0"<< endl;
    double sss = 0.0;
    for(int i = 0; i < N; i++) { cout << p[i] << "\t"; sss+=p[i]; } cout << endl;
    cout << sss << endl;

    // compute moments of the initial approximation 
    /*    bm[0] = cm[0];
    bm[1] = cm[1] + cm[0]*cm[0];
    bm[2] = 3*cm[1]*cm[0] + cm[0]*cm[0]*cm[0];
    bm[3] = 3*cm[1]*cm[1] + 6*cm[1]*cm[0]*cm[0] + cm[0]*cm[0]*cm[0]*cm[0];
    bm[4] = 15*cm[1]*cm[1]*cm[0] + 10*cm[1]*cm[0]*cm[0]*cm[0] + cm[0]*cm[0]*cm[0]*cm[0]*cm[0];
    bm[5] = 15*cm[1]*cm[1]*cm[1] + 45*cm[1]*cm[1]*cm[0]*cm[0] + 15*cm[1]*cm[0]*cm[0]*cm[0]*cm[0] + cm[0]*cm[0]*cm[0]*cm[0]*cm[0]*cm[0];
    */

    bm[0] = 0.0;
    bm[1] = cm[1];
    bm[2] = 0.0;
    bm[3] = 3.0*cm[1]*cm[1];
    bm[4] = 0.0;
    bm[5] = 15.0*cm[1]*cm[1]*cm[1];

  for (int i = 0; i < N - 1; i++) d[i] = p[i];

    double sumP = 0.0;
    for (int i = 0; i < N; i++){
      sumP += p[i];
    }
    
    printf("INITIAL SUM IS %f\n", sumP);


    a[0]=0;

  int fact = 1;
  for (int k = 0; k < O; k++){

    // compute differences
    for (int i = 1; i < N; i++) d2[i] = d[i] - d[i-1]; 
    for (int i = 0; i < N; i++) d[i] = d2[i];
    cout << "k=" << k << endl;
    cout << "d"<< endl;
    for(int i = 0; i < N; i++) { cout << d[i] << "\t"; } cout << endl;

    cout << "a"<< endl;
    for(int i = 0; i < k; i++) { cout << a[i] << "\t"; } cout << endl;
    
    //compute k-th moment of current approximating distribution
    double mu = bm[k];
    cout << "mu=" << mu << endl;
    for (int j = 0; j < k; j++) {
	double s = 0;
	for (int i = j; i < k; i++)
	  s += c[k][j][i] * pow(delta, i) * bm[k-i-1];
	//	s += c[k][j][k] * pow(delta, k);
	cout << "j=" <<  j << "\ts=" << s << "\ta[j]=" << a[j] << "\t";
	mu += a[j] * s;  
	cout << "mu=" << mu << endl;
      }

    // compute $a_k$ 
    fact *= (k+1);
    cout << a[k] << "\t" << mu << "\t" << nu[k] << "\t" << mu-nu[k]<<endl;
    a[k] = (nu[k] - mu) / (double)fact * ((k%2 == 0) ? -1.0 : 1.0);
    cout << a[k] << "~~~" << endl;
    // improve approximation
    double sumP = 0.0;
    for (int i = 0; i < N; i++){
      p[i] += a[k] * d[i];
      //cout << i << "\t" << a[k] << "\t" << d[i] << endl;
      FATALIF(p[i]>1.0, "P is too large");
      FATALIF(p[i]<-1.0, "P is negative");
      //FATALIF(!isfinite(p[i]), "NORMAL APPROXIMATION FAILED");
      //cout << d[i] << endl;
      sumP += p[i];
    }
    
    printf("Total SUM IS %f\n", sumP);
  }
  }

  float Equal(float a) {
    int i = 0;
    double sigma = pow(cm[1], 0.5);
    while (x[i] < a) i++;
    if (i == 0)
      return p[i] / x[0];
    if (i == N - 2)
      return p[i] / (sumDet - x[N - 2]);  
    return p[i] / (2.0 * B * sigma / ((double)N - 2.0));
  }

  float Greater(float a) {
    double sigma = pow(cm[1], 0.5);
    double s = cm[0] - B * sigma;
    double res = 0;
    int i = -1;
    while (s < a) {
      res += p[i];
      s += 2.0 * B * sigma / ((double)N - 2.0);
      i++;
    }
    res += (a - s) * p[i + 1] / sigma;
    return 1.0 - res;
  }

  float GreaterEq(float a) { 
    int i = 0; double r = 0.0;
    double sigma = pow(cm[1], 0.5);
    while (x[i] < a) { r += p[i]; i++;}
    if (i == 0)
      return a * p[i] / x[0];
    if (i == N - 2)
      return r + (a - p[i]) * p[i] / (sumDet - x[i]);  
    return r + p[i] / (2.0 * B * sigma / ((double)N - 2.0));  
  }

  c_dbl ConfidenceInterval(float conf){
  double pp = (1.0 - conf) / 2.0, a = 0.0;
  //cout << "conf="<<conf<<" pp=" << pp<< endl;
  int l = 0, i = 0;
  while (a < pp && i < N){
    a += p[i];
    i++;
  }
  l = i;
  int h = N - 2;
  while (a < 1.0 - pp && i<N){
    a += p[i];
    i++;
  }
  h = i;
  c_dbl cf = {x[l], x[h]};
  return cf;
}

 void GetResult(DOUBLE &a, DOUBLE &b){
   double conf = 0.95;  
   Finalize(); // not automatically called
   c_dbl cf = ConfidenceInterval (conf);
   a = cf.a;
   b = cf.b;
 }

};

#endif // _SUMPROBKOLMOGOROVGLA_H_ 
