#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
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

/* Helper: skip whitespace in string */
static const char *skip_ws(const char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

/* Helper: parse octal number from string, returns -1 on error */
static int parse_octal(const char *s) {
    if (!s || !*s) return -1;
    int result = 0;
    while (*s) {
        if (*s < '0' || *s > '7') return -1;
        result = (result << 3) | (*s - '0');
        s++;
    }
    return result;
}

/* Helper: parse positive long from string, returns false on error */
static bool parse_positive_long(const char *s, long *out) {
    if (!s || !*s) return false;
    char *endptr;
    long val = strtol(s, &endptr, 10);
    if (endptr == s || val <= 0) return false;
    *out = val;
    return true;
}

/* Forward declaration for print_usage */
static void print_usage(const char *cmd);

/* Helper: parse single octal argument */
static bool parse_single_octal_arg(const char *args, int *out, const char *cmd_name) {
    int v = parse_octal(skip_ws(args));
    if (v < 0) {
        print_usage(cmd_name);
        return false;
    }
    *out = v;
    return true;
}

/* Helper: parse two octal arguments */
static bool parse_two_octal_args(const char *args, int *a, int *b, const char *cmd_name) {
    /* Find the space that separates the two arguments */
    const char *space = strchr(args, ' ');
    if (!space) {
        print_usage(cmd_name);
        return false;
    }
    
    /* Parse first arg: only the part before the space */
    int first_len = space - args;
    char first_buf[16];
    if (first_len >= (int)sizeof(first_buf)) {
        print_usage(cmd_name);
        return false;
    }
    memcpy(first_buf, args, first_len);
    first_buf[first_len] = '\0';
    int first = parse_octal(skip_ws(first_buf));
    
    /* Parse second arg: skip leading ws, then find end of octal digits */
    const char *second_start = skip_ws(space);
    const char *p = second_start;
    while (*p && *p >= '0' && *p <= '7') p++;
    int second_len = p - second_start;
    if (second_len == 0) {
        print_usage(cmd_name);
        return false;
    }
    char second_buf[16];
    if (second_len >= (int)sizeof(second_buf)) {
        print_usage(cmd_name);
        return false;
    }
    memcpy(second_buf, second_start, second_len);
    second_buf[second_len] = '\0';
    int second = parse_octal(second_buf);
    
    if (first < 0 || second < 0) {
        print_usage(cmd_name);
        return false;
    }
    *a = first;
    *b = second;
    return true;
}

/* Helper: parse non-negative long argument */
static bool parse_non_negative_long(const char *args, long *out, const char *cmd_name) {
    long v;
    if (!parse_positive_long(args, &v) || v < 0) {
        print_usage(cmd_name);
        return false;
    }
    *out = v;
    return true;
}

/* Helper: split line into command and args */
static void split_command(char *line, char **cmd, char **args) {
    *cmd = (char *)skip_ws(line);
    *args = "";
    char *p = *cmd;
    while (*p && !isspace((unsigned char)*p)) p++;
    if (*p) {
        *p = '\0';
        *args = (char *)skip_ws(p + 1);
    }
}

/* Helper: print colored tag with format */
static void print_colored(const char *tag, const char *color, const char *fmt, ...) {
    va_list ap;
    printf("%s%s" CLR_RESET ": ", color, tag);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
}

/* Minimal AGC disassembler for core opcodes */
static void disasm_word(agc_word_t instr, char *buf, size_t buf_size) {
    unsigned int opcode = (instr >> 12) & 07;   /* top 3 bits */
    unsigned int addr   = instr & 07777;        /* 12-bit address */

    const char *mnemonic = "???";

    switch (opcode) {
        case 0: mnemonic = "TC";    break;
        case 1: mnemonic = "XCH";   break;
        case 2: mnemonic = "TS";    break;
        case 3: mnemonic = "CA";    break;
        case 4: mnemonic = "CCS";   break;
        case 5: mnemonic = "INDEX"; break;
        case 6: mnemonic = "ADS";   break;
        case 7: mnemonic = "BUSY";  break;
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

/* Command function typedef */
typedef bool (*command_fn)(agc_cpu_t *cpu, const char *args, bool *rom_loaded);

/* Command table entry */
typedef struct {
    const char  *name;
    const char  *usage;
    command_fn   run;
} repl_command_t;

/* Command implementations */
static bool cmd_dump(agc_cpu_t *cpu, const char *args, bool *rom_loaded) {
    (void)args; (void)rom_loaded;
    dump_cpu(cpu);
    return true;
}

static bool cmd_step(agc_cpu_t *cpu, const char *args, bool *rom_loaded) {
    (void)args; (void)rom_loaded;
    agc_cpu_step(cpu);
    return true;
}

static bool cmd_run(agc_cpu_t *cpu, const char *args, bool *rom_loaded) {
    (void)rom_loaded;
    long n;
    if (!parse_positive_long(args, &n)) {
        print_usage("run");
        return false;
    }
    for (long i = 0; i < n; ++i) {
        agc_word_t instr = agc_memory_read(cpu, cpu->Z);
        char dis[32];
        disasm_word(instr, dis, sizeof(dis));
        printf("PC %04o: %04o  (%s)\n", cpu->Z, instr, dis);
        agc_cpu_step(cpu);
    }
    return true;
}

static bool cmd_load(agc_cpu_t *cpu, const char *args, bool *rom_loaded) {
    (void)rom_loaded;
    int addr, value;
    if (!parse_two_octal_args(args, &addr, &value, "load"))
        return false;

    agc_memory_write(cpu, (agc_word_t)addr, (agc_word_t)value);
    printf("Loaded %04o into %04o (EB:%d FB:%d)\n", value, addr, cpu->EB, cpu->FB);
    return true;
}

static bool cmd_dis(agc_cpu_t *cpu, const char *args, bool *rom_loaded) {
    (void)rom_loaded;
    int addr;
    if (!parse_single_octal_arg(args, &addr, "dis"))
        return false;

    agc_word_t instr = agc_memory_read(cpu, (agc_word_t)addr);
    char dis[32];
    disasm_word(instr, dis, sizeof(dis));
    printf("(%d:%04o) %04o  %s\n", (addr < 02000) ? cpu->EB : cpu->FB, addr, instr, dis);
    return true;
}

static bool cmd_eb(agc_cpu_t *cpu, const char *args, bool *rom_loaded) {
    (void)rom_loaded;
    long b;
    if (!parse_non_negative_long(args, &b, "eb"))
        return false;
    cpu->EB = (uint8_t)b;
    printf("Switched to erasable bank %d\n", cpu->EB);
    return true;
}

static bool cmd_fb(agc_cpu_t *cpu, const char *args, bool *rom_loaded) {
    (void)rom_loaded;
    long b;
    if (!parse_non_negative_long(args, &b, "fb"))
        return false;
    cpu->FB = (uint8_t)b;
    printf("Switched to fixed bank %d\n", cpu->FB);
    return true;
}

static bool cmd_bank(agc_cpu_t *cpu, const char *args, bool *rom_loaded) {
    (void)rom_loaded;
    long b;
    if (!parse_non_negative_long(args, &b, "bank"))
        return false;
    cpu->EB = (uint8_t)b;
    cpu->FB = (uint8_t)b;
    printf("Switched to bank %ld (EB=%d FB=%d)\n", b, cpu->EB, cpu->FB);
    return true;
}

static bool cmd_peek(agc_cpu_t *cpu, const char *args, bool *rom_loaded) {
    (void)rom_loaded;
    int addr;
    if (!parse_single_octal_arg(args, &addr, "peek"))
        return false;
    agc_word_t v = agc_memory_read(cpu, (agc_word_t)addr);
    printf("%04o: %04o\n", addr, v);
    return true;
}

static bool cmd_poke(agc_cpu_t *cpu, const char *args, bool *rom_loaded) {
    (void)rom_loaded;
    int addr, value;
    if (!parse_two_octal_args(args, &addr, &value, "poke"))
        return false;
    agc_memory_write(cpu, (agc_word_t)addr, (agc_word_t)value);
    printf("Wrote %04o into %04o (EB:%d FB:%d)\n", value, addr, cpu->EB, cpu->FB);
    return true;
}

static bool cmd_mem(agc_cpu_t *cpu, const char *args, bool *rom_loaded) {
    (void)rom_loaded;
    int start, end;
    if (!parse_two_octal_args(args, &start, &end, "mem"))
        return false;
    if (start > end) {
        print_usage("mem");
        return false;
    }

    printf("\nMemory dump (EB:%d FB:%d):\n", cpu->EB, cpu->FB);

    int addr = start;
    while (addr <= end) {
        printf(CLR_ADDR "%04o" CLR_RESET ": ", addr);

        for (int i = 0; i < 8 && addr <= end; ++i, ++addr) {
            agc_word_t v = agc_memory_read(cpu, (agc_word_t)addr);

            const char *color = (v == 0) ? CLR_ZERO : CLR_NONZERO;
            if (addr == cpu->Z) {
                color = CLR_PC;
            }

            printf("%s%04o" CLR_RESET " ", color, v);
        }
        printf("\n");
    }
    printf("\n");
    return true;
}

static bool cmd_rom(agc_cpu_t *cpu, const char *args, bool *rom_loaded) {
    (void)cpu;
    char filename[128];
    if (sscanf(args, "%127s", filename) != 1) {
        print_colored("Usage", CLR_ERROR, "rom <filename>");
        return false;
    }
    if (agc_load_rom(filename)) {
        printf("ROM loaded from %s\n", filename);
        *rom_loaded = true;
    } else {
        printf("Failed to load ROM from %s\n", filename);
        return false;
    }
    return true;
}

static bool cmd_quit(agc_cpu_t *cpu, const char *args, bool *rom_loaded) {
    (void)cpu; (void)args; (void)rom_loaded;
    return false;  /* signal to exit */
}

/* Command table */
static const repl_command_t commands[] = {
    { "dump", "dump                       - show CPU registers", cmd_dump },
    { "step", "step                      - execute one instruction", cmd_step },
    { "run",  "run <positive_number>     - execute n instructions", cmd_run },
    { "load", "load <addr> <octal_value> - write instruction/data", cmd_load },
    { "dis",  "dis <addr>                - disassemble word at addr", cmd_dis },
    { "eb",   "eb <n>                    - set erasable bank (EB)", cmd_eb },
    { "fb",   "fb <n>                    - set fixed bank (FB)", cmd_fb },
    { "bank", "bank <n>                  - set both banks to n", cmd_bank },
    { "peek", "peek <addr>               - read memory at addr", cmd_peek },
    { "poke", "poke <addr> <val>         - write val to addr", cmd_poke },
    { "mem",  "mem <start> <end>         - dump memory range", cmd_mem },
    { "rom",  "rom <filename>            - load ROM binary", cmd_rom },
    { "quit", "quit                      - exit emulator", cmd_quit },
};

static void print_usage(const char *cmd) {
    if (cmd) {
        /* Find and print usage for specific command */
        for (size_t i = 0; i < sizeof(commands)/sizeof(commands[0]); i++) {
            if (strcmp(commands[i].name, cmd) == 0) {
                print_colored("Usage", CLR_ERROR, "%s", commands[i].usage);
                return;
            }
        }
    }
    /* Print all commands */
    printf(CLR_HEADER "Available commands:\n" CLR_RESET);
    for (size_t i = 0; i < sizeof(commands)/sizeof(commands[0]); i++) {
        printf("  " CLR_INFO "%s\n" CLR_RESET, commands[i].usage);
    }
    printf("\n");
}

static const repl_command_t *find_command(const char *name) {
    for (size_t i = 0; i < sizeof(commands)/sizeof(commands[0]); i++) {
        if (strcmp(commands[i].name, name) == 0) {
            return &commands[i];
        }
    }
    return NULL;
}

static void repl(void) {
    agc_cpu_t cpu;
    agc_cpu_reset(&cpu);

    bool rom_loaded = false;

    printf(CLR_HEADER "AGC Emulator Interactive Mode\n" CLR_RESET);
    print_usage(NULL);

    char line[256];

    for (;;) {
        printf(CLR_PROMPT "agc> " CLR_RESET);
        if (!fgets(line, sizeof(line), stdin))
            break;

        /* strip newline */
        line[strcspn(line, "\r\n")] = '\0';

        /* skip empty lines */
        if (skip_ws(line)[0] == '\0')
            continue;

        char *cmd, *args;
        split_command(line, &cmd, &args);

        const repl_command_t *entry = find_command(cmd);
        if (!entry) {
            printf(CLR_ERROR "Unknown command: %s\n" CLR_RESET, cmd);
            print_usage(NULL);
            continue;
        }

        if (!entry->run(&cpu, args, &rom_loaded))
            break;
    }
}

int main(void) {
    repl();
    return 0;
}
