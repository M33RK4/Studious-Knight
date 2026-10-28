// Studious Knight, a pixel game for studying. Made by M33RK4.
//
// Answer a question right and your knight cuts the enemy down (the more complete the answer, the
// more XP). Get it wrong and the enemy hits you, the screen flashes red and you see the answer.
// Miss 45% of a level's questions and your knight goes down.
#include "raylib.h"
#include "rlgl.h"
#include "tekst.hpp"
#include "pitanja.hpp"
#include "statistika.hpp"
#include "ulaz.hpp"
#include "grafika.hpp"
#include "zvuk.hpp"
#include "jezik.hpp"

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <random>
#include <ctime>
#include <cstdio>
#include <fstream>
#include <cstdarg>
#include <cctype>

// Settings
static const char* NASLOV_IGRE = "STUDIOUS KNIGHT";   // same in both languages
static const char* AUTOR       = "M33RK4";
static const int   EKRAN_W = 1280, EKRAN_H = 720;
static const int   BROJ_SLOTOVA = 3;
static const float PROCENAT_SMRTI = 0.45f;   // miss this much of a level and you're dead
static const int   MAX_DUZINA_IMENA = 16;
static const float VITEZ_X  = 70.0f;    // where the knight stands in the 320x180 world
static const float ORK_X = 235.0f;   // where the enemy stands
static const float LUTKA_X  = 225.0f;
static const float RAZMAK_NEPRIJATELJA = 200.0f;
static const float MAC_MIR = -45.0f;    // sword angle when standing still, 45 degrees is the only perfectly straight pixel diagonal

static const Color ZLATNA = { 255, 214, 102, 255 };
static const Color KREM   = { 255, 244, 214, 255 };
static const Color PANEL  = { 22, 18, 38, 222 };
static const Color OKVIR  = { 214, 170, 60, 255 };
static const Color ZELENA = { 120, 230, 120, 255 };
static const Color CRVENA = { 245, 90, 90, 255 };
static const Color SIVA   = { 170, 168, 196, 255 };
static const Color SMEDJA = { 92, 56, 30, 255 };

Jezik JEZIK = Jezik::BOSANSKI;

enum class Ekran { SPREMANJA, MENI, BORBA, VJEZBA, STATISTIKA, OKLOPI, PREDMET, NIVOI, DOSTIGNUCA };

// Računarske mreže has 4 levels, easiest to hardest. Each one has its own map (0 valley, 1 swamp,
// 2 dungeon, 3 volcano) and its own enemies. The questions are in the pitanja folder next to the game.
struct NivoPitanja {
    const char* datoteka;        // inside the pitanja folder
    const char* naziv;           // name of the source document
    const char* fokusBs; const char* fokusEn;
    const char* mapaBs;  const char* mapaEn;
    std::vector<Pitanje> pitanja;
    std::string greska;
};
static const int BROJ_NIVOA = 4;
static NivoPitanja NIVOI[BROJ_NIVOA] = {
    { "rm_1_esejska_1p.txt",  "RM Esejska - 1P", "1. parcijala", "1st Midterm Focus", "Dolina stećaka", "Valley of Tombstones", {}, "" },
    { "rm_2_integralno.txt",  "Pitanja i odgovori RM integralno", "Integralni ispit", "Integral Exam Q&A", "Krvava močvara", "Blood Swamp", {}, "" },
    { "rm_3_esejska_2p.txt",  "RM Esejska - 2P", "2. parcijala", "2nd Midterm Focus", "Tamnica kostiju", "Dungeon of Bones", {}, "" },
    { "rm_4_kompletno.txt",   "RAČUNARSKE MREŽE - kompletna skripta", "Cijeli kurs", "Full Course", "Vulkansko carstvo", "Volcanic Realm", {}, "" },
};
// NAPAD is the final blow, DJELIMICNO is a sentence that only wounds the enemy
enum class Faza { HOD, PITANJE, NAPAD, DJELIMICNO, ORK_NAPADA, OTKRIVANJE, BIJEG, SMRT, POBJEDA };

// the answer box (see ObradiUnos and CrtajUnos)
struct Unos {
    std::string tekst;
    int kursor = 0;          // byte position in the text
    int skrol = 0;           // first visible line
    float zeljeniX = -1;     // remembers the column when moving up and down
    bool pratiKursor = true; // scroll follows the cursor, turned off once you scroll yourself
    void Ocisti() { *this = Unos(); }
};

struct Cestica { Vector2 p, v; float zivot, max; Color c; float vel, grav; };
struct Natpis  { std::string tekst; Vector2 p; float zivot, max; Color c; const Font* f; };

struct Borba {
    std::vector<int> red;          // order of the questions
    int i = 0, hp = 5, maxHp = 5;
    Faza faza = Faza::HOD;
    float t = 0;
    float kamera = 0, kamStart = 0, kamCilj = 0;
    float vitezOff = 0, macUgao = MAC_MIR, vitezBljesak = 0, vitezRot = 0;
    float orkOffX = 0, orkOffY = 0, orkAlfa = 1, orkBljesak = 0, oruzje = -110;
    float orkRot = 0;              // enemy tilt (staggering when hit, falling when it dies)
    float orkZamah = 0;
    float vitezY = 0;              // the knight's little hop during the dash
    bool prasina = false;          // so the landing dust only spawns once
    bool orkDesno = false;
    float zamah = 0;
    bool udareno = false, xpDodan = false, smijeh = false;
    Unos unos;
    Ocjena ocjena;                 // all the sentences sent for this question added up
    int poslano = 0;
    int noviPogoci = 0;            // new key points from the last sentence
    bool nastaviPoslije = false;   // enemy hit you for a sentence with nothing new, the fight goes on
    float hpPrikaz = 1;            // enemy health shown on the bar (0..1), it catches up smoothly
    bool noviNivoOtkljucan = false, andjeoOtkljucan = false;   // messages for the victory screen
    bool sveSavrseno = true;       // every answer this fight was 100% (Perfectionist achievement)
    bool skokNaBossa = false;      // dev "boss fight" command, skip straight to the boss after this enemy
    int tacnih = 0, netacnih = 0, ubijeno = 0, xpOvaj = 0;
    bool pauza = false;
    int pauzaIzbor = 0;
    float tresenjeUnosa = 0;
};

struct Vjezba {
    int i = 0;
    Unos unos;
    int poslano = 0;               // sentences sent this try, the key points add up in ocjena
    int noviPogoci = 0;            // new key points from the last sentence
    bool zavrsni = false;          // this hit ends the try and shows the result
    std::string zadnjiTekst;       // last sentence sent, it goes back in the box after a wrong answer
    bool rezultat = false, zaviri = false, napad = false, pogodak = false, udareno = false;
    Ocjena ocjena;
    float t = 0;
    float nagib = 0, brzina = 0, lutkaBljesak = 0;
    float vitezOff = 0, macUgao = MAC_MIR, zamah = 0;
    float tresenjeUnosa = 0;
};

struct Slotovi {
    int izbor = 0;
    bool postoji[BROJ_SLOTOVA] = {};
    Statistika pregled[BROJ_SLOTOVA];
    float okret[BROJ_SLOTOVA] = {};    // 0 is the back, 1 the front (for the flip animation)
    bool unosImena = false, brisanje = false;
    bool hardcore = false;             // picked when making a new save
    bool zensko = false;               // picked when making a new save
    std::string ime;
    float ulazT = -1;                  // 0 or more means we're on our way into the game
    float tresenje = 0;
};

struct Igra {
    Ekran ekran = Ekran::SPREMANJA;
    int slot = -1;
    Statistika stat;
    std::string statPutanja;
    std::string folderPodataka;
    std::vector<Pitanje> pitanja;
    std::string greska;
    float vrijeme = 0, tresenje = 0, crveno = 0, spasiTimer = 0;
    float xpPrikaz = 1, noviNivo = 0;
    std::vector<Cestica> cestice;
    std::vector<Natpis> natpisi;
    std::mt19937 rng;
    int meniIzbor = 0;
    int oklopIzbor = 0;
    float oklopTresenje = 0;
    float kameraMeni = 0;
    bool izlaz = false;
    bool noc = false;                  // night mode (Konami code)
    bool samoPregled = false;          // just looking at a fallen hardcore knight's stats
    std::vector<int> tipke;            // last keys pressed, for the Konami code
    int nivo = 0;                      // which level you picked
    bool zaBorbu = true;               // whether the level screen leads to battle or the training yard
    int nivoIzbor = 0;
    float nivoTresenje = 0;            // shakes a locked level
    int kliknutoSunce = 0;             // sun clicks in the menu, 3 gives you a hint
    float sunceSapat = 0;              // how long "try konami" stays visible
    std::string slova;                 // last letters typed in the menu, for "bosnia"
    Borba b;
    Vjezba v;
    Slotovi s;
};

static Igra G;

// Helpers
static float Lerp(float a, float b, float t) { return a + (b - a) * t; }
static float Klamp(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
static float Glatko(float t) { t = Klamp(t, 0, 1); return t * t * (3 - 2 * t); }
static float IzlazBrzo(float t) { t = Klamp(t, 0, 1); return 1 - (1 - t) * (1 - t); }
static float Rnd(float a, float b) { return std::uniform_real_distribution<float>(a, b)(G.rng); }

static std::string Fmt(const char* f, ...) {
    char b[512];
    va_list a; va_start(a, f); std::vsnprintf(b, sizeof b, f, a); va_end(a);
    return b;
}

static void Cestice(Vector2 p, int n, Color c, float brzina, float grav, float vel = 1.5f) {
    for (int k = 0; k < n; k++) {
        float ug = Rnd(0, 2 * PI), s = Rnd(brzina * 0.3f, brzina);
        float z = Rnd(0.4f, 0.9f);
        G.cestice.push_back({ p, { std::cos(ug) * s, std::sin(ug) * s - brzina * 0.3f }, z, z, c, vel, grav });
    }
}

static void Natpisi(const std::string& t, Vector2 svijetPoz, Color c, const Font* f = nullptr) {
    // svijetPoz is in world coordinates (320x180), the text gets drawn on screen (4x)
    G.natpisi.push_back({ t, { svijetPoz.x * 4, svijetPoz.y * 4 }, 1.3f, 1.3f, c, f ? f : &F.srednji });
}

static void SacuvajStat() { if (G.slot >= 0) SacuvajStatistiku(G.stat, G.statPutanja); }

// Armor
// Bosnian text in the male or female form depending on the knight, English stays the same
static const char* LR(bool zensko, const char* bsM, const char* bsZ, const char* en) {
    return JEZIK == Jezik::ENGLESKI ? en : (zensko ? bsZ : bsM);
}
static const char* LR(const char* bsM, const char* bsZ, const char* en) { return LR(G.stat.zensko, bsM, bsZ, en); }

static const char* ImeOklopa(int i) {
    if (i == 0 && G.stat.zensko) return L("RUŽIČASTA VITEZICA", "ROSE KNIGHT");
    switch (i) {
        case 1: return L("BRONZANI ŠTITONOŠA", "BRONZE SQUIRE");
        case 2: return L("GRIMIZNA STRAŽA", "CRIMSON GUARD");
        case 3: return L("VITEZ SJENE", "SHADOW KNIGHT");
        case 4: return L("ZLATNI VITEZ", "GOLDEN KNIGHT");
        case 5: return L("NEBESKI VITEZ", "CELESTIAL KNIGHT");
        case 6: return L("BOSANSKI VITEZ", "BOSNIAN KNIGHT");
        case 7: return L("ANĐEOSKI VITEZ", "ANGELIC KNIGHT");
        default: return L("KLASIČNI OKLOP", "CLASSIC ARMOR");
    }
}
static bool DevRezim();   // defined down by the battle code
static bool OklopOtkljucan(int i) {
    if (DevRezim()) return true;   // dev gets every armor, the secret one too
    if (i == Grafika::OKLOP_BOSANSKI) return G.stat.bosanskiOklop;
    if (i == Grafika::OKLOP_ANDJEO) return G.stat.zavrsenNivo[2];   // made it to the volcano
    return G.stat.nivo >= Grafika::OKLOP_NIVO[i];
}
static int AktivniOklop() {
    int o = G.stat.oklop;
    return (o >= 0 && o < Grafika::BROJ_OKLOPA && OklopOtkljucan(o)) ? o : 0;
}
// the lady knight wears the female version of every armor (and starts in the pink one)
static int IzgledOklopa(int o) { return Grafika::IzgledViteza(o, G.stat.zensko); }

// Achievements
// Each one is a bit in G.stat.dostignuca. They get checked every frame and a new one pops up in
// the corner, no matter which screen you're on.
enum Dostignuce { D_KONAMI, D_BOSNA, D_JAJA, D_SAVRSENSTVO, D_UBIJ30, D_UBIJ50, D_UBIJ100, D_UBIJ200, D_SVI_BOSSOVI, BROJ_DOSTIGNUCA };
static const int UBISTVA_ZA[BROJ_DOSTIGNUCA] = { 0, 0, 0, 0, 30, 50, 100, 200, 0 };

static const char* ImeDostignuca(int d) {
    switch (d) {
        case D_KONAMI:      return L("RADOZNALAC", "CURIOUS ONE");
        case D_BOSNA:       return L("OŠTRO OKO", "SHARP EYE");
        case D_JAJA:        return L("LOVAC NA TAJNE", "EGG HUNTER");
        case D_SAVRSENSTVO: return L("PERFEKCIONISTA", "PERFECTIONIST");
        case D_UBIJ30:      return L("UBICA ČUDOVIŠTA", "MONSTER SLAYER");
        case D_UBIJ50:      return L("PREKALJENI RATNIK", "SEASONED WARRIOR");
        case D_UBIJ100:     return L("CENTURION", "CENTURION");
        case D_UBIJ200:     return L("ŽIVA LEGENDA", "LIVING LEGEND");
        default:            return LR("ZAISTA STUDIOZAN", "ZAISTA STUDIOZNA", "TRULY STUDIOUS");
    }
}
static const char* OpisDostignuca(int d) {
    switch (d) {
        case D_KONAMI:      return L("Unesi Konami šifru.", "Enter the Konami code.");
        case D_BOSNA:       return L("Unesi šifru \"bosnia\".", "Enter the code \"bosnia\".");
        case D_JAJA:        return L("Pronađi sve skrivene tajne.", "Find all the easter eggs.");
        case D_SAVRSENSTVO: return L("Pređi nivo sa 100% na svakom pitanju.", "Beat a level answering 100% on every question.");
        case D_UBIJ30:      return L("Pobijedi 30 neprijatelja.", "Defeat 30 enemies.");
        case D_UBIJ50:      return L("Pobijedi 50 neprijatelja.", "Defeat 50 enemies.");
        case D_UBIJ100:     return L("Pobijedi 100 neprijatelja.", "Defeat 100 enemies.");
        case D_UBIJ200:     return L("Pobijedi 200 neprijatelja.", "Defeat 200 enemies.");
        default:            return LR("Porazi sve bossove - zvanično si dovoljno studiozan!", "Porazi sve bossove - zvanično si dovoljno studiozna!",
                                      "Defeat every boss - you are officially studious enough!");
    }
}
// the secret ones don't show their description until you find them
static bool TajnoDostignuce(int d) { return d == D_KONAMI || d == D_BOSNA; }

static bool UslovDostignuca(int d) {
    const Statistika& s = G.stat;
    switch (d) {
        case D_KONAMI:      return s.konami;
        case D_BOSNA:       return s.bosanskiOklop;
        case D_JAJA:        return s.konami && s.bosanskiOklop;
        case D_SAVRSENSTVO: return s.perfekcionista;
        case D_SVI_BOSSOVI: return s.zavrsenNivo[0] && s.zavrsenNivo[1] && s.zavrsenNivo[2] && s.zavrsenNivo[3];
        default:            return s.ubijeno >= UBISTVA_ZA[d];
    }
}
static bool ImaDostignuce(int d) { return (G.stat.dostignuca >> d) & 1u; }
static int BrojDostignuca() { int n = 0; for (int d = 0; d < BROJ_DOSTIGNUCA; d++) n += ImaDostignuce(d); return n; }

struct ObavijestDostignuca { int d; float t; };
static std::vector<ObavijestDostignuca> obavijesti;   // shown one at a time

// tiho is for loading a save, achievements earned earlier just get marked without a popup
static void ProvjeriDostignuca(bool tiho) {
    if (G.slot < 0 || G.stat.pao) return;
    bool novo = false;
    for (int d = 0; d < BROJ_DOSTIGNUCA; d++) {
        if (ImaDostignuce(d) || !UslovDostignuca(d)) continue;
        G.stat.dostignuca |= 1u << d;
        novo = true;
        if (!tiho) obavijesti.push_back({ d, 0 });
    }
    if (novo) SacuvajStat();
}

// Files
static std::string SlotPutanja(int i) { return G.folderPodataka + "StudiousKnight_save" + std::to_string(i + 1) + ".txt"; }
static std::string PostavkePutanja() { return G.folderPodataka + "StudiousKnight_settings.txt"; }

static void UcitajPostavke() {
    std::ifstream f(PostavkePutanja());
    std::string red;
    while (std::getline(f, red)) {
        if (!red.empty() && red.back() == '\r') red.pop_back();
        if (red == "jezik=en") JEZIK = Jezik::ENGLESKI;
        if (red == "jezik=bs") JEZIK = Jezik::BOSANSKI;
        if (red == "noc=1") G.noc = true;
    }
}

static void SacuvajPostavke() {
    std::ofstream f(PostavkePutanja(), std::ios::trunc);
    f << "# Studious Knight - postavke / settings\n";
    f << "jezik=" << (JEZIK == Jezik::ENGLESKI ? "en" : "bs") << "\n";
    f << "noc=" << (G.noc ? 1 : 0) << "\n";
}

static void PromijeniJezik() {
    JEZIK = JEZIK == Jezik::ENGLESKI ? Jezik::BOSANSKI : Jezik::ENGLESKI;
    SacuvajPostavke();
    Zvuk::Pusti(Zvuk::MENI);
}

static void OsvjeziSlotove() {
    for (int i = 0; i < BROJ_SLOTOVA; i++) {
        G.s.postoji[i] = SpremanjePostoji(SlotPutanja(i));
        G.s.pregled[i] = Statistika();
        if (G.s.postoji[i]) UcitajStatistiku(G.s.pregled[i], SlotPutanja(i));
    }
}

static void OtvoriSlot(int i) {
    G.slot = i;
    G.stat = Statistika();
    G.statPutanja = SlotPutanja(i);
    UcitajStatistiku(G.stat, G.statPutanja);
    G.xpPrikaz = G.stat.nivo + (float)G.stat.xp / XpZaNivo(G.stat.nivo);
    obavijesti.clear();
    ProvjeriDostignuca(true);
    G.meniIzbor = 0;
    G.samoPregled = G.stat.pao;   // a fallen hardcore knight can only look at the stats
    G.ekran = G.stat.pao ? Ekran::STATISTIKA : Ekran::MENI;
}

static void OtvoriEkranSpremanja() {
    if (!G.samoPregled) SacuvajStat();
    G.samoPregled = false;
    OsvjeziSlotove();
    G.s.unosImena = G.s.brisanje = false;
    G.s.ulazT = -1;
    for (int i = 0; i < BROJ_SLOTOVA; i++) G.s.okret[i] = G.s.postoji[i] ? 1.0f : 0.0f;
    if (G.slot >= 0) G.s.izbor = G.slot;
    G.ekran = Ekran::SPREMANJA;
}

// Basic UI
static void Panel(Rectangle r, Color fill = PANEL, Color okvir = OKVIR) {
    int x = (int)r.x, y = (int)r.y, w = (int)r.width, h = (int)r.height;
    DrawRectangle(x + 4, y, w - 8, h, fill);
    DrawRectangle(x, y + 4, w, h - 8, fill);
    DrawRectangle(x + 8, y, w - 16, 4, okvir);
    DrawRectangle(x + 8, y + h - 4, w - 16, 4, okvir);
    DrawRectangle(x, y + 8, 4, h - 16, okvir);
    DrawRectangle(x + w - 4, y + 8, 4, h - 16, okvir);
    DrawRectangle(x + 4, y + 4, 4, 4, okvir);
    DrawRectangle(x + w - 8, y + 4, 4, 4, okvir);
    DrawRectangle(x + 4, y + h - 8, 4, 4, okvir);
    DrawRectangle(x + w - 8, y + h - 8, 4, 4, okvir);
    DrawRectangle(x + 8, y + 4, w - 16, 2, { 255, 255, 255, 30 });
}

static bool Prazno(const std::string& s) { return s.find_first_not_of(" \t\n") == std::string::npos; }

static std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(' ');
    if (a == std::string::npos) return "";
    return s.substr(a, s.find_last_not_of(' ') - a + 1);
}

// One line text input (the knight's name): letters including č ć š ž đ, backspace, ctrl+backspace
static bool ObradiUnos(std::string& unos, int maxZnakova) {
    bool promjena = false;
    int c;
    while ((c = Ulaz::Znak()) != 0) {
        if (c >= 32 && Utf8Duzina(unos) < maxZnakova) { Utf8Dodaj(unos, c); promjena = true; }
    }
    if (Ulaz::PritisnutoPonovo(KEY_BACKSPACE)) {
        if (Ulaz::Ctrl()) Utf8IzbrisiRijec(unos);
        else Utf8IzbrisiZadnji(unos);
        promjena = true;
    }
    if (promjena) Zvuk::Pusti(Zvuk::TIPKA);
    return promjena;
}

