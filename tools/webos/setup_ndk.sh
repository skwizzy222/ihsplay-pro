#!/usr/bin/env bash
set -euo pipefail
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

mkdir -p /tmp/webos-ndk
cd /tmp/webos-ndk

ARCHIVE=arm-webos-linux-gnueabi_sdk-buildroot-x86_64.tar.gz
URL=https://github.com/openlgtv/buildroot-nc4/releases/download/webos-a38c582/${ARCHIVE}
SDK_DIR=arm-webos-linux-gnueabi_sdk-buildroot

if [ ! -f "$ARCHIVE" ]; then
  echo "Downloading NDK..."
  curl -L --fail -o "$ARCHIVE" "$URL"
fi

ls -lh "$ARCHIVE"

if [ ! -d "$SDK_DIR" ]; then
  echo "Extracting..."
  tar xzf "$ARCHIVE"
  ./"$SDK_DIR"/relocate-sdk.sh
fi

test -f "$SDK_DIR/usr/share/buildroot/toolchainfile.cmake"
echo "TOOLCHAIN_FILE=$PWD/$SDK_DIR/usr/share/buildroot/toolchainfile.cmake"
echo NDK_OK
