#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "vad.h"
#include "pav_analysis.h"

const float FRAME_TIME = 10.0F; /* in ms. */
const int N_init = 10; // Numero de muestras iniciales que cogemos
const int alfa0 = 6; // Margen k0 = k0 + alfa1
const int alfa1 = 12; // Margen k1 = k0 + alfa1

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

  VAD_DATA * vad_open(float rate, float alfa0) {
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->alfa0 = alfa0;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3;
  vad_data->k0;
  vad_data->k1;
  vad_data->k2;
  vad_data->nsamples_silence = 0; // # Mostres consecutives de silenci
  vad_data->nsamples_voice = 0; // # Mostres consecutives de veu
  //vad_data->time_wait = 90; // Referència del silenci real
  vad_data->sample_wait = 1440; //time_wait*fm
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
    if(vad_data->nsamples_silence < N_init){
      vad_data->k0+=pow(10,f.p/10);
      //vad_data->k0 = f.p + vad_data->alfa0
      vad_data->nsamples_silence ++;
    } else {
      vad_data->nsamples_silence = 0;
      vad_data->k0 = 10*log10(vad_data->k0/N_init);
      vad_data->k1 = vad_data->k0 + alfa0;
      vad_data->k2 = vad_data->k0 + alfa1;
      vad_data->state = ST_SILENCE;
    }
    break;

  case ST_SILENCE:
    if (f.p > vad_data->k0) //f.p indica la señal de la trama
      vad_data->state = ST_VOICE;
        vad_data->nsamples_voice = 0;
    break;

  case ST_VOICE:
    if (f.p < vad_data->k0){
      vad_data->state = ST_SILENCE;
      vad_data->nsamples_silence = 0;
    }
    break;

  case ST_MAYBESILENCE:
      if(vad_data->nsamples_silence < vad_data->sample_wait){
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
      }
    break;

  case ST_MAYBEVOICE:
    if(f.p > vad_data->k2){
      vad_data->state = ST_VOICE;
    }else if(f.p < vad_data->k1){
      vad_data->state = ST_SILENCE;
    }else{
      vad_data->nsamples_voice += 1;
    }
    break;

  case ST_UNDEF:
    break;
  }

  if (vad_data->state == ST_SILENCE ||
      vad_data->state == ST_VOICE)
    return vad_data->state;
  else
    return ST_UNDEF;
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}
