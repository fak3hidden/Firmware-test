#pragma once
#include <Arduino.h>

namespace Storage {
    void init();
    bool ready();           /* FFat or SD mounted */
    bool sd();
    const char* root();     /* "/ext" or "/int" */
    bool mkdirp(const char* path);
    bool exists(const char* path);
    bool remove(const char* path);
    size_t writeFile(const char* path, const uint8_t* data, size_t n);
    size_t readFile(const char* path, uint8_t* data, size_t maxn);
    String readText(const char* path);
    bool writeText(const char* path, const char* s);
    /* list directory, returns count, fills names[] of size maxn (each 32 chars) */
    int list(const char* path, char names[][32], int maxn, bool dirs = false);
    uint64_t totalBytes();
    uint64_t usedBytes();
}
