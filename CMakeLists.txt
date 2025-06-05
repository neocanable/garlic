cmake_minimum_required(VERSION 3.26)

project(garlic C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter -Wno-unused-but-set-variable")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-implicit-function-declaration -Wno-incompatible-pointer-types")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-misleading-indentation")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--gc-sections")
endif ()

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")

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
aux_source_directory(src/decompiler DECOMPILER)
aux_source_directory(src/decompiler/transformer TRANSFORMER)
aux_source_directory(src/jar JAR)
aux_source_directory(src/jvm JVM)
aux_source_directory(src/libs/trie TRIE)

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
        ${THREADPOOL}
)

add_executable(garlic 
        ${JAVA_DEC}
        src/jvm.c
)

if (UNIX)
    target_link_libraries(garlic pthread)
endif ()

if (WIN32)
    target_link_libraries(garlic ws2_32)
endif ()