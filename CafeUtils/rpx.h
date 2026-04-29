#pragma once
#include "image.h"

// ============================================================
//  RPX/RPL Format Constants  (Wii U — CafeOS)
// ============================================================

// RPX uses ELF e_type = 0xFE01
// RPL (shared library) uses e_type = 0xFE02
static constexpr uint16_t ET_RPX = 0xFE01;
static constexpr uint16_t ET_RPL = 0xFE02;

// Special section header flags used by RPX
// SHF_RPX_DEFLATE (0x08000000): section data is zlib-compressed
static constexpr uint32_t SHF_RPX_DEFLATE = 0x08000000;

// The first 4 bytes of a compressed section contain the
// uncompressed size as a big-endian uint32_t
static constexpr size_t RPX_DECOMP_SIZE_FIELD = 4;

// RPX entry point / CRC section name
static constexpr const char* RPX_CRC_SECTION = ".rpxcrcs";

// ============================================================
//  RPX Loader
// ============================================================
// Reads a Wii U .rpx file from memory, decompresses any
// zlib-compressed sections (SHF_RPX_DEFLATE) and maps all
// sections into an Image using the CafeUtils abstraction.
//
// The caller owns the returned Image and its data buffer.
// Returns an empty Image on failure.
// ============================================================
Image RpxLoadImage(const uint8_t* data, size_t size);
