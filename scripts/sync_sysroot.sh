#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────
#  sync_sysroot.sh — Raspberry Pi'den cross-compile için
#  gerekli header ve library dosyalarını PC'ye kopyalar
# ─────────────────────────────────────────────────────────────
set -euo pipefail

PI_USER="${PI_USER:-pi}"
PI_HOST="${PI_HOST:-raspberrypi.local}"
SYSROOT="${SYSROOT:-$(pwd)/pi_sysroot}"

echo "=== Pi sysroot senkronizasyonu ==="
echo "  Pi : ${PI_USER}@${PI_HOST}"
echo "  Hedef: ${SYSROOT}"

mkdir -p "${SYSROOT}/usr/include" \
        "${SYSROOT}/usr/lib/aarch64-linux-gnu/pkgconfig" \
        "${SYSROOT}/usr/share/opencv4" \
        "${SYSROOT}/lib/aarch64-linux-gnu"

# OpenCV headers + libs + system libs
rsync -avz --delete \
    "${PI_USER}@${PI_HOST}:/usr/include/"            "${SYSROOT}/usr/include/"

rsync -avz --delete \
    "${PI_USER}@${PI_HOST}:/usr/lib/aarch64-linux-gnu/" "${SYSROOT}/usr/lib/aarch64-linux-gnu/"

rsync -avz --delete \
    "${PI_USER}@${PI_HOST}:/usr/share/opencv4/"      "${SYSROOT}/usr/share/opencv4/" 2>/dev/null || true

rsync -avz --delete \
    "${PI_USER}@${PI_HOST}:/lib/aarch64-linux-gnu/"  "${SYSROOT}/lib/aarch64-linux-gnu/"

# pkg-config dosyaları
rsync -avz \
    "${PI_USER}@${PI_HOST}:/usr/lib/aarch64-linux-gnu/pkgconfig/" \
    "${SYSROOT}/usr/lib/aarch64-linux-gnu/pkgconfig/" 2>/dev/null || true

# Symlink düzelt — mutlak linkler sysroot'a göre göreceli olmalı
echo "=== Symlink'ler düzeltiliyor ==="
find "${SYSROOT}" -type l | while read -r link; do
    target=$(readlink "$link")
    if [[ "$target" == /* ]]; then
        new_target="${SYSROOT}${target}"
        if [[ -e "$new_target" ]]; then
            ln -sf "$new_target" "$link"
        fi
    fi
done

echo "=== Sysroot hazır: ${SYSROOT} ==="
