#ifndef AGC_H
#define AGC_H

#include "agc_types.h"

typedef struct {
    agc_word registers[16];
    agc_word *erasable;
    const agc_word *rope;
} agc_state;

void agc_init(agc_state *state);
void agc_tick(agc_state *state);

#endif
