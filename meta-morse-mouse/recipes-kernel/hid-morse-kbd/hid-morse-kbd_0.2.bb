SUMMARY = "MorseMouse Linux input handler"
DESCRIPTION = "QEMU-targeted Linux input handler for the MorseMouse project."
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

FILESEXTRAPATHS:prepend := "${THISDIR}/../../../driver:${THISDIR}/../../../tools:"

SRC_URI = " \
    file://Makefile \
    file://hid_morse_kbd.c \
    file://morse_fsm_core.h \
    file://morse_map_kernel.h \
"

S = "${WORKDIR}"

inherit module
CLEANBROKEN = "1"

KERNEL_MODULE_AUTOLOAD += "hid_morse_kbd"

COMPATIBLE_MACHINE = "(qemuarm64|raspberrypi4-64)"
