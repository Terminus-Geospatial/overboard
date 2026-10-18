#!/usr/bin/env bash
# Build the Pi Zero (aarch64) target inside a Debian container and emit
# the .deb into the repo root — replaces compiling on the device.
#
# Usage: ./scripts/docker_build.sh [build.sh args]
#   Extra args forward to scripts/build.sh inside the container
#   (e.g. -j 8, -c, -d, -r, -x). TARGET_DEVICE=ZERO and SIMULATOR=off
#   are always set.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
IMAGE="overboard-pizero:trixie"
# Bind-mount a dedicated subdir under ./build so artifacts (calc_app,
# the .deb staging tree) are visible on the host and the incremental
# cache persists. Kept separate from ./build root, which holds the
# native macOS/SDL build with an incompatible CMake cache.
BUILD_DIR="${PROJECT_DIR}/build/pizero"
mkdir -p "${BUILD_DIR}"

echo "=== Building Docker image (${IMAGE})... ==="
docker build -f "${PROJECT_DIR}/docker/Dockerfile.pi-zero" \
    -t "${IMAGE}" "${PROJECT_DIR}/docker"

# Default to all host cores unless -j was passed — unlike the Pi, the
# host has the cores and RAM to spare. Docker allocates its own CPU
# quota, so cap at the host count.
JOBS_ARG=""
if [[ " $* " != *" -j "* ]]; then
    JOBS_ARG="-j $(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)"
fi

echo ""
echo "=== Compiling in container (linux/arm64)... ==="
docker run --rm \
    --platform linux/arm64 \
    -v "${PROJECT_DIR}:/src" \
    -v "${BUILD_DIR}:/build" \
    -w /src \
    "${IMAGE}" \
    env OVB_BUILD_DIR=/build \
        CCACHE_DIR=/build/.ccache \
        OVB_CMAKE_EXTRA_ARGS="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache" \
        bash -c './scripts/build.sh -p ZERO -s off ${JOBS_ARG} "$@" && \
                 cmake --install /build --prefix /tmp/staging && \
                 cpack --config /build/CPackConfig.cmake -B /build && \
                 cp /build/overboard-*-arm64.deb /src/' _ "$@"

echo ""
echo "=== Done ==="
ls -lh "${PROJECT_DIR}"/overboard-*-arm64.deb
