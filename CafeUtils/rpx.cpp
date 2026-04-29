// ============================================================
//  rpx.cpp — Wii U RPX/RPL Loader  (CafeRecomp)
//
//  RPX = ELF32 big-endian PPC + SHF_RPX_DEFLATE extension.
//  Compressed sections start with 4 bytes uncompressed size
//  (big-endian), followed immediately by a raw zlib stream.
//
//  IMPORTANT: the section-name string table (.shstrtab) is
//  ALSO compressed in RPX files.  We must decompress it in a
//  first pass before we can resolve any section names.
// ============================================================

#include "rpx.h"
#include "elf.h"
#include "byteswap.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
#include <zlib.h>

static inline uint32_t be32(uint32_t v) { return ByteSwap(v); }
static inline uint16_t be16(uint16_t v) { return ByteSwap(v); }

// Decompress one RPX section into a heap buffer.
// Returns empty vector on failure.
static std::vector<uint8_t> DecompressSection(
    const uint8_t* fileData, size_t fileSize,
    const elf32_shdr& sh)
{
    const uint32_t shOff  = be32(sh.sh_offset);
    const uint32_t shSize = be32(sh.sh_size);
    const uint32_t shType = be32(sh.sh_type);
    const uint32_t shFlags = be32(sh.sh_flags);

    if (shType == SHT_NULL || shSize == 0)
        return {};

    if (shFlags & SHF_RPX_DEFLATE)
    {
        if (shOff + RPX_DECOMP_SIZE_FIELD > fileSize || shSize < RPX_DECOMP_SIZE_FIELD)
            return {};

        uint32_t uncompSize = be32(*reinterpret_cast<const uint32_t*>(fileData + shOff));
        if (uncompSize == 0 || uncompSize > 256u * 1024 * 1024)
            return {};

        const uint8_t* src = fileData + shOff + RPX_DECOMP_SIZE_FIELD;
        uLongf srcLen = shSize - RPX_DECOMP_SIZE_FIELD;
        uLongf dstLen = uncompSize;

        std::vector<uint8_t> buf(uncompSize);
        if (uncompress(buf.data(), &dstLen, src, srcLen) != Z_OK)
            return {};

        buf.resize(dstLen);
        return buf;
    }
    else if (shType == SHT_NOBITS)
    {
        return std::vector<uint8_t>(shSize, 0);
    }
    else
    {
        if (shOff + shSize > fileSize)
            return {};
        return std::vector<uint8_t>(fileData + shOff, fileData + shOff + shSize);
    }
}

// ── RpxLoadImage ─────────────────────────────────────────────────────────────

