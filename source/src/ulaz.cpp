#include "ulaz.hpp"
#include <fstream>
#include <vector>
#include <deque>
#include <set>
#include <cstdio>

namespace {
    struct Komanda { std::string tip, arg; };
    std::vector<Komanda> skripta;
    size_t poz = 0;
    int cekanje = 0;
    bool testni = false;
    bool kraj = false;
    std::set<int> tipkeOvajFrame;
    std::deque<int> znakovi;
    std::string snimak;
    float skala = 1, px = 0, py = 0;
    Vector2 testMis = { -100, -100 };   // where the mouse is during a test ("click X Y")
    bool testKlik = false;

    int ImeUTipku(const std::string& s) {
        if (s == "ENTER") return KEY_ENTER;
        if (s == "ESC") return KEY_ESCAPE;
        if (s == "BACKSPACE") return KEY_BACKSPACE;
        if (s == "TAB") return KEY_TAB;
        if (s == "F1") return KEY_F1;
        if (s == "UP") return KEY_UP;
        if (s == "DOWN") return KEY_DOWN;
        if (s == "LEFT") return KEY_LEFT;
        if (s == "RIGHT") return KEY_RIGHT;
        if (s == "SPACE") return KEY_SPACE;
        if (s == "DELETE") return KEY_DELETE;
        if (s == "F2") return KEY_F2;
        if (s == "HOME") return KEY_HOME;
        if (s == "END") return KEY_END;
        if (s == "PAGE_UP") return KEY_PAGE_UP;
        if (s == "PAGE_DOWN") return KEY_PAGE_DOWN;
        if (s == "SHIFT") return KEY_LEFT_SHIFT;
        if (s == "CTRL") return KEY_LEFT_CONTROL;
        if (s.size() == 1 && s[0] >= 'A' && s[0] <= 'Z') return KEY_A + (s[0] - 'A');
        return 0;
    }
}

namespace Ulaz {

bool UcitajSkriptu(const std::string& putanja) {
    std::ifstream f(putanja);
    if (!f) return false;
    std::string red;
    while (std::getline(f, red)) {
        if (!red.empty() && red.back() == '\r') red.pop_back();
        if (red.empty() || red[0] == '#') continue;
        size_t sp = red.find(' ');
        Komanda k;
        k.tip = red.substr(0, sp);
        k.arg = sp == std::string::npos ? "" : red.substr(sp + 1);
        skripta.push_back(k);
    }
    testni = true;
    return true;
}

bool TestniRezim() { return testni; }
bool KrajSkripte() { return kraj; }
std::string ZatrazenSnimak() { return snimak; }

void Pocetak() {
    if (!testni) return;
    tipkeOvajFrame.clear();
    snimak.clear();
    testKlik = false;
    if (cekanje > 0) { cekanje--; return; }
    while (poz < skripta.size()) {
        Komanda& k = skripta[poz++];
        if (k.tip == "wait") { cekanje = std::atoi(k.arg.c_str()) - 1; return; }
        if (k.tip == "key") {
            // combos like "SHIFT+ENTER" or "CTRL+LEFT", the modifier counts as held for that frame
            std::string ostalo = k.arg;
            size_t plus;
            while ((plus = ostalo.find('+')) != std::string::npos) {
                tipkeOvajFrame.insert(ImeUTipku(ostalo.substr(0, plus)));
                ostalo = ostalo.substr(plus + 1);
            }
            tipkeOvajFrame.insert(ImeUTipku(ostalo));
            return;
        }
        if (k.tip == "type") {
            const char* s = k.arg.c_str();
            int i = 0;
            while (s[i]) { int n = 0; int cp = GetCodepointNext(s + i, &n); znakovi.push_back(cp); i += n; }
            return;
        }
        if (k.tip == "shot") { snimak = k.arg; return; }
        if (k.tip == "click") {   // "click X Y" in game coordinates (1280x720)
            std::sscanf(k.arg.c_str(), "%f %f", &testMis.x, &testMis.y);
            testKlik = true;
            return;
        }
        if (k.tip == "quit") { kraj = true; return; }
    }
    kraj = true;
}

bool Pritisnuto(int key) {
    if (testni) return tipkeOvajFrame.count(key) > 0;
    return IsKeyPressed(key);
}

bool PritisnutoPonovo(int key) {
    if (testni) return tipkeOvajFrame.count(key) > 0;
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

bool Drzi(int key) {
    if (testni) return tipkeOvajFrame.count(key) > 0;
    return IsKeyDown(key);
}

bool Shift() { return Drzi(KEY_LEFT_SHIFT) || Drzi(KEY_RIGHT_SHIFT); }
bool Ctrl()  { return Drzi(KEY_LEFT_CONTROL) || Drzi(KEY_RIGHT_CONTROL); }

float Tockic() {
    if (testni) return 0;
    return GetMouseWheelMove();
}

int Znak() {
    if (testni) {
        if (znakovi.empty()) return 0;
        int c = znakovi.front(); znakovi.pop_front(); return c;
    }
    return GetCharPressed();
}

void PostaviSkaliranje(float s, float x, float y) { skala = s; px = x; py = y; }

Vector2 Mis() {
    if (testni) return testMis;
    Vector2 m = GetMousePosition();
    return { (m.x - px) / skala, (m.y - py) / skala };
}

bool Klik() {
    if (testni) return testKlik;
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

}
