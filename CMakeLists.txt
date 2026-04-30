cmake_minimum_required(VERSION 3.16)
project(FaceTracker)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

find_package(OpenCV REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(face_tracker
    src/main.cpp
    src/crypto_service.cpp
    src/cloud_service.cpp
    src/control_service.cpp
    src/kalman_tracker.cpp
)

target_link_libraries(face_tracker
    ${OpenCV_LIBS}
    ${OPENSSL_LIBRARIES}
    pthread
)