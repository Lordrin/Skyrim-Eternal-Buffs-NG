#pragma once

#include <string>

// void LogAllSavedSpellData();
void LogKeywords(RE::BGSKeywordForm* keywordForm, const std::string& indent = "    ");
void LogAllActiveEffectsOfSpell(RE::SpellItem* spellItem);
void getTitem(RE::FormID formID);
void LogAllActiveEffectsOnActor(RE::Actor& actor);