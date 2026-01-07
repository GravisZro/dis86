cmake_minimum_required(VERSION 3.16)

set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON" FORCE)

set(TARGET_NAME "dis86")

project(${TARGET_NAME}
    DESCRIPTION ${TARGET_NAME}
    LANGUAGES C CXX
)

set(CMAKE_INCLUDE_CURRENT_DIR OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# output name
add_executable(${TARGET_NAME})

# complier and linker flags

set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")

target_include_directories(${TARGET_NAME} PUBLIC src subprojects)

set(HEADERS_COMMON
  src/common/common.h
  src/common/segment.h
  src/common/dynarray.h
)

set(SOURCES_COMMON
  src/common/common.cpp
  src/common/dynarray.cpp
)

set(HEADERS_BSL
src/bsl/bsl.h
)

set(SOURCES_BSL
src/bsl/bsl.cpp
)

set(HEADERS_DECOMPILER
src/decompile/symbols.h
src/decompile/type.h
src/decompile/util.h
src/decompile/transform.h
src/decompile/config.h
src/decompile/decompile_private.h
src/decompile/labels.h
src/decompile/value.h
src/decompile/expr.h
)

set(HEADERS_DISASSEMBLER
src/array.h
src/instr.h
src/cmdarg/cmdarg.h
src/binary.h
src/header.h
src/datamap.h
src/segoff.h
src/str.h
src/dis86.h
)

set(SOURCES_DECOMPILER
src/decompile/value.cpp
src/decompile/type.cpp
src/decompile/expr.cpp
src/decompile/symbols.cpp
src/decompile/config.cpp
src/decompile/decompile.cpp
src/decompile/transform.cpp
)

set(SOURCES_DISASSEMBLER
src/dis86.cpp
src/datamap.cpp
src/app/main.cpp
src/app/disassembler.cpp
src/app/decompiler.cpp
src/cmdarg/cmdarg.cpp
src/print_intel_syntax.cpp
src/instr.cpp
src/decode.cpp
)


set(HEADERS_PLATFORM
  src/platform/dos.h
)

set(SOURCES_PLATFORM
  src/platform/dos.cpp
)



target_sources(${TARGET_NAME} PUBLIC
  ${HEADERS_DISASSEMBLER} ${SOURCES_DISASSEMBLER}
  ${HEADERS_DECOMPILER} ${SOURCES_DECOMPILER}
  ${HEADERS_BSL} ${SOURCES_BSL}	
  ${HEADERS_COMMON} ${SOURCES_COMMON}
  ${HEADERS_PLATFORM} ${SOURCES_PLATFORM}
)
