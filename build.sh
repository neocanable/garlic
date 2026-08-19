#!/bin/bash

SUPPORTED_PLATFORMS=(
  "android-arm64-v8a"
  "linux-aarch64"
  "linux-i686"
  "linux-x64"
  "macos-aarch64"
  "macos-x64"
  "win32"
  "win64"
)

if [ -z "$1" ]; then
  echo "Usage: $0 <platform_name|all>"
  echo "    Supported platforms:"
  echo "      all"
  for p in "${SUPPORTED_PLATFORMS[@]}"; do
    echo "      ${p}"
  done
  exit 1
fi

TARGET="$1"

if [ "$TARGET" = "all" ]; then
  PLATFORMS=("${SUPPORTED_PLATFORMS[@]}")
else
  if [[ ! " ${SUPPORTED_PLATFORMS[@]} " =~ " ${TARGET} " ]]; then
    echo "Error: Unsupported platform '${TARGET}'"
    echo "    Supported platforms:"
    echo "      all"
    for p in "${SUPPORTED_PLATFORMS[@]}"; do
      echo "      ${p}"
    done
    exit 1
  fi
  PLATFORMS=("${TARGET}")
fi

build_platform() {
  local PLATFORM="$1"

  local BUILD_DIR="build/build-${PLATFORM}"

  mkdir -p "${BUILD_DIR}"

  cd "${BUILD_DIR}" || exit 1

  local TOOLCHAIN_FILE="../../toolchains/toolchain-${PLATFORM}.cmake"

  if [ ! -f "${TOOLCHAIN_FILE}" ]; then
    echo "Error: Toolchain file '${TOOLCHAIN_FILE}' not found!"
    echo "Please make sure the toolchain file exists for platform: ${PLATFORM}"
    exit 1
  fi

  echo "Running CMake with toolchain: ${TOOLCHAIN_FILE}"
  cmake -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" -DPLATFORM_NAME="${PLATFORM}" -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=../ ../../

  echo "Building..."
  cmake --build .

  echo "Build directory for platform '${PLATFORM}' is ready: ${BUILD_DIR}"

  cd ../.. || exit 1
}

for platform in "${PLATFORMS[@]}"; do
  build_platform "${platform}"
done
