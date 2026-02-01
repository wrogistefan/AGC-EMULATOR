#ifndef AGC_INSTRUCTIONS_H
#define AGC_INSTRUCTIONS_H

#include "agc_types.h"
#include "agc_cpu.h"

/*
 * Opcode layout in the AGC (Block II):
 *
 *  15 14 13 | 12 .................. 0
 *  ---------|------------------------
 *   opcode  |     12-bit address
 *
 * The opcode is 3 bits (0–7).
 * The remaining 12 bits represent an address or modifier.
 */

#define AGC_OPCODE_MASK   07000   // 0x7000 - top 3 bits for opcode
#define AGC_ADDRESS_MASK  01777   // 0x0FFF - 12-bit address field

// Extract opcode (top 3 bits)
static inline uint8_t agc_get_opcode(agc_word_t instr) {
    return (instr & AGC_OPCODE_MASK) >> 12;
}

// Extract 12-bit address field
static inline uint16_t agc_get_address(agc_word_t instr) {
    return instr & AGC_ADDRESS_MASK;
}

/*
 * Execute a single AGC instruction.
 * This function is called from agc_cpu_step().
 */
void agc_execute_instruction(agc_cpu_t *cpu, agc_word_t instr);

/*
 * Individual instruction handlers.
 * These will be implemented in agc_instructions.c.
 */

void agc_instr_TC(agc_cpu_t *cpu, uint16_t address);     // Transfer Control
void agc_instr_XCH(agc_cpu_t *cpu, uint16_t address);    // Exchange
void agc_instr_CCS(agc_cpu_t *cpu, uint16_t address);    // Count, Compare, Skip
void agc_instr_INDEX(agc_cpu_t *cpu, uint16_t address);  // Modify next instruction

#endif // AGC_INSTRUCTIONS_H
