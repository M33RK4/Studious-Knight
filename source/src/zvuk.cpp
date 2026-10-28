#include "zvuk.hpp"
#include "raylib.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace {
    const int RATE = 22050;
    Sound zvukovi[Zvuk::BROJ_EFEKATA];
    bool spreman = false;

    enum Talas { KVADRAT, TROUGAO, SUM };

    // A tone that slides from f0 to f1 Hz.
    void Ton(std::vector<float>& buf, float f0, float f1, float trajanje, Talas t, float jacina) {
        int n = (int)(trajanje * RATE);
        float faza = 0;
        float sumVal = 0;
        for (int i = 0; i < n; i++) {
            float k = (float)i / n;
            float f = f0 + (f1 - f0) * k;
            faza += f / RATE;
            float v = 0;
            if (t == KVADRAT) v = std::fmod(faza, 1.0f) < 0.5f ? 1.f : -1.f;
            else if (t == TROUGAO) { float p = std::fmod(faza, 1.0f); v = 4 * std::fabs(p - 0.5f) - 1; }
            else { if ((int)(faza * 2) != (int)((faza - f / RATE) * 2)) sumVal = (std::rand() / (float)RAND_MAX) * 2 - 1; v = sumVal; }
            float env = std::min(1.f, i / (RATE * 0.005f)) * (1 - k);   // quick attack, then it fades out
            buf.push_back(v * env * jacina);
        }
    }

    Sound Napravi(const std::vector<float>& buf) {
        std::vector<short> s(buf.size());
        for (size_t i = 0; i < buf.size(); i++) {
            float v = std::max(-1.f, std::min(1.f, buf[i]));
            s[i] = (short)(v * 32000);
        }
        Wave w;
        w.frameCount = (unsigned int)s.size();
        w.sampleRate = RATE;
        w.sampleSize = 16;
        w.channels = 1;
        w.data = s.data();
        return LoadSoundFromWave(w);
    }
}

namespace Zvuk {

void Inicijalizuj() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;
    std::vector<float> b;
    b.clear(); Ton(b, 1800, 300, 0.18f, SUM, 0.35f);                                       zvukovi[MAC] = Napravi(b);
    b.clear(); Ton(b, 220, 60, 0.20f, KVADRAT, 0.35f); Ton(b, 900, 100, 0.12f, SUM, 0.3f); zvukovi[UDARAC] = Napravi(b);
    b.clear(); Ton(b, 400, 90, 0.35f, KVADRAT, 0.35f);                                     zvukovi[BOL] = Napravi(b);
    b.clear(); Ton(b, 660, 660, 0.07f, KVADRAT, 0.2f); Ton(b, 990, 990, 0.12f, KVADRAT, 0.2f); zvukovi[XP] = Napravi(b);
    b.clear(); for (float f : { 523.f, 659.f, 784.f, 1046.f }) Ton(b, f, f, 0.1f, KVADRAT, 0.22f); Ton(b, 1046, 1046, 0.3f, TROUGAO, 0.4f); zvukovi[NIVO] = Napravi(b);
    b.clear(); Ton(b, 880, 880, 0.04f, KVADRAT, 0.15f);                                    zvukovi[MENI] = Napravi(b);
    b.clear(); for (float f : { 392.f, 330.f, 262.f }) Ton(b, f, f * 0.97f, 0.25f, TROUGAO, 0.5f); Ton(b, 196, 98, 0.6f, TROUGAO, 0.5f); zvukovi[SMRT] = Napravi(b);
    b.clear(); for (float f : { 523.f, 523.f, 523.f, 698.f, 880.f, 784.f, 1046.f }) Ton(b, f, f, 0.12f, KVADRAT, 0.22f); Ton(b, 1046, 1046, 0.5f, TROUGAO, 0.45f); zvukovi[POBJEDA] = Napravi(b);
    b.clear(); Ton(b, 300, 200, 0.10f, TROUGAO, 0.35f); Ton(b, 200, 140, 0.15f, TROUGAO, 0.35f); zvukovi[PROMASAJ] = Napravi(b);
    b.clear(); Ton(b, 2000, 1500, 0.015f, SUM, 0.08f);                                     zvukovi[TIPKA] = Napravi(b);
    spreman = true;
}

void Ugasi() {
    if (!spreman) return;
    for (Sound& s : zvukovi) UnloadSound(s);
    CloseAudioDevice();
}

void Pusti(Efekat e) {
    if (spreman) PlaySound(zvukovi[e]);
}

}
