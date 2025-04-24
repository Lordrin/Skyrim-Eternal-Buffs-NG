#include "SpellApplication.h"

#include <RE/A/ActorValueOwner.h>  // <<< Add this line

constexpr RE::FormID MyPermanentAbilitySpellFormID = 0x000800;
const char* MyPluginName = "LoricaNG.esl";
// const float permanentSpellDuration = 86313600.0f;

/**
 * @brief Applies configuration rules to an active effect.
 * @param activeEffect The active effect to apply rules to.
 * @return True if the rules were applied successfully, false otherwise.
 * @note This function checks if the active effect's spell matches any rules in the configuration.
 */
bool ApplyConfigRulesToActiveEffect(RE::ActiveEffect* activeEffect) {
    if (!activeEffect) {
        SKSE::log::warn("ApplyConfigRulesToActiveEffect: activeEffect is null");
        return false;
    }

    auto spellRuleIt = GBL::GetSpellRules().find(Utilities::RemoveWhitespace(activeEffect->spell->GetFullName()));
    if (spellRuleIt != GBL::GetSpellRules().end()) {
        SpellRule spellRule = spellRuleIt->second;
        spellRule.ShouldApplyRuleToSpell(activeEffect->spell->As<RE::SpellItem>());
        spellRule.ApplySpellRulesToActiveEffect(activeEffect);
        return true;

    } else {
        SKSE::log::info("ApplyConfigRulesToSpell: Spell {} not found in rules.", activeEffect->spell->GetName());
        return false;
    }
}

// void ResetShoutCooldown(RE::Actor* player, float cooldown = 1.0f) {
//     if (!player) {
//         SKSE::log::warn("ResetShoutCooldown: Player is null.");
//         return;
//     }

//     // Get the player's shout cooldown timer
//     auto shoutCooldown = player->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary,
//     RE::ActorValue::kShoutRecoveryMult); SKSE::log::info("Current shout cooldown: {:.2f}", shoutCooldown);

//     // Set the cooldown to the desired value (e.g., 1 second)
//     player->ModActorValue(RE::ActorValue::kShoutRecoveryMult, cooldown);
//     SKSE::log::info("Shout cooldown set to {:.2f} seconds.", cooldown);
// }

