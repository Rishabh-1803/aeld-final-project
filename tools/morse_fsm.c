#include "morse_fsm.h"

/*
 * Userspace API wrappers.
 *
 * The actual FSM implementation is shared with the kernel driver
 * through the shared morse_fsm_core.h header.
 */

void morse_fsm_init(MorseFsm *fsm)
{
    morse_fsm_core_init(fsm);
}

void morse_fsm_reset(MorseFsm *fsm)
{
    morse_fsm_core_reset(fsm);
}

void morse_fsm_resync_begin(MorseFsm *fsm)
{
    morse_fsm_core_resync_begin(fsm);
}

void morse_fsm_sync_buttons(
    MorseFsm *fsm,
    bool left_pressed,
    bool right_pressed,
    bool middle_pressed)
{
    morse_fsm_core_sync_buttons(
        fsm, left_pressed, right_pressed, middle_pressed);
}

MorseFsmAction morse_fsm_handle(
    MorseFsm *fsm,
    MorseFsmButton button,
    int value)
{
    return morse_fsm_core_handle(fsm, button, value);
}

bool morse_fsm_is_locked_out(const MorseFsm *fsm)
{
    return morse_fsm_core_is_locked_out(fsm);
}
