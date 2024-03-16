#include "adsr.h"

bool tick_adsr(struct adsr *adsr) {
    if (adsr->active) {
        switch (adsr->phase) {
            case a:
                if (adsr->value < 1.0f) {
                    adsr->value += adsr->attack_add;
                    adsr->pos += adsr->tick;
                    adsr->out = adsr->value * adsr->multiplier;
                    return true;
                } else {
                    adsr->phase = d;
                }
            case d:
                if (adsr->value > adsr->sustain_val) {
                    adsr->value += adsr->decay_add;
                    adsr->pos += adsr->tick;
                    adsr->out = adsr->value * adsr->multiplier;
                    return true;
                } else {
                    adsr->phase = s;
                }
            case s:
                if (adsr->pos < adsr->sustain_end) {
                    adsr->pos += adsr->tick;
                    return true;
                } else if (adsr->sustain_end == 0.0f) {
                    adsr->playing = false;
                    return true;
                } else {
                    adsr->phase = r;
                }
            case r:
                if (adsr->value > (-1.0f * adsr->release_add)) {
                    adsr->value += adsr->release_add;
                    adsr->pos += adsr->tick;
                    adsr->out = adsr->value * adsr->multiplier;
                    return true;
                } else {
                    adsr->phase = a;
                    adsr->pos = 0.0f;
                    adsr->value = 0.0f;
                    adsr->out = 0.0f;
                    adsr->active = false;
                    adsr->playing = false;
                    return false;
                }
        }
    } else {
        return false;
    }
};