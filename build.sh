#!/bin/bash

if [ -z "$1" ]; then
  echo "Usage: $0 <platform_name>"
  echo "    Supported platforms:"
  echo "      linux-aarch64"
  echo "      linux-i686"
  echo "      linux-x64"
  echo "      macos-aarch64"
  echo "      macos-x64"
  echo "      win32"
  echo "      win64"
  exit 1
fi

PLATFORM="$1"

SUPPORTED_PLATFORMS=(
  "linux-aarch64"
  "linux-i686"
  "linux-x64"
  "macos-aarch64"
  "macos-x64"
  "win32"
  "win64"
)

if [[ ! " ${SUPPORTED_PLATFORMS[@]} " =~ " ${PLATFORM} " ]]; then
  echo "Error: Unsupported platform '$PLATFORM'"
  echo "    Supported platforms:"
  echo "      linux-aarch64"
  echo "      linux-i686"
  echo "      linux-x64"
  echo "      macos-aarch64"
  echo "      macos-x64"
  echo "      win32"
  echo "      win64"
  exit 1
fi

BUILD_DIR="build/build-${PLATFORM}"

mkdir -p "${BUILD_DIR}"

cd "${BUILD_DIR}" || exit 1

TOOLCHAIN_FILE="../../toolchains/toolchain-${PLATFORM}.cmake"

if [ ! -f "${TOOLCHAIN_FILE}" ]; then
  echo "Error: Toolchain file '${TOOLCHAIN_FILE}' not found!"
  echo "Please make sure the toolchain file exists for platform: ${PLATFORM}"
  exit 1
fi

echo "Running CMake with toolchain: ${TOOLCHAIN_FILE}"
cmake -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=../ ../../

echo "Building..."
cmake --build .

echo "Build directory for platform '${PLATFORM}' is ready: ${BUILD_DIR}"
