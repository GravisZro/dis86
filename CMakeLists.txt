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

set(HEADERS_BSL
src/bsl/bsl.h
)

set(SOURCES_BSL
src/bsl/bsl.c
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
src/app/exec_mode.h
src/cmdarg/cmdarg.h
src/binary.h
src/header.h
src/datamap.h
src/segoff.h
src/str.h
src/dis86.h
src/instr_tbl.h
src/dis86_private.h
)

set(SOURCES_DECOMPILER
src/decompile/value.c
src/decompile/type.c
src/decompile/expr.c
src/decompile/symbols.c
src/decompile/config.c
src/decompile/decompile.c
src/decompile/transform.c
)

set(SOURCES_DISASSEMBLER
src/core.c
src/datamap.c
src/app/dis86.c
src/app/exec_mode_dis.c
src/app/exec_mode_decomp.c
src/cmdarg/cmdarg.c
src/print_intel_syntax.c
src/instr.c
src/decode.c
)



target_sources(${TARGET_NAME} PUBLIC
  ${HEADERS_DISASSEMBLER} ${SOURCES_DISASSEMBLER}
  ${HEADERS_DECOMPILER} ${SOURCES_DECOMPILER}
  ${HEADERS_BSL} ${SOURCES_BSL}	
)
