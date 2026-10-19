#ifndef PSP_POWER_H
#define PSP_POWER_H

#include <stdint.h>
#include <stddef.h>

/* Initialize power management, set CPU to 333 MHz and bus to 166 MHz */
void psp_power_init(void);

/* Send keep-awake tick to prevent display sleep during document viewing */
void psp_power_tick(void);

/* Returns battery percentage (0..100) */
int psp_power_get_battery_percent(void);

/* Returns 1 if battery is charging, 0 otherwise */
int psp_power_is_charging(void);

/* Gets current UTC timestamp in seconds */
uint64_t psp_power_get_utc_seconds(void);

/* Formats current local time into HH:MM according to system 12h/24h preference */
void psp_power_format_time(char *out_str, size_t max_len);

#endif /* PSP_POWER_H */
