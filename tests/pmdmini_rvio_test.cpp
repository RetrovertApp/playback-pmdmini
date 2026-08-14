#include "pmd_io.h"

#include <cstdlib>
#include <cstring>

static int g_load_count;
static int g_free_count;

static void* load_file(const char* filename, unsigned long long* size) {
    static const unsigned char song[] = { 0x00, 0x18, 0x00, 0xaa, 0xbb };
    if (std::strcmp(filename, "memory://song.m") != 0) {
        *size = 0;
        return nullptr;
    }

    void* data = std::malloc(sizeof(song));
    if (data == nullptr) {
        *size = 0;
        return nullptr;
    }
    std::memcpy(data, song, sizeof(song));
    *size = sizeof(song);
    ++g_load_count;
    return data;
}

static void free_file(void* data) {
    ++g_free_count;
    std::free(data);
}

int main() {
    pmd_set_file_callbacks(load_file, free_file);

    if (pmd_is_pmd("memory://song.m") != 1)
        return 1;
    if (pmd_is_pmd("memory://missing.m") != 0)
        return 2;
    if (g_load_count != 1 || g_free_count != 1)
        return 3;

    FILE* file = pmd_io_fopen("memory://song.m", "rb");
    if (file == nullptr)
        return 4;
    if (pmd_io_fseek(file, -2, SEEK_END) != 0 || pmd_io_ftell(file) != 3)
        return 5;

    unsigned char tail[2] = {};
    if (pmd_io_fread(tail, 1, sizeof(tail), file) != sizeof(tail))
        return 6;
    if (tail[0] != 0xaa || tail[1] != 0xbb)
        return 7;
    if (pmd_io_fclose(file) != 0)
        return 8;
    if (g_load_count != 2 || g_free_count != 2)
        return 9;

    return 0;
}
