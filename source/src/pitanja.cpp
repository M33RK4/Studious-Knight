#include "pitanja.hpp"
#include <fstream>
#include <sstream>
#include <set>
#include <cmath>
#include <cstdlib>
#include <algorithm>

// Makes answers easy to compare: lowercase, no marks on letters (đ becomes dj) and anything
// that isn't a letter or a digit turns into a space. So "Čvor-ruter" and "cvor ruter" end up the same.
static int SljedeciZnak(const std::string& s, size_t& i) {
    unsigned char c = (unsigned char)s[i];
    int cp = c, len = 1;
    if (c >= 0xF0 && i + 3 < s.size() + 0) { cp = ((c & 0x07) << 18) | ((s[i+1] & 0x3F) << 12) | ((s[i+2] & 0x3F) << 6) | (s[i+3] & 0x3F); len = 4; }
    else if (c >= 0xE0 && i + 2 < s.size()) { cp = ((c & 0x0F) << 12) | ((s[i+1] & 0x3F) << 6) | (s[i+2] & 0x3F); len = 3; }
    else if (c >= 0xC0 && i + 1 < s.size()) { cp = ((c & 0x1F) << 6) | (s[i+1] & 0x3F); len = 2; }
    i += len;
    return cp;
}

static int Superskript(int cp) {
    switch (cp) {
        case 0x2070: return 0; case 0x00B9: return 1; case 0x00B2: return 2; case 0x00B3: return 3;
        case 0x2074: return 4; case 0x2075: return 5; case 0x2076: return 6; case 0x2077: return 7;
        case 0x2078: return 8; case 0x2079: return 9;
    }
    return -1;
}

std::string Normalizuj(const std::string& s) {
    std::string out = " ";
    bool prosliSup = false;
    size_t i = 0;
    while (i < s.size()) {
        int cp = SljedeciZnak(s, i);
        int sup = Superskript(cp);
        std::string dodaj;
        if (sup >= 0) {
            if (!prosliSup) dodaj += ' ';
            dodaj += char('0' + sup);
        }
        else if (cp >= 'A' && cp <= 'Z') dodaj = char(cp - 'A' + 'a');
        else if ((cp >= 'a' && cp <= 'z') || (cp >= '0' && cp <= '9')) dodaj = char(cp);
        else if (cp == 0x10C || cp == 0x10D || cp == 0x106 || cp == 0x107) dodaj = "c";
        else if (cp == 0x160 || cp == 0x161) dodaj = "s";
        else if (cp == 0x17D || cp == 0x17E) dodaj = "z";
        else if (cp == 0x110 || cp == 0x111) dodaj = "dj";
        else dodaj = " ";
        prosliSup = (sup >= 0);
        for (char c : dodaj) {
            if (c == ' ' && out.back() == ' ') continue;
            out += c;
        }
    }
    if (out.back() != ' ') out += ' ';
    return out;   // always starts and ends with a space, so whole words are easy to spot
}

static std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static Alternativa NapraviAlternativu(const std::string& tekst) {
    Alternativa a;
    std::string n = Normalizuj(tekst);
    a.norm = n.substr(1, n.size() >= 2 ? n.size() - 2 : 0);
    a.cijelaRijec = a.norm.find(' ') == std::string::npos && a.norm.size() <= 3;
    return a;
}

static void ZavrsiPitanje(Pitanje& p, std::vector<Pitanje>& out) {
    if (p.tekst.empty()) return;
    // No key points written for this question? Then the longer words from the answer will do.
    if (p.pojmovi.empty()) {
        std::set<std::string> vidjeno;
        for (const std::string& red : p.odgovor) {
            std::istringstream ss(Normalizuj(red));
            std::string w;
            while (ss >> w && p.pojmovi.size() < 12) {
                if (w.size() >= 7 && !vidjeno.count(w)) {
                    vidjeno.insert(w);
                    KljucniPojam k; k.naziv = w;
                    k.alternative.push_back({ w.substr(0, w.size() - 1), false }); // without the last letter, so other grammatical cases still match
                    p.pojmovi.push_back(k);
                }
            }
        }
        if (p.prag <= 0) p.prag = std::max(1, (int)std::ceil(p.pojmovi.size() * 0.3));
    }
    if (p.prag <= 0) p.prag = std::max(1, (int)std::ceil(p.pojmovi.size() / 2.0));
    if (p.prag > (int)p.pojmovi.size()) p.prag = (int)p.pojmovi.size();
    out.push_back(p);
    p = Pitanje();
}

bool UcitajPitanja(const std::string& putanja, std::vector<Pitanje>& out, std::string& greska) {
    std::ifstream f(putanja, std::ios::binary);
    if (!f) { greska = "Ne mogu otvoriti datoteku: " + putanja; return false; }
    out.clear();
    Pitanje p;
    std::string red;
    bool prvi = true;
    while (std::getline(f, red)) {
        if (prvi && red.size() >= 3 && (unsigned char)red[0] == 0xEF && (unsigned char)red[1] == 0xBB && (unsigned char)red[2] == 0xBF)
            red = red.substr(3);   // skip the UTF8 BOM if the file has one
        prvi = false;
        red = Trim(red);
        if (red.empty() || red[0] == '#') continue;
        if (red.rfind("---", 0) == 0) { ZavrsiPitanje(p, out); continue; }
        if (red.rfind("P:", 0) == 0) {
            ZavrsiPitanje(p, out);
            p.tekst = Trim(red.substr(2));
        } else if (red.rfind("K:", 0) == 0) {
            std::string sadrzaj = red.substr(2);
            KljucniPojam k;
            size_t jed = sadrzaj.find('=');
            std::string alt;
            if (jed == std::string::npos) { k.naziv = Trim(sadrzaj); alt = sadrzaj; }
            else { k.naziv = Trim(sadrzaj.substr(0, jed)); alt = sadrzaj.substr(jed + 1); }
            std::stringstream ss(alt);
            std::string a;
            while (std::getline(ss, a, '|')) {
                a = Trim(a);
                if (a.empty()) continue;
                Alternativa al = NapraviAlternativu(a);
                if (!al.norm.empty()) k.alternative.push_back(al);
            }
            if (!k.alternative.empty()) p.pojmovi.push_back(k);
        } else if (red.rfind("PRAG:", 0) == 0) {
            p.prag = std::atoi(red.substr(5).c_str());
        } else if (red.rfind("O:", 0) == 0) {
            p.odgovor.push_back(Trim(red.substr(2)));
        } else if (!p.odgovor.empty()) {
            p.odgovor.back() += " " + red;   // a long answer can go over several lines
        }
    }
    ZavrsiPitanje(p, out);
    if (out.empty()) { greska = "U datoteci nema nijednog pitanja: " + putanja; return false; }
    return true;
}

Ocjena OcijeniOdgovor(const Pitanje& p, const std::string& odgovor) {
    Ocjena o;
    std::string h = Normalizuj(odgovor);
    o.potrebno = p.prag;
    for (const KljucniPojam& k : p.pojmovi) {
        bool hit = false;
        for (const Alternativa& a : k.alternative) {
            std::string igla = " " + a.norm + (a.cijelaRijec ? " " : "");
            if (h.find(igla) != std::string::npos) { hit = true; break; }
        }
        o.pogodak.push_back(hit);
        if (hit) o.pogodjeno++;
    }
    o.tacno = o.pogodjeno >= o.potrebno && o.potrebno > 0;
    return o;
}
