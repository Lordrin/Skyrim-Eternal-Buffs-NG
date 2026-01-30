#pragma once

#include <string>

void LogKeywords(RE::BGSKeywordForm* keywordForm, const std::string& indent = "    ");
void LogAllActiveEffectsOfSpell(RE::SpellItem* spellItem);
void LogAllActiveEffectsOnActor(RE::Actor& actor);
void LogSpellItemDetails(RE::SpellItem* spellItem, const std::string& indent = "  ", const std::string& prefix = "Player casting spell:");
void LogActiveEffectDetails(RE::ActiveEffect* activeEffect);
void LogAllSpellsOnActor(RE::Actor& actor);
void CheckEffectStatus(RE::Actor& actor);