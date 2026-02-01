#include <stdio.h>
#include "agc_cpu.h"
#include "agc_memory.h"
#include "agc_instructions.h"

int test_tc(void);
int test_ca(void);
int test_ts(void);
int test_xch(void);

int main(void) {
    int failed = 0;

    failed |= test_tc();
    failed |= test_ca();
    failed |= test_ts();
    failed |= test_xch();

    if (failed) {
        printf("SOME TESTS FAILED\n");
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}

int test_tc(void) {
    agc_cpu_t cpu;
    agc_cpu_reset(&cpu);

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
        printf("TEST FAILED: TC - Expected Z = 01234, got %04o\n", cpu.Z);
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

int test_ca(void) {
    agc_cpu_t cpu;
    agc_cpu_reset(&cpu);

    // Set up memory with a known value
    agc_word_t test_value = 05555;
    agc_memory_write(&cpu, 0100, test_value);

    // Set A to something else
    cpu.A = 01234;

    // Load a CA instruction at address 0
    agc_word_t ca_instr = 030100; // CA 0100
    agc_memory_write(&cpu, 0, ca_instr);

    // Execute one instruction
    agc_cpu_step(&cpu);

    // Check A was loaded with memory value
    if (cpu.A != test_value) {
        printf("TEST FAILED: CA - Expected A = %04o, got %04o\n", test_value, cpu.A);
        return 1;
    }

    printf("TEST PASSED: CA loaded value from memory into A\n");
    return 0;
}

int test_ts(void) {
    agc_cpu_t cpu;
    agc_cpu_reset(&cpu);

    // Set A to a known value
    cpu.A = 07777;

    // Load a TS instruction at address 0
    agc_word_t ts_instr = 020200; // TS 0200
    agc_memory_write(&cpu, 0, ts_instr);

    // Execute one instruction
    agc_cpu_step(&cpu);

    // Check memory was updated with A value
    agc_word_t mem_value = agc_memory_read(&cpu, 0200);
    if (mem_value != cpu.A || mem_value != 07777) {
        printf("TEST FAILED: TS - Expected memory[0200] = 07777, got %04o\n", mem_value);
        return 1;
    }

    printf("TEST PASSED: TS stored A into memory\n");
    return 0;
}

int test_xch(void) {
    agc_cpu_t cpu;
    agc_cpu_reset(&cpu);

    // Set up memory with a known value
    agc_word_t mem_value = 03333;
    agc_memory_write(&cpu, 0150, mem_value);

    // Set A to a different value
    cpu.A = 06666;

    // Load an XCH instruction at address 0
    agc_word_t xch_instr = 010150; // XCH 0150
    agc_memory_write(&cpu, 0, xch_instr);

    // Execute one instruction
    agc_cpu_step(&cpu);

    // Check A got the memory value
    if (cpu.A != mem_value) {
        printf("TEST FAILED: XCH - Expected A = %04o, got %04o\n", mem_value, cpu.A);
        return 1;
    }

    // Check memory got A's original value
    agc_word_t new_mem_value = agc_memory_read(&cpu, 0150);
    if (new_mem_value != 06666) {
        printf("TEST FAILED: XCH - Expected memory[0150] = 06666, got %04o\n", new_mem_value);
        return 1;
    }

    printf("TEST PASSED: XCH swapped A with memory\n");
    return 0;
}
