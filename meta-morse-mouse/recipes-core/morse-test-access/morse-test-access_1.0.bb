SUMMARY = "Optional locally provisioned MorseMouse test access"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
FILESEXTRAPATHS:prepend := "${MORSE_PRIVATE_FILES}:"
SRC_URI = "file://authorized_keys"
SRC_URI:append:raspberrypi4-64 = " file://wpa_supplicant.conf"
S = "${WORKDIR}"
PACKAGE_ARCH = "${MACHINE_ARCH}"
RDEPENDS:${PN} = "dropbear"
RDEPENDS:${PN}:append:raspberrypi4-64 = " wpa-supplicant wpa-supplicant-cli iw wireless-regdb usbutils avahi-daemon"
do_install() {
    install -d -m 0700 ${D}${ROOT_HOME}/.ssh
    install -m 0600 ${WORKDIR}/authorized_keys ${D}${ROOT_HOME}/.ssh/authorized_keys
}
do_install:append:raspberrypi4-64() {
    install -d -m 0700 ${D}${sysconfdir}/wpa_supplicant
    install -m 0600 ${WORKDIR}/wpa_supplicant.conf ${D}${sysconfdir}/wpa_supplicant/wpa_supplicant-wlan0.conf
}
FILES:${PN} += "${ROOT_HOME}/.ssh"
