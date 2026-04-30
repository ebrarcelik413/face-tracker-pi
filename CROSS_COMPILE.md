# Face Tracker — Cross-Compile Rehberi (Windows → Raspberry Pi)

Windows PC'de WSL2 üzerinden derleyip, Raspberry Pi'ye (aarch64) gönderme adımları.

---

## 0. Ön Koşul: WSL2 Kurulumu (bir kez)

Windows'ta PowerShell'i **yönetici olarak** aç:

```powershell
wsl --install -d Ubuntu-22.04
```

Bilgisayarı yeniden başlat, ardından Ubuntu terminalini aç ve kullanıcı oluştur.

> WSL2 zaten kuruluysa bu adımı atla.

---

## 1. Projeyi PC'ye Kopyala

### Yöntem A — WSL terminalinden (önerilen)

```bash
# WSL Ubuntu terminalinde:
scp -r pi@<PI_IP>:/home/pi/face_tracker ~/face_tracker
```

### Yöntem B — Windows'tan WinSCP / FileZilla ile

1. WinSCP veya FileZilla'yı indir
2. Pi'ye bağlan: `pi@<PI_IP>` (port 22)
3. `/home/pi/face_tracker` klasörünü `\\wsl$\Ubuntu-22.04\home\<user>\` altına kopyala

> **Not:** Projeyi doğrudan WSL dosya sistemine kopyala (`~/`), Windows tarafına (`/mnt/c/`) değil.
> WSL içinden `/mnt/c/` erişimi yavaştır ve derleme sürelerini 5-10x artırır.

---

## 2. WSL İçinde Gerekli Paketleri Kur

```bash
# WSL Ubuntu terminalinde:
sudo apt update
sudo apt install -y \
    cmake \
    build-essential \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    rsync \
    ssh
```

---

## 3. Pi'den Sysroot Kopyala

Cross-compile için Pi'deki OpenCV header ve library dosyaları gereklidir.

```bash
cd ~/face_tracker

# Pi'nin IP adresini ayarla
export PI_USER=pi
export PI_HOST=192.168.1.100    # ← Pi'nin gerçek IP'sini yaz

# Sysroot'u indir (~500MB-1GB, ilk seferde uzun sürebilir)
./scripts/sync_sysroot.sh
```

Bu komut `pi_sysroot/` klasörüne Pi'nin `/usr/include`, `/usr/lib/aarch64-linux-gnu` ve
`/lib/aarch64-linux-gnu` dizinlerini kopyalar.

> **Pi'nin IP adresini bulmak için:** Pi üzerinde `hostname -I` çalıştır.

---

## 4. Cross-Compile

```bash
./scripts/cross_build.sh
```

Çıktı: `build_cross/face_tracker` (aarch64 ELF binary)

Kontrol:
```bash
file build_cross/face_tracker
# ELF 64-bit LSB pie executable, ARM aarch64 ...
```

---

## 5. Pi'ye Gönder ve Çalıştır

```bash
# Sadece gönder:
./scripts/deploy.sh

# Gönder ve hemen çalıştır:
./scripts/deploy.sh --run
```

---

## Ortam Değişkenleri

| Değişken   | Varsayılan              | Açıklama                    |
|------------|-------------------------|-----------------------------|
| `PI_USER`  | `pi`                    | Pi SSH kullanıcısı          |
| `PI_HOST`  | `raspberrypi.local`     | Pi IP adresi veya hostname  |
| `PI_DEST`  | `/home/pi/face_tracker` | Pi'deki proje dizini        |
| `SYSROOT`  | `./pi_sysroot`          | Yerel sysroot dizini        |

---

## Hızlı İş Akışı (Özet)

WSL Ubuntu terminalinde:

```bash
# İlk kurulum (bir kez):
export PI_HOST=192.168.1.100
./scripts/sync_sysroot.sh

# Her değişiklikte:
./scripts/cross_build.sh && ./scripts/deploy.sh --run
```

---

## Windows'tan VS Code ile Geliştirme

1. VS Code'da **Remote - WSL** eklentisini kur
2. WSL terminalinde proje dizinine git: `cd ~/face_tracker`
3. `code .` ile VS Code'u aç — artık IntelliSense + terminal WSL içinde çalışır

---

## Sorun Giderme

### WSL'den Pi'ye SSH bağlanamıyorum
```bash
# Pi'nin IP'sini kontrol et:
ping 192.168.1.100

# Windows Firewall WSL ağ trafiğini engelleyebilir.
# PowerShell (yönetici):
# New-NetFirewallRule -DisplayName "WSL SSH" -Direction Outbound -Action Allow -Protocol TCP -RemotePort 22
```

### "OpenCV bulunamadı" hatası
Sysroot'un doğru indirildiğinden emin ol:
```bash
ls pi_sysroot/usr/lib/aarch64-linux-gnu/cmake/opencv4/
```

### Derleme çok yavaş
Projenin WSL dosya sisteminde (`~/`) olduğundan emin ol, `/mnt/c/` altında değil.

### "raspberrypi.local" çözümlenemedi
mDNS Windows'ta her zaman çalışmaz. Doğrudan IP kullan:
```bash
export PI_HOST=192.168.1.100
```

### Sysroot güncellemesi
Pi'de yeni paket kurulursa, sysroot'u güncelle:
```bash
./scripts/sync_sysroot.sh
```




```bash
scp -r /home/akif/face_tracker pi@192.168.1.107:~/face_tracker
rsync -avz --exclude='build/' --exclude='pi_sysroot/' /home/akif/face_tracker/ pi@192.168.1.107:~/face_tracker/

cmake .. & make -j4 & ./face_tracker

```