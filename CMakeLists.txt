cmake_minimum_required(VERSION 3.16)
project(FaceTracker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

# Cross-compile: sysroot içinde OpenCV'yi bul
if(DEFINED PI_SYSROOT)
    set(OpenCV_DIR "${PI_SYSROOT}/usr/lib/aarch64-linux-gnu/cmake/opencv4"
        CACHE PATH "OpenCV cmake dir in sysroot")
endif()

find_package(OpenCV REQUIRED)

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(face_tracker
    src/main.cpp
)

target_link_libraries(face_tracker
    ${OpenCV_LIBS}
    pthread
)