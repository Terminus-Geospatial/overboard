#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
# OVB_BUILD_DIR overrides the build dir (used by the Docker flow so the
# container's objects don't share a directory with host artifacts).
BUILD_DIR="${OVB_BUILD_DIR:-${PROJECT_DIR}/build}"

BUILD_TYPE="Debug"
CLEAN=false
TARGET_DEVICE="SDL"
SIMULATOR=ON
JOBS="1"
VERBOSE=false
TOOLCHAIN_FILE=""
DEBUG_LAYOUT=OFF

get_cpu_count() {
    # Try multiple methods to get CPU count, in order of preference
    # Can be extended for other platforms
    nproc 2>/dev/null || \
        sysctl -n hw.ncpu 2>/dev/null || \
        echo 1
}

usage() {
    cat <<EOF
Usage: $(basename "$0") [options]

Options:
  -h           Show this help message
  -c           Clean build directory, then build
  -d           Debug build (default)
  -r           Release build
  -p <target>  Hardware target: SDL, PICOSDL, RP2350, PICOCALC, ZERO, or WASM (default: SDL)
  -s <on|off>  Simulator mode: on or off (default: on)
  -j [jobs]   Parallel jobs (default: 1, or all cores if omitted)
  -t           Trace/verbose build output
  -T <file>   CMake toolchain file (for cross-compilation)
  -x           Enable debug layout borders (DEBUG_LAYOUT=ON)
  -w           WASM/Emscripten build (short for -p WASM -T cmake/wasm.cmake)

Examples:
  $(basename "$0")              # SDL simulator, debug build
  $(basename "$0") -p RP2350    # RP2350 target, simulator off
  $(basename "$0") -p PICOSDL   # PicoCalc SDL target, no keyboard UI
  $(basename "$0") -p ZERO      # Pi Zero DRM target, direct HDMI
  $(basename "$0") -p SDL -r    # SDL simulator, release build
  $(basename "$0") -c -r         # Clean then release build
  $(basename "$0") -c            # Clean then build
  $(basename "$0") -j 4          # Use 4 parallel jobs
  $(basename "$0") -p RP2350 -T cmake/arm-none-eabi-gcc.cmake  # ARM cross-compile
  $(basename "$0") -w            # WASM build with Emscripten
EOF
}

while getopts ":hcdrp:s:j::tT:xw" opt; do
    case "${opt}" in
        h) usage; exit 0 ;;
        c) CLEAN=true ;;
        d) BUILD_TYPE="Debug" ;;
        r) BUILD_TYPE="Release" ;;
        p) TARGET_DEVICE="${OPTARG}" ;;
        s) SIMULATOR="${OPTARG}" ;;
        w) TARGET_DEVICE="WASM"; TOOLCHAIN_FILE="${PROJECT_DIR}/cmake/wasm.cmake" ;;
        j)
            if [[ -n "${OPTARG}" && "${OPTARG}" != -* ]]; then
                JOBS="${OPTARG}"
            else
                JOBS="$(get_cpu_count)"
                # OPTARG was another option, put it back
                [[ -n "${OPTARG}" ]] && OPTIND=$((OPTIND - 1))
            fi
            ;;
        t) VERBOSE=true ;;
        T) TOOLCHAIN_FILE="${OPTARG}" ;;
        x) DEBUG_LAYOUT=ON ;;
        :) echo "Error: option -${OPTARG} requires an argument." >&2; usage; exit 1 ;;
        \?) echo "Error: unknown option -${OPTARG}" >&2; usage; exit 1 ;;
    esac
done

# Validate target device and classify
case "${TARGET_DEVICE}" in
    SDL|PICOSDL|RP2350|PICOCALC|ZERO|WASM) ;;
    *) echo "Error: unknown target '${TARGET_DEVICE}'. Valid targets: SDL, PICOSDL, RP2350, PICOCALC, ZERO, WASM" >&2; usage; exit 1 ;;
esac

# Classify targets: SDL-based vs embedded (non-SDL)
TARGET_USES_SDL=OFF
case "${TARGET_DEVICE}" in
    SDL|PICOSDL|WASM) TARGET_USES_SDL=ON ;;
    *) TARGET_USES_SDL=OFF ;;
esac

# Normalize simulator flag
case "${SIMULATOR}" in
    on|ON|On|1|true|TRUE|True) SIMULATOR=ON ;;
    off|OFF|Off|0|false|FALSE|False) SIMULATOR=OFF ;;
    *) echo "Error: -s requires 'on' or 'off'" >&2; usage; exit 1 ;;
