#include "agc_instructions.h"
#include "agc_memory.h"

/*
 * Main instruction dispatcher.
 * The AGC has only 8 primary opcodes (0–7).
 * Each opcode selects a family of instructions.
 */
void agc_execute_instruction(agc_cpu_t *cpu, agc_word_t instr) {
    uint8_t opcode = agc_get_opcode(instr);
    uint16_t address = agc_get_address(instr);

    switch (opcode) {
        case 0: // 00000 – TC (Transfer Control)
            agc_instr_TC(cpu, address);
            break;

        case 1: // 01000 – XCH (Exchange A with memory)
            agc_instr_XCH(cpu, address);
            break;

        case 2: // 02000 – TS (Transfer to Storage)
            agc_instr_TS(cpu, address);
            break;

        case 3: // 03000 – CA (Clear and Add)
            agc_instr_CA(cpu, address);
            break;

        default:
            // Unimplemented or invalid opcode
            // Real AGC would trigger a restart; we ignore for now.
            break;
    }
}

/*
 * TC – Transfer Control
 * Jump to the given address.
 * This is the AGC equivalent of a branch/jump instruction.
 */
void agc_instr_TC(agc_cpu_t *cpu, uint16_t address) {
    cpu->Z = agc_normalize(address);
}

/*
 * XCH – Exchange
 * Swap the contents of register A with memory[address].
 */
void agc_instr_XCH(agc_cpu_t *cpu, uint16_t address) {
    agc_word_t temp = agc_memory_read(cpu, address);
    agc_memory_write(cpu, address, cpu->A);
    cpu->A = temp;
}

/*
 * TS – Transfer to Storage
 *
 * Store the contents of register A into memory[address].
 * Writes to fixed memory (ROM) are silently ignored,
 * as per real AGC hardware behavior.
 */
void agc_instr_TS(agc_cpu_t *cpu, uint16_t address) {
    agc_memory_write(cpu, address, cpu->A);
}

/*
 * CA – Clear and Add
 *
 * Load the value from memory[address] into register A.
 * This is equivalent to: A = M[addr]
 */
void agc_instr_CA(agc_cpu_t *cpu, uint16_t address) {
    cpu->A = agc_memory_read(cpu, address);
}

