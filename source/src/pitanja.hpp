// Loads the questions and grades the answers.
#pragma once
#include <string>
#include <vector>

struct Alternativa {
    std::string norm;     // lowercase, no marks on letters
    bool cijelaRijec;     // short ones have to be a whole word, otherwise "ip" would match "zip"
};

struct KljucniPojam {
    std::string naziv;                 // what the player sees, like "NAT / PAT"
    std::vector<Alternativa> alternative;
};

struct Pitanje {
    std::string tekst;
    std::vector<std::string> odgovor;
    std::vector<KljucniPojam> pojmovi;
    int prag = 0;                      // how many key points you need to get it right
};

struct Ocjena {
    bool tacno = false;
    int pogodjeno = 0;
    int potrebno = 0;
    std::vector<bool> pogodak;         // one per key point
};

bool UcitajPitanja(const std::string& putanja, std::vector<Pitanje>& out, std::string& greska);
Ocjena OcijeniOdgovor(const Pitanje& p, const std::string& odgovor);
std::string Normalizuj(const std::string& utf8);
