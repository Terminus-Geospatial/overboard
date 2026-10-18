#!/bin/bash
# Sync local code to Pi Zero and build remotely

set -e

REMOTE_HOST="marvin@pizero"
REMOTE_PATH="Projects/overboard"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
CLEAN_FLAG=""
VERBOSE_FLAG=""
DEBUG_FLAG=""
DEBUG_LAYOUT_FLAG=""
SYNC_ONLY=false
PACKAGE_FLAG=false
DOCKER_BUILD=false
JOBS=1

while getopts ":cvdxnpj:D" opt; do
    case "${opt}" in
        c) CLEAN_FLAG="-c" ;;
        v) VERBOSE_FLAG="-t" ;;
        d) DEBUG_FLAG="-d" ;;
        x) DEBUG_LAYOUT_FLAG="-x" ;;
        n) SYNC_ONLY=true ;;
        p) PACKAGE_FLAG=true ;;
        D) DOCKER_BUILD=true ;;
        j) JOBS="${OPTARG}" ;;
        \?)
            echo "Usage: $0 [-c] [-d] [-n] [-p] [-v] [-x] [-D] [-j jobs]"
            echo "  -c  Clean build"
            echo "  -d  Debug build type"
            echo "  -D  Build in Docker (arm64 container) instead of on the Pi"
            echo "  -j  Parallel build jobs (default: 1; -j 3 is a good Zero 2W value)"
            echo "  -n  Sync only (no build)"
            echo "  -p  Package, install, and restart the overboard service"
            echo "  -v  Verbose build output"
            echo "  -x  Enable debug layout borders"
            exit 1
            ;;
    esac
done

if ${DOCKER_BUILD}; then
    echo "=== Building in Docker (linux/arm64)... ==="
    "${SCRIPT_DIR}/docker_build.sh" ${CLEAN_FLAG} ${DEBUG_FLAG} \
        ${VERBOSE_FLAG} ${DEBUG_LAYOUT_FLAG} -j "${JOBS}"

    DEB_FILE="$(ls -t "${PROJECT_DIR}"/overboard-*-arm64.deb | head -n1)"
    DEB_NAME="$(basename "${DEB_FILE}")"

    if ${PACKAGE_FLAG}; then
        echo ""
        echo "=== Pushing package (${DEB_NAME})... ==="
        rsync -avz "${DEB_FILE}" "${REMOTE_HOST}:${REMOTE_PATH}/"
        echo ""
        echo "=== Installing package and restarting service (may prompt for sudo)... ==="
        ssh -t "${REMOTE_HOST}" "cd ${REMOTE_PATH} && sudo dpkg -i ${DEB_NAME} && sudo systemctl daemon-reload && sudo systemctl restart overboard"
        echo ""
        echo "=== Service status ==="
        ssh "${REMOTE_HOST}" "systemctl status overboard --no-pager -l | head -n 12"
    fi
    echo "Done!"
    exit 0
fi

echo "=== Syncing code to ${REMOTE_HOST}... ==="
rsync -avz \
    --exclude='build/' \
    --exclude='.git/' \
    --exclude='venv/' \
    --exclude='*.o' \
    --exclude='*.a' \
    --exclude='*.so' \
    ./ "${REMOTE_HOST}:${REMOTE_PATH}/"

if ${SYNC_ONLY}; then
    echo "Sync complete (skipping build)."
    exit 0
fi

echo ""
echo "=== Running remote build... ==="
ssh "${REMOTE_HOST}" "set -o pipefail && cd ${REMOTE_PATH} && ./scripts/build.sh ${CLEAN_FLAG} ${DEBUG_FLAG} ${VERBOSE_FLAG} ${DEBUG_LAYOUT_FLAG} -j ${JOBS} -p ZERO -s off 2>&1 | tee build.log"

echo ""
echo "=== Build complete. Pulling updated logs... ==="
rsync -avz "${REMOTE_HOST}:${REMOTE_PATH}/build.log" ./

if ${PACKAGE_FLAG}; then
    echo ""
    echo "=== Creating Debian package... ==="
    ssh "${REMOTE_HOST}" "cd ${REMOTE_PATH} && cmake --install build --prefix /tmp/overboard-staging && cpack --config build/CPackConfig.cmake"

    DEB_NAME=$(ssh "${REMOTE_HOST}" "cd ${REMOTE_PATH} && ls -t overboard-*-arm64.deb | head -n1")
    echo ""
    echo "=== Pulling package (${DEB_NAME})... ==="
    rsync -avz "${REMOTE_HOST}:${REMOTE_PATH}/${DEB_NAME}" ./

    echo ""
    echo "=== Installing package and restarting service (may prompt for sudo)... ==="
    ssh -t "${REMOTE_HOST}" "cd ${REMOTE_PATH} && sudo dpkg -i ${DEB_NAME} && sudo systemctl daemon-reload && sudo systemctl restart overboard"
    echo ""
    echo "=== Service status ==="
    ssh "${REMOTE_HOST}" "systemctl status overboard --no-pager -l | head -n 12"
fi

echo "Done!"
