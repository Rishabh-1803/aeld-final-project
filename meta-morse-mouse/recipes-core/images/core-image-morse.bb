SUMMARY = "MorseMouse Embedded Linux Development Image"
DESCRIPTION = "A minimal Yocto image for the MorseMouse AELD project."

LICENSE = "MIT"

inherit core-image

# MorseMouse project packages.
IMAGE_INSTALL:append = " packagegroup-morse-mouse"
