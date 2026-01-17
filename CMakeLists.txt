cmake_minimum_required(VERSION 3.16)


SET(RUN_TESTS "option_value" OFF CACHE BOOL "Run validation tests?")

set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON" FORCE)

set(TARGET_NAME "dis86")

project(${TARGET_NAME}
    DESCRIPTION ${TARGET_NAME}
    LANGUAGES C CXX
)

set(CMAKE_INCLUDE_CURRENT_DIR OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# output name
add_executable(${TARGET_NAME})

# complier and linker flags

set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")

include(CheckIncludeFileCXX)
check_include_file_cxx(format HAVE_STDFORMAT_HEADER CMAKE_REQUIRED_QUIET)

if(HAVE_STDFORMAT_HEADER)
  target_include_directories(${TARGET_NAME} PUBLIC src subprojects)
else()
  target_include_directories(${TARGET_NAME} PUBLIC src subprojects polyfill)
endif()

set(COMMON_HEADERS
  src/common/segment.h
  src/common/expanded_base.h
  src/common/result.h
  src/common/fileio.h
  src/common/dynarray.h
)

set(COMMON_SOURCES
  src/common/dynarray.cpp
)

set(ASM_HEADERS
  src/region.h
  src/segoff.h
  src/asm/instr.h
  src/asm/instr_fmt.h
  src/asm/decode.h
  src/asm/intel_syntax.h
)

set(ASM_SOURCES
  src/region.cpp
  src/segoff.cpp
  src/asm/instr.cpp
  src/asm/instr_fmt.cpp
  src/asm/decode.cpp
  src/asm/intel_syntax.cpp
)

set(BINFMT_HEADERS
  src/binfmt/mz/defs.h
  src/binfmt/mz/exe.h
  src/binfmt/mz/overlay.h
)

set(BINFMT_SOURCES
  src/binfmt/mz/exe.cpp
  src/binfmt/mz/overlay.cpp
)

set(IR_HEADERS
  src/util/dvec.h
  src/ir/opcode.h
  src/ir/defs.h
  src/ir/sym.h
  src/ir/ir.h
)

set(IR_SOURCES
  src/util/dvec.cpp
  src/ir/opcode.cpp
  src/ir/defs.cpp
  src/ir/sym.cpp
  src/ir/ir.cpp
)


set(HEADERS
  src/bsl/bsl.h
  src/binary.h
  src/types.h
  src/config.h
  src/spec.h
  src/region.h
  src/segoff.h
)

set(SOURCES
  src/bsl/bsl.cpp
  src/binary.cpp
  src/types.cpp
  src/config.cpp
  src/spec.cpp
  src/region.cpp
  src/segoff.cpp
)

if(RUN_TESTS)
  add_definitions(-DENABLE_TESTS)
  set(TEST_SOURCES
    src/bsl/bsl_test.cpp
    src/test.cpp)
endif()

set(HEADERS_PLATFORM
  src/platform/dos.h
)

set(SOURCES_PLATFORM
  src/platform/dos.cpp
)



target_sources(${TARGET_NAME} PUBLIC
  ${COMMON_HEADERS} ${COMMON_SOURCES}
  ${HEADERS} ${SOURCES}
  ${ASM_HEADERS} ${ASM_SOURCES}
  ${BINFMT_HEADERS} ${BINFMT_SOURCES}
  ${IR_HEADERS} ${IR_SOURCES}
  ${TEST_SOURCES}
)
