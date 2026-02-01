#include <stdio.h>
#include "agc_cpu.h"
#include "agc_memory.h"
#include "agc_instructions.h"

int main(void) {
    agc_cpu_t cpu;

    // Reset CPU and memory
    agc_cpu_reset(&cpu);

    // Capture initial state
    agc_word_t initial_A = cpu.A;
    agc_word_t initial_L = cpu.L;
    agc_word_t initial_Q = cpu.Q;
    agc_word_t initial_Z = cpu.Z;

    // Load a TC instruction at address 0
    agc_word_t tc_instr = 01234; // TC 01234
    agc_memory_write(&cpu, 0, tc_instr);

    // Execute one instruction
    agc_cpu_step(&cpu);

    // Check Z changed correctly
    if (cpu.Z != 01234) {
        printf("TEST FAILED: Expected Z = 01234, got %04o\n", cpu.Z);
        return 1;
    }

    // Check other registers unchanged
    if (cpu.A != initial_A ||
        cpu.L != initial_L ||
        cpu.Q != initial_Q) {
        printf("TEST FAILED: TC modified registers other than Z\n");
        return 1;
    }

    printf("TEST PASSED: TC updated only Z as expected\n");
    return 0;
}
