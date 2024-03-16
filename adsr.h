
#ifndef ADSR_H
#define ADSR_H
#include "pico/stdlib.h"

enum adsr_phase { a,
                  d,
                  s,
                  r };

#ifndef SAMPLE_RATE
#define SAMPLE_RATE 9000.0f
#endif

struct adsr {
    float attack_add;
    float decay_add;
    float sustain_val;
    float sustain_end;
    float release_add;
    float pos;
    float tick;
    float value;
    float multiplier;
    enum adsr_phase phase;
    bool active;
    bool playing;
    float out;
};

bool tick_adsr(struct adsr* adsr);

static inline void construct_adsr(struct adsr* new, float attack, float decay, float sustain, float release, float multiplier, float duration) {
    new->tick = 1.0f / SAMPLE_RATE;
    new->attack_add = (1.0f / attack) * new->tick;
    new->decay_add = (-1.0f + sustain) * (1.0f / decay) * new->tick;
    new->release_add = (-1.0f * sustain) * (1.0f / release) * new->tick;
    new->sustain_val = sustain;
    new->sustain_end = duration;
    new->multiplier = multiplier;
    new->active = false;
    new->playing = false;
    new->phase = a;
    new->pos = 0;
    new->value = 0;
}

static inline void restart_envelope(struct adsr* env) {
    env->phase = a;
    env->pos = 0;
    env->value = 0;
    env->active = true;
    env->playing = true;
}

#endif