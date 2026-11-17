SUMMARY = "MorseMouse Development and Input Diagnostics Image"
DESCRIPTION = "Development image for testing MorseMouse input devices and userspace decoding."

require core-image-morse.bb

IMAGE_INSTALL:append = " evtest uinput-selftest morse-ref-decoder morse-hid-test"

IMAGE_INSTALL:append = " hid-morse-kbd"

IMAGE_INSTALL:append = " morsed"
