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
        case 0: // TC – Transfer Control
            agc_instr_TC(cpu, address);
            break;

        case 1: // CCS – Count, Compare, Skip
            agc_instr_CCS(cpu, address);
            break;

        case 2: // INDEX – Modify next instruction
            agc_instr_INDEX(cpu, address);
            break;

        case 3: // XCH – Exchange
            agc_instr_XCH(cpu, address);
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
 * CCS – Count, Compare, Skip
 *
 * This is one of the most unusual AGC instructions.
 * It loads the value from memory into A, negates it,
 * and then performs a conditional skip based on the sign.
 *
 * Behavior summary:
 *   1. A = -memory[address]
 *   2. If A > 0: skip next instruction
 *   3. If A == +0: skip next 2 instructions
 *   4. If A < 0: skip next 3 instructions
 */
void agc_instr_CCS(agc_cpu_t *cpu, uint16_t address) {
    agc_word_t value = agc_memory_read(cpu, address);

    // 1's complement negation
    cpu->A = agc_negate(value);

    // Determine skip count
    if (!agc_is_negative(cpu->A) && cpu->A != 0) {
        cpu->Z = agc_normalize(cpu->Z + 1); // skip 1
    } else if (cpu->A == 0) {
        cpu->Z = agc_normalize(cpu->Z + 2); // skip 2
    } else {
        cpu->Z = agc_normalize(cpu->Z + 3); // skip 3
    }
}

/*
 * INDEX – Modify the next instruction
 *
 * The AGC modifies the address field of the *next* instruction
 * by adding the value stored at memory[address].
 *
 * This is how the AGC implements indirect addressing.
 */
void agc_instr_INDEX(agc_cpu_t *cpu, uint16_t address) {
    agc_word_t offset = agc_memory_read(cpu, address);

    // Read next instruction
    agc_word_t next_instr = agc_memory_read(cpu, cpu->Z);

    // Modify address field using proper 1's complement arithmetic
    // This correctly handles negative offsets
    agc_word_t new_addr = agc_add(agc_get_address(next_instr), offset) & AGC_ADDRESS_MASK;

    // Reconstruct instruction
    agc_word_t modified =
        (next_instr & AGC_OPCODE_MASK) | new_addr;

    // Write back modified instruction
    agc_memory_write(cpu, cpu->Z, modified);
}

