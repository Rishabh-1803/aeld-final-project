/* SPDX-License-Identifier: MIT */
#ifndef MORSE_KEYMAP_SHARED_H
#define MORSE_KEYMAP_SHARED_H

#include <linux/input-event-codes.h>

/*
 * Shared MorseMouse US QWERTY mapping.
 *
 * Used by both the userspace reference decoder and the kernel
 * virtual keyboard. Outputs are cleared before validation.
 *
 * Returns 1 on success, 0 for unsupported input.
 */
static inline int morse_shared_ascii_to_key(
    unsigned char ascii,
    unsigned short *keycode,
    unsigned char *shift)
{
    static const unsigned short letter_keys[26] = {
        KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G,
        KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N,
        KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U,
        KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z
    };

    static const unsigned short digit_keys[10] = {
        KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
        KEY_5, KEY_6, KEY_7, KEY_8, KEY_9
    };

    if (!keycode || !shift)
        return 0;

    *keycode = 0;
    *shift = 0;

    if (ascii < 32 || ascii > 126)
        return 0;

    if (ascii >= 'a' && ascii <= 'z') {
        *keycode = letter_keys[ascii - 'a'];
        return 1;
    }

    if (ascii >= 'A' && ascii <= 'Z') {
        *keycode = letter_keys[ascii - 'A'];
        *shift = 1;
        return 1;
    }

    if (ascii >= '0' && ascii <= '9') {
        *keycode = digit_keys[ascii - '0'];
        return 1;
    }

    switch (ascii) {
    case ' ':
        *keycode = KEY_SPACE;
        break;

    case '!':
        *keycode = KEY_1;
        *shift = 1;
        break;
    case '"':
        *keycode = KEY_APOSTROPHE;
        *shift = 1;
        break;
    case '#':
        *keycode = KEY_3;
        *shift = 1;
        break;
    case '$':
        *keycode = KEY_4;
        *shift = 1;
        break;
    case '%':
        *keycode = KEY_5;
        *shift = 1;
        break;
    case '&':
        *keycode = KEY_7;
        *shift = 1;
        break;
    case '\'':
        *keycode = KEY_APOSTROPHE;
        break;
    case '(':
        *keycode = KEY_9;
        *shift = 1;
        break;
    case ')':
        *keycode = KEY_0;
        *shift = 1;
        break;
    case '*':
        *keycode = KEY_8;
        *shift = 1;
        break;
    case '+':
        *keycode = KEY_EQUAL;
        *shift = 1;
        break;
    case ',':
        *keycode = KEY_COMMA;
        break;
    case '-':
        *keycode = KEY_MINUS;
        break;
    case '.':
        *keycode = KEY_DOT;
        break;
    case '/':
        *keycode = KEY_SLASH;
        break;

    case ':':
        *keycode = KEY_SEMICOLON;
        *shift = 1;
        break;
    case ';':
        *keycode = KEY_SEMICOLON;
        break;
    case '<':
        *keycode = KEY_COMMA;
        *shift = 1;
        break;
    case '=':
        *keycode = KEY_EQUAL;
        break;
    case '>':
        *keycode = KEY_DOT;
        *shift = 1;
        break;
    case '?':
        *keycode = KEY_SLASH;
        *shift = 1;
        break;
    case '@':
        *keycode = KEY_2;
        *shift = 1;
        break;

    case '[':
        *keycode = KEY_LEFTBRACE;
        break;
    case '\\':
        *keycode = KEY_BACKSLASH;
        break;
    case ']':
        *keycode = KEY_RIGHTBRACE;
        break;
    case '^':
        *keycode = KEY_6;
        *shift = 1;
        break;
    case '_':
        *keycode = KEY_MINUS;
        *shift = 1;
        break;
    case '`':
        *keycode = KEY_GRAVE;
        break;

    case '{':
        *keycode = KEY_LEFTBRACE;
        *shift = 1;
        break;
    case '|':
        *keycode = KEY_BACKSLASH;
        *shift = 1;
        break;
    case '}':
        *keycode = KEY_RIGHTBRACE;
        *shift = 1;
        break;
    case '~':
        *keycode = KEY_GRAVE;
        *shift = 1;
        break;

    default:
        return 0;
    }

    return 1;
}

#endif
