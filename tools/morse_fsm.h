#ifndef MORSE_FSM_H
#define MORSE_FSM_H

#include <stdbool.h>

typedef enum {
    MORSE_FSM_BUTTON_LEFT = 0,
    MORSE_FSM_BUTTON_RIGHT,
    MORSE_FSM_BUTTON_MIDDLE,
    MORSE_FSM_BUTTON_OTHER
} MorseFsmButton;

typedef enum {
    MORSE_FSM_VALUE_RELEASE = 0,
    MORSE_FSM_VALUE_PRESS = 1,
    MORSE_FSM_VALUE_REPEAT = 2
} MorseFsmValue;

typedef enum {
    MORSE_FSM_ACTION_NONE = 0,
    MORSE_FSM_ACTION_APPEND_DOT,
    MORSE_FSM_ACTION_APPEND_DASH,
    MORSE_FSM_ACTION_COMMIT,
    MORSE_FSM_ACTION_CANCEL,
    MORSE_FSM_ACTION_IGNORE
} MorseFsmAction;

typedef struct {
    bool left_pressed;
    bool right_pressed;
    bool chord_lockout;
} MorseFsm;

void morse_fsm_init(MorseFsm *fsm);
void morse_fsm_reset(MorseFsm *fsm);

/*
 * Called after SYN_DROPPED.
 *
 * Input remains locked until the current button state has been
 * synchronized and both primary buttons are known to be released.
 */
void morse_fsm_resync_begin(MorseFsm *fsm);

/*
 * Synchronize the primary-button state using EVIOCGKEY.
 */
void morse_fsm_sync_buttons(MorseFsm *fsm,
                            bool left_pressed,
                            bool right_pressed);

/*
 * Process one normalized button event.
 *
 * Press   -> value 1
 * Release -> value 0
 * Repeat  -> value 2
 */
MorseFsmAction morse_fsm_handle(MorseFsm *fsm,
                                MorseFsmButton button,
                                int value);

bool morse_fsm_is_locked_out(const MorseFsm *fsm);

#endif
