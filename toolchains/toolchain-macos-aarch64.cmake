set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR arm64)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_C_FLAGS_INIT "-arch arm64")
set(CMAKE_CXX_FLAGS_INIT "-arch arm64")

set(STRIP_COMMAND strip)
set(PLATFORM_NAME macos-aarch64)

set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "Build architectures for macOS")
