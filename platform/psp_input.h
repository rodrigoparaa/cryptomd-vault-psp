#ifndef PSP_INPUT_H
#define PSP_INPUT_H

#include "../core/app_state.h"

/* Initializes PSP controller sampling cycle and analog mode */
void psp_input_init(void);

/* Polls PSP hardware controls and updates app->input tracker & analog values */
void psp_input_poll(app_state *app, int8_t *analog_x, int8_t *analog_y);

#endif /* PSP_INPUT_H */