// The answer box: multiple lines, a cursor (arrows, Home/End, mouse), Shift+Enter for a new line,
// and the mouse wheel or PageUp/PageDown to scroll.
static const Rectangle UNOS_OKVIR = { 80, 488, 1120, 216 };
static const float UNOS_SIRINA = UNOS_OKVIR.width - 56, UNOS_RED = 32, UNOS_VRH = 46;
static const int UNOS_VIDLJIVO = 4, UNOS_MAX = 1500;

struct LinijaUnosa { int od, doo; };   // bytes [od, doo) of one line on screen

static int Utf8Sljedeci(const std::string& s, int i) {
    if (i >= (int)s.size()) return (int)s.size();
    i++;
    while (i < (int)s.size() && ((unsigned char)s[i] & 0xC0) == 0x80) i++;
    return i;
}
static int Utf8Prethodni(const std::string& s, int i) {
    if (i <= 0) return 0;
    i--;
    while (i > 0 && ((unsigned char)s[i] & 0xC0) == 0x80) i--;
    return i;
}

// wraps the text into lines (by words, '\n' starts a new one) and keeps track of where each line is
static std::vector<LinijaUnosa> RasporediUnos(const std::string& s) {
    std::vector<LinijaUnosa> L;
    int n = (int)s.size(), start = 0;
    while (true) {
        int j = start, razmak = -1;
        bool preliv = false;
        while (j < n && s[j] != '\n') {
            int k = Utf8Sljedeci(s, j);
            if (j > start && SirinaTeksta(F.tekst, s.substr(start, k - start)) > UNOS_SIRINA) { preliv = true; break; }
            if (s[j] == ' ') razmak = j;
            j = k;
        }
        if (!preliv) {
            L.push_back({ start, j });
            if (j >= n) break;
            start = j + 1;                                  // skip the '\n'
        } else if (s[j] == ' ') { L.push_back({ start, j }); start = j + 1; }
        else if (razmak > start) { L.push_back({ start, razmak }); start = razmak + 1; }
        else { L.push_back({ start, j }); start = j; }    // a word too long for the line gets split by letters
    }
    return L;
}

static int RedKursora(const std::vector<LinijaUnosa>& L, int kursor) {
    int r = 0;
    for (int i = 0; i < (int)L.size(); i++) if (L[i].od <= kursor) r = i;
    return r;
}

// the spot in a line closest to x
static int PozicijaURedu(const std::string& s, const LinijaUnosa& l, float x) {
    int naj = l.od;
    float najD = 1e9f;
    for (int i = l.od; ; i = Utf8Sljedeci(s, i)) {
        float d = std::fabs(SirinaTeksta(F.tekst, s.substr(l.od, i - l.od)) - x);
        if (d < najD) { najD = d; naj = i; }
        if (i >= l.doo) break;
    }
    return naj;
}

static bool ObradiUnos(Unos& u) {
    std::string& s = u.tekst;
    bool promjena = false, pomak = false;
    bool ctrl = Ulaz::Ctrl();
    auto ubaci = [&](const std::string& t) {
        if (Utf8Duzina(s) >= UNOS_MAX) return;
        s.insert(u.kursor, t); u.kursor += (int)t.size(); promjena = true;
    };
    int c;
    while ((c = Ulaz::Znak()) != 0) {
        if (c < 32) continue;
        std::string t; Utf8Dodaj(t, c); ubaci(t);
    }
    if (Ulaz::PritisnutoPonovo(KEY_ENTER) && Ulaz::Shift()) ubaci("\n");
    if (Ulaz::PritisnutoPonovo(KEY_BACKSPACE) && u.kursor > 0) {
        int p = Utf8Prethodni(s, u.kursor);
        if (ctrl) {   // the whole word before the cursor
            p = u.kursor;
            while (p > 0 && (s[p - 1] == ' ' || s[p - 1] == '\n')) p--;
            while (p > 0 && s[p - 1] != ' ' && s[p - 1] != '\n') p = Utf8Prethodni(s, p);
        }
        s.erase(p, u.kursor - p); u.kursor = p; promjena = true;
    }
    if (Ulaz::PritisnutoPonovo(KEY_DELETE) && u.kursor < (int)s.size()) {
        s.erase(u.kursor, Utf8Sljedeci(s, u.kursor) - u.kursor); promjena = true;
    }
    if (Ulaz::PritisnutoPonovo(KEY_LEFT) && u.kursor > 0) {
        u.kursor = Utf8Prethodni(s, u.kursor);
        if (ctrl) {
            while (u.kursor > 0 && s[u.kursor] == ' ') u.kursor = Utf8Prethodni(s, u.kursor);
            while (u.kursor > 0 && s[u.kursor - 1] != ' ' && s[u.kursor - 1] != '\n') u.kursor = Utf8Prethodni(s, u.kursor);
        }
        pomak = true;
    }
    if (Ulaz::PritisnutoPonovo(KEY_RIGHT) && u.kursor < (int)s.size()) {
        u.kursor = Utf8Sljedeci(s, u.kursor);
        if (ctrl) {
            while (u.kursor < (int)s.size() && s[u.kursor] != ' ' && s[u.kursor] != '\n') u.kursor = Utf8Sljedeci(s, u.kursor);
            while (u.kursor < (int)s.size() && s[u.kursor] == ' ') u.kursor++;
        }
        pomak = true;
    }
    bool gore = Ulaz::PritisnutoPonovo(KEY_UP), dolje = Ulaz::PritisnutoPonovo(KEY_DOWN);
    bool pocetak = Ulaz::Pritisnuto(KEY_HOME), kraj = Ulaz::Pritisnuto(KEY_END);
    Vector2 m = Ulaz::Mis();
    bool klik = Ulaz::Klik() && CheckCollisionPointRec(m, UNOS_OKVIR);
    if (gore || dolje || pocetak || kraj || klik) {
        std::vector<LinijaUnosa> L = RasporediUnos(s);
        int r = RedKursora(L, u.kursor);
        float x = SirinaTeksta(F.tekst, s.substr(L[r].od, u.kursor - L[r].od));
        if (gore || dolje) {
            if (u.zeljeniX < 0) u.zeljeniX = x;
            int nr = r + (dolje ? 1 : -1);
            if (nr < 0) u.kursor = 0;
            else if (nr >= (int)L.size()) u.kursor = (int)s.size();
            else u.kursor = PozicijaURedu(s, L[nr], u.zeljeniX);
        }
        if (pocetak) u.kursor = ctrl ? 0 : L[r].od;
        if (kraj) u.kursor = ctrl ? (int)s.size() : L[r].doo;
        if (klik) {
            int nr = u.skrol + (int)std::floor((m.y - UNOS_OKVIR.y - UNOS_VRH) / UNOS_RED);
            nr = std::max(0, std::min((int)L.size() - 1, nr));
            u.kursor = PozicijaURedu(s, L[nr], m.x - UNOS_OKVIR.x - 28);
        }
        pomak = true;
    }
    if (!(gore || dolje)) { if (pomak || promjena) u.zeljeniX = -1; }
    if (pomak || promjena) u.pratiKursor = true;

    // scrolling without moving the cursor
    float t = Ulaz::Tockic();
    int skrol = 0;
    if (t != 0 && CheckCollisionPointRec(m, UNOS_OKVIR)) skrol = t > 0 ? -1 : 1;
    if (Ulaz::PritisnutoPonovo(KEY_PAGE_UP)) skrol = -(UNOS_VIDLJIVO - 1);
    if (Ulaz::PritisnutoPonovo(KEY_PAGE_DOWN)) skrol = UNOS_VIDLJIVO - 1;
    if (skrol) {
        int redova = (int)RasporediUnos(s).size();
        u.skrol = std::max(0, std::min(std::max(0, redova - UNOS_VIDLJIVO), u.skrol + skrol));
        u.pratiKursor = false;
    }
    if (promjena) Zvuk::Pusti(Zvuk::TIPKA);
    return promjena;
}

// ENTER sends the answer, Shift+Enter is a new line
static bool PosaljiOdgovor() { return Ulaz::Pritisnuto(KEY_ENTER) && !Ulaz::Shift(); }

static bool Treperi() { return std::fmod(G.vrijeme, 1.2f) < 0.85f; }

static void CrtajPitanje(const std::string& tekst, float y) {
    std::vector<std::string> linije = PrelomiTekst(F.veliki, tekst, 1080);
    float h = linije.size() * 44.0f + 30;
    Panel({ 80, y, 1120, h });
    for (size_t k = 0; k < linije.size(); k++)
        TekstCentar(F.veliki, linije[k], 640, y + 18 + k * 44.0f, ZLATNA);
}

static void CrtajUnos(Unos& u, bool aktivan, const std::string& naslov, const std::string& dno, float tresenje) {
    float ox = tresenje > 0 ? std::sin(tresenje * 60) * 8 * tresenje : 0;
    Rectangle r = UNOS_OKVIR;
    r.x += ox;
    Panel(r, PANEL, aktivan ? OKVIR : Color{ 110, 100, 130, 255 });
    Tekst(F.mali, naslov, r.x + 24, r.y + 18, aktivan ? ZLATNA : SIVA);
    if (aktivan) {
        std::string h = L("SHIFT+ENTER = novi red   PGUP/PGDN = pomjeri", "SHIFT+ENTER = new line   PGUP/PGDN = scroll");
        Tekst(F.tekstMali, h, r.x + r.width - 28 - SirinaTeksta(F.tekstMali, h), r.y + 12, { 120, 116, 150, 255 });
    }
    bool kursor = aktivan && std::fmod(G.vrijeme, 1.0f) < 0.55f;
    const std::string& s = u.tekst;
    std::vector<LinijaUnosa> R = RasporediUnos(s);
    int n = (int)R.size(), rk = RedKursora(R, u.kursor);
    if (u.pratiKursor) {
        if (rk < u.skrol) u.skrol = rk;
        if (rk >= u.skrol + UNOS_VIDLJIVO) u.skrol = rk - UNOS_VIDLJIVO + 1;
    }
    u.skrol = std::max(0, std::min(std::max(0, n - UNOS_VIDLJIVO), u.skrol));
    float y0 = r.y + UNOS_VRH, x0 = r.x + 28;
    if (s.empty() && aktivan)
        Tekst(F.tekst, L("Piši odgovor ovdje... (kvačice nisu obavezne)", "Type your answer here... (Bosnian letters optional)"), x0 + 6, y0, { 120, 116, 150, 255 });
    for (int k = u.skrol; k < n && k < u.skrol + UNOS_VIDLJIVO; k++) {
        float y = y0 + (k - u.skrol) * UNOS_RED;
        Tekst(F.tekst, s.substr(R[k].od, R[k].doo - R[k].od), x0, y, KREM);
        if (k == rk && kursor) {
            float kx = x0 + SirinaTeksta(F.tekst, s.substr(R[k].od, u.kursor - R[k].od));
            DrawRectangle((int)kx, (int)y + 2, 3, 28, ZLATNA);
        }
    }
    if (n > UNOS_VIDLJIVO) {   // scrollbar
        float tx = r.x + r.width - 22, ty = y0 + 2, th = UNOS_VIDLJIVO * UNOS_RED - 4;
        DrawRectangle((int)tx, (int)ty, 6, (int)th, { 50, 44, 70, 255 });
        float dio = th * UNOS_VIDLJIVO / n;
        float poz = (th - dio) * u.skrol / std::max(1, n - UNOS_VIDLJIVO);
        DrawRectangle((int)tx, (int)(ty + poz), 6, (int)dio, aktivan ? ZLATNA : SIVA);
    }
    Tekst(F.tekstMali, dno, x0, r.y + r.height - 34, SIVA);
}

// Panel with the correct answer and its key points.
// o == nullptr shows the key points as a plain list
static void CrtajOdgovor(const Pitanje& p, const Ocjena* o, const std::string& naslov, Color bojaNaslova,
                         const std::string& dno, int potrebno = -1) {
    const float X = 60, W = 1160, MAX_H = 640, VRH = 66;
    float x0 = X + 32, sirina = W - 64;

    std::vector<std::string> sve;
    std::vector<bool> prvi;
    for (const std::string& red : p.odgovor) {
        std::vector<std::string> l = PrelomiTekst(F.tekstMali, red, sirina - 24);
        for (size_t k = 0; k < l.size(); k++) { sve.push_back(l[k]); prvi.push_back(k == 0); }
    }
    std::vector<std::string> pl = PrelomiTekst(F.tekstMali, std::string(L("Pitanje: ", "Question: ")) + p.tekst, sirina);
    int treba = potrebno >= 0 ? potrebno : (o ? o->potrebno : p.prag);
    std::string info = o
        ? Fmt(L("Ključni pojmovi u tvom odgovoru: %d / %d  (potrebno %d)", "Key points in your answer: %d / %d  (needed %d)"), o->pogodjeno, (int)p.pojmovi.size(), treba)
        : Fmt(L("Ključni pojmovi (potrebno pogoditi %d od %d):", "Key points (you need %d of %d):"), treba, (int)p.pojmovi.size());

    // drawn in two passes, the first one only measures how tall it is
    auto sadrzaj = [&](float y, bool crtaj, float razmak) -> float {
        if (crtaj) TekstCentar(F.srednji, naslov, 640, y, bojaNaslova);
        y += 42;
        for (auto& l : pl) { if (crtaj) Tekst(F.tekstMali, l, x0, y, ZLATNA); y += 24; }
        y += 6;
        if (crtaj) Tekst(F.tekstMali, info, x0, y, SIVA);
        y += 30;
        float cx = x0;
        for (size_t k = 0; k < p.pojmovi.size(); k++) {
            bool hit = o && k < o->pogodak.size() && o->pogodak[k];
            std::string t = (o ? (hit ? "+ " : "x ") : "") + p.pojmovi[k].naziv;
            float w = SirinaTeksta(F.tekstMali, t) + 20;
            if (cx + w > x0 + sirina) { cx = x0; y += 34; }
            if (crtaj) {
                Color bg = !o ? Color{ 58, 52, 92, 255 } : (hit ? Color{ 34, 96, 46, 255 } : Color{ 112, 34, 40, 255 });
                Color ok = !o ? Color{ 130, 120, 190, 255 } : (hit ? ZELENA : CRVENA);
                DrawRectangle((int)cx, (int)y, (int)w, 28, bg);
                DrawRectangleLinesEx({ cx, y, w, 28 }, 2, ok);
                Tekst(F.tekstMali, t, cx + 10, y + 2, KREM);
            }
            cx += w + 8;
        }
        y += 44;
        if (crtaj) DrawRectangle((int)x0, (int)y, (int)sirina, 2, { 214, 170, 60, 120 });
        y += 14;
        for (size_t k = 0; k < sve.size(); k++) {
            if (crtaj) {
                if (prvi[k]) DrawRectangle((int)x0 + 4, (int)y + 10, 6, 6, ZLATNA);
                Tekst(F.tekstMali, sve[k], x0 + 24, y, KREM);
            }
            y += razmak;
        }
        return y;
    };

    float razmak = 25;
    float h = sadrzaj(0, false, razmak) + 22 + 64;
    if (h > MAX_H) {
        float visak = h - MAX_H;
        razmak = std::max(19.0f, razmak - visak / std::max<size_t>(1, sve.size()));
        h = std::min(MAX_H, sadrzaj(0, false, razmak) + 22 + 64);
    }
    float y0 = VRH + (MAX_H - h) / 2;
    Panel({ X, y0, W, h });
    sadrzaj(y0 + 22, true, razmak);
    if (Treperi()) TekstCentar(F.mali, dno, 640, y0 + h - 40, ZLATNA, false);
}

static void CrtajXpTraku(float x, float y, float w) {
    int nivo = (int)std::floor(G.xpPrikaz);
    float udio = G.xpPrikaz - nivo;
    std::string b = Fmt(L("NIVO %d", "LEVEL %d"), nivo);
    TekstSjena(F.mali, b, x, y + 3, ZLATNA, 2);
    float bx = x + SirinaTeksta(F.mali, b) + 16;
    float bw = w - (bx - x);
    DrawRectangle((int)bx - 4, (int)y - 2, (int)bw + 8, 26, { 20, 16, 30, 255 });
    DrawRectangle((int)bx, (int)y + 2, (int)bw, 18, { 50, 44, 70, 255 });
    DrawRectangle((int)bx, (int)y + 2, (int)(bw * udio), 18, { 90, 200, 255, 255 });
    DrawRectangle((int)bx, (int)y + 2, (int)(bw * udio), 4, { 180, 236, 255, 255 });
    if (G.noviNivo > 0 && std::fmod(G.vrijeme, 0.2f) < 0.1f) DrawRectangle((int)bx, (int)y + 2, (int)bw, 18, { 255, 255, 255, 120 });
    b = Fmt("%d / %d XP", G.stat.xp, XpZaNivo(G.stat.nivo));
    TekstSjena(F.tekstMali, b, bx + bw - SirinaTeksta(F.tekstMali, b), y + 26, KREM, 2);
}

// ================================================================== SAVE SLOTS
static float KartaX(int i) { return 640.0f + (i - 1) * 300.0f; }
static const float KARTA_Y = 150, KARTA_SKALA = 4;

static void AzurirajSpremanja(float dt) {
    Slotovi& s = G.s;
    s.tresenje = std::max(0.0f, s.tresenje - dt * 2.5f);
    for (int i = 0; i < BROJ_SLOTOVA; i++) {
        float cilj = s.postoji[i] ? 1.0f : 0.0f;
        if (s.okret[i] < cilj) s.okret[i] = std::min(cilj, s.okret[i] + dt * 1.8f);
        if (s.okret[i] > cilj) s.okret[i] = std::max(cilj, s.okret[i] - dt * 1.8f);
    }

    if (s.ulazT >= 0) {            // the card flips and lifts, then we're in
        s.ulazT += dt;
        if (s.ulazT > 1.0f && s.okret[s.izbor] >= 1.0f) OtvoriSlot(s.izbor);
        return;
    }

    if (s.unosImena) {
        ObradiUnos(s.ime, MAX_DUZINA_IMENA);
        if (Ulaz::Pritisnuto(KEY_ESCAPE)) { s.unosImena = false; Zvuk::Pusti(Zvuk::MENI); return; }
        // TAB or a click toggles hardcore, only now, it can't be changed later
        if (Ulaz::Pritisnuto(KEY_TAB) || (Ulaz::Klik() && CheckCollisionPointRec(Ulaz::Mis(), { 380, 424, 520, 40 }))) {
            s.hardcore = !s.hardcore;
            Zvuk::Pusti(s.hardcore ? Zvuk::BOL : Zvuk::MENI);
        }
        // knight or lady knight: left/right arrows or click the button
        bool staro = s.zensko;
        if (Ulaz::Pritisnuto(KEY_LEFT)) s.zensko = false;
        if (Ulaz::Pritisnuto(KEY_RIGHT)) s.zensko = true;
        if (Ulaz::Klik()) {
            if (CheckCollisionPointRec(Ulaz::Mis(), { 380, 370, 256, 40 })) s.zensko = false;
            if (CheckCollisionPointRec(Ulaz::Mis(), { 644, 370, 256, 40 })) s.zensko = true;
        }
        if (staro != s.zensko) Zvuk::Pusti(Zvuk::MENI);
        if (Ulaz::Pritisnuto(KEY_ENTER)) {
            std::string ime = Trim(s.ime);
            if (ime.empty()) { s.tresenje = 1; Zvuk::Pusti(Zvuk::PROMASAJ); return; }
            Statistika nova;
            nova.ime = ime;
            nova.hardcore = s.hardcore;
            nova.zensko = s.zensko;
            SacuvajStatistiku(nova, SlotPutanja(s.izbor));
            s.postoji[s.izbor] = true;
            s.pregled[s.izbor] = nova;
            s.unosImena = false;
            s.ulazT = 0;
            Zvuk::Pusti(Zvuk::NIVO);
        }
        return;
    }

    if (s.brisanje) {
        if (Ulaz::Pritisnuto(KEY_ESCAPE)) { s.brisanje = false; Zvuk::Pusti(Zvuk::MENI); }
        else if (Ulaz::Pritisnuto(KEY_ENTER)) {
            ObrisiSpremanje(SlotPutanja(s.izbor));
            if (G.slot == s.izbor) G.slot = -1;
            s.postoji[s.izbor] = false;
            s.pregled[s.izbor] = Statistika();
            s.brisanje = false;
            Zvuk::Pusti(Zvuk::SMRT);
        }
        return;
    }

    int stari = s.izbor;
    if (Ulaz::Pritisnuto(KEY_LEFT) || Ulaz::Pritisnuto(KEY_A)) s.izbor = (s.izbor + BROJ_SLOTOVA - 1) % BROJ_SLOTOVA;
    if (Ulaz::Pritisnuto(KEY_RIGHT) || Ulaz::Pritisnuto(KEY_D)) s.izbor = (s.izbor + 1) % BROJ_SLOTOVA;
    bool otvori = Ulaz::Pritisnuto(KEY_ENTER) || Ulaz::Pritisnuto(KEY_SPACE);
    Vector2 m = Ulaz::Mis();
    for (int i = 0; i < BROJ_SLOTOVA; i++) {
        Rectangle r = { KartaX(i) - 108, KARTA_Y - 20, 216, 400 };
        if (CheckCollisionPointRec(m, r)) { s.izbor = i; if (Ulaz::Klik()) otvori = true; }
    }
    if (CheckCollisionPointRec(m, { 960, 676, 320, 44 }) && Ulaz::Klik()) PromijeniJezik();
    if (stari != s.izbor) Zvuk::Pusti(Zvuk::MENI);
    if (Ulaz::Pritisnuto(KEY_L)) PromijeniJezik();
    if ((Ulaz::Pritisnuto(KEY_DELETE) || Ulaz::Pritisnuto(KEY_X)) && s.postoji[s.izbor]) {
        s.brisanje = true; Zvuk::Pusti(Zvuk::PROMASAJ); return;
    }
    if (otvori) {
        if (s.postoji[s.izbor]) { s.ulazT = 0.5f; Zvuk::Pusti(Zvuk::MAC); }
        else { s.unosImena = true; s.ime.clear(); s.hardcore = false; s.zensko = false; Zvuk::Pusti(Zvuk::MENI); }
    }
}

