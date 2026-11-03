SUMMARY = "MorseMouse userspace reference decoder and evdev prototype"
DESCRIPTION = "Decodes the MorseMouse ASCII table and maps Linux mouse button events to Morse symbols."

LICENSE = "CLOSED"

FILESEXTRAPATHS:prepend := "${THISDIR}/../../../tools:${THISDIR}/../../../config:"

SRC_URI = "file://morse_ref_decoder.c file://morse_map.csv"

S = "${WORKDIR}"
B = "${WORKDIR}/build"

do_compile() {
    install -d ${B}
    ${CC} ${CPPFLAGS} ${CFLAGS} ${LDFLAGS} \
        -std=c11 -Wall -Wextra -Wpedantic -Werror \
        -o ${B}/morse-ref-decoder ${S}/morse_ref_decoder.c
}

do_install() {
    install -D -m 0755 ${B}/morse-ref-decoder ${D}${bindir}/morse-ref-decoder
    install -D -m 0644 ${S}/morse_map.csv ${D}${datadir}/morsemouse/morse_map.csv
}

# Include the runtime mapping table in the package.
FILES:${PN} += "${datadir}/morsemouse/morse_map.csv"
