#ifndef PSP_ENTROPY_H
#define PSP_ENTROPY_H

#include <stdint.h>
#include <stddef.h>

/* Gathers hardware entropy (RTC microsecond ticks, CPU cycles, analog stick noise,
 * battery millivolts, human keystroke jitter) and hashes it with SHA-256 to produce
 * cryptographically strong random bytes. */
void psp_entropy_gather(uint8_t *out_buf, size_t len);

/* Feed additional keystroke timing or analog noise jitter into the entropy pool */
void psp_entropy_feed(uint32_t val);

#endif /* PSP_ENTROPY_H */
