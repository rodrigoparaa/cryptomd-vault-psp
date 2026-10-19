#include "psp_power.h"
#include <stdio.h>
#include <time.h>

#ifdef __PSP__
#include <pspkernel.h>
#include <psppower.h>
#include <psprtc.h>
#include <psputility.h>
#include <psputility_sysparam.h>
#endif

void psp_power_init(void) {
#ifdef __PSP__
    scePowerSetClockFrequency(333, 333, 166);
#endif
}

void psp_power_tick(void) {
#ifdef __PSP__
    scePowerTick(PSP_POWER_TICK_ALL);
#endif
}

int psp_power_get_battery_percent(void) {
#ifdef __PSP__
    int pct = scePowerGetBatteryLifePercent();
    return (pct < 0) ? 0 : (pct > 100 ? 100 : pct);
#else
    return 100;
#endif
}

int psp_power_is_charging(void) {
#ifdef __PSP__
    return scePowerIsBatteryCharging() ? 1 : 0;
#else
    return 0;
#endif
}

uint64_t psp_power_get_utc_seconds(void) {
#ifdef __PSP__
    uint64_t ticks = 0;
    sceRtcGetCurrentTick(&ticks);
    uint32_t tick_res = sceRtcGetTickResolution();
    if (tick_res == 0) tick_res = 1000000;
    uint64_t sec_since_0001 = ticks / tick_res;
    /* 62135596800 seconds between 0001-01-01 and 1970-01-01 */
    const uint64_t epoch_diff = 62135596800ULL;
    if (sec_since_0001 >= epoch_diff) {
        return sec_since_0001 - epoch_diff;
    }
    return 0;
#else
    return (uint64_t)time(NULL);
#endif
}

void psp_power_format_time(char *out_str, size_t max_len) {
    if (!out_str || max_len < 6) return;

    time_t now = (time_t)psp_power_get_utc_seconds();
    struct tm *tm_info = localtime(&now);
    if (!tm_info) {
        tm_info = gmtime(&now);
    }

    if (tm_info) {
        snprintf(out_str, max_len, "%02d:%02d", tm_info->tm_hour, tm_info->tm_min);
    } else {
        snprintf(out_str, max_len, "12:00");
    }
}
