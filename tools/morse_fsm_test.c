#include <stdio.h>

#include "morse_fsm.h"

static unsigned int checks;
static unsigned int failures;

#define CHECK_ACTION(label, expression, expected)                  \
    do {                                                           \
        MorseFsmAction actual = (expression);                      \
        checks++;                                                  \
        if (actual != (expected)) {                                \
            fprintf(stderr,                                        \
                    "FAIL: %s (expected action %d, got %d)\n",     \
                    label, (int)(expected), (int)actual);          \
            failures++;                                            \
        }                                                          \
    } while (0)

#define CHECK_STATE(label, expression, expected)                   \
    do {                                                           \
        bool actual = (expression);                                \
        checks++;                                                  \
        if (actual != (expected)) {                                \
            fprintf(stderr,                                        \
                    "FAIL: %s (expected %d, got %d)\n",            \
                    label, (int)(expected), (int)actual);          \
            failures++;                                            \
        }                                                          \
    } while (0)

static void test_basic_button_events(void)
{
    MorseFsm fsm;

    morse_fsm_init(&fsm);

    CHECK_ACTION("left press produces DOT",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 1),
        MORSE_FSM_ACTION_APPEND_DOT);

    CHECK_ACTION("duplicate left press ignored",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 1),
        MORSE_FSM_ACTION_IGNORE);

    CHECK_ACTION("left repeat ignored",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 2),
        MORSE_FSM_ACTION_IGNORE);

    CHECK_ACTION("left release adds no symbol",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 0),
        MORSE_FSM_ACTION_NONE);

    CHECK_ACTION("new left press after release produces DOT",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 1),
        MORSE_FSM_ACTION_APPEND_DOT);

    CHECK_ACTION("left release after second press",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 0),
        MORSE_FSM_ACTION_NONE);

    CHECK_ACTION("right press produces DASH",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_RIGHT, 1),
        MORSE_FSM_ACTION_APPEND_DASH);

    CHECK_ACTION("right repeat ignored",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_RIGHT, 2),
        MORSE_FSM_ACTION_IGNORE);

    CHECK_ACTION("right release adds no symbol",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_RIGHT, 0),
        MORSE_FSM_ACTION_NONE);
}

static void test_middle_commit(void)
{
    MorseFsm fsm;

    morse_fsm_init(&fsm);

    CHECK_ACTION("middle press requests COMMIT",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_MIDDLE, 1),
        MORSE_FSM_ACTION_COMMIT);

    CHECK_ACTION("middle release does not COMMIT again",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_MIDDLE, 0),
        MORSE_FSM_ACTION_NONE);

    CHECK_ACTION("middle repeat ignored",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_MIDDLE, 2),
        MORSE_FSM_ACTION_IGNORE);
}

static void test_chord_lockout_and_recovery(void)
{
    MorseFsm fsm;

    morse_fsm_init(&fsm);

    CHECK_ACTION("initial left press",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 1),
        MORSE_FSM_ACTION_APPEND_DOT);

    CHECK_ACTION("overlapping right press cancels sequence",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_RIGHT, 1),
        MORSE_FSM_ACTION_CANCEL);

    CHECK_STATE("chord lockout activated",
        morse_fsm_is_locked_out(&fsm), true);

    CHECK_ACTION("middle COMMIT ignored during lockout",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_MIDDLE, 1),
        MORSE_FSM_ACTION_IGNORE);

    CHECK_ACTION("left release during lockout",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 0),
        MORSE_FSM_ACTION_NONE);

    CHECK_STATE("lockout remains while right is held",
        morse_fsm_is_locked_out(&fsm), true);

    CHECK_ACTION("left press ignored while right remains held",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 1),
        MORSE_FSM_ACTION_IGNORE);

    CHECK_ACTION("left release while right remains held",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 0),
        MORSE_FSM_ACTION_NONE);

    CHECK_ACTION("right release ends lockout",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_RIGHT, 0),
        MORSE_FSM_ACTION_NONE);

    CHECK_STATE("lockout cleared after both releases",
        morse_fsm_is_locked_out(&fsm), false);

    CHECK_ACTION("input accepted after lockout recovery",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 1),
        MORSE_FSM_ACTION_APPEND_DOT);

    CHECK_ACTION("release after recovery",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 0),
        MORSE_FSM_ACTION_NONE);
}

static void test_sync_loss_recovery(void)
{
    MorseFsm fsm;

    morse_fsm_init(&fsm);

    (void)morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 1);

    morse_fsm_resync_begin(&fsm);

    CHECK_STATE("sync loss enters fail-safe lockout",
        morse_fsm_is_locked_out(&fsm), true);

    morse_fsm_sync_buttons(&fsm, true, false);

    CHECK_STATE("one held button preserves lockout",
        morse_fsm_is_locked_out(&fsm), true);

    CHECK_ACTION("release after synchronization",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 0),
        MORSE_FSM_ACTION_NONE);

    CHECK_STATE("release clears synchronization lockout",
        morse_fsm_is_locked_out(&fsm), false);

    CHECK_ACTION("input accepted after synchronization",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_RIGHT, 1),
        MORSE_FSM_ACTION_APPEND_DASH);

    morse_fsm_reset(&fsm);
    morse_fsm_resync_begin(&fsm);
    morse_fsm_sync_buttons(&fsm, false, false);

    CHECK_STATE("no held buttons clears synchronization lockout",
        morse_fsm_is_locked_out(&fsm), false);

    morse_fsm_resync_begin(&fsm);
    morse_fsm_sync_buttons(&fsm, true, true);

    CHECK_STATE("two held buttons preserve lockout",
        morse_fsm_is_locked_out(&fsm), true);

    morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 0);

    CHECK_STATE("one remaining held button keeps lockout",
        morse_fsm_is_locked_out(&fsm), true);

    morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_RIGHT, 0);

    CHECK_STATE("both releases clear lockout",
        morse_fsm_is_locked_out(&fsm), false);
}

static void test_other_inputs_and_reset(void)
{
    MorseFsm fsm;

    morse_fsm_init(&fsm);

    CHECK_ACTION("unknown button ignored",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_OTHER, 1),
        MORSE_FSM_ACTION_IGNORE);

    CHECK_ACTION("invalid event value ignored",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 7),
        MORSE_FSM_ACTION_IGNORE);

    (void)morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 1);
    morse_fsm_reset(&fsm);

    CHECK_STATE("reset clears lockout",
        morse_fsm_is_locked_out(&fsm), false);

    CHECK_ACTION("reset allows new DOT",
        morse_fsm_handle(&fsm, MORSE_FSM_BUTTON_LEFT, 1),
        MORSE_FSM_ACTION_APPEND_DOT);
}

int main(void)
{
    test_basic_button_events();
    test_middle_commit();
    test_chord_lockout_and_recovery();
    test_sync_loss_recovery();
    test_other_inputs_and_reset();

    if (failures != 0) {
        fprintf(stderr, "FAIL: %u of %u FSM checks failed.\n",
                failures, checks);
        return 1;
    }

    printf("PASS: all %u FSM checks passed.\n", checks);
    return 0;
}
