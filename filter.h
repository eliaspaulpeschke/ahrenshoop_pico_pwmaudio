#ifndef FILTER_H
#define FILTER_H
#include "pico/stdlib.h"
#include "math.h"

#ifndef SAMPLE_RATE
#define SAMPLE_RATE 9000.0f
#endif

#define FILTER_ORDER 4

struct filter {
    float ys[FILTER_ORDER];
    float oldys[FILTER_ORDER-1];
    float oldx;
    float k;
    float p;
    float r;
};

static inline void set_filter(struct filter *fil, float cutoff, float reso, bool init){
    float f = 2 * cutoff / SAMPLE_RATE;
    fil->k = 3.6f * f - 1.6f*f*f - 1.0f;
    fil->p = (fil->k + 1.0f)*0.5f;
    float scale = expf(1.0f - fil->p) * 1.386249;
    fil->r = reso * scale;
    if (init){
      fil->ys[0] = 0;
      fil->ys[1] = 0;
      fil->ys[2] = 0;
      fil->ys[3] = 0;
      fil->oldys[0] = 0;
      fil->oldys[1] = 0;
      fil->oldys[2] = 0;
      fil->oldx = 0;
    }
}

float filter_audio(struct filter *fil, float in);
#endif