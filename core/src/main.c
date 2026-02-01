#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "agc_cpu.h"
#include "agc_memory.h"
#include "agc_instructions.h"

/* ANSI colors */
#define CLR_RESET   "\033[0m"
#define CLR_PROMPT  "\033[1;36m"
#define CLR_INFO    "\033[1;32m"
#define CLR_ERROR   "\033[1;31m"
#define CLR_HEADER  "\033[1;35m"
#define CLR_ADDR    "\033[1;36m"
#define CLR_DATA    "\033[1;32m"
#define CLR_ZERO    "\033[1;30m"
#define CLR_NONZERO "\033[1;33m"
#define CLR_PC      "\033[1;34m"

/* Minimal AGC disassembler for core opcodes */
static void disasm_word(agc_word_t instr, char *buf, size_t buf_size) {
    unsigned int opcode = (instr >> 12) & 07;   /* top 3 bits */
    unsigned int addr   = instr & 07777;        /* 12-bit address */

    const char *mnemonic = "???";

    switch (opcode) {
        case 0: mnemonic = "TC";    break;
        case 1: mnemonic = "CCS";   break;
        case 2: mnemonic = "INDEX"; break;
        case 3: mnemonic = "XCH";   break;
        default: mnemonic = "???";  break;
    }

    snprintf(buf, buf_size, "%s %04o", mnemonic, addr);
}

static void dump_cpu(const agc_cpu_t *cpu) {
    printf(CLR_HEADER "\n=== AGC CPU STATE ===\n" CLR_RESET);
    printf(CLR_INFO "EB: %d, FB: %d\n" CLR_RESET, cpu->EB, cpu->FB);
    printf("A: %04o\n", cpu->A);
    printf("L: %04o\n", cpu->L);
    printf("Q: %04o\n", cpu->Q);
    printf("Z: %04o\n", cpu->Z);
    printf(CLR_HEADER "=====================\n\n" CLR_RESET);
}

