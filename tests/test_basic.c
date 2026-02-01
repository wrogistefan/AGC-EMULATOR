#include <stdio.h>
#include "agc_cpu.h"
#include "agc_memory.h"
#include "agc_instructions.h"

/*
 * Basic test: verify that TC (Transfer Control) works correctly.
 * We load a TC instruction at address 0 and check if Z jumps to the target.
 */

int main(void) {
    agc_cpu_t cpu;

    // Reset CPU and memory
    agc_cpu_reset(&cpu);

    // Load a TC instruction at address 0
    // Opcode 0 (TC) + address 01234 (octal)
    agc_word_t tc_instr = 0x0000 | 01234; // 01234 in octal = 668 decimal
    agc_memory_write(&cpu, 0, tc_instr);

    // Execute one instruction
    agc_cpu_step(&cpu);

    // Check result
    if (cpu.Z == 01234) {
        printf("TEST PASSED: TC instruction correctly set Z to %04o\n", cpu.Z);
        return 0;
    } else {
        printf("TEST FAILED: Expected Z = 01234, got %04o\n", cpu.Z);
        return 1;
    }
}
