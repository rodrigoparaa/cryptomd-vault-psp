#include "psp_input.h"
#include "psp_entropy.h"

#ifdef __PSP__
#include <pspctrl.h>
#endif

void psp_input_init(void) {
#ifdef __PSP__
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
#endif
}

void psp_input_poll(app_state *app, int8_t *analog_x, int8_t *analog_y) {
    if (!app) return;

#ifdef __PSP__
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(&pad, 1);

    if (analog_x) *analog_x = (int8_t)((int)pad.Lx - 128);
    if (analog_y) *analog_y = (int8_t)((int)pad.Ly - 128);

    input_tracker_update(&app->input, pad.Buttons);

    if (pad.Buttons != 0 || pad.Lx != 128 || pad.Ly != 128) {
        psp_entropy_feed((uint32_t)pad.Buttons ^ ((uint32_t)pad.Lx << 16) ^ (uint32_t)pad.Ly);
    }
#else
    if (analog_x) *analog_x = 0;
    if (analog_y) *analog_y = 0;
    input_tracker_update(&app->input, 0);
#endif
}
