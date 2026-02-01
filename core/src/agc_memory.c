#include "agc_memory.h"
#include <stdio.h>
#include <string.h>

// AGC memory arrays
static agc_word_t erasable[AGC_RAM_SIZE];
static agc_word_t fixed[AGC_ROM_SIZE];

/*
 * Read a word from AGC memory.
 * Physical address = bank * 020000 + (addr & 017777)
 */
agc_word_t agc_memory_read(agc_cpu_t *cpu, agc_word_t addr) {
    int phys = cpu->current_bank * 020000 + (addr & 017777);

    if (phys < AGC_RAM_SIZE) {
        return erasable[phys];
    } else {
        return fixed[phys - AGC_RAM_SIZE];
    }
}

/*
 * Write a word to AGC memory.
 * Writes to ROM are ignored (as in real hardware).
 * Physical address = bank * 020000 + (addr & 017777)
 */
void agc_memory_write(agc_cpu_t *cpu, agc_word_t addr, agc_word_t value) {
    int phys = cpu->current_bank * 020000 + (addr & 017777);

    if (phys < AGC_RAM_SIZE) {
        erasable[phys] = value & 017777;
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

