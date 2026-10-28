#include "tekst.hpp"
#include "font_data.h"
#include <cmath>

Fontovi F;

static std::vector<int> SviZnakovi() {
    std::vector<int> cp;
    for (int c = 32; c <= 126; c++) cp.push_back(c);
    for (int c = 160; c <= 383; c++) cp.push_back(c);   // covers č ć š ž đ
    const int dodatni[] = { 0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x201E, 0x2026, 0x2022, 0x00B2, 0x00B3, 0x00B9 };
    for (int c : dodatni) cp.push_back(c);
    return cp;
}

static Font Ucitaj(const unsigned char* data, int size, int px) {
    std::vector<int> cp = SviZnakovi();
    Font f = LoadFontFromMemory(".ttf", data, size, px, cp.data(), (int)cp.size());
    SetTextureFilter(f.texture, TEXTURE_FILTER_POINT);   // keeps the pixels sharp
    return f;
}

void UcitajFontove() {
    F.naslov    = Ucitaj(FONT_PRESS_START, FONT_PRESS_START_SIZE, 48);
    F.veliki    = Ucitaj(FONT_PRESS_START, FONT_PRESS_START_SIZE, 32);
    F.srednji   = Ucitaj(FONT_PRESS_START, FONT_PRESS_START_SIZE, 24);
    F.mali      = Ucitaj(FONT_PRESS_START, FONT_PRESS_START_SIZE, 16);
    F.tekst     = Ucitaj(FONT_VT323, FONT_VT323_SIZE, 32);
    F.tekstMali = Ucitaj(FONT_VT323, FONT_VT323_SIZE, 24);
}

void OslobodiFontove() {
    UnloadFont(F.naslov); UnloadFont(F.veliki); UnloadFont(F.srednji);
    UnloadFont(F.mali); UnloadFont(F.tekst); UnloadFont(F.tekstMali);
}

float SirinaTeksta(const Font& f, const std::string& s) {
    return MeasureTextEx(f, s.c_str(), (float)f.baseSize, 0).x;
}

std::vector<std::string> PrelomiTekst(const Font& f, const std::string& s, float maxSirina) {
    std::vector<std::string> linije;
    std::string linija, rijec;
    auto dodajRijec = [&](const std::string& w) {
        if (w.empty()) return;
        std::string proba = linija.empty() ? w : linija + " " + w;
        if (SirinaTeksta(f, proba) <= maxSirina) { linija = proba; return; }
        if (!linija.empty()) { linije.push_back(linija); linija.clear(); }
        // a single word longer than the whole line gets split by letters
        if (SirinaTeksta(f, w) > maxSirina) {
            std::string dio;
            int i = 0;
            while (i < (int)w.size()) {
                int len = 1;
                unsigned char c = (unsigned char)w[i];
                if (c >= 0xF0) len = 4; else if (c >= 0xE0) len = 3; else if (c >= 0xC0) len = 2;
                std::string zn = w.substr(i, len);
                if (SirinaTeksta(f, dio + zn) > maxSirina && !dio.empty()) { linije.push_back(dio); dio.clear(); }
                dio += zn; i += len;
            }
            linija = dio;
        } else {
            linija = w;
        }
    };
    for (size_t i = 0; i <= s.size(); i++) {
        if (i == s.size() || s[i] == ' ' || s[i] == '\n') {
            dodajRijec(rijec); rijec.clear();
            if (i < s.size() && s[i] == '\n') { linije.push_back(linija); linija.clear(); }
        } else {
            rijec += s[i];
        }
    }
    if (!linija.empty() || linije.empty()) linije.push_back(linija);
    return linije;
}

// Press Start 2P squishes the capital Č Ć Š Ž so the mark fits in its 8x8 box, which makes them
// look smaller than C S Z. So we draw a full size C/S/Z and put the mark on top ourselves.
static bool JePikselFont(const Font& f) {
    unsigned int id = f.texture.id;
    return id == F.naslov.texture.id || id == F.veliki.texture.id || id == F.srednji.texture.id || id == F.mali.texture.id;
}