static void CrtajSpremanjaUI() {
    Slotovi& s = G.s;
    DrawRectangle(0, 0, EKRAN_W, EKRAN_H, { 10, 8, 30, 140 });
    TekstCentar(F.naslov, NASLOV_IGRE, 640, 30, ZLATNA);
    TekstCentar(F.mali, L("Izaberi kartu sudbine - svoju spremljenu igru", "Choose your card of fate - your save file"), 640, 100, KREM);

    const char* rimski[] = { "I", "II", "III" };
    for (int i = 0; i < BROJ_SLOTOVA; i++) {
        bool izabran = s.izbor == i;
        float podigni = izabran ? -14 + std::sin(G.vrijeme * 3) * 3 : 0;
        if (izabran && s.ulazT >= 0) podigni -= IzlazBrzo(s.ulazT * 2) * 20;
        float cx = KartaX(i), y = KARTA_Y + podigni;
        float w = Grafika::KARTA_W * KARTA_SKALA, h = Grafika::KARTA_H * KARTA_SKALA;
        if (izabran) {
            unsigned char a = (unsigned char)(120 + 80 * std::sin(G.vrijeme * 5));
            float gw = w * std::max(0.02f, (float)std::fabs(std::cos(s.okret[i] * PI)));
            DrawRectangle((int)(cx - gw / 2 - 8), (int)(y - 8), (int)(gw + 16), (int)(h + 16), { 255, 214, 102, a });
        }
        float okret = s.okret[i];
        float faktor = std::fabs(std::cos(okret * PI));
        bool lice = okret > 0.5f;
        bool pao = s.postoji[i] && s.pregled[i].pao;
        Color nijansa = pao ? Color{ 120, 112, 120, 255 } : (izabran ? WHITE : Color{ 190, 186, 200, 255 });
        Grafika::CrtajKartu(cx, y, KARTA_SKALA, std::max(0.02f, faktor), lice, nijansa, s.pregled[i].zensko);
        if (lice && faktor > 0.85f) {
            TekstCentar(F.mali, rimski[i], cx, y + 5 * KARTA_SKALA + 12, SMEDJA, false);
            std::string ime = s.pregled[i].ime.empty() ? "?" : s.pregled[i].ime;
            const Font& fi = SirinaTeksta(F.tekst, ime) > w - 50 ? F.tekstMali : F.tekst;
            TekstCentar(fi, ime, cx, y + 72 * KARTA_SKALA + (52 - fi.baseSize) / 2.0f, SMEDJA, false);
            if (pao) {   // a stećak tombstone over the card window
                float sx = cx - 40, sy = y + 150;
                DrawRectangle((int)sx, (int)sy + 20, 80, 100, { 150, 144, 128, 255 });
                DrawRectangle((int)sx + 10, (int)sy, 60, 24, { 150, 144, 128, 255 });
                DrawRectangleLinesEx({ sx, sy + 20, 80, 100 }, 4, { 62, 58, 54, 255 });
                DrawRectangle((int)cx - 4, (int)sy + 40, 8, 50, { 62, 58, 54, 255 });     // cross
                DrawRectangle((int)cx - 20, (int)sy + 54, 40, 8, { 62, 58, 54, 255 });
                TekstCentar(F.mali, LR(s.pregled[i].zensko, "PAO", "PALA", "FALLEN"), cx, sy + 98, CRVENA);
            }
        }
        // text under the card
        float ty = KARTA_Y + h + 22;
        if (s.postoji[i]) {
            const Statistika& p = s.pregled[i];
            TekstCentar(F.mali, Fmt(L("NIVO %d", "LEVEL %d"), p.nivo), cx, ty, izabran ? ZLATNA : KREM);
            TekstCentar(F.tekstMali, Fmt(L("%d ubijenih  -  %s", "%d slain  -  %s"), p.ubijeno, FormatVremena(p.sekundeIgre).c_str()), cx, ty + 26, KREM);
            if (p.hardcore)
                TekstCentar(F.tekstMali, pao ? LR(p.zensko, "HARDCORE - PAO (samo pregled)", "HARDCORE - PALA (samo pregled)", "HARDCORE - FALLEN (view only)") : "HARDCORE", cx, ty + 52, CRVENA);
        } else {
            TekstCentar(F.mali, L("PRAZNO", "EMPTY"), cx, ty, izabran ? ZLATNA : SIVA);
            TekstCentar(F.tekstMali, L("nova igra", "new game"), cx, ty + 26, SIVA);
        }
    }

    std::string h = L("STRELICE = izaberi    ENTER = otvori    DEL = obriši", "ARROWS = choose    ENTER = open    DEL = delete");
    TekstSjena(F.tekstMali, h, 640 - SirinaTeksta(F.tekstMali, h) / 2, 640, SIVA, 2);
    TekstSjena(F.mali, std::string(L("Autor: ", "Author: ")) + AUTOR, 24, 690, KREM, 2);
    std::string j = L("L = JEZIK: BOSANSKI", "L = LANGUAGE: ENGLISH");
    TekstSjena(F.mali, j, 1256 - SirinaTeksta(F.mali, j), 690, { 140, 210, 255, 255 }, 2);

    if (s.unosImena) {
        DrawRectangle(0, 0, EKRAN_W, EKRAN_H, { 0, 0, 0, 150 });
        float ox = s.tresenje > 0 ? std::sin(s.tresenje * 60) * 8 * s.tresenje : 0;
        Panel({ 300 + ox, 196, 680, 360 }, PANEL, s.hardcore ? CRVENA : OKVIR);
        TekstCentar(F.srednji, s.zensko ? L("KAKO SE ZOVEŠ, VITEZICE?", "WHAT IS YOUR NAME, KNIGHT?") : L("KAKO SE ZOVEŠ, VITEŽE?", "WHAT IS YOUR NAME, KNIGHT?"),
                    640, 232, ZLATNA);
        TekstCentar(F.tekstMali, Fmt(L("Spremanje %s", "Save file %s"), rimski[s.izbor]), 640, 270, SIVA, false);
        Rectangle r = { 380 + ox, 300, 520, 56 };
        DrawRectangleRec(r, { 12, 10, 24, 255 });
        DrawRectangleLinesEx(r, 3, OKVIR);
        Tekst(F.tekst, s.ime, r.x + 18, r.y + 12, KREM);
        if (std::fmod(G.vrijeme, 1.0f) < 0.55f) Tekst(F.tekst, "_", r.x + 20 + SirinaTeksta(F.tekst, s.ime), r.y + 12, ZLATNA);
        // knight or lady knight
        for (int k = 0; k < 2; k++) {
            Rectangle sr = { 380 + ox + k * 264, 370, 256, 40 };
            bool izabrano = s.zensko == (k == 1);
            Color boja = k == 1 ? Color{ 255, 140, 190, 255 } : Color{ 140, 210, 255, 255 };
            DrawRectangleRec(sr, izabrano ? Fade(boja, 0.25f) : Color{ 12, 10, 24, 255 });
            DrawRectangleLinesEx(sr, 2, izabrano ? boja : Color{ 110, 100, 130, 255 });
            TekstCentar(F.mali, k == 1 ? L("ŽENSKO", "FEMALE") : L("MUŠKO", "MALE"), sr.x + sr.width / 2, sr.y + 12, izabrano ? boja : SIVA, false);
        }
        // hardcore
        Rectangle hr = { 380 + ox, 424, 520, 40 };
        DrawRectangleRec(hr, s.hardcore ? Color{ 70, 14, 22, 255 } : Color{ 12, 10, 24, 255 });
        DrawRectangleLinesEx(hr, 2, s.hardcore ? CRVENA : Color{ 110, 100, 130, 255 });
        DrawRectangle((int)hr.x + 12, (int)hr.y + 10, 20, 20, { 30, 24, 40, 255 });
        if (s.hardcore) DrawRectangle((int)hr.x + 16, (int)hr.y + 14, 12, 12, CRVENA);
        Tekst(F.mali, s.hardcore ? L("HARDCORE: UKLJUČEN", "HARDCORE: ON") : L("HARDCORE: ISKLJUČEN", "HARDCORE: OFF"),
              hr.x + 44, hr.y + 12, s.hardcore ? CRVENA : SIVA);
        TekstCentar(F.tekstMali, s.hardcore ? L("Jedna smrt i vitez je zauvijek izgubljen. Ne može se kasnije promijeniti!",
                                                "One death and the knight is lost forever. This can't be changed later!")
                                            : L("TAB = hardcore (trajna smrt) - bira se samo sada", "TAB = hardcore (permadeath) - only chosen now"),
                    640, 472, s.hardcore ? CRVENA : SIVA, false);
        TekstCentar(F.tekstMali, L("STRELICE = spol    ENTER = potvrdi    ESC = nazad", "ARROWS = gender    ENTER = confirm    ESC = back"), 640, 508, SIVA, false);
    }
    if (s.brisanje) {
        DrawRectangle(0, 0, EKRAN_W, EKRAN_H, { 40, 0, 0, 160 });
        Panel({ 300, 250, 680, 220 }, { 40, 12, 20, 235 }, CRVENA);
        TekstCentar(F.srednji, L("OBRISATI SPREMANJE?", "DELETE THIS SAVE FILE?"), 640, 290, CRVENA);
        TekstCentar(F.tekst, s.pregled[s.izbor].ime, 640, 340, KREM, false);
        TekstCentar(F.tekstMali, LR(s.pregled[s.izbor].zensko, "Sav napredak ovog viteza će nestati.", "Sav napredak ove vitezice će nestati.", "All progress of this knight will be lost."), 640, 380, SIVA, false);
        TekstCentar(F.tekstMali, L("ENTER = da, obriši    ESC = ne", "ENTER = yes, delete    ESC = no"), 640, 420, ZLATNA, false);
    }
}

// ================================================================== BATTLE
// Every enemy is one question and its health is the answer's key points. You can answer sentence by
// sentence, each one hurts the enemy for the share of new key points it hits, and a sentence with
// nothing new costs a life. An empty ENTER is the final blow, the enemy goes down if you hit enough
// key points. A fully correct answer in one go kills it right away.
// The last 10% of the questions are the boss, one phase per question.
static const Pitanje& TrenutnoPitanje() { return G.pitanja[G.b.red[G.b.i]]; }

static int BrojFazaBossa() {
    int n = (int)G.b.red.size();
    if (n < 3) return 0;
    return std::max(1, (int)std::ceil(n * 0.10 - 1e-6));
}
static int PrviBoss() { return (int)G.b.red.size() - BrojFazaBossa(); }
static bool JeBoss() { return BrojFazaBossa() > 0 && G.b.i >= PrviBoss(); }
static int FazaBossa() { return G.b.i - PrviBoss(); }
static bool ZadnjaFazaBossa() { return JeBoss() && G.b.i == (int)G.b.red.size() - 1; }
static int BrojNeprijatelja() { return PrviBoss() + (BrojFazaBossa() > 0 ? 1 : 0); }
// which enemies show up depends on the level: orcs, swamp orcs, skeletons, demons
static int VrstaOrka() {
    const Grafika::NeprijateljiNivoa& n = Grafika::NEPRIJATELJI_NIVOA[G.nivo];
    return JeBoss() ? n.boss : n.prvi + G.b.red[G.b.i] % n.brojObicnih;
}

static const char* ImeOrka(int v) {
    switch (v) {
        case 1: return L("ORK BERSERKER", "ORC BERSERKER");
        case 2: return L("ORK ŠAMAN", "ORC SHAMAN");
        case 3: return L("ORK KOPLJANIK", "ORC SPEARMAN");
        case 4: return L("ORK GOROSTAS", "ORC BRUTE");
        case 5: return L("ORKOVSKI VOJSKOVOĐA", "ORC WARLORD");
        case 6: return L("MOČVARNI LOVAC", "BOG HUNTER");
        case 7: return L("ORK IZ BLATA", "MUD BRUTE");
        case 8: return L("MOČVARNI VRAČ", "BOG WITCH DOCTOR");
        case 9: return L("PUHAČ OTROVA", "POISON BLOWGUNNER");
        case 10: return L("POGLAVICA MOČVARE", "BOG CHIEFTAIN");
        case 11: return L("KOSTUR RATNIK", "SKELETON WARRIOR");
        case 12: return L("KOSTUR STRIJELAC", "SKELETON ARCHER");
        case 13: return L("NEKROMANT", "NECROMANCER");
        case 14: return L("KOSTUR VITEZ", "SKELETON KNIGHT");
        case 15: return L("KRALJ KOSTIJU", "LICH KING");
        case 16: return L("IMP", "IMP");
        case 17: return L("PLAMENI DEMON", "FIRE DEMON");
        case 18: return L("PAKLENSKI PAS", "HELLHOUND");
        case 19: return L("DEMON DŽELAT", "DEMON EXECUTIONER");
        case 20: return L("KRALJ DEMONA", "DEMON KING");
        default: return L("ORK RATNIK", "ORC GRUNT");
    }
}

// enemy family for the laughs and messages: 0 orcs, 1 skeletons, 2 demons
static int PorodicaNeprijatelja(int v) { return v >= 16 ? 2 : (v >= 11 ? 1 : 0); }
static bool JakUdarac(int v) { return v == 4 || v == 5 || v == 7 || v == 10 || v == 15 || v == 19 || v == 20; }

static int BrojPojmova() { return (int)TrenutnoPitanje().pojmovi.size(); }

// enemy health 0..1 (for a boss it's all the phases together)
static float ZivotNeprijatelja() {
    Borba& b = G.b;
    int uk = BrojPojmova();
    float udio = uk > 0 ? (float)b.ocjena.pogodjeno / uk : 1.0f;
    if (b.faza == Faza::NAPAD && b.udareno) udio = 1;
    if (!JeBoss()) return 1 - udio;
    return 1 - (FazaBossa() + udio) / BrojFazaBossa();
}

static void NovoPitanje() {
    Borba& b = G.b;
    b.unos.Ocisti();
    b.ocjena = Ocjena();
    b.ocjena.pogodak.assign(TrenutnoPitanje().pojmovi.size(), false);
    b.ocjena.potrebno = TrenutnoPitanje().prag;
    b.poslano = 0;
}

static void ResetujOrka() {
    Borba& b = G.b;
    b.orkOffX = 0; b.orkOffY = 0; b.orkAlfa = 1; b.orkBljesak = 0; b.orkRot = 0; b.orkZamah = 0;
    b.oruzje = -110; b.orkDesno = false; b.smijeh = false;
}

static void ZapocniBorbu() {
    Borba& b = G.b;
    b = Borba();
    for (int k = 0; k < (int)G.pitanja.size(); k++) b.red.push_back(k);
    std::shuffle(b.red.begin(), b.red.end(), G.rng);
    // lives = how many mistakes make 45% of the questions (10 questions, 5 mistakes and you're dead)
    b.maxHp = std::max(1, (int)std::ceil(b.red.size() * PROCENAT_SMRTI - 1e-6));
    b.hp = b.maxHp;
    b.kamStart = -RAZMAK_NEPRIJATELJA; b.kamCilj = 0; b.kamera = b.kamStart;
    b.faza = Faza::HOD;
    ResetujOrka();
    NovoPitanje();
    G.ekran = Ekran::BORBA;
    G.cestice.clear(); G.natpisi.clear();
}

static void SljedeciNeprijatelj() {
    Borba& b = G.b;
    bool bioBoss = JeBoss();
    b.i++;
    b.t = 0;
    if (b.skokNaBossa) {   // dev: skip the rest of the enemies
        b.skokNaBossa = false;
        if (!bioBoss && BrojFazaBossa() > 0) b.i = std::max(b.i, PrviBoss());
    }
    if (b.i >= (int)b.red.size()) {
        b.faza = Faza::POBJEDA;
        G.stat.pobjede++;
        b.noviNivoOtkljucan = !G.stat.zavrsenNivo[G.nivo] && G.nivo + 1 < BROJ_NIVOA;
        b.andjeoOtkljucan = !G.stat.zavrsenNivo[G.nivo] && G.nivo == 2;   // beat the dungeon, so the volcano and the angel unlock
        G.stat.zavrsenNivo[G.nivo] = true;
        if (b.sveSavrseno) G.stat.perfekcionista = true;
        SacuvajStat();
        Zvuk::Pusti(Zvuk::POBJEDA);
        return;
    }
    NovoPitanje();
    ResetujOrka();
    if (bioBoss) {   // next phase of the same boss, no walking
        b.faza = Faza::PITANJE;
        Natpisi(Fmt(L("FAZA %d / %d", "PHASE %d / %d"), FazaBossa() + 1, BrojFazaBossa()), { 160, 60 }, CRVENA, &F.veliki);
        return;
    }
    b.kamStart = b.kamera;
    b.kamCilj = b.kamera + RAZMAK_NEPRIJATELJA;
    b.faza = Faza::HOD;
    b.hpPrikaz = 1;
}

// XP: you get it per question. Beating levels 1 to 3 perfectly gives you exactly enough to reach the
// last armor's level (150), and every knight level costs the same, so a third of the questions gets
// you a third of the way. The amount per question comes from how many questions are in the files.
// The save remembers your best score on every question, so you only get XP for improving (stuff you
// actually learned). No improvement, like a question you already nailed, gives 1 XP.
static int XpPoPitanju() {
    int pitanja = 0;
    for (int k = 0; k < 3 && k < BROJ_NIVOA; k++) pitanja += (int)NIVOI[k].pitanja.size();
    int ciljniNivo = 1;
    for (int o = 0; o < Grafika::BROJ_OKLOPA; o++) ciljniNivo = std::max(ciljniNivo, Grafika::OKLOP_NIVO[o]);
    double potrebno = 0;
    for (int n = 1; n < ciljniNivo; n++) potrebno += XpZaNivo(n);
    return (int)std::ceil(potrebno / std::max(1, pitanja));
}

// a question's key in the save: level + question text (FNV-1a), so the same question in two levels counts twice
static unsigned int KljucPitanja(const Pitanje& p) {
    unsigned int h = 2166136261u;
    auto dodaj = [&](unsigned char c) { h ^= c; h *= 16777619u; };
    dodaj((unsigned char)('0' + G.nivo));
    for (char c : p.tekst) dodaj((unsigned char)c);
    return h;
}

// XP for an answer is the gap between this score and your best one on this question
// (both sides get rounded up, so the pieces always add up to exactly the full amount)
static int XpZaOdgovor(const Ocjena& o, int ukupno, bool* napredak) {
    int posto = ukupno > 0 ? (int)std::round(100.0 * o.pogodjeno / ukupno) : 100;
    unsigned int kljuc = KljucPitanja(TrenutnoPitanje());
    int najbolje = G.stat.najbolje.count(kljuc) ? G.stat.najbolje[kljuc] : 0;
    *napredak = posto > najbolje;
    if (!*napredak) return 1;
    double x = XpPoPitanju();
    G.stat.najbolje[kljuc] = posto;
    int xp = (int)std::ceil(x * posto / 100.0 - 1e-9) - (int)std::ceil(x * najbolje / 100.0 - 1e-9);
    return std::max(1, xp);
}

static void ZapocniNapad(Faza f) {
    Borba& b = G.b;
    b.faza = f; b.t = 0; b.udareno = false; b.xpDodan = false; b.prasina = false;
    Zvuk::Pusti(Zvuk::MAC);
}

static void ZapocniNapadOrka(bool nastavi) {
    Borba& b = G.b;
    b.faza = Faza::ORK_NAPADA; b.t = 0; b.udareno = false; b.prasina = false;
    b.sveSavrseno = false;
    b.nastaviPoslije = nastavi;
}

static void PosaljiRecenicu() {
    Borba& b = G.b;
    const Pitanje& p = TrenutnoPitanje();
    if (Prazno(b.unos.tekst)) {
        if (b.poslano == 0) { b.tresenjeUnosa = 1; Zvuk::Pusti(Zvuk::PROMASAJ); return; }
        // final blow
        if (b.ocjena.tacno) ZapocniNapad(Faza::NAPAD);
        else ZapocniNapadOrka(false);
        return;
    }
    Ocjena o = OcijeniOdgovor(p, b.unos.tekst);
    b.noviPogoci = 0;
    for (size_t k = 0; k < o.pogodak.size() && k < b.ocjena.pogodak.size(); k++)
        if (o.pogodak[k] && !b.ocjena.pogodak[k]) { b.ocjena.pogodak[k] = true; b.ocjena.pogodjeno++; b.noviPogoci++; }
    b.ocjena.tacno = b.ocjena.pogodjeno >= b.ocjena.potrebno && b.ocjena.potrebno > 0;
    bool prviOdgovor = b.poslano == 0;
    b.poslano++;
    b.unos.Ocisti();
    if (b.ocjena.pogodjeno >= (int)p.pojmovi.size() || (prviOdgovor && o.tacno)) ZapocniNapad(Faza::NAPAD);
    else if (b.noviPogoci > 0) ZapocniNapad(Faza::DJELIMICNO);
    else {
        Natpisi(L("NIŠTA NOVO!", "NOTHING NEW!"), { VITEZ_X + 10, Grafika::TLO_Y - 46 }, CRVENA);
        ZapocniNapadOrka(true);
    }
}