static void repl(void) {
    agc_cpu_t cpu;
    agc_cpu_reset(&cpu);

    bool rom_loaded = false;

    printf(CLR_HEADER "AGC Emulator Interactive Mode\n" CLR_RESET);
    printf("Commands:\n");
    printf("  " CLR_INFO "load <addr> <octal_value" CLR_RESET ">   - write instruction/data\n");
    printf("  " CLR_INFO "step" CLR_RESET "                       - execute one instruction\n");
    printf("  " CLR_INFO "run <n>" CLR_RESET "                    - execute n instructions\n");
    printf("  " CLR_INFO "dump" CLR_RESET "                       - show CPU registers\n");
    printf("  " CLR_INFO "dis <addr>" CLR_RESET "                 - disassemble word at addr\n");
    printf("  " CLR_INFO "eb <n>" CLR_RESET "                   - set erasable bank (EB)\n");
    printf("  " CLR_INFO "fb <n>" CLR_RESET "                   - set fixed bank (FB)\n");
    printf("  " CLR_INFO "mem <start> <end>" CLR_RESET "          - dump memory range\n");
    printf("  " CLR_INFO "peek <addr>" CLR_RESET "                - read memory at addr\n");
    printf("  " CLR_INFO "poke <addr> <val>" CLR_RESET "          - write val to addr\n");
    printf("  " CLR_INFO "rom <filename>" CLR_RESET "              - load ROM binary\n");
    printf("  " CLR_INFO "quit" CLR_RESET "                       - exit emulator\n\n");

    char line[256];

    for (;;) {
        printf(CLR_PROMPT "agc> " CLR_RESET);
        if (!fgets(line, sizeof(line), stdin))
            break;

        /* strip newline */
        line[strcspn(line, "\r\n")] = '\0';

        if (strncmp(line, "quit", 4) == 0)
            break;

        if (strncmp(line, "dump", 4) == 0) {
            dump_cpu(&cpu);
            continue;
        }

        if (strncmp(line, "step", 4) == 0) {
            agc_cpu_step(&cpu);
            continue;
        }

        if (strncmp(line, "run", 3) == 0) {
            long n = 0;
            if (sscanf(line, "run %ld", &n) == 1 && n > 0) {
                for (long i = 0; i < n; ++i) {
                    agc_word_t instr = agc_memory_read(&cpu, cpu.Z);
                    char dis[32];
                    disasm_word(instr, dis, sizeof(dis));
                    printf("PC %04o: %04o  (%s)\n", cpu.Z, instr, dis);
                    agc_cpu_step(&cpu);
                }
            } else {
                printf(CLR_ERROR "Usage: run <positive_number>\n" CLR_RESET);
            }
            continue;
        }

        if (strncmp(line, "load", 4) == 0) {
            int addr;
            unsigned int value;
            if (sscanf(line, "load %o %o", &addr, &value) == 2) {
                agc_memory_write(&cpu, addr, (agc_word_t)value);
                printf("Loaded %04o into %04o (EB:%d FB:%d)\n", value, addr, cpu.EB, cpu.FB);
            } else {
                printf(CLR_ERROR "Usage: load <addr> <octal_value>\n" CLR_RESET);
            }
            continue;
        }

        if (strncmp(line, "dis", 3) == 0) {
            int addr;
            if (sscanf(line, "dis %o", &addr) == 1) {
                agc_word_t instr = agc_memory_read(&cpu, addr);
                char dis[32];
                disasm_word(instr, dis, sizeof(dis));
                printf("(%d:%04o) %04o  %s\n", (addr < 02000) ? cpu.EB : cpu.FB, addr, instr, dis);
            } else {
                printf(CLR_ERROR "Usage: dis <addr>\n" CLR_RESET);
            }
            continue;
        }

        if (strncmp(line, "eb", 2) == 0) {
            int b;
            if (sscanf(line, "eb %d", &b) == 1 && b >= 0) {
                cpu.EB = b;
                printf("Switched to erasable bank %d\n", cpu.EB);
            } else {
                printf("Usage: eb <non_negative_integer>\n");
            }
            continue;
        }
        
        if (strncmp(line, "fb", 2) == 0) {
            int b;
            if (sscanf(line, "fb %d", &b) == 1 && b >= 0) {
                cpu.FB = b;
                printf("Switched to fixed bank %d\n", cpu.FB);
            } else {
                printf("Usage: fb <non_negative_integer>\n");
            }
            continue;
        }
        
        if (strncmp(line, "bank", 4) == 0) {
            int b;
            if (sscanf(line, "bank %d", &b) == 1 && b >= 0) {
                // Legacy command: set both EB and FB to same bank
                cpu.EB = b;
                cpu.FB = b;
                printf("Switched to bank %d (EB=%d FB=%d)\n", b, cpu.EB, cpu.FB);
            } else {
                printf("Usage: bank <non_negative_integer>\n");
            }
            continue;
        }
        
        if (strncmp(line, "peek", 4) == 0) {
            int addr;
            if (sscanf(line, "peek %o", &addr) == 1) {
                agc_word_t v = agc_memory_read(&cpu, addr);
                printf("%04o: %04o\n", addr, v);
            } else {
                printf("Usage: peek <addr>\n");
            }
            continue;
        }

        if (strncmp(line, "poke", 4) == 0) {
            int addr;
            unsigned int value;
            if (sscanf(line, "poke %o %o", &addr, &value) == 2) {
                agc_memory_write(&cpu, addr, (agc_word_t)value);
                printf("Wrote %04o into %04o (EB:%d FB:%d)\n", value, addr, cpu.EB, cpu.FB);
            } else {
                printf("Usage: poke <addr> <octal_value>\n");
            }
            continue;
        }

        if (strncmp(line, "mem", 3) == 0) {
            int start, end;
            if (sscanf(line, "mem %o %o", &start, &end) == 2) {
                printf("\nMemory dump (EB:%d FB:%d):\n", cpu.EB, cpu.FB);

                int addr = start;
                while (addr <= end) {
                    printf(CLR_ADDR "%04o" CLR_RESET ": ", addr);

                    for (int i = 0; i < 8 && addr <= end; ++i, ++addr) {
                        agc_word_t v = agc_memory_read(&cpu, addr);

                        const char *color = (v == 0) ? CLR_ZERO : CLR_NONZERO;
                        if (addr == cpu.Z) {
                            color = CLR_PC;
                        }

                        printf("%s%04o" CLR_RESET " ", color, v);
                    }
                    printf("\n");
                }
                printf("\n");
            } else {
                printf("Usage: mem <start> <end>\n");
            }
            continue;
        }

        if (strncmp(line, "rom", 3) == 0) {
            char filename[128];
            if (sscanf(line, "rom %127s", filename) == 1) {
                if (agc_load_rom(filename)) {
                    printf("ROM loaded from %s\n", filename);
                    rom_loaded = true;
                } else {
                    printf("Failed to load ROM from %s\n", filename);
                }
            } else {
                printf("Usage: rom <filename>\n");
            }
            continue;
        }

        printf(CLR_ERROR "Unknown command\n" CLR_RESET);
    }
}

int main(void) {
    repl();
    return 0;
}
