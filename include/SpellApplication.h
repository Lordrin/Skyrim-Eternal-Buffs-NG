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
void DispelSpellItemFromActor(RE::Actor* actor, RE::SpellItem* spellItem);

void ApplyAllSavedSpellsToActor(RE::Actor& actor);
void ApplyAllSavedPermanentSpellsToPlayer();

// void ApplyReserveSpellToPlayer(RE::SpellItem* spellToApply);

void ConvertToPermanentEffectOnPlayer(SpellCastInfo castInfo);

void CheckAndDispelReserveSpellFromPlayer(RE::ActiveEffect* activeEffect);