// dust under the feet (landing, hitting the ground)
static void Prasina(float x, int n = 8) {
    for (int k = 0; k < n; k++) {
        float s = k % 2 ? 1.0f : -1.0f;
        G.cestice.push_back({ { x + Rnd(-4, 4), Grafika::TLO_Y - 1 }, { s * Rnd(10, 35), Rnd(-18, -6) }, 0.45f, 0.45f,
                              { 214, 190, 150, 255 }, Rnd(1.5f, 2.5f), 30 });
    }
}

// Dev mode, only on save III named "dev".
static bool DevRezim() {
    std::string ime = Trim(G.stat.ime);
    for (char& c : ime) c = (char)std::tolower((unsigned char)c);
    return G.slot == 2 && ime == "dev";
}

// dev: 100% damage, every key point at once so the enemy goes down right away
static void DevUdarac() {
    Borba& b = G.b;
    b.noviPogoci = 0;
    for (size_t k = 0; k < b.ocjena.pogodak.size(); k++)
        if (!b.ocjena.pogodak[k]) { b.ocjena.pogodak[k] = true; b.ocjena.pogodjeno++; b.noviPogoci++; }
    b.ocjena.tacno = true;
    b.poslano++;
    ZapocniNapad(Faza::NAPAD);
}

// the dev "boss fight" command (caps and spaces don't matter)
static bool DevKomandaBoss(const std::string& tekst) {
    std::string s;
    for (char c : tekst) if (std::isalpha((unsigned char)c)) s += (char)std::tolower((unsigned char)c);
    return s == "bossfight";
}

// how far the knight dashes to the enemy (the angel stays put and shoots a beam instead)
static float Zalet() { return Grafika::JeAndjeo() ? 0.0f : 125.0f; }

static bool ZaletViteza(Borba& b) {
    if (Grafika::JeAndjeo()) {   // light gathers for 0.2s, then the beam, the hit lands at 0.26s
        b.vitezOff = 0;
        b.vitezY = b.t < 0.2f ? -std::sin(PI * b.t / 0.2f) * 2 : 0;
        if (b.t >= 0.2f && b.t < 0.32f) {
            b.zamah = 1;
            if (!b.udareno && b.t >= 0.26f) { b.udareno = true; return true; }
        }
        return false;
    }
    if (b.t < 0.2f) {
        float k = b.t / 0.2f;
        b.vitezOff = 125 * IzlazBrzo(k);
        b.vitezY = -std::sin(PI * k) * 6;
        b.macUgao = Lerp(MAC_MIR, -170, k);
    } else if (b.t < 0.32f) {
        float k = (b.t - 0.2f) / 0.12f;
        b.vitezOff = 125; b.vitezY = 0;
        if (!b.prasina) { b.prasina = true; Prasina(VITEZ_X + 125); }
        b.macUgao = Lerp(-170, 40, k);
        b.zamah = 1;
        if (!b.udareno && b.t >= 0.26f) { b.udareno = true; return true; }
    }
    return false;
}

static void AzurirajBorbu(float dt) {
    Borba& b = G.b;
    b.vitezBljesak = std::max(0.0f, b.vitezBljesak - dt * 4);
    b.tresenjeUnosa = std::max(0.0f, b.tresenjeUnosa - dt * 2.5f);
    if (b.faza != Faza::NAPAD) b.orkBljesak = std::max(0.0f, b.orkBljesak - dt * 4);
    if (b.i < (int)b.red.size()) {
        float cilj = ZivotNeprijatelja();
        b.hpPrikaz = b.hpPrikaz > cilj ? std::max(cilj, b.hpPrikaz - dt * 1.2f) : cilj;
    }

    if (b.pauza) {
        if (Ulaz::Pritisnuto(KEY_UP) || Ulaz::Pritisnuto(KEY_DOWN) || Ulaz::Pritisnuto(KEY_W) || Ulaz::Pritisnuto(KEY_S)) {
            b.pauzaIzbor = 1 - b.pauzaIzbor; Zvuk::Pusti(Zvuk::MENI);
        }
        Vector2 m = Ulaz::Mis();
        for (int k = 0; k < 2; k++) {
            Rectangle rr = { 440, 330.0f + k * 70, 400, 56 };
            if (CheckCollisionPointRec(m, rr)) { b.pauzaIzbor = k; if (Ulaz::Klik()) { if (k == 0) b.pauza = false; else { SacuvajStat(); G.ekran = Ekran::MENI; } } }
        }
        if (Ulaz::Pritisnuto(KEY_ESCAPE)) b.pauza = false;
        if (Ulaz::Pritisnuto(KEY_ENTER)) {
            if (b.pauzaIzbor == 0) b.pauza = false;
            else { SacuvajStat(); G.ekran = Ekran::MENI; }
        }
        return;
    }

    G.stat.sekundeIgre += dt;
    b.t += dt;
    float gx = ORK_X + b.orkOffX, gy = Grafika::TLO_Y - 10 * Grafika::SkalaOrka(VrstaOrka());
    switch (b.faza) {
    case Faza::HOD: {
        float k = Glatko(b.t / 2.0f);
        b.kamera = Lerp(b.kamStart, b.kamCilj, k);
        if (Ulaz::Pritisnuto(KEY_ESCAPE)) { b.pauza = true; b.pauzaIzbor = 0; }
        if (b.t >= 2.0f) { b.faza = Faza::PITANJE; b.t = 0; }
        break;
    }
    case Faza::PITANJE: {
        b.macUgao = MAC_MIR;
        if (Ulaz::Pritisnuto(KEY_ESCAPE)) { b.pauza = true; b.pauzaIzbor = 0; break; }
        // dev: SPACE in an empty box kills the enemy, otherwise it's a normal space so you can type "boss fight"
        if (DevRezim() && Prazno(b.unos.tekst) && Ulaz::Pritisnuto(KEY_SPACE)) { DevUdarac(); break; }
        ObradiUnos(b.unos);
        if (PosaljiOdgovor()) {
            if (DevRezim() && DevKomandaBoss(b.unos.tekst)) {   // dev: kill this one and go straight to the boss
                b.unos.Ocisti();
                b.skokNaBossa = true;
                Natpisi(L("NA BOSSA!", "TO THE BOSS!"), { 160, 60 }, { 255, 160, 60, 255 }, &F.veliki);
                DevUdarac();
            } else PosaljiRecenicu();
        }
        break;
    }
    case Faza::DJELIMICNO: {   // a sentence wounded the enemy, the fight goes on
        if (ZaletViteza(b)) {
            b.orkBljesak = 1;
            G.tresenje = 4;
            Zvuk::Pusti(Zvuk::UDARAC);
            Cestice({ gx, gy }, 12, { 255, 240, 160, 255 }, 70, 120);
            int posto = (int)std::round(100.0f * b.noviPogoci / std::max(1, BrojPojmova()));
            Natpisi(Fmt("-%d%%", posto), { gx, gy - 26 }, ZLATNA);
        }
        if (b.t >= 0.32f) {
            b.zamah = std::max(0.0f, 1 - (b.t - 0.32f) / 0.2f);
            float pos = std::max(0.0f, 1 - (b.t - 0.32f) / 0.35f);
            b.orkOffX = 6 * pos;                                        // enemy stumbles back
            b.orkRot = 14 * pos * pos;                                  // and leans
            float k = Glatko((b.t - 0.45f) / 0.35f);
            b.vitezOff = Zalet() * (1 - k);
            b.macUgao = Lerp(40, MAC_MIR, k);
            if (b.t >= 0.85f) { b.faza = Faza::PITANJE; b.t = 0; b.orkOffX = 0; b.orkRot = 0; b.zamah = 0; }
        }
        break;
    }
    case Faza::NAPAD: {        // final blow, the enemy falls (a boss only staggers until the last phase)
        int ukupno = BrojPojmova();
        bool savrseno = b.ocjena.pogodjeno >= ukupno;
        if (!savrseno) b.sveSavrseno = false;
        bool ubija = !JeBoss() || ZadnjaFazaBossa();
        if (b.t < 0.32f) {
            if (ZaletViteza(b)) {
                b.orkBljesak = 1;
                G.tresenje = savrseno ? 8 : 5;
                Zvuk::Pusti(Zvuk::UDARAC);
                Cestice({ gx, gy }, savrseno ? 36 : 22, { 255, 240, 160, 255 }, 90, 120);
                Cestice({ gx, gy }, 10, { 120, 200, 80, 255 }, 70, 160);
                const char* t = savrseno ? L("SAVRŠENO!", "PERFECT!")
                              : !JeBoss() ? L("POGODAK!", "HIT!")
                              : ubija ? L("VOJSKOVOĐA PAO!", "WARLORD DEFEATED!") : L("FAZA SAVLADANA!", "PHASE CLEARED!");
                Natpisi(t, { gx, gy - 30 }, ZLATNA);
                b.tacnih++;
                if (ubija) { G.stat.ubijeno++; b.ubijeno++; }
            }
        } else {
            b.zamah = std::max(0.0f, 1 - (b.t - 0.32f) / 0.2f);
            b.orkBljesak = b.t < 0.6f ? 1.0f : std::max(0.0f, b.orkBljesak - dt * 4);
            if (ubija) {   // enemy flies back, lands on its back in a puff of dust, then vanishes
                float k = IzlazBrzo((b.t - 0.32f) / 0.35f);
                b.orkOffX = 16 * k;
                b.orkRot = 90 * k;
                if (b.t >= 0.67f && b.t - dt < 0.67f) { Prasina(ORK_X + 16 + 8, 12); G.tresenje = 3; }
                b.orkAlfa = std::max(0.0f, 1 - (b.t - 0.8f) / 0.4f);
                if (b.t >= 0.85f && b.t - dt < 0.85f) {   // vanishes in smoke, skeletons fall apart into bones, demons burn into ash
                    int por = PorodicaNeprijatelja(VrstaOrka());
                    Vector2 tl = { ORK_X + 24, Grafika::TLO_Y - 4 };
                    Cestice(tl, 26, { 200, 200, 190, 255 }, 40, -20, 2.5f);
                    if (por == 1) { Cestice(tl, 16, { 230, 224, 204, 255 }, 70, 260, 2.0f); Cestice(tl, 8, { 170, 162, 140, 255 }, 60, 260, 1.5f); }
                    if (por == 2) { Cestice(tl, 22, { 255, 150, 40, 255 }, 50, -60, 1.5f); Cestice(tl, 14, { 70, 60, 60, 255 }, 30, -30, 2.5f); }
                    if (por == 0 && G.nivo == 1) Cestice(tl, 12, { 110, 80, 50, 255 }, 50, 220, 1.5f);   // mud
                }
            } else {       // a boss just staggers and leans
                float pos = std::max(0.0f, 1 - (b.t - 0.32f) / 0.5f);
                b.orkOffX = 10 * pos;
                b.orkRot = 18 * pos * pos;
            }
            if (b.t > 0.5f) {
                float k = Glatko((b.t - 0.5f) / 0.35f);
                b.vitezOff = Zalet() * (1 - k);
                b.macUgao = Lerp(40, MAC_MIR, k);
            }
            if (!b.xpDodan && b.t >= 0.95f && DevRezim()) b.xpDodan = true;   // the dev save gets no XP
            if (!b.xpDodan && b.t >= 0.95f) {
                b.xpDodan = true;
                bool napredak = false;
                int xp = XpZaOdgovor(b.ocjena, ukupno, &napredak);
                int posto = ukupno > 0 ? (int)std::round(100.0f * b.ocjena.pogodjeno / ukupno) : 100;
                b.xpOvaj += xp;
                int stariNivo = G.stat.nivo;
                bool nivo = DodajXp(G.stat, xp);
                if (napredak) Natpisi(Fmt("+%d XP (%d%%)", xp, posto), { VITEZ_X + 10, Grafika::TLO_Y - 40 }, { 120, 220, 255, 255 });
                else Natpisi(Fmt(L("+%d XP (već naučeno)", "+%d XP (already learned)"), xp), { VITEZ_X + 10, Grafika::TLO_Y - 40 }, { 160, 170, 190, 255 });
                if (nivo) { G.noviNivo = 2.5f; Zvuk::Pusti(Zvuk::NIVO); Natpisi(L("NOVI NIVO!", "LEVEL UP!"), { 160, 50 }, ZLATNA, &F.veliki); }
                else Zvuk::Pusti(Zvuk::XP);
                for (int o = 1; o < Grafika::BROJ_OKLOPA; o++)
                    if (Grafika::OKLOP_NIVO[o] > 0 && stariNivo < Grafika::OKLOP_NIVO[o] && G.stat.nivo >= Grafika::OKLOP_NIVO[o])
                        Natpisi(Fmt(L("NOVI OKLOP: %s!", "NEW ARMOR: %s!"), ImeOklopa(o)), { 160, 68 }, { 140, 210, 255, 255 });
                SacuvajStat();
            }
            if (b.t >= 1.7f) SljedeciNeprijatelj();
        }
        break;
    }
    case Faza::ORK_NAPADA: {
        // Every enemy attacks in its own way (Grafika::NapadNeprijatelja):
        //  spell (shaman, witch doctor, necromancer, fire demon): charges a ball and throws it at you
        //  arrow (archer, blowgunner): aims and shoots an arrow or poison dart
        //  thrust (spearman, hunter, skeleton knight, imp): pulls back, then rushes in and stabs
        //  leap (everyone else): winds up, jumps at the knight and smashes down
        int vrsta = VrstaOrka();
        Grafika::Napad napad = Grafika::NapadNeprijatelja(vrsta);
        float sk = Grafika::SkalaOrka(vrsta);
        auto pogodi = [&](Color krv) {
            b.udareno = true;
            b.hp--;
            b.netacnih++;
            b.vitezBljesak = 1;
            G.tresenje = 9;
            G.crveno = 1.2f;
            Zvuk::Pusti(Zvuk::BOL);
            Cestice({ VITEZ_X + 4, Grafika::TLO_Y - 12 }, 18, krv, 80, 160);
            Natpisi(L("-1 ŽIVOT", "-1 LIFE"), { VITEZ_X, Grafika::TLO_Y - 34 }, CRVENA);
        };
        b.orkZamah = std::max(0.0f, b.orkZamah - dt * 5);
        float kraj;   // when the enemy is back in place
        if (napad == Grafika::Napad::CAROLIJA) {
            kraj = 1.0f;
            Vector2 vrh = Grafika::IzvorProjektila(vrsta, ORK_X, Grafika::TLO_Y);
            Color boja = Grafika::BojaCarolije(vrsta);
            if (b.t < 0.35f) {                                  // raises the staff and charges the ball
                float k = Glatko(b.t / 0.35f);
                b.oruzje = Lerp(-110, -90, k);
                b.orkOffY = -2 * k;
                if (std::fmod(b.t, 0.05f) < dt)
                    G.cestice.push_back({ { vrh.x + Rnd(-7, 7), vrh.y + Rnd(-7, 7) }, { 0, -10 }, 0.35f, 0.35f, boja, 1.5f, 0 });
            } else if (b.t < 0.65f) {                           // the spell flies (drawn in CrtajBorbuSvijet)
                b.oruzje = -90;
                if (!b.udareno && b.t >= 0.62f) {
                    pogodi(boja);
                    Cestice({ VITEZ_X + 2, Grafika::TLO_Y - 9 }, 24, boja, 90, 0);
                }
            } else {
                float k = Glatko((b.t - 0.65f) / 0.35f);
                b.oruzje = Lerp(-90, -110, k);
                b.orkOffY = -2 * (1 - k);
            }
        } else if (napad == Grafika::Napad::STRIJELA) {
            kraj = 0.95f;
            if (b.t < 0.3f) {                                   // aims (bow or blowgun level)
                b.oruzje = Lerp(-110, -180, Glatko(b.t / 0.3f));
                b.orkOffX = 3 * Glatko(b.t / 0.3f);
            } else if (b.t < 0.5f) {                            // the arrow flies (drawn in CrtajBorbuSvijet)
                b.oruzje = -180;
                if (!b.udareno && b.t >= 0.48f) { pogodi(vrsta == 9 ? Color{ 120, 230, 90, 255 } : Color{ 230, 60, 60, 255 }); }
            } else {
                float k = Glatko((b.t - 0.5f) / 0.4f);
                b.oruzje = Lerp(-180, -110, k);
                b.orkOffX = 3 * (1 - k);
            }
        } else if (napad == Grafika::Napad::UBOD) {
            kraj = 0.95f;
            if (b.t < 0.22f) {                                  // pulls back and lowers the spear
                float k = Glatko(b.t / 0.22f);
                b.orkOffX = 8 * k;
                b.oruzje = Lerp(-110, -180, k);
            } else if (b.t < 0.4f) {                            // rushes in and stabs
                float k = IzlazBrzo((b.t - 0.22f) / 0.18f);
                b.orkOffX = Lerp(8, -112, k);
                b.oruzje = -180;
                if (!b.udareno && b.t >= 0.37f) { pogodi({ 230, 60, 60, 255 }); Prasina(VITEZ_X + 4, 5); }
            } else if (b.t < 0.55f) {
                b.orkOffX = -112;
            } else {                                            // hops back
                float k = Glatko((b.t - 0.55f) / 0.4f);
                b.orkOffX = Lerp(-112, 0, k);
                b.orkOffY = -std::sin(PI * std::min(1.0f, k)) * 10;
                b.oruzje = Lerp(-180, -110, k);
            }
        } else {
            kraj = 0.95f;
            if (b.t < 0.2f) {                                   // winds up and crouches
                float k = Glatko(b.t / 0.2f);
                b.orkOffX = 5 * k;
                b.orkOffY = 1 * k;
                b.oruzje = Lerp(-110, -45, k);
            } else if (b.t < 0.42f) {                           // leaps at the knight
                float k = (b.t - 0.2f) / 0.22f;
                b.orkOffX = Lerp(5, -135, IzlazBrzo(k));
                b.orkOffY = -std::sin(PI * k) * 28;
                b.oruzje = -45;
            } else if (b.t < 0.52f) {                           // smashes down
                float k = (b.t - 0.42f) / 0.1f;
                b.orkOffX = -135; b.orkOffY = 0;
                b.oruzje = Lerp(-45, -210, k);
                b.orkZamah = 1;
                if (!b.udareno && b.t >= 0.47f) {
                    pogodi({ 230, 60, 60, 255 });
                    Prasina(ORK_X - 135, JakUdarac(vrsta) ? 16 : 10);
                    if (JakUdarac(vrsta)) G.tresenje = 12;
                }
            } else {                                            // jumps back
                float k = Glatko((b.t - 0.52f) / 0.43f);
                b.orkOffX = Lerp(-135, 0, k);
                b.orkOffY = -std::sin(PI * std::min(1.0f, k)) * 22;
                b.oruzje = Lerp(-210, -110, k);
                if (k >= 1 && !b.prasina) { b.prasina = true; Prasina(ORK_X, 6); }
            }
            (void)sk;
        }
        if (b.t >= kraj) {
            b.orkOffX = 0; b.orkOffY = 0; b.oruzje = -110;
            if (!b.smijeh) {
                b.smijeh = true;
                int por = PorodicaNeprijatelja(vrsta);
                const char* smijeh = vrsta == 2 || vrsta == 8 ? "HEH HEH HEH!"
                                   : por == 1 ? L("KRC KRC KRC!", "CLACK CLACK!")
                                   : vrsta == 18 ? "GRRRRR!"
                                   : por == 2 ? "MUAHAHA!" : "HAR HAR HAR!";
                Color bojaSmijeha = por == 1 ? Color{ 220, 214, 190, 255 } : por == 2 ? Color{ 255, 140, 60, 255 } : Color{ 160, 230, 110, 255 };
                Natpisi(smijeh, { ORK_X, Grafika::TLO_Y - 40 }, bojaSmijeha);
                Zvuk::Pusti(Zvuk::PROMASAJ);
            }
        }
        if (b.t >= 1.5f) {
            b.t = 0;
            b.faza = (b.nastaviPoslije && b.hp > 0) ? Faza::PITANJE : Faza::OTKRIVANJE;
        }
        break;
    }
    case Faza::OTKRIVANJE: {
        if (b.t > 0.3f && Ulaz::Pritisnuto(KEY_ENTER)) {
            b.t = 0;
            if (b.hp <= 0) {
                b.faza = Faza::SMRT;
                G.stat.smrti++;
                if (G.stat.hardcore) G.stat.pao = true;   // hardcore: this knight is gone for good
                SacuvajStat();
                Zvuk::Pusti(Zvuk::SMRT);
            } else if (JeBoss() && !ZadnjaFazaBossa()) {
                SljedeciNeprijatelj();                     // the boss stays, on to the next phase
            } else {
                b.faza = Faza::BIJEG;
                b.orkDesno = true;
            }
        }
        break;
    }
    case Faza::BIJEG: {
        b.orkOffX += 260 * dt;
        b.orkOffY = -std::fabs(std::sin(b.t * 14)) * 4;
        if (b.t >= 0.9f) SljedeciNeprijatelj();
        break;
    }
    case Faza::SMRT: {
        b.vitezRot = -90 * IzlazBrzo(b.t / 0.6f);
        if (b.t > 1.2f && Ulaz::Pritisnuto(KEY_ENTER)) {
            Zvuk::Pusti(Zvuk::MENI);
            if (G.stat.pao) OtvoriEkranSpremanja();
            else G.ekran = Ekran::MENI;
        }
        break;
    }
    case Faza::POBJEDA: {
        b.macUgao = -90 + std::sin(b.t * 6) * 12;
        if (b.t < 3.0f && std::fmod(b.t, 0.1f) < dt) {   // confetti
            for (int k = 0; k < 5; k++) {
                Color c = { (unsigned char)Rnd(120, 255), (unsigned char)Rnd(120, 255), (unsigned char)Rnd(80, 255), 255 };
                G.cestice.push_back({ { Rnd(0, 320), -4 }, { Rnd(-15, 15), Rnd(20, 45) }, 4.5f, 4.5f, c, 2, 8 });
            }
        }
        if (b.t > 1.0f && Ulaz::Pritisnuto(KEY_ENTER)) { G.ekran = Ekran::MENI; Zvuk::Pusti(Zvuk::MENI); }
        break;
    }
    }
}