bool RemoveSpecificActiveEffects(const std::vector<RE::FormID>& activeEffectIds) {
    RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::warn("RemoveEffects: Player handle invalid");
        return false;
    }

    RE::MagicTarget* magicTarget = player->GetMagicTarget();
    if (!magicTarget) {
        SKSE::log::warn("RemoveEffects: Player has no MagicTarget");
        return false;
    }

    RE::BSSimpleList<RE::ActiveEffect*>* activeEffects = magicTarget->GetActiveEffectList();
    if (!activeEffects || activeEffects->empty()) {
        SKSE::log::info("RemoveEffects: Player has no active effects list or it's empty.");
        return true;  // Nothing to remove, operation technically succeeded.
    }

    SKSE::log::info("--- Checking active effects for removal ---");

    // --- Pass 1: Collect the ActiveEffect pointers to remove ---
    // This avoids modifying the list while iterating over it.
    std::vector<RE::ActiveEffect*> effectsToRemove;
    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        // Add thorough null checks for safety
        if (!activeEffect || !activeEffect->effect || !activeEffect->effect->baseEffect) {
            SKSE::log::trace("RemoveEffects: Skipping effect with null pointers.");
            continue;
        }

        RE::EffectSetting* baseMGEF = activeEffect->effect->baseEffect;
        RE::FormID baseFormID = baseMGEF->GetFormID();

        // Check if the effect's base FormID is in the list to remove
        if (std::find(activeEffectIds.begin(), activeEffectIds.end(), baseFormID) != activeEffectIds.end()) {
            const char* mgefName = baseMGEF->GetName();
            if (!mgefName || mgefName[0] == '\0') {
                mgefName = "Unnamed Effect";
            }

            SKSE::log::info("Found effect to remove: {:#010x} ({})", baseFormID, mgefName);
            SKSE::log::info("  - Instance Ptr: {}", (void*)activeEffect);
            SKSE::log::info("  - Current Duration: {:.2f}", activeEffect->duration);
            SKSE::log::info("  - Elapsed Time: {:.2f}", activeEffect->elapsedSeconds);

            effectsToRemove.push_back(activeEffect);
        }
    }

    // --- Pass 2: Dispel the collected effects ---
    if (effectsToRemove.empty()) {
        SKSE::log::info("--- No matching effects found to remove. ---");
        return true;
    }

    SKSE::log::info("--- Attempting to dispel {} effects... ---", effectsToRemove.size());
    int dispelCount = 0;
    for (RE::ActiveEffect* effectToDispel : effectsToRemove) {
        // Get info again for logging clarity
        RE::EffectSetting* baseMGEF = effectToDispel->effect->baseEffect;
        const char* mgefName = baseMGEF->GetName();
        if (!mgefName || mgefName[0] == '\0') {
            mgefName = "Unnamed Effect";
        }
        RE::FormID baseFormID = baseMGEF->GetFormID();

        SKSE::log::info("Dispel(true) called on: {:#010x} ({}) Instance: {}", baseFormID, mgefName,
                        (void*)effectToDispel);

        // --- THIS IS THE KEY ---
        // Use Dispel(true) to attempt removal.
        effectToDispel->Dispel(true);  // true = force dispel
        // --- DO NOT CALL Finish(), EffectRemoved(), or modify duration/flags manually ---

        dispelCount++;
    }
    SKSE::log::info("--- Dispel calls finished for {} effects. ---", dispelCount);
    SKSE::log::info("Note: The effect list might take a frame or moment to fully update in-game.");

    // Optional: Log the list *after* the dispel calls (may not reflect immediate removal)
    SKSE::log::info("--- Active Effects on Player immediately after Dispel calls ---");
    // Re-fetch the list in case the pointer or structure changed (though usually not necessary just for reading)
    activeEffects = magicTarget->GetActiveEffectList();
    if (!activeEffects || activeEffects->empty()) {
        SKSE::log::info("  Player list is now null or empty.");
    } else {
        for (RE::ActiveEffect* activeEffect : *activeEffects) {
            if (!activeEffect || !activeEffect->effect || !activeEffect->effect->baseEffect) continue;
            const char* name = activeEffect->effect->baseEffect->GetName();
            if (!name || name[0] == '\0') name = "Unnamed Effect";
            SKSE::log::info("  - Post-Dispel Effect: {:#010x} ({}) - Instance Ptr: {}",
                            activeEffect->effect->baseEffect->GetFormID(), name, (void*)activeEffect);
        }
    }
    SKSE::log::info("--- End of Post-Dispel Effect List ---");

    return true;
}

void DispellAllSavedSpellsFromActor(RE::Actor& actor) {
    const SpellEffectsMap& savedSpells = SpellDataPersistence::GetAllSavedSpells();
    std::unordered_set<RE::FormID> flattenedSpellData = SpellDataPersistence::FlattenSpellEffectsMapToSet(savedSpells);

    RE::MagicTarget* magicTarget = actor.GetMagicTarget();
    if (!magicTarget) {
        SKSE::log::warn("DispellAllSavedSpellsFromActor: Actor has no MagicTarget.");
        return;
    }

    RE::BSSimpleList<RE::ActiveEffect*>* activeEffects = magicTarget->GetActiveEffectList();
    if (!activeEffects || activeEffects->empty()) {
        SKSE::log::info("DispellAllSavedSpellsFromActor: Actor has no active effects.");
        return;
    }
    // Iterate over the active effects and process them
    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject()) {
            continue;
        }

        RE::FormID effectFormID = activeEffect->GetBaseObject()->GetFormID();
        // Check if the effect is in the saved spell data
        if (flattenedSpellData.find(effectFormID) == flattenedSpellData.end()) {
            continue;
        }

        RE::FormID linkedSpellFormId = activeEffect->spell->GetFormID();
        // If the effect is linked to a saved spell - Reset duration
        if (savedSpells.find(linkedSpellFormId) != savedSpells.end()) {
            SKSE::log::info("Found active effect with form ID: {:#010x}. Dispel it.", effectFormID);
            activeEffect->Dispel(false);  // Remove the effect from the actor
            // SpellDataPersistence::RemoveSpellFromSave(linkedSpellFormId);  // Remove the spell from the save
        } else {
            SKSE::log::warn("  - Active effect not linked to a saved spell: {:#010x} - {}", linkedSpellFormId, 
                            activeEffect->spell->GetName());
            }
        }
}

