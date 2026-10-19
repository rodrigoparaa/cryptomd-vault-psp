#include "psp_entropy.h"
#include "../core/crypto_primitives.h"
#include <string.h>

#ifdef __PSP__
#include <pspkernel.h>
#include <psprtc.h>
#include <psppower.h>
#include <pspctrl.h>
#else
#include <stdio.h>
#include <time.h>
#endif

static uint32_t s_pool[32];
static size_t s_pool_idx = 0;
static uint32_t s_counter = 0;

void psp_entropy_feed(uint32_t val) {
    s_pool[s_pool_idx] ^= val;
    s_pool_idx = (s_pool_idx + 1) % 32;
}

void psp_entropy_gather(uint8_t *out_buf, size_t len) {
    if (!out_buf || len == 0) return;

#ifdef __PSP__
    uint64_t rtc_tick = 0;
    sceRtcGetCurrentTick(&rtc_tick);
    psp_entropy_feed((uint32_t)rtc_tick);
    psp_entropy_feed((uint32_t)(rtc_tick >> 32));

    uint32_t sys_time = sceKernelGetSystemTimeLow();
    psp_entropy_feed(sys_time);

    int volt = scePowerGetBatteryVolt();
    psp_entropy_feed((uint32_t)volt);

    SceCtrlData pad;
    sceCtrlPeekBufferPositive(&pad, 1);
    psp_entropy_feed((uint32_t)pad.Buttons);
    psp_entropy_feed(((uint32_t)pad.Lx << 8) | (uint32_t)pad.Ly);
#else
    FILE *f = fopen("/dev/urandom", "rb");
    if (f) {
        size_t rd = fread(out_buf, 1, len, f);
        fclose(f);
        if (rd == len) return;
    }
    psp_entropy_feed((uint32_t)time(NULL));
    psp_entropy_feed((uint32_t)clock());
#endif

    uint8_t digest[32];
    size_t written = 0;

    while (written < len) {
        s_counter++;
        s_pool[0] ^= s_counter;

        sha256_hash((const uint8_t *)s_pool, sizeof(s_pool), digest);

        size_t chunk = (len - written >= 32) ? 32 : (len - written);
        memcpy(out_buf + written, digest, chunk);
        written += chunk;

        /* Re-seed pool with output digest for forward secrecy */
        memcpy(s_pool, digest, 32);
    }

    crypto_zeroize(digest, sizeof(digest));
}
