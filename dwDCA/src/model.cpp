/* model.cpp Created by Layne Frechette on Feb. 17, 2021
 *
 * Implements DCA model class.
 */

#include "model.h"
#include <stdlib.h>
#include <vector>


model::model(){
  lambda = 0.01;
}

model::model(int N1, int q1, int nwell1, double lam, double Tm, std::string mc_init1){
  N = N1;
  q = q1;
  nwell = nwell1;
  lambda = lam;
  Tmix = Tm;
  avg_ene = 0;
  num_seqs = 100;
  seqs.resize(num_seqs);
  h1.zeros(N,q);
  J1.zeros(q,q,N*(N-1)/2);
  mom1.zeros(N,q);
  mom2.zeros(q,q,N*(N-1)/2);
  if(nwell==2){
    h2.zeros(N,q);
    J2.zeros(q,q,N*(N-1)/2);
    mom1_ene1.zeros(N,q);
    mom1_ene2.zeros(N,q);
    mom2_ene1.zeros(q,q,N*(N-1)/2);
    mom2_ene2.zeros(q,q,N*(N-1)/2);
  }
  mom1_err=0;
  mom2_err=0;
  mc_init = mc_init1;
}

model::~model(){
}


double model::get_energy(std::vector<int> &seq){

  double energy = 0;

  for(int i=0; i<N; i++){
    energy -= h1(i,seq[i]);
  }
  for(int i=0; i<(N-1); i++){
    for(int j=(i+1); j<N; j++){
      int index = (N-1)*i-i*(i+1)/2+j-1;
      //do we need to symmetrize?
      //energy -= J1(seq[i], seq[j], index);
      energy -= (J1(seq[i], seq[j], index) + J1(seq[j], seq[i], index))/2; //symmetrize the couplings for computing energy
    }
  }

  if(nwell==2){
    double e1 = energy;
    energy = exp(-e1/Tmix);
    double e2 = 0;
    for(int i=0; i<N; i++){
      e2 -= h2(i,seq[i]);
    }
    for(int i=0; i<(N-1); i++){
      for(int j=(i+1); j<N; j++){
        int index = (N-1)*i-i*(i+1)/2+j-1;
        //e2 -= J2(seq[i], seq[j], index);
        e2 -= (J2(seq[i], seq[j], index) + J2(seq[j], seq[i], index))/2;
      }
    } 
    energy += exp(-e2/Tmix);
    energy = -Tmix*log(energy/2);
  }

  return energy;
}

double model::get_delta_energy_single(std::vector<int> &seq, int well, int m, int r){

  double dE = 0;

  if(well==1) {
	  dE -= h1(m,r) - h1(m,seq[m]);
	  for(int i=0; i<m; i++){
		  int index = (N-1)*i-i*(i+1)/2+m-1;
		  dE -= (J1(seq[i], r, index) + J1(r, seq[i], index))/2;
		  dE += (J1(seq[i], seq[m], index) + J1(seq[m], seq[i], index))/2;
	  } 
	  for(int j=(m+1); j<N; j++){
		  int index = (N-1)*m-m*(m+1)/2+j-1;
		  dE -= (J1(r, seq[j], index) + J1(seq[j], r, index))/2;
		  dE += (J1(seq[m], seq[j], index) + J1(seq[j], seq[m], index))/2;
	  }
  } else if(well==2) {
	  dE -= h2(m,r) - h2(m,seq[m]);
	  for(int i=0; i<m; i++){
		  int index = (N-1)*i-i*(i+1)/2+m-1;
		  dE -= (J2(seq[i], r, index) + J2(r, seq[i], index))/2;
		  dE += (J2(seq[i], seq[m], index) + J2(seq[m], seq[i], index))/2;
	  }
	  for(int j=(m+1); j<N; j++){
		  int index = (N-1)*m-m*(m+1)/2+j-1;
		  dE -= (J2(r, seq[j], index) + J2(seq[j], r, index))/2;
		  dE += (J2(seq[m], seq[j], index) + J2(seq[j], seq[m], index))/2;
	  }
  } else {
	  std::cout << "index: " << well << std::endl;
	  printf("Not a valid well index. Exiting\n");
	  exit(-1);
  }

  return dE;
}

double model::get_energy_single(std::vector<int> &seq, int well){

  double energy = 0;

  for(int i=0; i<N; i++){
    if(well==1) energy -= h1(i,seq[i]);
    else if(well==2) energy -= h2(i,seq[i]);
    else{
      std::cout << "index: " << well << std::endl;
      printf("Not a valid well index. Exiting\n");
      exit(-1);
    }
  }
  for(int i=0; i<(N-1); i++){
    for(int j=(i+1); j<N; j++){
      int index = (N-1)*i-i*(i+1)/2+j-1;
      if(well==1) energy -= (J1(seq[i], seq[j], index) + J1(seq[j], seq[i], index))/2;
      else if(well==2) energy -= (J2(seq[i], seq[j], index) + J2(seq[j], seq[i], index))/2;
      else{
        std::cout << "index: " << well << std::endl;
        printf("Not a valid well index. Exiting\n");
        exit(-1);
      }
    }
  }
  return energy;
}

double model::get_Z(){

  //Uses code from https://www.geeksforgeeks.org/print-all-sequences-of-given-length/

  double Z=0;
  std::vector<int> seq(N);

  while(true){
    double ene = get_energy(seq); 
    Z += exp(-ene);
    for(int i=0; i<N; i++){
      printf("%d ", seq[i]);
    }
    printf("\n");
    int p=N-1;
    while(seq[p]==q-1) p--;
    if(p<0) break;
    seq[p]=seq[p]+1;
    for(int i=p+1; i<N; i++) seq[i]=0;
  }
  
  return Z;
}

void model::convert_to_zero_sum(){
  
  for(int i=0; i<N; i++){
    double sum=0;
    for(int a=0; a<q; a++) sum += h1(i,a);
    for(int a=0; a<q; a++) h1(i,a) -= sum/q;
    if(nwell==2){
      sum=0;
      for(int a=0; a<q; a++) sum += h2(i,a);
      for(int a=0; a<q; a++) h2(i,a) -= sum/q;
    }
  }
  for(int i=0; i<N-1; i++){
    for(int j=i+1; j<N; j++){
      int index = (N-1)*i-i*(i+1)/2+j-1;
      double dsum1=0;
      double dsum2=0;
      std::vector<double> sum1_a(q);
      std::vector<double> sum1_b(q);
      std::vector<double> sum2_a(q);
      std::vector<double> sum2_b(q);
      for(int a=0; a<q; a++){
        sum1_a[a]=0;
        sum1_b[a]=0;
        sum2_a[a]=0;
        sum2_b[a]=0;
      }
      for(int b=0; b<q; b++){
        for(int a=0; a<q; a++){
          sum1_b[b] += J1(a,b,index)/q;
          dsum1 += J1(a,b,index)/(q*q);
          sum1_a[a] += J1(a,b,index)/q;
          if(nwell==2){
            sum2_b[b] += J2(a,b,index)/q;
            dsum2 += J2(a,b,index)/(q*q);
            sum2_a[a] += J2(a,b,index)/q;
          }
        }
      }
      for(int a=0; a<q; a++){
        for(int b=0; b<q; b++){
          J1(a,b,index) = J1(a,b,index) - sum1_a[a] - sum1_b[b] + dsum1;
          if(nwell==2) J2(a,b,index) = J2(a,b,index) - sum2_a[a] - sum2_b[b] + dsum2;
        }
      }
    }
  }
}
