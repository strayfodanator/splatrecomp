// rpx_test.cpp — teste standalone do RPX loader
// Carrega o Gambit.rpx e imprime todas as seções detectadas.
// Compilar: clang++ -std=c++23 rpx_test.cpp -I../CafeUtils -I../thirdparty ../build/CafeUtils/libCafeUtils.a -lz -o rpx_test
#include <cstdio>
#include <vector>
#include <fstream>
#include "CafeUtils/image.h"

int main(int argc, char** argv)
{
    const char* path = argc > 1 ? argv[1]
        : "/mnt/myfuckinghd/dev/splatdecomp/game/Splatoon [Game] [0005000010176900]/code/Gambit.rpx";

    printf("Loading: %s\n", path);

    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) { fprintf(stderr, "Cannot open file\n"); return 1; }

    auto size = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> data(size);
    f.read(reinterpret_cast<char*>(data.data()), size);

    auto image = Image::ParseImage(data.data(), data.size());

    printf("\nSections (%zu):\n", image.sections.size());
    printf("  %-25s  %-12s  %-10s  %s\n", "Name", "Base", "Size", "Flags");
    printf("  %s\n", std::string(65, '-').c_str());
    for (const auto& s : image.sections)
    {
        printf("  %-25s  0x%08zX    0x%08X  %s\n",
            s.name.c_str(), s.base, s.size,
            (s.flags & SectionFlags_Code) ? "[CODE]" : "");
    }
    printf("\nEntry point: 0x%08zX\n", image.entry_point);
    printf("Image base:  0x%08zX\n", image.base);
    return 0;
}
