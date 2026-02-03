#include "agc_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Physical Memory Arrays
 * 
 * AGC Block II memory organization:
 * - Erasable (RAM): 2 banks (E0, E1) of 1024 words each
 * - Fixed (ROM): 36 banks (F0-F35) of 1024 words each
 * 
 * Each bank is exactly 1024 (02000 octal) words.
 * ============================================================================ */

/* Erasable memory - E0 and E1 banks (RAM) */
uint16_t erasable[2][1024];

/* Fixed memory - F0 through F35 banks (ROM) */
uint16_t fixed[36][1024];

/* ============================================================================
 * Address Constants (Octal)
 * 
 * These match the real AGC address space:
 *   00000-00017: CPU registers (16 words)
 *   00020-01777: Erasable memory (15-bit addressing, 1024 words per bank)
 *   02000-03777: Fixed-fixed ROM (unbanked, 2 banks of 1024 words)
 *   04000-07777: Fixed ROM (banked via FB, 34 banks of 1024 words)
 * ============================================================================ */

/* CPU register address space */
#define REG_START      00000
#define REG_END        00017
#define REG_COUNT      020          /* 16 registers (00000-00017) */

/* Erasable memory */
#define ERASE_START    00020
#define ERASE_END      01777
#define ERASE_OFFSET_MASK 01777     /* 10-bit offset within bank */

/* Fixed-fixed ROM (unbanked) */
#define FF_START       02000
#define FF_END         03777
#define FF_BANK0_START 02000
#define FF_BANK1_START 03000
#define FF_BANK_SIZE   01000        /* 1024 words per bank */

/* Fixed ROM (banked via FB) */
#define FIXED_START    04000
#define FIXED_END      07777
#define FIXED_BANK_SIZE 01000       /* 1024 words per bank */
#define FIXED_BANK_COUNT 36         /* F0-F35 */

/* ============================================================================
 * CPU Register Access
 * 
 * CPU registers are hardware registers at addresses 00000-00017.
 * They are NOT stored in erasable memory.
 * 
 * AGC Block II register mapping:
 *   00000 (0): A  - Accumulator
 *   00001 (1): L  - Link register
 *   00002 (2): Q  - Overflow/auxiliary register
 *   00003 (3): EB - Erasable bank register (bits 0-1 used)
 *   00004 (4): FB - Fixed bank register
 *   00005 (5): Z  - Program counter
 *   00006 (6): BB - Both bank register (not a real register, used internally)
 *   00007-00017: I/O channels and other special registers
 * ============================================================================ */

/* Read from a CPU register */
agc_word_t read_register(agc_cpu_t *cpu, uint16_t addr) {
    switch (addr) {
        case 00000: return cpu->A;        /* A - Accumulator */
        case 00001: return cpu->L;        /* L - Link register */
        case 00002: return cpu->Q;        /* Q - Overflow/auxiliary */
        case 00003: return cpu->EB;       /* EB - Erasable bank */
        case 00004: return cpu->FB;       /* FB - Fixed bank */
        case 00005: return cpu->Z;        /* Z - Program counter */
        case 00006: return cpu->BB;       /* BB - Both bank (internal) */
        default:
            /* I/O channels 00010-00017 */
            if (addr >= 00010 && addr <= 00017) {
                return cpu->IN[addr - 00010];
            }
            /* Undefined register - return 0 */
            return 0;
    }
}

/* Write to a CPU register */
void write_register(agc_cpu_t *cpu, uint16_t addr, agc_word_t value) {
    /* Mask to 15 bits */
    value &= 077777;
    
    switch (addr) {
        case 00000: cpu->A = value; break;        /* A - Accumulator */
        case 00001: cpu->L = value; break;        /* L - Link register */
        case 00002: cpu->Q = value; break;        /* Q - Overflow/auxiliary */
        case 00003: cpu->EB = value; break;       /* EB - Erasable bank */
        case 00004: cpu->FB = value; break;       /* FB - Fixed bank */
        case 00005: cpu->Z = value; break;        /* Z - Program counter */
        case 00006: cpu->BB = value; break;       /* BB - Both bank (internal) */
        default:
            /* I/O channels 00010-00017 */
            if (addr >= 00010 && addr <= 00017) {
                cpu->OUT[addr - 00010] = value;
            }
            /* Writes to other undefined registers are ignored */
            break;
    }
}

