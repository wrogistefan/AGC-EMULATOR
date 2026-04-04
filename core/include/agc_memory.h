#ifndef AGC_MEMORY_H
#define AGC_MEMORY_H

#include "agc_types.h"
#include "agc_cpu.h"

/*
 * AGC Block II Memory Model
 *
 * Erasable (RAM):
 *   - 8 banks (E0-E7) x 256 words = 2048 words total
 *   - Unswitched region (00000-01377): always bank E0
 *   - Switched region (01400-01777): bank = (EB >> 8) & 07
 *
 * Fixed (ROM):
 *   - 36 banks (F0-F35) x 1024 words = 36864 words total
 *   - Fixed-fixed (02000-03777): always bank 0
 *   - Fixed-switchable (04000-07777): bank = (FB >> 10) & 037
 *
 * Registers:
 *   - EB: full 15-bit, bank = (EB >> 8) & 07
 *   - FB: full 15-bit, bank = (FB >> 10) & 037
 *   - Register addresses (00000-00017) are memory-mapped, writable via agc_write
 */

/* ============================================================================
 * Public Memory API
 * ============================================================================ */

agc_word_t agc_read(agc_cpu_t *cpu, uint16_t addr);
void agc_write(agc_cpu_t *cpu, uint16_t addr, agc_word_t value);
agc_word_t agc_instruction_fetch(agc_cpu_t *cpu, uint16_t addr);
bool agc_load_rom(const char *filename);

#endif // AGC_MEMORY_H
