FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:append = " file://morse-dropbear.default"
do_install:append() {
    install -m 0644 ${WORKDIR}/morse-dropbear.default ${D}${sysconfdir}/default/dropbear
}
