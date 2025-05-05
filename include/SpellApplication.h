#pragma once

#include <unordered_set>

#include "ConfigLoader.h"
#include "SpellDataPersistence.h"

// Simple structure or use std::pair/tuple to pass data to the task
struct SpellCastInfo {
    RE::SpellItem* spellItem;
    RE::ActorHandle playerHandle;
    bool alreadyOnPlayer;
};

void DispellAllSavedSpellsFromActor(RE::Actor& actor);
void DispellAllSavedSpellsFromPlayer();

void ApplyAllSavedSpellsToActor(RE::Actor& actor);
void ApplyAllSavedPermanentSpellsToPlayer();

void ConvertToPermanentEffectOnPlayer(SpellCastInfo castInfo);

// Only if the spdlog level is set to debug or lower
void LogActiveEffectDetails(RE::ActiveEffect* activeEffect);