# ─── Cross-compile toolchain: PC (x86_64) → Raspberry Pi (aarch64) ───
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Cross-compiler (apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu)
set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Sysroot — Pi'den rsync ile kopyalanmış kök dosya sistemi
# deploy.sh bu değişkeni otomatik ayarlar; manual build için:
#   cmake -DCMAKE_TOOLCHAIN_FILE=... -DPI_SYSROOT=/path/to/pi_sysroot ...
if(DEFINED PI_SYSROOT)
    set(CMAKE_SYSROOT ${PI_SYSROOT})
    set(CMAKE_FIND_ROOT_PATH ${PI_SYSROOT})
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
