// Fonts and helpers for UTF8 text, so č ć š ž đ show up properly.
#pragma once
#include "raylib.h"
#include <string>
#include <vector>

struct Fontovi {
    Font naslov;   // Press Start 2P, 48px, the game title
    Font veliki;   // Press Start 2P, 32px, questions
    Font srednji;  // Press Start 2P, 24px
    Font mali;     // Press Start 2P, 16px, HUD and menus
    Font tekst;    // VT323, 32px, typing answers
    Font tekstMali; // VT323, 24px, correct answers
};
extern Fontovi F;

void UcitajFontove();
void OslobodiFontove();

float SirinaTeksta(const Font& f, const std::string& s);
std::vector<std::string> PrelomiTekst(const Font& f, const std::string& s, float maxSirina);

void Tekst(const Font& f, const std::string& s, float x, float y, Color c);
void TekstSjena(const Font& f, const std::string& s, float x, float y, Color c, int pomak = 3);
void TekstCentar(const Font& f, const std::string& s, float cx, float y, Color c, bool sjena = true);

void Utf8IzbrisiZadnji(std::string& s);
void Utf8IzbrisiRijec(std::string& s);
void Utf8Dodaj(std::string& s, int codepoint);
int  Utf8Duzina(const std::string& s);
