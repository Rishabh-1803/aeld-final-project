SUMMARY = "MorseMouse Development and Input Diagnostics Image"
DESCRIPTION = "Development image for testing MorseMouse input devices and uinput."

require core-image-morse.bb

IMAGE_INSTALL:append = " evtest"
