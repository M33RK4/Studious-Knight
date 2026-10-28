#include "grafika.hpp"
#include "rlgl.h"
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>

namespace {

struct Sprite { Texture2D tex{}, bijela{}; int w = 0, h = 0; };
typedef std::map<char, Color> Paleta;

Sprite vitezMir[Grafika::BROJ_IZGLEDA], vitezKorak[Grafika::BROJ_IZGLEDA], lutka;
int aktivniOklop = 0;
Sprite orci[Grafika::BROJ_NEPRIJATELJA];
std::vector<Vector2> oci[Grafika::BROJ_NEPRIJATELJA], zar[Grafika::BROJ_NEPRIJATELJA];   // eye and glowing pixels for every enemy, so the animations know where to draw
struct OblikNeprijatelja { Vector2 ruka; int vrat, pojas, porub; };   // in the trimmed sprite's coordinates
OblikNeprijatelja oblik[Grafika::BROJ_NEPRIJATELJA];
Texture2D stecci[4];
int stecakW[4], stecakH[4];
Texture2D srcePuno, srcePrazno;
Texture2D kartaPoledjina, kartaLice, kartaLiceZ;
Image ikona;

// The knight faces right. Round helmet with a plume, gold collar, belt, mail skirt and knee guards.
// Colors come from the armor: S/s/d armor, R/r plume, B/b padding, Y gold trim, L belt.
const char* VITEZ_GORE[17] = {
    "...KKKKK........",
    "..KRRRRRKK......",
    ".KRRrrRRYYK.....",
    ".KRrKKKKKKKKK...",
    ".KrKSWWSSSSSsK..",
    ".KrKSWSSSKKKKKK.",
    ".KrKSSSSKdKdKdK.",
    "..KKsSYSKdKdKdK.",
    "...KsSSSKKKKKKK.",
    "...KdsSSSSSSsdK.",
    "....KKKYYYYKKK..",
    "..KYYSKSSSSKYYK.",
    "..KSSsKSWSSKSsK.",
    "..KBsKsSSSsKdSK.",
    "..KdKKLLYLLKdK..",
    "...KKBBBBBBBK...",
    "....KbBKBBbK....",
};
const char* VITEZ_NOGE_MIR[4] = {
    "....KSSK.KSSK...",
    "....KYYK.KYYK...",
    "....KssK.KssK...",
    "...KKKKK.KKKKK..",
};
const char* VITEZ_NOGE_KORAK[4] = {
    "...KSSK...KSSK..",
    "...KYYK...KYYK..",
    "...KssK...KssK..",
    "..KKKKK...KKKKK.",
};
#include "neprijatelji_sprite.inl"
Image SlikaIzRedova(const char* const* redovi, int h, const Paleta& p);

// palette written like "R:176,58,44 r:118,34,28 ..." (K is the outline unless you set it)
Paleta ParsirajPaletu(const char* s) {
    Paleta p = { {'K', {24, 20, 22, 255}} };
    std::string t = s;
    size_t i = 0;
    while (i < t.size()) {
        while (i < t.size() && t[i] == ' ') i++;
        if (i + 2 >= t.size()) break;
        char c = t[i];
        int r = 0, g = 0, b = 0;
        std::sscanf(t.c_str() + i + 2, "%d,%d,%d", &r, &g, &b);
        p[c] = { (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
        while (i < t.size() && t[i] != ' ') i++;
    }
    return p;
}


const char* LUTKA[] = {
    "......KKKK......",
    ".....KttttK.....",
    "....KttttttK....",
    "....KKtKKtKK....",
    "....KtKttKtK....",
    "....KKtKKtKK....",
    "....KttttttK....",
    "....KtTTTTtK....",
    ".....KttttK.....",
    "......KrrK......",
    "KPPPPKyyyyKPPPPK",
    "KPPPPyyyyyyPPPPK",
    "KKKKKyyyyyyKKKKK",
    "....KyyRRyyK....",
    "....KyRWWRyK....",
    "....KyRWWRyK....",
    "....KyyRRyyK....",
    "....KrrrrrrK....",
    "....KyyyyyyK....",
    "....KyYyyYyK....",
    ".....KyyyyK.....",
    "......KPPK......",
    "......KPPK......",
    "......KPPK......",
    "......KPPK......",
    "......KPPK......",
    "......KPPK......",
    "......KPPK......",
    "....KKPPPPKK....",
    "...KPPPPPPPPK...",
};

const char* SRCE[] = {
    ".KKK.KKK.",
    "KRRWKRRRK",
    "KRWRRRRRK",
    "KRRRRRRRK",
    ".KRRRRRK.",
    "..KRRRK..",
    "...KRK...",
    "....K....",
};

Image SlikaIzRedova(const char* const* redovi, int h, const Paleta& p) {
    int w = (int)std::strlen(redovi[0]);
    Image img = GenImageColor(w, h, BLANK);
    for (int y = 0; y < h; y++) {
        int rw = (int)std::strlen(redovi[y]);
        if (rw != w) std::printf("UPOZORENJE: sprite red %d ima sirinu %d umjesto %d\n", y, rw, w);
        for (int x = 0; x < rw && x < w; x++) {
            auto it = p.find(redovi[y][x]);
            if (it != p.end()) ImageDrawPixel(&img, x, y, it->second);
        }
    }
    return img;
}

Sprite SpriteIzSlike(Image img) {
    Sprite s;
    s.w = img.width; s.h = img.height;
    s.tex = LoadTextureFromImage(img);
    Image b = ImageCopy(img);
    for (int y = 0; y < b.height; y++)
        for (int x = 0; x < b.width; x++)
            if (GetImageColor(b, x, y).a > 0) ImageDrawPixel(&b, x, y, WHITE);
    s.bijela = LoadTextureFromImage(b);
    UnloadImage(b);
    SetTextureFilter(s.tex, TEXTURE_FILTER_POINT);
    SetTextureFilter(s.bijela, TEXTURE_FILTER_POINT);
    return s;
}

// Armor colors: K outline, S/s/d armor (light, mid, dark), R/r plume, B/b tunic, Y tunic symbol,
// L belt, then the sword's guard and blade.
struct OklopBoje { Color K, S, s, d, R, r, B, b, Y, L, stitnik, sjecivo; };
const OklopBoje OKLOPI[Grafika::OKLOP_VITEZICA + 1] = {
    // 0: classic, steel with a blue tunic
    { {27, 24, 40, 255}, {214, 222, 230, 255}, {150, 162, 176, 255}, {90, 100, 116, 255},
      {214, 48, 49, 255}, {150, 28, 36, 255}, {40, 86, 170, 255}, {24, 52, 110, 255},
      {240, 200, 60, 255}, {120, 78, 40, 255}, {240, 200, 60, 255}, {214, 222, 232, 255} },
    // 1: bronze squire, dark red plume and padding
    { {34, 22, 18, 255}, {214, 164, 108, 255}, {164, 112, 66, 255}, {106, 68, 40, 255},
      {160, 40, 46, 255}, {104, 22, 30, 255}, {136, 34, 40, 255}, {86, 20, 26, 255},
      {240, 214, 140, 255}, {96, 62, 34, 255}, {206, 136, 64, 255}, {214, 222, 232, 255} },
    // 2: crimson guard, darker steel, red tunic, white plume
    { {27, 20, 30, 255}, {200, 206, 216, 255}, {136, 142, 158, 255}, {80, 84, 102, 255},
      {244, 244, 244, 255}, {176, 176, 190, 255}, {176, 32, 44, 255}, {112, 18, 30, 255},
      {240, 200, 60, 255}, {44, 32, 32, 255}, {240, 200, 60, 255}, {214, 222, 232, 255} },
    // 3: shadow knight, black armor and purple
    { {10, 8, 16, 255}, {92, 88, 112, 255}, {62, 58, 80, 255}, {42, 38, 56, 255},
      {170, 80, 230, 255}, {106, 44, 156, 255}, {64, 32, 96, 255}, {38, 18, 60, 255},
      {210, 140, 255, 255}, {30, 26, 36, 255}, {170, 80, 230, 255}, {156, 146, 188, 255} },
    // 4: golden knight
    { {40, 26, 12, 255}, {252, 220, 110, 255}, {218, 168, 58, 255}, {152, 106, 32, 255},
      {40, 86, 170, 255}, {24, 52, 110, 255}, {248, 248, 242, 255}, {200, 200, 212, 255},
      {222, 170, 40, 255}, {120, 78, 40, 255}, {252, 220, 110, 255}, {236, 240, 248, 255} },
    // 5: celestial knight, silver armor with a night blue tunic and a star
    { {20, 26, 60, 255}, {240, 246, 255, 255}, {176, 204, 238, 255}, {112, 142, 194, 255},
      {130, 234, 255, 255}, {64, 172, 224, 255}, {44, 54, 140, 255}, {26, 32, 92, 255},
      {255, 250, 184, 255}, {190, 200, 232, 255}, {255, 250, 184, 255}, {196, 240, 255, 255} },
    // 6: Bosnian knight (secret), tunic in the Kotromanić colors with a gold plume
    { {27, 24, 40, 255}, {214, 222, 230, 255}, {150, 162, 176, 255}, {90, 100, 116, 255},
      {250, 214, 80, 255}, {200, 150, 40, 255}, {30, 70, 170, 255}, {18, 44, 116, 255},
      {250, 210, 60, 255}, {110, 70, 36, 255}, {250, 210, 60, 255}, {214, 222, 232, 255} },
    // 7: angelic knight, white and silver with gold
    { {44, 40, 66, 255}, {250, 250, 254, 255}, {210, 216, 236, 255}, {152, 162, 200, 255},
      {255, 240, 170, 255}, {226, 196, 100, 255}, {236, 240, 255, 255}, {176, 190, 232, 255},
      {255, 214, 90, 255}, {210, 176, 96, 255}, {255, 214, 90, 255}, {226, 244, 255, 255} },
    // 8: the pink lady knight women start with
    { {40, 24, 44, 255}, {232, 226, 240, 255}, {176, 166, 196, 255}, {112, 102, 136, 255},
      {255, 130, 180, 255}, {204, 72, 130, 255}, {236, 110, 162, 255}, {168, 58, 112, 255},
      {255, 244, 248, 255}, {150, 84, 120, 255}, {255, 196, 220, 255}, {222, 226, 238, 255} },
};

// looks 8 to 13 are the lady versions and borrow colors from armor 1 to 6, 7 is the pink one
int OklopIzgleda(int izgled) {
    if (izgled < Grafika::BROJ_OKLOPA) return izgled;
    int o = izgled - Grafika::BROJ_OKLOPA;
    return o == 0 ? Grafika::OKLOP_VITEZICA : o;   // she gets her own pink palette
}

// The lady knight (every female armor): crown, long hair around the face, dress with a belt, cape
// and boots. Dress uses the tunic colors (B/b), cape the plume colors (R/r), the rest the armor.
// Hair, face and crown stay the same in every armor.
const char* VITEZICA_GORE[18] = {
    "....C.C.C.......",
    "....CcCcC.......",
    "...KHHHHHHK.....",
    "..KHHjjHHHHK....",
    "..KHHHHHHFFHK...",
    ".KHHHHHFFEFHK...",
    ".KHHHHHFFFfHK...",
    ".KHHHHhKFFKhK...",
    ".KHHHhRKKFK.K...",
    ".KHHhRKSSBSSK...",
    ".KHhRRKsBBBSsK..",
    ".KhRRKBBBYBKSK..",
    "..KRRKBBYYYKSK..",
    "..KRRKLLLYLKdK..",
    "..KRrKBBBBBBK...",
    "..KRrKBBBBBBBK..",
    ".KRrKBBBBBBBBK..",
    ".KrKbbbbbbbbbK..",
};
const char* VITEZICA_NOGE_MIR[3] = {
    ".KK...KdK.KdK...",
    "......KdK.KdK...",
    ".....KKKK.KKKK..",
};
const char* VITEZICA_NOGE_KORAK[3] = {
    ".KK..KdK...KdK..",
    ".....KdK...KdK..",
    "....KKKK...KKKK.",
};

Image VitezSlika(bool korak, int izgled = 0) {
    int oklop = OklopIzgleda(izgled);
    bool zensko = izgled >= Grafika::BROJ_OKLOPA;
    std::string redovi[21];
    if (!zensko) {
        for (int i = 0; i < 17; i++) redovi[i] = VITEZ_GORE[i];
        for (int i = 0; i < 4; i++) redovi[17 + i] = (korak ? VITEZ_NOGE_KORAK : VITEZ_NOGE_MIR)[i];
        if (oklop == Grafika::OKLOP_BOSANSKI) {   // white sash across the chest and gold lilies on the skirt
            redovi[11] = "..KYYSKWSSSKYYK.";
            redovi[12] = "..KSSsKSWSSKSsK.";
            redovi[13] = "..KBsKsSSWsKdSK.";
            redovi[15] = "...KKBYBBBYBK...";
        }
        if (oklop == Grafika::OKLOP_ANDJEO) {     // gold sun on the chest and gold trim on the skirt
            redovi[11] = "..KYYSKSYSSKYYK.";
            redovi[12] = "..KSSsKYYYSKSsK.";
            redovi[13] = "..KBsKsSYSsKdSK.";
            redovi[15] = "...KKYBBYBBYK...";
            redovi[16] = "....KYBKBBYK....";
        }
    } else {
        for (int i = 0; i < 18; i++) redovi[i] = VITEZICA_GORE[i];
        for (int i = 0; i < 3; i++) redovi[18 + i] = (korak ? VITEZICA_NOGE_KORAK : VITEZICA_NOGE_MIR)[i];
        if (oklop == Grafika::OKLOP_VITEZICA) redovi[11] = ".KhRRKBBYBYKSK..";   // heart instead of a cross
        if (oklop == Grafika::OKLOP_BOSANSKI) {                                   // white sash across the dress
            redovi[14] = "..KRrKWBBBBBK...";
            redovi[15] = "..KRrKBWBBBBBK..";
            redovi[16] = ".KRrKBBBWBBBBK..";
        }
        if (oklop == Grafika::OKLOP_ANDJEO) {     // gold embroidery on the dress
            redovi[15] = "..KRrKBBYBBBBK..";
            redovi[17] = ".KrKbYbbYbbYbK..";
        }
    }
    const char* r[21];
    for (int i = 0; i < 21; i++) r[i] = redovi[i].c_str();
    const OklopBoje& o = OKLOPI[oklop];
    Paleta p = {
        {'K', o.K}, {'S', o.S}, {'s', o.s}, {'d', o.d}, {'R', o.R}, {'r', o.r},
        {'B', o.B}, {'b', o.b}, {'Y', o.Y}, {'L', o.L}, {'W', {240, 240, 244, 255}},
        // lady knight: hair, face and crown
        {'H', {150, 70, 40, 255}}, {'h', {104, 44, 30, 255}}, {'j', {190, 100, 56, 255}},
        {'F', {246, 200, 164, 255}}, {'f', {226, 150, 130, 255}}, {'E', {40, 24, 30, 255}},
        {'C', {250, 206, 80, 255}}, {'c', {200, 140, 40, 255}},
    };
    return SlikaIzRedova(r, 21, p);
}

// Stećci (the tombstones in the valley)
unsigned int Hash(int n) {
    unsigned int x = (unsigned int)n * 374761393u + 668265263u;
    x = (x ^ (x >> 13)) * 1274126177u;
    return x ^ (x >> 16);
}

// Shading for the enemies. Light comes from the top left, so top and left edges get brighter and
// bottom and right edges darker. Big flat areas get a bit of grain so they don't look like plastic.
// Anything in "svijetle" (eyes, fire, magic) is left alone so it keeps glowing.
Color Osvijetli(Color c, float f) {
    if (f >= 1) {
        float k = std::min(1.0f, f - 1);
        return { (unsigned char)(c.r + (255 - c.r) * k), (unsigned char)(c.g + (255 - c.g) * k), (unsigned char)(c.b + (255 - c.b) * k), c.a };
    }
    return { (unsigned char)(c.r * f), (unsigned char)(c.g * f), (unsigned char)(c.b * f), c.a };
}

void Sjencenje(Image& img, const char* const* redovi, int h, const char* svijetle, int seed) {
    int w = (int)std::strlen(redovi[0]);
    auto znak = [&](int x, int y) -> char {
        if (x < 0 || y < 0 || y >= h || x >= (int)std::strlen(redovi[y])) return '.';
        return redovi[y][x];
    };
    auto ivica = [](char c) { return c == '.' || c == 'K'; };
    Image izvor = ImageCopy(img);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            char c = znak(x, y);
            if (ivica(c) || std::strchr(svijetle, c)) continue;
            Color b = GetImageColor(izvor, x, y);
            if (b.a == 0) continue;
            bool gore = ivica(znak(x, y - 1)), lijevo = ivica(znak(x - 1, y));
            bool dolje = ivica(znak(x, y + 1)), desno = ivica(znak(x + 1, y));
            float f = 1;
            if (gore) f += 0.2f;
            if (lijevo) f += 0.1f;
            if (dolje) f -= 0.24f;
            if (desno) f -= 0.18f;
            if (!gore && !lijevo && !dolje && !desno && znak(x, y - 1) == c && znak(x + 1, y) == c) {
                unsigned int hh = Hash(x * 31 + y * 17 + seed * 101) % 7;   // grain
                if (hh == 0) f -= 0.1f; else if (hh == 1) f += 0.07f;
            }
            ImageDrawPixel(&img, x, y, Osvijetli(b, f));
        }
    UnloadImage(izvor);
}

const Color KAMEN_SV = { 206, 199, 178, 255 };
const Color KAMEN    = { 172, 165, 145, 255 };
const Color KAMEN_TM = { 120, 114, 100, 255 };
const Color KAMEN_OB = { 62, 58, 54, 255 };
const Color MAHOVINA = { 104, 138, 60, 255 };

// rectangle outline that works the same in every raylib version
void Obris(Image& img, int x, int y, int w, int h, Color c) {
    ImageDrawRectangle(&img, x, y, w, 1, c);
    ImageDrawRectangle(&img, x, y + h - 1, w, 1, c);
    ImageDrawRectangle(&img, x, y, 1, h, c);
    ImageDrawRectangle(&img, x + w - 1, y, 1, h, c);
}

void Istrosenost(Image& img, int seed) {
    for (int i = 0; i < img.width * img.height / 9; i++) {
        unsigned int h = Hash(seed * 1000 + i);
        int x = h % img.width, y = (h / 97) % img.height;
        Color c = GetImageColor(img, x, y);
        if (c.a == 0 || (c.r == KAMEN_OB.r && c.g == KAMEN_OB.g)) continue;
        Color n = { (unsigned char)(c.r * 0.88f), (unsigned char)(c.g * 0.88f), (unsigned char)(c.b * 0.88f), 255 };
        ImageDrawPixel(&img, x, y, n);
    }
}

void Mahovina(Image& img, int seed) {
    for (int x = 1; x < img.width - 1; x++) {
        unsigned int h = Hash(seed * 77 + x);
        int vis = h % 3;
        for (int y = img.height - 2; y >= img.height - 2 - vis; y--)
            if (GetImageColor(img, x, y).a > 0) ImageDrawPixel(&img, x, y, MAHOVINA);
    }
}

// Sljemenjak, the house shaped one with a two sided roof
Image Sljemenjak() {
    int W = 30, H = 26;
    Image img = GenImageColor(W, H, BLANK);
    // body
    ImageDrawRectangle(&img, 3, 12, 24, 14, KAMEN);
    ImageDrawRectangle(&img, 3, 12, 3, 14, KAMEN_SV);
    ImageDrawRectangle(&img, 24, 12, 3, 14, KAMEN_TM);
    // roof
    for (int y = 2; y <= 11; y++) {
        int k = y - 2;
        int x0 = 8 - k * 7 / 9, x1 = 21 + k * 7 / 9;
        ImageDrawRectangle(&img, x0, y, x1 - x0 + 1, 1, y < 7 ? KAMEN_SV : KAMEN);
    }
    // outline
    for (int y = 2; y <= 11; y++) {
        int k = y - 2;
        ImageDrawPixel(&img, 8 - k * 7 / 9 - 1, y, KAMEN_OB);
        ImageDrawPixel(&img, 21 + k * 7 / 9 + 1, y, KAMEN_OB);
    }
    ImageDrawRectangle(&img, 7, 1, 16, 1, KAMEN_OB);
    ImageDrawRectangle(&img, 0, 12, 30, 1, KAMEN_OB);
    ImageDrawRectangle(&img, 2, 13, 1, 13, KAMEN_OB);
    ImageDrawRectangle(&img, 27, 13, 1, 13, KAMEN_OB);
    ImageDrawRectangle(&img, 1, 11, 28, 1, KAMEN_TM);
    // zigzag under the roof
    for (int x = 4; x < 26; x++) ImageDrawPixel(&img, x, 14 + (x % 2), KAMEN_TM);
    // rosette
    ImageDrawCircleLines(&img, 10, 19, 3, KAMEN_TM);
    ImageDrawPixel(&img, 10, 19, KAMEN_TM);
    // crescent
    ImageDrawCircle(&img, 20, 19, 3, KAMEN_TM);
    ImageDrawCircle(&img, 21, 18, 3, KAMEN);
    Istrosenost(img, 1);
    Mahovina(img, 1);
    return img;
}

// Sanduk, the chest shaped one with arches
Image Sanduk() {
    int W = 28, H = 16;
    Image img = GenImageColor(W, H, BLANK);
    ImageDrawRectangle(&img, 0, 0, 28, 3, KAMEN_SV);       // slab on top
    ImageDrawRectangle(&img, 0, 3, 28, 1, KAMEN_TM);
    ImageDrawRectangle(&img, 1, 4, 26, 12, KAMEN);
    ImageDrawRectangle(&img, 1, 4, 2, 12, KAMEN_SV);
    ImageDrawRectangle(&img, 25, 4, 2, 12, KAMEN_TM);
    // arches
    for (int i = 0; i < 4; i++) {
        int x = 4 + i * 5;
        ImageDrawRectangle(&img, x, 9, 1, 6, KAMEN_TM);
        ImageDrawRectangle(&img, x + 4, 9, 1, 6, KAMEN_TM);
        ImageDrawRectangle(&img, x + 1, 7, 3, 1, KAMEN_TM);
        ImageDrawPixel(&img, x, 8, KAMEN_TM);
        ImageDrawPixel(&img, x + 4, 8, KAMEN_TM);
    }
    // spiral on the slab
    for (int x = 3; x < 25; x += 3) ImageDrawPixel(&img, x, 1, KAMEN_TM);
    // outline
    Obris(img, 0, 0, 28, 16, KAMEN_OB);
    Istrosenost(img, 2);
    Mahovina(img, 2);
    return img;
}

// Pillar with a raised hand carved into it
Image StupRuka() {
    int W = 14, H = 26;
    Image img = GenImageColor(W, H, BLANK);
    ImageDrawRectangle(&img, 2, 4, 10, 22, KAMEN);
    ImageDrawRectangle(&img, 3, 2, 8, 2, KAMEN);
    ImageDrawRectangle(&img, 5, 1, 4, 1, KAMEN);
    ImageDrawRectangle(&img, 2, 4, 2, 22, KAMEN_SV);
    ImageDrawRectangle(&img, 10, 4, 2, 22, KAMEN_TM);
    // the hand, fingers spread
    const char* ruka[] = {
        ".x.x.x.",
        ".x.x.x.",
        ".xxxxx.",
        "xxxxxx.",
        ".xxxxx.",
        "..xxx..",
        "..xxx..",
        "..xxx..",
        "..xxx..",
    };
    for (int y = 0; y < 9; y++)
        for (int x = 0; x < 7; x++)
            if (ruka[y][x] == 'x') ImageDrawPixel(&img, 3 + x, 7 + y, KAMEN_TM);
    // outline
    ImageDrawRectangle(&img, 1, 4, 1, 22, KAMEN_OB);
    ImageDrawRectangle(&img, 12, 4, 1, 22, KAMEN_OB);
    ImageDrawRectangle(&img, 2, 2, 1, 2, KAMEN_OB);
    ImageDrawRectangle(&img, 11, 2, 1, 2, KAMEN_OB);
    ImageDrawRectangle(&img, 3, 1, 2, 1, KAMEN_OB);
    ImageDrawRectangle(&img, 9, 1, 2, 1, KAMEN_OB);
    ImageDrawRectangle(&img, 5, 0, 4, 1, KAMEN_OB);
    Istrosenost(img, 3);
    Mahovina(img, 3);
    return img;
}

// Small chest with a sword and shield
Image SandukMac() {
    int W = 22, H = 13;
    Image img = GenImageColor(W, H, BLANK);
    ImageDrawRectangle(&img, 0, 0, 22, 13, KAMEN);
    ImageDrawRectangle(&img, 0, 0, 22, 2, KAMEN_SV);
    ImageDrawRectangle(&img, 19, 2, 3, 11, KAMEN_TM);
    // sword
    ImageDrawRectangle(&img, 4, 6, 12, 1, KAMEN_TM);
    ImageDrawRectangle(&img, 6, 4, 1, 5, KAMEN_TM);
    // shield
    ImageDrawRectangle(&img, 16, 4, 3, 4, KAMEN_TM);
    ImageDrawPixel(&img, 17, 8, KAMEN_TM);
    Obris(img, 0, 0, 22, 13, KAMEN_OB);
    Istrosenost(img, 4);
    Mahovina(img, 4);
    return img;
}


// Tarot cards
const Color ZLATO   = { 214, 170, 60, 255 };
const Color ZLATO_SV= { 250, 214, 110, 255 };
const Color TAMNA   = { 30, 14, 20, 255 };

void OkvirKarte(Image& img) {
    int W = img.width, H = img.height;
    Obris(img, 0, 0, W, H, TAMNA);
    Obris(img, 1, 1, W - 2, H - 2, ZLATO);
    Obris(img, 2, 2, W - 4, H - 4, ZLATO_SV);
    Obris(img, 3, 3, W - 6, H - 6, TAMNA);
    // corner decorations
    int ug[4][2] = { { 5, 5 }, { W - 8, 5 }, { 5, H - 8 }, { W - 8, H - 8 } };
    for (auto& u : ug) {
        ImageDrawPixel(&img, u[0] + 1, u[1], ZLATO);
        ImageDrawRectangle(&img, u[0], u[1] + 1, 3, 1, ZLATO);
        ImageDrawPixel(&img, u[0] + 1, u[1] + 2, ZLATO);
    }
}

// Back of the card: dark red with a diamond pattern and a sun and moon in the middle
Image KartaPoledjinaSlika() {
    const int W = 54, H = 90;
    Image img = GenImageColor(W, H, { 96, 20, 36, 255 });
    for (int y = 4; y < H - 4; y++)
        for (int x = 4; x < W - 4; x++)
            if ((x + y) % 8 == 0 || (x - y + 96) % 8 == 0) ImageDrawPixel(&img, x, y, { 66, 12, 24, 255 });
    for (int y = 4; y < H - 4; y++)
        for (int x = 4; x < W - 4; x++)
            if ((x + y) % 8 == 4 && (x - y + 96) % 8 == 4) ImageDrawPixel(&img, x, y, { 170, 120, 50, 255 });
    // medallion
    int cx = W / 2, cy = H / 2;
    ImageDrawCircle(&img, cx, cy, 15, TAMNA);
    ImageDrawCircle(&img, cx, cy, 14, ZLATO);
    ImageDrawCircle(&img, cx, cy, 12, { 26, 22, 58, 255 });
    // sun
    ImageDrawCircle(&img, cx, cy - 1, 4, ZLATO_SV);
    for (int k = 0; k < 8; k++) {
        float a = k * PI / 4;
        ImageDrawPixel(&img, cx + (int)std::round(std::cos(a) * 7), cy - 1 + (int)std::round(std::sin(a) * 7), ZLATO);
        ImageDrawPixel(&img, cx + (int)std::round(std::cos(a) * 8), cy - 1 + (int)std::round(std::sin(a) * 8), ZLATO);
    }
    // crescent under it
    ImageDrawCircle(&img, cx, cy + 8, 3, { 220, 220, 240, 255 });
    ImageDrawCircle(&img, cx + 1, cy + 7, 3, { 26, 22, 58, 255 });
    // stars
    int z[4][2] = { { cx - 8, cy - 7 }, { cx + 8, cy - 7 }, { cx - 9, cy + 5 }, { cx + 9, cy + 5 } };
    for (auto& t : z) ImageDrawPixel(&img, t[0], t[1], { 240, 240, 255, 255 });
    // diamonds at the top and bottom
    for (int k = 0; k < 2; k++) {
        int dy = k ? H - 16 : 12;
        for (int i = 0; i < 4; i++) ImageDrawRectangle(&img, cx - 3 + i, dy - i, 7 - 2 * i, 1, ZLATO);
        for (int i = 1; i < 4; i++) ImageDrawRectangle(&img, cx - 3 + i, dy + i, 7 - 2 * i, 1, ZLATO);
    }
    OkvirKarte(img);
    return img;
}

// Front of the card: parchment and an arched window with the knight holding his sword up
Image KartaLiceSlika(int oklop) {
    const int W = 54, H = 90;
    Image img = GenImageColor(W, H, { 228, 208, 164, 255 });
    for (int i = 0; i < W * H / 6; i++) {
        unsigned int h = Hash(i * 13 + 5);
        ImageDrawPixel(&img, h % W, (h / W) % H, (h % 3) ? Color{ 214, 192, 146, 255 } : Color{ 238, 222, 184, 255 });
    }
    // top strip for the roman numeral, bottom strip for the name
    ImageDrawRectangle(&img, 5, 5, W - 10, 9, { 196, 168, 118, 255 });
    Obris(img, 5, 5, W - 10, 9, { 120, 84, 50, 255 });
    ImageDrawRectangle(&img, 5, 72, W - 10, 13, { 196, 168, 118, 255 });
    Obris(img, 5, 72, W - 10, 13, { 120, 84, 50, 255 });
    // the arched window
    int x0 = 8, x1 = W - 9, yv = 36, yd = 68, cx = W / 2;
    int r = (x1 - x0) / 2;
    for (int y = 16; y <= yd; y++)
        for (int x = x0; x <= x1; x++) {
            bool unutra = y >= yv || (x - cx) * (x - cx) + (y - yv) * (y - yv) <= r * r;
            if (!unutra) continue;
            float t = (y - 16) / 52.0f;
            Color nebo = { (unsigned char)(70 + 150 * t), (unsigned char)(110 + 110 * t), (unsigned char)(190 + 40 * t), 255 };
            ImageDrawPixel(&img, x, y, nebo);
        }
    // mountains and grass inside it
    for (int x = x0; x <= x1; x++) {
        int hy = 52 - (int)(5 * std::sin(x * 0.35f) + 3 * std::sin(x * 0.9f + 1));
        for (int y = hy; y <= yd; y++) ImageDrawPixel(&img, x, y, { 120, 140, 176, 255 });
        for (int y = 60; y <= yd; y++) ImageDrawPixel(&img, x, y, { 96, 156, 72, 255 });
    }
    // arch frame
    for (int y = 14; y <= yd + 1; y++)
        for (int x = x0 - 2; x <= x1 + 2; x++) {
            int dx = x - cx, dy = y - yv;
            bool rub;
            if (y >= yv) rub = (x == x0 - 1 || x == x1 + 1);
            else { int d2 = dx * dx + dy * dy; rub = d2 > r * r && d2 <= (r + 1) * (r + 1) + 2; }
            if (rub) ImageDrawPixel(&img, x, y, { 120, 84, 50, 255 });
        }
    ImageDrawRectangle(&img, x0 - 1, yd + 1, x1 - x0 + 3, 1, { 120, 84, 50, 255 });
    // the knight at 2x with his sword straight up
    Image v = VitezSlika(false, oklop);
    ImageResizeNN(&v, v.width * 2, v.height * 2);
    // Centers the knight together with his sword (x 20 to 27 from the sprite's left edge) in the
    // window, otherwise the plume and cape pull him off to one side.
    int lijevo = 20, desno = 27;
    for (int y = 0; y < v.height; y++)
        for (int x = 0; x < v.width; x++)
            if (GetImageColor(v, x, y).a > 0) { lijevo = std::min(lijevo, x); desno = std::max(desno, x); }
    int kx = cx - (lijevo + desno + 1) / 2, ky = yd - 42;
    for (int y = 0; y < v.height; y++)          // copied by hand so it works in every raylib version
        for (int x = 0; x < v.width; x++) {
            Color c = GetImageColor(v, x, y);
            if (c.a > 0) ImageDrawPixel(&img, kx + x, ky + y, c);
        }
    UnloadImage(v);
    int hx = kx + 23, hy = ky + 26;                       // hand
    ImageDrawRectangle(&img, hx - 1, hy - 30, 4, 29, { 60, 66, 80, 255 });   // blade outline
    ImageDrawRectangle(&img, hx, hy - 29, 2, 28, { 220, 226, 236, 255 });    // blade
    ImageDrawPixel(&img, hx, hy - 30, { 220, 226, 236, 255 });
    ImageDrawRectangle(&img, hx - 3, hy - 2, 8, 2, ZLATO);                    // guard
    ImageDrawRectangle(&img, hx, hy, 2, 4, { 110, 70, 40, 255 });            // grip
    ImageDrawRectangle(&img, hx - 1, hy + 4, 4, 2, ZLATO);                    // pommel
    OkvirKarte(img);
    return img;
}

void CrtajSprite(const Sprite& s, float x, float y, float bljesak, float rot, float alfa, bool flip = false, float skala = 1) {
    Rectangle src = { 0, 0, (float)(flip ? -s.w : s.w), (float)s.h };
    Rectangle dst = { std::floor(x), std::floor(y), s.w * skala, s.h * skala };
    Vector2 org = { s.w * skala / 2.0f, s.h * skala };
    DrawTexturePro(s.tex, src, dst, org, rot, Fade(WHITE, alfa));
    if (bljesak > 0) DrawTexturePro(s.bijela, src, dst, org, rot, Fade(WHITE, bljesak * alfa));
}

// rectangle along a direction, from distance a to b away from (x, y)
void Segment(float x, float y, float ugao, float a, float b, float debljina, Color c) {
    DrawRectanglePro({ x, y, b - a, debljina }, { -a, debljina / 2 }, ugao, c);
}

} // namespace

namespace Grafika {

void Inicijalizuj() {
    for (int o = 0; o < BROJ_IZGLEDA; o++) {
        Image a = VitezSlika(false, o);
        if (o == 0) ikona = ImageCopy(a);
        vitezMir[o] = SpriteIzSlike(a); UnloadImage(a);
        Image b = VitezSlika(true, o);
        vitezKorak[o] = SpriteIzSlike(b); UnloadImage(b);
    }

    for (int i = 0; i < BROJ_NEPRIJATELJA; i++) {
        const NeprijateljSprite& ns = NEPRIJATELJI[i];
        // pad every row to the same width and trim the empty space (feet stay at the bottom)
        int sirina = 0;
        for (const char* r : ns.redovi) sirina = std::max(sirina, (int)std::strlen(r));
        std::vector<std::string> m;
        for (const char* r : ns.redovi) { std::string s = r; s.resize(sirina, '.'); m.push_back(s); }
        int x0 = sirina, x1 = -1, y0 = (int)m.size();
        for (int y = 0; y < (int)m.size(); y++)
            for (int x = 0; x < sirina; x++)
                if (m[y][x] != '.') { x0 = std::min(x0, x); x1 = std::max(x1, x); y0 = std::min(y0, y); }
        std::vector<std::string> obr;
        for (int y = y0; y < (int)m.size(); y++) obr.push_back(m[y].substr(x0, x1 - x0 + 1));
        std::vector<const char*> redovi;
        for (auto& s : obr) redovi.push_back(s.c_str());
        int h = (int)redovi.size();
        Image o = SlikaIzRedova(redovi.data(), h, ParsirajPaletu(ns.paleta));
        // glowing stuff: eyes everywhere, plus magic, embers and fire in the dungeon and volcano
        const char* svijetle = i < 11 ? "E" : (i < 16 ? "EG" : "EYFf");
        Sjencenje(o, redovi.data(), h, svijetle, i);
        orci[i] = SpriteIzSlike(o); UnloadImage(o);
        oblik[i] = { { (float)(ns.rukaX - x0), (float)(ns.rukaY - y0) }, ns.vrat - y0, ns.pojas - y0, ns.porub ? ns.porub - y0 : 0 };
        // remember where the eyes and glowing pixels are for blinking, glowing and flames
        oci[i].clear(); zar[i].clear();
        for (int y = 0; y < h; y++)
            for (int x = 0; redovi[y][x]; x++) {
                char c = redovi[y][x];
                if (c == 'E') oci[i].push_back({ (float)x, (float)y });
                else if ((i >= 16 && (c == 'Y' || c == 'F' || c == 'f')) || (i >= 11 && i < 16 && c == 'G')) zar[i].push_back({ (float)x, (float)y });
            }
    }

    Paleta pl = {
        {'K', {40, 30, 26, 255}}, {'t', {200, 170, 120, 255}}, {'T', {140, 110, 72, 255}},
        {'r', {170, 130, 70, 255}}, {'P', {140, 95, 55, 255}}, {'y', {226, 196, 100, 255}},
        {'Y', {186, 152, 68, 255}}, {'R', {200, 50, 50, 255}}, {'W', {240, 235, 220, 255}},
    };
    Image l = SlikaIzRedova(LUTKA, 30, pl);
    lutka = SpriteIzSlike(l); UnloadImage(l);

    Image st[4] = { Sljemenjak(), Sanduk(), StupRuka(), SandukMac() };
    for (int i = 0; i < 4; i++) {
        stecci[i] = LoadTextureFromImage(st[i]);
        stecakW[i] = st[i].width; stecakH[i] = st[i].height;
        UnloadImage(st[i]);
    }

    Image kp = KartaPoledjinaSlika();
    kartaPoledjina = LoadTextureFromImage(kp); UnloadImage(kp);
    Image kl = KartaLiceSlika(0);
    kartaLice = LoadTextureFromImage(kl); UnloadImage(kl);
    Image kz = KartaLiceSlika(OKLOP_VITEZICA);
    kartaLiceZ = LoadTextureFromImage(kz); UnloadImage(kz);

    Paleta ps = { {'K', {27, 24, 40, 255}}, {'R', {220, 40, 50, 255}}, {'W', {255, 200, 200, 255}} };
    Image s1 = SlikaIzRedova(SRCE, 8, ps);
    srcePuno = LoadTextureFromImage(s1); UnloadImage(s1);
    Paleta pe = { {'K', {27, 24, 40, 255}}, {'R', {70, 60, 80, 255}}, {'W', {90, 80, 100, 255}} };
    Image s2 = SlikaIzRedova(SRCE, 8, pe);
    srcePrazno = LoadTextureFromImage(s2); UnloadImage(s2);
}

void Oslobodi() {
    UnloadTexture(lutka.tex); UnloadTexture(lutka.bijela);
    for (Sprite& s : orci) { UnloadTexture(s.tex); UnloadTexture(s.bijela); }
    for (int o = 0; o < BROJ_IZGLEDA; o++) {
        UnloadTexture(vitezMir[o].tex); UnloadTexture(vitezMir[o].bijela);
        UnloadTexture(vitezKorak[o].tex); UnloadTexture(vitezKorak[o].bijela);
    }
    for (Texture2D& t : stecci) UnloadTexture(t);
    UnloadTexture(srcePuno); UnloadTexture(srcePrazno);
    UnloadTexture(kartaPoledjina); UnloadTexture(kartaLice); UnloadTexture(kartaLiceZ);
    UnloadImage(ikona);
}

Image IkonaProzora() {
    Image i = ImageCopy(ikona);
    ImageResizeNN(&i, 64, 84);
    return i;
}

// Backgrounds
static void Oblak(float x, float y, float s) {
    Color c = { 250, 248, 240, 255 };
    Color sj = { 214, 222, 238, 255 };
    DrawRectangle((int)x, (int)(y + 3 * s), (int)(26 * s), (int)(5 * s), sj);
    DrawRectangle((int)(x + 2 * s), (int)(y + 2 * s), (int)(22 * s), (int)(4 * s), c);
    DrawRectangle((int)(x + 6 * s), (int)(y - 1 * s), (int)(10 * s), (int)(4 * s), c);
    DrawRectangle((int)(x + 13 * s), (int)(y - 3 * s), (int)(7 * s), (int)(4 * s), c);
}

static void Drvo(int x, int y, Color c, Color tm) {
    DrawRectangle(x, y - 6, 3, 5, c);
    DrawRectangle(x - 1, y - 4, 5, 3, c);
    DrawRectangle(x + 1, y - 1, 1, 2, tm);
}

// Night sky: dark gradient, twinkling stars and the moon
static void NocnoNebo(float t, int visina) {
    DrawRectangleGradientV(0, 0, SVIJET_W, visina * 6 / 10, { 8, 10, 32, 255 }, { 26, 34, 78, 255 });
    DrawRectangleGradientV(0, visina * 6 / 10, SVIJET_W, visina - visina * 6 / 10, { 26, 34, 78, 255 }, { 60, 58, 110, 255 });
    for (int i = 0; i < 70; i++) {
        unsigned int h = Hash(i * 17 + 3);
        int x = h % SVIJET_W, y = (h / SVIJET_W) % (visina - 20);
        float tr = 0.55f + 0.45f * std::sin(t * (1.0f + (h % 7) * 0.4f) + i);
        DrawPixel(x, y, { 240, 240, 255, (unsigned char)(255 * tr) });
    }
    DrawCircle(256, 22, 9, { 236, 236, 220, 255 });
    DrawCircle(260, 19, 8, { 14, 18, 46, 255 });   // crescent
    DrawCircle(256, 22, 15, { 200, 210, 255, 30 }); // glow
}

// Darkens the landscape at night. The characters get drawn after, so they stay easy to see.
static void NocniFilter(int odY) {
    DrawRectangle(0, odY, SVIJET_W, SVIJET_H - odY, { 16, 22, 70, 130 });
}

static bool nocniRezim = false;
void PostaviNoc(bool noc) { nocniRezim = noc; }

// A layer of hills with trees. The scroll gets rounded to whole pixels so the hills and trees move
// together, otherwise the trees would jitter on top of the hills.
template <typename Visina>
static void SlojBrda(float pomak, Visina visina, unsigned int seed, unsigned int rijetkost, Color boja, Color drvo) {
    int p = (int)std::floor(pomak);
    for (int x = 0; x < SVIJET_W; x++) {
        int y = visina(x + p);
        DrawRectangle(x, y, 1, 100 - y, boja);
    }
    for (int wx = p - 4; wx < p + SVIJET_W + 4; wx++)
        if (Hash(wx + seed) % rijetkost == 0) Drvo(wx - p, visina(wx) + 1, drvo, { 70, 60, 50, 255 });
}

static void CrtajZelenuDolinu(float kam, float t) {
    if (nocniRezim) {
        NocnoNebo(t, 100);
    } else {
        // sky
        DrawRectangleGradientV(0, 0, SVIJET_W, 60, { 74, 120, 194, 255 }, { 140, 186, 228, 255 });
        DrawRectangleGradientV(0, 60, SVIJET_W, 40, { 140, 186, 228, 255 }, { 244, 222, 176, 255 });
        // sun
        DrawCircle(256, 30, 14, { 255, 236, 180, 70 });
        DrawCircle(256, 30, 9, { 255, 244, 200, 255 });
        // clouds
        for (int i = 0; i < 6; i++) {
            float x = std::fmod(i * 97.0f + t * 3.0f - kam * 0.06f + 10000.0f, 420.0f) - 60.0f;
            float y = 12.0f + (i * 37) % 36;
            Oblak(x, y, i % 2 ? 1.0f : 0.7f);
        }
    }
    // far mountains
    int pp = (int)std::floor(kam * 0.12f);
    for (int x = 0; x < SVIJET_W; x++) {
        float wx = (float)(x + pp);
        float h = 14 * std::sin(wx * 0.021f) + 8 * std::sin(wx * 0.057f + 1) + 4 * std::sin(wx * 0.13f + 2);
        int y = (int)(70 - h);
        DrawRectangle(x, y, 1, 100 - y, { 128, 148, 184, 255 });
        if (y < 52) DrawRectangle(x, y, 1, 2, { 226, 232, 244, 255 });
    }
    // middle hills with trees
    SlojBrda(kam * 0.3f, [](int wx) { return (int)(82 - 8 * std::sin(wx * 0.03f) - 5 * std::sin(wx * 0.071f + 2)); },
             0, 23, { 96, 132, 98, 255 }, { 62, 98, 70, 255 });
    // closer hills
    SlojBrda(kam * 0.55f, [](int wx) { return (int)(92 - 4 * std::sin(wx * 0.045f + 3) - 3 * std::sin(wx * 0.11f)); },
             5000, 31, { 86, 142, 72, 255 }, { 54, 104, 52, 255 });
    // grass
    DrawRectangle(0, 98, SVIJET_W, 82, { 106, 168, 79, 255 });
    DrawRectangle(0, 98, SVIJET_W, 1, { 143, 207, 95, 255 });
    // stećci, roughly every 70 pixels but not always
    int prvi = (int)std::floor((kam - 40) / 70.0f), zadnji = (int)std::floor((kam + SVIJET_W) / 70.0f);
    for (int c = prvi; c <= zadnji; c++) {
        unsigned int h = Hash(c * 31 + 7);
        if (h % 100 >= 45) continue;
        int v = (h / 100) % 4;
        float sx = c * 70.0f + (h / 7) % 40 - kam;
        int w = stecakW[v], hh = stecakH[v];
        DrawEllipse((int)(sx + w / 2), 103, w / 2 + 2, 2, { 70, 120, 56, 255 });
        DrawTexture(stecci[v], (int)std::floor(sx), 104 - hh, WHITE);
    }
    // the path
    DrawRectangle(0, 104, SVIJET_W, 14, { 182, 144, 96, 255 });
    DrawRectangle(0, 104, SVIJET_W, 1, { 150, 114, 72, 255 });
    DrawRectangle(0, 117, SVIJET_W, 1, { 150, 114, 72, 255 });
    int k0 = (int)std::floor(kam);
    for (int x = 0; x < SVIJET_W; x++) {
        unsigned int h = Hash(x + k0);
        if (h % 9 == 0) DrawPixel(x, 106 + (h / 9) % 10, { 150, 114, 72, 255 });
        if (h % 41 == 0) DrawRectangle(x, 107 + (h / 41) % 8, 2, 1, { 206, 180, 140, 255 });
    }
    // grass tufts and flowers up front
    for (int x = -4; x < SVIJET_W + 4; x++) {
        int wx = x + k0;
        unsigned int h = Hash(wx * 13 + 1);
        if (h % 11 == 0) {
            int y = 120 + (h / 11) % 50;
            DrawPixel(x, y, { 70, 128, 56, 255 });
            DrawPixel(x - 1, y + 1, { 70, 128, 56, 255 });
            DrawPixel(x + 1, y + 1, { 70, 128, 56, 255 });
        }
        if (h % 97 == 0) {
            int y = 121 + (h / 97) % 50;
            Color cv[] = { { 250, 230, 90, 255 }, { 250, 250, 250, 255 }, { 190, 120, 220, 255 }, { 240, 110, 110, 255 } };
            DrawPixel(x, y, cv[(h / 7) % 4]);
            DrawPixel(x, y + 1, { 70, 128, 56, 255 });
        }
        if (h % 17 == 0) {
            DrawPixel(x, 102, { 80, 140, 60, 255 });
            DrawPixel(x, 101, { 80, 140, 60, 255 });
        }
    }
    if (nocniRezim) NocniFilter(40);
}

// Swamp (level 2)
// dead tree, trunk and a few dry branches
static void MrtvoDrvo(int x, int dnoY, int visina, Color c, unsigned int h) {
    DrawRectangle(x, dnoY - visina, 3, visina, c);
    DrawRectangle(x - 1, dnoY - 2, 5, 2, c);
    for (int k = 0; k < 3; k++) {
        int y = dnoY - visina + 4 + k * visina / 4;
        int dir = ((h >> k) & 1) ? 1 : -1, d = 3 + (h >> (k + 3)) % 4;
        for (int i = 0; i < d; i++) DrawPixel(x + 1 + dir * (2 + i), y - i / 2, c);
    }
}

// willow, trunk and hanging branches
static void Vrba(int x, int dnoY, Color list, Color tm, Color deblo) {
    DrawRectangle(x, dnoY - 10, 2, 10, deblo);
    DrawEllipse(x + 1, dnoY - 13, 7, 4, list);
    for (int i = -6; i <= 7; i += 2) DrawRectangle(x + i, dnoY - 12, 1, 6 + (i * 7 & 3), tm);
}

static void CrtajMocvaru(float kam, float t) {
    // misty green sky and a pale sun behind the haze
    DrawRectangleGradientV(0, 0, SVIJET_W, 70, { 58, 78, 70, 255 }, { 132, 146, 112, 255 });
    DrawRectangleGradientV(0, 70, SVIJET_W, 30, { 132, 146, 112, 255 }, { 160, 166, 120, 255 });
    DrawCircle(250, 34, 12, { 220, 220, 170, 60 });
    DrawCircle(250, 34, 7, { 226, 226, 186, 160 });
    // dead trees far away
    int p0 = (int)std::floor(kam * 0.12f);
    for (int wx = p0 - 10; wx < p0 + SVIJET_W + 10; wx++) {
        unsigned int h = Hash(wx * 7 + 11);
        if (h % 19 == 0) MrtvoDrvo(wx - p0, 86, 18 + h % 16, { 72, 88, 76, 255 }, h);
    }
    DrawRectangle(0, 84, SVIJET_W, 16, { 76, 96, 80, 255 });
    // middle layer: mossy mounds with willows
    SlojBrda(kam * 0.3f, [](int wx) { return (int)(88 - 4 * std::sin(wx * 0.04f) - 3 * std::sin(wx * 0.09f + 1)); },
             900, 37, { 62, 92, 60, 255 }, { 46, 74, 44, 255 });
    int p1 = (int)std::floor(kam * 0.3f);
    for (int wx = p1 - 10; wx < p1 + SVIJET_W + 10; wx++)
        if (Hash(wx + 4242) % 53 == 0) Vrba(wx - p1, 90, { 70, 110, 58, 255 }, { 52, 88, 44, 255 }, { 60, 46, 34, 255 });
    // fog slowly drifting by
    for (int i = 0; i < 4; i++) {
        float x = std::fmod(i * 110.0f + t * (4 + i) - kam * 0.2f + 10000.0f, 440.0f) - 80.0f;
        DrawEllipse((int)x, 80 + i * 3, 60, 4, { 200, 210, 190, 40 });
    }
    // murky water with lily pads and ripples
    int p2 = (int)std::floor(kam * 0.55f);
    DrawRectangle(0, 92, SVIJET_W, 12, { 54, 82, 70, 255 });
    DrawRectangle(0, 92, SVIJET_W, 1, { 90, 120, 100, 255 });
    for (int x = 0; x < SVIJET_W; x++) {
        unsigned int h = Hash(x + p2);
        if ((h % 13) == 0) DrawPixel(x, 95 + (h / 13) % 7 + (int)(std::sin(t * 2 + x) * 0.6f), { 96, 128, 110, 255 });
        if ((h % 71) == 0) { DrawEllipse(x, 98 + (h / 71) % 4, 3, 1, { 70, 130, 60, 255 }); DrawPixel(x + 1, 97 + (h / 71) % 4, { 230, 120, 160, 255 }); }
        if ((h % 29) == 0) for (int k = 0; k < 6; k++) DrawPixel(x + (k & 1), 103 - k - (h % 4), { 70, 104, 50, 255 });   // reeds
    }
    // muddy path with puddles
    DrawRectangle(0, 104, SVIJET_W, 14, { 96, 80, 56, 255 });
    DrawRectangle(0, 104, SVIJET_W, 1, { 70, 58, 40, 255 });
    DrawRectangle(0, 117, SVIJET_W, 1, { 70, 58, 40, 255 });
    int k0 = (int)std::floor(kam);
    for (int x = -10; x < SVIJET_W + 10; x++) {
        unsigned int h = Hash(x + k0 + 777);
        if (h % 9 == 0) DrawPixel(x, 106 + (h / 9) % 10, { 74, 62, 42, 255 });
        if (h % 97 == 0) DrawEllipse(x, 109 + (h / 97) % 5, 6, 1, { 60, 84, 72, 255 });   // puddle
    }
    // rotten logs, stumps and skulls on stakes instead of stećci
    int prvi = (int)std::floor((kam - 40) / 70.0f), zadnji = (int)std::floor((kam + SVIJET_W) / 70.0f);
    for (int c = prvi; c <= zadnji; c++) {
        unsigned int h = Hash(c * 31 + 9);
        if (h % 100 >= 45) continue;
        int sx = (int)std::floor(c * 70.0f + (h / 7) % 40 - kam);
        switch ((h / 100) % 3) {
        case 0:   // rotten log
            DrawRectangle(sx, 98, 22, 6, { 74, 56, 40, 255 });
            DrawRectangle(sx, 98, 22, 1, { 100, 78, 54, 255 });
            DrawEllipse(sx + 21, 101, 2, 3, { 120, 96, 66, 255 });
            for (int i = 0; i < 4; i++) DrawPixel(sx + 3 + i * 5, 97, { 80, 130, 60, 255 });
            break;
        case 1:   // stump with mushrooms
            DrawRectangle(sx + 4, 94, 8, 10, { 82, 62, 44, 255 });
            DrawRectangle(sx + 4, 94, 8, 1, { 120, 96, 66, 255 });
            DrawRectangle(sx + 1, 100, 3, 2, { 200, 70, 60, 255 }); DrawPixel(sx + 2, 99, { 240, 220, 200, 255 });
            break;
        default:  // skull on a stake
            DrawRectangle(sx + 6, 88, 2, 16, { 96, 70, 46, 255 });
            DrawRectangle(sx + 4, 84, 6, 5, { 226, 220, 196, 255 });
            DrawPixel(sx + 5, 86, { 30, 24, 24, 255 }); DrawPixel(sx + 8, 86, { 30, 24, 24, 255 });
            break;
        }
    }
    // front: dark grass, reeds and fireflies
    DrawRectangle(0, 118, SVIJET_W, 62, { 60, 86, 50, 255 });
    DrawRectangle(0, 118, SVIJET_W, 1, { 82, 112, 62, 255 });
    for (int x = -4; x < SVIJET_W + 4; x++) {
        unsigned int h = Hash((x + k0) * 13 + 5);
        if (h % 11 == 0) { int y = 121 + (h / 11) % 50; DrawPixel(x, y, { 44, 70, 40, 255 }); DrawPixel(x - 1, y + 1, { 44, 70, 40, 255 }); DrawPixel(x + 1, y + 1, { 44, 70, 40, 255 }); }
        if (h % 83 == 0) { int y = 124 + (h / 83) % 46; for (int k = 0; k < 7; k++) DrawPixel(x + (k > 4), y - k, { 76, 110, 54, 255 }); DrawRectangle(x, y - 9, 2, 3, { 110, 74, 44, 255 }); }
    }
    for (int i = 0; i < 14; i++) {
        unsigned int h = Hash(i * 97 + 3);
        float x = std::fmod(h % SVIJET_W + std::sin(t * 0.7f + i) * 10 - kam * 0.8f + 10000.0f, (float)SVIJET_W);
        float y = 70 + (h / 7) % 90 + std::sin(t * 1.3f + i * 2) * 4;
        float sjaj = 0.5f + 0.5f * std::sin(t * 3 + i * 1.7f);
        DrawCircle((int)x, (int)y, 2, { 220, 255, 120, (unsigned char)(50 * sjaj) });
        DrawPixel((int)x, (int)y, { 230, 255, 150, (unsigned char)(255 * sjaj) });
    }
    if (nocniRezim) NocniFilter(40);
}

// Dungeon (level 3)
static void Baklja(int x, int y, float t, int seed) {
    DrawRectangle(x, y, 2, 7, { 90, 62, 40, 255 });
    DrawRectangle(x - 1, y + 2, 4, 1, { 60, 60, 70, 255 });
    float tr = std::sin(t * 13 + seed) * 0.5f + std::sin(t * 7.3f + seed * 2) * 0.5f;
    DrawCircle(x + 1, y - 3, 16 + tr * 2, { 255, 160, 60, 26 });
    DrawCircle(x + 1, y - 3, 9 + tr, { 255, 180, 80, 40 });
    DrawRectangle(x - 1, y - 4 + (int)tr, 4, 4, { 255, 140, 40, 255 });
    DrawRectangle(x, y - 6 + (int)tr, 2, 3, { 255, 220, 120, 255 });
}

static void CrtajTamnicu(float kam, float t) {
    // stone block wall in the back (scrolls slower)
    DrawRectangle(0, 0, SVIJET_W, 104, { 44, 42, 54, 255 });
    int pz = (int)std::floor(kam * 0.4f);
    for (int y = 6; y < 104; y += 8) {
        DrawRectangle(0, y, SVIJET_W, 1, { 30, 28, 38, 255 });
        int pom = ((y / 8) % 2) * 8;
        for (int x = -((pz + pom) % 16); x < SVIJET_W; x += 16) DrawRectangle(x, y, 1, 8, { 30, 28, 38, 255 });
    }
    for (int x = 0; x < SVIJET_W; x++) {   // stone shades
        unsigned int h = Hash(x + pz * 3);
        if (h % 5 == 0) DrawPixel(x, 8 + (h / 5) % 94, { 56, 54, 66, 255 });
    }
    // arches and pillars with torches and chains
    int ps = (int)std::floor(kam * 0.7f);
    for (int c = (ps - 120) / 90; c <= (ps + SVIJET_W + 60) / 90; c++) {
        int x = c * 90 - ps;
        DrawRectangle(x, 0, 14, 104, { 62, 60, 74, 255 });
        DrawRectangle(x, 0, 2, 104, { 80, 78, 94, 255 });
        DrawRectangle(x + 12, 0, 2, 104, { 40, 38, 50, 255 });
        for (int y = 10; y < 104; y += 12) DrawRectangle(x, y, 14, 1, { 40, 38, 50, 255 });
        DrawRing({ (float)x + 52, 30 }, 34, 38, 180, 360, 20, { 62, 60, 74, 255 });   // arch
        unsigned int h = Hash(c * 17 + 1);
        if (h % 2) Baklja(x + 30, 52, t, c);
        if (h % 3 == 0) {   // chains
            for (int k = 0; k < 9; k++) DrawRectangle(x + 64 + (k % 2), 8 + k * 3, 2, 2, { 110, 108, 120, 255 });
            DrawRectangle(x + 62, 36, 6, 3, { 110, 108, 120, 255 });
        }
        if (h % 5 == 1) {   // barred window
            DrawRectangle(x + 44, 40, 14, 18, { 18, 16, 26, 255 });
            for (int k = 0; k < 4; k++) DrawRectangle(x + 46 + k * 3, 40, 1, 18, { 90, 88, 100, 255 });
        }
    }
    // darker at the top
    DrawRectangleGradientV(0, 0, SVIJET_W, 30, { 10, 8, 16, 230 }, { 10, 8, 16, 0 });
    // stone floor
    DrawRectangle(0, 104, SVIJET_W, 14, { 82, 78, 90, 255 });
    DrawRectangle(0, 104, SVIJET_W, 1, { 110, 106, 120, 255 });
    int k0 = (int)std::floor(kam);
    for (int x = -((k0) % 12 + 12) % 12; x < SVIJET_W; x += 12) DrawRectangle(x, 104, 1, 14, { 60, 56, 68, 255 });
    DrawRectangle(0, 111, SVIJET_W, 1, { 60, 56, 68, 255 });
    // front: darker floor, cracks, bones and skulls
    DrawRectangle(0, 118, SVIJET_W, 62, { 52, 48, 60, 255 });
    DrawRectangle(0, 118, SVIJET_W, 1, { 70, 66, 80, 255 });
    for (int x = -6; x < SVIJET_W + 6; x++) {
        unsigned int h = Hash((x + k0) * 11 + 3);
        if (h % 23 == 0) { int y = 122 + (h / 23) % 50; DrawLine(x, y, x + 4, y + 2, { 36, 32, 44, 255 }); }
        if (h % 131 == 0) { int y = 124 + (h / 131) % 44; DrawRectangle(x, y, 5, 1, { 210, 204, 186, 255 }); DrawRectangle(x, y - 1, 1, 3, { 210, 204, 186, 255 }); DrawRectangle(x + 4, y - 1, 1, 3, { 210, 204, 186, 255 }); }
        if (h % 211 == 0) { int y = 126 + (h / 211) % 40; DrawRectangle(x, y, 5, 4, { 220, 214, 196, 255 }); DrawPixel(x + 1, y + 1, { 30, 26, 30, 255 }); DrawPixel(x + 3, y + 1, { 30, 26, 30, 255 }); }
    }
    // dark edges on the sides
    DrawRectangleGradientH(0, 0, 40, SVIJET_H, { 0, 0, 0, 120 }, { 0, 0, 0, 0 });
    DrawRectangleGradientH(SVIJET_W - 40, 0, 40, SVIJET_H, { 0, 0, 0, 0 }, { 0, 0, 0, 120 });
}

// Volcanic Realm (level 4)
// lava color from dark red to white hot (k = 0..1)
static Color Lava(float k) {
    k = std::max(0.0f, std::min(1.0f, k));
    if (k < 0.5f) { float a = k / 0.5f; return { (unsigned char)(170 + 85 * a), (unsigned char)(40 + 90 * a), (unsigned char)(10 + 20 * a), 255 }; }
    float a = (k - 0.5f) / 0.5f;
    return { 255, (unsigned char)(130 + 110 * a), (unsigned char)(30 + 130 * a), 255 };
}

static void CrtajVulkan(float kam, float t) {
    // red and black sky with a glowing horizon
    DrawRectangleGradientV(0, 0, SVIJET_W, 60, { 22, 6, 10, 255 }, { 104, 26, 20, 255 });
    DrawRectangleGradientV(0, 60, SVIJET_W, 40, { 104, 26, 20, 255 }, { 200, 78, 30, 255 });
    // layers of ash clouds lit red from below
    for (int sl = 0; sl < 3; sl++) {
        float pomak = kam * (0.02f + sl * 0.015f) + t * (1.5f + sl);
        int dno = 10 + sl * 12;
        for (int x = 0; x < SVIJET_W; x++) {
            float wx = x + pomak;
            float d = 4 + 3 * std::sin(wx * 0.045f + sl * 2) + 2 * std::sin(wx * 0.13f + sl) + 1.5f * std::sin(wx * 0.31f);
            if (d < 1) continue;
            unsigned char a = (unsigned char)(150 - sl * 30);
            DrawRectangle(x, dno - (int)d, 1, (int)d, { (unsigned char)(34 + sl * 10), (unsigned char)(16 + sl * 4), (unsigned char)(20 + sl * 3), a });
            DrawPixel(x, dno, { (unsigned char)(150 + sl * 30), (unsigned char)(50 + sl * 12), 30, (unsigned char)(a * 0.8f) });
        }
    }
    // lightning in the ash clouds, rare and quick
    {
        float ciklus = std::fmod(t, 5.3f);
        if (ciklus < 0.18f && (int)(ciklus * 30) % 2 == 0) {
            int sx = 60 + (int)(Hash((int)(t / 5.3f)) % 200) - (int)(kam * 0.06f) % 40;
            int lx = sx, ly = 8;
            for (int i = 0; i < 12; i++) {
                int nx = lx + (int)(Hash(i * 7 + (int)(t / 5.3f)) % 7) - 3, ny = ly + 3;
                DrawLine(lx, ly, nx, ny, { 255, 220, 200, 230 });
                lx = nx; ly = ny;
            }
            DrawRectangle(0, 0, SVIJET_W, 50, { 255, 120, 80, 25 });
        }
    }
    // mountain range far away with veins of lava
    int pd = (int)std::floor(kam * 0.03f);
    for (int x = 0; x < SVIJET_W; x++) {
        int wx = x + pd;
        int y = (int)(70 - 12 * std::fabs(std::sin(wx * 0.021f)) - 7 * std::fabs(std::sin(wx * 0.057f + 2)));
        DrawRectangle(x, y, 1, 100 - y, { 58, 26, 30, 255 });
        if (Hash(wx / 2 * 5 + 1) % 41 == 0) DrawPixel(x, y + 2 + (int)(Hash(wx) % 4), Lava(0.15f + 0.15f * std::sin(t * 2 + wx)));
    }
    // the big volcano: burning crater, lava running down, smoke and eruptions
    int pv = (int)std::floor(kam * 0.06f);
    int vx = 200 - pv % 400;
    for (int y = 26; y < 100; y++) {
        int w = (y - 26) * 2 + 12;
        DrawRectangle(vx - w / 2, y, w, 1, { 40, 18, 22, 255 });
        DrawRectangle(vx - w / 2, y, std::max(1, w / 5), 1, { 62, 30, 32, 255 });   // lit side
    }
    float krater = 0.6f + 0.4f * std::sin(t * 3);
    DrawEllipse(vx, 26, 14, 6, { 255, 120, 40, (unsigned char)(70 * krater) });
    DrawRectangle(vx - 6, 26, 12, 2, Lava(0.7f + 0.3f * krater));
    for (int k = 0; k < 4; k++) {   // lava streams, the bright part travels downhill
        int x = vx - 3 + k * 2, y = 28;
        for (int i = 0; i < 70 && y < 99; i++) {
            x += (int)std::round(std::sin(i * 0.45f + k * 2.3f)) * (k % 2 ? 1 : -1);
            y += 1;
            int px = x + (k - 1.5f) * i / 2.6f;
            float sj = 0.55f + 0.45f * std::sin(i * 0.5f - t * 4 + k);
            DrawPixel(px, y, Lava(sj * (1 - i / 110.0f)));
        }
    }
    for (int i = 0; i < 10; i++) {   // smoke, lit from below
        float f = std::fmod(t * 0.12f + i / 10.0f, 1.0f);
        float r = 6 + f * 14;
        int cx = vx + (int)(std::sin(i * 2.1f) * 8 + f * 34), cy = (int)(24 - f * 34);
        DrawCircle(cx, cy, r, { 52, 40, 42, (unsigned char)(160 * (1 - f)) });
        DrawCircle(cx, cy + (int)(r * 0.5f), r * 0.6f, { 140, 50, 30, (unsigned char)(60 * (1 - f) * (1 - f)) });
    }
    {   // eruption: glowing rocks flying out of the crater
        float ciklus = std::fmod(t, 4.0f);
        if (ciklus < 2.2f) {
            int seme = (int)(t / 4.0f);
            for (int k = 0; k < 7; k++) {
                unsigned int h = Hash(seme * 31 + k);
                float f = ciklus / 2.2f;
                float brzX = ((int)(h % 60) - 30) * 0.9f, brzY = 40 + h % 25;
                for (int tr = 3; tr >= 0; tr--) {
                    float ff = std::max(0.0f, f - tr * 0.025f) * 2.2f;
                    float bx = vx + brzX * ff, by = 26 - brzY * ff + 32 * ff * ff;
                    if (by > 100) continue;
                    DrawPixel((int)bx, (int)by, tr == 0 ? Color{ 255, 240, 170, 255 } : Fade(Lava(0.6f - tr * 0.12f), 1 - tr * 0.25f));
                }
            }
        }
    }
    // middle layer: jagged black rocks with glowing cracks and lava falls
    int p1 = (int)std::floor(kam * 0.3f);
    int vrhStijene[SVIJET_W];
    for (int x = 0; x < SVIJET_W; x++) {
        int wx = x + p1;
        int y = (int)(80 - 10 * std::fabs(std::sin(wx * 0.05f)) - 6 * std::fabs(std::sin(wx * 0.13f + 1)));
        vrhStijene[x] = y;
        DrawRectangle(x, y, 1, 100 - y, { 38, 26, 28, 255 });
        if (Hash(wx * 7) % 3 == 0) DrawPixel(x, y, { 92, 44, 36, 255 });   // lit edge
        if (Hash(wx * 3) % 29 == 0) DrawRectangle(x, y + 4, 1, 3 + (int)(Hash(wx) % 4), Lava(0.3f + 0.15f * std::sin(t * 3 + wx)));
    }
    for (int c = p1 / 140 - 1; c <= (p1 + SVIJET_W) / 140 + 1; c++) {   // a few lava falls from the rocks into the river
        unsigned int h = Hash(c * 13 + 3);
        if (h % 3 == 0) continue;
        int x = c * 140 + (int)(h % 90) - p1;
        if (x < 1 || x >= SVIJET_W - 3) continue;
        int vrh = std::min(vrhStijene[x], std::min(vrhStijene[x + 1], vrhStijene[x + 2])) + 3;
        DrawRectangle(x - 1, vrh, 5, 92 - vrh, { 255, 120, 40, 40 });   // glow around the fall
        for (int y = vrh; y < 92; y++)
            for (int dx = 0; dx < 3; dx++) {
                float sj = 0.5f + 0.5f * std::sin(y * 0.7f - t * 12 + dx * 2 + c);
                DrawPixel(x + dx, y, Lava((dx == 1 ? 0.6f : 0.4f) + 0.35f * sj));
            }
        for (int k = 0; k < 3; k++) {   // splashes in the river
            float f = std::fmod(t * 2 + k * 0.33f, 1.0f);
            DrawPixel(x + 1 + (k - 1) * (int)(1 + f * 3), 92 - (int)(std::sin(f * PI) * 3), Lava(0.9f - f * 0.4f));
        }
    }
    // demon temple ruins across the river: broken pillars with glowing runes, skulls on stakes
    {
        float pr = kam * 0.42f;
        int prvi = (int)std::floor(pr / 90.0f) - 1, zadnji = (int)std::floor((pr + SVIJET_W) / 90.0f) + 1;
        for (int c = prvi; c <= zadnji; c++) {
            unsigned int h = Hash(c * 17 + 3);
            int sx = (int)std::floor(c * 90.0f + h % 40 - pr);
            if (h % 3 == 0) {   // pillar
                int v = 14 + (h / 3) % 12;
                DrawRectangle(sx, 92 - v, 6, v, { 52, 36, 40, 255 });
                DrawRectangle(sx, 92 - v, 1, v, { 80, 50, 50, 255 });
                DrawRectangle(sx - 1, 92 - v, 8, 2, { 62, 42, 46, 255 });
                DrawRectangle(sx + 4 - (int)(h % 3), 92 - v - 2, 3, 2, { 52, 36, 40, 255 });   // broken top
                float sj = 0.5f + 0.5f * std::sin(t * 2 + c);
                for (int k = 0; k < 3; k++) DrawPixel(sx + 2 + k % 2, 92 - v + 4 + k * 3, Lava(0.3f + 0.5f * sj));   // runes
            } else if (h % 3 == 1) {   // skull on a stake
                DrawRectangle(sx + 2, 80, 1, 12, { 70, 50, 40, 255 });
                DrawRectangle(sx + 1, 78, 3, 3, { 210, 200, 180, 255 });
                DrawPixel(sx + 1, 79, { 30, 20, 20, 255 }); DrawPixel(sx + 3, 79, { 30, 20, 20, 255 });
            } else {   // half collapsed gate
                DrawRectangle(sx, 74, 4, 18, { 48, 34, 38, 255 });
                DrawRectangle(sx + 14, 80, 4, 12, { 48, 34, 38, 255 });
                for (int k = 0; k < 10; k++) DrawRectangle(sx + 2 + k, 72 + (int)(std::fabs(k - 5) * 0.6f), 2, 3, { 48, 34, 38, 255 });
                DrawPixel(sx + 8, 75, Lava(0.6f + 0.4f * std::sin(t * 3 + c)));   // eye above the gate
            }
        }
    }
    // the lava river: current, floating crust, bubbles and heat shimmer
    DrawRectangle(0, 92, SVIJET_W, 12, { 214, 74, 18, 255 });
    int p2 = (int)std::floor(kam * 0.55f + t * 8);
    for (int x = 0; x < SVIJET_W; x++) {
        unsigned int h = Hash((x + p2) / 3);
        DrawPixel(x, 93 + h % 10, { 255, 190, 60, 255 });
        if (h % 7 == 0) DrawPixel(x, 94 + (h / 7) % 8, { 255, 240, 140, 255 });
        float val = std::sin((x + p2) * 0.18f) + std::sin((x + p2) * 0.07f + 1);
        if (val > 1.2f) DrawPixel(x, 97 + (int)(val * 2) % 4, { 255, 220, 110, 255 });
    }
    for (int k = 0; k < 6; k++) {   // cooled crust floating by
        unsigned int h = Hash(k * 41 + 9);
        float px = std::fmod(h % 400 - kam * 0.55f - t * 8 + 4000.0f, 400.0f) - 40;
        int py = 95 + (int)(h / 400 % 6);
        int w = 5 + h % 6;
        DrawRectangle((int)px, py, w, 2, { 70, 34, 26, 255 });
        DrawRectangle((int)px + 1, py - 1, w - 2, 1, { 96, 46, 30, 255 });
        DrawPixel((int)px + w / 2, py, { 255, 160, 60, 255 });   // crack
    }
    DrawRectangle(0, 92, SVIJET_W, 1, { 255, 200, 90, 255 });
    DrawRectangle(0, 91, SVIJET_W, 1, { 255, 140, 60, 70 });
    for (int i = 0; i < 7; i++) {   // popping lava bubbles
        float f = std::fmod(t * 0.8f + i * 0.37f, 1.0f);
        int x = (int)(std::fmod(Hash(i * 13) % SVIJET_W - kam * 0.55f + 4000.0f, (float)SVIJET_W));
        int y = 95 + (int)(Hash(i * 5) % 7);
        if (f < 0.8f) DrawCircle(x, y - (int)(f * 2), 1 + f * 1.5f, { 255, 220, 120, (unsigned char)(255 * (1 - f)) });
        else { DrawPixel(x - 2, y - 3, { 255, 230, 140, 200 }); DrawPixel(x + 2, y - 4, { 255, 230, 140, 200 }); DrawPixel(x, y - 5, { 255, 230, 140, 200 }); }
    }
    for (int x = 0; x < SVIJET_W; x += 2) {   // heat shimmer above the river
        float o = std::sin(x * 0.2f + t * 6);
        if (o > 0.7f) DrawPixel(x, 89 + (int)(o * 2), { 255, 200, 120, 50 });
    }
    // basalt path with glowing cracks
    DrawRectangle(0, 104, SVIJET_W, 14, { 64, 48, 44, 255 });
    DrawRectangle(0, 104, SVIJET_W, 1, { 120, 60, 40, 255 });
    DrawRectangle(0, 117, SVIJET_W, 1, { 40, 30, 28, 255 });
    int k0 = (int)std::floor(kam);
    for (int x = -10; x < SVIJET_W + 10; x++) {
        unsigned int h = Hash(x + k0 + 999);
        if (h % 9 == 0) DrawPixel(x, 106 + (h / 9) % 10, { 46, 34, 32, 255 });
        if (h % 61 == 0) { float g = 0.6f + 0.4f * std::sin(t * 3 + x); DrawLine(x, 107 + (h / 61) % 6, x + 5, 108 + (h / 61) % 6, { 255, (unsigned char)(100 * g), 30, 255 }); }
        if (h % 13 == 0) DrawPixel(x, 105 + (h / 13) % 11, { 92, 66, 58, 255 });   // lighter pebbles
        if (h % 211 == 0) {   // bones and skulls on the path
            int y = 108 + (h / 211) % 6;
            if (h % 2) { DrawRectangle(x, y, 3, 2, { 210, 200, 180, 255 }); DrawPixel(x, y, { 40, 30, 30, 255 }); DrawPixel(x + 2, y, { 40, 30, 30, 255 }); }
            else { DrawRectangle(x, y + 1, 5, 1, { 196, 186, 166, 255 }); DrawPixel(x, y, { 196, 186, 166, 255 }); DrawPixel(x + 4, y + 2, { 196, 186, 166, 255 }); }
        }
    }
    DrawRectangle(0, 103, SVIJET_W, 1, { 255, 120, 40, 50 });   // the river reflecting on the edge of the path
    // obsidian spikes (purple shine, embers at the base) and iron braziers with fire
    int prvi = (int)std::floor((kam - 40) / 70.0f), zadnji = (int)std::floor((kam + SVIJET_W) / 70.0f);
    for (int c = prvi; c <= zadnji; c++) {
        unsigned int h = Hash(c * 31 + 5);
        int sx = (int)std::floor(c * 70.0f + (h / 7) % 40 - kam);
        if (h % 100 < 45) {
            int v = 10 + h % 12;
            DrawEllipse(sx + 4, 104, 7, 2, { 255, 100, 30, (unsigned char)(60 + 30 * std::sin(t * 2 + c)) });
            for (int i = 0; i < v; i++) DrawRectangle(sx + i / 3, 104 - i, std::max(1, 8 - i * 2 / 3), 1, { 30, 22, 30, 255 });
            DrawRectangle(sx + 1, 104 - v / 2, 1, v / 2, { 90, 70, 110, 255 });
            DrawPixel(sx + 2, 104 - v / 2 - 1, { 160, 140, 200, 255 });
            if (h % 4 == 0) {   // smaller spike next to the big one
                for (int i = 0; i < v / 2; i++) DrawRectangle(sx + 8 + i / 3, 104 - i, std::max(1, 4 - i / 2), 1, { 36, 26, 34, 255 });
            }
        } else if (h % 100 < 62) {   // brazier: post, bowl and a flickering flame
            DrawRectangle(sx + 3, 92, 2, 12, { 40, 36, 40, 255 });
            DrawRectangle(sx + 1, 103, 6, 1, { 40, 36, 40, 255 });
            DrawRectangle(sx, 89, 8, 3, { 60, 54, 58, 255 });
            DrawRectangle(sx, 89, 8, 1, { 96, 88, 92, 255 });
            DrawCircle(sx + 4, 88, 8, { 255, 140, 40, (unsigned char)(40 + 15 * std::sin(t * 9 + c)) });
            for (int k = 0; k < 6; k++) {
                float f = std::fmod(t * 2.2f + k * 0.17f + c * 0.3f, 1.0f);
                int fx = sx + 1 + k + (int)std::round(std::sin(t * 8 + k * 2) * 0.8f);
                int fv = (int)((4 + Hash(k + c) % 4) * (1 - f * 0.5f));
                DrawRectangle(fx, 89 - fv, 1, fv, Lava(0.5f + 0.5f * (1 - f)));
            }
            DrawRectangle(sx + 3, 87, 2, 2, { 255, 246, 200, 255 });
        }
    }
    // front: dark rock with glowing cracks, lava pools and steam vents
    DrawRectangle(0, 118, SVIJET_W, 62, { 46, 34, 32, 255 });
    DrawRectangle(0, 118, SVIJET_W, 1, { 90, 50, 36, 255 });
    for (int x = -8; x < SVIJET_W + 8; x++) {
        unsigned int h = Hash((x + k0) * 13 + 7);
        if (h % 19 == 0) DrawPixel(x, 122 + (h / 19) % 56, { 34, 24, 24, 255 });
        if (h % 23 == 0) DrawPixel(x, 121 + (h / 23) % 56, { 70, 52, 46, 255 });
        if (h % 83 == 0) {   // a crack that winds and pulses
            int px = x, py = 124 + (h / 83) % 48;
            float sj = 0.5f + 0.5f * std::sin(t * 2.5f + x * 0.3f);
            for (int i = 0; i < 9; i++) {
                DrawPixel(px, py, Lava(0.25f + 0.45f * sj - i * 0.02f));
                px += 1; py += (int)(Hash(h + i) % 3) - 1;
            }
        }
    }
    for (int x = -8; x < SVIJET_W + 8; x++) {   // lava pools with bubbles (on top of the cracks)
        unsigned int h = Hash((x + k0) * 13 + 7);
        if (h % 157 == 0) {
            int y = 128 + (h / 157) % 40;
            DrawEllipse(x, y, 9, 3, { 120, 40, 20, 255 });
            DrawEllipse(x, y, 7, 2, { 230, 90, 20, 255 });
            DrawEllipse(x, y, 4, 1, { 255, 200, 80, 255 });
            float f = std::fmod(t * 0.9f + (h % 10) * 0.1f, 1.0f);
            DrawCircle(x - 3 + (int)(h % 6), y - (int)(f * 2), 0.5f + f, { 255, 240, 160, (unsigned char)(255 * (1 - f)) });
            DrawEllipse(x, y, 12, 5, { 255, 120, 40, 25 });
        }
        if (h % 263 == 0) {   // vent with steam coming out
            int y = 124 + (h / 263) % 40;
            DrawRectangle(x - 2, y, 5, 1, { 20, 14, 14, 255 });
            DrawPixel(x, y, { 255, 150, 60, 255 });
            for (int k = 0; k < 4; k++) {
                float f = std::fmod(t * 0.6f + k * 0.25f, 1.0f);
                DrawCircle(x + (int)(std::sin(t * 2 + k) * 2 * f), y - (int)(f * 18), 1 + f * 3, { 160, 150, 150, (unsigned char)(90 * (1 - f)) });
            }
        }
    }
    for (int i = 0; i < 26; i++) {   // embers
        unsigned int h = Hash(i * 31 + 17);
        float f = std::fmod(t * (0.15f + (h % 5) * 0.03f) + i * 0.13f, 1.0f);
        float x = std::fmod(h % SVIJET_W - kam * 0.7f + std::sin(t + i) * 6 + 10000.0f, (float)SVIJET_W);
        DrawPixel((int)x, (int)(178 - f * 170), { 255, (unsigned char)(120 + 100 * (1 - f)), 40, (unsigned char)(255 * (1 - f)) });
    }
    for (int i = 0; i < 20; i++) {   // falling ash
        unsigned int h = Hash(i * 53 + 2);
        float f = std::fmod(t * 0.08f + i * 0.05f, 1.0f);
        DrawPixel((int)std::fmod(h % SVIJET_W + std::sin(t * 0.5f + i) * 8 - kam * 0.4f + 10000.0f, (float)SVIJET_W), (int)(f * 180), { 120, 110, 110, 200 });
    }
    for (int i = 0; i < 6; i++) {   // bigger glowing embers
        unsigned int h = Hash(i * 71 + 3);
        float f = std::fmod(t * 0.11f + i / 6.0f, 1.0f);
        float x = std::fmod(h % SVIJET_W - kam * 0.8f + std::sin(t * 1.3f + i) * 10 + 10000.0f, (float)SVIJET_W);
        float y = 176 - f * 160;
        DrawCircle((int)x, (int)y, 2, { 255, 140, 40, (unsigned char)(50 * (1 - f)) });
        DrawRectangle((int)x, (int)y, 2, 2, { 255, 210, 120, (unsigned char)(230 * (1 - f)) });
    }
    // the heat: reddish edges that pulse a little
    unsigned char vr = (unsigned char)(40 + 15 * std::sin(t * 1.5f));
    DrawRectangleGradientV(0, 150, SVIJET_W, 30, { 255, 80, 20, 0 }, { 255, 80, 20, vr });
    DrawRectangleGradientH(0, 0, 20, SVIJET_H, { 120, 20, 10, vr }, { 120, 20, 10, 0 });
    DrawRectangleGradientH(SVIJET_W - 20, 0, 20, SVIJET_H, { 120, 20, 10, 0 }, { 120, 20, 10, vr });
}

static int trenutnaMapa = 0;
void PostaviMapu(int mapa) { trenutnaMapa = mapa; }

void CrtajDolinu(float kam, float t) {
    switch (trenutnaMapa) {
        case 1: CrtajMocvaru(kam, t); break;
        case 2: CrtajTamnicu(kam, t); break;
        case 3: CrtajVulkan(kam, t); break;
        default: CrtajZelenuDolinu(kam, t); break;
    }
}

// small map previews for the level screen, drawn once at the start
static RenderTexture2D pregledMape[4];
static bool imaPregleda = false;
void PripremiPregledeMapa() {
    int stara = trenutnaMapa;
    bool noc = nocniRezim;
    nocniRezim = false;
    for (int m = 0; m < 4; m++) {
        pregledMape[m] = LoadRenderTexture(SVIJET_W, SVIJET_H);
        SetTextureFilter(pregledMape[m].texture, TEXTURE_FILTER_POINT);
        BeginTextureMode(pregledMape[m]);
        ClearBackground(BLACK);
        trenutnaMapa = m;
        CrtajDolinu(37, 3.2f);
        EndTextureMode();
    }
    trenutnaMapa = stara; nocniRezim = noc;
    imaPregleda = true;
}

void CrtajPregledMape(int mapa, Rectangle dst) {
    if (!imaPregleda || mapa < 0 || mapa > 3) return;
    Texture2D t = pregledMape[mapa].texture;
    // takes y 30 to 140 of the picture (landscape and path). Render textures are upside down,
    // so it grabs rows H-140 to H-30 and flips them with a negative height.
    const float y0 = 30, h = 110, w = dst.width * h / dst.height;
    DrawTexturePro(t, { (SVIJET_W - w) / 2, SVIJET_H - y0 - h, w, -h }, dst, { 0, 0 }, 0, WHITE);
}

void CrtajVjezbaliste(float t) {
    if (nocniRezim) {
        NocnoNebo(t, 70);
    } else {
        // morning sky
        DrawRectangleGradientV(0, 0, SVIJET_W, 70, { 120, 170, 226, 255 }, { 236, 226, 200, 255 });
        for (int i = 0; i < 4; i++) {
            float x = std::fmod(i * 120.0f + t * 2.0f, 440.0f) - 60.0f;
            Oblak(x, 8.0f + i * 9 % 20, 0.8f);
        }
    }
    // castle walls
    Color zid = { 132, 126, 118, 255 }, zidTm = { 98, 94, 90, 255 }, zidSv = { 160, 154, 144, 255 };
    DrawRectangle(0, 36, SVIJET_W, 64, zid);
    for (int x = 0; x < SVIJET_W; x += 14) DrawRectangle(x, 28, 8, 8, zid);   // battlements
    for (int x = 0; x < SVIJET_W; x += 14) DrawRectangle(x, 28, 8, 1, zidSv);
    for (int y = 36; y < 100; y += 6) {
        DrawRectangle(0, y, SVIJET_W, 1, zidTm);
        int pom = ((y / 6) % 2) * 6;
        for (int x = pom; x < SVIJET_W; x += 12) DrawRectangle(x, y, 1, 6, zidTm);
    }
    // tower
    DrawRectangle(18, 6, 46, 94, zid);
    for (int x = 18; x < 64; x += 12) DrawRectangle(x, 0, 8, 6, zid);
    for (int y = 6; y < 100; y += 6) {
        DrawRectangle(18, y, 46, 1, zidTm);
        int pom = ((y / 6) % 2) * 6;
        for (int x = 18 + pom; x < 64; x += 12) DrawRectangle(x, y, 1, 6, zidTm);
    }
    DrawRectangle(38, 14, 4, 10, { 30, 26, 34, 255 });  // window
    DrawRectangle(62, 6, 2, 94, zidTm);
    // Medieval Bosnian banner (Kotromanić): blue field, white sash and gold lilies. It hangs from a
    // bar and sways, the bottom moves more than the top and the folds show up as light and shadow.
    auto zastava = [&](int x, int y, float faza) {
        const int w = 16, h = 30;
        const char* ljiljan[4] = { ".Y.", "YYY", ".Y.", "Y.Y" };
        const int ljiljani[4][2] = { { 11, 4 }, { 12, 12 }, { 3, 16 }, { 4, 24 } };   // top left corner
        auto boja = [&](int u, int v) -> Color {
            for (auto& l : ljiljani) {
                int lu = u - l[0], lv = v - l[1];
                if (lu >= 0 && lu < 3 && lv >= 0 && lv < 4 && ljiljan[lv][lu] == 'Y') return { 240, 200, 60, 255 };
            }
            float d = u - v * (float)w / h;   // distance from the sash
            if (std::fabs(d) <= 1.8f) return { 236, 236, 240, 255 };
            if (u == 0 || u == w - 1) return { 26, 56, 128, 255 };   // darker edge
            return { 36, 76, 160, 255 };
        };
        for (int v = 0; v < h; v++) {
            float njihaj = std::sin(t * 2.2f + faza - v * 0.12f) * 2.2f * v / h;
            int dx = (int)std::round(njihaj);
            int rep = v - (h - 6);   // swallowtail cut in the bottom rows
            for (int u = 0; u < w; u++) {
                if (rep > 0 && std::fabs(u - (w - 1) / 2.0f) < rep * 0.8f) continue;
                Color c = boja(u, v);
                float nabor = 0.9f + 0.12f * std::sin(t * 3.0f + faza + u * 0.7f - v * 0.1f);
                c = { (unsigned char)std::min(255.0f, c.r * nabor), (unsigned char)std::min(255.0f, c.g * nabor),
                      (unsigned char)std::min(255.0f, c.b * nabor), 255 };
                DrawPixel(x + u + dx, y + v, c);
            }
        }
        DrawRectangle(x - 2, y - 2, w + 4, 2, { 110, 76, 44, 255 });           // bar
        DrawRectangle(x - 3, y - 3, 2, 3, { 240, 200, 60, 255 });               // gold tips on the bar
        DrawRectangle(x + w + 1, y - 3, 2, 3, { 240, 200, 60, 255 });
    };
    zastava(33, 30, 0);
    zastava(250, 40, 1.5f);
    // wooden fence
    for (int x = -2; x < SVIJET_W; x += 7) {
        int h = 26 + (int)(Hash(x + 99) % 4);
        DrawRectangle(x, 100 - h, 6, h, { 132, 90, 52, 255 });
        DrawRectangle(x, 100 - h, 1, h, { 168, 120, 72, 255 });
        DrawRectangle(x + 5, 100 - h, 1, h, { 96, 64, 38, 255 });
        DrawTriangle({ (float)x + 3, (float)(100 - h - 4) }, { (float)x, (float)(100 - h) }, { (float)x + 6, (float)(100 - h) }, { 150, 104, 62, 255 });
    }
    DrawRectangle(0, 82, SVIJET_W, 2, { 96, 64, 38, 255 });
    // sand
    DrawRectangle(0, 98, SVIJET_W, 82, { 200, 168, 112, 255 });
    DrawRectangle(0, 98, SVIJET_W, 2, { 168, 136, 86, 255 });
    for (int i = 0; i < 260; i++) {
        unsigned int h = Hash(i * 7 + 3);
        DrawPixel(h % SVIJET_W, 100 + (h / SVIJET_W) % 80, i % 3 ? Color{ 176, 144, 94, 255 } : Color{ 222, 196, 146, 255 });
    }
    // archery target
    DrawLine(146, 96, 140, 104, { 110, 76, 44, 255 });
    DrawLine(154, 96, 160, 104, { 110, 76, 44, 255 });
    DrawCircle(150, 86, 10, { 240, 236, 220, 255 });
    DrawCircle(150, 86, 8, { 200, 50, 50, 255 });
    DrawCircle(150, 86, 6, { 240, 236, 220, 255 });
    DrawCircle(150, 86, 4, { 200, 50, 50, 255 });
    DrawCircle(150, 86, 2, { 240, 200, 60, 255 });
    DrawLine(153, 83, 160, 78, { 110, 76, 44, 255 });  // arrow
    DrawPixel(161, 77, { 240, 236, 220, 255 });
    // weapon rack
    DrawRectangle(276, 80, 30, 2, { 110, 76, 44, 255 });
    DrawRectangle(278, 80, 2, 24, { 110, 76, 44, 255 });
    DrawRectangle(302, 80, 2, 24, { 110, 76, 44, 255 });
    for (int i = 0; i < 4; i++) DrawRectangle(283 + i * 5, 66, 1, 36, { 150, 104, 62, 255 });
    for (int i = 0; i < 4; i++) DrawRectangle(282 + i * 5, 64, 3, 3, { 190, 196, 206, 255 });
    DrawCircle(291, 96, 6, { 36, 76, 160, 255 });   // shield
    DrawCircle(291, 96, 2, { 240, 200, 60, 255 });
    // hay bales
    auto bala = [](int x, int y) {
        DrawRectangle(x, y, 22, 12, { 226, 192, 96, 255 });
        DrawRectangle(x, y, 22, 1, { 246, 220, 130, 255 });
        DrawRectangle(x + 5, y, 1, 12, { 170, 130, 60, 255 });
        DrawRectangle(x + 16, y, 1, 12, { 170, 130, 60, 255 });
        DrawRectangle(x, y + 11, 22, 1, { 170, 130, 60, 255 });
    };
    bala(2, 92); bala(14, 80); bala(26, 92);
    // barrel
    DrawRectangle(110, 90, 12, 14, { 130, 86, 48, 255 });
    DrawRectangle(110, 93, 12, 1, { 70, 60, 60, 255 });
    DrawRectangle(110, 100, 12, 1, { 70, 60, 60, 255 });
    // footprints in the sand
    for (int i = 0; i < 8; i++) DrawRectangle(84 + i * 16, 114 + (i % 2) * 3, 3, 1, { 176, 144, 94, 255 });
    if (nocniRezim) NocniFilter(0);
}

// Characters
void PostaviOklop(int oklop) { aktivniOklop = (oklop < 0 || oklop >= BROJ_IZGLEDA) ? 0 : oklop; }

static void SegmentPomak(float x, float y, float ugao, float a, float b, float debljina, float pomak, Color c);

// silueta != nullptr draws the whole knight (sword too) in one color, that's the locked armor look
static void VitezCrtez(int oklop, float x, float y, int okvir, float ugaoMaca, float bljesak, float rotacija, float alfa,
                       const Color* silueta) {
    const OklopBoje& o = OKLOPI[OklopIzgleda(oklop)];
    bool andjeo = OklopIzgleda(oklop) == OKLOP_ANDJEO;
    if (andjeo) okvir = 0;   // the angel floats, so his legs don't move
    const Sprite& s = okvir ? vitezKorak[oklop] : vitezMir[oklop];
    float bob = (okvir ? -1.0f : 0.0f);
    float vrijeme = (float)GetTime();
    // shadow (smaller for the angel since he's floating)
    DrawEllipse((int)x, (int)y, andjeo ? 6 : 8, 2, { 0, 0, 0, (unsigned char)((andjeo ? 40 : 60) * alfa) });
    if (andjeo) {
        float tloY = y;
        // float a bit above the ground and bob up and down
        y -= 4 + std::round(std::sin(vrijeme * 2.5f) * 1.0f);
        float srceY = y - 12 + bob;
        if (!silueta) {
            // column of light under the feet and a soft glow on the ground
            DrawRectangleGradientV((int)x - 4, (int)y - 2, 8, (int)(tloY - y) + 2, Fade({ 255, 246, 200, 255 }, 0), Fade({ 255, 246, 200, 255 }, 0.35f * alfa));
            DrawEllipse((int)x, (int)tloY, 10, 2, Fade({ 255, 236, 150, 255 }, 0.3f * alfa));
            // warm glow behind him with slowly turning rays
            DrawCircleV({ x - 3, srceY }, 11, Fade({ 255, 230, 150, 255 }, 0.10f * alfa));
            for (int k = 0; k < 8; k++) {
                float u = vrijeme * 0.35f + k * PI / 4;
                float d = 15 + 4 * std::sin(vrijeme * 2 + k * 1.7f);
                DrawLineV({ x - 3 + std::cos(u) * 7, srceY + std::sin(u) * 7 }, { x - 3 + std::cos(u) * d, srceY + std::sin(u) * d * 0.85f },
                          Fade({ 255, 226, 130, 255 }, 0.22f * alfa));
            }
        }
        // Big wings: long feathers packed close so they read as one shape with a jagged edge, then two
        // layers of shorter feathers and a gold edge on top. They flap slow and wide, and the far wing
        // is a bit blue and smaller.
        float mah = std::sin(vrijeme * 2.6f) * 12;
        static const float DUZ[11] = { 8, 10, 12.5f, 15, 17.5f, 19.5f, 21, 21.5f, 20, 17, 12 };
        for (int krilo = 1; krilo >= 0; krilo--) {
            float kx = x - 2 + krilo * 3, ky = y - 14 + bob + krilo;
            float sk = krilo ? 0.86f : 1.0f;
            Color pero  = silueta ? *silueta : (krilo ? Color{ 196, 206, 236, 255 } : Color{ 255, 255, 255, 255 });
            Color pero2 = silueta ? *silueta : (krilo ? Color{ 168, 180, 220, 255 } : Color{ 222, 230, 250, 255 });
            Color pero3 = silueta ? *silueta : (krilo ? Color{ 150, 162, 206, 255 } : Color{ 196, 208, 240, 255 });
            Color obris = silueta ? *silueta : Color{ 62, 64, 104, 255 };
            Color zlato = silueta ? *silueta : Color{ 255, 222, 120, 255 };
            auto ugaoPera = [&](int k) { return -198 + k * 10.5f + mah * (1 - k * 0.07f) - krilo * 9; };
            for (int k = 0; k < 11; k++) Segment(kx, ky, ugaoPera(k), 0, DUZ[k] * sk + 1, 5.2f, Fade(obris, alfa));
            for (int k = 0; k < 11; k++) {
                float u = ugaoPera(k), d = DUZ[k] * sk;
                Segment(kx, ky, u, 0, d, 3.6f, Fade(pero, alfa));
                SegmentPomak(kx, ky, u, d * 0.35f, d - 0.5f, 0.8f, -1.1f, Fade(pero3, alfa));                // shadow along the feather's edge
                if (k >= 4 && k <= 9) Segment(kx, ky, u, d - 1.6f, d, 3.6f, Fade(zlato, alfa));          // gold tips
            }
            for (int k = 1; k < 10; k += 1) {   // middle feathers
                float u = ugaoPera(k) + 3, d = DUZ[k] * sk * 0.62f;
                Segment(kx, ky, u, 0, d + 0.8f, 4.2f, Fade(obris, alfa));
            }
            for (int k = 1; k < 10; k += 1) {
                float u = ugaoPera(k) + 3, d = DUZ[k] * sk * 0.62f;
                Segment(kx, ky, u, 0, d, 3.0f, Fade(pero2, alfa));
            }
            for (int k = 2; k < 10; k += 2) {   // small feathers at the shoulder
                float u = ugaoPera(k) + 6, d = DUZ[k] * sk * 0.32f;
                Segment(kx, ky, u, 0, d + 0.6f, 3.6f, Fade(obris, alfa));
                Segment(kx, ky, u, 0, d, 2.6f, Fade(pero, alfa));
            }
            Segment(kx, ky, ugaoPera(8) + 2, 0, DUZ[8] * sk * 0.62f, 1.0f, Fade(zlato, alfa));          // gold top edge
        }
    }
    // where the hand holding the sword is, relative to the feet, turned with the body
    float hx = 3.5f, hy = -7.5f + bob;
    float r = rotacija * DEG2RAD;
    float rx = hx * std::cos(r) - hy * std::sin(r);
    float ry = hx * std::sin(r) + hy * std::cos(r);
    float mx = x + rx, my = y + ry;
    float ug = ugaoMaca + rotacija;
    auto boja = [&](Color c) { return Fade(silueta ? *silueta : c, alfa); };
    // The sword gets drawn pixel by pixel along a line instead of as rotated rectangles, so the
    // blade is always a clean pixel line and the outline, blade and shine line up perfectly.
    auto mac = [&]() {
        float u = ug * DEG2RAD;
        float dx = std::cos(u), dy = std::sin(u);
        float px = -dy, py = dx;
        float hx0 = std::round(mx), hy0 = std::round(my);
        auto tacka = [&](float s, float o, Color c) {
            DrawRectangle((int)std::round(hx0 + dx * s + px * o), (int)std::round(hy0 + dy * s + py * o), 1, 1, c);
        };
        const float VRH = 22;
        // outline: the blade plus a little cross stamped around every point
        for (float s = 2; s <= VRH + 1; s += 0.5f)
            for (int k = 0; k < 5; k++) {
                static const float ox[5] = { 0, 1, -1, 0, 0 }, oy[5] = { 0, 0, 0, 1, -1 };
                DrawRectangle((int)std::round(hx0 + dx * s) + (int)ox[k], (int)std::round(hy0 + dy * s) + (int)oy[k], 1, 1, boja({ 52, 58, 72, 255 }));
            }
        for (float s = 2; s <= VRH; s += 0.5f) {
            tacka(s, 0.7f, boja(o.sjecivo));                               // blade
            tacka(s, 0, boja(s < VRH - 1 ? Color{ 250, 252, 255, 255 } : o.sjecivo));   // shine down the middle
        }
        for (float k = -3; k <= 3; k += 0.5f) tacka(1.5f, k, boja(o.stitnik));             // guard
        for (float s = -2.5f; s <= 0.5f; s += 0.5f) tacka(s, 0, boja({ 110, 70, 40, 255 }));  // grip
        tacka(-3.5f, 0, boja(o.stitnik)); tacka(-3.5f, 0.7f, boja(o.stitnik));               // pommel
        tacka(-4.2f, 0, boja(o.stitnik));
    };
    if (silueta) {
        Rectangle src = { 0, 0, (float)s.w, (float)s.h };
        Rectangle dst = { std::floor(x), std::floor(y + bob), (float)s.w, (float)s.h };
        DrawTexturePro(s.bijela, src, dst, { s.w / 2.0f, (float)s.h }, rotacija, Fade(*silueta, alfa));
    } else {
        CrtajSprite(s, x, y + bob, bljesak, rotacija, alfa);
    }
    if (!andjeo) mac();
    else if (!silueta) {
        // no sword for the angel, he holds an orb of light and shoots beams from it
        Vector2 kugla = { std::round(mx) + 2, std::round(my) - 1 };
        float puls = 0.5f + 0.5f * std::sin(vrijeme * 5);
        DrawCircleV(kugla, 4.5f + puls, Fade({ 255, 236, 150, 255 }, 0.22f * alfa));
        DrawCircleV(kugla, 2.6f, Fade({ 255, 244, 190, 255 }, 0.8f * alfa));
        DrawCircleV(kugla, 1.4f, Fade(WHITE, alfa));
        for (int k = 0; k < 3; k++) {   // sparks circling the orb
            float u = vrijeme * 4 + k * 2.1f;
            DrawPixel((int)(kugla.x + std::cos(u) * 4), (int)(kugla.y + std::sin(u) * 2.5f), Fade({ 255, 250, 210, 255 }, alfa));
        }
        // halo: two shiny gold rings with short rays
        float sj = 0.6f + 0.4f * std::sin(vrijeme * 2);
        float oy = y - 24 + bob + std::sin(vrijeme * 1.7f) * 0.5f;
        DrawEllipse((int)x, (int)oy, 8, 3, Fade({ 255, 230, 120, 255 }, alfa * 0.22f * sj));
        DrawEllipseLines((int)x, (int)oy, 5, 1, Fade({ 255, 214, 80, 255 }, alfa));
        DrawEllipseLines((int)x, (int)oy - 1, 5, 1, Fade({ 255, 248, 200, 255 }, alfa * 0.8f));
        for (int k = 0; k < 6; k++) {
            float u = -PI + k * PI / 5;
            float d = 7 + 2 * std::sin(vrijeme * 3 + k);
            DrawLineV({ x + std::cos(u) * 5, oy - 1 + std::sin(u) * 2 }, { x + std::cos(u) * d, oy - 1 + std::sin(u) * d * 0.6f }, Fade({ 255, 236, 150, 255 }, 0.3f * sj * alfa));
        }
        // particles: gold and white sparks rising, little stars twinkling, a feather drifting down now and then
        for (int k = 0; k < 14; k++) {
            unsigned int h = Hash(k * 37 + 101);
            float f = std::fmod(vrijeme * (0.35f + (h % 5) * 0.05f) + k * 0.071f, 1.0f);
            float px = x - 16 + h % 30 + std::sin(vrijeme * 1.5f + k) * 2;
            float py = y + 2 - f * 34;
            Color b = k % 3 ? Color{ 255, 226, 120, 255 } : Color{ 255, 255, 255, 255 };
            DrawPixel((int)px, (int)py, Fade(b, std::sin(f * PI) * alfa));
        }
        for (int k = 0; k < 4; k++) {
            unsigned int h = Hash(k * 59 + 7);
            float f = std::fmod(vrijeme * 0.7f + k * 0.29f, 1.0f);
            if (f > 0.35f) continue;
            float a = std::sin(f / 0.35f * PI) * alfa;
            int sx = (int)(x - 14 + h % 26), sy = (int)(y - 6 - (h / 26) % 24);
            DrawPixel(sx, sy, Fade(WHITE, a));
            DrawPixel(sx - 1, sy, Fade({ 255, 236, 150, 255 }, a * 0.7f)); DrawPixel(sx + 1, sy, Fade({ 255, 236, 150, 255 }, a * 0.7f));
            DrawPixel(sx, sy - 1, Fade({ 255, 236, 150, 255 }, a * 0.7f)); DrawPixel(sx, sy + 1, Fade({ 255, 236, 150, 255 }, a * 0.7f));
        }
        for (int k = 0; k < 2; k++) {   // feathers floating down from the wings
            float f = std::fmod(vrijeme * 0.18f + k * 0.5f, 1.0f);
            float px = x - 10 - k * 6 - f * 6 + std::sin(f * 12 + k) * 3, py = y - 22 + f * 26;
            float u = std::sin(f * 12 + k) * 40 - 20;
            Segment(px, py, u, -1.5f, 1.5f, 1.2f, Fade({ 250, 250, 255, 255 }, (1 - f) * alfa));
        }
    }
    // celestial knight: little stars twinkling around him
    if (OklopIzgleda(oklop) == 5 && !silueta) {
        float t = (float)GetTime();
        for (int k = 0; k < 5; k++) {
            unsigned int h = Hash(k * 53 + 11);
            float faza = std::fmod(t * 0.9f + k * 0.37f, 1.0f);
            float sx = x - 9 + (h % 19), sy = y - 4 - (h / 19) % 22 - faza * 6;
            unsigned char a = (unsigned char)(255 * alfa * std::sin(faza * PI));
            DrawPixel((int)sx, (int)sy, { 220, 250, 255, a });
        }
    }
}

void CrtajViteza(float x, float y, int okvir, float ugaoMaca, float bljesak, float rotacija, float alfa) {
    VitezCrtez(aktivniOklop, x, y, okvir, ugaoMaca, bljesak, rotacija, alfa, nullptr);
}

bool JeAndjeo() { return OklopIzgleda(aktivniOklop) == OKLOP_ANDJEO; }

Vector2 RukaViteza(float x, float y) {
    if (!JeAndjeo()) return { x + 3.5f, y - 7.5f };
    float yh = y - 4 - std::round(std::sin((float)GetTime() * 2.5f));   // same bobbing as in VitezCrtez
    return { std::round(x + 3.5f) + 2, std::round(yh - 7.5f) - 1 };
}

void CrtajNabojAndjela(Vector2 ruka, float k) {
    if (k <= 0) return;
    float t = (float)GetTime();
    DrawCircleV(ruka, 3 + 7 * k, Fade({ 255, 236, 150, 255 }, 0.25f * k));
    DrawCircleV(ruka, 2 + 3 * k, Fade({ 255, 250, 220, 255 }, 0.7f));
    DrawCircleV(ruka, 1 + 1.5f * k, WHITE);
    for (int i = 0; i < 8; i++) {   // light gathering into the orb
        float u = i * PI / 4 + t * 3;
        float f = std::fmod(k * 2 + i / 8.0f, 1.0f);
        float d = (1 - f) * 16;
        DrawPixel((int)(ruka.x + std::cos(u) * d), (int)(ruka.y + std::sin(u) * d), Fade(i % 2 ? Color{ 255, 226, 120, 255 } : WHITE, f));
    }
    DrawLineV({ ruka.x - 6 * k, ruka.y }, { ruka.x + 6 * k, ruka.y }, Fade(WHITE, 0.6f * k));
    DrawLineV({ ruka.x, ruka.y - 6 * k }, { ruka.x, ruka.y + 6 * k }, Fade(WHITE, 0.6f * k));
}

void CrtajSvjetlosniZrak(Vector2 od, Vector2 cilj, float jacina) {
    if (jacina <= 0) return;
    float t = (float)GetTime();
    float dx = cilj.x - od.x, dy = cilj.y - od.y, duz = std::sqrt(dx * dx + dy * dy);
    if (duz < 1) return;
    float nx = -dy / duz, ny = dx / duz;
    float w = (5 + std::sin(t * 60) * 0.8f) * jacina;
    DrawLineEx(od, cilj, w * 2.4f, Fade({ 255, 220, 120, 255 }, 0.25f * jacina));   // outer glow
    DrawLineEx(od, cilj, w, Fade({ 255, 240, 190, 255 }, 0.85f * jacina));
    DrawLineEx(od, cilj, std::max(1.0f, w * 0.4f), Fade(WHITE, jacina));          // white core
    for (int k = 0; k < 14; k++) {   // sparks spiraling around the beam
        float s = std::fmod(k / 14.0f + t * 2.5f, 1.0f);
        float o = std::sin(s * 22 + t * 25 + k) * w * 0.9f;
        DrawPixel((int)(od.x + dx * s + nx * o), (int)(od.y + dy * s + ny * o), Fade(k % 2 ? Color{ 255, 226, 120, 255 } : WHITE, jacina));
    }
    // flash at both ends (four pointed star)
    DrawCircleV(od, 4 * jacina + 1, Fade(WHITE, jacina));
    DrawCircleV(cilj, 9 * jacina, Fade({ 255, 236, 150, 255 }, 0.35f * jacina));
    DrawCircleV(cilj, 4.5f * jacina, Fade(WHITE, jacina));
    float z = 14 * jacina;
    DrawLineEx({ cilj.x - z, cilj.y }, { cilj.x + z, cilj.y }, 1.5f, Fade(WHITE, jacina));
    DrawLineEx({ cilj.x, cilj.y - z }, { cilj.x, cilj.y + z }, 1.5f, Fade(WHITE, jacina));
    DrawLineEx({ cilj.x - z * 0.5f, cilj.y - z * 0.5f }, { cilj.x + z * 0.5f, cilj.y + z * 0.5f }, 1, Fade({ 255, 236, 150, 255 }, jacina * 0.7f));
    DrawLineEx({ cilj.x - z * 0.5f, cilj.y + z * 0.5f }, { cilj.x + z * 0.5f, cilj.y - z * 0.5f }, 1, Fade({ 255, 236, 150, 255 }, jacina * 0.7f));
}

void CrtajVitezaPregled(int oklop, float x, float y, float skala, bool zakljucan, float vrijeme) {
    if (oklop < 0 || oklop >= BROJ_IZGLEDA) oklop = 0;
    const Color silueta = { 14, 12, 26, 255 };
    rlDrawRenderBatchActive();
    rlPushMatrix();
    rlTranslatef(x, y, 0);
    rlScalef(skala, skala, 1);
    float mac = -45;   // kept still so the pixel sword stays straight
    VitezCrtez(oklop, 0, 0, 0, mac, 0, 0, 1, zakljucan ? &silueta : nullptr);
    rlDrawRenderBatchActive();
    rlPopMatrix();
}

// enemy size: a sprite 20 pixels tall is 1 (weapons, shadow and the health bar go off this)
float SkalaOrka(int vrsta) {
    int v = vrsta < 0 ? 0 : (vrsta >= BROJ_NEPRIJATELJA ? BROJ_NEPRIJATELJA - 1 : vrsta);
    return orci[v].h > 0 ? orci[v].h / 20.0f : 1.0f;
}

Napad NapadNeprijatelja(int v) {
    switch (v) {
        case 3: case 6: case 14: case 16: return Napad::UBOD;       // spear, trident, halberd, pitchfork
        case 2: case 8: case 13: case 17: return Napad::CAROLIJA;   // shaman, witch doctor, necromancer, fire demon
        case 9: case 12: return Napad::STRIJELA;                    // blowgunner, archer
        default: return Napad::SKOK;
    }
}

Color BojaCarolije(int v) {
    switch (v) {
        case 8: return { 120, 230, 90, 255 };    // poison green
        case 13: return { 90, 255, 170, 255 };   // necromancer green
        case 17: return { 255, 140, 40, 255 };   // fireball
        default: return { 170, 90, 255, 255 };   // purple
    }
}

bool Lebdi(int v) { return v == 16; }

// hand of an enemy facing left and standing straight (x, y = feet)
Vector2 RukaNeprijatelja(int vrsta, float x, float y) {
    int v = vrsta < 0 ? 0 : (vrsta >= BROJ_NEPRIJATELJA ? BROJ_NEPRIJATELJA - 1 : vrsta);
    const Sprite& s = orci[v];
    Vector2 r = { std::floor(x) + oblik[v].ruka.x + 0.5f - s.w / 2.0f, std::floor(y) + oblik[v].ruka.y + 0.5f - s.h };
    if (Lebdi(v)) r.y -= 4;
    return r;
}

Vector2 IzvorProjektila(int v, float x, float y) {
    float sk = SkalaOrka(v);
    Vector2 ruka = RukaNeprijatelja(v, x, y);
    float hx = ruka.x, hy = ruka.y;
    switch (v) {
        case 2: case 8: case 13: return { hx, hy - 16 * sk };   // tip of the staff held up high
        case 9: return { hx - 14 * sk, hy };                    // end of the blowgun
        case 12: return { hx - 3 * sk, hy };                    // bow
        default: return { hx, hy };                             // straight from the hand (fire demon)
    }
}

void CrtajProjektil(int v, Vector2 a, Vector2 c, float k) {
    if (NapadNeprijatelja(v) == Napad::STRIJELA) {   // arrows and darts fly straight
        Vector2 p = { a.x + (c.x - a.x) * k, a.y + (c.y - a.y) * k };
        float d = std::atan2(c.y - a.y, c.x - a.x) * RAD2DEG;
        bool strelica = v == 9;
        Segment(p.x, p.y, d, -7, 0, 1, strelica ? Color{ 150, 120, 70, 255 } : Color{ 130, 96, 60, 255 });
        Segment(p.x, p.y, d, 0, 2, 2, { 200, 204, 214, 255 });
        if (strelica) Segment(p.x, p.y, d, -8, -6, 2, { 120, 230, 90, 255 });      // poison tip
        else { Segment(p.x, p.y, d, -8, -5, 3, { 230, 230, 230, 255 }); }
        return;
    }
    Color b = BojaCarolije(v);
    for (int tr = 4; tr >= 0; tr--) {
        float kk = std::max(0.0f, k - tr * 0.06f);
        Vector2 p = { a.x + (c.x - a.x) * kk, a.y + (c.y - a.y) * kk - std::sin(PI * kk) * 14 };
        DrawCircleV(p, 4.0f - tr * 0.6f, Fade(b, 0.35f - tr * 0.06f));
        if (tr == 0) {
            DrawCircleV(p, 2.5f, { (unsigned char)std::min(255, b.r + 40), (unsigned char)std::min(255, b.g + 40), (unsigned char)std::min(255, b.b + 40), 255 });
            DrawCircleV(p, 1.2f, WHITE);
        }
    }
}

// like Segment but shifted sideways (for an axe blade sitting on one side of the handle)
static void SegmentPomak(float x, float y, float ugao, float a, float b, float debljina, float pomak, Color c) {
    DrawRectanglePro({ x, y, b - a, debljina }, { -a, debljina / 2 - pomak }, ugao, c);
}

void CrtajOrka(float x, float y, float ugaoOruzja, float bljesak, float alfa, bool premaDesno, int vrsta, float rotacija) {
    int v = vrsta < 0 ? 0 : (vrsta >= BROJ_NEPRIJATELJA ? BROJ_NEPRIJATELJA - 1 : vrsta);
    float sk = SkalaOrka(v);
    const Sprite& s = orci[v];
    const OblikNeprijatelja& ob = oblik[v];
    DrawEllipse((int)x, (int)y, (int)(s.w * 0.45f), 2, { 0, 0, 0, (unsigned char)(60 * alfa) });
    if (Lebdi(v)) y -= 4 + std::round(std::sin((float)GetTime() * 4) * 1.5f);   // the imp floats
    float smjer = premaDesno ? -1.0f : 1.0f;
    // breathing: the body drops a pixel on the exhale and the head follows a moment later
    // (no breathing while falling or flying at you)
    const float tDis = (float)GetTime() * (sk > 1.6f ? 1.6f : 2.2f) + v * 0.9f;
    const bool disanje = std::fabs(rotacija) < 2 && alfa > 0.5f;
    const float trupOff = disanje && std::sin(tDis) > 0.2f ? 1.0f : 0.0f;
    const float glavaOff = disanje && std::sin(tDis - 0.7f) > 0.2f ? 1.0f : 0.0f;
    // the hand from the sprite, relative to the feet and turned with the body when it staggers or falls
    float hx = (ob.ruka.x + 0.5f - s.w / 2.0f) * smjer, hy = ob.ruka.y + 0.5f - s.h + trupOff, r = rotacija * DEG2RAD;
    float mx = x + hx * std::cos(r) - hy * std::sin(r), my = y + hx * std::sin(r) + hy * std::cos(r);
    float ug = (premaDesno ? 180 - ugaoOruzja : ugaoOruzja) + rotacija;
    float p = premaDesno ? -1.0f : 1.0f;                    // which side the blade is on
    auto c = [&](Color k) { return Fade(k, alfa); };
    const Color drvo = { 120, 80, 44, 255 }, drvoTm = { 86, 56, 30, 255 };
    const Color zeljezo = { 170, 176, 190, 255 }, zeljezoTm = { 90, 94, 110, 255 }, obris = { 24, 20, 22, 255 };

    // long weapons go behind the body, the rest in front
    auto oruzje = [&]() {
        // bosses would carry giant weapons if they scaled fully, so they grow slower
        const float sk = SkalaOrka(v) > 1.4f ? 1.4f + (SkalaOrka(v) - 1.4f) * 0.4f : SkalaOrka(v);
        const Color koza = { 60, 36, 24, 255 }, svjetloDrvo = { 158, 112, 66, 255 }, sjaj = { 230, 236, 244, 255 };
        const Color crvena = { 190, 40, 40, 255 }, perje = { 230, 220, 190, 255 };
        const Color kost = { 230, 220, 190, 255 }, kostTm = { 170, 160, 130, 255 }, rda = { 150, 86, 50, 255 };
        auto tackaNa = [&](float s) { float u = ug * DEG2RAD; return Vector2{ mx + std::cos(u) * s * sk, my + std::sin(u) * s * sk }; };
        switch (v) {
        // swamp
        case 6:   // trident
            Segment(mx, my, ug, -8 * sk, 17 * sk, 1.5f * sk, c(drvo));
            Segment(mx, my, ug, 16 * sk, 17 * sk, 6 * sk, c(zeljezoTm));                       // crossbar
            Segment(mx, my, ug, 17 * sk, 22 * sk, 1 * sk, c(zeljezo));                         // middle prong
            SegmentPomak(mx, my, ug, 17 * sk, 20.5f * sk, 1 * sk, 2.5f * sk, c(zeljezo));
            SegmentPomak(mx, my, ug, 17 * sk, 20.5f * sk, 1 * sk, -2.5f * sk, c(zeljezo));
            Segment(mx, my, ug, 14 * sk, 15.5f * sk, 2.4f * sk, c(kost));                      // bone charm
            break;
        case 7:   // mossy log
            Segment(mx, my, ug, -1, 4 * sk, 2.5f * sk, c(drvoTm));
            Segment(mx, my, ug, 4 * sk, 13 * sk, 5 * sk, c(obris));
            Segment(mx, my, ug, 4.5f * sk, 12.5f * sk, 4 * sk, c(drvo));
            SegmentPomak(mx, my, ug, 5 * sk, 12 * sk, 1, 1.4f * sk * p, c(svjetloDrvo));
            SegmentPomak(mx, my, ug, 8 * sk, 12 * sk, 1.5f * sk, -1.5f * sk * p, c({ 86, 122, 54, 255 }));   // moss
            Segment(mx, my, ug, 12.5f * sk, 13 * sk, 3 * sk, c(svjetloDrvo));                  // cut end
            break;
        case 8: { // frog staff
            Segment(mx, my, ug, -6 * sk, 15 * sk, 2 * sk, c(drvoTm));
            Segment(mx, my, ug, 11 * sk, 12.5f * sk, 2.6f * sk, c(kost));
            Vector2 vrh = tackaNa(16);
            float puls = 0.5f + 0.5f * std::sin((float)GetTime() * 5);
            DrawCircleV(vrh, 4.5f * sk, Fade({ 120, 230, 90, 255 }, alfa * (0.25f + 0.2f * puls)));
            DrawCircleV(vrh, 2.5f * sk, c({ 80, 170, 60, 255 }));                               // the frog
            DrawRectangle((int)(vrh.x - 2 * sk), (int)(vrh.y - 2 * sk), (int)(1 * sk + 1), (int)(1 * sk + 1), c({ 255, 230, 90, 255 }));
            DrawRectangle((int)(vrh.x + 1 * sk), (int)(vrh.y - 2 * sk), (int)(1 * sk + 1), (int)(1 * sk + 1), c({ 255, 230, 90, 255 }));
            break;
        }
        case 9:   // reed blowgun
            Segment(mx, my, ug, -4 * sk, 14 * sk, 1.5f * sk, c({ 150, 140, 80, 255 }));
            for (int k = 0; k < 3; k++) Segment(mx, my, ug, (1 + k * 4) * sk, (1.6f + k * 4) * sk, 2 * sk, c({ 96, 84, 44, 255 }));
            break;
        case 10:  // bone cleaver
            Segment(mx, my, ug, -3 * sk, 14 * sk, 2 * sk, c(drvoTm));
            for (int k = 0; k < 3; k++) Segment(mx, my, ug, (-2 + k * 1.5f) * sk, (-1.2f + k * 1.5f) * sk, 2.4f * sk, c(koza));
            SegmentPomak(mx, my, ug, 7 * sk, 15 * sk, 7 * sk, 3.5f * sk * p, c(obris));
            SegmentPomak(mx, my, ug, 7.5f * sk, 14.5f * sk, 6 * sk, 3.5f * sk * p, c(kost));
            SegmentPomak(mx, my, ug, 7.5f * sk, 14.5f * sk, 1.5f * sk, 6 * sk * p, c(kostTm));
            for (int k = 0; k < 3; k++) SegmentPomak(mx, my, ug, (9 + k * 2) * sk, (9.6f + k * 2) * sk, 1 * sk, 6.8f * sk * p, c(obris));   // teeth
            break;
        // dungeon
        case 11:  // rusty sword
            Segment(mx, my, ug, -2.5f * sk, 1 * sk, 1.5f * sk, c(koza));
            Segment(mx, my, ug, 1 * sk, 2 * sk, 5 * sk, c(rda));
            Segment(mx, my, ug, 2 * sk, 15 * sk, 2.5f * sk, c(obris));
            Segment(mx, my, ug, 2 * sk, 14.5f * sk, 1.5f * sk, c({ 170, 160, 150, 255 }));
            SegmentPomak(mx, my, ug, 6 * sk, 9 * sk, 1, 0.5f * sk, c(rda));
            SegmentPomak(mx, my, ug, 11 * sk, 12.5f * sk, 1, -0.5f * sk, c(rda));
            break;
        case 12: { // bow
            float bx = mx - 1.5f * sk * smjer, by = my;
            float a0 = premaDesno ? -60 : 120, a1 = premaDesno ? 60 : 240;
            DrawRing({ bx + 4 * sk * smjer, by }, 6.5f * sk, 7.5f * sk, a0, a1, 16, c(drvo));
            DrawLine((int)(bx + 4 * sk * smjer + std::cos(a0 * DEG2RAD) * 7 * sk), (int)(by + std::sin(a0 * DEG2RAD) * 7 * sk),
                     (int)(bx + 4 * sk * smjer + std::cos(a1 * DEG2RAD) * 7 * sk), (int)(by + std::sin(a1 * DEG2RAD) * 7 * sk), c({ 220, 220, 210, 255 }));
            break;
        }
        case 13: { // skull staff
            Segment(mx, my, ug, -6 * sk, 14 * sk, 1.6f * sk, c({ 70, 60, 54, 255 }));
            Vector2 vrh = tackaNa(16);
            float puls = 0.5f + 0.5f * std::sin((float)GetTime() * 4);
            DrawCircleV(vrh, 5 * sk, Fade({ 90, 255, 170, 255 }, alfa * (0.2f + 0.2f * puls)));
            DrawRectangle((int)(vrh.x - 2 * sk), (int)(vrh.y - 2 * sk), (int)(4 * sk), (int)(4 * sk), c(kost));
            DrawRectangle((int)(vrh.x - 1.5f * sk), (int)(vrh.y - 0.5f * sk), (int)(1 * sk), (int)(1 * sk), c({ 90, 255, 170, 255 }));
            DrawRectangle((int)(vrh.x + 0.5f * sk), (int)(vrh.y - 0.5f * sk), (int)(1 * sk), (int)(1 * sk), c({ 90, 255, 170, 255 }));
            break;
        }
        case 14:  // halberd
            Segment(mx, my, ug, -8 * sk, 18 * sk, 1.5f * sk, c({ 90, 64, 44, 255 }));
            SegmentPomak(mx, my, ug, 13 * sk, 17 * sk, 4 * sk, 2.5f * sk * p, c(zeljezoTm));   // blade
            SegmentPomak(mx, my, ug, 13.5f * sk, 16.5f * sk, 1, 4.2f * sk * p, c(zeljezo));
            SegmentPomak(mx, my, ug, 14 * sk, 15.5f * sk, 2 * sk, -2 * sk * p, c(zeljezoTm));  // hook
            Segment(mx, my, ug, 18 * sk, 22 * sk, 1.5f * sk, c(zeljezo));                       // spike
            break;
        case 15: { // lich king's scythe, long handle and a curved blade up top
            Segment(mx, my, ug, -6 * sk, 17 * sk, 1.6f * sk, c({ 60, 44, 70, 255 }));
            Vector2 vrh = tackaNa(17);
            float n = premaDesno ? -1.0f : 1.0f;
            // the blade bends back towards the handle in three parts, outline first then the edge
            float smjer1 = ug + 90 * n, smjer2 = ug + 125 * n, smjer3 = ug + 160 * n;
            Vector2 t1 = { vrh.x + std::cos(smjer1 * DEG2RAD) * 5 * sk, vrh.y + std::sin(smjer1 * DEG2RAD) * 5 * sk };
            Vector2 t2 = { t1.x + std::cos(smjer2 * DEG2RAD) * 4 * sk, t1.y + std::sin(smjer2 * DEG2RAD) * 4 * sk };
            Segment(vrh.x, vrh.y, smjer1, 0, 5.5f * sk, 3.6f * sk, c(obris));
            Segment(t1.x, t1.y, smjer2, 0, 4.5f * sk, 3.2f * sk, c(obris));
            Segment(t2.x, t2.y, smjer3, 0, 3.5f * sk, 2.6f * sk, c(obris));
            Segment(vrh.x, vrh.y, smjer1, 0, 5 * sk, 2.4f * sk, c({ 210, 218, 236, 255 }));
            Segment(t1.x, t1.y, smjer2, 0, 4 * sk, 2 * sk, c({ 210, 218, 236, 255 }));
            Segment(t2.x, t2.y, smjer3, 0, 3 * sk, 1.4f * sk, c({ 240, 246, 255, 255 }));
            DrawCircleV(vrh, 1.5f * sk, c({ 120, 255, 140, 255 }));   // green stone at the top
            break;
        }
        // volcano
        case 16:  // pitchfork
            Segment(mx, my, ug, -6 * sk, 13 * sk, 1.2f * sk, c({ 70, 40, 30, 255 }));
            Segment(mx, my, ug, 12.5f * sk, 13.5f * sk, 5 * sk, c(zeljezoTm));
            for (int k = -1; k <= 1; k++) SegmentPomak(mx, my, ug, 13 * sk, 16.5f * sk, 1 * sk, 2 * k * sk, c(zeljezo));
            break;
        case 17: { // fire in the fire demon's hand
            float tr = 0.5f + 0.5f * std::sin((float)GetTime() * 9);
            DrawCircleV({ mx, my - 1 }, (2.5f + tr) * sk, c({ 255, 140, 40, 160 }));
            DrawCircleV({ mx, my - 1 }, 1.3f * sk, c({ 255, 230, 120, 255 }));
            break;
        }
        case 18:  // the hellhound has no weapon
            break;
        case 19:  // the executioner's huge cleaver
            Segment(mx, my, ug, -2 * sk, 6 * sk, 2 * sk, c(koza));
            SegmentPomak(mx, my, ug, 6 * sk, 16 * sk, 8 * sk, 3 * sk * p, c(obris));
            SegmentPomak(mx, my, ug, 6.5f * sk, 15.5f * sk, 7 * sk, 3 * sk * p, c({ 110, 106, 116, 255 }));
            SegmentPomak(mx, my, ug, 6.5f * sk, 15.5f * sk, 1.2f * sk, 6.3f * sk * p, c(sjaj));
            SegmentPomak(mx, my, ug, 12 * sk, 14 * sk, 2 * sk, 2 * sk * p, c(obris));         // hole in the cleaver
            SegmentPomak(mx, my, ug, 8 * sk, 12 * sk, 1, 1 * sk * p, c({ 140, 30, 30, 255 }));
            break;
        case 20: { // demon king's flaming sword
            Segment(mx, my, ug, -3 * sk, 1 * sk, 1.6f * sk, c(koza));
            Segment(mx, my, ug, 1 * sk, 2 * sk, 6 * sk, c({ 240, 190, 60, 255 }));
            Segment(mx, my, ug, 2 * sk, 17 * sk, 3 * sk, c(obris));
            Segment(mx, my, ug, 2 * sk, 16.5f * sk, 2 * sk, c({ 255, 120, 40, 255 }));
            Segment(mx, my, ug, 2.5f * sk, 15.5f * sk, 0.8f * sk, c({ 255, 230, 140, 255 }));
            float t = (float)GetTime();
            for (int k = 0; k < 6; k++) {   // flames along the blade
                Vector2 q = tackaNa(4 + k * 2.2f);
                float f = std::fmod(t * 3 + k * 0.37f, 1.0f);
                DrawRectangle((int)(q.x + std::sin(t * 7 + k) * sk), (int)(q.y - f * 5 * sk), (int)(1 * sk + 1), (int)(1 * sk + 1),
                              Fade(f < 0.5f ? Color{ 255, 220, 90, 255 } : Color{ 255, 110, 30, 255 }, alfa * (1 - f)));
            }
            break;
        }
        // valley (orcs)
        case 0:   // spiked club
            Segment(mx, my, ug, -1, 5 * sk, 2 * sk, c(drvo));
            Segment(mx, my, ug, -1, 1.5f * sk, 2.4f * sk, c(koza));                 // leather grip
            Segment(mx, my, ug, 5 * sk, 11 * sk, 4 * sk, c(drvoTm));
            Segment(mx, my, ug, 6 * sk, 10 * sk, 5 * sk, c(drvoTm));
            SegmentPomak(mx, my, ug, 6 * sk, 10.5f * sk, 1, 1.2f * sk * p, c(svjetloDrvo));  // wood shine
            Segment(mx, my, ug, 5.5f * sk, 6.3f * sk, 5.4f * sk, c(zeljezoTm));     // iron ring
            Segment(mx, my, ug, 7 * sk, 8 * sk, 7 * sk, c(zeljezo));
            Segment(mx, my, ug, 9 * sk, 10 * sk, 7 * sk, c(zeljezo));
            Segment(mx, my, ug, 11 * sk, 12 * sk, 1.5f * sk, c(zeljezo));           // spike on top
            break;
        case 1:   // axe
            Segment(mx, my, ug, -2, 13 * sk, 2 * sk, c(drvo));
            for (int k = 0; k < 3; k++) Segment(mx, my, ug, (-0.5f + k * 1.5f) * sk, (0.3f + k * 1.5f) * sk, 2.4f * sk, c(koza));
            SegmentPomak(mx, my, ug, 8 * sk, 13 * sk, 6 * sk, 3 * sk * p, c(obris));
            SegmentPomak(mx, my, ug, 8.5f * sk, 12.5f * sk, 5 * sk, 3 * sk * p, c(zeljezo));
            SegmentPomak(mx, my, ug, 8.5f * sk, 12.5f * sk, 1.5f * sk, 1.2f * sk * p, c(zeljezoTm));
            SegmentPomak(mx, my, ug, 8.5f * sk, 12.5f * sk, 1 * sk, 5.5f * sk * p, c(sjaj));
            SegmentPomak(mx, my, ug, 9.5f * sk, 11.5f * sk, 2.5f * sk, -2 * sk * p, c(zeljezoTm));  // back spike
            Segment(mx, my, ug, 13 * sk, 14 * sk, 1.5f * sk, c(zeljezo));
            break;
        case 2: { // shaman staff with cloth, feathers and a glowing orb
            Segment(mx, my, ug, -6 * sk, 15 * sk, 2 * sk, c(drvoTm));
            Segment(mx, my, ug, 12 * sk, 14.5f * sk, 2.6f * sk, c({ 104, 64, 140, 255 }));   // cloth
            float r = ug * DEG2RAD;
            Vector2 smjerP = { std::cos(r), std::sin(r) };
            for (int k = 0; k < 2; k++) {   // feathers hanging off the staff
                Vector2 o = { mx + smjerP.x * (12.5f + k) * sk, my + smjerP.y * (12.5f + k) * sk };
                float njih = std::sin((float)GetTime() * 3 + k) * 0.8f * sk;
                DrawRectangle((int)(o.x - 0.5f * sk + njih), (int)o.y, (int)(1 * sk), (int)(4 * sk), c(k ? crvena : perje));
            }
            Vector2 vrh = { mx + smjerP.x * 16 * sk, my + smjerP.y * 16 * sk };
            float puls = 0.5f + 0.5f * std::sin((float)GetTime() * 5);
            DrawCircleV(vrh, 4.5f * sk, Fade({ 170, 90, 255, 255 }, alfa * (0.25f + 0.2f * puls)));
            DrawCircleV(vrh, 2.5f * sk, c({ 200, 140, 255, 255 }));
            DrawCircleV(vrh, 1.2f * sk, c({ 250, 230, 255, 255 }));
            DrawCircleV({ vrh.x - 0.8f * sk, vrh.y - 0.8f * sk }, 0.6f * sk, c(WHITE));
            break;
        }
        case 3: { // spear with a red tassel
            Segment(mx, my, ug, -8 * sk, 18 * sk, 1.5f * sk, c(drvo));
            Segment(mx, my, ug, -1, 1.5f * sk, 2 * sk, c(koza));
            Segment(mx, my, ug, 16 * sk, 17.2f * sk, 2.2f * sk, c(koza));            // binding
            Segment(mx, my, ug, 17 * sk, 20 * sk, 4 * sk, c(zeljezoTm));
            Segment(mx, my, ug, 20 * sk, 23 * sk, 2.5f * sk, c(zeljezo));
            Segment(mx, my, ug, 23 * sk, 24.5f * sk, 1 * sk, c(zeljezo));
            SegmentPomak(mx, my, ug, 18 * sk, 23 * sk, 1, 0.6f * sk, c(sjaj));
            float r = ug * DEG2RAD;
            Vector2 o = { mx + std::cos(r) * 16.5f * sk, my + std::sin(r) * 16.5f * sk };
            float njih = std::sin((float)GetTime() * 4) * 0.8f * sk;
            DrawRectangle((int)(o.x - sk + njih), (int)o.y, (int)(2 * sk), (int)(3 * sk), c(crvena));   // tassel
            break;
        }
        case 4:   // war hammer
            Segment(mx, my, ug, -2, 12 * sk, 2 * sk, c(drvo));
            for (int k = 0; k < 3; k++) Segment(mx, my, ug, (-0.5f + k * 1.5f) * sk, (0.3f + k * 1.5f) * sk, 2.4f * sk, c(koza));
            SegmentPomak(mx, my, ug, 10.5f * sk, 12.5f * sk, 2 * sk, -5 * sk * p, c(zeljezoTm));  // spike
            Segment(mx, my, ug, 9 * sk, 14 * sk, 8 * sk, c(obris));
            Segment(mx, my, ug, 9.5f * sk, 13.5f * sk, 7 * sk, c(zeljezoTm));
            Segment(mx, my, ug, 10 * sk, 13 * sk, 5 * sk, c(zeljezo));
            Segment(mx, my, ug, 9.5f * sk, 10.2f * sk, 7 * sk, c(obris));            // bands
            Segment(mx, my, ug, 12.8f * sk, 13.5f * sk, 7 * sk, c(obris));
            SegmentPomak(mx, my, ug, 10.5f * sk, 12.5f * sk, 1, 2 * sk * p, c(sjaj));
            break;
        default:  // warlord: big double axe with a red ribbon
            Segment(mx, my, ug, -5 * sk, 16 * sk, 2 * sk, c(drvoTm));
            for (int k = 0; k < 4; k++) Segment(mx, my, ug, (-2 + k * 1.5f) * sk, (-1.2f + k * 1.5f) * sk, 2.4f * sk, c(koza));
            Segment(mx, my, ug, 10 * sk, 16 * sk, 11 * sk, c(obris));
            Segment(mx, my, ug, 10.5f * sk, 15.5f * sk, 10 * sk, c({ 120, 124, 140, 255 }));
            Segment(mx, my, ug, 10.5f * sk, 15.5f * sk, 2 * sk, c(obris));
            SegmentPomak(mx, my, ug, 11 * sk, 15 * sk, 1, 4.5f * sk, c(sjaj));
            SegmentPomak(mx, my, ug, 11 * sk, 15 * sk, 1, -4.5f * sk, c(sjaj));
            SegmentPomak(mx, my, ug, 12 * sk, 14 * sk, 1.2f * sk, 2.5f * sk, c({ 70, 72, 86, 255 }));   // notches
            SegmentPomak(mx, my, ug, 12 * sk, 14 * sk, 1.2f * sk, -2.5f * sk, c({ 70, 72, 86, 255 }));
            Segment(mx, my, ug, 16 * sk, 19 * sk, 1.5f * sk, c(zeljezo));
            {
                float r = ug * DEG2RAD;
                Vector2 o = { mx + std::cos(r) * 9 * sk, my + std::sin(r) * 9 * sk };
                float njih = std::sin((float)GetTime() * 3) * 1.0f * sk;
                DrawRectangle((int)(o.x - sk + njih), (int)o.y, (int)(2 * sk), (int)(5 * sk), c(crvena));   // red ribbon
            }
            break;
        }
    };
    // the little things that make them feel alive: blinking, glowing eyes, particles and auras
    const float t = (float)GetTime();
    const bool ziv = alfa > 0.3f && std::fabs(rotacija) < 30;
    // how far a sprite row got moved by breathing (head, body or legs)
    auto pomakReda = [&](float red) { return red < ob.vrat ? glavaOff : (red < ob.pojas ? trupOff : 0.0f); };
    // sprite pixel (px, py) to a point in the world, with flipping and the body's rotation
    auto uSvijet = [&](float px, float py) {
        float lx = (px + 0.5f - s.w / 2.0f) * (premaDesno ? -1.0f : 1.0f), ly = py + 0.5f - s.h;
        return Vector2{ std::floor(x) + lx * std::cos(r) - ly * std::sin(r), std::floor(y) + lx * std::sin(r) + ly * std::cos(r) };
    };
    auto piksel = [&](Vector2 p, float vel, Color b, float a) {
        DrawRectangleV({ std::floor(p.x), std::floor(p.y) }, { vel, vel }, Fade(b, std::max(0.0f, std::min(1.0f, a)) * alfa));
    };
    // particles rising around the body (embers, magic, steam)
    auto uzlazne = [&](int n, Color b, float sirina, float visina, float brzina, float odY = 2) {
        for (int k = 0; k < n; k++) {
            unsigned int hk = Hash(v * 97 + k * 13 + 5);
            float f = std::fmod(t * brzina + (float)k / n + (hk % 100) / 100.0f, 1.0f);
            Vector2 p = { x + ((hk % 1000) / 1000.0f - 0.5f) * sirina * sk + std::sin(t * 2 + k) * 1.5f, y - odY * sk - f * visina * sk };
            piksel(p, 1, b, std::sin(f * PI));
        }
    };
    // drops falling off the body (water, mud, bone dust)
    auto kapi = [&](int n, Color b, float brzina) {
        for (int k = 0; k < n; k++) {
            unsigned int hk = Hash(v * 53 + k * 29 + 1);
            float f = std::fmod(t * brzina + (hk % 100) / 100.0f, 1.0f);
            float od = (5 + hk % 9) * sk;
            Vector2 p = { x + ((float)(hk / 100 % 13) - 6.5f) * sk * (premaDesno ? -1.0f : 1.0f), y - od + f * f * od };
            piksel(p, 1, b, f < 0.85f ? 1.0f : (1 - f) / 0.15f);
        }
    };
    // flies buzzing around the head
    auto muhe = [&](int n) {
        Vector2 g = uSvijet(s.w / 2.0f, 2 + glavaOff);
        for (int k = 0; k < n; k++) {
            Vector2 p = { g.x + std::sin(t * (5 + k) + k * 2) * 7 * sk + std::sin(t * 17 + k) * 1.2f, g.y - 3 * sk + std::cos(t * (4 + k * 0.7f) + k * 3) * 4 * sk };
            piksel(p, 1, { 20, 20, 24, 255 }, 1);
        }
    };
    // souls or green sparks circling the body, with a little tail
    auto duse = [&](int n, Color b, float rx, float ry) {
        for (int k = 0; k < n; k++)
            for (int tr = 4; tr >= 0; tr--) {
                float u = t * 1.8f + k * 2 * PI / n - tr * 0.12f;
                Vector2 p = { x + std::cos(u) * rx * sk, y - 12 * sk + std::sin(u) * ry * sk + std::sin(t * 3 + k) * 1.5f };
                if (tr == 0) DrawCircleV(p, 2.2f, Fade(b, 0.3f * alfa));
                piksel(p, tr == 0 ? 2.0f : 1.0f, tr == 0 ? Color{ 230, 255, 240, 255 } : b, 0.9f - tr * 0.18f);
            }
    };
    // flames licking up from the glowing pixels (fire demon's head, hellhound's mane)
    auto plamen = [&](int korak, float visina) {
        for (size_t k = 0; k < zar[v].size(); k += korak) {
            float f = std::fmod(t * 1.6f + Hash((int)k * 7 + v) % 100 / 100.0f, 1.0f);
            Vector2 p = uSvijet(zar[v][k].x, zar[v][k].y + pomakReda(zar[v][k].y));
            p.y -= f * visina * sk; p.x += std::sin(t * 9 + k) * 0.8f;
            piksel(p, 1, f < 0.4f ? Color{ 255, 230, 120, 255 } : Color{ 255, 110, 30, 255 }, 1 - f);
        }
    };
    auto aura = [&](Color b, float rx, float jacina) {   // colored mist or glow under the feet
        float p = 0.75f + 0.25f * std::sin(t * 2.5f);
        DrawEllipse((int)x, (int)y, (int)(rx * sk), (int)(3 * sk), Fade(b, jacina * p * alfa));
        DrawEllipse((int)x, (int)y, (int)(rx * 0.6f * sk), (int)(2 * sk), Fade(b, jacina * 1.3f * p * alfa));
    };
    Color bojaOka = (v == 13 || v == 15) ? Color{ 120, 255, 140, 255 } : v == 14 ? Color{ 255, 140, 40, 255 }
                  : v < 16 ? Color{ 255, 70, 60, 255 } : Color{ 255, 210, 80, 255 };
    auto prijeTijela = [&]() {
        if (alfa < 0.05f) return;
        switch (v) {
            case 5: aura({ 200, 40, 40, 255 }, 14, 0.18f); break;
            case 10: aura({ 90, 140, 60, 255 }, 14, 0.25f); break;
            case 13: aura({ 90, 255, 160, 255 }, 11, 0.22f); break;
            case 15: aura({ 130, 60, 200, 255 }, 15, 0.3f); duse(2, { 120, 255, 160, 255 }, 13, 5); break;
            case 17: aura({ 255, 120, 30, 255 }, 10, 0.3f); break;
            case 20: aura({ 255, 70, 20, 255 }, 16, 0.35f); break;
            default: break;
        }
    };
    auto posljeTijela = [&]() {
        if (alfa < 0.05f) return;
        // living things blink now and then, skeletons and demons have glowing eyes instead
        if (v < 11 || v == 18) {
            float period = 3.2f + (Hash(v * 7 + 3) % 3) * 0.7f;
            if (ziv && std::fmod(t + v * 0.77f, period) < 0.12f)
                for (auto& e : oci[v]) piksel({ uSvijet(e.x, e.y + pomakReda(e.y)).x - 0.5f, uSvijet(e.x, e.y + pomakReda(e.y)).y - 0.5f }, 1, obris, 1);
        }
        if (v >= 11) {
            float puls = 0.6f + 0.4f * std::sin(t * 3 + v);
            for (auto& e : oci[v]) DrawCircleV(uSvijet(e.x, e.y + pomakReda(e.y)), 1.5f + 0.3f * sk, Fade(bojaOka, 0.16f * puls * alfa));
        }
        switch (v) {
            case 1: uzlazne(3, { 255, 200, 190, 255 }, 6, 8, 0.9f, 18); break;                    // steaming with rage
            case 2: uzlazne(5, { 200, 140, 255, 255 }, 16, 22, 0.35f); break;                     // shaman sparks
            case 5: uzlazne(5, { 255, 90, 60, 255 }, 18, 24, 0.3f); break;
            case 6: kapi(3, { 140, 190, 210, 255 }, 0.7f); break;                                 // water dripping from the dreads
            case 7: kapi(4, { 96, 66, 40, 255 }, 0.55f); muhe(3); break;                          // mud and flies
            case 8: {                                                                             // poison bubbles from the staff
                float u = ug * DEG2RAD;
                Vector2 vrh = { mx + std::cos(u) * 16 * sk, my + std::sin(u) * 16 * sk };
                for (int k = 0; k < 4; k++) {
                    float f = std::fmod(t * 0.7f + k * 0.25f, 1.0f);
                    Vector2 p = { vrh.x + std::sin(t * 4 + k * 2) * 2, vrh.y - 3 - f * 14 };
                    DrawCircleLinesV(p, 0.8f + f * 1.2f, Fade({ 150, 240, 110, 255 }, (1 - f) * alfa));
                }
                uzlazne(3, { 150, 240, 110, 255 }, 14, 18, 0.3f);
                break;
            }
            case 9:                                                                               // falling leaves
                for (int k = 0; k < 2; k++) {
                    float f = std::fmod(t * 0.35f + k * 0.5f, 1.0f);
                    Vector2 p = { x + (k ? 5.0f : -6.0f) + std::sin(f * 9 + k) * 3, y - 20 + f * 20 };
                    piksel(p, 2, k ? Color{ 86, 122, 54, 255 } : Color{ 120, 150, 60, 255 }, 1 - f * f);
                }
                break;
            case 10: kapi(4, { 96, 66, 40, 255 }, 0.5f); muhe(4);                                 // mud, flies and swamp gas
                for (int k = 0; k < 3; k++) {
                    float f = std::fmod(t * 0.5f + k * 0.33f, 1.0f);
                    DrawCircleLinesV({ x - 10 * sk + k * 10 * sk, y - 1 - f * 8 }, 0.5f + f * 1.5f, Fade({ 150, 200, 90, 255 }, (1 - f) * 0.8f * alfa));
                }
                break;
            case 11: case 12: case 14: kapi(2, { 200, 196, 180, 255 }, 0.3f); break;             // bone dust
            case 13: duse(3, { 120, 255, 160, 255 }, 10, 4); uzlazne(4, { 120, 255, 160, 255 }, 16, 20, 0.3f); break;
            case 15: duse(3, { 120, 255, 160, 255 }, 14, 6); uzlazne(6, { 190, 120, 255, 255 }, 22, 30, 0.25f); break;
            case 16: uzlazne(4, { 255, 160, 40, 255 }, 10, 16, 0.6f, 4); break;                   // sparks
            case 17: {                                                                            // magma pulsing, head on fire
                float puls = 0.5f + 0.5f * std::sin(t * 4);
                for (auto& z : zar[v]) piksel(uSvijet(z.x, z.y + pomakReda(z.y)), 1, { 255, 250, 200, 255 }, 0.35f * puls);
                plamen(1, 7);
                uzlazne(5, { 255, 140, 40, 255 }, 14, 24, 0.5f);
                break;
            }
            case 18: {                                                                            // mane on fire, smoke from the nose
                plamen(1, 5);
                Vector2 nos = uSvijet(ob.ruka.x, ob.ruka.y - 3 + trupOff);
                for (int k = 0; k < 3; k++) {
                    float f = std::fmod(t * 0.9f + k * 0.33f, 1.0f);
                    DrawCircleV({ nos.x - f * 5 * (premaDesno ? -1.0f : 1.0f), nos.y - f * 6 }, 0.6f + f * 1.6f, Fade({ 90, 80, 80, 255 }, (1 - f) * 0.6f * alfa));
                }
                break;
            }
            case 19: {                                                                            // smoke from the mouth, embers
                Vector2 usta = oci[v].empty() ? uSvijet(5, 8) : uSvijet(oci[v][0].x + 1, oci[v][0].y + 3 + glavaOff);
                for (int k = 0; k < 3; k++) {
                    float f = std::fmod(t * 0.6f + k * 0.33f, 1.0f);
                    DrawCircleV({ usta.x - f * 4 * (premaDesno ? -1.0f : 1.0f), usta.y - f * 8 }, 0.6f + f * 2, Fade({ 80, 70, 70, 255 }, (1 - f) * 0.5f * alfa));
                }
                uzlazne(4, { 255, 120, 40, 255 }, 16, 22, 0.4f);
                break;
            }
            case 20: {                                                                            // flames at his feet, embers, crown glint
                for (int k = 0; k < 9; k++) {
                    float f = std::fmod(t * 1.8f + Hash(k * 11) % 100 / 100.0f, 1.0f);
                    float px = x + (k - 4) * 3.2f * sk + std::sin(t * 7 + k) * 1.0f;
                    float vis = (3 + Hash(k * 5) % 4) * sk * (1 - f);
                    DrawRectangleV({ px, y - vis - 1 }, { 1.5f, vis }, Fade(f < 0.5f ? Color{ 255, 200, 80, 255 } : Color{ 255, 90, 30, 255 }, (1 - f) * 0.8f * alfa));
                }
                uzlazne(10, { 255, 140, 40, 255 }, 30, 40, 0.35f);
                Vector2 kruna = uSvijet(s.w * 0.4f, 1 + glavaOff);
                float sj = std::fmod(t * 0.5f, 1.0f);
                if (sj < 0.15f) {                                                                 // glint on the crown
                    float a = std::sin(sj / 0.15f * PI);
                    DrawLineV({ kruna.x - 3, kruna.y }, { kruna.x + 3, kruna.y }, Fade(WHITE, a * alfa));
                    DrawLineV({ kruna.x, kruna.y - 3 }, { kruna.x, kruna.y + 3 }, Fade(WHITE, a * alfa));
                }
                break;
            }
            default: break;
        }
    };

    // staffs and spears are held behind the body, everything else in front
    bool iza = (v == 2 || v == 3 || v == 6 || v == 8 || v == 13 || v == 14 || v == 15 || v == 16);
    prijeTijela();
    // bat wings (big and slow for the demon king, small and fast for the imp): arm bone, fingers and
    // skin between them with a jagged bottom edge. Skipped while falling.
    if ((v == 20 || v == 16) && std::fabs(rotacija) < 1) {
        bool kralj = v == 20;
        float mah = std::sin(t * (kralj ? 2.6f : 11.0f)) * (kralj ? 5.0f : 3.0f);
        float raspon = kralj ? 20 : 9, rame = kralj ? 0.66f : 0.58f;
        Color opna = c(kralj ? Color{ 92, 24, 34, 255 } : Color{ 120, 30, 36, 255 });
        Color opnaTm = c(kralj ? Color{ 58, 14, 24, 255 } : Color{ 80, 18, 24, 255 });
        Color kost = c(kralj ? Color{ 30, 16, 20, 255 } : Color{ 40, 20, 22, 255 });
        for (int strana = 1; strana >= -1; strana -= 2) {   // far wing first
            float korX = std::floor(x) + strana * 2, korY = std::floor(y) - s.h * rame + trupOff;
            Vector2 lakat = { korX + strana * raspon * 0.45f, korY - raspon * 0.55f - mah };
            Vector2 prsti[4];
            for (int k = 0; k < 4; k++) {
                float u = (-70 + k * 32) * DEG2RAD;   // fingers spread from top to bottom
                float d = raspon * (0.75f - k * 0.08f);
                prsti[k] = { lakat.x + strana * std::cos(u) * d, lakat.y + std::sin(u) * d + mah * (0.6f - k * 0.15f) + raspon * 0.25f };
            }
            Vector2 dno = { korX + strana * raspon * 0.2f, korY + raspon * 0.35f };
            Color o = strana > 0 ? opnaTm : opna;
            for (int k = 0; k < 3; k++) {   // skin between fingers (both winding orders so it always shows)
                Vector2 sred = { (prsti[k].x + prsti[k + 1].x) / 2, (prsti[k].y + prsti[k + 1].y) / 2 - raspon * 0.08f };
                DrawTriangle(lakat, prsti[k], sred, o); DrawTriangle(lakat, sred, prsti[k], o);
                DrawTriangle(lakat, sred, prsti[k + 1], o); DrawTriangle(lakat, prsti[k + 1], sred, o);
            }
            DrawTriangle({ korX, korY }, lakat, prsti[3], o); DrawTriangle({ korX, korY }, prsti[3], lakat, o);
            DrawTriangle({ korX, korY }, prsti[3], dno, o); DrawTriangle({ korX, korY }, dno, prsti[3], o);
            DrawLineEx({ korX, korY }, lakat, kralj ? 2.0f : 1.2f, kost);
            for (int k = 0; k < 4; k++) DrawLineEx(lakat, prsti[k], kralj ? 1.0f : 0.8f, kost);
            DrawCircleV(lakat, kralj ? 1.4f : 0.8f, c({ 200, 180, 150, 255 }));   // claw on the joint
        }
    }
    if (iza) oruzje();
    // the sprite is drawn in pieces: legs stay put, the robe hem sways, body and head breathe
    auto crtajDio = [&](int odR, int doR, float dx, float dy) {
        odR = std::max(0, odR); doR = std::min(s.h, doR);
        if (doR <= odR) return;
        float sw = (float)s.w, vis = (float)(doR - odR);
        Rectangle src = { 0, (float)odR, premaDesno ? -sw : sw, vis };
        float ox = dx * std::cos(r) - dy * std::sin(r), oy = dx * std::sin(r) + dy * std::cos(r);
        Rectangle dst = { std::floor(x) + ox, std::floor(y) + oy, sw, vis };
        Vector2 org = { sw / 2.0f, (float)(s.h - odR) };
        DrawTexturePro(s.tex, src, dst, org, rotacija, Fade(WHITE, alfa));
        if (bljesak > 0) DrawTexturePro(s.bijela, src, dst, org, rotacija, Fade(WHITE, bljesak * alfa));
    };
    float njihanje = 0;
    if (ob.porub > 0 && disanje) { float n = std::sin(t * 2.4f + v); njihanje = n > 0.45f ? 1.0f : (n < -0.45f ? -1.0f : 0.0f); }
    int krajNogu = ob.porub > 0 ? ob.porub : s.h;
    crtajDio(ob.pojas, krajNogu, 0, 0);
    if (ob.porub > 0) crtajDio(ob.porub, s.h, njihanje, 0);
    crtajDio(ob.vrat - 1, ob.pojas, 0, trupOff);   // one row of overlap so no gap opens under the head
    crtajDio(0, ob.vrat, 0, glavaOff);
    if (!iza) oruzje();
    posljeTijela();
}

void CrtajZamahOrka(float x, float y, float od, float doUgla, float radijus, float alfa) {
    if (alfa <= 0) return;
    float a0 = std::fmin(od, doUgla), a1 = std::fmax(od, doUgla);
    DrawRing({ x, y }, radijus * 0.7f, radijus, a0, a1, 20, Fade({ 255, 230, 200, 255 }, alfa * 0.7f));
    DrawRing({ x, y }, radijus * 0.85f, radijus * 0.97f, a0, a1, 20, Fade({ 255, 255, 255, 255 }, alfa));
}

void CrtajLutku(float x, float y, float nagib, float bljesak) {
    DrawEllipse((int)x, (int)y, 9, 2, { 0, 0, 0, 60 });
    CrtajSprite(lutka, x, y + 1, bljesak, nagib, 1.0f);
}

void CrtajZamah(float x, float y, float od, float doUgla, float alfa) {
    if (alfa <= 0) return;
    float a0 = std::fmin(od, doUgla), a1 = std::fmax(od, doUgla);
    DrawRing({ x, y }, 18, 26, a0, a1, 24, Fade({ 255, 255, 255, 255 }, alfa * 0.8f));
    DrawRing({ x, y }, 22, 25, a0, a1, 24, Fade({ 255, 246, 190, 255 }, alfa));
}

void CrtajKartu(float cx, float y, float skala, float sirinaFaktor, bool lice, Color nijansa, bool zensko) {
    Texture2D t = !lice ? kartaPoledjina : (zensko ? kartaLiceZ : kartaLice);
    float w = t.width * skala * sirinaFaktor, h = t.height * skala;
    // shadow
    DrawRectangle((int)(cx - w / 2 + 8), (int)(y + 10), (int)w, (int)h, { 0, 0, 0, 90 });
    DrawTexturePro(t, { 0, 0, (float)t.width, (float)t.height }, { cx - w / 2, y, w, h }, { 0, 0 }, 0, nijansa);
}

void CrtajSrce(float x, float y, float skala, bool puno) {
    DrawTextureEx(puno ? srcePuno : srcePrazno, { x, y }, 0, skala, WHITE);
}

}
