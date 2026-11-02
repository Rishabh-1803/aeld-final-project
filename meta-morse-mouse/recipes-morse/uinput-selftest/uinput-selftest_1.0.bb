SUMMARY = "MorseMouse uinput virtual-keyboard self-test"
DESCRIPTION = "Creates a temporary virtual keyboard and verifies KEY_A events through evdev."

LICENSE = "CLOSED"

SRC_URI = "file://uinput-selftest.c"

S = "${WORKDIR}"
B = "${WORKDIR}"

do_compile() {
    ${CC} ${CPPFLAGS} ${CFLAGS} ${LDFLAGS} \
        -o ${B}/uinput-selftest ${S}/uinput-selftest.c
}

do_install() {
    install -D -m 0755 ${B}/uinput-selftest ${D}${bindir}/uinput-selftest
}
