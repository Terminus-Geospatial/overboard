#!/usr/bin/env bash

set -e

CLEAN_BUILD=0
DEBUG_BUILD=0
DEBUG_LAYOUT=0
RUN_LLDB=0
SKIP_RUN=0
RUN_TESTS=0
BUILD_WASM=0

while getopts "cdxlstw" opt; do
    case $opt in
        c) CLEAN_BUILD=1 ;;
        d) DEBUG_BUILD=1 ;;
        x) DEBUG_LAYOUT=1 ;;
        l) RUN_LLDB=1 ;;
        s) SKIP_RUN=1 ;;
        t) RUN_TESTS=1 ;;
        w) BUILD_WASM=1 ;;
        \?)
            echo "Usage: $0 [-c] [-d] [-x] [-l] [-s] [-t] [-w]"
            echo "  -c  Clean build (clear old build)"
            echo "  -d  Debug build type"
            echo "  -x  Enable debug layout borders"
            echo "  -l  Run with lldb debugger"
            echo "  -s  Skip running simulator (build only)"
            echo "  -t  Run unit tests instead of simulator"
            echo "  -w  Build WASM/Emscripten target and launch in browser"
            exit 1
            ;;
    esac
done

if [ $BUILD_WASM -eq 1 ]; then
    echo "Building WASM (TH33 macropad in browser)..."
    BUILD_ARGS="-j 4 -w"
    if [ $CLEAN_BUILD -eq 1 ]; then
        BUILD_ARGS="-c $BUILD_ARGS"
    fi
    if [ $DEBUG_BUILD -eq 1 ]; then
        BUILD_ARGS="$BUILD_ARGS -d"
    fi
    if [ $DEBUG_LAYOUT -eq 1 ]; then
        BUILD_ARGS="$BUILD_ARGS -x"
    fi

    ./scripts/build.sh $BUILD_ARGS 2>&1 | tee build.log
    BUILD_EXIT=${PIPESTATUS[0]}

    if [ $BUILD_EXIT -eq 0 ]; then
        echo "Build successful."
        if [ $SKIP_RUN -eq 0 ]; then
            echo "Launching browser..."
            # Find emrun in EMSDK
            EMSDK_PATH="${EMSDK:-${HOME}/emsdk}"
            if [ -f "${EMSDK_PATH}/upstream/emscripten/emrun" ]; then
                "${EMSDK_PATH}/upstream/emscripten/emrun" ./build/calc_sim.html
            elif command -v emrun >/dev/null 2>&1; then
                emrun ./build/calc_sim.html
            else
                echo "Error: emrun not found. Please source emsdk_env.sh or set EMSDK."
                exit 1
            fi
        else
            echo "Skipping browser launch (build only)."
        fi
    else
        echo "Build failed. Full output saved to build.log."
        exit 1
    fi
    exit 0
fi

echo "Building for SDL (MK20 macropad)..."

BUILD_ARGS="-j 4 -p SDL"
if [ $CLEAN_BUILD -eq 1 ]; then
    BUILD_ARGS="-c $BUILD_ARGS"
fi
if [ $DEBUG_BUILD -eq 1 ]; then
    BUILD_ARGS="$BUILD_ARGS -d"
fi
if [ $DEBUG_LAYOUT -eq 1 ]; then
    BUILD_ARGS="$BUILD_ARGS -x"
fi

./scripts/build.sh $BUILD_ARGS 2>&1 | tee build.log
BUILD_EXIT=${PIPESTATUS[0]}

if [ $BUILD_EXIT -eq 0 ]; then
    echo "Build successful."
    if [ $RUN_TESTS -eq 1 ]; then
        echo "Running unit tests..."
        cd build
        ctest --output-on-failure
        cd ..
    elif [ $SKIP_RUN -eq 0 ]; then
        echo "Starting simulator..."
        if [ $RUN_LLDB -eq 1 ]; then
            lldb -- ./build/calc_sim --layout "$(pwd)/data/configs/mk20" -v trace
        else
            ./build/calc_sim --layout ./data/configs/mk20 -v trace
        fi
    else
        echo "Skipping simulator run (build only)."
    fi
else
    echo "Build failed. Full output saved to build.log."
    exit 1
fi

