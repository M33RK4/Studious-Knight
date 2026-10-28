// Bosnian or English. L("bosanski", "english") gives you the text for whichever one is on.
#pragma once

enum class Jezik { BOSANSKI, ENGLESKI };
extern Jezik JEZIK;

inline const char* L(const char* bs, const char* en) { return JEZIK == Jezik::ENGLESKI ? en : bs; }
