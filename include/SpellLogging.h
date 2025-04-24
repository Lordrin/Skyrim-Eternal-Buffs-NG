#pragma once

#include <string>

void LogKeywords(RE::BGSKeywordForm* keywordForm, const std::string& indent = "    ");
void LogAllActiveEffectsOfSpell(RE::SpellItem* spellItem);
void LogAllActiveEffectsOnActor(RE::Actor& actor);