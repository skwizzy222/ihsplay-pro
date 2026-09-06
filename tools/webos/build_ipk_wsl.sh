#!/usr/bin/env bash
set -euo pipefail
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

SRC_WIN=/mnt/c/Users/Michael/Projects/ihsplay-pro
SRC=/tmp/ihsplay-src
BUILD=/tmp/ihsplay-build
TOOLCHAIN_FILE=$HOME/webos-ndk/arm-webos-linux-gnueabi_sdk-buildroot/usr/share/buildroot/toolchainfile.cmake

echo "Syncing sources to $SRC ..."
rm -rf "$SRC"
mkdir -p "$SRC"
rsync -a \
  --exclude build-desktop \
  --exclude build-webos \
  --exclude dist \
  --exclude '.git' \
  "$SRC_WIN/" "$SRC/"

test -f "$SRC/core/include/ihslib/client.h"
echo "0.3.0" > "$SRC/VERSION"

# Make CMake prefer VERSION file
python3 - <<'PY'
from pathlib import Path
p = Path("/tmp/ihsplay-src/CMakeLists.txt")
text = p.read_text()
old = '''execute_process(COMMAND git describe --tags --abbrev=0 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE IHSPLAY_VERSION ERROR_QUIET)
if (IHSPLAY_VERSION)
    string(STRIP "${IHSPLAY_VERSION}" IHSPLAY_VERSION)
    string(SUBSTRING "${IHSPLAY_VERSION}" 1 -1 IHSPLAY_VERSION)
else ()
    set(IHSPLAY_VERSION "0.3.0")
endif ()'''
new = '''if (EXISTS "${CMAKE_SOURCE_DIR}/VERSION")
    file(READ "${CMAKE_SOURCE_DIR}/VERSION" IHSPLAY_VERSION)
    string(STRIP "${IHSPLAY_VERSION}" IHSPLAY_VERSION)
elseif ()
    execute_process(COMMAND git describe --tags --abbrev=0 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE IHSPLAY_VERSION ERROR_QUIET)
    if (IHSPLAY_VERSION)
        string(STRIP "${IHSPLAY_VERSION}" IHSPLAY_VERSION)
        string(SUBSTRING "${IHSPLAY_VERSION}" 1 -1 IHSPLAY_VERSION)
    else ()
        set(IHSPLAY_VERSION "0.3.0")
    endif ()
endif ()'''
# Fix invalid elseif () - use else
new = new.replace('elseif ()', 'else ()')
if old in text:
    p.write_text(text.replace(old, new))
    print("Patched CMakeLists.txt for VERSION file")
else:
    print("CMakeLists version block not matched; continuing")
PY

export TOOLCHAIN_FILE
export CMAKE_BINARY_DIR="$BUILD"
rm -rf "$BUILD"

cd "$SRC"
chmod +x tools/webos/*.sh
sed -i 's/\r$//' tools/webos/*.sh
bash tools/webos/build_ipk.sh -DCMAKE_BUILD_TYPE=Release

mkdir -p "$SRC_WIN/dist"
cp -f "$SRC"/dist/*.ipk "$SRC_WIN/dist/"
ls -lh "$SRC_WIN"/dist/*.ipk
echo BUILD_IPK_OK
