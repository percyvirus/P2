#include <stdio.h>
#include <math.h>
#include "pav_analysis.h"

//Calculo de la potencia media en dB
float compute_power(const float *x, unsigned int N) {
  float sum = 0;
    for(int i=0; i<N;i++){
        sum = sum + (x[i]*x[i]);
    }
  float p = 10*log10(sum/N);
  //printf("Pot: %f\n",p);

    return p;
}

float compute_am(const float *x, unsigned int N) {
  float sum = 0;
    for(int i = 0; i < N; i ++){
        sum = sum + fabs(x[i]);
    }
  
  float p = sum/N;
  //printf("Ampitude: %f\n", p);
  
  return p;
}

int signo(float x){
    if(x>=0){
        return 1;
    } else {
        return 0;
    }
}

float compute_zcr(const float *x, unsigned int N, float fm){
  float sum = 0;
    for(int i = 1; i < N ; i++){
        if(signo(x[i-1])!=signo(x[i])){
            sum += 1;
        }
    }
  
  float p = (fm/2)*(sum/(N-1));
  printf("Los cruces por 0: %f\n", p);
  
  return p;
}
 