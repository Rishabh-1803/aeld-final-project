SUMMARY = "MorseMouse Project Package Group"
DESCRIPTION = "Collects packages required by the MorseMouse system."

LICENSE = "MIT"

inherit packagegroup

# Allow an initially empty package group.
# Additional MorseMouse packages will be added as they are implemented.
ALLOW_EMPTY:${PN} = "1"