static float OrkEkranX() { return G.b.kamCilj + ORK_X - G.b.kamera + G.b.orkOffX; }

static void CrtajBorbuSvijet() {
    Borba& b = G.b;
    Grafika::CrtajDolinu(b.kamera, G.vrijeme);
    bool hoda = b.faza == Faza::HOD && b.t < 2.0f;
    int okvir = hoda ? ((int)(b.t / 0.16f)) % 2 : 0;
    float bob = (!hoda && b.faza != Faza::SMRT) ? std::round(std::sin(G.vrijeme * 3) * 0.6f) : 0;
    float vy = Grafika::TLO_Y;
    if (b.faza == Faza::POBJEDA) vy -= std::fabs(std::sin(b.t * 5)) * 6;
    bool orkVidljiv = !(b.faza == Faza::POBJEDA) && b.i < (int)b.red.size();
    if (orkVidljiv) {
        int vrsta = VrstaOrka();
        bool miruje = b.faza == Faza::PITANJE || b.faza == Faza::OTKRIVANJE || b.faza == Faza::HOD;
        float gb = miruje ? std::round(std::sin(G.vrijeme * 4 + 1) * 0.8f) : 0;
        float njihanje = b.faza != Faza::ORK_NAPADA ? std::sin(G.vrijeme * 2.2f) * 6 : 0;   // weapon sways a little
        float ox = OrkEkranX(), oy = Grafika::TLO_Y + b.orkOffY + gb;
        Grafika::CrtajOrka(ox, oy, b.oruzje + njihanje, b.orkBljesak, b.orkAlfa, b.orkDesno, vrsta, b.orkRot);
        float sk = Grafika::SkalaOrka(vrsta);
        if (b.orkZamah > 0)   // arc of the enemy's swing
            { Vector2 ru = Grafika::RukaNeprijatelja(vrsta, ox, oy); Grafika::CrtajZamahOrka(ru.x, ru.y, -210, -45, 13 * sk, b.orkZamah); }
        // spell (ball with a trail) or arrow flying from the enemy to the knight
        if (b.faza == Faza::ORK_NAPADA) {
            Grafika::Napad napad = Grafika::NapadNeprijatelja(vrsta);
            Vector2 a = Grafika::IzvorProjektila(vrsta, ox, Grafika::TLO_Y), c = { VITEZ_X + 2, Grafika::TLO_Y - 9 };
            if (napad == Grafika::Napad::CAROLIJA && b.t >= 0.35f && b.t < 0.62f)
                Grafika::CrtajProjektil(vrsta, a, c, (b.t - 0.35f) / 0.27f);
            if (napad == Grafika::Napad::STRIJELA && b.t >= 0.3f && b.t < 0.48f)
                Grafika::CrtajProjektil(vrsta, a, c, (b.t - 0.3f) / 0.18f);
        }
    }
    // the knight jerks back when he gets hit
    float trzaj = b.faza == Faza::SMRT ? 0 : b.vitezBljesak;
    float vx = VITEZ_X + b.vitezOff - 8 * trzaj, vrot = b.vitezRot - 14 * trzaj;
    // dash: see through "ghosts" behind the knight
    if ((b.faza == Faza::NAPAD || b.faza == Faza::DJELIMICNO) && b.t < 0.3f && b.vitezOff > 8)
        for (int k = 3; k >= 1; k--)
            Grafika::CrtajViteza(VITEZ_X + b.vitezOff * (1 - 0.22f * k), vy + b.vitezY * (1 - 0.22f * k), 0, b.macUgao, 0, 0, 0.36f - 0.1f * k);
    Grafika::CrtajViteza(vx, vy + bob + b.vitezY, okvir, b.macUgao, b.vitezBljesak, vrot);
    if (Grafika::JeAndjeo()) {   // light gathers in his hand, then the beam hits the enemy
        Vector2 ruka = Grafika::RukaViteza(vx, vy + bob + b.vitezY);
        if ((b.faza == Faza::NAPAD || b.faza == Faza::DJELIMICNO) && b.t < 0.2f) Grafika::CrtajNabojAndjela(ruka, b.t / 0.2f);
        if (b.zamah > 0 && b.i < (int)b.red.size())
            Grafika::CrtajSvjetlosniZrak(ruka, { OrkEkranX(), Grafika::TLO_Y - 10 * Grafika::SkalaOrka(VrstaOrka()) }, b.zamah);
    } else if (b.zamah > 0) Grafika::CrtajZamah(VITEZ_X + b.vitezOff + 3.5f, Grafika::TLO_Y - 7.5f, -150, 40, b.zamah);
}

static void CrtajZivote() {
    Borba& b = G.b;
    if (b.maxHp <= 24) {
        float sk = b.maxHp <= 10 ? 4.0f : 2.0f;
        for (int k = 0; k < b.maxHp; k++) {
            int red = b.maxHp <= 10 ? 0 : k / 12, kol = b.maxHp <= 10 ? k : k % 12;
            Grafika::CrtajSrce(24.0f + kol * 11 * sk, 18.0f + red * 18, sk, k < b.hp);
        }
    } else {   // too many questions for hearts, so a bar
        DrawRectangle(24, 18, 300, 26, { 20, 16, 30, 255 });
        DrawRectangle(28, 22, (int)(292.0f * b.hp / b.maxHp), 18, { 220, 40, 50, 255 });
        TekstSjena(F.tekstMali, Fmt("%d / %d", b.hp, b.maxHp), 336, 18, KREM, 2);
    }
}

// enemy name and health bar above its head
static void CrtajZivotOrka() {
    Borba& b = G.b;
    if (b.faza == Faza::POBJEDA || b.faza == Faza::BIJEG || b.i >= (int)b.red.size() || b.orkAlfa <= 0) return;
    int v = VrstaOrka();
    float sk = Grafika::SkalaOrka(v), a = b.orkAlfa;
    float cx = (OrkEkranX() - b.orkOffX) * 4;   // the bar stays put even when the enemy jumps at you
    float w = JeBoss() ? 240.0f : 150.0f, h = 14;
    float y = (Grafika::TLO_Y - 20 * sk - 7) * 4 - h;
    float x = cx - w / 2;
    TekstCentar(F.tekstMali, ImeOrka(v), cx, y - 28, Fade(JeBoss() ? CRVENA : KREM, a));
    DrawRectangle((int)x - 3, (int)y - 3, (int)w + 6, (int)h + 6, Fade({ 20, 16, 30, 255 }, a));
    DrawRectangle((int)x, (int)y, (int)w, (int)h, Fade({ 70, 20, 26, 255 }, a));
    DrawRectangle((int)x, (int)y, (int)(w * b.hpPrikaz), (int)h, Fade({ 220, 50, 50, 255 }, a));
    DrawRectangle((int)x, (int)y, (int)(w * b.hpPrikaz), 3, Fade({ 255, 150, 150, 255 }, a));
    if (JeBoss())
        for (int k = 1; k < BrojFazaBossa(); k++)
            DrawRectangle((int)(x + w * k / BrojFazaBossa()) - 1, (int)y, 2, (int)h, Fade({ 20, 16, 30, 255 }, a));
}

static void CrtajHud() {
    CrtajZivote();
    std::string b = G.b.faza == Faza::POBJEDA ? std::string(L(NIVOI[G.nivo].mapaBs, NIVOI[G.nivo].mapaEn))
        : JeBoss()
        ? Fmt(L("%s - FAZA %d / %d", "%s - PHASE %d / %d"), ImeOrka(VrstaOrka()), FazaBossa() + 1, BrojFazaBossa())
        : Fmt(L("NEPRIJATELJ %d / %d", "ENEMY %d / %d"), std::min(G.b.i + 1, BrojNeprijatelja()), BrojNeprijatelja());
    TekstCentar(F.mali, b, 640, 28, KREM);
    if (DevRezim()) TekstCentar(F.tekstMali, L("DEV REŽIM: SPACE ubija, \"boss fight\" vodi na bossa", "DEV MODE: SPACE kills, \"boss fight\" skips to boss"), 640, 52, { 255, 160, 60, 255 });
    CrtajXpTraku(870, 20, 386);
}

static void CrtajBorbuUI() {
    Borba& b = G.b;
    CrtajZivotOrka();
    CrtajHud();
    bool imaPitanje = b.faza == Faza::PITANJE || b.faza == Faza::NAPAD || b.faza == Faza::DJELIMICNO || b.faza == Faza::ORK_NAPADA;
    if (imaPitanje) {
        CrtajPitanje(TrenutnoPitanje().tekst, 80);
        std::string dno = Fmt(L("ENTER = udari (i rečenica po rečenica)   prazan ENTER = završni udarac   ESC = pauza   (treba %d od %d pojmova)",
                                "ENTER = strike (one sentence at a time is fine)   empty ENTER = final blow   ESC = pause   (need %d of %d key points)"),
                              TrenutnoPitanje().prag, BrojPojmova());
        std::string naslov = b.poslano > 0
            ? Fmt(L("TVOJ ODGOVOR - POGOĐENO %d / %d", "YOUR ANSWER - %d / %d HIT"), b.ocjena.pogodjeno, BrojPojmova())
            : std::string(L("TVOJ ODGOVOR:", "YOUR ANSWER:"));
        CrtajUnos(b.unos, b.faza == Faza::PITANJE, naslov, dno, b.tresenjeUnosa);
    }
    if (b.faza == Faza::HOD && b.t > 0.6f && b.i < (int)b.red.size()) {
        std::string t = JeBoss() ? Fmt(L("%s SE PRIBLIŽAVA!", "%s APPROACHES!"), ImeOrka(VrstaOrka()))
                                 : std::string(L("NEPRIJATELJ SE PRIBLIŽAVA...", "AN ENEMY APPROACHES..."));
        if (std::fmod(G.vrijeme, 0.8f) < 0.6f) TekstCentar(F.srednji, t, 640, 150, JeBoss() ? CRVENA : KREM);
        if (b.i == 0) {
            TekstCentar(F.mali, Fmt(L("Imaš %d života - svaki promašaj košta jedan!", "You have %d lives - every miss costs one!"), b.maxHp), 640, 200, ZLATNA);
            TekstCentar(F.mali, Fmt(L("NIVO %d: %s", "LEVEL %d: %s"), G.nivo + 1, L(NIVOI[G.nivo].mapaBs, NIVOI[G.nivo].mapaEn)), 640, 228, { 140, 210, 255, 255 });
        }
        if (JeBoss() && BrojFazaBossa() > 1)
            TekstCentar(F.mali, Fmt(L("Vojskovođa ima %d faze - svaka je jedno pitanje.", "The warlord has %d phases - one question each."), BrojFazaBossa()), 640, 200, ZLATNA);
    }
    if (b.faza == Faza::OTKRIVANJE)
        CrtajOdgovor(TrenutnoPitanje(), &b.ocjena, L("NETAČNO! TAČAN ODGOVOR JE:", "WRONG! THE CORRECT ANSWER IS:"), CRVENA,
                     b.hp <= 0 ? L("ENTER - nastavi...", "ENTER - continue...")
                     : (JeBoss() && !ZadnjaFazaBossa()) ? L("ENTER - sljedeća faza", "ENTER - next phase") : L("ENTER - sljedeći neprijatelj", "ENTER - next enemy"));
    if (b.faza == Faza::SMRT && b.t > 1.0f) {
        float a = Klamp((b.t - 1.0f) / 0.5f, 0, 1);
        DrawRectangle(0, 0, EKRAN_W, EKRAN_H, { 50, 0, 0, (unsigned char)(170 * a) });
        TekstCentar(F.naslov, LR("PAO SI U BOJU...", "PALA SI U BOJU...", "YOU HAVE FALLEN..."), 640, 190, Fade(CRVENA, a));
        TekstCentar(F.mali, Fmt(L("Orci su te udarili %d puta.", "The orcs struck you %d times."), b.netacnih), 640, 270, Fade(KREM, a));
        if (G.stat.pao)
            TekstCentar(F.mali, LR("HARDCORE: tvoj vitez je zauvijek pao. Spremanje postaje spomenik.", "HARDCORE: tvoja vitezica je zauvijek pala. Spremanje postaje spomenik.",
                                  "HARDCORE: your knight has fallen forever. The save becomes a memorial."), 640, 300, Fade(CRVENA, a));
        else
            TekstCentar(F.mali, L("Uči u vježbalištu i vrati se!", "Train in the training yard and come back!"), 640, 300, Fade(KREM, a));
        TekstCentar(F.srednji, Fmt(L("UBIJENO NEPRIJATELJA: %d", "ENEMIES SLAIN: %d"), b.ubijeno), 640, 360, Fade(KREM, a));
        TekstCentar(F.srednji, Fmt(L("OSVOJENO XP: %d", "XP GAINED: %d"), b.xpOvaj), 640, 405, Fade(KREM, a));
        TekstCentar(F.srednji, Fmt(L("UKUPNO SMRTI: %d", "TOTAL DEATHS: %d"), G.stat.smrti), 640, 450, Fade(KREM, a));
        if (b.t > 1.2f && Treperi())
            TekstCentar(F.mali, G.stat.pao ? L("ENTER - povratak na spremanja", "ENTER - back to save files") : L("ENTER - povratak u meni", "ENTER - back to menu"), 640, 530, ZLATNA);
    }
    if (b.faza == Faza::POBJEDA && b.t > 0.8f) {
        float a = Klamp((b.t - 0.8f) / 0.5f, 0, 1);
        DrawRectangle(0, 0, EKRAN_W, EKRAN_H, { 0, 0, 0, (unsigned char)(120 * a) });
        TekstCentar(F.naslov, L("POBJEDA!", "VICTORY!"), 640, 150, Fade(ZLATNA, a));
        const char* mapa = L(NIVOI[G.nivo].mapaBs, NIVOI[G.nivo].mapaEn);
        TekstCentar(F.mali, b.tacnih == (int)b.red.size() ? Fmt(LR("Porazio si sve neprijatelje: %s!", "Porazila si sve neprijatelje: %s!", "You defeated every enemy: %s!"), mapa)
                                                           : Fmt(LR("Prošao si: %s!", "Prošla si: %s!", "You made it through: %s!"), mapa), 640, 230, Fade(KREM, a));
        TekstCentar(F.srednji, Fmt(L("TAČNIH ODGOVORA: %d / %d", "CORRECT ANSWERS: %d / %d"), b.tacnih, (int)b.red.size()), 640, 300, Fade(KREM, a));
        TekstCentar(F.srednji, Fmt(L("OSVOJENO XP: %d", "XP GAINED: %d"), b.xpOvaj), 640, 345, Fade(KREM, a));
        TekstCentar(F.srednji, Fmt(L("PREOSTALO ŽIVOTA: %d / %d", "LIVES LEFT: %d / %d"), b.hp, b.maxHp), 640, 390, Fade(KREM, a));
        TekstCentar(F.srednji, Fmt(L("NIVO: %d", "LEVEL: %d"), G.stat.nivo), 640, 435, Fade(KREM, a));
        float py = 470;
        if (b.noviNivoOtkljucan) {
            const NivoPitanja& sl = NIVOI[G.nivo + 1];
            TekstCentar(F.mali, Fmt(L("OTKLJUČAN NOVI NIVO: %s", "NEW LEVEL UNLOCKED: %s"), L(sl.mapaBs, sl.mapaEn)), 640, py, Fade({ 140, 210, 255, 255 }, a));
            py += 24;
        }
        if (b.andjeoOtkljucan)
            TekstCentar(F.mali, L("NOVI OKLOP: ANĐEOSKI VITEZ!", "NEW ARMOR: ANGELIC KNIGHT!"), 640, py, Fade(ZLATNA, a));
        if (b.t > 1.0f && Treperi()) TekstCentar(F.mali, L("ENTER - povratak u meni", "ENTER - back to menu"), 640, 540, ZLATNA);
    }
    if (b.pauza) {
        DrawRectangle(0, 0, EKRAN_W, EKRAN_H, { 0, 0, 0, 150 });
        Panel({ 400, 220, 480, 260 });
        TekstCentar(F.veliki, L("PAUZA", "PAUSED"), 640, 252, ZLATNA);
        const char* op[] = { L("NASTAVI", "RESUME"), L("NAPUSTI BORBU", "LEAVE BATTLE") };
        for (int k = 0; k < 2; k++) {
            bool s = b.pauzaIzbor == k;
            std::string t = s ? std::string("> ") + op[k] + " <" : op[k];
            TekstCentar(F.srednji, t, 640, 340.0f + k * 70, s ? ZLATNA : KREM);
        }
    }
}

// ================================================================== TRAINING YARD
// New try on the current question: the key points from all the sentences start from zero again
static void NoviPokusajVjezbe() {
    Vjezba& v = G.v;
    const Pitanje& p = G.pitanja[v.i];
    v.ocjena = Ocjena();
    v.ocjena.pogodak.assign(p.pojmovi.size(), false);
    v.ocjena.potrebno = p.prag;
    v.poslano = 0;
    v.zadnjiTekst.clear();
}

static void ZapocniVjezbu() {
    G.v = Vjezba();
    NoviPokusajVjezbe();
    G.ekran = Ekran::VJEZBA;
    G.cestice.clear(); G.natpisi.clear();
}

static void PromijeniPitanjeVjezbe(int smjer) {
    Vjezba& v = G.v;
    int n = (int)G.pitanja.size();
    v.i = ((v.i + smjer) % n + n) % n;
    v.unos.Ocisti(); v.rezultat = false; v.zaviri = false;
    NoviPokusajVjezbe();
    Zvuk::Pusti(Zvuk::MENI);
}

static bool PotpunoTacno(const Ocjena& o, const Pitanje& p) { return o.pogodjeno >= (int)p.pojmovi.size(); }

