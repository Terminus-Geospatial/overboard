# cmake/packaging.cmake
# CPack configuration for building a Debian (.deb) package of the Pi Zero target.
# Included by the root CMakeLists.txt when TARGET_DEVICE=ZERO.
#
# Usage:
#   cmake -B build -DTARGET_DEVICE=ZERO
#   cmake --build build
#   cmake --install build --prefix /tmp/overboard-staging
#   cpack --config build/CPackConfig.cmake

include(GNUInstallDirs)

# ── Install Rules ──────────────────────────────────────────────────────────────

# Main application binary
install(TARGETS calc_app
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT runtime
)

# Keyboard config files (includes icons/ subdirectory)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/data/configs/mk20/
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/overboard/configs/mk20
    COMPONENT runtime
)

# systemd service unit
install(FILES ${CMAKE_SOURCE_DIR}/data/hardware/pi_zero/overboard.service
    DESTINATION lib/systemd/system
    COMPONENT runtime
)

# udev rules
install(FILES ${CMAKE_SOURCE_DIR}/data/hardware/pi_zero/99-overboard-drm.rules
    DESTINATION lib/udev/rules.d
    COMPONENT runtime
)

# ── CPack / Debian Configuration ───────────────────────────────────────────────

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "overboard")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "Marvin Smith <marvin@terminus-geospatial.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Overboard calculator application for Raspberry Pi Zero")
set(CPACK_PACKAGE_DESCRIPTION
"Overboard is a keyboard-driven calculator application designed for
the Raspberry Pi Zero with a DRM/KMS HDMI display. It runs as a
systemd service under a dedicated unprivileged user.")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://terminus-geospatial.github.io/overboard/")

set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

# Runtime dependencies on the Pi Zero
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libdrm2 (>= 2.4), libgbm1, libpng16-16, adduser"
)

# Debian maintainer scripts
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_SOURCE_DIR}/packaging/debian/postinst"
    "${CMAKE_SOURCE_DIR}/packaging/debian/prerm"
)

# Output filename: overboard-<version>-arm64.deb
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}"
)

include(CPack)
