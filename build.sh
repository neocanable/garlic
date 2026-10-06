#!/bin/bash
#
# Build the garlic CLI for one or more platforms.
#
#   ./build.sh                    the host platform - same as `./build.sh host`
#   ./build.sh host               the host platform, named explicitly
#   ./build.sh macos-aarch64      a named platform
#   ./build.sh -n all             print what would be built, build nothing
#   ./build.sh all                every platform this host may build
#
# `all` is deliberately narrow. It means the host platform unless
# GARLIC_ALLOW_CROSS=1 is set. See the header of ../garlic/build.sh: the cross
# toolchains exist on a mac, so `all` used to succeed and produce artifacts that
# could not be run on the machine that made them, which then travelled in
# build/ to the next machine and were mistaken for local builds.
#
# Output lands in build/build-<platform>/bin. It used to land in the shared
# build/ directory, one level up, next to every other platform's binary and next
# to the CMake cache - so `garlic-macos-aarch64` and `garlic-win64.exe` sat in
# the same drawer, and a stale one was indistinguishable from a fresh one.

set -u

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
. "${REPO_ROOT}/scripts/host.sh"

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

usage() {
    cat <<EOF
Usage: $0 [-n|--dry-run] <platform...>

  platform is one of:  host  all
  ${SUPPORTED_PLATFORMS[*]}

  host   the platform this machine is  (${HOST_PLATFORM:-unknown})
  all    the host platform only, unless GARLIC_ALLOW_CROSS=1 is set

  -n, --dry-run   print the resolved platform list and stop
EOF
}

DRY_RUN=0
REQUESTED=""
while [ $# -gt 0 ]; do
    case "$1" in
        -n|--dry-run) DRY_RUN=1 ;;
        -h|--help)    usage; exit 0 ;;
        -*)           echo "$0: unknown option '$1'" >&2; usage >&2; exit 2 ;;
        *)            REQUESTED="$REQUESTED $1" ;;
    esac
    shift
done

host_or_die || exit 1
[ -n "$REQUESTED" ] || REQUESTED=" host"

RESOLVED=""
for p in $REQUESTED; do
    case "$p" in
        host) RESOLVED="$RESOLVED $HOST_PLATFORM" ;;
        all)
            if [ "${GARLIC_ALLOW_CROSS:-0}" = "1" ]; then
                RESOLVED="$RESOLVED ${SUPPORTED_PLATFORMS[*]}"
            else
                RESOLVED="$RESOLVED $HOST_PLATFORM"
            fi ;;
        *)    RESOLVED="$RESOLVED $p" ;;
    esac
done

PLATFORMS=""
for p in $RESOLVED; do
    case " $PLATFORMS " in
        *" $p "*) ;;
        *) PLATFORMS="$PLATFORMS $p" ;;
    esac
done
PLATFORMS="${PLATFORMS# }"

for p in $PLATFORMS; do
    case " ${SUPPORTED_PLATFORMS[*]} " in
        *" $p "*) ;;
        *) echo "$0: unsupported platform '$p'" >&2; usage >&2; exit 2 ;;
    esac
done

if [ "$DRY_RUN" = "1" ]; then
    echo "host:      ${HOST_PLATFORM}"
    echo "platforms: ${PLATFORMS}"
    exit 0
fi

build_platform() {
    local platform="$1"
    local build_dir="${REPO_ROOT}/build/build-${platform}"
    local toolchain="${REPO_ROOT}/toolchains/toolchain-${platform}.cmake"

    if [ ! -f "$toolchain" ]; then
        echo "Error: toolchain file not found: toolchains/toolchain-${platform}.cmake" >&2
        return 1
    fi

    echo "======================================"
    echo "Building ${platform}"
    echo "Toolchain: ${toolchain}"
    echo "======================================"

    mkdir -p "${build_dir}/bin" || return 1

    # Subshell, so a failure here cannot leave the caller in the build dir - and
    # so that one platform failing does not end the run for the others. This
    # used to `exit 1` on a missing toolchain, which killed the whole loop.
    (
        cd "$build_dir" || exit 1
        cmake \
            -DCMAKE_TOOLCHAIN_FILE="${toolchain}" \
            -DPLATFORM_NAME="${platform}" \
            -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="${build_dir}/bin" \
            "${REPO_ROOT}" || exit 1
        cmake --build . || exit 1
    ) || {
        echo "Error: build failed for ${platform}" >&2
        return 1
    }

    stamp_write "$platform" "$build_dir" "$REPO_ROOT"

    echo "Build directory for platform '${platform}' is ready: ${build_dir}"
    return 0
}

FAILED=""
for p in $PLATFORMS; do
    build_platform "$p" || FAILED="$FAILED $p"
done

if [ -n "$FAILED" ]; then
    echo
    echo "Failed platforms:${FAILED}" >&2
    exit 1
fi

echo
echo "Built: ${PLATFORMS}"
