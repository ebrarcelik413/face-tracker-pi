#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────
#  cross_build.sh — PC üzerinde aarch64 için cross-compile
# ─────────────────────────────────────────────────────────────
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build_cross"
SYSROOT="${SYSROOT:-${PROJECT_DIR}/pi_sysroot}"
TOOLCHAIN="${PROJECT_DIR}/cmake/aarch64-linux-gnu.toolchain.cmake"

echo "=== Cross-build başlıyor ==="
echo "  Proje  : ${PROJECT_DIR}"
echo "  Sysroot: ${SYSROOT}"
echo "  Build  : ${BUILD_DIR}"

if [[ ! -d "${SYSROOT}/usr/include" ]]; then
    echo "[HATA] Sysroot bulunamadı: ${SYSROOT}"
    echo "  Önce çalıştır: ./scripts/sync_sysroot.sh"
    exit 1
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake "${PROJECT_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
    -DPI_SYSROOT="${SYSROOT}" \
    -DCMAKE_BUILD_TYPE=Release

make -j"$(nproc)"

echo ""
echo "=== Build tamamlandı ==="
echo "  Binary: ${BUILD_DIR}/face_tracker"
file "${BUILD_DIR}/face_tracker"
