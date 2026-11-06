SUMMARY = "MorseMouse userspace reference decoder with optional uinput keyboard"
DESCRIPTION = "Decodes MorseMouse input and optionally emits characters through a virtual Linux keyboard."

LICENSE = "CLOSED"

FILESEXTRAPATHS:prepend := "${THISDIR}/../../../tools:${THISDIR}/../../../config:"

SRC_URI = " \
    file://morse_ref_decoder.c \
    file://morse_fsm.c \
    file://morse_fsm.h \
    file://morse_fsm_core.h \
    file://morse_keymap.c \
    file://morse_keymap.h \
    file://morse_uinput.c \
    file://morse_uinput.h \
    file://morse_map.csv \
"

S = "${WORKDIR}"
B = "${WORKDIR}/build"

do_compile() {
    install -d ${B}

    ${CC} ${CPPFLAGS} ${CFLAGS} ${LDFLAGS} \
        -std=c11 -Wall -Wextra -Wpedantic -Werror \
        -o ${B}/morse-ref-decoder \
        ${S}/morse_ref_decoder.c \
        ${S}/morse_fsm.c \
        ${S}/morse_keymap.c \
        ${S}/morse_uinput.c
}

do_install() {
    install -D -m 0755 ${B}/morse-ref-decoder \
        ${D}${bindir}/morse-ref-decoder

    install -D -m 0644 ${S}/morse_map.csv \
        ${D}${datadir}/morsemouse/morse_map.csv
}

FILES:${PN} += "${datadir}/morsemouse/morse_map.csv"
