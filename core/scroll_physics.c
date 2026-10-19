#include "scroll_physics.h"
#include <stdlib.h>
#include <math.h>

void scroll_init(scroll_state *st, float total_doc_height, float viewport_height) {
    if (!st) return;
    st->scroll_y = 0.0f;
    st->velocity_y = 0.0f;
    st->target_y = 0.0f;

    if (total_doc_height > viewport_height) {
        st->max_scroll_y = (total_doc_height - viewport_height) + SCROLL_BOTTOM_CUSHION;
    } else {
        st->max_scroll_y = 0.0f;
    }
}

void scroll_update(scroll_state *st, int8_t analog_y) {
    if (!st) return;

    int abs_y = abs(analog_y);
    if (abs_y > SCROLL_DEADZONE) {
        float norm = (float)(abs_y - SCROLL_DEADZONE) / (float)(128 - SCROLL_DEADZONE);
        if (norm > 1.0f) norm = 1.0f;
        float sgn = (analog_y > 0) ? 1.0f : -1.0f;

        /* Quadratic non-linear acceleration */
        float accel_v = sgn * (14.0f * norm * norm + 2.0f * norm);
        st->velocity_y = accel_v;
    } else {
        /* Exponential decay */
        st->velocity_y *= SCROLL_FRICTION;
        if (fabsf(st->velocity_y) < 0.05f) {
            st->velocity_y = 0.0f;
        }
    }

    st->scroll_y += st->velocity_y;

    /* Clamp boundaries */
    if (st->scroll_y < 0.0f) {
        st->scroll_y = 0.0f;
        st->velocity_y = 0.0f;
    } else if (st->scroll_y > st->max_scroll_y) {
        st->scroll_y = st->max_scroll_y;
        st->velocity_y = 0.0f;
    }
}

void scroll_page_up(scroll_state *st) {
    if (!st) return;
    st->scroll_y -= SCROLL_PAGE_STEP;
    if (st->scroll_y < 0.0f) st->scroll_y = 0.0f;
    st->velocity_y = 0.0f;
}

void scroll_page_down(scroll_state *st) {
    if (!st) return;
    st->scroll_y += SCROLL_PAGE_STEP;
    if (st->scroll_y > st->max_scroll_y) st->scroll_y = st->max_scroll_y;
    st->velocity_y = 0.0f;
}

void scroll_step(scroll_state *st, float delta_y) {
    if (!st) return;
    st->scroll_y += delta_y;
    if (st->scroll_y < 0.0f) st->scroll_y = 0.0f;
    if (st->scroll_y > st->max_scroll_y) st->scroll_y = st->max_scroll_y;
}
