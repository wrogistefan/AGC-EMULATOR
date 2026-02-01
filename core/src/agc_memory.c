#include "agc_memory.h"
#include <stdio.h>
#include <string.h>

// AGC memory arrays
static agc_word_t erasable[AGC_RAM_SIZE];
static agc_word_t fixed[AGC_ROM_SIZE];

/*
 * Translate a logical AGC address into a physical memory index.
 * For now, this is a simplified model without bank switching.
 */
static inline uint32_t agc_resolve_address(agc_cpu_t *cpu, agc_word_t address) {
    (void)cpu; // unused for now

    // Erasable memory (RAM)
    if (address < AGC_RAM_SIZE) {
        return address;
    }

    // Fixed memory (ROM)
    uint32_t rom_addr = address - AGC_RAM_SIZE;
    if (rom_addr < AGC_ROM_SIZE) {
        return AGC_RAM_SIZE + rom_addr;
    }

    // Out of range (should not happen)
    return 0;
}

/*
 * Read a word from AGC memory.
 */
agc_word_t agc_memory_read(agc_cpu_t *cpu, agc_word_t address) {
    uint32_t phys = agc_resolve_address(cpu, address);

    if (phys < AGC_RAM_SIZE) {
        return erasable[phys];
    } else {
        return fixed[phys - AGC_RAM_SIZE];
    }
}

/*
 * Write a word to AGC memory.
 * Writes to ROM are ignored (as in real hardware).
 */
void agc_memory_write(agc_cpu_t *cpu, agc_word_t address, agc_word_t value) {
    uint32_t phys = agc_resolve_address(cpu, address);

    if (phys < AGC_RAM_SIZE) {
        erasable[phys] = agc_normalize(value);
    }
}

/*
 * Load a ROM binary into fixed memory.
 * This will be used for Colossus/Luminary rope memory images.
 */
void agc_memory_load_rom(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return;

    fread(fixed, sizeof(agc_word_t), AGC_ROM_SIZE, f);
    fclose(f);
}

