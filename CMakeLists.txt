cmake_minimum_required(VERSION 3.26)

project(garlic C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-function")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-implicit-function-declaration")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-incompatible-pointer-types")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-misleading-indentation")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-discarded-qualifiers")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-braces")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-parentheses")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-cpp")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-but-set-variable")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-warning-option")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-variable")

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--gc-sections")
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")

if(NOT WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-ignored-optimization-argument")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
else()
    # MinGW ld needs --enable-stdcall-fixup to resolve Win32 API calls
    # (e.g. CreateDirectoryA -> CreateDirectoryA@8) without discarding
    # the import-stub sections from libkernel32.a.
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--enable-stdcall-fixup")
endif ()

if (UNIX)
    SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -pthread")
endif ()

include_directories(src/common)
include_directories(src/libs/memory)
include_directories(src/libs/hashmap)
include_directories(src/libs/list)
include_directories(src/libs/bitset)
include_directories(src/libs/queue)
include_directories(src/libs/str)
include_directories(src/libs/zip)
include_directories(src/libs/threadpool)
include_directories(src/libs/cjson)
include_directories(src/libs/md4c)
include_directories(libs/include)
include_directories(src/ai)
include_directories(src/jar)
include_directories(src/dalvik)
include_directories(src)

aux_source_directory(src/common COMMON)
aux_source_directory(src/libs/list LIST)
aux_source_directory(src/libs/hashmap HASHMAP)
aux_source_directory(src/parser/class/ CLASS_PARSER)
aux_source_directory(src/parser/dex DEX_PARSER)
aux_source_directory(src/parser/pe PE_PARSER)
aux_source_directory(src/libs/queue DQUEUE)
aux_source_directory(src/libs/str STR)
aux_source_directory(src/libs/memory MEMORY_POOL)
aux_source_directory(src/libs/zip ZIP)
aux_source_directory(src/libs/threadpool THREADPOOL)
aux_source_directory(src/libs/bitset BITSET)
aux_source_directory(src/libs/cjson CJSON)
aux_source_directory(src/decompiler DECOMPILER)
aux_source_directory(src/decompiler/transformer TRANSFORMER)
aux_source_directory(src/jar JAR)
aux_source_directory(src/jvm JVM)
aux_source_directory(src/apk APK)
aux_source_directory(src/dalvik DEX)
aux_source_directory(src/libs/trie TRIE)
aux_source_directory(src/analyzer ANALYZER)
aux_source_directory(src/ai AI)
set(MD4C
    src/libs/md4c/md4c.c
    src/libs/md4c/md4c-html.c
    src/libs/md4c/entity.c
)

set(REPORT
    src/report/report.c
)

set(HTTP_SERVER
    src/ai/http_server.c
)

set(JAVA_DEC
        ${COMMON}
        ${MEMORY_POOL}
        ${LIST}
        ${HASHMAP}
        ${BITSET}
        ${DQUEUE}
        ${STR}
        ${TRIE}
        ${DECOMPILER}
        ${TRANSFORMER}
        ${CLASS_PARSER}
        ${DEX_PARSER}
        ${JVM}
        ${ZIP}
        ${JAR}
        ${APK}
        ${DEX}
        ${THREADPOOL}
        ${CJSON}
        ${AI}
        ${ANALYZER}
        ${MD4C}
        ${REPORT}
        ${HTTP_SERVER}
)

# ---------------------------------------------------------------------------
# librosemarylib — locate the pre-built shared library for the target
# platform, then embed it as a C data array so the garlic binary is
# self-contained (extracted to temp + dlopen'd at runtime).
# ---------------------------------------------------------------------------

if(DEFINED PLATFORM_NAME)
    # Cross-compilation: PLATFORM_NAME matches the lib subdirectory
    # (linux-aarch64, macos-x64, win64, etc.)
    if(PLATFORM_NAME MATCHES "^win")
        set(ROSEMARYLIB_FILE "${CMAKE_SOURCE_DIR}/libs/${PLATFORM_NAME}/librosemarylib.dll")
    elseif(PLATFORM_NAME MATCHES "^macos")
        set(ROSEMARYLIB_FILE "${CMAKE_SOURCE_DIR}/libs/${PLATFORM_NAME}/librosemarylib.dylib")
    else()
        set(ROSEMARYLIB_FILE "${CMAKE_SOURCE_DIR}/libs/${PLATFORM_NAME}/librosemarylib.so")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(ROSEMARYLIB_FILE "${CMAKE_SOURCE_DIR}/libs/linux-aarch64/librosemarylib.so")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
        set(ROSEMARYLIB_FILE "${CMAKE_SOURCE_DIR}/libs/linux-x64/librosemarylib.so")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i686|i386")
        set(ROSEMARYLIB_FILE "${CMAKE_SOURCE_DIR}/libs/linux-i686/librosemarylib.so")
    else()
        message(FATAL_ERROR "Unsupported Linux architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(ROSEMARYLIB_FILE "${CMAKE_SOURCE_DIR}/libs/macos-aarch64/librosemarylib.dylib")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        set(ROSEMARYLIB_FILE "${CMAKE_SOURCE_DIR}/libs/macos-x64/librosemarylib.dylib")
    else()
        message(FATAL_ERROR "Unsupported macOS architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        set(ROSEMARYLIB_FILE "${CMAKE_SOURCE_DIR}/libs/win64/librosemarylib.dll")
    else()
        set(ROSEMARYLIB_FILE "${CMAKE_SOURCE_DIR}/libs/win32/librosemarylib.dll")
    endif()
else()
    message(FATAL_ERROR "Unsupported operating system: ${CMAKE_SYSTEM_NAME}")
endif()

if(ROSEMARYLIB_FILE AND EXISTS "${ROSEMARYLIB_FILE}")
    # Convert the shared library to a C source file using a pure-CMake
    # script so that no external tool (xxd, Python, etc.) is needed.
    set(EMBED_SRC ${CMAKE_BINARY_DIR}/rosemarylib_data.c)
    add_custom_command(
        OUTPUT ${EMBED_SRC}
        COMMAND ${CMAKE_COMMAND}
                -DINPUT="${ROSEMARYLIB_FILE}"
                -DOUTPUT=${EMBED_SRC}
                -DVARNAME=rosemarylib_data
                -P ${CMAKE_SOURCE_DIR}/cmake/bin2c.cmake
        MAIN_DEPENDENCY "${ROSEMARYLIB_FILE}"
        COMMENT "Embedding ${ROSEMARYLIB_FILE} into garlic binary"
    )
    add_compile_definitions(ROSEMARYLIB_EMBEDDED=1)
    message(STATUS "Embedding librosemarylib: ${ROSEMARYLIB_FILE}")
else()
    # Without a pre-built library the ELF-analysis (-n) feature is disabled
    message(STATUS "librosemarylib embedding disabled (library not found: ${ROSEMARYLIB_FILE})")
endif()

# -------------------------------------------------------------------------

add_executable(garlic
        ${JAVA_DEC}
        src/garlic.c
        src/rosemary/rosemary_embed.c
)

if(ROSEMARYLIB_FILE AND EXISTS "${ROSEMARYLIB_FILE}")
    target_sources(garlic PRIVATE ${CMAKE_BINARY_DIR}/rosemarylib_data.c)
endif()


if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(garlic pthread dl)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_link_libraries(garlic pthread)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Android")
  # pthread is part of bionic libc (no libpthread stub since NDK r27);
  # libdl is only needed when the embedded ELF analyzer is present.
  target_link_libraries(garlic dl)
elseif (WIN32)
  target_link_libraries(garlic ws2_32 advapi32)
endif ()


if (RELEASE)
    if (DEFINED PLATFORM_NAME)
    add_executable(garlic-${PLATFORM_NAME}
            ${JAVA_DEC}
            src/garlic.c
            src/rosemary/rosemary_embed.c
    )

    if(ROSEMARYLIB_FILE AND EXISTS "${ROSEMARYLIB_FILE}")
        target_sources(garlic-${PLATFORM_NAME} PRIVATE ${CMAKE_BINARY_DIR}/rosemarylib_data.c)
    endif()
    endif()


    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        if(DEFINED PLATFORM_NAME)
            target_link_libraries(garlic-${PLATFORM_NAME} pthread dl)
        else()
            target_link_libraries(garlic pthread dl)
        endif()
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        if(DEFINED PLATFORM_NAME)
            target_link_libraries(garlic-${PLATFORM_NAME} pthread)
        else()
            target_link_libraries(garlic pthread)
        endif()
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Android")
        # pthread is part of bionic libc (no libpthread stub since NDK r27);
        # libdl is only needed when the embedded ELF analyzer is present.
        if(DEFINED PLATFORM_NAME)
            target_link_libraries(garlic-${PLATFORM_NAME} dl)
        else()
            target_link_libraries(garlic dl)
        endif()
    elseif (WIN32)
        if (DEFINED PLATFORM_NAME)
            target_link_libraries(garlic-${PLATFORM_NAME} ws2_32 advapi32)
        else()
             target_link_libraries(garlic ws2_32 advapi32)
        endif()
    endif ()

    if (DEFINED STRIP_COMMAND)
        if (DEFINED PLATFORM_NAME)
            add_custom_command(
                    TARGET garlic-${PLATFORM_NAME} POST_BUILD
                    COMMAND ${STRIP_COMMAND} $<TARGET_FILE:garlic-${PLATFORM_NAME}>
            )
        else()
            add_custom_command(
                    TARGET garlic POST_BUILD
                    COMMAND ${STRIP_COMMAND} $<TARGET_FILE:garlic>
            )
        endif()
    endif ()

endif ()
