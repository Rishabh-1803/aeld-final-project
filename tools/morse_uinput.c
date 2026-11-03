#define _POSIX_C_SOURCE 200809L

#include "morse_uinput.h"
#include "morse_keymap.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MORSE_UINPUT_DEVICE_NAME "MorseMouse Reference Keyboard"

static int write_input_event(MorseUinputKeyboard *keyboard,
                             unsigned int type,
                             unsigned int code,
                             int value)
{
    struct input_event event;
    ssize_t written;

    memset(&event, 0, sizeof(event));

    event.type = (unsigned short)type;
    event.code = (unsigned short)code;
    event.value = value;

    do {
        written = write(keyboard->fd, &event, sizeof(event));
    } while (written < 0 && errno == EINTR);

    if (written != (ssize_t)sizeof(event)) {
        if (written >= 0)
            errno = EIO;
        return -1;
    }

    if (type == EV_KEY)
        keyboard->events_pending = true;
    else if (type == EV_SYN && code == SYN_REPORT)
        keyboard->events_pending = false;

    return 0;
}

static int write_sync(MorseUinputKeyboard *keyboard)
{
    return write_input_event(keyboard, EV_SYN, SYN_REPORT, 0);
}

int morse_uinput_open(MorseUinputKeyboard *keyboard)
{
    struct uinput_setup setup;
    unsigned int ascii;

    if (keyboard == NULL) {
        errno = EINVAL;
        return -1;
    }

    memset(keyboard, 0, sizeof(*keyboard));
    keyboard->fd = -1;

    keyboard->fd = open("/dev/uinput", O_WRONLY | O_CLOEXEC);
    if (keyboard->fd < 0)
        return -1;

    if (ioctl(keyboard->fd, UI_SET_EVBIT, EV_KEY) < 0)
        goto fail;

    /*
     * Enable exactly the keycodes required by the 95-character
     * US QWERTY mapping. Repeated UI_SET_KEYBIT calls are harmless.
     */
    for (ascii = 32; ascii <= 126; ascii++) {
        MorseKeyStroke stroke;

        if (!morse_ascii_to_keystroke((unsigned char)ascii, &stroke)) {
            errno = EINVAL;
            goto fail;
        }

        if (ioctl(keyboard->fd, UI_SET_KEYBIT,
                  (int)stroke.keycode) < 0)
            goto fail;

        if (stroke.shift &&
            ioctl(keyboard->fd, UI_SET_KEYBIT, KEY_LEFTSHIFT) < 0)
            goto fail;
    }

    memset(&setup, 0, sizeof(setup));

    setup.id.bustype = BUS_VIRTUAL;
    setup.id.vendor = 0x0001;
    setup.id.product = 0x0001;
    setup.id.version = 1;

    (void)snprintf(setup.name, sizeof(setup.name), "%s",
                   MORSE_UINPUT_DEVICE_NAME);

    if (ioctl(keyboard->fd, UI_DEV_SETUP, &setup) < 0)
        goto fail;

    if (ioctl(keyboard->fd, UI_DEV_CREATE) < 0)
        goto fail;

    keyboard->device_created = true;

    printf("UINPUT: created virtual keyboard '%s'.\n",
           MORSE_UINPUT_DEVICE_NAME);

    return 0;

fail:
    {
        int saved_errno = errno;

        if (keyboard->device_created)
            (void)ioctl(keyboard->fd, UI_DEV_DESTROY);

        if (keyboard->fd >= 0)
            (void)close(keyboard->fd);

        memset(keyboard, 0, sizeof(*keyboard));
        keyboard->fd = -1;

        errno = saved_errno;
        return -1;
    }
}

int morse_uinput_release_all(MorseUinputKeyboard *keyboard)
{
    int saved_errno = 0;

    if (keyboard == NULL || keyboard->fd < 0) {
        errno = EINVAL;
        return -1;
    }

    /* Release the main key before releasing Shift. */
    if (keyboard->key_pressed) {
        if (write_input_event(keyboard, EV_KEY,
                              keyboard->active_keycode, 0) < 0) {
            saved_errno = errno;
        } else {
            keyboard->key_pressed = false;
            keyboard->active_keycode = 0;
        }
    }

    if (keyboard->shift_pressed) {
        if (write_input_event(keyboard, EV_KEY, KEY_LEFTSHIFT, 0) < 0) {
            if (saved_errno == 0)
                saved_errno = errno;
        } else {
            keyboard->shift_pressed = false;
        }
    }

    /*
     * Also retry synchronization when a previous SYN_REPORT failed.
     */
    if (keyboard->events_pending) {
        if (write_sync(keyboard) < 0 && saved_errno == 0)
            saved_errno = errno;
    }

    if (saved_errno != 0) {
        errno = saved_errno;
        return -1;
    }

    return 0;
}

int morse_uinput_emit_ascii(MorseUinputKeyboard *keyboard,
                            unsigned char ascii)
{
    MorseKeyStroke stroke;
    int saved_errno;

    if (keyboard == NULL || keyboard->fd < 0 ||
        !keyboard->device_created) {
        errno = EINVAL;
        return -1;
    }

    if (keyboard->key_pressed || keyboard->shift_pressed) {
        errno = EBUSY;
        return -1;
    }

    if (!morse_ascii_to_keystroke(ascii, &stroke)) {
        errno = EINVAL;
        return -1;
    }

    if (stroke.shift) {
        if (write_input_event(keyboard, EV_KEY, KEY_LEFTSHIFT, 1) < 0)
            goto fail;

        keyboard->shift_pressed = true;
    }

    if (write_input_event(keyboard, EV_KEY, stroke.keycode, 1) < 0)
        goto fail;

    keyboard->key_pressed = true;
    keyboard->active_keycode = stroke.keycode;

    if (write_sync(keyboard) < 0)
        goto fail;

    if (write_input_event(keyboard, EV_KEY, stroke.keycode, 0) < 0)
        goto fail;

    keyboard->key_pressed = false;
    keyboard->active_keycode = 0;

    if (stroke.shift) {
        if (write_input_event(keyboard, EV_KEY, KEY_LEFTSHIFT, 0) < 0)
            goto fail;

        keyboard->shift_pressed = false;
    }

    if (write_sync(keyboard) < 0)
        goto fail;

    return 0;

fail:
    saved_errno = errno;

    (void)morse_uinput_release_all(keyboard);

    errno = saved_errno;
    return -1;
}

int morse_uinput_close(MorseUinputKeyboard *keyboard)
{
    int saved_errno = 0;

    if (keyboard == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (keyboard->fd < 0)
        return 0;

    if (morse_uinput_release_all(keyboard) < 0)
        saved_errno = errno;

    if (keyboard->device_created) {
        if (ioctl(keyboard->fd, UI_DEV_DESTROY) < 0 &&
            saved_errno == 0)
            saved_errno = errno;
    }

    if (close(keyboard->fd) < 0 && saved_errno == 0)
        saved_errno = errno;

    memset(keyboard, 0, sizeof(*keyboard));
    keyboard->fd = -1;

    if (saved_errno != 0) {
        errno = saved_errno;
        return -1;
    }

    puts("UINPUT: virtual keyboard destroyed.");

    return 0;
}
