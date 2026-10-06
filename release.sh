#!/bin/bash
#
# Build the garlic CLI for the platform this machine is and record the release.
#
#   ./release.sh            build and record
#   ./release.sh --check    sanity-check this machine, build nothing
#   ./release.sh -h
#
# This is the second of three. It consumes the engine garlic publishes into
# libs/, builds its own CLI around it, and hands both to Thyme through the
# third_party/garlic symlink. Thyme's release.sh is what packages the result.
#
#   cd ../garlic            && ./release.sh
#   cd .                    && ./release.sh
#   cd ../Thyme             && ./release.sh
#
# The upstream step is not invoked from here: the repositories have separate
# remotes and separate lifecycles, and a script that reaches outside its own
# checkout breaks on a machine that has only one of them.

set -eu

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$REPO_ROOT"
. "./scripts/host.sh"

CHECK_ONLY=0
while [ $# -gt 0 ]; do
    case "$1" in
        --check)   CHECK_ONLY=1 ;;
        -h|--help) sed -n '2,22p' "$0" | sed 's/^# \?//'; exit 0 ;;
        *)         echo "$0: unknown option '$1'" >&2; exit 2 ;;
    esac
    shift
done

host_or_die || exit 1

case "$HOST_PLATFORM" in
    macos-*) lib_ext=.dylib ;;
    linux-*) lib_ext=.so ;;
    win*)    lib_ext=.dll ;;
esac

LIB="${REPO_ROOT}/libs/${HOST_PLATFORM}/librosemarylib${lib_ext}"
DIST="${REPO_ROOT}/dist/${HOST_PLATFORM}"
EXE="${REPO_ROOT}/build/build-${HOST_PLATFORM}/bin/garlic-${HOST_PLATFORM}"

echo "host:      ${HOST_PLATFORM}"
echo "engine:    ${LIB}"
echo "dist:      ${DIST}"
echo

# --- what this build needs, before spending it -------------------------------
#
# The engine is embedded into the CLI as a byte array (see cmake/bin2c.cmake),
# so a missing one does not fail the link - the CLI builds and then cannot
# disassemble anything. Checking here turns that into a failed release with the
# one instruction that fixes it.
if [ ! -f "$LIB" ]; then
    cat >&2 <<EOF
release.sh: no engine for ${HOST_PLATFORM}:

  $LIB

Build and publish it first:

  cd ../garlic && ./release.sh
EOF
    exit 1
fi
verify_artifact "$HOST_PLATFORM" "$LIB" || exit 1
echo "engine:    ok"
echo

if [ "$CHECK_ONLY" = "1" ]; then
    echo "check:     would build ${HOST_PLATFORM}"
    exit 0
fi

# --- build ------------------------------------------------------------------
./build.sh host

if [ ! -x "$EXE" ] && [ ! -f "$EXE" ]; then
    # Windows names it .exe.
    EXE="${EXE}.exe"
    if [ ! -f "$EXE" ]; then
        echo "release.sh: no binary at build/build-${HOST_PLATFORM}/bin/" >&2
        exit 1
    fi
fi

# --- record -----------------------------------------------------------------
rm -rf "$DIST"
mkdir -p "$DIST"
cp "$EXE" "$DIST/"

(
    cd "$DIST"
    sha256_of *.exe *.tar.gz 2>/dev/null > SHA256SUMS || sha256_of * > SHA256SUMS
)

echo
echo "released ${HOST_PLATFORM}:"
ls -1 "$DIST" | sed 's/^/  /'
