#include "storage.h"
#include "board.h"
#include <FFat.h>
#include <SD.h>
#include <FS.h>

extern void Board_setSdPresent(bool v);

static bool gFat = false, gSd = false;
static fs::FS* gFs = nullptr;

void Storage::init() {
    Board::deselectAll();
    gFat = FFat.begin(true);
    Board::spiLock();
    Board::deselectAll();
    Board::setSpiHz(20000000);
    gSd = SD.begin(BOARD_SD_CS, Board::spi(), 20000000);
    Board::deselectAll();
    Board::spiUnlock();
    Board_setSdPresent(gSd);
    gFs = gSd ? (fs::FS*)&SD : (gFat ? (fs::FS*)&FFat : nullptr);
    if (gFs) {
        mkdirp("/ext/subghz");
        mkdirp("/ext/nfc");
        mkdirp("/ext/infrared");
        mkdirp("/ext/badusb");
        mkdirp("/ext/apps");
        mkdirp("/ext/ibutton");
        mkdirp("/ext/lfrfid");
    }
}

bool Storage::ready() { return gFs != nullptr; }
bool Storage::sd() { return gSd; }
const char* Storage::root() { return "/ext"; }

static String norm(const char* p) {
    if (!p || !*p) return "/";
    if (p[0] == '/') return String(p);
    return String("/") + p;
}

bool Storage::mkdirp(const char* path) {
    if (!gFs) return false;
    String s = norm(path);
    if (gFs->exists(s)) return true;
    /* walk */
    String acc;
    for (size_t i = 0; i < s.length(); i++) {
        acc += s[i];
        if (s[i] == '/' && acc.length() > 1) {
            String d = acc.substring(0, acc.length() - 1);
            if (!gFs->exists(d)) gFs->mkdir(d);
        }
    }
    if (!gFs->exists(s)) gFs->mkdir(s);
    return gFs->exists(s);
}

bool Storage::exists(const char* path) { return gFs && gFs->exists(norm(path)); }
bool Storage::remove(const char* path) { return gFs && gFs->remove(norm(path)); }

size_t Storage::writeFile(const char* path, const uint8_t* data, size_t n) {
    if (!gFs) return 0;
    String s = norm(path);
    int slash = s.lastIndexOf('/');
    if (slash > 0) mkdirp(s.substring(0, slash).c_str());
    File f = gFs->open(s, FILE_WRITE);
    if (!f) return 0;
    size_t w = f.write(data, n);
    f.close();
    return w;
}

size_t Storage::readFile(const char* path, uint8_t* data, size_t maxn) {
    if (!gFs) return 0;
    File f = gFs->open(norm(path), FILE_READ);
    if (!f) return 0;
    size_t r = f.read(data, maxn);
    f.close();
    return r;
}

String Storage::readText(const char* path) {
    if (!gFs) return String();
    File f = gFs->open(norm(path), FILE_READ);
    if (!f) return String();
    String s = f.readString();
    f.close();
    return s;
}

bool Storage::writeText(const char* path, const char* s) {
    return writeFile(path, (const uint8_t*)s, strlen(s)) == strlen(s);
}

int Storage::list(const char* path, char names[][32], int maxn, bool dirs) {
    if (!gFs) return 0;
    File d = gFs->open(norm(path));
    if (!d || !d.isDirectory()) return 0;
    int n = 0;
    File e = d.openNextFile();
    while (e && n < maxn) {
        if (e.isDirectory() == dirs) {
            const char* nm = e.name();
            const char* slash = strrchr(nm, '/');
            if (slash) nm = slash + 1;
            strncpy(names[n], nm, 31);
            names[n][31] = 0;
            n++;
        }
        e = d.openNextFile();
    }
    return n;
}

uint64_t Storage::totalBytes() {
    if (gSd) return SD.totalBytes();
    if (gFat) return FFat.totalBytes();
    return 0;
}
uint64_t Storage::usedBytes() {
    if (gSd) return SD.usedBytes();
    if (gFat) return FFat.usedBytes();
    return 0;
}
