#pragma once

#include <unordered_set>

#include "FileParsing.h"
#include "SpellDataPersistence.h"

// Simple structure or use std::pair/tuple to pass data to the task
struct SpellCastInfo {
    RE::SpellItem* spellItem;
    RE::ActorHandle playerHandle;
};

bool RemovePermanentSpellFromActor(RE::SpellItem* spellItem, RE::Actor* actor);
bool ApplyPermanentSpellToActor(RE::SpellItem* spellItem, RE::Actor* actor);
void ApplyAllSavedPermanentSpellsToPlayer();
void ConvertToPermanentEffectOnPlayer(SpellCastInfo castInfo);