#ifndef AGC_TYPES_H
#define AGC_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/*
 * AGC uses 15-bit words:
 *  - bit 14: sign bit (1 = negative)
 *  - bits 0–13: magnitude
 * Arithmetic is 1's complement, so:
 *  - negative numbers are bitwise NOT of positive
 *  - there are two zeros: +0 (0x0000) and -0 (0x7FFF)
 */

typedef uint16_t agc_word_t;

#define AGC_WORD_MASK   0x7FFF   // 15 bits
#define AGC_SIGN_BIT    0x4000   // bit 14

// Normalize to 15 bits (masking out garbage)
static inline agc_word_t agc_normalize(agc_word_t w) {
    return w & AGC_WORD_MASK;
}

// Check if value is negative
static inline bool agc_is_negative(agc_word_t w) {
    return (w & AGC_SIGN_BIT) != 0;
}

// Convert to negative (1's complement)
static inline agc_word_t agc_negate(agc_word_t w) {
    return agc_normalize(~w);
}

// Add two AGC words with proper 1's complement arithmetic
// Handles negative offsets correctly
static inline agc_word_t agc_add(agc_word_t a, agc_word_t b) {
    // In 1's complement: a + b = a + b (with end-around carry)
    uint32_t sum = (uint32_t)a + (uint32_t)b;
    uint32_t carry = sum >> 15;
    // Second normalization needed: adding carry can reintroduce a 16th bit
    return agc_normalize(agc_normalize((agc_word_t)sum) + (agc_word_t)carry);
}

#endif // AGC_TYPES_H
