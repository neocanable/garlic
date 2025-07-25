#!/bin/bash

# 检查是否传入参数
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

# 获取参数
PLATFORM="$1"

# 定义支持的平台列表
SUPPORTED_PLATFORMS=(
  "linux-aarch64"
  "linux-i686"
  "linux-x64"
  "macos-aarch64"
  "macos-x64"
  "win32"
  "win64"
)

# 检查参数是否合法
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

# 构建目录
BUILD_DIR="build/build-${PLATFORM}"

# 创建构建目录
mkdir -p "${BUILD_DIR}"

# 进入构建目录
cd "${BUILD_DIR}" || exit 1

# 根据平台设置工具链文件路径
TOOLCHAIN_FILE="../../toolchains/toolchain-${PLATFORM}.cmake"

# 检查工具链文件是否存在
if [ ! -f "${TOOLCHAIN_FILE}" ]; then
  echo "Error: Toolchain file '${TOOLCHAIN_FILE}' not found!"
  echo "Please make sure the toolchain file exists for platform: ${PLATFORM}"
  exit 1
fi

# 执行 CMake 配置（这里只是示例，你可以根据需要扩展）
echo "Running CMake with toolchain: ${TOOLCHAIN_FILE}"
cmake -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=../ ../../

# 可选：执行构建
echo "Building..."
cmake --build .

echo "Build directory for platform '${PLATFORM}' is ready: ${BUILD_DIR}"