static void AzurirajVjezbu(float dt) {
    Vjezba& v = G.v;
    bool strogi = G.stat.strogiRezim;
    G.stat.sekundeIgre += dt;
    float acc = -70 * v.nagib - 4.5f * v.brzina;      // the dummy wobbles like a spring
    v.brzina += acc * dt; v.nagib += v.brzina * dt;
    v.lutkaBljesak = std::max(0.0f, v.lutkaBljesak - dt * 4);
    v.tresenjeUnosa = std::max(0.0f, v.tresenjeUnosa - dt * 2.5f);

    if (v.napad) {
        v.t += dt;
        if (v.t < 0.2f) { float k = v.t / 0.2f; v.vitezOff = Zalet() * IzlazBrzo(k); v.macUgao = Lerp(MAC_MIR, -170, k); }
        else if (v.t < 0.32f) {
            float k = (v.t - 0.2f) / 0.12f; v.vitezOff = Zalet(); v.macUgao = Lerp(-170, 40, k); v.zamah = 1;
            if (!v.udareno && v.t >= 0.26f) {
                v.udareno = true;
                Vector2 p = { LUTKA_X, Grafika::TLO_Y - 16 };
                Vector2 np = { LUTKA_X, Grafika::TLO_Y - 46 };
                if (!v.zavrsni && v.noviPogoci > 0) {          // a sentence with new key points
                    int uk = std::max(1, (int)G.pitanja[v.i].pojmovi.size());
                    v.brzina += 170; v.lutkaBljesak = 1; G.tresenje = 2;
                    Zvuk::Pusti(Zvuk::UDARAC);
                    Cestice(p, 12, { 236, 206, 110, 255 }, 60, 140);
                    Natpisi(Fmt("+%d%%", (int)std::round(100.0f * v.noviPogoci / uk)), np, ZLATNA);
                } else if (!v.zavrsni) {                       // a sentence with nothing new
                    v.brzina += 50;
                    Zvuk::Pusti(Zvuk::PROMASAJ);
                    Natpisi(L("NIŠTA NOVO!", "NOTHING NEW!"), np, CRVENA);
                } else if (v.pogodak) {
                    v.brzina += 300; v.lutkaBljesak = 1; G.tresenje = 4;
                    Zvuk::Pusti(Zvuk::UDARAC);
                    Cestice(p, 24, { 236, 206, 110, 255 }, 80, 140);
                    Natpisi(L("TAČNO!", "CORRECT!"), np, ZELENA);
                } else {
                    v.brzina += 70;
                    Zvuk::Pusti(Zvuk::PROMASAJ);
                    Cestice(p, 6, { 236, 206, 110, 255 }, 40, 140);
                    Natpisi(L("PROMAŠAJ!", "MISS!"), { LUTKA_X, Grafika::TLO_Y - 46 }, CRVENA);
                }
            }
        } else {
            v.zamah = std::max(0.0f, 1 - (v.t - 0.32f) / 0.2f);
            float k = Glatko((v.t - 0.45f) / 0.35f);
            v.vitezOff = Zalet() * (1 - k); v.macUgao = Lerp(40, MAC_MIR, k);
            if (v.t >= 0.9f) {
                v.napad = false; v.t = 0;
                if (v.zavrsni) { v.rezultat = true; G.natpisi.clear(); }   // otherwise you keep writing the next sentence
            }
        }
        return;
    }

    if (Ulaz::Pritisnuto(KEY_ESCAPE)) {
        if (v.zaviri) v.zaviri = false;
        else { SacuvajStat(); G.ekran = Ekran::MENI; Zvuk::Pusti(Zvuk::MENI); }
        return;
    }
    if (Ulaz::Pritisnuto(KEY_F2) && !v.rezultat) {
        G.stat.strogiRezim = !G.stat.strogiRezim;
        SacuvajStat();
        Zvuk::Pusti(Zvuk::MENI);
        return;
    }
    if (Ulaz::Pritisnuto(KEY_TAB)) {
        bool rijeseno = v.rezultat && v.pogodak;
        if (strogi && !rijeseno) {
            Natpisi(L("PRVO ODGOVORI POTPUNO TAČNO!", "ANSWER FULLY CORRECT FIRST!"), { 160, 40 }, CRVENA);
            Zvuk::Pusti(Zvuk::PROMASAJ);
            return;
        }
        PromijeniPitanjeVjezbe(Ulaz::Shift() ? -1 : 1);
        return;
    }
    if (v.rezultat) {
        v.t += dt;
        // You stay on one question: ENTER never moves on, only TAB does. After a right answer it starts
        // fresh, after a wrong one the key points you got stay and your last text goes back in the box
        // so you can add to it (or keep going sentence by sentence).
        if (v.t > 0.3f && Ulaz::Pritisnuto(KEY_ENTER)) {
            if (v.pogodak) { v.unos.Ocisti(); NoviPokusajVjezbe(); }
            else { v.unos.Ocisti(); v.unos.tekst = v.zadnjiTekst; v.unos.kursor = (int)v.zadnjiTekst.size(); }
            v.rezultat = false;
            Zvuk::Pusti(Zvuk::MENI);
        }
        return;
    }
    if (Ulaz::Pritisnuto(KEY_F1)) { v.zaviri = !v.zaviri; Zvuk::Pusti(Zvuk::MENI); return; }
    if (v.zaviri) return;
    ObradiUnos(v.unos);
    if (PosaljiOdgovor()) {
        // You can send it sentence by sentence (the key points add up) or all at once.
        // An empty ENTER means you're done and shows the result of everything together.
        const Pitanje& p = G.pitanja[v.i];
        auto tacno = [&](const Ocjena& o) { return strogi ? PotpunoTacno(o, p) : o.tacno; };
        if (Prazno(v.unos.tekst)) {
            if (v.poslano == 0) { v.tresenjeUnosa = 1; Zvuk::Pusti(Zvuk::PROMASAJ); return; }
            v.zavrsni = true;
            v.pogodak = tacno(v.ocjena);
        } else {
            Ocjena o = OcijeniOdgovor(p, v.unos.tekst);
            v.noviPogoci = 0;
            for (size_t k = 0; k < o.pogodak.size() && k < v.ocjena.pogodak.size(); k++)
                if (o.pogodak[k] && !v.ocjena.pogodak[k]) { v.ocjena.pogodak[k] = true; v.ocjena.pogodjeno++; v.noviPogoci++; }
            v.ocjena.tacno = v.ocjena.pogodjeno >= v.ocjena.potrebno && v.ocjena.potrebno > 0;
            bool cijeliOdjednom = v.poslano == 0 && tacno(o);   // a whole paragraph that's right straight away
            v.poslano++;
            v.zadnjiTekst = v.unos.tekst;
            v.unos.Ocisti();
            v.zavrsni = cijeliOdjednom || PotpunoTacno(v.ocjena, p);
            v.pogodak = v.zavrsni ? tacno(v.ocjena) : v.noviPogoci > 0;
        }
        v.napad = true; v.udareno = false; v.t = 0;
        Zvuk::Pusti(Zvuk::MAC);
    }
}

static void CrtajVjezbuSvijet() {
    Vjezba& v = G.v;
    Grafika::CrtajVjezbaliste(G.vrijeme);
    Grafika::CrtajLutku(LUTKA_X, Grafika::TLO_Y, v.nagib, v.lutkaBljesak);
    float bob = v.napad ? 0 : std::round(std::sin(G.vrijeme * 3) * 0.6f);
    float skok = (v.napad && v.t < 0.2f && !Grafika::JeAndjeo()) ?-std::sin(PI * v.t / 0.2f) * 6 : 0;
    if (v.napad && v.t < 0.3f && v.vitezOff > 8)   // see through dash "ghosts"
        for (int k = 3; k >= 1; k--)
            Grafika::CrtajViteza(VITEZ_X + v.vitezOff * (1 - 0.22f * k), Grafika::TLO_Y + skok * (1 - 0.22f * k), 0, v.macUgao, 0, 0, 0.36f - 0.1f * k);
    Grafika::CrtajViteza(VITEZ_X + v.vitezOff, Grafika::TLO_Y + bob + skok, 0, v.macUgao, 0);
    if (Grafika::JeAndjeo()) {
        Vector2 ruka = Grafika::RukaViteza(VITEZ_X + v.vitezOff, Grafika::TLO_Y + bob + skok);
        if (v.napad && v.t < 0.2f) Grafika::CrtajNabojAndjela(ruka, v.t / 0.2f);
        if (v.zamah > 0) Grafika::CrtajSvjetlosniZrak(ruka, { LUTKA_X, Grafika::TLO_Y - 16 }, v.zamah);
    } else if (v.zamah > 0) Grafika::CrtajZamah(VITEZ_X + v.vitezOff + 3.5f, Grafika::TLO_Y - 7.5f, -150, 40, v.zamah);
}

static void CrtajVjezbuUI() {
    Vjezba& v = G.v;
    const Pitanje& p = G.pitanja[v.i];
    bool strogi = G.stat.strogiRezim;
    // secret: tiny text "carved" into the fence between the target and the dummy
    DrawTextEx(F.tekstMali, L("šifra: bosnia", "code: bosnia"), { 716, 382 }, 12, 0, { 104, 70, 40, 255 });
    TekstSjena(F.srednji, L("VJEŽBALIŠTE", "TRAINING YARD"), 24, 22, ZLATNA, 3);
    TekstSjena(F.tekstMali, L("Ovdje se uči - nema iskustva ni povreda.", "Here you learn - no XP, no injuries."), 26, 52, KREM, 2);
    std::string b = Fmt(L("PITANJE %d / %d", "QUESTION %d / %d"), v.i + 1, (int)G.pitanja.size());
    TekstSjena(F.mali, b, 1256 - SirinaTeksta(F.mali, b), 22, KREM, 2);
    std::string sr = strogi ? L("STROGI REŽIM: UKLJUČEN", "STRICT MODE: ON") : L("STROGI REŽIM: ISKLJUČEN", "STRICT MODE: OFF");
    TekstSjena(F.tekstMali, sr + "  (F2)", 1256 - SirinaTeksta(F.tekstMali, sr + "  (F2)"), 46, strogi ? Color{ 255, 140, 120, 255 } : SIVA, 2);

    if (v.rezultat) {
        std::string naslov, dno;
        Color boja;
        if (v.pogodak) { naslov = LR("TAČNO! BRAVO, VITEŽE!", "TAČNO! BRAVO, VITEZICE!", "CORRECT! WELL DONE, KNIGHT!"); boja = ZELENA; dno = L("ENTER - ponovi pitanje    TAB - sljedeće    ESC - meni", "ENTER - repeat question    TAB - next    ESC - menu"); }
        else if (strogi) { naslov = L("NIJE POTPUNO TAČNO - POKUŠAJ PONOVO", "NOT FULLY CORRECT - TRY AGAIN"); boja = CRVENA; dno = L("ENTER - pokušaj ponovo    ESC - meni", "ENTER - try again    ESC - menu"); }
        else { naslov = L("NETAČNO - EVO TAČNOG ODGOVORA:", "WRONG - HERE IS THE CORRECT ANSWER:"); boja = CRVENA; dno = L("ENTER - pokušaj ponovo    TAB - sljedeće    ESC - meni", "ENTER - try again    TAB - next    ESC - menu"); }
        CrtajOdgovor(p, &v.ocjena, naslov, boja, dno, strogi ? (int)p.pojmovi.size() : -1);
        return;
    }
    if (v.zaviri) {
        CrtajOdgovor(p, nullptr, L("ŠAPAT UČITELJA (ODGOVOR)", "THE MASTER WHISPERS (ANSWER)"), { 170, 160, 255, 255 },
                     L("F1 - zatvori i pokušaj sam", "F1 - close and try yourself"), strogi ? (int)p.pojmovi.size() : -1);
        return;
    }
    CrtajPitanje(p.tekst, 80);
    std::string dno = strogi
        ? L("ENTER = udari (i rečenica po rečenica)   prazan ENTER = gotovo   F1 = odgovor   F2 = strogi režim (SVI pojmovi)",
            "ENTER = strike (one sentence at a time is fine)   empty ENTER = done   F1 = answer   F2 = strict mode (ALL key points)")
        : L("ENTER = udari (i rečenica po rečenica)   prazan ENTER = gotovo   F1 = odgovor   F2 = strogi režim   TAB = sljedeće",
            "ENTER = strike (one sentence at a time is fine)   empty ENTER = done   F1 = answer   F2 = strict mode   TAB = next");
    std::string naslov = v.poslano > 0
        ? Fmt(L("TVOJ ODGOVOR - POGOĐENO %d / %d", "YOUR ANSWER - %d / %d HIT"), v.ocjena.pogodjeno, (int)p.pojmovi.size())
        : std::string(L("TVOJ ODGOVOR:", "YOUR ANSWER:"));
    CrtajUnos(v.unos, !v.napad, naslov, dno, v.tresenjeUnosa);
}

// ================================================================== SUBJECT AND LEVELS
static std::vector<int> VidljiviOklopi();
static bool ImaIjednoPitanje() {
    for (const NivoPitanja& n : NIVOI) if (!n.pitanja.empty()) return true;
    return false;
}
static int UkupnoPitanja() { int s = 0; for (const NivoPitanja& n : NIVOI) s += (int)n.pitanja.size(); return s; }

// in battle levels unlock in order (beat one and the next opens), the training yard is always open
static bool NivoOtkljucan(int i) {
    return !G.zaBorbu || i == 0 || G.stat.zavrsenNivo[i - 1] || DevRezim();
}

static void PokreniNivo(int i) {
    if (NIVOI[i].pitanja.empty() || !NivoOtkljucan(i)) { G.nivoTresenje = 1; Zvuk::Pusti(Zvuk::PROMASAJ); return; }
    G.nivo = i;
    G.pitanja = NIVOI[i].pitanja;
    if (G.zaBorbu) { Zvuk::Pusti(Zvuk::MAC); ZapocniBorbu(); }
    else { Zvuk::Pusti(Zvuk::MENI); ZapocniVjezbu(); }
}

static const Rectangle PREDMET_KARTA = { 440, 170, 400, 380 };

static void AzurirajPredmet(float dt) {
    G.kameraMeni += 10 * dt;
    bool dalje = Ulaz::Pritisnuto(KEY_ENTER) || Ulaz::Pritisnuto(KEY_SPACE) ||
                 (Ulaz::Klik() && CheckCollisionPointRec(Ulaz::Mis(), PREDMET_KARTA));
    if (dalje) {
        Zvuk::Pusti(Zvuk::MENI);
        // cursor starts on the first level you haven't beaten (in battle) or on the first one
        G.nivoIzbor = 0;
        if (G.zaBorbu) for (int i = 0; i < BROJ_NIVOA; i++) if (NivoOtkljucan(i)) G.nivoIzbor = i;
        G.ekran = Ekran::NIVOI;
    }
    if (Ulaz::Pritisnuto(KEY_ESCAPE) || Ulaz::Pritisnuto(KEY_BACKSPACE)) { G.ekran = Ekran::MENI; Zvuk::Pusti(Zvuk::MENI); }
}

// little network icon (nodes connected by lines)
static void IkonaMreze(float cx, float cy, float r) {
    Vector2 c[5] = { { cx, cy }, { cx - r, cy - r * 0.6f }, { cx + r, cy - r * 0.6f }, { cx - r * 0.8f, cy + r * 0.8f }, { cx + r * 0.8f, cy + r * 0.8f } };
    for (int i = 1; i < 5; i++) DrawLineEx(c[0], c[i], 4, { 140, 210, 255, 255 });
    DrawLineEx(c[1], c[2], 3, { 90, 140, 200, 255 });
    DrawLineEx(c[3], c[4], 3, { 90, 140, 200, 255 });
    for (int i = 0; i < 5; i++) {
        float vel = i == 0 ? 16 : 11;
        DrawRectangle((int)(c[i].x - vel), (int)(c[i].y - vel * 0.7f), (int)(vel * 2), (int)(vel * 1.4f), { 20, 16, 30, 255 });
        DrawRectangle((int)(c[i].x - vel + 3), (int)(c[i].y - vel * 0.7f + 3), (int)(vel * 2 - 6), (int)(vel * 1.4f - 6), i == 0 ? ZLATNA : Color{ 140, 210, 255, 255 });
    }
}

static void CrtajPredmetUI() {
    DrawRectangle(0, 0, EKRAN_W, EKRAN_H, { 10, 8, 30, 150 });
    TekstCentar(F.veliki, G.zaBorbu ? L("KRENI U BOJ", "START BATTLE") : L("VJEŽBALIŠTE", "TRAINING YARD"), 640, 50, ZLATNA);
    TekstCentar(F.mali, L("Izaberi predmet", "Choose a subject"), 640, 104, KREM);
    Rectangle r = PREDMET_KARTA;
    r.y -= 8 + std::sin(G.vrijeme * 3) * 3;
    Panel(r, PANEL, ZLATNA);
    IkonaMreze(640, r.y + 120, 60);
    TekstCentar(F.srednji, L("RAČUNARSKE", "COMPUTER"), 640, r.y + 220, ZLATNA);
    TekstCentar(F.srednji, L("MREŽE", "NETWORKS"), 640, r.y + 254, ZLATNA);
    TekstCentar(F.tekstMali, Fmt(L("%d nivoa  -  %d pitanja", "%d levels  -  %d questions"), BROJ_NIVOA, UkupnoPitanja()), 640, r.y + 300, KREM, false);
    int zavrseno = 0; for (int i = 0; i < BROJ_NIVOA; i++) zavrseno += G.stat.zavrsenNivo[i];
    TekstCentar(F.tekstMali, Fmt(L("pređeno nivoa: %d / %d", "levels cleared: %d / %d"), zavrseno, BROJ_NIVOA), 640, r.y + 326, SIVA, false);
    TekstCentar(F.tekstMali, L("Uskoro još predmeta...", "More subjects coming soon..."), 640, 580, SIVA, false);
    std::string h = L("ENTER = izaberi    ESC = nazad", "ENTER = choose    ESC = back");
    TekstSjena(F.tekstMali, h, 640 - SirinaTeksta(F.tekstMali, h) / 2, 620, SIVA, 2);
}

static Rectangle NivoKarta(int i) { return { 50 + i * 300.0f, 150, 280, 440 }; }

static void AzurirajNivoe(float dt) {
    G.kameraMeni += 10 * dt;
    G.nivoTresenje = std::max(0.0f, G.nivoTresenje - dt * 2.5f);
    int stari = G.nivoIzbor;
    if (Ulaz::Pritisnuto(KEY_LEFT) || Ulaz::Pritisnuto(KEY_A)) G.nivoIzbor = (G.nivoIzbor + BROJ_NIVOA - 1) % BROJ_NIVOA;
    if (Ulaz::Pritisnuto(KEY_RIGHT) || Ulaz::Pritisnuto(KEY_D)) G.nivoIzbor = (G.nivoIzbor + 1) % BROJ_NIVOA;
    bool kreni = Ulaz::Pritisnuto(KEY_ENTER) || Ulaz::Pritisnuto(KEY_SPACE);
    Vector2 m = Ulaz::Mis();
    for (int i = 0; i < BROJ_NIVOA; i++)
        if (CheckCollisionPointRec(m, NivoKarta(i))) { G.nivoIzbor = i; if (Ulaz::Klik()) kreni = true; }
    if (stari != G.nivoIzbor) { Zvuk::Pusti(Zvuk::MENI); G.nivoTresenje = 0; }
    if (kreni) PokreniNivo(G.nivoIzbor);
    if (Ulaz::Pritisnuto(KEY_ESCAPE) || Ulaz::Pritisnuto(KEY_BACKSPACE)) { G.ekran = Ekran::PREDMET; Zvuk::Pusti(Zvuk::MENI); }
}

static void CrtajNivoeUI() {
    DrawRectangle(0, 0, EKRAN_W, EKRAN_H, { 10, 8, 30, 160 });
    TekstCentar(F.veliki, L("RAČUNARSKE MREŽE", "COMPUTER NETWORKS"), 640, 46, ZLATNA);
    TekstCentar(F.mali, G.zaBorbu ? L("Izaberi nivo - od najlakšeg do najtežeg", "Choose a level - from easiest to hardest")
                                  : L("Vježbalište - izaberi skriptu za učenje", "Training yard - choose what to study"), 640, 100, KREM);
    const Color boje[BROJ_NIVOA] = { { 120, 200, 120, 255 }, { 200, 90, 70, 255 }, { 150, 150, 190, 255 }, { 255, 130, 50, 255 } };
    for (int i = 0; i < BROJ_NIVOA; i++) {
        const NivoPitanja& n = NIVOI[i];
        bool izabran = G.nivoIzbor == i, otvoren = NivoOtkljucan(i) && !n.pitanja.empty(), zavrsen = G.stat.zavrsenNivo[i];
        Rectangle r = NivoKarta(i);
        if (izabran) {
            r.y -= 10 + std::sin(G.vrijeme * 3) * 3;
            if (!otvoren && G.nivoTresenje > 0) r.x += std::sin(G.nivoTresenje * 60) * 8 * G.nivoTresenje;
        }
        Panel(r, otvoren ? PANEL : Color{ 16, 12, 26, 235 }, izabran ? ZLATNA : (zavrsen ? ZELENA : Color{ 110, 100, 130, 255 }));
        float cx = r.x + r.width / 2;
        // title and difficulty (one diamond per step)
        TekstCentar(F.mali, Fmt(L("NIVO %d", "LEVEL %d"), i + 1), cx, r.y + 20, otvoren ? boje[i] : SIVA);
        for (int k = 0; k < BROJ_NIVOA; k++) {
            float dx = cx - 36 + k * 24;
            Color c = k <= i ? (otvoren ? ZLATNA : SIVA) : Color{ 60, 54, 80, 255 };
            DrawTriangle({ dx, r.y + 46 }, { dx - 7, r.y + 53 }, { dx + 7, r.y + 53 }, c);
            DrawTriangle({ dx - 7, r.y + 53 }, { dx, r.y + 60 }, { dx + 7, r.y + 53 }, c);
        }
        // map preview
        Rectangle pm = { r.x + 18, r.y + 72, r.width - 36, 130 };
        Grafika::CrtajPregledMape(i, pm);
        if (!otvoren) DrawRectangleRec(pm, { 10, 8, 20, 170 });
        DrawRectangleLinesEx(pm, 3, otvoren ? boje[i] : Color{ 60, 54, 80, 255 });
        TekstCentar(F.tekstMali, L(n.mapaBs, n.mapaEn), cx, r.y + 210, otvoren ? KREM : SIVA, false);
        // source document and focus
        std::vector<std::string> ime = PrelomiTekst(F.tekstMali, n.naziv, r.width - 30);
        float y = r.y + 244;
        for (auto& red : ime) { TekstCentar(F.tekstMali, red, cx, y, otvoren ? ZLATNA : SIVA, false); y += 22; }
        TekstCentar(F.tekstMali, L(n.fokusBs, n.fokusEn), cx, y + 2, otvoren ? Color{ 140, 210, 255, 255 } : SIVA, false);
        TekstCentar(F.tekstMali, n.pitanja.empty() ? std::string(L("nema pitanja!", "no questions!"))
                                                    : Fmt(L("%d pitanja", "%d questions"), (int)n.pitanja.size()), cx, y + 28, SIVA, false);
        TekstCentar(F.tekstMali, Fmt("Boss: %s", ImeOrka(Grafika::NEPRIJATELJI_NIVOA[i].boss)), cx, y + 52, otvoren ? Color{ 255, 140, 120, 255 } : SIVA, false);
        // status
        float sy = r.y + r.height - 44;
        if (n.pitanja.empty()) TekstCentar(F.mali, L("NEMA DATOTEKE", "FILE MISSING"), cx, sy, CRVENA);
        else if (!otvoren) TekstCentar(F.tekstMali, Fmt(L("ZAKLJUČANO - pređi nivo %d", "LOCKED - clear level %d"), i), cx, sy, CRVENA, false);
        else if (zavrsen) TekstCentar(F.mali, L("PREĐENO", "CLEARED"), cx, sy, ZELENA);
        else TekstCentar(F.mali, izabran ? L("ENTER = KRENI", "ENTER = GO") : L("OTVORENO", "OPEN"), cx, sy, izabran ? ZLATNA : KREM);
    }
    std::string h = L("STRELICE = izaberi    ENTER = kreni    ESC = nazad", "ARROWS = choose    ENTER = go    ESC = back");
    TekstSjena(F.tekstMali, h, 640 - SirinaTeksta(F.tekstMali, h) / 2, 626, SIVA, 2);
    if (G.nivoTresenje > 0 && !NivoOtkljucan(G.nivoIzbor))
        TekstCentar(F.mali, Fmt(L("Prvo pređi nivo %d u boju!", "Clear level %d in battle first!"), G.nivoIzbor), 640, 660, CRVENA);
}

