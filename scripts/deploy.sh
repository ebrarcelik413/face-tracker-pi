#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────
#  deploy.sh — Cross-compile edilmiş binary'yi Pi'ye gönder
#  ve çalıştır
# ─────────────────────────────────────────────────────────────
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build_cross"
BINARY="${BUILD_DIR}/face_tracker"

PI_USER="${PI_USER:-pi}"
PI_HOST="${PI_HOST:-raspberrypi.local}"
PI_DEST="${PI_DEST:-/home/pi/face_tracker}"

if [[ ! -f "${BINARY}" ]]; then
    echo "[HATA] Binary bulunamadı: ${BINARY}"
    echo "  Önce çalıştır: ./scripts/cross_build.sh"
    exit 1
fi

echo "=== Deploy ==="
echo "  Binary : ${BINARY}"
echo "  Hedef  : ${PI_USER}@${PI_HOST}:${PI_DEST}/"

# Binary gönder
scp "${BINARY}" "${PI_USER}@${PI_HOST}:${PI_DEST}/build/face_tracker"

# Cascade dosyasını da gönder (güncellenmiş olabilir)
scp -r "${PROJECT_DIR}/configs/" "${PI_USER}@${PI_HOST}:${PI_DEST}/configs/"

echo "=== Binary gönderildi ==="

# Opsiyonel: doğrudan çalıştır
if [[ "${1:-}" == "--run" ]]; then
    echo "=== Pi üzerinde çalıştırılıyor ==="
    ssh -t "${PI_USER}@${PI_HOST}" "cd ${PI_DEST} && ./build/face_tracker"
else
    echo ""
    echo "Çalıştırmak için:"
    echo "  ssh ${PI_USER}@${PI_HOST} 'cd ${PI_DEST} && ./build/face_tracker'"
    echo ""
    echo "Veya: ./scripts/deploy.sh --run"
fi
