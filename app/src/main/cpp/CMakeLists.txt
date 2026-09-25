cmake_minimum_required(VERSION 3.22.1)
project("taizou_core" LANGUAGES CXX)

# This project is distributed with ARM64 native payloads. Build only the matching ABI.
set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -fPIC -frtti -fexceptions")
# Android 15 / 16KB page size support
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--exclude-libs,ALL -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384")

add_library(taizou_core SHARED
    taizou_core.cpp
)

target_include_directories(taizou_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(taizou_core
    android
    log
    dl
)
