#include "agc_cpu.h"
#include "agc_memory.h"
#include "agc_instructions.h"

#include <string.h> // memset

/*
 * Reset the AGC CPU to its initial state.
 * This models the hardware reset condition of the Block II AGC.
 */
void agc_cpu_reset(agc_cpu_t *cpu) {
    if (!cpu) return;

    // Clear main registers
    cpu->A = 0;
    cpu->L = 0;
    cpu->Q = 0;

    // Program counter (Z) starts at 0 after reset.
    // Some simulators start at 02000 (start of fixed memory),
    // but we keep it at 0 until ROM loading is implemented.
    cpu->Z = 0;
    
    // Memory bank registers - start in bank 0
    cpu->EB = 0;
    cpu->FB = 0;
    cpu->BB = 0;

    // Clear I/O channels
    memset(cpu->IN, 0, sizeof(cpu->IN));
    memset(cpu->OUT, 0, sizeof(cpu->OUT));

    // Internal CPU state
    cpu->current_instruction = 0;
    cpu->cycle_count = 0;
}

/*
 * Execute a single AGC instruction.
 * This is the core of the emulator: fetch → decode → execute.
 *
 * The AGC is not pipelined. Each instruction is executed sequentially.
 * Timing will be added later for cycle-accurate behavior.
 */
void agc_cpu_step(agc_cpu_t *cpu) {
    if (!cpu) return;

    // Fetch instruction from memory at address Z
    agc_word_t instr = agc_instruction_fetch(cpu, cpu->Z);

    cpu->current_instruction = instr;

    // Increment program counter (AGC increments Z before execution)
    cpu->Z = agc_normalize(cpu->Z + 1);

    // Decode and execute the instruction
    agc_execute_instruction(cpu, instr);

    // Increase cycle counter (placeholder for real timing)
    cpu->cycle_count++;
}
