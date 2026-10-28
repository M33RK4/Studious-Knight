// Keyboard and mouse. Everything goes through here so a test script can play the game too.
#pragma once
#include "raylib.h"
#include <string>

namespace Ulaz {
    void Pocetak();                 // call once per frame, before anything else
    bool Pritisnuto(int key);       // just pressed (with key repeat where it makes sense)
    bool PritisnutoPonovo(int key); // pressed, or held long enough to repeat
    bool Drzi(int key);
    bool Shift();
    bool Ctrl();
    int  Znak();                    // next typed character (Unicode), 0 if there isn't one
    Vector2 Mis();                  // in game coordinates (1280x720)
    bool Klik();
    float Tockic();                 // positive is up, negative is down

    // automated testing
    bool UcitajSkriptu(const std::string& putanja);
    bool TestniRezim();
    std::string ZatrazenSnimak();   // screenshot to take this frame, "" if none
    bool KrajSkripte();

    void PostaviSkaliranje(float skala, float pomakX, float pomakY);
}
