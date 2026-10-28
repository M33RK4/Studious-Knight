#include "statistika.hpp"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

int XpZaNivo(int) { return XP_PO_NIVOU; }

bool DodajXp(Statistika& s, int kolicina) {
    bool novi = false;
    s.xp += kolicina;
    s.ukupnoXp += kolicina;
    while (s.xp >= XpZaNivo(s.nivo)) {
        s.xp -= XpZaNivo(s.nivo);
        s.nivo++;
        novi = true;
    }
    return novi;
}

void UcitajStatistiku(Statistika& s, const std::string& putanja) {
    std::ifstream f(putanja);
    if (!f) return;
    std::string red;
    while (std::getline(f, red)) {
        if (!red.empty() && red.back() == '\r') red.pop_back();
        size_t j = red.find('=');
        if (j == std::string::npos) continue;
        std::string k = red.substr(0, j), v = red.substr(j + 1);
        if (k == "ime") s.ime = v;
        else if (k == "spol") s.zensko = (v == "z");
        else if (k == "strogi") s.strogiRezim = (v == "1");
        else if (k == "vrijeme") s.sekundeIgre = std::atof(v.c_str());
        else if (k == "ubijeno") s.ubijeno = std::atoi(v.c_str());
        else if (k == "smrti") s.smrti = std::atoi(v.c_str());
        else if (k == "pobjede") s.pobjede = std::atoi(v.c_str());
        else if (k == "nivo") s.nivo = std::max(1, std::atoi(v.c_str()));
        else if (k == "xp") s.xp = std::atoi(v.c_str());
        else if (k == "ukupno_xp") s.ukupnoXp = std::atoi(v.c_str());
        else if (k == "oklop") s.oklop = std::max(0, std::atoi(v.c_str()));
        else if (k == "bosanski_oklop") s.bosanskiOklop = (v == "1");
        else if (k == "konami") s.konami = (v == "1");
        else if (k == "zavrseni_nivoi")
            for (size_t i = 0; i < 4 && i < v.size(); i++) s.zavrsenNivo[i] = (v[i] == '1');
        else if (k == "perfekcionista") s.perfekcionista = (v == "1");
        else if (k == "dostignuca") s.dostignuca = (unsigned int)std::strtoul(v.c_str(), nullptr, 10);
        else if (k == "najbolje") {   // saved as "key:percent,key:percent,..."
            std::istringstream ss(v);
            std::string par;
            while (std::getline(ss, par, ',')) {
                size_t d = par.find(':');
                if (d == std::string::npos) continue;
                unsigned int kljuc = (unsigned int)std::strtoul(par.substr(0, d).c_str(), nullptr, 10);
                s.najbolje[kljuc] = std::max(0, std::min(100, std::atoi(par.c_str() + d + 1)));
            }
        }
        else if (k == "hardcore") s.hardcore = (v == "1");
        else if (k == "pao") s.pao = (v == "1");
    }
}

void SacuvajStatistiku(const Statistika& s, const std::string& putanja) {
    std::ofstream f(putanja, std::ios::trunc);
    if (!f) return;
    f << "# Studious Knight - spremljena igra / save file\n";
    f << "ime=" << s.ime << "\n";
    f << "spol=" << (s.zensko ? "z" : "m") << "\n";
    f << "strogi=" << (s.strogiRezim ? 1 : 0) << "\n";
    f << "vrijeme=" << s.sekundeIgre << "\n";
    f << "ubijeno=" << s.ubijeno << "\n";
    f << "smrti=" << s.smrti << "\n";
    f << "pobjede=" << s.pobjede << "\n";
    f << "nivo=" << s.nivo << "\n";
    f << "xp=" << s.xp << "\n";
    f << "ukupno_xp=" << s.ukupnoXp << "\n";
    f << "oklop=" << s.oklop << "\n";
    f << "bosanski_oklop=" << (s.bosanskiOklop ? 1 : 0) << "\n";
    f << "konami=" << (s.konami ? 1 : 0) << "\n";
    f << "zavrseni_nivoi=";
    for (int i = 0; i < 4; i++) f << (s.zavrsenNivo[i] ? '1' : '0');
    f << "\n";
    f << "perfekcionista=" << (s.perfekcionista ? 1 : 0) << "\n";
    f << "dostignuca=" << s.dostignuca << "\n";
    f << "najbolje=";
    bool prvi = true;
    for (auto& p : s.najbolje) { f << (prvi ? "" : ",") << p.first << ":" << p.second; prvi = false; }
    f << "\n";
    f << "hardcore=" << (s.hardcore ? 1 : 0) << "\n";
    f << "pao=" << (s.pao ? 1 : 0) << "\n";
}

std::string FormatVremena(double sek) {
    long t = (long)sek;
    char b[64];
    std::snprintf(b, sizeof b, "%ldh %02ldm %02lds", t / 3600, (t / 60) % 60, t % 60);
    return b;
}

bool SpremanjePostoji(const std::string& putanja) {
    std::ifstream f(putanja);
    return (bool)f;
}

void ObrisiSpremanje(const std::string& putanja) {
    std::remove(putanja.c_str());
}
