#include "input_logic.h"
#include "crypto_primitives.h"
#include <string.h>

void input_tracker_init(input_tracker *tr) {
    if (!tr) return;
    tr->current_buttons = 0;
    tr->previous_buttons = 0;
    tr->pressed_buttons = 0;
    tr->released_buttons = 0;
    tr->lr_hold_frames = 0;
    tr->inactivity_frames = 0;
    tr->panic_triggered = 0;
    tr->xray_active = 0;
}

void input_tracker_update(input_tracker *tr, uint32_t raw_buttons) {
    if (!tr) return;
    tr->previous_buttons = tr->current_buttons;
    tr->current_buttons = raw_buttons;
    tr->pressed_buttons = (tr->current_buttons ^ tr->previous_buttons) & tr->current_buttons;
    tr->released_buttons = (tr->current_buttons ^ tr->previous_buttons) & tr->previous_buttons;

    /* Inactivity counter */
    if (tr->current_buttons != 0) {
        tr->inactivity_frames = 0;
    } else {
        tr->inactivity_frames++;
    }

    /* L + R Chord detector */
    uint32_t lr_mask = BTN_LTRIGGER | BTN_RTRIGGER;
    if ((tr->current_buttons & lr_mask) == lr_mask) {
        tr->lr_hold_frames++;
        if (tr->lr_hold_frames >= PANIC_CHORD_FRAMES) {
            tr->panic_triggered = 1;
        }
    } else {
        tr->lr_hold_frames = 0;
        tr->panic_triggered = 0;
    }

    /* X-Ray Mode: SELECT held down */
    tr->xray_active = (tr->current_buttons & BTN_SELECT) ? 1 : 0;
}

void input_reset_inactivity(input_tracker *tr) {
    if (!tr) return;
    tr->inactivity_frames = 0;
    tr->lr_hold_frames = 0;
    tr->panic_triggered = 0;
}

uint8_t input_mask_to_code(uint32_t mask) {
    if (mask & BTN_UP)       return BTN_CODE_UP;
    if (mask & BTN_DOWN)     return BTN_CODE_DOWN;
    if (mask & BTN_LEFT)     return BTN_CODE_LEFT;
    if (mask & BTN_RIGHT)    return BTN_CODE_RIGHT;
    if (mask & BTN_LTRIGGER) return BTN_CODE_L;
    if (mask & BTN_RTRIGGER) return BTN_CODE_R;
    if (mask & BTN_CROSS)    return BTN_CODE_CROSS;
    if (mask & BTN_SQUARE)   return BTN_CODE_SQUARE;
    if (mask & BTN_TRIANGLE) return BTN_CODE_TRIANGLE;
    return BTN_CODE_NONE;
}

const char *input_code_to_name(uint8_t code) {
    switch (code) {
        case BTN_CODE_UP:       return "ARRIBA";
        case BTN_CODE_DOWN:     return "ABAJO";
        case BTN_CODE_LEFT:     return "IZQUIERDA";
        case BTN_CODE_RIGHT:    return "DERECHA";
        case BTN_CODE_L:        return "GATILLO L";
        case BTN_CODE_R:        return "GATILLO R";
        case BTN_CODE_CROSS:    return "CRUZ (X)";
        case BTN_CODE_SQUARE:   return "CUADRADO";
        case BTN_CODE_TRIANGLE: return "TRIANGULO";
        default: return "";
    }
}

void pin_input_init(pin_input_state *pin, size_t max_len) {
    if (!pin) return;
    pin->count = 0;
    pin->max_len = (max_len > MAX_PIN_LEN) ? MAX_PIN_LEN : (max_len < 2 ? 4 : max_len);
    memset(pin->buttons, 0, sizeof(pin->buttons));
}

int pin_input_add(pin_input_state *pin, uint8_t btn_id) {
    if (!pin) return 0;
    if (pin->count < pin->max_len) {
        pin->buttons[pin->count++] = btn_id;
        return 1;
    }
    return 0;
}

int pin_input_backspace(pin_input_state *pin) {
    if (!pin) return 0;
    if (pin->count > 0) {
        pin->buttons[--pin->count] = 0;
        return 1;
    }
    return 0;
}

void pin_input_clear(pin_input_state *pin) {
    if (!pin) return;
    crypto_zeroize(pin->buttons, sizeof(pin->buttons));
    pin->count = 0;
}

void drum_input_init(drum_input_state *drum, size_t default_len) {
    if (!drum) return;
    if (default_len < 2) default_len = 4;
    if (default_len > MAX_PIN_LEN) default_len = MAX_PIN_LEN;
    drum->length = default_len;
    drum->cursor = 0;
    for (size_t i = 0; i < drum->length; i++) {
        drum->word[i] = 'A';
    }
    drum->word[drum->length] = '\0';
}

void drum_input_cycle_char(drum_input_state *drum, int delta) {
    if (!drum || drum->cursor >= drum->length) return;
    char c = drum->word[drum->cursor];
    if (c < 'A' || c > 'Z') c = 'A';
    int val = (c - 'A') + delta;
    while (val < 0) val += 26;
    val %= 26;
    drum->word[drum->cursor] = (char)('A' + val);
}

void drum_input_move_cursor(drum_input_state *drum, int delta) {
    if (!drum || drum->length == 0) return;
    int cur = (int)drum->cursor + delta;
    if (cur < 0) cur = 0;
    if (cur >= (int)drum->length) cur = (int)drum->length - 1;
    drum->cursor = (size_t)cur;
}
