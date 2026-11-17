SUMMARY = "MorseMouse character-device userspace test client"
DESCRIPTION = "Tests the MorseMouse diagnostic character-device UAPI."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://morse_hid_test.c;md5=b828a4076c076b1d1d1ed38f0a3b6934"

FILESEXTRAPATHS:prepend := "${THISDIR}/../../../tools:${THISDIR}/../../../include:"

SRC_URI = " \
    file://morse_hid_test.c \
    file://morse_hid_uapi.h \
"

S = "${WORKDIR}"

do_compile() {
    install -d ${B}
    ${CC} ${CPPFLAGS} ${CFLAGS} -I${S} \
        ${LDFLAGS} \
        -o ${B}/morse-hid-test \
        ${S}/morse_hid_test.c
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/morse-hid-test \
        ${D}${bindir}/morse-hid-test
}
