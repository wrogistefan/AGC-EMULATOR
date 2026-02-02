#include "agc_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// AGC memory arrays
static agc_word_t erasable[AGC_RAM_SIZE];
static agc_word_t fixed[AGC_ROM_SIZE];

// Bank size constants
#define AGC_ERASE_BANK_SIZE  02000   // 1024 words (1K) per erasable bank
#define AGC_FIXED_BANK_SIZE  010000  // 4096 words (4K) per fixed bank

/*
 * Read a word from AGC memory.
 * Routes through EB/FB bank registers for proper bank switching:
 *   - Erasable addresses (0-01777): use EB bank register
 *   - Fixed addresses (02000+): use FB bank register
 */
agc_word_t agc_memory_read(agc_cpu_t *cpu, agc_word_t addr) {
    // Normalize address to 15 bits
    addr = addr & 077777;

    if (addr < AGC_ERASE_BANK_SIZE) {
        // Erasable memory - banked via EB
        // Clamp EB to valid range (0 to AGC_RAM_SIZE/AGC_ERASE_BANK_SIZE - 1)
        uint8_t eb = cpu->EB % (AGC_RAM_SIZE / AGC_ERASE_BANK_SIZE);
        int phys = eb * AGC_ERASE_BANK_SIZE + addr;
        return erasable[phys];
    } else {
        // Fixed memory - banked via FB
        // Clamp FB to valid range (0 to AGC_ROM_SIZE/AGC_FIXED_BANK_SIZE - 1)
        uint8_t fb = cpu->FB % (AGC_ROM_SIZE / AGC_FIXED_BANK_SIZE);
        int phys = fb * AGC_FIXED_BANK_SIZE + (addr - AGC_ERASE_BANK_SIZE);
        // Ensure physical address is within bounds
        if (phys < 0) phys = 0;
        if (phys >= AGC_ROM_SIZE) phys = AGC_ROM_SIZE - 1;
        return fixed[phys];
    }
}

/*
 * Write a word to AGC memory.
 * Writes to ROM are ignored (as in real hardware).
 * Routes through EB/FB bank registers for proper bank switching.
 */
void agc_memory_write(agc_cpu_t *cpu, agc_word_t addr, agc_word_t value) {
    // Normalize address to 15 bits
    addr = addr & 077777;

    if (addr < AGC_ERASE_BANK_SIZE) {
        // Erasable memory - banked via EB
        // Clamp EB to valid range (0 to AGC_RAM_SIZE/AGC_ERASE_BANK_SIZE - 1)
        uint8_t eb = cpu->EB % (AGC_RAM_SIZE / AGC_ERASE_BANK_SIZE);
        int phys = eb * AGC_ERASE_BANK_SIZE + addr;
        erasable[phys] = agc_normalize(value);
    }
    // Writes to fixed memory (ROM) are ignored
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

/*
 * Load a ROM binary into fixed memory.
 * Reads 2 bytes per word (big-endian) and masks to 15 bits.
 */
bool agc_load_rom(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return false;

    for (int i = 0; i < AGC_ROM_SIZE; i++) {
        uint8_t hi, lo;
        if (fread(&hi, 1, 1, f) != 1) break;
        if (fread(&lo, 1, 1, f) != 1) break;

        uint16_t word = ((hi << 8) | lo) & 077777; // 15 bits
        fixed[i] = word;
    }

    fclose(f);
    return true;
}

/*
 * Erasable memory helpers for testing.
 * Direct access to erasable memory without going through bank registers.
 */
void agc_erasable_set(uint8_t bank, uint16_t addr, agc_word_t value) {
    uint8_t eb = bank % (AGC_RAM_SIZE / AGC_ERASE_BANK_SIZE);
    int phys = eb * AGC_ERASE_BANK_SIZE + (addr & 0777);
    erasable[phys] = agc_normalize(value);
}

agc_word_t agc_erasable_get(uint8_t bank, uint16_t addr) {
    uint8_t eb = bank % (AGC_RAM_SIZE / AGC_ERASE_BANK_SIZE);
    int phys = eb * AGC_ERASE_BANK_SIZE + (addr & 0777);
    return erasable[phys];
}

/*
 * ROM/Fixed memory helpers for testing.
 * Direct access to fixed memory without going through bank registers.
 */
void agc_rom_set(uint32_t addr, agc_word_t value) {
    if (addr < AGC_ROM_SIZE) {
        fixed[addr] = agc_normalize(value);
    }
}

agc_word_t agc_rom_get(uint32_t addr) {
    if (addr < AGC_ROM_SIZE) {
        return fixed[addr];
    }
    return 0;
}

