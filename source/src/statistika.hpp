// One save slot: the knight's name and all their stats.
// Each one lives in StudiousKnight_save1.txt, _save2.txt or _save3.txt next to the game.
#pragma once
#include <string>
#include <map>

struct Statistika {
    std::string ime;
    bool zensko = false;      // lady knight, picked when the save is made
    bool strogiRezim = false; // training yard won't let you move on until the answer is 100%
    double sekundeIgre = 0;   // time spent in battle and in the training yard
    int ubijeno = 0;
    int smrti = 0;
    int pobjede = 0;          // full runs where every enemy went down
    int nivo = 1;
    int xp = 0;               // XP inside the current level, not the total
    int ukupnoXp = 0;
    int oklop = 0;
    bool bosanskiOklop = false; // secret armor from typing "bosnia"
    bool konami = false;      // found the Konami code (night mode)
    bool zavrsenNivo[4] = {}; // beaten levels, each one opens the next
    bool perfekcionista = false; // beat a whole level with every answer at 100%
    unsigned int dostignuca = 0; // one bit per achievement, the list is in main.cpp
    std::map<unsigned int, int> najbolje; // best score (0 to 100%) on every question, so replays only pay for improvement
    bool hardcore = false;    // can only be picked when the save is made
    bool pao = false;         // the hardcore knight died, so the save is just a memorial now
};

const int XP_PO_NIVOU = 1000;
int  XpZaNivo(int nivo);                 // same for every level
bool DodajXp(Statistika& s, int kolicina); // true if you leveled up
void UcitajStatistiku(Statistika& s, const std::string& putanja);
void SacuvajStatistiku(const Statistika& s, const std::string& putanja);
std::string FormatVremena(double sekunde);
bool SpremanjePostoji(const std::string& putanja);
void ObrisiSpremanje(const std::string& putanja);
