#include "morse_keymap.h"
#include "morse_keymap_shared.h"

bool morse_ascii_to_keystroke(unsigned char ascii, MorseKeyStroke *stroke)
{
    unsigned short keycode;
    unsigned char shift;

    if (!stroke)
        return false;

    stroke->keycode = 0;
    stroke->shift = false;

    if (!morse_shared_ascii_to_key(ascii, &keycode, &shift))
        return false;

    stroke->keycode = keycode;
    stroke->shift = shift != 0;

    return true;
}
