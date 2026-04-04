#ifndef AGC_CPU_H
#define AGC_CPU_H

#include "agc_types.h"

typedef struct {
    agc_word_t A;   /* Accumulator */
    agc_word_t L;   /* Link register */
    agc_word_t Q;   /* Overflow / auxiliary */
    agc_word_t Z;   /* Program counter */
    
    agc_word_t EB;    /* Erasable bank register (full 15-bit) */
    agc_word_t FB;    /* Fixed bank register (full 15-bit) */

    agc_word_t IN[16];
    agc_word_t OUT[16];

    agc_word_t current_instruction;
    uint64_t cycle_count;

} agc_cpu_t;

void agc_cpu_reset(agc_cpu_t *cpu);
void agc_cpu_step(agc_cpu_t *cpu);

#endif /* AGC_CPU_H */
