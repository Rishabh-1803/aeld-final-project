/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MORSE_FSM_CORE_H
#define MORSE_FSM_CORE_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdbool.h>
#endif

enum morse_fsm_core_button {
    MORSE_CORE_BUTTON_LEFT = 0,
    MORSE_CORE_BUTTON_RIGHT,
    MORSE_CORE_BUTTON_MIDDLE,
    MORSE_CORE_BUTTON_OTHER
};

enum morse_fsm_core_action {
    MORSE_CORE_ACTION_NONE = 0,
    MORSE_CORE_ACTION_APPEND_DOT,
    MORSE_CORE_ACTION_APPEND_DASH,
    MORSE_CORE_ACTION_COMMIT,
    MORSE_CORE_ACTION_CANCEL,
    MORSE_CORE_ACTION_IGNORE
};

struct morse_fsm_core {
    bool left_pressed;
    bool right_pressed;
    bool middle_pressed;
    bool chord_lockout;
};

static inline void morse_fsm_core_init(struct morse_fsm_core *fsm)
{
    if (!fsm)
        return;

    fsm->left_pressed = false;
    fsm->right_pressed = false;
    fsm->middle_pressed = false;
    fsm->chord_lockout = false;
}

static inline void morse_fsm_core_reset(struct morse_fsm_core *fsm)
{
    morse_fsm_core_init(fsm);
}

static inline void morse_fsm_core_resync_begin(
    struct morse_fsm_core *fsm)
{
    if (!fsm)
        return;

    /*
     * The event stream is no longer trustworthy.
     * Enter fail-safe lockout until button state is synchronized.
     */
    fsm->left_pressed = false;
    fsm->right_pressed = false;
    fsm->middle_pressed = false;
    fsm->chord_lockout = true;
}

static inline void morse_fsm_core_sync_buttons(
    struct morse_fsm_core *fsm,
    bool left_pressed,
    bool right_pressed,
    bool middle_pressed)
{
    bool was_locked;

    if (!fsm)
        return;

    was_locked = fsm->chord_lockout;

    fsm->left_pressed = left_pressed;
    fsm->right_pressed = right_pressed;
    fsm->middle_pressed = middle_pressed;

    if (was_locked) {
        /*
         * Preserve synchronization lockout until both buttons
         * are confirmed released.
         */
        fsm->chord_lockout = left_pressed || right_pressed;
    } else {
        fsm->chord_lockout = left_pressed && right_pressed;
    }
}

static inline enum morse_fsm_core_action
morse_fsm_core_handle(
    struct morse_fsm_core *fsm,
    enum morse_fsm_core_button button,
    int value)
{
    bool *pressed;
    bool opposite_pressed;
    enum morse_fsm_core_action symbol_action;

    if (!fsm)
        return MORSE_CORE_ACTION_IGNORE;

    if (button == MORSE_CORE_BUTTON_MIDDLE) {
        if (value == 0) {
            fsm->middle_pressed = false;
            return MORSE_CORE_ACTION_NONE;
        }

        if (value == 2 || value != 1)
            return MORSE_CORE_ACTION_IGNORE;

        /*
         * Record the physical press even during chord lockout.
         * This prevents a duplicate down event from committing after
         * the left/right buttons have subsequently been released.
         */
        if (fsm->middle_pressed)
            return MORSE_CORE_ACTION_IGNORE;

        fsm->middle_pressed = true;

        if (fsm->chord_lockout)
            return MORSE_CORE_ACTION_IGNORE;

        return MORSE_CORE_ACTION_COMMIT;
    }

    if (button == MORSE_CORE_BUTTON_LEFT) {
        pressed = &fsm->left_pressed;
        opposite_pressed = fsm->right_pressed;
        symbol_action = MORSE_CORE_ACTION_APPEND_DOT;
    } else if (button == MORSE_CORE_BUTTON_RIGHT) {
        pressed = &fsm->right_pressed;
        opposite_pressed = fsm->left_pressed;
        symbol_action = MORSE_CORE_ACTION_APPEND_DASH;
    } else {
        return MORSE_CORE_ACTION_IGNORE;
    }

    if (value == 0) {
        *pressed = false;

        if (fsm->chord_lockout &&
            !fsm->left_pressed &&
            !fsm->right_pressed)
            fsm->chord_lockout = false;

        return MORSE_CORE_ACTION_NONE;
    }

    if (value == 2)
        return MORSE_CORE_ACTION_IGNORE;

    if (value != 1)
        return MORSE_CORE_ACTION_IGNORE;

    /* Duplicate-down events cannot create additional symbols. */
    if (*pressed)
        return MORSE_CORE_ACTION_IGNORE;

    *pressed = true;

    if (fsm->chord_lockout)
        return MORSE_CORE_ACTION_IGNORE;

    if (opposite_pressed) {
        fsm->chord_lockout = true;
        return MORSE_CORE_ACTION_CANCEL;
    }

    return symbol_action;
}

static inline bool morse_fsm_core_is_locked_out(
    const struct morse_fsm_core *fsm)
{
    return fsm && fsm->chord_lockout;
}

#endif
