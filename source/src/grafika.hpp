// All the pixel art: sprites, maps, the training yard, cards.
// Everything gets drawn into a tiny 320x180 texture and blown up 4x, that's where the pixel look comes from.
#pragma once
#include "raylib.h"

namespace Grafika {
    const int SVIJET_W = 320;
    const int SVIJET_H = 180;
    const float TLO_Y = 112.0f;     // where everyone's feet are

    void Inicijalizuj();
    void Oslobodi();
    Image IkonaProzora();           // the knight as the window icon (caller frees it)

    // Maps: 0 valley, 1 swamp, 2 dungeon, 3 volcano. kamera is how far the world has scrolled.
    void PostaviMapu(int mapa);
    void CrtajDolinu(float kamera, float vrijeme);   // draws whichever map is set
    void PripremiPregledeMapa();                     // small map previews for the level screen, call once after Inicijalizuj
    void CrtajPregledMape(int mapa, Rectangle dst);  // in screen coordinates
    void CrtajVjezbaliste(float vrijeme);

    // Armor skins. They unlock at the levels in OKLOP_NIVO, a 0 there means it unlocks some other way:
    // the Bosnian knight comes from the secret code and the angel from reaching the volcano.
    const int BROJ_OKLOPA = 8, OKLOP_BOSANSKI = 6, OKLOP_ANDJEO = 7;
    const int OKLOP_NIVO[BROJ_OKLOPA] = { 1, 10, 30, 60, 100, 150, 0, 0 };
    // Looks 0 to 7 are the armors, 8 to 15 the same ones for a lady knight (8 is the pink one she
    // starts with). OKLOP_VITEZICA is also the index of her palette.
    const int OKLOP_VITEZICA = BROJ_OKLOPA, BROJ_IZGLEDA = 2 * BROJ_OKLOPA;
    inline int IzgledViteza(int oklop, bool zensko) { return zensko ? BROJ_OKLOPA + oklop : oklop; }
    void PostaviOklop(int oklop);   // which armor CrtajViteza uses
    void PostaviNoc(bool noc);      // night version of the maps (Konami code)

    // x is the middle of the character, y is where the feet are. okvir 0 is standing, 1 is a step.
    void CrtajViteza(float x, float y, int okvir, float ugaoMaca, float bljesak, float rotacija = 0, float alfa = 1);
    // Knight in a given armor, in screen coordinates (1280x720). zakljucan draws a dark silhouette.
    void CrtajVitezaPregled(int oklop, float x, float y, float skala, bool zakljucan, float vrijeme);
    // Enemies ("vrsta"):
    //  valley:  0 grunt, 1 berserker, 2 shaman, 3 spearman, 4 brute, 5 warlord (boss)
    //  swamp:   6 hunter, 7 mud brute, 8 witch doctor, 9 blowgunner, 10 bog chieftain (boss)
    //  dungeon: 11 skeleton warrior, 12 archer, 13 necromancer, 14 skeleton knight, 15 lich king (boss)
    //  volcano: 16 imp, 17 fire demon, 18 hellhound, 19 executioner, 20 demon king (boss)
    const int BROJ_VRSTA_ORKA = 6, ORK_VOJSKOVODJA = 5, BROJ_NEPRIJATELJA = 21;
    struct NeprijateljiNivoa { int prvi, brojObicnih, boss; };
    const NeprijateljiNivoa NEPRIJATELJI_NIVOA[4] = { { 0, 5, 5 }, { 6, 4, 10 }, { 11, 4, 15 }, { 16, 4, 20 } };
    enum class Napad { SKOK, UBOD, CAROLIJA, STRIJELA };   // leap and smash, thrust, magic ball, arrow or dart
    Napad NapadNeprijatelja(int vrsta);
    Color BojaCarolije(int vrsta);
    Vector2 IzvorProjektila(int vrsta, float x, float y);   // where a spell or arrow flies out from (x, y = feet)
    Vector2 RukaNeprijatelja(int vrsta, float x, float y);  // the hand holding the weapon (x, y = feet)
    bool Lebdi(int vrsta);                                  // floats above the ground (the imp)
    float SkalaOrka(int vrsta);
    void CrtajOrka(float x, float y, float ugaoOruzja, float bljesak, float alfa, bool premaDesno, int vrsta, float rotacija = 0);
    // arc of an enemy's weapon swing (x, y = hand)
    void CrtajZamahOrka(float x, float y, float od, float doUgla, float radijus, float alfa);
    // spell or arrow mid flight, k goes from 0 at the enemy to 1 at the knight
    void CrtajProjektil(int vrsta, Vector2 od, Vector2 doCilja, float k);
    void CrtajLutku(float x, float y, float nagib, float bljesak);

    // the arc behind a sword swing
    void CrtajZamah(float x, float y, float od, float doUgla, float alfa);

    // The angel has no sword. He stays put and fires a beam of light from the orb in his hand.
    bool JeAndjeo();
    Vector2 RukaViteza(float x, float y);                  // where the orb of light is (x, y = feet)
    void CrtajNabojAndjela(Vector2 ruka, float k);         // light gathering in the orb, k = 0..1
    void CrtajSvjetlosniZrak(Vector2 od, Vector2 cilj, float jacina);

    // Tarot card for a save slot. lice = false is the back (empty save), true is the front with the
    // knight. sirinaFaktor (0..1) squishes it for the flip animation.
    const int KARTA_W = 54, KARTA_H = 90;
    void CrtajKartu(float cx, float y, float skala, float sirinaFaktor, bool lice, Color nijansa = WHITE, bool zensko = false);

    // heart for the HUD (screen coordinates)
    void CrtajSrce(float x, float y, float skala, bool puno);
}
