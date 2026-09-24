// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Non-standalone touch/input excerpt mirrored from:
 *   isomorphismes/wegert/code/wegert.c
 *   commit a2be5077ad562c2ecfa505840428600c05bad90c
 *
 * This is kept locally so the working Android NDK touch path does not have
 * to be rediscovered from another repository.
 */

static void placement_control_centers(
    const struct engine *engine,
    float *zero_x,
    float *pole_x,
    float *center_y
) {
    float radius = placement_control_radius(engine);
    float margin = fmaxf(18.0f, radius * 0.38f);
    *zero_x = margin + radius;
    *pole_x = *zero_x + 2.0f * radius + margin * 0.55f;
    *center_y = (float)engine->height - margin - radius;
}

static bool placement_control_hit(
    const struct engine *engine,
    float x,
    float y,
    enum factor_kind *kind
) {
    if (engine->width <= 0 || engine->height <= 0) {
        return false;
    }

    float radius = placement_control_radius(engine);
    float zero_center_x = 0.0f;
    float pole_center_x = 0.0f;
    float center_y = 0.0f;
    placement_control_centers(
        engine,
        &zero_center_x,
        &pole_center_x,
        &center_y
    );

    if (hypotf(x - zero_center_x, y - center_y) <= radius) {
        *kind = FACTOR_ZERO;
        return true;
    }
    if (hypotf(x - pole_center_x, y - center_y) <= radius) {
        *kind = FACTOR_POLE;
        return true;
    }
    return false;
}

static float pointer_distance(const AInputEvent *event) {
    float dx = AMotionEvent_getX(event, 0) - AMotionEvent_getX(event, 1);
    float dy = AMotionEvent_getY(event, 0) - AMotionEvent_getY(event, 1);
    return hypotf(dx, dy);
}

static void pointer_midpoint(const AInputEvent *event, float *x, float *y) {
    *x = 0.5f * (AMotionEvent_getX(event, 0) + AMotionEvent_getX(event, 1));
    *y = 0.5f * (AMotionEvent_getY(event, 0) + AMotionEvent_getY(event, 1));
}

