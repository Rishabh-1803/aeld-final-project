#include "morse_keymap.h"

#include <linux/input-event-codes.h>
#include <stddef.h>

#define ASCII_PRINTABLE_MIN 32
#define ASCII_PRINTABLE_MAX 126

bool morse_ascii_to_keystroke(unsigned char ascii, MorseKeyStroke *stroke)
{
    static const uint16_t letter_keys[26] = {
        KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G,
        KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N,
        KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U,
        KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z
    };

    static const uint16_t digit_keys[10] = {
        KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
        KEY_5, KEY_6, KEY_7, KEY_8, KEY_9
    };

    if (stroke == NULL)
        return false;

    /* Invalid input must never leave a stale key mapping behind. */
    stroke->keycode = 0;
    stroke->shift = false;

    if (ascii < ASCII_PRINTABLE_MIN || ascii > ASCII_PRINTABLE_MAX)
        return false;

    /* Lowercase letters. */
    if (ascii >= 'a' && ascii <= 'z') {
        stroke->keycode = letter_keys[ascii - 'a'];
        return true;
    }

    /* Uppercase letters require Shift. */
    if (ascii >= 'A' && ascii <= 'Z') {
        stroke->keycode = letter_keys[ascii - 'A'];
        stroke->shift = true;
        return true;
    }

    /* Digits. */
    if (ascii >= '0' && ascii <= '9') {
        stroke->keycode = digit_keys[ascii - '0'];
        return true;
    }

    /* US QWERTY punctuation and whitespace. */
    switch (ascii) {
    case ' ':
        stroke->keycode = KEY_SPACE;
        break;

    case '!':
        stroke->keycode = KEY_1;
        stroke->shift = true;
        break;
    case '"':
        stroke->keycode = KEY_APOSTROPHE;
        stroke->shift = true;
        break;
    case '#':
        stroke->keycode = KEY_3;
        stroke->shift = true;
        break;
    case '$':
        stroke->keycode = KEY_4;
        stroke->shift = true;
        break;
    case '%':
        stroke->keycode = KEY_5;
        stroke->shift = true;
        break;
    case '&':
        stroke->keycode = KEY_7;
        stroke->shift = true;
        break;
    case '\'':
        stroke->keycode = KEY_APOSTROPHE;
        break;
    case '(':
        stroke->keycode = KEY_9;
        stroke->shift = true;
        break;
    case ')':
        stroke->keycode = KEY_0;
        stroke->shift = true;
        break;
    case '*':
        stroke->keycode = KEY_8;
        stroke->shift = true;
        break;
    case '+':
        stroke->keycode = KEY_EQUAL;
        stroke->shift = true;
        break;
    case ',':
        stroke->keycode = KEY_COMMA;
        break;
    case '-':
        stroke->keycode = KEY_MINUS;
        break;
    case '.':
        stroke->keycode = KEY_DOT;
        break;
    case '/':
        stroke->keycode = KEY_SLASH;
        break;

    case ':':
        stroke->keycode = KEY_SEMICOLON;
        stroke->shift = true;
        break;
    case ';':
        stroke->keycode = KEY_SEMICOLON;
        break;
    case '<':
        stroke->keycode = KEY_COMMA;
        stroke->shift = true;
        break;
    case '=':
        stroke->keycode = KEY_EQUAL;
        break;
    case '>':
        stroke->keycode = KEY_DOT;
        stroke->shift = true;
        break;
    case '?':
        stroke->keycode = KEY_SLASH;
        stroke->shift = true;
        break;
    case '@':
        stroke->keycode = KEY_2;
        stroke->shift = true;
        break;

    case '[':
        stroke->keycode = KEY_LEFTBRACE;
        break;
    case '\\':
        stroke->keycode = KEY_BACKSLASH;
        break;
    case ']':
        stroke->keycode = KEY_RIGHTBRACE;
        break;
    case '^':
        stroke->keycode = KEY_6;
        stroke->shift = true;
        break;
    case '_':
        stroke->keycode = KEY_MINUS;
        stroke->shift = true;
        break;
    case '`':
        stroke->keycode = KEY_GRAVE;
        break;

    case '{':
        stroke->keycode = KEY_LEFTBRACE;
        stroke->shift = true;
        break;
    case '|':
        stroke->keycode = KEY_BACKSLASH;
        stroke->shift = true;
        break;
    case '}':
        stroke->keycode = KEY_RIGHTBRACE;
        stroke->shift = true;
        break;
    case '~':
        stroke->keycode = KEY_GRAVE;
        stroke->shift = true;
        break;

    default:
        return false;
    }

    return true;
}
