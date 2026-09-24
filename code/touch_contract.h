// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BYRNES_EUCLID_TOUCH_CONTRACT_H
#define BYRNES_EUCLID_TOUCH_CONTRACT_H

#include <stdbool.h>

#define TOUCH_TARGET_RADIUS_DP 24.0f
#define TOUCH_DRAG_THRESHOLD_PIXELS 12.0f

enum touch_capture_kind {
    TOUCH_CAPTURE_NONE,
    TOUCH_CAPTURE_OBJECT,
    TOUCH_CAPTURE_VIEW,
    TOUCH_CAPTURE_PINCH,
    TOUCH_CAPTURE_BLOCKED
};

struct touch_point {
    int id;
    float x;
    float y;
};

struct touch_capture {
    enum touch_capture_kind kind;
    int object_id;
    float down_x;
    float down_y;
    float last_x;
    float last_y;
    bool moved;
};

static inline float touch_target_radius_pixels(int density_dpi) {
    bool has_concrete_density =
        density_dpi > 0 && density_dpi != 0xfffe && density_dpi != 0xffff;
    float scale = has_concrete_density ? (float)density_dpi / 160.0f : 1.0f;
    return TOUCH_TARGET_RADIUS_DP * scale;
}

static inline bool touch_drag_threshold_exceeded(float delta_x, float delta_y) {
    return delta_x * delta_x + delta_y * delta_y >
        TOUCH_DRAG_THRESHOLD_PIXELS * TOUCH_DRAG_THRESHOLD_PIXELS;
}

#endif