static int32_t handle_input(struct android_app *app, AInputEvent *event) {
    struct engine *engine = app->userData;
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) {
        return 0;
    }

    int32_t action = AMotionEvent_getAction(event);
    int32_t masked_action = action & AMOTION_EVENT_ACTION_MASK;
    size_t pointer_count = AMotionEvent_getPointerCount(event);

    switch (masked_action) {
        case AMOTION_EVENT_ACTION_DOWN: {
            float x = AMotionEvent_getX(event, 0);
            float y = AMotionEvent_getY(event, 0);
            enum factor_kind selected_kind = FACTOR_ZERO;
            if (placement_control_hit(engine, x, y, &selected_kind)) {
                engine->placement_kind = selected_kind;
                engine->gesture = GESTURE_BLOCKED;
                engine->moved = false;
                engine->dirty = true;
                return 1;
            }
            if (clear_button_contains(engine, x, y)) {
                engine->gesture = GESTURE_CLEAR_BUTTON;
                engine->moved = false;
                return 1;
            }
            if (polynomial_overlay_contains(engine, x, y)) {
                engine->gesture = GESTURE_BLOCKED;
                engine->moved = false;
                return 1;
            }
            engine->moved = false;
            engine->down_x = x;
            engine->down_y = y;
            engine->last_x = engine->down_x;
            engine->last_y = engine->down_y;
            struct factor_target target = factor_target_at(engine, x, y);
            if (target.found) {
                capture_factor(engine, &target);
                engine->gesture = GESTURE_FACTOR;
            } else {
                engine->gesture = GESTURE_SINGLE;
            }
            return 1;
        }

        case AMOTION_EVENT_ACTION_POINTER_DOWN: {
            if (gesture_is_ui_hold(engine->gesture)) {
                engine->gesture = GESTURE_BLOCKED;
                engine->moved = false;
                return 1;
            }
            if (gesture_pointer_down_resets(engine->gesture, (int)pointer_count)) {
                reset_all(engine);
                engine->gesture = GESTURE_BLOCKED;
                return 1;
            }
            if (gesture_pointer_down_starts_pinch(engine->gesture, (int)pointer_count)) {
                engine->gesture = GESTURE_PINCH;
                engine->moved = false;
                engine->pinch_last_distance = pointer_distance(event);
                pointer_midpoint(event, &engine->pinch_last_mid_x, &engine->pinch_last_mid_y);
                return 1;
            }
            return 0;
        }

        case AMOTION_EVENT_ACTION_MOVE: {
            if (engine->gesture == GESTURE_FACTOR && pointer_count == 1) {
                float x = AMotionEvent_getX(event, 0);
                float y = AMotionEvent_getY(event, 0);
                float from_down_x = x - engine->down_x;
                float from_down_y = y - engine->down_y;
                if (drag_threshold_exceeded(from_down_x, from_down_y)) {
                    engine->moved = true;
                }
                if (engine->moved) {
                    move_captured_factor(engine, x, y);
                }
                return 1;
            }

            if (engine->gesture == GESTURE_SINGLE && pointer_count == 1) {
                float x = AMotionEvent_getX(event, 0);
                float y = AMotionEvent_getY(event, 0);
                float from_down_x = x - engine->down_x;
                float from_down_y = y - engine->down_y;
                if (drag_threshold_exceeded(from_down_x, from_down_y)) {
                    engine->moved = true;
                }
                if (engine->moved) {
                    pan_by_pixels(engine, x - engine->last_x, y - engine->last_y);
                }
                engine->last_x = x;
                engine->last_y = y;
                return 1;
            }

            if (engine->gesture == GESTURE_PINCH && pointer_count >= 2) {
                float midpoint_x = 0.0f;
                float midpoint_y = 0.0f;
                pointer_midpoint(event, &midpoint_x, &midpoint_y);
                float distance = pointer_distance(event);

                pan_by_pixels(
                    engine,
                    midpoint_x - engine->pinch_last_mid_x,
                    midpoint_y - engine->pinch_last_mid_y
                );

                if (gesture_apply_pinch_zoom(
                    engine->pinch_last_distance,
                    distance,
                    &engine->half_height
                )) {
                    engine->dirty = true;
                }

                engine->pinch_last_distance = distance;
                engine->pinch_last_mid_x = midpoint_x;
                engine->pinch_last_mid_y = midpoint_y;
                return 1;
            }
            return gesture_is_ui_hold(engine->gesture) ? 1 : 0;
        }

        case AMOTION_EVENT_ACTION_POINTER_UP: {
            if (engine->gesture == GESTURE_PINCH && pointer_count == 2) {
                engine->gesture = GESTURE_BLOCKED;
                return 1;
            }
            return gesture_is_ui_hold(engine->gesture) ? 1 : 0;
        }

        case AMOTION_EVENT_ACTION_UP: {
#ifndef NDEBUG
            if (engine->gesture == GESTURE_FACTOR && engine->moved) {
                LOGI(
                    "factor drag completed: kind=%s index=%d",
                    engine->captured_factor_kind == FACTOR_POLE ? "pole" : "zero",
                    engine->captured_factor_index
                );
            }
#endif
            if (engine->gesture == GESTURE_SINGLE && !engine->moved) {
                float x = AMotionEvent_getX(event, 0);
                float y = AMotionEvent_getY(event, 0);
                if (engine->placement_kind == FACTOR_POLE) {
                    add_pole(engine, x, y);
                } else {
                    add_zero(engine, x, y);
                }
            } else if (
                engine->gesture == GESTURE_CLEAR_BUTTON &&
                clear_button_contains(
                    engine,
                    AMotionEvent_getX(event, 0),
                    AMotionEvent_getY(event, 0)
                )
            ) {
                clear_function(engine);
                LOGI("factors cleared");
            }
            engine->gesture = GESTURE_NONE;
            engine->moved = false;
            return 1;
        }

        case AMOTION_EVENT_ACTION_CANCEL:
            engine->gesture = GESTURE_NONE;
            engine->moved = false;
            return 1;

        default:
            return 0;
    }
}

static void handle_command(struct android_app *app, int32_t command) {
    struct engine *engine = app->userData;

    switch (command) {
        case APP_CMD_INIT_WINDOW:
            if (app->window != NULL && engine->display == EGL_NO_DISPLAY) {
                initialize_display(engine);
            }
            break;

        case APP_CMD_TERM_WINDOW:
            terminate_display(engine);
            break;

        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_CONTENT_RECT_CHANGED:
        case APP_CMD_CONFIG_CHANGED:
            update_surface_size(engine);
            break;

        case APP_CMD_GAINED_FOCUS:
            engine->dirty = true;
            break;

        default:
            break;
    }
}

void android_main(struct android_app *app) {
    struct engine engine = {
        .app = app,
        .display = EGL_NO_DISPLAY,
        .surface = EGL_NO_SURFACE,
        .context = EGL_NO_CONTEXT,
        .gesture = GESTURE_NONE,
        .dirty = true,
        .logged_first_frame = false
    };
    initialize_function(&engine);

    app->userData = &engine;
    app->onAppCmd = handle_command;
    app->onInputEvent = handle_input;

    while (true) {
        int events = 0;
        struct android_poll_source *source = NULL;
        int timeout = engine.dirty && engine.display != EGL_NO_DISPLAY ? 0 : -1;
        int ident = ALooper_pollOnce(timeout, NULL, &events, (void **)&source);

        if (ident >= 0 && source != NULL) {
            source->process(app, source);
        }

        if (app->destroyRequested != 0) {
            terminate_display(&engine);
            return;
        }

        if (engine.dirty) {
