#ifndef MORSE_KEYMAP_H
#define MORSE_KEYMAP_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t keycode;
    bool shift;
} MorseKeyStroke;

/*
 * Translate one printable ASCII character into a Linux input keycode
 * and modifier requirement for a US QWERTY keyboard.
 *
 * Returns true on success.
 * Returns false for non-printable ASCII or a NULL output pointer.
 */
bool morse_ascii_to_keystroke(unsigned char ascii, MorseKeyStroke *stroke);

#endif