void DispellAllSavedSpellsFromPlayer() {
    RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::warn("SpellCastEvent: Player handle invalid");
        return;
    }
    DispellAllSavedSpellsFromActor(*player);
}

void ApplyAllSavedSpellsToActor(RE::Actor& actor) {
    const SpellEffectsMap& savedSpells = SpellDataPersistence::GetAllSavedSpells();

    SKSE::log::info("-------------------Active Effects on Player on load:-------------------");
    RE::MagicTarget* magicTarget = actor.GetMagicTarget();
    if (!magicTarget) {
        SKSE::log::warn("ApplyAllSavedSpellsToActor: Actor has no MagicTarget.");
        return;
    }

    RE::BSSimpleList<RE::ActiveEffect*>* activeEffects = magicTarget->GetActiveEffectList();
    if (!activeEffects || activeEffects->empty()) {
        SKSE::log::info("ApplyAllSavedSpellsToActor: Actor has no active effects.");
        return;
    }

    SKSE::log::info("-------------------Active Effects on Player on load complete.-------------------");

    std::unordered_set<RE::FormID> flattenedSpellData = SpellDataPersistence::FlattenSpellEffectsMapToSet(savedSpells);
    SpellEffectsMap AllSavedSpells = SpellDataPersistence::GetAllSavedSpells();
    std::unordered_set<RE::FormID> appliedSpellsIDs;

    // std::unordered_set<RE::FormID> SpellsFAppliedRulesToFormID;
    // Iterate over the active effects and process them
    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject()) {
            continue;
        }

        RE::FormID effectFormID = activeEffect->GetBaseObject()->GetFormID();
        // SKSE::log::info("  - Active Effect: {:#010x} - {}", effectFormID, activeEffect->GetBaseObject()->GetName());

        // Check if the effect is in the saved spell data
        if (flattenedSpellData.find(effectFormID) == flattenedSpellData.end()) {
            continue;
        }

        RE::FormID linkedSpellFormId = activeEffect->spell->GetFormID();
        RE::SpellItem* spellItem = activeEffect->spell->As<RE::SpellItem>();
        if (spellItem) {
            bool isRuleApplied = ApplyConfigRulesToActiveEffect(activeEffect);  // Apply config rules to the spell
            if (isRuleApplied) {
                appliedSpellsIDs.insert(linkedSpellFormId);  // Add to the list of applied spells
                logger::info("ApplyConfigRulesToSpell: Spell rules applied to active effect: {:#010x} - {}. Spell: {}",
                             effectFormID, activeEffect->GetBaseObject()->GetName(), spellItem->GetName());
                // SpellsFAppliedRulesToFormID.insert(linkedSpellFormId);  // Add to the set of applied rules
                continue;  // Skip to the next effect if rules were applied successfully
            }
        } else {
            SKSE::log::warn("ApplyConfigRulesToSpell: MagicItem is not a SpellItem.");
        }

        // If the effect is linked to a saved spell - Reset duration
        if (AllSavedSpells.find(linkedSpellFormId) != AllSavedSpells.end()) {
            SKSE::log::info("Found active effect with form ID: {:#010x}. Resetting duration.", effectFormID);
            activeEffect->duration = permanentSpellDuration;  // Set to permanent duration
            activeEffect->elapsedSeconds = 0.0f;              // Reset elapsed time
            appliedSpellsIDs.insert(linkedSpellFormId);       // Add to the list of applied spells

        } else {
            SKSE::log::warn("  - Active effect not linked to a saved spell: {:#010x} - {}", linkedSpellFormId,
                            activeEffect->spell->GetName());
        }
    }

    // If not all saved spells were applied, then clean the saved spells of the ones not found.
    if (appliedSpellsIDs.size() != AllSavedSpells.size()) {
        SKSE::log::info("Not all saved spells were applied to the player. {} out of {} spells applied.",
                        appliedSpellsIDs.size(), AllSavedSpells.size());
        std::vector<RE::FormID> notAppliedSpellsIDs;  // Create a copy of the vector

        // Iterate over the keys in the map
        for (const auto& [key, value] : AllSavedSpells) {
            // Check if the key is not in the set
            if (appliedSpellsIDs.find(key) == appliedSpellsIDs.end()) {
                notAppliedSpellsIDs.push_back(key);  // Add the missing key to the result
            }
        }

        for (const auto& formId : notAppliedSpellsIDs) {
            SKSE::log::info("Spell ({:#010x}) was not applied to the player. Removing from save.", formId);
            SpellDataPersistence::RemoveSpellFromSave(formId);  // Remove the spell from the save
        }
    } else {
        SKSE::log::info("All saved spells were applied to the player.");
    }

    SKSE::log::info("Finished applying permanent spells to player.");
}

