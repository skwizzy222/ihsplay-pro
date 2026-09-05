#!/usr/bin/env bash
# Generate Homebrew Channel repository index from a built IPK + metadata.
# Usage: ./tools/webos/gen_homebrew_repo.sh dist/org.ihsplay.pro_*_arm.ipk
set -euo pipefail

IPK="${1:?IPK path required}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT_DIR="${ROOT}/homebrew"
VERSION="$(basename "$IPK" | sed -E 's/^org\.ihsplay\.pro_([^_]+)_arm\.ipk$/\1/')"
SHA256="$(sha256sum "$IPK" | awk '{print $1}')"
ICON_URL="https://raw.githubusercontent.com/skwizzy222/ihsplay-pro/main/deploy/webos/icon.png"
SOURCE_URL="https://github.com/skwizzy222/ihsplay-pro"
RELEASE_BASE="https://github.com/skwizzy222/ihsplay-pro/releases/download/v${VERSION}"

mkdir -p "${OUT_DIR}"
cp -f "$IPK" "${OUT_DIR}/"

cat > "${OUT_DIR}/org.ihsplay.pro.manifest.json" <<EOF
{
  "id": "org.ihsplay.pro",
  "version": "${VERSION}",
  "type": "native",
  "title": "Steam Remote Play",
  "appDescription": "Steam Remote Play client for webOS (IHSplay Pro)",
  "iconUri": "${ICON_URL}",
  "sourceUrl": "${SOURCE_URL}",
  "rootRequired": false,
  "ipkUrl": "${RELEASE_BASE}/org.ihsplay.pro_${VERSION}_arm.ipk",
  "ipkHash": {
    "sha256": "${SHA256}"
  }
}
EOF

cat > "${OUT_DIR}/repo.json" <<EOF
{
  "packages": [
    {
      "id": "org.ihsplay.pro",
      "title": "Steam Remote Play",
      "iconUri": "${ICON_URL}",
      "manifestUrl": "${RELEASE_BASE}/org.ihsplay.pro.manifest.json",
      "manifest": {
        "id": "org.ihsplay.pro",
        "version": "${VERSION}",
        "type": "native",
        "title": "Steam Remote Play",
        "appDescription": "Steam Remote Play client for webOS (IHSplay Pro) — Steam-styled UI, manual IP, streaming PIN, cancel while connecting.",
        "iconUri": "${ICON_URL}",
        "sourceUrl": "${SOURCE_URL}",
        "rootRequired": false,
        "ipkUrl": "${RELEASE_BASE}/org.ihsplay.pro_${VERSION}_arm.ipk",
        "ipkHash": {
          "sha256": "${SHA256}"
        }
      }
    }
  ]
}
EOF

# Stable raw URL for Homebrew Channel (points at latest committed homebrew/repo.json on main)
# Users add: https://raw.githubusercontent.com/skwizzy222/ihsplay-pro/main/homebrew/repo.json

echo "Generated Homebrew Channel files in ${OUT_DIR}"
echo "SHA256=${SHA256}"
echo "VERSION=${VERSION}"