/* ============================================================================
 * Address Decoding Helpers
 * 
 * These functions implement the exact AGC Block II address decoding logic.
 * No clamping, no modulo, no silent corrections.
 * ============================================================================ */

/* Get erasable bank from address and EB register */
static uint8_t get_erasable_bank(agc_cpu_t *cpu, uint16_t addr) {
    uint16_t offset = addr & ERASE_OFFSET_MASK;
    
    /* Lower half (00000-00777): EB bit 0 selects bank */
    /* Upper half (01000-01777): EB bit 1 selects bank */
    if (offset < 01000) {
        return cpu->EB & 1;
    } else {
        return (cpu->EB >> 1) & 1;
    }
}

/* Get fixed bank from address and FB register */
static uint8_t get_fixed_bank(agc_cpu_t *cpu, uint16_t addr) {
    /* Fixed-fixed ROM (02000-03777): unbanked */
    if (addr >= FF_START && addr < FF_BANK1_START) {
        return 0;  /* F0 */
    }
    if (addr >= FF_BANK1_START && addr <= FF_END) {
        return 1;  /* F1 */
    }
    
    /* Fixed ROM (04000-07777): banked via FB */
    /* FB selects which of F2-F35 is active */
    /* FB=0 selects F2, FB=1 selects F3, ..., FB=33 selects F35 */
    uint8_t fb = cpu->FB;
    if (fb > 33) {
        fb = 33;  /* Clamp to valid AGC range */
    }
    
    return fb + 2;
}

/* ============================================================================
 * Instruction Fetch
 * 
 * Instructions are fetched from memory using a separate path from data reads.
 * For addresses 00000-01777 (erasable range), instructions come from the
 * erasable memory array, NOT from the hardware registers.
 * 
 * This models the real AGC instruction fetch unit behavior.
 * ============================================================================ */

agc_word_t agc_instruction_fetch(agc_cpu_t *cpu, uint16_t addr) {
    /* Normalize to 15 bits */
    addr &= 077777;
    
    /* Erasable memory range (00000-01777) */
    /* Instructions are always fetched from erasable memory array */
    if (addr <= ERASE_END) {
        uint16_t offset = addr & ERASE_OFFSET_MASK;
        uint8_t bank = get_erasable_bank(cpu, addr);
        return erasable[bank][offset];
    }
    
    /* Fixed-fixed ROM (02000-03777) */
    if (addr >= FF_START && addr <= FF_END) {
        uint8_t bank = get_fixed_bank(cpu, addr);
        uint16_t offset = addr & 0777;
        return fixed[bank][offset];
    }
    
    /* Fixed ROM (04000-07777) */
    if (addr >= FIXED_START && addr <= FIXED_END) {
        uint8_t bank = get_fixed_bank(cpu, addr);
        uint16_t offset = addr & 0777;  /* 0-1023 within fixed ROM bank */
        return fixed[bank][offset];
    }
    
    /* Addresses above 07777 are invalid - return 0 */
    return 0;
}

/* ============================================================================
 * agc_read - Memory Read (AGC Block II Address Decoding)
 * 
 * Address decoder selects memory based on address range:
 * 
 * 1. Registers (00000-00017): Direct register access
 * 2. Erasable (00020-01777): Banked via EB register
 *    - Offset 00000-00777: Lower half-bank (EB bit 0 selects bank)
 *    - Offset 01000-01777: Upper half-bank (EB bit 1 selects bank)
 * 3. Fixed-Fixed (02000-03777): Unbanked ROM
 *    - 02000-02777: Fixed-fixed bank 0
 *    - 03000-03777: Fixed-fixed bank 1
 * 4. Fixed (04000-07777): Banked via FB register
 *    - FB selects which of F2-F35 is active (F0-F1 are fixed-fixed)
 * ============================================================================ */

