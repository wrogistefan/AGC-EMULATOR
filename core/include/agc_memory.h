#ifndef AGC_MEMORY_H
#define AGC_MEMORY_H

#include "agc_types.h"
#include "agc_cpu.h"

/*
 * Apollo Guidance Computer (AGC Block II) Memory Model
 * 
 * This implements the exact hardware memory architecture:
 * - 2 erasable (RAM) banks of 1024 words each (E0, E1)
 * - 36 fixed (ROM) banks of 1024 words each (F0-F35)
 * - CPU registers mapped at addresses 00000-00017
 * 
 * Address decoding follows the real AGC central logic exactly.
 */

/* ============================================================================
 * Physical Memory Arrays
 * 
 * Erasable memory: 2 banks × 1024 words = 2048 words total
 * Fixed memory: 36 banks × 1024 words = 36864 words total
 * ============================================================================ */

/* Erasable (RAM) memory - E0 and E1 banks */
extern uint16_t erasable[2][1024];

/* Fixed (ROM) memory - F0 through F35 banks */
extern uint16_t fixed[36][1024];

/* ============================================================================
 * CPU Register Access
 * 
 * Registers are at addresses 00000-00017. These are hardware registers,
 * NOT stored in erasable memory. The mapping is:
 *   00000: A  (Accumulator)
 *   00001: L  (Link register)
 *   00002: Q  (Overflow/auxiliary)
 *   00003: EB (Erasable bank register)
 *   00004: FB (Fixed bank register)
 *   00005: Z  (Program counter / instruction address)
 *   00006-00017: Other special registers (I/O, internal state)
 * ============================================================================ */

/* Read from a CPU register at address 00000-00017 */
agc_word_t read_register(agc_cpu_t *cpu, uint16_t addr);

/* Write to a CPU register at address 00000-00017 */
void write_register(agc_cpu_t *cpu, uint16_t addr, agc_word_t value);

/* ============================================================================
 * Memory Access API
 * 
 * These are the ONLY entry points for memory access in the emulator.
 * All memory operations go through these functions.
 * 
 * Address ranges:
 *   00000-00017: CPU registers (16 words) - READ ONLY
 *   00020-01777: Erasable memory (banked via EB)
 *   02000-03777: Fixed-fixed ROM (banks 0-1, unbanked)
 *   04000-07777: Fixed ROM (banks 2-35, banked via FB)
 * 
 * ROM is write-protected. All writes to fixed memory are ignored.
 * Writes to register addresses (00000-00017) are ignored.
 * ============================================================================ */

/* Read a word from AGC memory */
agc_word_t agc_read(agc_cpu_t *cpu, uint16_t addr);

/* Write a word to AGC memory */
void agc_write(agc_cpu_t *cpu, uint16_t addr, agc_word_t value);

/* ============================================================================
 * Instruction Fetch
 * 
 * Instruction fetches are separate from general memory reads.
 * Instructions are always fetched from erasable memory (00000-01777),
 * even at addresses that would map to registers for data reads.
 * 
 * This models the real AGC behavior where the instruction fetch
 * unit accesses the erasable memory array directly.
 * ============================================================================ */

/* Fetch instruction from memory (always from erasable for 00000-01777) */
agc_word_t agc_instruction_fetch(agc_cpu_t *cpu, uint16_t addr);

/* ============================================================================
 * ROM Loading Functions
 * 
 * Used for loading Colossus/Luminary rope memory images.
 * ============================================================================ */

/* Load ROM from binary file (2 bytes per word, big-endian) */
bool agc_load_rom(const char *filename);

#endif // AGC_MEMORY_H
