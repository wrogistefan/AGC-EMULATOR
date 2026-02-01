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

#endif // AGC_MEMORY_H
