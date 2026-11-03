#include "morse_keymap.h"

#include <linux/input-event-codes.h>
#include <stdio.h>
#include <stddef.h>

typedef struct {
    unsigned char ascii;
    uint16_t keycode;
    bool shift;
} ExpectedMapping;

static const uint16_t expected_letters[26] = {
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G,
    KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N,
    KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U,
    KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z
};

static const uint16_t expected_digits[10] = {
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
    KEY_5, KEY_6, KEY_7, KEY_8, KEY_9
};

static const ExpectedMapping expected_punctuation[] = {
    {' ', KEY_SPACE, false},
    {'!', KEY_1, true},
    {'"', KEY_APOSTROPHE, true},
    {'#', KEY_3, true},
    {'$', KEY_4, true},
    {'%', KEY_5, true},
    {'&', KEY_7, true},
    {'\'', KEY_APOSTROPHE, false},
    {'(', KEY_9, true},
    {')', KEY_0, true},
    {'*', KEY_8, true},
    {'+', KEY_EQUAL, true},
    {',', KEY_COMMA, false},
    {'-', KEY_MINUS, false},
    {'.', KEY_DOT, false},
    {'/', KEY_SLASH, false},

    {':', KEY_SEMICOLON, true},
    {';', KEY_SEMICOLON, false},
    {'<', KEY_COMMA, true},
    {'=', KEY_EQUAL, false},
    {'>', KEY_DOT, true},
    {'?', KEY_SLASH, true},
    {'@', KEY_2, true},

    {'[', KEY_LEFTBRACE, false},
    {'\\', KEY_BACKSLASH, false},
    {']', KEY_RIGHTBRACE, false},
    {'^', KEY_6, true},
    {'_', KEY_MINUS, true},
    {'`', KEY_GRAVE, false},

    {'{', KEY_LEFTBRACE, true},
    {'|', KEY_BACKSLASH, true},
    {'}', KEY_RIGHTBRACE, true},
    {'~', KEY_GRAVE, true}
};

static bool check_one(unsigned char ascii,
                      uint16_t expected_keycode,
                      bool expected_shift)
{
    MorseKeyStroke actual;

    if (!morse_ascii_to_keystroke(ascii, &actual)) {
        fprintf(stderr, "FAIL: ASCII %u was rejected.\n",
                (unsigned int)ascii);
        return false;
    }

    if (actual.keycode != expected_keycode ||
        actual.shift != expected_shift) {
        fprintf(stderr,
                "FAIL: ASCII %u: expected keycode=%u shift=%d, "
                "received keycode=%u shift=%d.\n",
                (unsigned int)ascii,
                (unsigned int)expected_keycode,
                expected_shift ? 1 : 0,
                (unsigned int)actual.keycode,
                actual.shift ? 1 : 0);
        return false;
    }

    return true;
}

static bool test_invalid_inputs(void)
{
    static const unsigned char invalid_ascii[] = {
        0, 1, 8, 9, 10, 13, 27, 31, 127, 128, 255
    };

    MorseKeyStroke stroke;
    size_t i;

    for (i = 0; i < sizeof(invalid_ascii) / sizeof(invalid_ascii[0]); i++) {
        stroke.keycode = 123;
        stroke.shift = true;

        if (morse_ascii_to_keystroke(invalid_ascii[i], &stroke)) {
            fprintf(stderr, "FAIL: invalid ASCII %u was accepted.\n",
                    (unsigned int)invalid_ascii[i]);
            return false;
        }

        if (stroke.keycode != 0 || stroke.shift) {
            fprintf(stderr,
                    "FAIL: invalid ASCII %u did not clear the output.\n",
                    (unsigned int)invalid_ascii[i]);
            return false;
        }
    }

    if (morse_ascii_to_keystroke('A', NULL)) {
        fputs("FAIL: NULL output pointer was accepted.\n", stderr);
        return false;
    }

    puts("PASS: invalid ASCII values and NULL output are rejected.");
    return true;
}

int main(void)
{
    size_t i;
    size_t tested = 0;
    size_t punctuation_count =
        sizeof(expected_punctuation) / sizeof(expected_punctuation[0]);

    /* Lowercase and uppercase letters. */
    for (i = 0; i < 26; i++) {
        if (!check_one((unsigned char)('a' + i),
                       expected_letters[i], false))
            return 1;
        tested++;

        if (!check_one((unsigned char)('A' + i),
                       expected_letters[i], true))
            return 1;
        tested++;
    }

    /* Digits. */
    for (i = 0; i < 10; i++) {
        if (!check_one((unsigned char)('0' + i),
                       expected_digits[i], false))
            return 1;
        tested++;
    }

    /* Space and punctuation. */
    for (i = 0; i < punctuation_count; i++) {
        if (!check_one(expected_punctuation[i].ascii,
                       expected_punctuation[i].keycode,
                       expected_punctuation[i].shift))
            return 1;
        tested++;
    }

    if (tested != 95) {
        fprintf(stderr, "FAIL: expected 95 tests, ran %zu.\n", tested);
        return 1;
    }

    printf("PASS: all %zu printable ASCII characters map correctly.\n",
           tested);

    if (!test_invalid_inputs())
        return 1;

    return 0;
}
