#!/usr/bin/env bash
#
# Build the Unturned Command Generator for Linux.
#
# Produces:
#   build/UnturnedCmdGen              - the executable
#   build/unturnedcmdgen_*.deb        - the Debian package
#
# Usage:
#   ./build.sh [BUILD_DIR]            # BUILD_DIR defaults to "build"
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="${1:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

if ! command -v cmake >/dev/null 2>&1; then
    echo "error: cmake is required (apt install cmake)" >&2
    exit 1
fi
if ! command -v ninja >/dev/null 2>&1; then
    echo "error: ninja is required (apt install ninja-build)" >&2
    exit 1
fi
if ! command -v cpack >/dev/null 2>&1; then
    echo "error: cpack is required (ships with cmake)" >&2
    exit 1
fi

echo "==> Configuring (${BUILD_TYPE}) in ${BUILD_DIR}"
cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

echo "==> Building executable"
cmake --build "$BUILD_DIR"

echo "==> Building .deb package"
(cd "$BUILD_DIR" && cpack -G DEB)

echo
echo "==> Done"
echo "Executable: ${BUILD_DIR}/UnturnedCmdGen"
echo "Deb:        ${BUILD_DIR}/$(cd "$BUILD_DIR" && ls -1 *.deb | head -1)"
