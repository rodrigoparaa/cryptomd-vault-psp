#ifndef SCROLL_PHYSICS_H
#define SCROLL_PHYSICS_H

#include <stdint.h>

#define SCROLL_DEADZONE 22
#define SCROLL_FRICTION 0.88f
#define SCROLL_BOTTOM_CUSHION 90.0f
#define SCROLL_PAGE_STEP 200.0f

typedef struct {
    float scroll_y;
    float velocity_y;
    float target_y;
    float max_scroll_y;
} scroll_state;

void scroll_init(scroll_state *st, float total_doc_height, float viewport_height);
void scroll_update(scroll_state *st, int8_t analog_y);
void scroll_page_up(scroll_state *st);
void scroll_page_down(scroll_state *st);
void scroll_step(scroll_state *st, float delta_y);

#endif /* SCROLL_PHYSICS_H */
