# cmake/bin2c.cmake
# Convert a binary file to a C source file with the data as a const array.
# Usage:
#   cmake -DINPUT=<binfile> -DOUTPUT=<cfile> -DVARNAME=<varname> -P bin2c.cmake

cmake_minimum_required(VERSION 3.16)

if(NOT INPUT OR NOT OUTPUT OR NOT VARNAME)
    message(FATAL_ERROR "INPUT, OUTPUT, and VARNAME are required")
endif()

# Read the binary as a hex string (e.g. "48656c6c6f" for "Hello")
file(READ "${INPUT}" hex HEX)
string(LENGTH "${hex}" hex_len)
math(EXPR nbytes "${hex_len} / 2")

# "AABB" → "0xAA,0xBB,"  (one regex pass over the whole hex string)
string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1," c_array "${hex}")
# Remove the trailing comma
string(REGEX REPLACE ",$" "" c_array "${c_array}")

file(WRITE "${OUTPUT}"
    "/* Auto-generated from ${INPUT} */\n"
    "#include <stddef.h>\n"
    "const unsigned char ${VARNAME}[] = {\n"
    "  ${c_array}\n"
    "};\n"
    "const size_t ${VARNAME}_len = ${nbytes};\n"
)

message(STATUS "bin2c: ${INPUT} -> ${OUTPUT}  (${nbytes} bytes)")
