#ifndef MORSE_UINPUT_H
#define MORSE_UINPUT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int fd;
    bool device_created;
    bool key_pressed;
    bool shift_pressed;
    bool events_pending;
    uint16_t active_keycode;
} MorseUinputKeyboard;

/* Create the MorseMouse virtual keyboard through /dev/uinput. */
int morse_uinput_open(MorseUinputKeyboard *keyboard);

/* Emit one printable ASCII character as Linux keyboard events. */
int morse_uinput_emit_ascii(MorseUinputKeyboard *keyboard,
                            unsigned char ascii);

/* Release any key or modifier that remains pressed. */
int morse_uinput_release_all(MorseUinputKeyboard *keyboard);

/* Release keys, destroy the virtual device and close its descriptor. */
int morse_uinput_close(MorseUinputKeyboard *keyboard);

#endif
