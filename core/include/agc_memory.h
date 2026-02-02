#ifndef AGC_MEMORY_H
#define AGC_MEMORY_H

#include "agc_types.h"
#include "agc_cpu.h"

// Sizes of AGC memory regions
#define AGC_RAM_SIZE 2048      // 2K words of erasable memory
#define AGC_ROM_SIZE 36864     // 36K words of fixed memory

// Memory access API
agc_word_t agc_memory_read(agc_cpu_t *cpu, agc_word_t address);
void agc_memory_write(agc_cpu_t *cpu, agc_word_t address, agc_word_t value);

// ROM loading (for Colossus/Luminary binaries)
void agc_memory_load_rom(const char *path);

bool agc_load_rom(const char *filename);

// Erasable memory helpers (for testing)
void agc_erasable_set(uint8_t bank, uint16_t addr, agc_word_t value);
agc_word_t agc_erasable_get(uint8_t bank, uint16_t addr);

// ROM/Fixed memory helpers (for testing)
void agc_rom_set(uint32_t addr, agc_word_t value);
agc_word_t agc_rom_get(uint32_t addr);

#endif 
