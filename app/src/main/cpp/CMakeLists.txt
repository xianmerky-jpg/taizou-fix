cmake_minimum_required(VERSION 3.22.1)
project("taizou_core" LANGUAGES CXX)

# ABI is driven by Gradle (ndk.abiFilters = arm64-v8a). Do not hard-code
# CMAKE_ANDROID_ARCH_ABI/ANDROID_ABI here; Gradle passes the correct
# -DANDROID_ABI per build, and a hard-coded value would silently conflict
# if the filter list ever changes.

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
