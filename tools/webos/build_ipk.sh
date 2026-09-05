#!/usr/bin/env bash
# Build webOS IPK using openlgtv NDK (must run setup_ndk.sh first, or set TOOLCHAIN_FILE).
set -euo pipefail
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

if [ ! -d app ] || [ ! -f CMakeLists.txt ]; then
  echo "Please invoke this script from project tooling under tools/webos"
  exit 1
fi

CMAKE_BIN=$(command -v cmake)
CPACK_BIN=$(command -v cpack)
CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR:-build-webos}

if [ -z "${TOOLCHAIN_FILE:-}" ]; then
  if [ -f /tmp/webos-ndk/arm-webos-linux-gnueabi_sdk-buildroot/usr/share/buildroot/toolchainfile.cmake ]; then
    TOOLCHAIN_FILE=/tmp/webos-ndk/arm-webos-linux-gnueabi_sdk-buildroot/usr/share/buildroot/toolchainfile.cmake
  elif [ -f /opt/webos-sdk-x86_64/1.0.g/sysroots/x86_64-webossdk-linux/usr/share/cmake/OEToolchainConfig.cmake ]; then
    # shellcheck disable=SC1091
    . /opt/webos-sdk-x86_64/1.0.g/environment-setup-armv7a-neon-webos-linux-gnueabi
    TOOLCHAIN_FILE=/opt/webos-sdk-x86_64/1.0.g/sysroots/x86_64-webossdk-linux/usr/share/cmake/OEToolchainConfig.cmake
  else
    echo "TOOLCHAIN_FILE not set. Run tools/webos/setup_ndk.sh first."
    exit 1
  fi
fi

echo "Using toolchain: $TOOLCHAIN_FILE"
if [ -d .git ] || [ -f .git ]; then
  git submodule update --init --recursive || true
fi

mkdir -p "$CMAKE_BINARY_DIR"
BUILD_OPTIONS="-DTARGET_WEBOS=ON -DBUILD_TESTS=OFF"

# shellcheck disable=SC2068
"$CMAKE_BIN" -B"$CMAKE_BINARY_DIR" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" $BUILD_OPTIONS "$@"
"$CMAKE_BIN" --build "$CMAKE_BINARY_DIR" -- -j "$(nproc)"

echo "Packaging IPK..."
# Ensure ares-package is available
if ! command -v ares-package >/dev/null 2>&1; then
  echo "ares-package not found — installing ares-cli-rs..."
  TMP=$(mktemp -d)
  curl -sL "https://api.github.com/repos/webosbrew/ares-cli-rs/releases/latest" \
    | grep -oE 'https://[^"]+ares-package_[^"]+_amd64\.deb' | head -1 \
    | xargs -I{} curl -L -o "$TMP/ares.deb" {}
  sudo apt-get install -y "$TMP/ares.deb"
fi

(
  cd "$CMAKE_BINARY_DIR"
  "$CPACK_BIN"
)

echo "IPKs:"
ls -lh dist/*.ipk 2>/dev/null || ls -lh "$ROOT"/dist/*.ipk
