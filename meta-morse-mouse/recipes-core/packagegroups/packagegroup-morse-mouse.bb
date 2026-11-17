SUMMARY = "MorseMouse Project Package Group"
DESCRIPTION = "Collects packages required by the MorseMouse system."

LICENSE = "MIT"

inherit packagegroup

# Runtime packages used by both production and development images.
ALLOW_EMPTY:${PN} = "1"

RDEPENDS:${PN} = "hid-morse-kbd morsed"
