# Android NDK toolchain — arm64-v8a (aarch64, linux-android)
#
# Requirements:
#   - Android NDK installed (r21+; tested against r28)
#   - Point the toolchain at it via one of:
#       * cmake -DANDROID_NDK=/path/to/ndk ...
#       * env ANDROID_NDK_HOME  (or ANDROID_NDK_ROOT)
#       * env ANDROID_HOME / ANDROID_SDK_ROOT  (looks under <sdk>/ndk/<version>)
#     The standard macOS/Android Studio location is auto-detected as a fallback.
#
# minSdkVersion (Android API level) is set via CMAKE_SYSTEM_VERSION below.

set(CMAKE_SYSTEM_NAME Android)
# minSdkVersion: 23 — garlic uses open_memstream(), which bionic only
# provides from API 23 onwards (Android 6.0, 2015).
set(CMAKE_SYSTEM_VERSION 23)

set(CMAKE_ANDROID_ARCH_ABI arm64-v8a) # aarch64-linux-android<api>-clang
set(CMAKE_ANDROID_STL_TYPE none)      # C-only project

# ---------------------------------------------------------------------------
# Locate the Android NDK.
# ---------------------------------------------------------------------------
if(DEFINED ANDROID_NDK AND NOT "${ANDROID_NDK}" STREQUAL "")
    set(_GARLIC_NDK "${ANDROID_NDK}")
elseif(DEFINED ENV{ANDROID_NDK_HOME} AND NOT "$ENV{ANDROID_NDK_HOME}" STREQUAL "")
    set(_GARLIC_NDK "$ENV{ANDROID_NDK_HOME}")
elseif(DEFINED ENV{ANDROID_NDK_ROOT} AND NOT "$ENV{ANDROID_NDK_ROOT}" STREQUAL "")
    set(_GARLIC_NDK "$ENV{ANDROID_NDK_ROOT}")
else()
    # Fall back to the Android SDK (env) and the default install locations.
    set(_GARLIC_NDK_SEARCH "")
    foreach(_sdk_var ANDROID_HOME ANDROID_SDK_ROOT)
        if(DEFINED ENV{${_sdk_var}} AND NOT "$ENV{${_sdk_var}}" STREQUAL "")
            file(GLOB _sdk_ndk "$ENV{${_sdk_var}}/ndk/*")
            if(_sdk_ndk)
                list(APPEND _GARLIC_NDK_SEARCH ${_sdk_ndk})
            endif()
        endif()
    endforeach()
    foreach(_default_ndk
            "$ENV{HOME}/Library/Android/sdk/ndk/*"
            "$ENV{HOME}/Android/Sdk/ndk/*")
        file(GLOB _sdk_ndk ${_default_ndk})
        if(_sdk_ndk)
            list(APPEND _GARLIC_NDK_SEARCH ${_sdk_ndk})
        endif()
    endforeach()

    if(_GARLIC_NDK_SEARCH)
        # Pick the highest-numbered NDK version.
        list(SORT _GARLIC_NDK_SEARCH)
        list(REVERSE _GARLIC_NDK_SEARCH)
        list(GET _GARLIC_NDK_SEARCH 0 _GARLIC_NDK)
    endif()
endif()

if(NOT _GARLIC_NDK OR NOT EXISTS "${_GARLIC_NDK}")
    message(FATAL_ERROR
        "Android NDK not found. Set ANDROID_NDK_HOME (or ANDROID_NDK_ROOT) to the "
        "NDK root directory, or pass -DANDROID_NDK=/path/to/ndk.")
endif()

set(CMAKE_ANDROID_NDK "${_GARLIC_NDK}")

# ---------------------------------------------------------------------------
# Strip binary — llvm-strip lives in the NDK host toolchain, not on PATH.
# ---------------------------------------------------------------------------
set(_GARLIC_NDK_HOST_TAG "")
foreach(_host darwin-arm64 darwin-x86_64 linux-x86_64 windows-x86_64)
    if(EXISTS "${_GARLIC_NDK}/toolchains/llvm/prebuilt/${_host}")
        set(_GARLIC_NDK_HOST_TAG "${_host}")
        break()
    endif()
endforeach()
if(NOT _GARLIC_NDK_HOST_TAG)
    message(FATAL_ERROR
        "Could not determine the NDK host toolchain under "
        "${_GARLIC_NDK}/toolchains/llvm/prebuilt/")
endif()

set(STRIP_COMMAND "${_GARLIC_NDK}/toolchains/llvm/prebuilt/${_GARLIC_NDK_HOST_TAG}/bin/llvm-strip")
set(PLATFORM_NAME android-arm64-v8a)
set(RELEASE TRUE)
