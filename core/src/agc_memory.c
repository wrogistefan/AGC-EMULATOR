#include "agc_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================================
 * Physical Memory Arrays
 *
 * AGC Block II memory organization (ref: MIT IL E-2536):
 *   Erasable (RAM): 8 banks (E0-E7) × 256 words = 2048 words total
 *   Fixed   (ROM): 36 banks (F0-F35) × 1024 words = 36864 words total
 * ============================================================================ */

static uint16_t erasable[8][256];
static uint16_t fixed[36][1024];

/* ============================================================================
 * Address Constants (Octal)
 *
 * AGC Block II address space (ref: MIT IL E-2536 §2):
 *
 *   00000-00017₈  CPU registers (16 words)
 *   00020-01377₈  Unswitched erasable (banks E0-E2, decoded from addr bits 9-8)
 *   01400-01777₈  Switched erasable   (bank selected by EB register)
 *   02000-03777₈  Fixed-fixed ROM     (bank 0, 1K, always mapped, no banking)
 *   04000-07777₈  Fixed ROM           (bank selected by FB register)
 *
 * Erasable bank encoding (ref: E-2536 §2.3; yaAGC agc_engine.c):
 *   EB = bank_number × 256  (bank number in bits 10-8)
 *   Extract: (EB >> 8) & 7
 *
 * Fixed bank encoding (ref: E-2536 §3; yaAGC agc_engine.c):
 *   FB = bank_number × 1024  (bank number in bits 14-10)
 *   Extract: (FB >> 10) & 037
 *   Banks 32-35 require SBANK (not yet implemented — PR4).
 * ============================================================================ */

/* CPU register space */
#define REG_START             00000
#define REG_END               00017

/* Erasable */
#define ERASE_START           00020
#define ERASE_END             01777
#define ERASE_SWITCHED_START  01400   /* 768 decimal — start of switched window */
#define ERASE_BANK_SIZE       0400    /* 256 words per erasable bank (octal) */
#define ERASE_BANK_COUNT      8       /* E0-E7 */

/* Fixed-fixed ROM */
#define FF_START              02000
#define FF_END                03777
#define FF_BANK_SIZE          02000   /* 1024 words */

/* Fixed ROM (banked) */
#define FIXED_START           04000
#define FIXED_END             07777
#define FIXED_BANK_SIZE       02000   
#define FIXED_BANK_COUNT      36      

/* ============================================================================
 * CPU Register Access
 *
 * The register file occupies addresses 00000-00017.
 * These are hardware registers, NOT stored in the erasable array.
 *
 * EB register: full 15-bit value. Bank = (EB >> 8) & 07.
 * FB register: full 15-bit value. Bank = (FB >> 10) & 037.
 * BB is not a hardware register (ignored).
 * ============================================================================ */

agc_word_t read_register(agc_cpu_t *cpu, uint16_t addr) {
    switch (addr) {
        case 00000: return cpu->A;
        case 00001: return cpu->L;
        case 00002: return cpu->Q;
        case 00003: return cpu->EB;
        case 00004: return cpu->FB;
        case 00005: return cpu->Z;
        case 00006: return 0;  /* BB: not a real hardware register */
        case 00007: return 0;  /* Hardwired zero */
        default:
            if (addr >= 00010 && addr <= 00017) {
                return cpu->IN[addr - 00010];
            }
            return 0;
    }
}

void write_register(agc_cpu_t *cpu, uint16_t addr, agc_word_t value) {
    value &= 077777;
    switch (addr) {
        case 00000: cpu->A  = value; break;
        case 00001: cpu->L  = value; break;
        case 00002: cpu->Q  = value; break;
        case 00003: cpu->EB = value; break;  /* Full 15-bit: bank = (EB >> 8) & 07 */
        case 00004: cpu->FB = value; break;  /* Full 15-bit: bank = (FB >> 10) & 037 */
        case 00005: cpu->Z  = value; break;
        case 00006: break;  /* BB: not a real hardware register, ignore */
        case 00007: break;  /* Hardwired zero, ignore */
        default:
            if (addr >= 00010 && addr <= 00017) {
                cpu->OUT[addr - 00010] = value;
            }
            break;
    }
}

/* ============================================================================
 * Address Decoding Helpers
 *
 * No clamping, no modulo, no silent corrections.
 * ============================================================================ */

/* Erasable bank selection.
 *
 * Real AGC Block II:
 *   - Unswitched region (00000-01377): always bank 0
 *   - Switched region (01400-01777): EB register selects bank
 *   - Bank = (EB >> 8) & 07
 */
static uint8_t get_erasable_bank(agc_cpu_t *cpu, uint16_t addr) {
    if (addr < ERASE_SWITCHED_START) {
        return 0;  /* Unswitched: always bank 0 */
    }
    return (cpu->EB >> 8) & 7;  /* Switched: bank from EB */
}

/* Fixed bank selection.
 *
 * Real AGC Block II:
 *   - Fixed-fixed (02000-03777): always bank 0
 *   - Fixed-switchable (04000-07777): FB register selects bank
 *   - Bank = (FB >> 10) & 037
 *
 * Returns FIXED_BANK_COUNT as invalid sentinel if bank >= 36.
 */
static uint8_t get_fixed_bank(agc_cpu_t *cpu, uint16_t addr) {
    if (addr >= FF_START && addr <= FF_END) {
        return 0;
    }
    uint8_t bank = (cpu->FB >> 10) & 037;
    if (bank >= FIXED_BANK_COUNT) {
        return FIXED_BANK_COUNT;
    }
    return bank;
}

