#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "vad.h"
#include "pav_analysis.h"

const float FRAME_TIME = 10.0F; /* in ms. */
const int TRAMES_VEU_NO_DECIDIDES = 1;     
const int TRAMES_SILENCI_NO_DECIDIDES = 11;
const int N_init = 10; // Numero de mostras inicials que agafem
const int alfa1 = 4; // Margen k1 = k0 + alfa1
const int alfa2 = 10; // Margen k2 = k1 + alfa2 = k0 + alfa1 + alfa2

/* 
 * As the output state is only ST_VOICE, ST_SILENCE, or ST_UNDEF,
 * only this labels are needed. You need to add all labels, in case
 * you want to print the internal state in string format
 */

const char *state_str[] = {
  "UNDEF", "S", "V", "INIT", "MS", "MV"
};

const char *state2str(VAD_STATE st) {
  return state_str[st];
}

/* Define a datatype with interesting features */
typedef struct {
  float zcr;
  float p;
  float am;
} Features;

/* 
 * TODO: Delete and use your own features!
 */

Features compute_features(const float *x, int N) {
  /*
   * Input: x[i] : i=0 .... N-1 
   * Ouput: computed features
   */
  /* 
   * DELETE and include a call to your own functions
   *
   * For the moment, compute random value between 0 and 1 
   */
  Features feat;
  //feat.zcr = feat.p = feat.am = (float) rand()/RAND_MAX;
  feat.zcr = compute_zcr(x,N,16000);
  feat.p = compute_power(x,N);
  feat.am = compute_am(x,N);
  return feat;
}

/* 
 * TODO: Init the values of vad_data
 */

  VAD_DATA * vad_open(float rate) {
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3;
  vad_data->k0 = 0; //Potencia mitja inicial
  vad_data->k1 = 0; //Umbral de desició 1
  vad_data->k2 = 0; //Umbral de desició 2
  vad_data->last_sample = 0; //Indica si la última trama va ser V o S
  vad_data->nsamples = 0; //Número de trama actual
  vad_data->last_state = ST_INIT; //Indica l'últim estat útil per nosaltres que va estar (V o S)

  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data) {
  /* 
   * TODO: decide what to do with the last undecided frames
   */
  VAD_STATE state;
  if (vad_data->state==ST_MAYBEVOICE) //Decidim ST_VOICE si penúltim estat fos ST_MAYBEVOICE
    state=ST_VOICE;
  else if(vad_data->state==ST_MAYBESILENCE) //Decidim ST_SILENCE si penúltim estat fos ST_MAYBESILENCE
    state=ST_SILENCE;
  else 
    state=vad_data->state;

  free(vad_data);
  return state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;
}

/* 
 * TODO: Implement the Voice Activity Detection 
 * using a Finite State Automata
 */

VAD_STATE vad(VAD_DATA *vad_data, float *x) {

  /* 
   * TODO: You can change this, using your own features,
   * program finite state automaton, define conditions, etc.
   */

  Features f = compute_features(x, vad_data->frame_length);
  vad_data->last_feature = f.p; /* save feature, in case you want to show */

  switch (vad_data->state) {
  case ST_INIT:
    if(vad_data->nsamples < N_init){
      vad_data->k0+=pow(10,f.p/10); //Calculem el sumatori de les N_init primeres mostres
    } else {                        //la funció pow() es de la llibreria math.h
      vad_data->k0 = 10*log10(vad_data->k0/N_init); //Calculem el valor dela potencia mitja inicial de les N_init primeres mostres
      vad_data->k1 = vad_data->k0 + alfa1;  //Calcuelm l'umbral k1 = k0 + alfa1
      vad_data->k2 = vad_data->k1 + alfa2;  //Calcuelm l'umbral k2 = k1 + alfa2
      vad_data->state = ST_SILENCE;
    }
    break;

  case ST_SILENCE:
    if (f.p > vad_data->k0){ //si la trama actual té més potencia que k0
      vad_data->state = ST_MAYBEVOICE;  //Decidim que és ST_VOICE
      vad_data->last_state = ST_SILENCE;  //Penúltim estat passa a aser ST_SILENCE
      vad_data->last_sample = vad_data->nsamples; //Indiquem quina ha estat la última trama
      }
    break;

  case ST_VOICE:
    if (f.p < vad_data->k0){  //si la trama actual té menys potencia que k0
      vad_data->state = ST_MAYBESILENCE;  //Decidim que és ST_MAYBESILENCE
      vad_data->last_state = ST_VOICE;  //Penúltim estat passa a aser ST_VOICE
      vad_data->last_sample = vad_data->nsamples; //Indiquem quina ha estat la última trama
    }
    break;

  case ST_MAYBESILENCE:
    if(f.p > vad_data->k2){   //si la trama actual té més potencia que k2
      vad_data->state = ST_VOICE;  //Decidim que és ST_VOICE
    }
      else if ((vad_data->nsamples - vad_data->last_sample) == TRAMES_SILENCI_NO_DECIDIDES){
      vad_data->state = ST_SILENCE;  //Decidim que és ST_SILENCE si portem més de TRAMES_SILENCI_NO_DECIDIDES
    }                                //en l'estat ST_MAYBESILENCE
    break;

  case ST_MAYBEVOICE:
    if(f.p < vad_data->k1){   //si la trama actual té menys potencia que k1
      vad_data->state = ST_SILENCE;  //Decidim que és ST_SILENCE
    }
    else if ((vad_data->nsamples - vad_data->last_sample) == TRAMES_VEU_NO_DECIDIDES){
      vad_data->state = ST_VOICE; //Decidim que és ST_VOICE si portem més de TRAMES_SILENCI_NO_DECIDIDES
    }                             //en l'estat ST_MAYBEVOICE
    break;

  case ST_UNDEF:
    break;
  }

  vad_data->nsamples += 1;

  if (vad_data->state == ST_SILENCE ||
      vad_data->state == ST_VOICE){
    return vad_data->state; //Retornem l'estat en el que estem
  }else if (vad_data->state == ST_INIT){
    return ST_SILENCE;  //Considerem que els primers segments de la senyal son sempre silenci
  }
  else{
    return vad_data->last_state;  //Si l'estat es ST_UNDEF retornem l'últim estat en el que estavem vàlid (V o S)
  }
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}