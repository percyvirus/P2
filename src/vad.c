#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "vad.h"
#include "pav_analysis.h"

const float FRAME_TIME = 10.0F; /* in ms. */
const int TRAMAS_VOZ_NO_DECIDIDAS = 1;     
const int TRAMAS_SILENCIO_NO_DECIDIDAS = 11;
const int N_init = 10; // Numero de muestras iniciales que cogemos
const int alfa1 = 4; // Margen k1 = k0 + alfa1
const int alfa2 = 10; // Margen k2 = k0 + alfa2

/* 
 * As the output state is only ST_VOICE, ST_SILENCE, or ST_UNDEF,
 * only this labels are needed. You need to add all labels, in case
 * you want to print the internal state in string format
 */

const char *state_str[] = {
  "UNDEF", "S", "V", "INIT"
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
  vad_data->k0 = 0;
  vad_data->k1 = 0;
  vad_data->k2 = 0;
  vad_data->last_sample = 0; //Indica ULTIMA TRAMA amb V o S
  vad_data->nsamples = 0; //Número de TRAMA actual
  vad_data->last_state = ST_INIT; //Indica ÚLTIM ESTAT (V o S)
  //vad_data->time_wait = 90; //Referència del silenci real
  //vad_data->sample_wait = 1440; //time_wait*fm
  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data) {
  /* 
   * TODO: decide what to do with the last undecided frames
   */
  VAD_STATE state = vad_data->state;

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
    /*vad_data->k0 = f.p + vad_data->alfa0;
    vad_data->state = ST_SILENCE;*/
    if(vad_data->nsamples < N_init){
      vad_data->k0+=pow(10,f.p/10);
      //vad_data->k0 = f.p + vad_data->alfa0
      //vad_data->nsamples += 1;
    } else {
      //vad_data->nsamples_silence = 0;
      vad_data->k0 = 10*log10(vad_data->k0/N_init);
      //vad_data->k1 = vad_data->k0 + alfa1;
      //vad_data->k2 = vad_data->k1 + alfa2;
      vad_data->state = ST_SILENCE;
    }
    break;

  case ST_SILENCE:
    if (f.p > vad_data->k0){ //f.p indica la señal de la trama
      vad_data->state = ST_MAYBEVOICE;
      vad_data->last_state = ST_SILENCE;
      vad_data->last_sample = vad_data->nsamples;
      //vad_data->nsamples_voice = 0;
      }
    break;

  case ST_VOICE:
    if (f.p < vad_data->k0){
      vad_data->state = ST_MAYBESILENCE;
      vad_data->last_state = ST_VOICE;
      vad_data->last_sample = vad_data->nsamples;
      //vad_data->nsamples_silence = 0;
    }
    break;

  case ST_MAYBESILENCE:
      /*if(vad_data->nsamples_silence < vad_data->sample_wait){
        if(f.p > vad_data->k2){
          vad_data->state = ST_VOICE;
        }
        else{
          vad_data->nsamples_silence += 1;
        }
      }else{
        if(f.p < vad_data->k1){
          vad_data->state = ST_SILENCE;
        }
        else{
          vad_data->nsamples_silence += 1;
        }
      }*/
      if(f.p > vad_data->k0 + alfa1 + alfa2){ 
        vad_data->state = ST_VOICE;
      }
        else if ((vad_data->nsamples - vad_data->last_sample) == TRAMAS_SILENCIO_NO_DECIDIDAS){
        vad_data->state = ST_SILENCE;
      }
    break;

  case ST_MAYBEVOICE:
    /*if(f.p > vad_data->k2){
      vad_data->state = ST_VOICE;
    }else if(f.p < vad_data->k1){
      vad_data->state = ST_SILENCE;
    }else{
      vad_data->nsamples_voice += 1;
    }*/
    if(f.p < vad_data->k0 + alfa1){
      vad_data->state = ST_SILENCE;
    }
    else if ((vad_data->nsamples - vad_data->last_sample) == TRAMAS_VOZ_NO_DECIDIDAS){
      vad_data->state = ST_VOICE;
    }
    break;

  case ST_UNDEF:
    /*if(vad_data->last_state == ST_SILENCE){
      vad_data->state = ST_SILENCE;
    }else if(vad_data->last_state == ST_VOICE){
      vad_data->state = ST_VOICE;
    }*/
    break;
  }

  vad_data->nsamples += 1;

  if (vad_data->state == ST_SILENCE ||
      vad_data->state == ST_VOICE){
    return vad_data->state;
  }else if (vad_data->state == ST_INIT){
    return ST_SILENCE;
  }
  else{
    return vad_data->last_state;
  }
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}
