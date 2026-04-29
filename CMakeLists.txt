cmake_minimum_required(VERSION 3.20)
project(CafeRecomp CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Prefer Clang (required for __builtin_* and SIMD intrinsics) ──────────────
if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(WARNING "CafeRecomp is designed for Clang. Other compilers may not work.")
endif()

# ── Global compiler flags ─────────────────────────────────────────────────────
add_compile_options(-Wall -Wextra -Wno-unused-parameter)

# ── Third-party dependencies ──────────────────────────────────────────────────
# disasm: bundled PPC disassembler (from binutils, stripped down)
add_subdirectory(thirdparty/disasm)

# simde: portable SIMD intrinsics
# NOTE: include path is thirdparty/ (parent), so #include <simde/x86/sse2.h> resolves correctly
add_library(simde INTERFACE)
target_include_directories(simde INTERFACE thirdparty)

# fmt: string formatting
add_subdirectory(thirdparty/fmt EXCLUDE_FROM_ALL)

# tomlplusplus: config file parsing
add_subdirectory(thirdparty/tomlplusplus EXCLUDE_FROM_ALL)

# zlib: RPX section decompression (Wii U specific)
find_package(ZLIB REQUIRED)

# xxHash: fast hashing (needed by SaveCurrentOutData)
add_subdirectory(thirdparty/xxHash/cmake_unofficial EXCLUDE_FROM_ALL)

# ── Sub-projects ──────────────────────────────────────────────────────────────
add_subdirectory(CafeUtils)
add_subdirectory(CafeAnalyse)
add_subdirectory(CafeRecomp)
