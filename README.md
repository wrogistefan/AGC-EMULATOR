# Apollo Guidance Computer Emulator  
### Block II Central Processor and Memory Subsystem Reconstruction  
### Technical Documentation — Revision 1.0

```
        ___    ____     ____ 
       /   |  / __ \   / __ \
      / /| | / / / /  / / / /
     / ___ |/ /_/ /  / /_/ / 
    /_/  |_/_____/   \____/  
      APOLLO GUIDANCE COMPUTER
```

---

## 1.0 PURPOSE

This repository provides a clean-room, from-scratch emulator of the Apollo Guidance Computer (AGC) Block II central processor.  
The objective is to deliver a transparent, verifiable, and extensible model of the AGC suitable for:

- architectural study  
- instruction-level analysis  
- flight software experimentation  
- simulation of Apollo-era guidance and navigation systems  
- integration with higher-level Saturn V and CSM/LM simulations  

The implementation prioritizes clarity, correctness, and traceability over performance.

---

## 2.0 SYSTEM OVERVIEW

The AGC was a 15-bit, 1’s complement machine with a banked memory architecture and a compact instruction set.  
This emulator reproduces:

- CPU registers (A, L, Q, Z, EB, FB)  
- 1’s complement arithmetic and overflow behavior  
- erasable and fixed memory with bank switching  
- core instruction set semantics  
- a minimal REPL for interactive inspection and execution  
- a growing suite of verification tests  

The design follows the structure and terminology of the original AGC Program Logic Manuals and GSOP documentation.

---

## 3.0 ARCHITECTURE

### 3.1 CPU Registers

```
+---------+---------------------------------------------+
| Register| Description                                 |
+---------+---------------------------------------------+
| A       | Accumulator (15-bit)                        |
| L       | Link Register (1-bit extension of A)        |
| Q       | Return Register (used by TC instructions)   |
| Z       | Program Counter (12-bit)                    |
| EB      | Erasable Memory Bank Register               |
| FB      | Fixed Memory Bank Register                  |
+---------+---------------------------------------------+
```

### 3.2 Word Format

```
15-bit word (1’s complement)
+-------------------------------+
| S |          Magnitude        |
+-------------------------------+
  ^
  Sign bit (0 = positive, 1 = negative)
```

### 3.3 Memory Organization

```
                AGC MEMORY MAP (Block II)
   +---------------------------------------------------+
   | 00000–01777 : Erasable Memory (Bank 0)            |
   | 02000–03777 : Erasable Memory (Bank 1)            |
   | ...                                               |
   | (EB selects bank)                                 |
   +---------------------------------------------------+
   | 04000–07777 : Fixed Memory (Bank 0)               |
   | 10000–13777 : Fixed Memory (Bank 1)               |
   | ...                                               |
   | (FB selects bank)                                 |
   +---------------------------------------------------+
```

---

## 4.0 INSTRUCTION SET SUMMARY

The emulator currently implements the core AGC instructions:

- TC — Transfer Control  
- XCH — Exchange A with memory  
- TS — Transfer to Storage  
- CA — Clear and Add  
- INDEX — Modify next instruction  
- CCS — Count, Compare, and Skip  
- ADS — Add to Storage  
- BUSY — Placeholder for unimplemented opcodes  

The disassembler and execution engine share a unified opcode table to ensure consistency.

---

## 5.0 INTERACTIVE MONITOR (REPL)

A minimal command-line interface is provided for:

- loading octal words  
- inspecting memory  
- stepping instructions  
- selecting erasable/fixed banks  
- disassembling memory regions  

Example session:

```
> load 0200 01234
> peek 0200
0200: 01234
> step
PC 0000: 010200 (XCH 0200)
```

---

## 6.0 VERIFICATION PHILOSOPHY

The emulator is validated through incremental unit tests covering:

- instruction semantics  
- register interactions  
- memory banking behavior  
- 1’s complement normalization  
- erasable vs fixed memory access rules  

The XCH test suite includes:

- erasable bank 0  
- ROM access (read-only)  
- erasable bank N via EB  
- register preservation  
- correct PC/Z increment  

This approach mirrors NASA’s original verification strategy:  
**small, deterministic tests with well-defined expected outcomes.**

---

## 7.0 ROADMAP

### 7.1 Near-Term
- Full implementation of CCS, INDEX, and overflow behavior  
- Complete disassembler  
- Expanded test coverage  
- DSKY interface module  

### 7.2 Mid-Term
- IMU simulation  
- Gimbal angle modeling  
- Executive and Waitlist scheduling  
- Interrupt handling  

### 7.3 Long-Term
- Saturn V launch vehicle guidance logic  
- CSM/LM flight software compatibility  
- Telemetry and downlink channels  
- Full mission simulation capability  

---

## 8.0 HISTORICAL NOTES

The Apollo Guidance Computer was one of the earliest digital flight computers, designed by MIT Instrumentation Laboratory.  
Its architecture influenced modern embedded systems, real-time scheduling, and fault-tolerant computing.

This emulator is not affiliated with NASA; it is an independent reconstruction for educational and research purposes.

---

## 9.0 BUILD AND EXECUTION

### Build
```
make
```

### Run
```
./agc
```

### Run Tests
```
make test
```

---

## 10.0 LICENSE

This project is released under the MIT License.  
See `LICENSE` for details.

---

## 11.0 ACKNOWLEDGMENTS

This work draws inspiration from:

- AGC Program Logic Manuals  
- GSOP (Guidance System Operations Plan)  
- surviving AGC source code (Colossus, Luminary)  
- the broader Apollo simulation community  
