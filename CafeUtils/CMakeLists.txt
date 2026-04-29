add_library(CafeUtils STATIC
    image.cpp
    image.h
    rpx.cpp
    rpx.h
    elf.h
    byteswap.h
    disasm.cpp
    disasm.h
    file.h
    memory_mapped_file.cpp
    memory_mapped_file.h
    ppc_context.h
    section.h
    symbol.h
    symbol_table.h
)

target_include_directories(CafeUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(CafeUtils PUBLIC
    fmt::fmt
    simde
    ZLIB::ZLIB
    disasm
)

# ppc_config.h must define PPC_IMAGE_BASE, PPC_IMAGE_SIZE, PPC_CODE_BASE
# These are filled in by CafeRecomp at generation time.
# For the utils library itself, provide a default.
target_compile_definitions(CafeUtils PUBLIC
    PPC_IMAGE_BASE=0
    PPC_IMAGE_SIZE=0x10000000
    PPC_CODE_BASE=0x02000000
)
