// Simple 8bit sound effects made in code, so the game doesn't need any sound files.
#pragma once

namespace Zvuk {
    enum Efekat { MAC, UDARAC, BOL, XP, NIVO, MENI, SMRT, POBJEDA, PROMASAJ, TIPKA, BROJ_EFEKATA };
    void Inicijalizuj();
    void Ugasi();
    void Pusti(Efekat e);
}
