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

// Define the possible outcomes for handling the spell effect
enum class SpellHandlingAction {
    kDispel,         // Dispel the effect and remove from persistence
    kApplyConfig,    // Keep the effect, apply duration/config rules
    kNoActionNeeded  // Take no action (e.g., summon spell + key not held, handled implicitly)
};

void DispellAllSavedSpellsFromActor(RE::Actor& actor);
void DispellAllSavedSpellsFromPlayer();

void ApplyAllSavedSpellsToActor(RE::Actor& actor);
void ApplyAllSavedPermanentSpellsToPlayer();

void ConvertToPermanentEffectOnPlayer(SpellCastInfo castInfo);

// Only if the spdlog level is set to debug or lower
void LogActiveEffectDetails(RE::ActiveEffect* activeEffect);