// ================================================================== MENU
static const int BROJ_STAVKI = 8;
static const int STAVKA_JEZIK = 5;
static std::string StavkaMenija(int k) {
    switch (k) {
        case 0: return L("KRENI U BOJ", "START BATTLE");
        case 1: return L("VJEŽBALIŠTE", "TRAINING YARD");
        case 2: return L("OKLOPI", "ARMOR");
        case 3: return L("STATISTIKA", "STATISTICS");
        case 4: return L("DOSTIGNUĆA", "ACHIEVEMENTS");
        case 5: return L("JEZIK: BOSANSKI", "LANGUAGE: ENGLISH");
        case 6: return L("PROMIJENI SPREMANJE", "CHANGE SAVE FILE");
        default: return L("IZLAZ", "EXIT");
    }
}
static const float MENI_Y = 262, MENI_KORAK = 46;

// Secret codes in the menu: the Konami code switches night mode and typing "bosnia" unlocks the
// secret Bosnian knight.
static void TajneSifre() {
    static const int KONAMI[] = { KEY_UP, KEY_UP, KEY_DOWN, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_LEFT, KEY_RIGHT, KEY_B, KEY_A };
    const int n = (int)(sizeof KONAMI / sizeof KONAMI[0]);
    const int prate[] = { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_B, KEY_A };
    for (int k : prate) {
        if (!Ulaz::Pritisnuto(k)) continue;
        G.tipke.push_back(k);
        if ((int)G.tipke.size() > n) G.tipke.erase(G.tipke.begin());
        if ((int)G.tipke.size() == n && std::equal(G.tipke.begin(), G.tipke.end(), KONAMI)) {
            G.tipke.clear();
            G.noc = !G.noc;
            SacuvajPostavke();
            if (!G.stat.konami) { G.stat.konami = true; SacuvajStat(); }   // counts as a found secret
            Zvuk::Pusti(Zvuk::NIVO);
            Natpisi(G.noc ? L("PALA JE NOĆ...", "NIGHT HAS FALLEN...") : L("SVANUO JE DAN!", "THE SUN RISES!"), { 160, 58 }, { 160, 190, 255, 255 }, &F.veliki);
        }
    }
    int c;
    while ((c = Ulaz::Znak()) != 0) {
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if (c < 'a' || c > 'z') { G.slova.clear(); continue; }
        G.slova += (char)c;
        if (G.slova.size() > 6) G.slova.erase(0, G.slova.size() - 6);
        if (G.slova == "bosnia") {
            G.slova.clear();
            bool novo = !G.stat.bosanskiOklop;
            G.stat.bosanskiOklop = true;
            G.stat.oklop = Grafika::OKLOP_BOSANSKI;   // and put it on right away
            SacuvajStat();
            Zvuk::Pusti(Zvuk::POBJEDA);
            Natpisi(novo ? L("TAJNI OKLOP: BOSANSKI VITEZ!", "SECRET ARMOR: BOSNIAN KNIGHT!") : L("BOSANSKI VITEZ!", "BOSNIAN KNIGHT!"),
                    { 160, 58 }, ZLATNA, &F.veliki);
        }
    }
}

// The sun (or moon) in the menu: click it 3 times and a tiny "try konami" shows up next to it
static Vector2 PozicijaSunca() {
    // the world (320x180) is drawn scaled up to 1296x729 and shifted by -8, -8
    float s = (EKRAN_W + 16.0f) / Grafika::SVIJET_W;
    return { -8 + 256 * s, -8 + (G.noc ? 22 : 30) * s };
}

static void KlikNaSunce(float dt) {
    G.sunceSapat = std::max(0.0f, G.sunceSapat - dt);
    if (!Ulaz::Klik() || CheckCollisionPointCircle(Ulaz::Mis(), PozicijaSunca(), 48) == false) return;
    Zvuk::Pusti(Zvuk::TIPKA);
    if (++G.kliknutoSunce >= 3) { G.kliknutoSunce = 0; G.sunceSapat = 6; }
}

static void AzurirajMeni(float dt) {
    G.kameraMeni += 16 * dt;
    TajneSifre();
    KlikNaSunce(dt);
    int stari = G.meniIzbor;
    if (Ulaz::Pritisnuto(KEY_DOWN) || Ulaz::Pritisnuto(KEY_S)) G.meniIzbor = (G.meniIzbor + 1) % BROJ_STAVKI;
    if (Ulaz::Pritisnuto(KEY_UP) || Ulaz::Pritisnuto(KEY_W)) G.meniIzbor = (G.meniIzbor + BROJ_STAVKI - 1) % BROJ_STAVKI;
    bool aktiviraj = Ulaz::Pritisnuto(KEY_ENTER) || Ulaz::Pritisnuto(KEY_SPACE);
    if (G.meniIzbor == STAVKA_JEZIK && (Ulaz::Pritisnuto(KEY_LEFT) || Ulaz::Pritisnuto(KEY_RIGHT))) aktiviraj = true;
    Vector2 m = Ulaz::Mis();
    for (int k = 0; k < BROJ_STAVKI; k++) {
        Rectangle r = { 400, MENI_Y - 10 + k * MENI_KORAK, 480, MENI_KORAK - 4 };
        if (CheckCollisionPointRec(m, r)) { G.meniIzbor = k; if (Ulaz::Klik()) aktiviraj = true; }
    }
    if (stari != G.meniIzbor) Zvuk::Pusti(Zvuk::MENI);
    if (Ulaz::Pritisnuto(KEY_L)) PromijeniJezik();
    if (!aktiviraj) return;
    bool imaPitanja = ImaIjednoPitanje();
    switch (G.meniIzbor) {
        case 0: if (imaPitanja) { Zvuk::Pusti(Zvuk::MENI); G.zaBorbu = true; G.ekran = Ekran::PREDMET; } else Zvuk::Pusti(Zvuk::PROMASAJ); break;
        case 1: if (imaPitanja) { Zvuk::Pusti(Zvuk::MENI); G.zaBorbu = false; G.ekran = Ekran::PREDMET; } else Zvuk::Pusti(Zvuk::PROMASAJ); break;
        case 2: {
            Zvuk::Pusti(Zvuk::MENI);
            std::vector<int> vid = VidljiviOklopi();
            G.oklopIzbor = 0;
            for (int i = 0; i < (int)vid.size(); i++) if (vid[i] == AktivniOklop()) G.oklopIzbor = i;
            G.ekran = Ekran::OKLOPI;
            break;
        }
        case 3: Zvuk::Pusti(Zvuk::MENI); G.ekran = Ekran::STATISTIKA; break;
        case 4: Zvuk::Pusti(Zvuk::MENI); G.ekran = Ekran::DOSTIGNUCA; break;
        case 5: PromijeniJezik(); break;
        case 6: Zvuk::Pusti(Zvuk::MENI); OtvoriEkranSpremanja(); break;
        case 7: G.izlaz = true; break;
    }
}

static void CrtajMeniSvijet(bool vitez) {
    Grafika::CrtajDolinu(G.kameraMeni, G.vrijeme);
    if (vitez) Grafika::CrtajViteza(VITEZ_X, Grafika::TLO_Y, ((int)(G.vrijeme / 0.2f)) % 2, MAC_MIR, 0);
}

static void CrtajMeniUI() {
    DrawRectangleGradientV(0, 0, EKRAN_W, 260, { 10, 8, 30, 170 }, { 10, 8, 30, 0 });
    float ty = 54 + std::sin(G.vrijeme * 1.5f) * 4;
    TekstCentar(F.naslov, NASLOV_IGRE, 640, ty, ZLATNA);
    TekstCentar(F.mali, L("Pobijedi neznanje - ork po ork!", "Defeat ignorance - one orc at a time!"), 640, ty + 76, KREM);
    std::string b = Fmt(LR("VITEZ: %s   -   NIVO %d   -   %d / %d XP", "VITEZICA: %s   -   NIVO %d   -   %d / %d XP", "KNIGHT: %s   -   LEVEL %d   -   %d / %d XP"),
                        G.stat.ime.c_str(), G.stat.nivo, G.stat.xp, XpZaNivo(G.stat.nivo));
    if (G.stat.hardcore) b += "   -   HARDCORE";
    TekstCentar(F.mali, b, 640, ty + 112, G.stat.hardcore ? Color{ 255, 140, 120, 255 } : Color{ 140, 210, 255, 255 });

    Panel({ 380, MENI_Y - 26, 520, BROJ_STAVKI * MENI_KORAK + 30 });
    for (int k = 0; k < BROJ_STAVKI; k++) {
        bool s = G.meniIzbor == k;
        bool onemoguceno = (k < 2 && !ImaIjednoPitanje());
        Color c = onemoguceno ? Color{ 100, 96, 120, 255 } : (s ? ZLATNA : KREM);
        if (k == STAVKA_JEZIK && !s) c = { 140, 210, 255, 255 };
        float y = MENI_Y + k * MENI_KORAK;
        std::string t = StavkaMenija(k);
        TekstCentar(F.srednji, t, 640, y, c);
        if (s) {
            float w = SirinaTeksta(F.srednji, t);
            float pom = std::sin(G.vrijeme * 8) * 4;
            TekstSjena(F.srednji, ">", 640 - w / 2 - 40 + pom, y, ZLATNA, 2);
            TekstSjena(F.srednji, "<", 640 + w / 2 + 16 - pom, y, ZLATNA, 2);
        }
    }
    if (!G.greska.empty()) {
        Panel({ 200, 628, 880, 56 }, { 60, 10, 20, 230 }, CRVENA);
        TekstCentar(F.tekstMali, G.greska, 640, 636, KREM, false);
        TekstCentar(F.tekstMali, L("Stavi pitanja.txt pored igre i pokreni je ponovo.", "Put pitanja.txt next to the game and start it again."), 640, 656, KREM, false);
    }
    TekstSjena(F.mali, std::string(L("Autor: ", "Author: ")) + AUTOR, 24, 690, KREM, 2);
    std::string h = L("STRELICE + ENTER     F11 = CIJELI EKRAN", "ARROWS + ENTER     F11 = FULLSCREEN");
    TekstSjena(F.mali, h, 1256 - SirinaTeksta(F.mali, h), 690, SIVA, 2);
    if (G.sunceSapat > 0) {   // the tiny whisper next to the sun
        Vector2 s = PozicijaSunca();
        float a = std::min(1.0f, G.sunceSapat / 1.5f);
        DrawTextEx(F.tekstMali, L("probaj konami", "try konami"), { s.x + 44, s.y - 6 }, 12, 0, Fade({ 250, 244, 220, 255 }, a));
    }
}

// ================================================================== ARMOR
// The secret Bosnian knight stays hidden until you unlock it with the code. The angel shows up
// locked until you reach the volcano. The cards follow the list of visible armors.
static std::vector<int> VidljiviOklopi() {
    std::vector<int> v;
    for (int o = 0; o < Grafika::BROJ_OKLOPA; o++)
        if (o != Grafika::OKLOP_BOSANSKI || OklopOtkljucan(o)) v.push_back(o);
    return v;
}

static const float OKLOP_RAZMAK = 10, OKLOP_KARTA_H = 400, OKLOP_KARTA_Y = 150, OKLOPI_SIRINA = 1240;
static Rectangle OklopKarta(int i) {
    int n = (int)VidljiviOklopi().size();
    float w = (OKLOPI_SIRINA - (n - 1) * OKLOP_RAZMAK) / n;
    return { (EKRAN_W - OKLOPI_SIRINA) / 2 + i * (w + OKLOP_RAZMAK), OKLOP_KARTA_Y, w, OKLOP_KARTA_H };
}

static void ObuciOklop(int o) {
    if (!OklopOtkljucan(o)) {
        G.oklopTresenje = 1;
        Zvuk::Pusti(Zvuk::PROMASAJ);
        return;
    }
    if (G.stat.oklop == o) return;
    G.stat.oklop = o;
    SacuvajStat();
    Zvuk::Pusti(Zvuk::MAC);
}

static void AzurirajOklope(float dt) {
    G.kameraMeni += 10 * dt;
    G.oklopTresenje = std::max(0.0f, G.oklopTresenje - dt * 2.5f);
    std::vector<int> vid = VidljiviOklopi();
    int n = (int)vid.size(), stari = G.oklopIzbor;
    if (G.oklopIzbor >= n) G.oklopIzbor = 0;
    if (Ulaz::Pritisnuto(KEY_LEFT) || Ulaz::Pritisnuto(KEY_A)) G.oklopIzbor = (G.oklopIzbor + n - 1) % n;
    if (Ulaz::Pritisnuto(KEY_RIGHT) || Ulaz::Pritisnuto(KEY_D)) G.oklopIzbor = (G.oklopIzbor + 1) % n;
    bool obuci = Ulaz::Pritisnuto(KEY_ENTER) || Ulaz::Pritisnuto(KEY_SPACE);
    Vector2 m = Ulaz::Mis();
    for (int i = 0; i < n; i++)
        if (CheckCollisionPointRec(m, OklopKarta(i))) { G.oklopIzbor = i; if (Ulaz::Klik()) obuci = true; }
    if (stari != G.oklopIzbor) { Zvuk::Pusti(Zvuk::MENI); G.oklopTresenje = 0; }
    if (obuci) ObuciOklop(vid[G.oklopIzbor]);
    if (Ulaz::Pritisnuto(KEY_ESCAPE) || Ulaz::Pritisnuto(KEY_BACKSPACE)) { G.ekran = Ekran::MENI; Zvuk::Pusti(Zvuk::MENI); }
}

// how a locked armor gets unlocked (short, for the card and the message)
static std::string UslovOklopa(int o) {
    if (o == Grafika::OKLOP_ANDJEO) return L("Stigni do vulkana", "Reach the volcano");
    return Fmt(L("NIVO %d", "LEVEL %d"), Grafika::OKLOP_NIVO[o]);
}

static void CrtajOklopeUI() {
    DrawRectangle(0, 0, EKRAN_W, EKRAN_H, { 10, 8, 30, 150 });
    TekstCentar(F.veliki, L("OKLOPI", "ARMOR"), 640, 50, ZLATNA);
    TekstCentar(F.mali, Fmt(L("Tvoj nivo: %d  -  novi oklopi se otključavaju kako napreduješ", "Your level: %d  -  new armor unlocks as you level up"), G.stat.nivo), 640, 104, KREM);

    int aktivni = AktivniOklop();
    std::vector<int> vid = VidljiviOklopi();
    for (int i = 0; i < (int)vid.size(); i++) {
        int o = vid[i];
        bool izabran = G.oklopIzbor == i, otkljucan = OklopOtkljucan(o), obucen = aktivni == o;
        Rectangle r = OklopKarta(i);
        if (izabran) {
            r.y -= 10 + std::sin(G.vrijeme * 3) * 3;
            if (!otkljucan && G.oklopTresenje > 0) r.x += std::sin(G.oklopTresenje * 60) * 8 * G.oklopTresenje;
        }
        Color okvir = izabran ? ZLATNA : (obucen ? ZELENA : Color{ 110, 100, 130, 255 });
        Panel(r, otkljucan ? PANEL : Color{ 16, 12, 26, 230 }, okvir);
        float cx = r.x + r.width / 2;

        // the knight with his sword takes about 31 sprite pixels, so the scale is picked to fit the card
        // and he gets moved left by half of the extra sword width.
        // The angel has no sword but big wings on his left (about 36 pixels wide).
        bool andjeo = o == Grafika::OKLOP_ANDJEO;
        float sk = andjeo ? std::min(5.0f, std::floor((r.width - 20) / 36.0f)) : std::min(6.0f, std::floor((r.width - 20) / 31.0f));
        float vx = andjeo ? cx + 7 * sk : cx - 7.5f * sk, vy = r.y + 250;
        DrawEllipse((int)vx, (int)vy, (int)(9 * sk), (int)(2 * sk), otkljucan ? Color{ 255, 214, 102, 40 } : Color{ 0, 0, 0, 60 });
        Grafika::CrtajVitezaPregled(IzgledOklopa(o), vx, vy, sk, !otkljucan, G.vrijeme + i);
        if (!otkljucan) TekstCentar(F.veliki, "?", vx, vy - 14 * sk, { 110, 100, 130, 255 });

        // name (two lines if it needs them)
        std::vector<std::string> ime = PrelomiTekst(F.tekstMali, ImeOklopa(o), r.width - 20);
        float y = r.y + 274;
        for (auto& red : ime) { TekstCentar(F.tekstMali, red, cx, y, otkljucan ? KREM : SIVA, false); y += 24; }

        // status
        float sy = r.y + r.height - 62;
        if (obucen) TekstCentar(F.mali, L("OBUČEN", "EQUIPPED"), cx, sy, ZELENA);
        else if (otkljucan) TekstCentar(F.mali, L("OTKLJUČAN", "UNLOCKED"), cx, sy, izabran ? ZLATNA : KREM);
        else if (o == Grafika::OKLOP_ANDJEO) {
            std::vector<std::string> u = PrelomiTekst(F.tekstMali, UslovOklopa(o), r.width - 20);
            for (size_t k = 0; k < u.size(); k++) TekstCentar(F.tekstMali, u[k], cx, sy + k * 24, CRVENA, false);
        } else TekstCentar(F.mali, UslovOklopa(o), cx, sy, CRVENA);
        if (!otkljucan && Grafika::OKLOP_NIVO[o] > 0) {
            int ostalo = Grafika::OKLOP_NIVO[o] - G.stat.nivo;
            std::string t = ostalo == 1 ? L("još 1 nivo", "1 level to go") : Fmt(L("još %d nivoa", "%d levels to go"), ostalo);
            TekstCentar(F.tekstMali, t, cx, sy + 24, SIVA, false);
        }
    }

    std::string h = L("STRELICE = izaberi    ENTER = obuci    ESC = nazad", "ARROWS = choose    ENTER = equip    ESC = back");
    TekstSjena(F.tekstMali, h, 640 - SirinaTeksta(F.tekstMali, h) / 2, 600, SIVA, 2);
    if (G.oklopTresenje > 0 && G.oklopIzbor < (int)vid.size() && !OklopOtkljucan(vid[G.oklopIzbor])) {
        int o = vid[G.oklopIzbor];
        std::string t = o == Grafika::OKLOP_ANDJEO
            ? std::string(L("Ovaj oklop se otključava kad stigneš do vulkanskog carstva!", "This armor unlocks when you reach the volcanic realm!"))
            : Fmt(L("Ovaj oklop se otključava na nivou %d!", "This armor unlocks at level %d!"), Grafika::OKLOP_NIVO[o]);
        TekstCentar(F.mali, t, 640, 640, CRVENA);
    }
}

// ================================================================== STATISTICS
static void AzurirajStatistiku(float) {
    if (Ulaz::Pritisnuto(KEY_ESCAPE) || Ulaz::Pritisnuto(KEY_ENTER) || Ulaz::Pritisnuto(KEY_BACKSPACE) || Ulaz::Klik()) {
        Zvuk::Pusti(Zvuk::MENI);
        if (G.samoPregled) OtvoriEkranSpremanja();   // a fallen hardcore knight has no menu
        else G.ekran = Ekran::MENI;
    }
}

static void CrtajStatistikuUI() {
    DrawRectangle(0, 0, EKRAN_W, EKRAN_H, { 10, 8, 30, 120 });
    Panel({ 190, 50, 900, 630 });
    TekstCentar(F.veliki, L("STATISTIKA", "STATISTICS"), 640, 80, ZLATNA);
    std::string ime = G.stat.ime;
    if (G.stat.hardcore) ime += G.stat.pao ? LR("  -  HARDCORE (PAO U BOJU)", "  -  HARDCORE (PALA U BOJU)", "  -  HARDCORE (FALLEN)") : "  -  HARDCORE";
    TekstCentar(F.tekst, ime, 640, 124, G.stat.pao ? CRVENA : Color{ 140, 210, 255, 255 }, false);
    struct Red { const char* naziv; std::string vrijednost; };
    Red redovi[] = {
        { LR("Vrijeme provedeno kao vitez", "Vrijeme provedeno kao vitezica", "Time spent as a knight"), FormatVremena(G.stat.sekundeIgre) },
        { L("Ubijeni neprijatelji", "Enemies slain"), std::to_string(G.stat.ubijeno) },
        { L("Smrti", "Deaths"), std::to_string(G.stat.smrti) },
        { L("Pobjede (pređena dolina)", "Victories (valley cleared)"), std::to_string(G.stat.pobjede) },
        { L("Nivo", "Level"), std::to_string(G.stat.nivo) },
        { L("Ukupno iskustvo (XP)", "Total experience (XP)"), std::to_string(G.stat.ukupnoXp) },
        // easter eggs: the "bosnia" code (secret armor) and the Konami code (night mode)
        { L("Pređeni nivoi", "Levels cleared"), Fmt("%d / %d", (int)G.stat.zavrsenNivo[0] + G.stat.zavrsenNivo[1] + G.stat.zavrsenNivo[2] + G.stat.zavrsenNivo[3], BROJ_NIVOA) },
        { L("Pronađene tajne", "Easter eggs found"), Fmt("%d / 2", (G.stat.bosanskiOklop ? 1 : 0) + (G.stat.konami ? 1 : 0)) },
    };
    float y = 164;
    for (auto& r : redovi) {
        TekstSjena(F.mali, r.naziv, 240, y + 6, KREM, 2);
        TekstSjena(F.srednji, r.vrijednost, 1040 - SirinaTeksta(F.srednji, r.vrijednost), y, ZLATNA, 3);
        DrawRectangle(240, (int)y + 36, 800, 2, { 214, 170, 60, 60 });
        y += 45;
    }
    CrtajXpTraku(240, y + 10, 800);
    TekstCentar(F.mali, std::string(L("Autor igre: ", "Game by: ")) + AUTOR, 640, 610, SIVA, false);
    if (Treperi()) TekstCentar(F.mali, L("ESC / ENTER - nazad", "ESC / ENTER - back"), 640, 640, ZLATNA, false);
}

