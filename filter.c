#include "filter.h"
float filter_audio(struct filter *fil, float in){
    float x = in - fil->r * fil->ys[3];

    fil->ys[0] = x * fil->p + fil->oldx * fil->p - fil->k * fil-> ys[0];
    fil->ys[1] = fil->ys[0] * fil->p + fil->oldys[0] * fil->p - fil->k * fil-> ys[1];
    fil->ys[2] = fil->ys[1] * fil->p + fil->oldys[1]* fil->p - fil->k * fil-> ys[2];
    fil->ys[3] = fil->ys[2] * fil->p + fil->oldys[2]* fil->p - fil->k * fil-> ys[3];

    fil->ys[3] = fil->ys[3] - (fil->ys[3]*fil->ys[3]*fil->ys[3])/6.0f;

    fil->oldx = x;
    fil->oldys[0] = fil->ys[0];
    fil->oldys[1] = fil->ys[1];
    fil->oldys[2] = fil->ys[2];
    
    return fil->ys[3];
}    

/*//Init
cutoff = cutoff freq in Hz
fs = sampling frequency //(e.g. 44100Hz)
res = resonance [0 - 1] //(minimum - maximum)

f = 2 * cutoff / fs; //[0 - 1]
k = 3.6*f - 1.6*f*f -1; //(Empirical tunning)
p = (k+1)*0.5;
scale = e^((1-p)*1.386249;
r = res*scale;
y4 = output;

y1=y2=y3=y4=oldx=oldy1=oldy2=oldy3=0;

//Loop
//--Inverted feed back for corner peaking
x = input - r*y4;

//Four cascaded onepole filters (bilinear transform)
y1=x*p + oldx*p - k*y1;
y2=y1*p+oldy1*p - k*y2;
y3=y2*p+oldy2*p - k*y3;
y4=y3*p+oldy3*p - k*y4;

//Clipper band limited sigmoid
y4 = y4 - (y4^3)/6;

oldx = x;
oldy1 = y1;
oldy2 = y2;
oldy3 = y3;/
i*/