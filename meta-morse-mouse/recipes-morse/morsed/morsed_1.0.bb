SUMMARY = "MorseMouse diagnostic daemon and read-only terminal HUD"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://morsed.c;beginline=1;endline=1;md5=234d7d4edd08962c0144e4604050e0b6"
FILESEXTRAPATHS:prepend := "${THISDIR}/../../../userspace:${THISDIR}/../../../include:"
SRC_URI = "file://morse_input_stress.c file://morsed.c file://morse_hud_client.c file://morse_hid_uapi.h file://morsed.conf file://morsed.init"
S = "${WORKDIR}"
inherit update-rc.d
INITSCRIPT_NAME = "morsed"
INITSCRIPT_PARAMS = "defaults 90"
CONFFILES:${PN} = "${sysconfdir}/morsemouse/morsed.conf"
do_compile() {
    ${CC} ${CPPFLAGS} ${CFLAGS} ${LDFLAGS} -o ${B}/morse-input-stress ${S}/morse_input_stress.c
    ${CC} ${CPPFLAGS} ${CFLAGS} -I${S} ${LDFLAGS} -o ${B}/morsed ${S}/morsed.c
    ${CC} ${CPPFLAGS} ${CFLAGS} ${LDFLAGS} -o ${B}/morse_hud_client ${S}/morse_hud_client.c
}
do_install() {
    install -d ${D}${bindir} ${D}${sysconfdir}/morsemouse ${D}${sysconfdir}/init.d
    install -m 0755 ${B}/morse-input-stress ${B}/morsed ${B}/morse_hud_client ${D}${bindir}/
    install -m 0644 ${S}/morsed.conf ${D}${sysconfdir}/morsemouse/
    install -m 0755 ${S}/morsed.init ${D}${sysconfdir}/init.d/morsed
}

PACKAGES =+ "${PN}-tests"
FILES:${PN}-tests = "${bindir}/morse-input-stress"