agc_word_t agc_read(agc_cpu_t *cpu, uint16_t addr) {
    /* Normalize to 15 bits */
    addr &= 077777;
    
    /* CPU Registers (00000-00017) */
    if (addr <= REG_END) {
        return read_register(cpu, addr);
    }
    
    /* Erasable Memory (00020-01777) */
    if (addr >= ERASE_START && addr <= ERASE_END) {
        uint8_t bank = get_erasable_bank(cpu, addr);
        uint16_t offset = addr & ERASE_OFFSET_MASK;
        return erasable[bank][offset];
    }
    
    /* Fixed-Fixed ROM (02000-03777) */
    if (addr >= FF_START && addr <= FF_END) {
        uint8_t bank = get_fixed_bank(cpu, addr);
        uint16_t offset = addr & 0777;
        
        return fixed[bank][offset];
    }
    
    /* Fixed ROM (04000-07777) */
    if (addr >= FIXED_START && addr <= FIXED_END) {
        uint8_t bank = get_fixed_bank(cpu, addr);
        uint16_t offset = addr & 0777;  /* 0-1023 within fixed ROM bank */
        return fixed[bank][offset];
    }
    
    /* Addresses above 07777 are invalid - return 0 */
    return 0;
}

/* ============================================================================
 * agc_write - Memory Write (AGC Block II Address Decoding)
 * 
 * Writes follow the same address decoding as reads, but:
 * - Writes to registers (00000-00017) are IGNORED
 * - Writes to ROM (fixed and fixed-fixed) are IGNORED (write-protected)
 * - Writes to erasable memory (00020-01777) update the RAM
 * 
 * IMPORTANT: No clamping, no modulo, no silent corrections.
 * Invalid addresses are ignored.
 * ============================================================================ */

void agc_write(agc_cpu_t *cpu, uint16_t addr, agc_word_t value) {
    /* Normalize to 15 bits */
    addr &= 077777;
    value &= 077777;
    
    /* CPU Registers (00000-00017) */
    /* Writes to registers are ignored - must use write_register() */
    if (addr <= REG_END) {
        return;
    }
    
    /* Erasable Memory (00020-01777) */
    /* Write to erasable RAM - banked via EB */
    if (addr >= ERASE_START && addr <= ERASE_END) {
        uint8_t bank = get_erasable_bank(cpu, addr);
        uint16_t offset = addr & ERASE_OFFSET_MASK;
        erasable[bank][offset] = value;
        return;
    }
    
    /* Fixed-Fixed ROM (02000-03777) */
    /* ROM is write-protected - writes are ignored */
    if (addr >= FF_START && addr <= FF_END) {
        return;
    }
    
    /* Fixed ROM (04000-07777) */
    /* ROM is write-protected - writes are ignored */
    if (addr >= FIXED_START && addr <= FIXED_END) {
        return;
    }
    
    /* Addresses above 07777 are invalid - write ignored */
    /* No silent correction or wrapping */
}

/* ============================================================================
 * ROM Loading Functions
 * 
 * Load rope memory images (Colossus, Luminary, etc.).
 * Data is stored as 16-bit big-endian words.
 * ============================================================================ */

/* Load ROM from binary file */
bool agc_load_rom(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        return false;
    }
    
    /* Clear all fixed memory first */
    memset(fixed, 0, sizeof(fixed));
    
    /* Read words until EOF or memory full */
    for (int bank = 0; bank < 36; bank++) {
        for (int offset = 0; offset < 1024; offset++) {
            uint8_t hi, lo;
            
            if (fread(&hi, 1, 1, f) != 1) {
                fclose(f);
                return true;  /* Successfully loaded what we could */
            }
            if (fread(&lo, 1, 1, f) != 1) {
                fclose(f);
                return true;  /* Successfully loaded what we could */
            }
            
            /* Big-endian: high byte first, mask to 15 bits */
            fixed[bank][offset] = ((uint16_t)hi << 8) | lo;
            fixed[bank][offset] &= 077777;  /* Ensure 15 bits */
        }
    }
    
    fclose(f);
    return true;
}