void ApplyAllSavedPermanentSpellsToPlayer() {
    RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::warn("SpellCastEvent: Player handle invalid");
        return;
    }
    ApplyAllSavedSpellsToActor(*player);
}

// Function to log active effects for debugging
void LogActiveEffectDetails(RE::ActiveEffect* activeEffect) {
    if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject()) {
        return;
    }

    RE::EffectSetting* mgef = activeEffect->GetBaseObject();
    RE::Effect* spellEffectEntry = activeEffect->effect;
    const char* mgefName = mgef->GetName();
    if (!mgefName || mgefName[0] == '\0') {
        mgefName = "Unnamed Effect";
    }

    SKSE::log::info("  -> Applied Effect Found:");
    SKSE::log::info("      Name: {}", mgefName);
    SKSE::log::info("      MGEF ID: {:#010x}", mgef->GetFormID());
    SKSE::log::info("      Spell Duration: {}", spellEffectEntry->effectItem.duration);
    SKSE::log::info("      Spell Magnitude: {}", spellEffectEntry->effectItem.magnitude);
    SKSE::log::info("      Spell Area: {}", spellEffectEntry->effectItem.area);
    SKSE::log::info("      Active Duration (Remaining): {:.2f}", activeEffect->duration);
    SKSE::log::info("      Active Magnitude: {:.2f}", activeEffect->magnitude);
    SKSE::log::info("      Elapsed Time: {:.2f}s", activeEffect->elapsedSeconds);
}

// Function to handle saved spells
void HandleSavedSpell(RE::ActiveEffect* activeEffect, const SpellCastInfo& castInfo, const char* spellName) {
    RE::EffectSetting* mgef = activeEffect->GetBaseObject();
    const char* mgefName = mgef->GetName();
    if (!mgefName || mgefName[0] == '\0') {
        mgefName = "Unnamed Effect";
    }

    // float effectDuration = permanentSpellDuration;
    // auto localSpellRules = GetSpellRules();
    // auto it = localSpellRules.find(activeEffect->spell->GetName());
    // if (it != localSpellRules.end()) {
    //     SpellRule spellRule = it->second;
    //     effectDuration = spellRule.durationFilter;  // Get the duration from the rule
    // }

    // logger::info("Checking effect: {:#010x} ({})", activeEffect->GetBaseObject()->GetFormID(), mgefName);
    // logger::info("Effect Duration: {}", effectDuration);
    // logger::info("Active Effect Duration: {}", activeEffect->duration);
    // logger::info("Active Effect Elapsed Time: {}", activeEffect->elapsedSeconds);
    // std::abs(activeEffect->duration - effectDuration) > 0.5f ? logger::info("Effect Duration is different")
    //                                                          : logger::info("Effect Duration is the same");

    // if (std::abs(activeEffect->duration - effectDuration) > 0.5f &&
    //     activeEffect->elapsedSeconds == 0) {      // Check if the duration is different
    //     activeEffect->duration = effectDuration;  // Set to permanent duration or the configured duration
    //     activeEffect->elapsedSeconds = 0.0f;      // Reset elapsed time
    //     logger::info("Setting duration to {} for effect: {:#010x} ({})", effectDuration,
    //                  activeEffect->GetBaseObject()->GetFormID(), mgefName);
    //     return;
    // }

    // logger::info("-----------------Dispelling effect from saved spell: {:#010x} ({})-----------------",
    //              activeEffect->GetBaseObject()->GetFormID(), mgefName);
    if (castInfo.alreadyOnPlayer) {
        SKSE::log::info("Spell '{}' ({:#010x}) is already on player. Dispel it.", spellName,
                        castInfo.spellItem.GetFormID());
        activeEffect->Dispel(false);  // Remove the effect from the actor
        SpellDataPersistence::RemoveSpellFromSave(castInfo.spellItem.GetFormID());
        SKSE::log::info("Spell '{}' ({:#010x}) is no longer saved.", spellName, castInfo.spellItem.GetFormID());
    } else {
        // The spell is already cached but has been dispelled already
        bool appliedConfig = ApplyConfigRulesToActiveEffect(activeEffect);
        if (!appliedConfig) {
            activeEffect->duration = permanentSpellDuration;
        }
        SKSE::log::info("Spell '{}' ({:#010x}) is not on player. Apply it.", spellName, castInfo.spellItem.GetFormID());
    }
}