// ================================================================== ACHIEVEMENTS
static void AzurirajDostignuca(float) {
    if (Ulaz::Pritisnuto(KEY_ESCAPE) || Ulaz::Pritisnuto(KEY_ENTER) || Ulaz::Pritisnuto(KEY_BACKSPACE) || Ulaz::Klik()) {
        Zvuk::Pusti(Zvuk::MENI);
        G.ekran = Ekran::MENI;
    }
}

// medal material: rim, base, inner circle, star and the two ribbons
struct Materijal { Color rub, osnova, unutra, zvijezda, traka1, traka2; };
static const Materijal BRONZA   = { { 92, 50, 24, 255 },  { 196, 124, 64, 255 },  { 166, 98, 46, 255 },  { 240, 184, 128, 255 }, { 150, 40, 40, 255 }, { 110, 70, 40, 255 } };
static const Materijal SREBRO   = { { 84, 90, 106, 255 }, { 204, 210, 222, 255 }, { 168, 176, 192, 255 }, { 246, 248, 255, 255 }, { 40, 86, 170, 255 }, { 200, 206, 220, 255 } };
static const Materijal ZLATO    = { { 120, 78, 20, 255 }, { 236, 186, 60, 255 },  { 214, 160, 40, 255 },  { 255, 236, 140, 255 }, { 190, 40, 50, 255 }, { 40, 86, 170, 255 } };
static const Materijal PLATINA  = { { 96, 118, 134, 255 },{ 226, 240, 246, 255 }, { 192, 214, 226, 255 }, { 255, 255, 255, 255 }, { 120, 200, 230, 255 }, { 230, 240, 250, 255 } };
static const Materijal OPSIDIJAN= { { 16, 8, 28, 255 },   { 74, 40, 116, 255 },   { 44, 22, 74, 255 },    { 200, 140, 255, 255 }, { 110, 50, 170, 255 }, { 30, 16, 50, 255 } };

static const Materijal& MaterijalDostignuca(int d) {
    switch (d) {
        case D_UBIJ30:                                   return BRONZA;
        case D_KONAMI: case D_BOSNA: case D_UBIJ50:      return SREBRO;
        case D_JAJA: case D_UBIJ100:                     return ZLATO;
        case D_UBIJ200:                                  return PLATINA;
        default:                                         return OPSIDIJAN;   // perfectionist, every boss
    }
}

// a medal in the achievement's material with a ribbon and a star, or a grey one with a lock
static void CrtajMedalju(float cx, float cy, float r, bool otkljucano, int d) {
    if (otkljucano) {
        const Materijal& m = MaterijalDostignuca(d);
        DrawTriangle({ cx - r * 0.7f, cy - r * 1.5f }, { cx - r * 0.15f, cy - r * 0.6f }, { cx - r * 0.75f, cy - r * 0.55f }, m.traka1);
        DrawTriangle({ cx + r * 0.7f, cy - r * 1.5f }, { cx + r * 0.75f, cy - r * 0.55f }, { cx + r * 0.15f, cy - r * 0.6f }, m.traka2);
        if (&m == &OPSIDIJAN) DrawCircleV({ cx, cy }, r + 9, Fade({ 170, 90, 255, 255 }, 0.18f + 0.08f * std::sin(G.vrijeme * 3)));   // purple glow
        if (&m == &PLATINA) DrawCircleV({ cx, cy }, r + 8, Fade({ 200, 240, 255, 255 }, 0.15f + 0.08f * std::sin(G.vrijeme * 2)));
        DrawCircleV({ cx, cy }, r + 4, m.rub);
        DrawCircleV({ cx, cy }, r, m.osnova);
        DrawCircleV({ cx, cy }, r * 0.78f, m.unutra);
        DrawPoly({ cx, cy }, 5, r * 0.55f, -90, m.zvijezda);   // star: a pentagon plus the points
        for (int k = 0; k < 5; k++) {
            float u = (-90 + k * 72) * DEG2RAD, u1 = u - 36 * DEG2RAD, u2 = u + 36 * DEG2RAD;
            DrawTriangle({ cx + std::cos(u) * r * 0.62f, cy + std::sin(u) * r * 0.62f },
                         { cx + std::cos(u1) * r * 0.26f, cy + std::sin(u1) * r * 0.26f },
                         { cx + std::cos(u2) * r * 0.26f, cy + std::sin(u2) * r * 0.26f }, m.zvijezda);
        }
        float sj = std::fmod(G.vrijeme * 0.6f, 1.0f);   // a shine that sweeps across now and then
        if (sj < 0.25f) DrawCircleV({ cx - r * 0.4f + sj * 4 * r * 0.8f, cy - r * 0.35f }, r * 0.18f, Fade(WHITE, 0.6f));
    } else {
        DrawCircleV({ cx, cy }, r + 4, { 30, 26, 40, 255 });
        DrawCircleV({ cx, cy }, r, { 70, 66, 84, 255 });
        DrawRectangle((int)(cx - r * 0.35f), (int)(cy - r * 0.05f), (int)(r * 0.7f), (int)(r * 0.55f), { 40, 36, 52, 255 });
        DrawRing({ cx, cy - r * 0.08f }, r * 0.18f, r * 0.3f, 180, 360, 12, { 40, 36, 52, 255 });
    }
}

static void CrtajDostignucaUI() {
    DrawRectangle(0, 0, EKRAN_W, EKRAN_H, { 10, 8, 30, 140 });
    Panel({ 120, 30, 1040, 660 });
    TekstCentar(F.veliki, L("DOSTIGNUĆA", "ACHIEVEMENTS"), 640, 56, ZLATNA);
    TekstCentar(F.mali, Fmt(L("Otključano: %d / %d", "Unlocked: %d / %d"), BrojDostignuca(), (int)BROJ_DOSTIGNUCA), 640, 104, KREM);
    const float W = 320, H = 150, RAZMAK = 16, x0 = 640 - (3 * W + 2 * RAZMAK) / 2, y0 = 142;
    for (int d = 0; d < BROJ_DOSTIGNUCA; d++) {
        bool ima = ImaDostignuce(d);
        float x = x0 + (d % 3) * (W + RAZMAK), y = y0 + (d / 3) * (H + 14);
        Panel({ x, y, W, H }, ima ? Color{ 40, 30, 60, 235 } : Color{ 18, 14, 28, 235 }, ima ? MaterijalDostignuca(d).zvijezda : Color{ 90, 84, 110, 255 });
        CrtajMedalju(x + 52, y + 74, 28, ima, d);
        float tx = x + 98, sirina = W - 108;
        std::vector<std::string> ime = PrelomiTekst(F.tekstMali, ImeDostignuca(d), sirina);
        float ty = y + 16;
        for (auto& red : ime) { TekstSjena(F.tekstMali, red, tx, ty, ima ? MaterijalDostignuca(d).zvijezda : SIVA, 2); ty += 22; }
        std::string opis = (!ima && TajnoDostignuce(d)) ? L("??? (tajna - istražuj)", "??? (secret - keep exploring)") : OpisDostignuca(d);
        for (auto& red : PrelomiTekst(F.tekstMali, opis, sirina)) { TekstSjena(F.tekstMali, red, tx, ty + 4, ima ? KREM : Color{ 130, 124, 150, 255 }, 2); ty += 20; }
        if (UBISTVA_ZA[d] > 0) {   // progress towards the kill count
            int n = std::min(G.stat.ubijeno, UBISTVA_ZA[d]);
            float bw = sirina - 8, by = y + H - 24;
            DrawRectangle((int)tx, (int)by, (int)bw, 10, { 20, 16, 30, 255 });
            DrawRectangle((int)tx, (int)by, (int)(bw * n / UBISTVA_ZA[d]), 10, ima ? ZLATNA : Color{ 140, 110, 200, 255 });
            std::string p = Fmt("%d / %d", n, UBISTVA_ZA[d]);
            TekstSjena(F.tekstMali, p, tx + bw - SirinaTeksta(F.tekstMali, p), by - 22, SIVA, 2);
        }
        if (!ima && d == D_JAJA) {
            int n = (G.stat.konami ? 1 : 0) + (G.stat.bosanskiOklop ? 1 : 0);
            TekstSjena(F.tekstMali, Fmt("%d / 2", n), tx, y + H - 30, SIVA, 2);
        }
        if (!ima && d == D_SVI_BOSSOVI) {
            int n = (int)G.stat.zavrsenNivo[0] + G.stat.zavrsenNivo[1] + G.stat.zavrsenNivo[2] + G.stat.zavrsenNivo[3];
            TekstSjena(F.tekstMali, Fmt(L("Bossovi: %d / %d", "Bosses: %d / %d"), n, BROJ_NIVOA), tx, y + H - 30, SIVA, 2);
        }
    }
    if (Treperi()) TekstCentar(F.mali, L("ESC / ENTER - nazad", "ESC / ENTER - back"), 640, 652, ZLATNA, false);
}

// the "achievement unlocked" popup flies in from the right, sits there for a bit, then flies out
static void CrtajObavijestDostignuca() {
    if (obavijesti.empty()) return;
    const ObavijestDostignuca& o = obavijesti.front();
    const float TRAJANJE = 4.0f;
    float ul = Glatko(o.t / 0.35f), iz = Glatko((o.t - (TRAJANJE - 0.35f)) / 0.35f);
    float w = 440, h = 92, x = EKRAN_W - (w + 20) * ul + (w + 20) * iz, y = 20;
    Panel({ x, y, w, h }, { 30, 22, 48, 245 }, ZLATNA);
    CrtajMedalju(x + 48, y + 52, 22, true, o.d);
    TekstSjena(F.tekstMali, L("DOSTIGNUĆE OTKLJUČANO!", "ACHIEVEMENT UNLOCKED!"), x + 88, y + 14, { 140, 210, 255, 255 }, 2);
    TekstSjena(F.mali, ImeDostignuca(o.d), x + 88, y + 42, ZLATNA, 2);
}

static void AzurirajObavijesti(float dt) {
    if (obavijesti.empty()) return;
    if (obavijesti.front().t == 0) Zvuk::Pusti(Zvuk::NIVO);
    obavijesti.front().t += dt;
    if (obavijesti.front().t > 4.0f) obavijesti.erase(obavijesti.begin());
}

// ================================================================== SHARED
static void AzurirajEfekte(float dt) {
    for (auto& c : G.cestice) {
        c.zivot -= dt;
        c.v.y += c.grav * dt;
        c.p.x += c.v.x * dt; c.p.y += c.v.y * dt;
    }
    G.cestice.erase(std::remove_if(G.cestice.begin(), G.cestice.end(), [](const Cestica& c) { return c.zivot <= 0; }), G.cestice.end());
    for (auto& n : G.natpisi) { n.zivot -= dt; n.p.y -= 40 * dt; }
    G.natpisi.erase(std::remove_if(G.natpisi.begin(), G.natpisi.end(), [](const Natpis& n) { return n.zivot <= 0; }), G.natpisi.end());
    G.tresenje = std::max(0.0f, G.tresenje - dt * 30);
    G.crveno = std::max(0.0f, G.crveno - dt);
    G.noviNivo = std::max(0.0f, G.noviNivo - dt);
    float cilj = G.stat.nivo + (float)G.stat.xp / XpZaNivo(G.stat.nivo);
    if (G.xpPrikaz < cilj) G.xpPrikaz = std::min(cilj, G.xpPrikaz + dt * std::max(0.6f, (cilj - G.xpPrikaz) * 2.5f));   // big jumps catch up faster
    else G.xpPrikaz = cilj;
}

static void CrtajCestice() {
    for (auto& c : G.cestice) {
        float a = Klamp(c.zivot / c.max, 0, 1);
        int s = (int)std::ceil(c.vel);
        DrawRectangle((int)c.p.x, (int)c.p.y, s, s, Fade(c.c, a));
    }
}

static void CrtajNatpise() {
    for (auto& n : G.natpisi) {
        float a = Klamp(n.zivot / (n.max * 0.4f), 0, 1);
        TekstCentar(*n.f, n.tekst, n.p.x, n.p.y, Fade(n.c, a));
    }
}

// Draws a render texture without blending, otherwise half see through pixels get darker
static void PrekopirajTeksturu(Texture2D t, Rectangle src, Rectangle dst) {
    rlDrawRenderBatchActive();
    rlDisableColorBlend();
    DrawTexturePro(t, src, dst, { 0, 0 }, 0, WHITE);
    rlDrawRenderBatchActive();
    rlEnableColorBlend();
}

static std::string PronadjiDatoteku(const char* ime) {
    std::string uz = std::string(GetApplicationDirectory()) + ime;
    if (FileExists(uz.c_str())) return uz;
    if (FileExists(ime)) return ime;
    std::string gore = std::string(GetApplicationDirectory()) + "../" + ime;   // like the build folder
    if (FileExists(gore.c_str())) return gore;
    return uz;
}

int main(int argc, char** argv) {
    // --test script.txt output_folder runs the game from a script, no player needed
    std::string testFolder;
    if (argc >= 4 && std::string(argv[1]) == "--test") {
        Ulaz::UcitajSkriptu(argv[2]);
        testFolder = argv[3];
    }
    bool test = Ulaz::TestniRezim();

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(EKRAN_W, EKRAN_H, "Studious Knight");
    SetWindowMinSize(640, 360);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    if (!test) Zvuk::Inicijalizuj();

    UcitajFontove();
    Grafika::Inicijalizuj();
    Grafika::PripremiPregledeMapa();
    Image ik = Grafika::IkonaProzora();
    SetWindowIcon(ik);
    UnloadImage(ik);

    G.rng.seed(test ? 42u : (unsigned)std::time(nullptr));
    G.folderPodataka = test ? testFolder + "/" : std::string(GetApplicationDirectory());
    UcitajPostavke();
    // questions: one file per level, in the pitanja folder next to the game
    for (NivoPitanja& n : NIVOI) {
        std::string put = PronadjiDatoteku((std::string("pitanja/") + n.datoteka).c_str());
        if (!UcitajPitanja(put, n.pitanja, n.greska)) n.pitanja.clear();
    }
    if (!ImaIjednoPitanje()) G.greska = NIVOI[0].greska;
    OtvoriEkranSpremanja();

    RenderTexture2D svijet = LoadRenderTexture(Grafika::SVIJET_W, Grafika::SVIJET_H);
    RenderTexture2D ekran = LoadRenderTexture(EKRAN_W, EKRAN_H);
    SetTextureFilter(svijet.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(ekran.texture, TEXTURE_FILTER_POINT);

    while (!WindowShouldClose() && !G.izlaz) {
        Ulaz::Pocetak();
        if (test && Ulaz::KrajSkripte()) break;
        float dt = test ? 1.0f / 60.0f : std::min(GetFrameTime(), 0.05f);
        G.vrijeme += dt;
        if (!test && IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();

        switch (G.ekran) {
            case Ekran::SPREMANJA:  AzurirajSpremanja(dt); G.kameraMeni += 10 * dt; break;
            case Ekran::MENI:       AzurirajMeni(dt); break;
            case Ekran::BORBA:      AzurirajBorbu(dt); break;
            case Ekran::VJEZBA:     AzurirajVjezbu(dt); break;
            case Ekran::STATISTIKA: AzurirajStatistiku(dt); break;
            case Ekran::DOSTIGNUCA: AzurirajDostignuca(dt); break;
            case Ekran::OKLOPI:     AzurirajOklope(dt); break;
            case Ekran::PREDMET:    AzurirajPredmet(dt); break;
            case Ekran::NIVOI:      AzurirajNivoe(dt); break;
        }
        ProvjeriDostignuca(false);
        AzurirajObavijesti(dt);
        AzurirajEfekte(dt);
        G.spasiTimer += dt;
        if (G.spasiTimer > 20) { G.spasiTimer = 0; SacuvajStat(); }

        std::string snimak = Ulaz::ZatrazenSnimak();
        if (test && snimak.empty()) continue;   // in a test only the frames with screenshots get drawn

        // the world (320x180)
        Grafika::PostaviOklop(IzgledOklopa(AktivniOklop()));
        Grafika::PostaviNoc(G.noc);
        Grafika::PostaviMapu(G.ekran == Ekran::BORBA ? G.nivo : 0);   // the level's map in battle, the valley everywhere else
        BeginTextureMode(svijet);
        ClearBackground(BLACK);
        switch (G.ekran) {
            case Ekran::SPREMANJA:
            case Ekran::OKLOPI:
            case Ekran::PREDMET:
            case Ekran::NIVOI:      CrtajMeniSvijet(false); break;
            case Ekran::MENI:
            case Ekran::STATISTIKA:
            case Ekran::DOSTIGNUCA: CrtajMeniSvijet(true); break;
            case Ekran::BORBA:      CrtajBorbuSvijet(); break;
            case Ekran::VJEZBA:     CrtajVjezbuSvijet(); break;
        }
        CrtajCestice();
        EndTextureMode();

        // the screen (1280x720): the world scaled up plus the UI
        BeginTextureMode(ekran);
        ClearBackground(BLACK);
        float sx = G.tresenje > 0 ? Rnd(-G.tresenje, G.tresenje) : 0;
        float sy = G.tresenje > 0 ? Rnd(-G.tresenje, G.tresenje) : 0;
        PrekopirajTeksturu(svijet.texture, { 0, 0, (float)Grafika::SVIJET_W, -(float)Grafika::SVIJET_H },
                           { sx - 8, sy - 8, EKRAN_W + 16.0f, EKRAN_H + 16.0f });
        // floating text goes under the panels, except the warnings in the training yard
        bool natpisiPoslije = G.ekran == Ekran::VJEZBA;
        if (!natpisiPoslije) CrtajNatpise();
        switch (G.ekran) {
            case Ekran::SPREMANJA:  CrtajSpremanjaUI(); break;
            case Ekran::MENI:       CrtajMeniUI(); break;
            case Ekran::BORBA:      CrtajBorbuUI(); break;
            case Ekran::VJEZBA:     CrtajVjezbuUI(); break;
            case Ekran::STATISTIKA: CrtajStatistikuUI(); break;
            case Ekran::DOSTIGNUCA: CrtajDostignucaUI(); break;
            case Ekran::OKLOPI:     CrtajOklopeUI(); break;
            case Ekran::PREDMET:    CrtajPredmetUI(); break;
            case Ekran::NIVOI:      CrtajNivoeUI(); break;
        }
        if (natpisiPoslije) CrtajNatpise();
        CrtajObavijestDostignuca();
        if (G.crveno > 0) {
            // the screen flashes red when the enemy hits the knight
            bool upaljeno = std::fmod(G.crveno, 0.3f) > 0.12f;
            DrawRectangle(0, 0, EKRAN_W, EKRAN_H, { 220, 20, 20, (unsigned char)((upaljeno ? 150 : 40) * std::min(1.0f, G.crveno)) });
        }
        EndTextureMode();

        // into the window, keeping 16:9
        BeginDrawing();
        ClearBackground(BLACK);
        float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
        float sk = std::min(sw / EKRAN_W, sh / EKRAN_H);
        float ox = (sw - EKRAN_W * sk) / 2, oy = (sh - EKRAN_H * sk) / 2;
        Ulaz::PostaviSkaliranje(sk, ox, oy);
        PrekopirajTeksturu(ekran.texture, { 0, 0, (float)EKRAN_W, -(float)EKRAN_H }, { ox, oy, EKRAN_W * sk, EKRAN_H * sk });
        if (!snimak.empty()) {
            Image img = LoadImageFromScreen();
            // the software renderer (tests only) gives the picture upside down with R and B swapped
            ImageFlipVertical(&img);
            ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            unsigned char* px = (unsigned char*)img.data;
            for (int k = 0; k < img.width * img.height; k++) std::swap(px[k * 4], px[k * 4 + 2]);
            ExportImage(img, (testFolder + "/" + snimak + ".png").c_str());
            UnloadImage(img);
        }
        EndDrawing();
    }

    SacuvajStat();
    UnloadRenderTexture(svijet);
    UnloadRenderTexture(ekran);
    Grafika::Oslobodi();
    OslobodiFontove();
    Zvuk::Ugasi();
    CloseWindow();
    return 0;
}