esac

# ── Emscripten environment setup for WASM builds ──────────────────────────────────────
setup_emscripten() {
    if command -v emcc >/dev/null 2>&1 && command -v em++ >/dev/null 2>&1; then
        return 0
    fi

    local emsdk_env_candidates=(
        "${EMSDK:-}/emsdk_env.sh"
        "${HOME}/emsdk/emsdk_env.sh"
        "${PROJECT_DIR}/../emsdk/emsdk_env.sh"
        "/Users/${USER}/emsdk/emsdk_env.sh"
        "/usr/local/emsdk/emsdk_env.sh"
    )

    for candidate in "${emsdk_env_candidates[@]}"; do
        if [[ -f "${candidate}" ]]; then
            # shellcheck source=/dev/null
            source "${candidate}" >/dev/null 2>&1
            # Add emsdk bin to PATH for emrun and other tools
            local emsdk_bin
            emsdk_bin="$(dirname "${candidate}")/upstream/emscripten"
            if [[ -d "${emsdk_bin}" ]]; then
                export PATH="${emsdk_bin}:${PATH}"
            fi
            if command -v emcc >/dev/null 2>&1 && command -v em++ >/dev/null 2>&1; then
                echo "Sourced Emscripten environment: ${candidate}"
                return 0
            fi
        fi
    done

    echo "Error: Emscripten SDK (emcc/em++) not found in PATH."
    echo "Please activate it first, e.g.:"
    echo "    source /path/to/emsdk/emsdk_env.sh"
    echo "Or set EMSDK to the SDK root directory."
    return 1
}

if [[ "${TARGET_DEVICE}" == "WASM" ]]; then
    setup_emscripten || exit 1
fi

if ${CLEAN}; then
    echo "Cleaning build directory: ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
    echo "Clean complete."
fi

echo "Build type    : ${BUILD_TYPE}"
echo "Target device : ${TARGET_DEVICE}"
echo "Simulator     : ${SIMULATOR}"
echo "Project dir   : ${PROJECT_DIR}"
echo "Build dir     : ${BUILD_DIR}"
echo ""

CMAKE_ARGS="-S ${PROJECT_DIR} -B ${BUILD_DIR} -DTARGET_DEVICE=${TARGET_DEVICE} -DSIMULATOR=${SIMULATOR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DDEBUG_LAYOUT=${DEBUG_LAYOUT} ${OVB_CMAKE_EXTRA_ARGS:-}"

# Disable LVGL SDL drivers for embedded targets
if [[ "${TARGET_USES_SDL}" == "OFF" ]]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DCONFIG_LV_USE_SDL=OFF"
fi

# Use emcmake for WASM to get Emscripten's own CMake toolchain
if [[ "${TARGET_DEVICE}" == "WASM" ]]; then
    CMAKE_CMD="emcmake cmake"
    # emcmake supplies its own toolchain; ignore the local one for this target
    TOOLCHAIN_FILE=""
else
    CMAKE_CMD="cmake"
fi

if [[ -n "${TOOLCHAIN_FILE}" ]]; then
    if [[ "${TOOLCHAIN_FILE}" = /* ]]; then
        CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}"
    else
        CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${PROJECT_DIR}/${TOOLCHAIN_FILE}"
    fi
fi

${CMAKE_CMD} ${CMAKE_ARGS}

if ${VERBOSE}; then
    if [[ "${TARGET_DEVICE}" == "WASM" ]]; then
        emmake make -C "${BUILD_DIR}" -j "${JOBS}" VERBOSE=1
    else
        cmake --build "${BUILD_DIR}" --parallel "${JOBS}" --verbose
    fi
else
    if [[ "${TARGET_DEVICE}" == "WASM" ]]; then
        emmake make -C "${BUILD_DIR}" -j "${JOBS}"
    else
        cmake --build "${BUILD_DIR}" --parallel "${JOBS}"
    fi
fi

echo ""
case "${TARGET_DEVICE}" in
    SDL|PICOSDL)
        echo "Build complete: ${BUILD_DIR}/calc_sim" ;;
    WASM)
        echo "Build complete: ${BUILD_DIR}/calc_sim.html" ;;
    ZERO)
        echo "Build complete: ${BUILD_DIR}/calc_app" ;;
    *)
        echo "Build complete: ${BUILD_DIR}/calc_firmware" ;;
esac