/* ============================================================================
 * Instruction Fetch
 *
 * Instructions in the erasable range (00000-01777₈) are always fetched from
 * the erasable memory array, bypassing the CPU register file.  This models
 * the real AGC instruction fetch unit, which reads the raw erasable cells
 * even at addresses that overlap with hardware registers.
 *
 * Invalid accesses (invalid bank index, unmapped address) return 0 by design.
 * No silent clamping or correction.
 * ============================================================================ */

agc_word_t agc_instruction_fetch(agc_cpu_t *cpu, uint16_t addr) {
    addr &= 077777;

    /* Erasable (00000-01777₈): fetch directly from array, bypassing registers */
    if (addr <= ERASE_END) {
        uint8_t  bank   = get_erasable_bank(cpu, addr);
        if (bank >= ERASE_BANK_COUNT) return 0;
        uint16_t offset = addr & (ERASE_BANK_SIZE - 1);
        return erasable[bank][offset];
    }

    /* Fixed-fixed ROM (02000-03777₈) */
    if (addr >= FF_START && addr <= FF_END) {
        uint8_t bank = get_fixed_bank(cpu, addr);
        if (bank >= FIXED_BANK_COUNT) return 0;
        uint16_t offset = addr & (FF_BANK_SIZE - 1);
        return fixed[bank][offset];
    }

    /* Fixed ROM (04000-07777₈) */
    if (addr >= FIXED_START && addr <= FIXED_END) {
        uint8_t bank = get_fixed_bank(cpu, addr);
        if (bank >= FIXED_BANK_COUNT) return 0;
        uint16_t offset = addr & (FIXED_BANK_SIZE - 1);
        return fixed[bank][offset];
    }

    return 0;
}

/* ============================================================================
 * agc_read — Memory Read
 *
 * Address decoding:
 *   00000-00017₈  → CPU register file
 *   00020-01377₈  → Unswitched erasable (bank implicit in address)
 *   01400-01777₈  → Switched erasable   (bank from EB)
 *   02000-03777₈  → Fixed-fixed ROM     (bank 0)
 *   04000-07777₈  → Fixed ROM           (bank from FB)
 *
 * Invalid accesses (invalid bank index, unmapped address) return 0 by design.
 * No silent clamping or correction — this is hardware-faithful error signaling.
 * ============================================================================ */

agc_word_t agc_read(agc_cpu_t *cpu, uint16_t addr) {
    addr &= 077777;

    if (addr <= REG_END) {
        return read_register(cpu, addr);
    }

    if (addr >= ERASE_START && addr <= ERASE_END) {
        uint8_t  bank   = get_erasable_bank(cpu, addr);
        if (bank >= ERASE_BANK_COUNT) return 0;
        uint16_t offset = addr & (ERASE_BANK_SIZE - 1);
        return erasable[bank][offset];
    }

    if (addr >= FF_START && addr <= FF_END) {
        uint8_t bank = get_fixed_bank(cpu, addr);
        if (bank >= FIXED_BANK_COUNT) return 0;
        uint16_t offset = addr & (FF_BANK_SIZE - 1);
        return fixed[bank][offset];
    }

    if (addr >= FIXED_START && addr <= FIXED_END) {
        uint8_t bank = get_fixed_bank(cpu, addr);
        if (bank >= FIXED_BANK_COUNT) return 0;
        uint16_t offset = addr & (FIXED_BANK_SIZE - 1);
        return fixed[bank][offset];
    }

    return 0;
}

/* ============================================================================
 * agc_write — Memory Write
 *
 * Writes to CPU registers (00000-00017₈) are routed through write_register,
 * which handles EB, FB, BB, and I/O channels correctly.
 *
 * Writes to ROM (fixed-fixed and fixed) are silently ignored.
 *
 * No clamping, no modulo, no silent corrections on addresses.
 * ============================================================================ */

void agc_write(agc_cpu_t *cpu, uint16_t addr, agc_word_t value) {
    addr  &= 077777;
    value &= 077777;

    if (addr <= REG_END) {
        write_register(cpu, addr, value);
        return;
    }

    if (addr >= ERASE_START && addr <= ERASE_END) {
        uint8_t  bank   = get_erasable_bank(cpu, addr);
        uint16_t offset = addr & (ERASE_BANK_SIZE - 1);
        erasable[bank][offset] = value;
        return;
    }

    /* Fixed memory is ROM — writes silently ignored */
}

/* ============================================================================
 * ROM Loading
 *
 * Loads a rope-memory binary image (Colossus, Luminary, etc.).
 * Format: 36 banks × 1024 words × 2 bytes (big-endian, 15 bits used).
 *
 * Returns:
 *   - true:  complete ROM image loaded successfully
 *   - false: file not found, read error, or incomplete (early EOF)
 *
 * No silent correction: partial images are rejected, not silently accepted.
 * ============================================================================ */

bool agc_load_rom(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return false;

    memset(fixed, 0, sizeof(fixed));

    for (int bank = 0; bank < FIXED_BANK_COUNT; bank++) {
        for (int offset = 0; offset < 1024; offset++) {
            uint8_t hi, lo;
            if (fread(&hi, 1, 1, f) != 1) { fclose(f); return false; }
            if (fread(&lo, 1, 1, f) != 1) { fclose(f); return false; }
            fixed[bank][offset] = (((uint16_t)hi << 8) | lo) & 077777;
        }
    }

    fclose(f);
    return true;
}