Image RpxLoadImage(const uint8_t* data, size_t fileSize)
{
    if (!data || fileSize < sizeof(elf32_hdr))
    {
        fprintf(stderr, "[RPX] File too small.\n");
        return {};
    }

    const auto* ehdr = reinterpret_cast<const elf32_hdr*>(data);

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3)
    {
        fprintf(stderr, "[RPX] Not an ELF file.\n");
        return {};
    }

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32 || ehdr->e_ident[EI_DATA] != ELFDATA2MSB)
    {
        fprintf(stderr, "[RPX] Expected 32-bit big-endian PPC ELF.\n");
        return {};
    }

    const uint32_t shoff    = be32(ehdr->e_shoff);
    const uint16_t shnum    = be16(ehdr->e_shnum);
    const uint16_t shstrndx = be16(ehdr->e_shstrndx);
    const uint32_t phoff    = be32(ehdr->e_phoff);
    const uint16_t phnum    = be16(ehdr->e_phnum);

    if (shoff + (size_t)shnum * sizeof(elf32_shdr) > fileSize)
    {
        fprintf(stderr, "[RPX] Section header table out of bounds.\n");
        return {};
    }

    const auto* shdrs = reinterpret_cast<const elf32_shdr*>(data + shoff);
    const auto* phdrs = phnum > 0
        ? reinterpret_cast<const elf32_phdr*>(data + phoff)
        : nullptr;

    // ── Image base from first PT_LOAD segment ─────────────────────────────
    size_t imageBase = 0;
    for (uint16_t i = 0; i < phnum; i++)
    {
        if (be32(phdrs[i].p_type) == PT_LOAD)
        {
            imageBase = be32(phdrs[i].p_vaddr);
            break;
        }
    }

    // ── Pass 1: decompress the section-name string table ─────────────────
    // In RPX, .shstrtab is often compressed just like code/data sections.
    std::vector<uint8_t> stringTableBuf;
    const char* stringTable = nullptr;
    if (shstrndx != SHN_UNDEF && shstrndx < shnum)
    {
        stringTableBuf = DecompressSection(data, fileSize, shdrs[shstrndx]);
        if (!stringTableBuf.empty())
            stringTable = reinterpret_cast<const char*>(stringTableBuf.data());
    }

    // ── Pass 2: decompress all sections ──────────────────────────────────
    struct SectionInfo
    {
        std::string name;
        uint32_t    addr;
        uint32_t    size;
        uint8_t     flags;          // CafeUtils SectionFlags
        size_t      bufIdx;         // index into sectionBuffers
    };

    std::vector<SectionInfo>          sectionInfos;
    std::vector<std::vector<uint8_t>> sectionBuffers;
    size_t totalBytes = 0;

    for (uint16_t i = 0; i < shnum; i++)
    {
        const auto& sh = shdrs[i];
        const uint32_t shType  = be32(sh.sh_type);
        const uint32_t shFlags = be32(sh.sh_flags);
        const uint32_t shAddr  = be32(sh.sh_addr);
        const uint32_t shSize  = be32(sh.sh_size);

        if (shType == SHT_NULL || shSize == 0)
            continue;

        // Resolve name using the decompressed string table
        std::string name;
        if (stringTable && be32(sh.sh_name) != 0)
            name = stringTable + be32(sh.sh_name);

        // Skip RPX CRC metadata
        if (name == RPX_CRC_SECTION)
            continue;

        // Section flags
        uint8_t cafeFlags = 0;
        if (shFlags & SHF_EXECINSTR)
            cafeFlags |= SectionFlags_Code;

        auto buf = DecompressSection(data, fileSize, sh);
        if (buf.empty() && shType != SHT_NOBITS)
        {
            fprintf(stderr, "[RPX] Failed to load section %d (%s), skipping.\n",
                i, name.c_str());
            continue;
        }

        uint32_t actualSize = (uint32_t)buf.size();
        totalBytes += actualSize;

        sectionBuffers.push_back(std::move(buf));
        sectionInfos.push_back({ name, shAddr, actualSize, cafeFlags,
                                  sectionBuffers.size() - 1 });
    }

    // ── Flatten into one allocation owned by Image ────────────────────────
    Image image{};
    image.base        = imageBase;
    image.size        = (uint32_t)totalBytes;
    image.entry_point = be32(ehdr->e_entry);
    image.data        = std::make_unique<uint8_t[]>(totalBytes ? totalBytes : 1);

    size_t cursor = 0;
    for (auto& info : sectionInfos)
    {
        auto& buf = sectionBuffers[info.bufIdx];
        if (!buf.empty())
            memcpy(image.data.get() + cursor, buf.data(), info.size);

        uint8_t* ptr = image.data.get() + cursor;
        const size_t rva = info.addr >= imageBase ? info.addr - imageBase : info.addr;
        image.Map(info.name, rva, info.size, info.flags, ptr);
        cursor += info.size;
    }

    // ── Summary print ──────────────────────────────────────────────────────
    fprintf(stdout, "[RPX] Loaded — base=0x%08zX  entry=0x%08zX  sections=%zu\n",
        image.base, image.entry_point, image.sections.size());
    for (const auto& sec : image.sections)
    {
        fprintf(stdout, "  %-24s  base=0x%08zX  size=0x%08X  %s\n",
            sec.name.c_str(), sec.base, sec.size,
            (sec.flags & SectionFlags_Code) ? "[CODE]" : "");
    }
    fflush(stdout);

    return image;
}
