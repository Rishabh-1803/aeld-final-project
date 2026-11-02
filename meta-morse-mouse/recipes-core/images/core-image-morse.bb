SUMMARY = "MorseMouse Embedded Linux Development Image"
DESCRIPTION = "A minimal Yocto image for the MorseMouse AELD project."

IMAGE_INSTALL = "packagegroup-core-boot ${CORE_IMAGE_EXTRA_INSTALL}"

IMAGE_LINGUAS = " "

LICENSE = "MIT"

inherit core-image

# MorseMouse project packages.
IMAGE_INSTALL:append = " packagegroup-morse-mouse"

# Match the upstream core-image-minimal filesystem sizing.
IMAGE_ROOTFS_SIZE ?= "8192"
IMAGE_ROOTFS_EXTRA_SPACE:append = "${@bb.utils.contains("DISTRO_FEATURES", "systemd", " + 4096", "", d)}"
