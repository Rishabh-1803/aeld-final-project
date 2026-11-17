FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:append:raspberrypi4-64 = "${@bb.utils.contains('MORSE_ENABLE_TEST_ACCESS', '1', ' file://morse-interfaces', '', d)}"
do_install:append:raspberrypi4-64() {
    if [ "${MORSE_ENABLE_TEST_ACCESS}" = "1" ]; then
        install -m 0644 ${WORKDIR}/morse-interfaces ${D}${sysconfdir}/network/interfaces
    fi
}
