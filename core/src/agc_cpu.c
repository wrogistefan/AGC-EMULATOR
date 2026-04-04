#include "agc_cpu.h"
#include "agc_memory.h"
#include "agc_instructions.h"

#include <string.h>

void agc_cpu_reset(agc_cpu_t *cpu) {
    if (!cpu) return;

    cpu->A = 0;
    cpu->L = 0;
    cpu->Q = 0;
    cpu->Z = 0;
    cpu->EB = 0;
    cpu->FB = 0;

    memset(cpu->IN, 0, sizeof(cpu->IN));
    memset(cpu->OUT, 0, sizeof(cpu->OUT));

    cpu->current_instruction = 0;
    cpu->cycle_count = 0;
}

void agc_cpu_step(agc_cpu_t *cpu) {
    if (!cpu) return;

    agc_word_t instr = agc_instruction_fetch(cpu, cpu->Z);

    cpu->current_instruction = instr;
    cpu->Z = agc_normalize(cpu->Z + 1);

    agc_execute_instruction(cpu, instr);

    cpu->cycle_count++;
}