// Helper function to check if an active effect is temporary
bool IsTemporaryEffect(RE::ActiveEffect* activeEffect) {
    return activeEffect->duration > 0.0f && activeEffect->duration < permanentSpellDuration;
}
// Function to handle unsaved spells
void HandleUnsavedSpell(RE::ActiveEffect* activeEffect, const SpellCastInfo& castInfo) {
    bool appliedConfig = ApplyConfigRulesToActiveEffect(activeEffect);
    if (!appliedConfig && IsTemporaryEffect(activeEffect)) {
        activeEffect->duration = permanentSpellDuration;
    }
    SpellDataPersistence::CacheSpellForSaving(&castInfo.spellItem);
}


// Main function to convert effects to permanent on the player
void ConvertToPermanentEffectOnPlayer(SpellCastInfo castInfo) {
    if (!castInfo.playerHandle) {
        SKSE::log::warn("CheckAppliedEffects: Player handle is null.");
        return;
    }

    RE::Actor* player = castInfo.playerHandle.get().get();
    if (!player) {
        SKSE::log::warn("CheckAppliedEffects: Player handle invalid on next frame.");
        return;
    }

    RE::MagicTarget* magicTarget = player->GetMagicTarget();
    if (!magicTarget) {
        return;
    }

    RE::BSSimpleList<RE::ActiveEffect*>* activeEffects = magicTarget->GetActiveEffectList();
    if (!activeEffects) {
        SKSE::log::warn("CheckAppliedEffects: Player has no ActiveEffects list.");
        return;
    }

    // int activeEffectsCount = 0;
    // for (auto* effect : *activeEffects) {
    //     if (effect) {
    //         ++activeEffectsCount;
    //     }
    // }
    // logger::info("active effects size: {}", activeEffectsCount);

    const char* spellName = castInfo.spellItem.GetName();
    if (!spellName || spellName[0] == '\0') {
        spellName = "Unnamed Spell";
    }

    SKSE::log::info("Checking applied effects for spell '{}' ({:#010x}) cast last frame...", spellName,
                    castInfo.spellItem.GetFormID());

    bool isSpellSaved = SpellDataPersistence::IsSpellSaved(castInfo.spellItem.GetFormID());
    if (isSpellSaved) {
        SKSE::log::info("Spell '{}' ({:#010x}) is saved.", spellName, castInfo.spellItem.GetFormID());
    }

    // LogAllActiveEffectsOnActor(*player);
    size_t count = 0;
    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject() ||
            !activeEffect->target || !activeEffect->caster) {
            continue;
        }

        if (activeEffect->spell->GetFormID() == castInfo.spellItem.GetFormID() &&
            activeEffect->caster == castInfo.playerHandle && activeEffect->target->MagicTargetIsActor() &&
            activeEffect->target == player->GetMagicTarget()) {
            LogActiveEffectDetails(activeEffect);

            if (isSpellSaved) {
                HandleSavedSpell(activeEffect, castInfo, spellName);
            } else {
                HandleUnsavedSpell(activeEffect, castInfo);
            }
        }
        count++;
    }
}