static bool ImaVelikuKvacicu(const std::string& s) {
    // UTF8 bytes: Č=C4 8C, Ć=C4 86, Š=C5 A0, Ž=C5 BD
    for (size_t i = 0; i + 1 < s.size(); i++) {
        unsigned char a = (unsigned char)s[i], b = (unsigned char)s[i + 1];
        if ((a == 0xC4 && (b == 0x8C || b == 0x86)) || (a == 0xC5 && (b == 0xA0 || b == 0xBD))) return true;
    }
    return false;
}

static void Kvacica(const Font& f, int baza, bool akut, float x, float y, Color c) {
    int gi = GetGlyphIndex(f, baza);
    float p = f.baseSize / 8.0f;                       // size of one font pixel
    float vrh = y + f.glyphs[gi].offsetY;
    float cx = x + f.glyphs[gi].offsetX + f.recs[gi].width / 2.0f;
    // two rows of pixels above the letter, with a one pixel gap
    const char* r0 = akut ? "...xx" : "xx.xx";
    const char* r1 = akut ? "..xx." : ".xxx.";
    for (int k = 0; k < 5; k++) {
        float px = std::floor(cx - 2.5f * p + k * p);
        if (r0[k] == 'x') DrawRectangle((int)px, (int)(vrh - 3 * p), (int)std::ceil(p), (int)std::ceil(p), c);
        if (r1[k] == 'x') DrawRectangle((int)px, (int)(vrh - 2 * p), (int)std::ceil(p), (int)std::ceil(p), c);
    }
}

void Tekst(const Font& f, const std::string& s, float x, float y, Color c) {
    x = (float)(int)x; y = (float)(int)y;
    if (!JePikselFont(f) || !ImaVelikuKvacicu(s)) {
        DrawTextEx(f, s.c_str(), { x, y }, (float)f.baseSize, 0, c);
        return;
    }
    const char* t = s.c_str();
    int i = 0;
    float px = x;
    while (t[i]) {
        int n = 0;
        int cp = GetCodepointNext(t + i, &n);
        i += n;
        int baza = cp; bool akut = false, kv = true;
        if (cp == 0x10C) baza = 'C';
        else if (cp == 0x106) { baza = 'C'; akut = true; }
        else if (cp == 0x160) baza = 'S';
        else if (cp == 0x17D) baza = 'Z';
        else kv = false;
        if (cp != ' ') DrawTextCodepoint(f, baza, { px, y }, (float)f.baseSize, c);
        if (kv) Kvacica(f, baza, akut, px, y, c);
        int gi = GetGlyphIndex(f, cp);
        px += f.glyphs[gi].advanceX ? f.glyphs[gi].advanceX : f.recs[gi].width;
    }
}

void TekstSjena(const Font& f, const std::string& s, float x, float y, Color c, int pomak) {
    Color sj = { 10, 8, 20, (unsigned char)(c.a * 0.85f) };
    Tekst(f, s, x + pomak, y + pomak, sj);
    Tekst(f, s, x, y, c);
}

void TekstCentar(const Font& f, const std::string& s, float cx, float y, Color c, bool sjena) {
    float w = SirinaTeksta(f, s);
    if (sjena) TekstSjena(f, s, cx - w / 2, y, c, f.baseSize >= 32 ? 4 : 2);
    else Tekst(f, s, cx - w / 2, y, c);
}

void Utf8IzbrisiZadnji(std::string& s) {
    if (s.empty()) return;
    size_t i = s.size() - 1;
    while (i > 0 && ((unsigned char)s[i] & 0xC0) == 0x80) i--;
    s.erase(i);
}

void Utf8IzbrisiRijec(std::string& s) {
    while (!s.empty() && s.back() == ' ') s.pop_back();
    while (!s.empty() && s.back() != ' ') Utf8IzbrisiZadnji(s);
}

void Utf8Dodaj(std::string& s, int cp) {
    int n = 0;
    const char* u = CodepointToUTF8(cp, &n);
    s.append(u, n);
}

int Utf8Duzina(const std::string& s) {
    int n = 0;
    for (unsigned char c : s) if ((c & 0xC0) != 0x80) n++;
    return n;
}
