#include <stddef.h>

#include "morse_fsm.h"

void morse_fsm_init(MorseFsm *fsm)
{
    if (fsm == NULL)
        return;

    fsm->left_pressed = false;
    fsm->right_pressed = false;
    fsm->chord_lockout = false;
}

void morse_fsm_reset(MorseFsm *fsm)
{
    morse_fsm_init(fsm);
}

void morse_fsm_resync_begin(MorseFsm *fsm)
{
    if (fsm == NULL)
        return;

    /*
     * The event stream is no longer trustworthy.
     * Fail safely until the actual button state is queried.
     */
    fsm->left_pressed = false;
    fsm->right_pressed = false;
    fsm->chord_lockout = true;
}

void morse_fsm_sync_buttons(MorseFsm *fsm,
                            bool left_pressed,
                            bool right_pressed)
{
    bool was_locked;

    if (fsm == NULL)
        return;

    was_locked = fsm->chord_lockout;

    fsm->left_pressed = left_pressed;
    fsm->right_pressed = right_pressed;

    if (was_locked) {
        /*
         * Preserve an existing lockout until both buttons
         * are confirmed released.
         */
        fsm->chord_lockout = left_pressed || right_pressed;
    } else {
        /*
         * If synchronization discovers an already-active
         * two-button chord, enter lockout.
         */
        fsm->chord_lockout = left_pressed && right_pressed;
    }
}

MorseFsmAction morse_fsm_handle(MorseFsm *fsm,
                                MorseFsmButton button,
                                int value)
{
    bool *pressed;
    bool opposite_pressed;
    MorseFsmAction symbol_action;

    if (fsm == NULL)
        return MORSE_FSM_ACTION_IGNORE;

    if (button == MORSE_FSM_BUTTON_MIDDLE) {
        if (value == MORSE_FSM_VALUE_RELEASE)
            return MORSE_FSM_ACTION_NONE;

        if (value != MORSE_FSM_VALUE_PRESS)
            return MORSE_FSM_ACTION_IGNORE;

        if (fsm->chord_lockout)
            return MORSE_FSM_ACTION_IGNORE;

        return MORSE_FSM_ACTION_COMMIT;
    }

    if (button == MORSE_FSM_BUTTON_LEFT) {
        pressed = &fsm->left_pressed;
        opposite_pressed = fsm->right_pressed;
        symbol_action = MORSE_FSM_ACTION_APPEND_DOT;
    } else if (button == MORSE_FSM_BUTTON_RIGHT) {
        pressed = &fsm->right_pressed;
        opposite_pressed = fsm->left_pressed;
        symbol_action = MORSE_FSM_ACTION_APPEND_DASH;
    } else {
        return MORSE_FSM_ACTION_IGNORE;
    }

    if (value == MORSE_FSM_VALUE_RELEASE) {
        *pressed = false;

        if (fsm->chord_lockout &&
            !fsm->left_pressed &&
            !fsm->right_pressed) {
            fsm->chord_lockout = false;
        }

        return MORSE_FSM_ACTION_NONE;
    }

    if (value == MORSE_FSM_VALUE_REPEAT)
        return MORSE_FSM_ACTION_IGNORE;

    if (value != MORSE_FSM_VALUE_PRESS)
        return MORSE_FSM_ACTION_IGNORE;

    /*
     * A duplicate down event must not generate another symbol.
     */
    if (*pressed)
        return MORSE_FSM_ACTION_IGNORE;

    *pressed = true;

    /*
     * Once locked out, all input remains ignored until both
     * primary buttons have been released.
     */
    if (fsm->chord_lockout)
        return MORSE_FSM_ACTION_IGNORE;

    /*
     * The opposite button is already down:
     * this is an overlapping left/right chord.
     */
    if (opposite_pressed) {
        fsm->chord_lockout = true;
        return MORSE_FSM_ACTION_CANCEL;
    }

    return symbol_action;
}

bool morse_fsm_is_locked_out(const MorseFsm *fsm)
{
    return fsm != NULL && fsm->chord_lockout;
}
