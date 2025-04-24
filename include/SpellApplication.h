#pragma once

#include <unordered_set>

#include "ConfigLoader.h"
#include "SpellDataPersistence.h"

// Simple structure or use std::pair/tuple to pass data to the task
struct SpellCastInfo {
    RE::SpellItem& spellItem;
    RE::ActorHandle playerHandle;
    bool alreadyOnPlayer;
};

bool RemovePermanentSpellFromActor(RE::SpellItem* spellItem, RE::Actor* actor);
void DispellAllSavedSpellsFromActor(RE::Actor& actor);
void DispellAllSavedSpellsFromPlayer();
bool ApplyPermanentSpellToActor(RE::SpellItem* spellItem, RE::Actor* actor);
void ApplyAllSavedPermanentSpellsToPlayer();
void ConvertToPermanentEffectOnPlayer(SpellCastInfo castInfo);
void LogActiveEffectDetails(RE::ActiveEffect* activeEffect);