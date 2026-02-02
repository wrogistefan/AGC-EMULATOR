#ifndef AGC_CPU_H
#define AGC_CPU_H

#include "agc_types.h"

/*
 * CPU state of the Apollo Guidance Computer.
 * This structure models the hardware registers exactly as in AGC Block II.
 */

typedef struct {

    // Main registers
    agc_word_t A;   // Accumulator
    agc_word_t L;   // Link register
    agc_word_t Q;   // Overflow / auxiliary
    agc_word_t Z;   // Program counter
    
    // Memory bank registers
    uint8_t EB;     // Erasable bank (RAM)
    uint8_t FB;     // Fixed bank (ROM)
    uint8_t BB;     // Both bank (for special addressing)

    // I/O channels (simplified model)
    agc_word_t IN[16];
    agc_word_t OUT[16];

    // Internal CPU state
    agc_word_t current_instruction;
    uint64_t cycle_count;

} agc_cpu_t;

// Initialize CPU to reset state
void agc_cpu_reset(agc_cpu_t *cpu);

// Execute one instruction (cycle-accurate step)
void agc_cpu_step(agc_cpu_t *cpu);

#endif // AGC_CPU_H
