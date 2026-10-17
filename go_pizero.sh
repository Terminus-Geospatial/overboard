#!/usr/bin/env bash

set -e

CLEAN_FLAG=""
VERBOSE_FLAG=""
DEBUG_FLAG=""
DEBUG_LAYOUT_FLAG=""
PACKAGE_FLAG=false

while getopts ":cvdxp" opt; do
    case "${opt}" in
        c) CLEAN_FLAG="-c" ;;
        v) VERBOSE_FLAG="-t" ;;
        d) DEBUG_FLAG="-d" ;;
        x) DEBUG_LAYOUT_FLAG="-x" ;;
        p) PACKAGE_FLAG=true ;;
        \?)
            echo "Usage: $0 [-c] [-d] [-p] [-v] [-x]"
            echo "  -c  Clean build"
            echo "  -d  Debug build type"
            echo "  -p  Create Debian package after build"
            echo "  -v  Verbose build output"
            echo "  -x  Enable debug layout borders"
            exit 1
            ;;
    esac
done

echo "Building for Pi Zero (DRM/KMS)..."
./scripts/build.sh ${CLEAN_FLAG} ${DEBUG_FLAG} ${VERBOSE_FLAG} ${DEBUG_LAYOUT_FLAG} -j -p ZERO 2>&1 | tee build.log
if [ ${PIPESTATUS[0]} -eq 0 ]; then
    echo "Build successful."
    echo "Binary: ./build/calc_app"
    echo ""
    echo "To run on Pi Zero:"
    echo "  1. Copy build/calc_app to the Pi Zero"
    echo "  2. Ensure keyboard.json is in the same directory"
    echo "  3. Run: ./calc_app --layout ./data/configs/mk20 -v trace"

    if ${PACKAGE_FLAG}; then
        echo ""
        echo "Creating Debian package..."
        cmake --install build --prefix /tmp/overboard-staging
        cpack --config build/CPackConfig.cmake
        echo "Package created: overboard-*.deb"
    fi
else
    echo "Build failed. Check build.log for details."
    tail -20 build.log
    exit 1
fi
