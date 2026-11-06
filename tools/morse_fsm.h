#ifndef MORSE_FSM_H
#define MORSE_FSM_H

#include "morse_fsm_core.h"

typedef enum morse_fsm_core_button MorseFsmButton;
typedef enum morse_fsm_core_action MorseFsmAction;
typedef struct morse_fsm_core MorseFsm;

#define MORSE_FSM_BUTTON_LEFT MORSE_CORE_BUTTON_LEFT
#define MORSE_FSM_BUTTON_RIGHT MORSE_CORE_BUTTON_RIGHT
#define MORSE_FSM_BUTTON_MIDDLE MORSE_CORE_BUTTON_MIDDLE
#define MORSE_FSM_BUTTON_OTHER MORSE_CORE_BUTTON_OTHER

#define MORSE_FSM_VALUE_RELEASE 0
#define MORSE_FSM_VALUE_PRESS 1
#define MORSE_FSM_VALUE_REPEAT 2

#define MORSE_FSM_ACTION_NONE MORSE_CORE_ACTION_NONE
#define MORSE_FSM_ACTION_APPEND_DOT MORSE_CORE_ACTION_APPEND_DOT
#define MORSE_FSM_ACTION_APPEND_DASH MORSE_CORE_ACTION_APPEND_DASH
#define MORSE_FSM_ACTION_COMMIT MORSE_CORE_ACTION_COMMIT
#define MORSE_FSM_ACTION_CANCEL MORSE_CORE_ACTION_CANCEL
#define MORSE_FSM_ACTION_IGNORE MORSE_CORE_ACTION_IGNORE

void morse_fsm_init(MorseFsm *fsm);
void morse_fsm_reset(MorseFsm *fsm);

void morse_fsm_resync_begin(MorseFsm *fsm);

void morse_fsm_sync_buttons(
    MorseFsm *fsm,
    bool left_pressed,
    bool right_pressed,
    bool middle_pressed);

MorseFsmAction morse_fsm_handle(
    MorseFsm *fsm,
    MorseFsmButton button,
    int value);

bool morse_fsm_is_locked_out(const MorseFsm *fsm);

#endif
