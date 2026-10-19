#ifndef INPUT_LOGIC_H
#define INPUT_LOGIC_H

#include <stdint.h>
#include <stddef.h>

#define MAX_PIN_LEN 16
#define PANIC_CHORD_FRAMES 60   /* 1 second at 60 FPS */
#define AUTO_LOCK_FRAMES 7200   /* 120 seconds at 60 FPS */

/* PSP Virtual Button Bitmasks (compatible with standard SCE bitmasks) */
#define BTN_SELECT   (1 << 0)
#define BTN_START    (1 << 3)
#define BTN_UP       (1 << 4)
#define BTN_RIGHT    (1 << 5)
#define BTN_DOWN     (1 << 6)
#define BTN_LEFT     (1 << 7)
#define BTN_LTRIGGER (1 << 8)
#define BTN_RTRIGGER (1 << 9)
#define BTN_TRIANGLE (1 << 12)
#define BTN_CIRCLE   (1 << 13)
#define BTN_CROSS    (1 << 14)
#define BTN_SQUARE   (1 << 15)

typedef enum {
    BTN_CODE_NONE     = 0,
    BTN_CODE_UP       = 1,
    BTN_CODE_DOWN     = 2,
    BTN_CODE_LEFT     = 3,
    BTN_CODE_RIGHT    = 4,
    BTN_CODE_L        = 5,
    BTN_CODE_R        = 6,
    BTN_CODE_CROSS    = 7,
    BTN_CODE_SQUARE   = 8,
    BTN_CODE_TRIANGLE = 9
} psp_btn_code;

uint8_t input_mask_to_code(uint32_t mask);
const char *input_code_to_name(uint8_t code);

typedef struct {
    uint8_t buttons[MAX_PIN_LEN];
    size_t count;
    size_t max_len;
} pin_input_state;

typedef struct {
    char word[MAX_PIN_LEN + 1];
    size_t cursor;
    size_t length;
} drum_input_state;

typedef struct {
    uint32_t current_buttons;
    uint32_t previous_buttons;
    uint32_t pressed_buttons;  /* Edge detection: 0 -> 1 */
    uint32_t released_buttons; /* Edge detection: 1 -> 0 */

    uint32_t lr_hold_frames;
    uint32_t inactivity_frames;
    int panic_triggered;

    int xray_active;           /* 1 if SELECT is currently held down */
} input_tracker;

void input_tracker_init(input_tracker *tr);
void input_tracker_update(input_tracker *tr, uint32_t raw_buttons);
void input_reset_inactivity(input_tracker *tr);

void pin_input_init(pin_input_state *pin, size_t max_len);
int pin_input_add(pin_input_state *pin, uint8_t btn_id);
int pin_input_backspace(pin_input_state *pin);
void pin_input_clear(pin_input_state *pin);

void drum_input_init(drum_input_state *drum, size_t default_len);
void drum_input_cycle_char(drum_input_state *drum, int delta);
void drum_input_move_cursor(drum_input_state *drum, int delta);

#endif /* INPUT_LOGIC_H */
