#include "SpellApplication.h"

/**
 * @brief Applies configuration rules to an active effect.
 * @param activeEffect The active effect to apply rules to.
 * @return True if the rules were applied successfully, false otherwise.
 * @note This function checks if the active effect's spell matches any rules in the configuration.
 */
bool ApplyConfigRulesToActiveEffect(RE::ActiveEffect* activeEffect) {
    if (!activeEffect) {
        logger::warn("ApplyConfigRulesToActiveEffect: activeEffect is null");
        return false;
    }

    auto spellRuleIt = Global::GetSpellRules().find(Utilities::RemoveWhitespace(activeEffect->spell->GetFullName()));
    if (spellRuleIt != Global::GetSpellRules().end()) {
        SpellRule spellRule = spellRuleIt->second;
        spellRule.ShouldApplyRuleToSpell(activeEffect->spell->As<RE::SpellItem>());
        spellRule.ApplySpellRulesToActiveEffect(activeEffect);
        return true;

    } else {
        logger::debug("ApplyConfigRulesToSpell: Spell {} not found in rules.", activeEffect->spell->GetName());
        return false;
    }
}

void DispellAllSavedSpellsFromActor(RE::Actor& actor) {
    const SpellEffectsMap& savedSpells = SpellDataPersistence::GetAllSavedSpells();
    std::unordered_set<RE::FormID> flattenedSpellData = SpellDataPersistence::FlattenSpellEffectsMapToSet(savedSpells);

    RE::MagicTarget* magicTarget = actor.GetMagicTarget();
    if (!magicTarget) {
        logger::warn("DispellAllSavedSpellsFromActor: Actor has no MagicTarget.");
        return;
    }

    RE::BSSimpleList<RE::ActiveEffect*>* activeEffects = magicTarget->GetActiveEffectList();
    if (!activeEffects || activeEffects->empty()) {
        logger::info("DispellAllSavedSpellsFromActor: Actor has no active effects.");
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
            logger::debug("Found active effect with form ID: {:#010x}. Dispel it.", effectFormID);
            activeEffect->Dispel(false);  // Remove the effect from the actor
        } else {
            logger::warn("  - Active effect not linked to a saved spell: {:#010x} - {}", linkedSpellFormId,
                         activeEffect->spell->GetName());
        }
    }
}

void DispellAllSavedSpellsFromPlayer() {
    RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        logger::warn("SpellCastEvent: Player handle invalid");
        return;
    }
    DispellAllSavedSpellsFromActor(*player);
}

void ApplyAllSavedSpellsToActor(RE::Actor& actor) {
    const SpellEffectsMap& savedSpells = SpellDataPersistence::GetAllSavedSpells();

    RE::MagicTarget* magicTarget = actor.GetMagicTarget();
    if (!magicTarget) {
        logger::warn("ApplyAllSavedSpellsToActor: Actor has no MagicTarget.");
        return;
    }

    RE::BSSimpleList<RE::ActiveEffect*>* activeEffects = magicTarget->GetActiveEffectList();
    if (!activeEffects || activeEffects->empty()) {
        logger::info("ApplyAllSavedSpellsToActor: Actor has no active effects.");
        return;
    }

    logger::debug("-------------------Active Effects on Player on load complete.-------------------");

    std::unordered_set<RE::FormID> flattenedSpellData = SpellDataPersistence::FlattenSpellEffectsMapToSet(savedSpells);
    SpellEffectsMap AllSavedSpells = SpellDataPersistence::GetAllSavedSpells();
    std::unordered_set<RE::FormID> appliedSpellsIDs;

    // Iterate over the active effects and process them
    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject()) {
            continue;
        }

        RE::FormID effectFormID = activeEffect->GetBaseObject()->GetFormID();
        logger::debug("  - Active Effect: {:#010x} - {}", effectFormID, activeEffect->GetBaseObject()->GetName());

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
                logger::debug("ApplyConfigRulesToSpell: Spell rules applied to active effect: {:#010x} - {}. Spell: {}",
                              effectFormID, activeEffect->GetBaseObject()->GetName(), spellItem->GetName());
                continue;  // Skip to the next effect if rules were applied successfully
            }
        } else {
            logger::warn("ApplyConfigRulesToSpell: MagicItem is not a SpellItem.");
        }

        // If the effect is linked to a saved spell - Reset duration
        if (AllSavedSpells.find(linkedSpellFormId) != AllSavedSpells.end()) {
            logger::debug("Found active effect with form ID: {:#010x}. Resetting duration.", effectFormID);
            activeEffect->duration = Global::permanentSpellDuration;  // Set to permanent duration
            activeEffect->elapsedSeconds = 0.0f;                      // Reset elapsed time
            appliedSpellsIDs.insert(linkedSpellFormId);               // Add to the list of applied spells

        } else {
            logger::warn("  - Active effect not linked to a saved spell: {:#010x} - {}", linkedSpellFormId,
                         activeEffect->spell->GetName());
        }
    }

    // If not all saved spells were applied, then clean the saved spells of the ones not found.
    if (appliedSpellsIDs.size() != AllSavedSpells.size()) {
        logger::debug("Not all saved spells were applied to the player. {} out of {} spells applied.",
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
            logger::debug("Spell ({:#010x}) was not applied to the player. Removing from save.", formId);
            SpellDataPersistence::RemoveSpellFromSave(formId);  // Remove the spell from the save
        }
    } else {
        logger::debug("All saved spells were applied to the player.");
    }

    logger::info("Finished applying permanent spells to player.");
}

void ApplyAllSavedPermanentSpellsToPlayer() {
    RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        logger::warn("SpellCastEvent: Player handle invalid");
        return;
    }
    ApplyAllSavedSpellsToActor(*player);
}

// Function to log active effects for debugging
void LogActiveEffectDetails(RE::ActiveEffect* activeEffect) {
    if (spdlog::get_level() < spdlog::level::debug) {
        return;
    }
    if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject()) {
        return;
    }

    RE::EffectSetting* mgef = activeEffect->GetBaseObject();
    RE::Effect* spellEffectEntry = activeEffect->effect;
    const char* mgefName = mgef->GetName();
    if (!mgefName || mgefName[0] == '\0') {
        mgefName = "Unnamed Effect";
    }

    logger::debug("  -> Applied Effect Found:");
    logger::debug("      Name: {}", mgefName);
    logger::debug("      MGEF ID: {:#010x}", mgef->GetFormID());
    logger::debug("      Spell Duration: {}", spellEffectEntry->effectItem.duration);
    logger::debug("      Spell Magnitude: {}", spellEffectEntry->effectItem.magnitude);
    logger::debug("      Spell Area: {}", spellEffectEntry->effectItem.area);
    logger::debug("      Active Duration (Remaining): {:.2f}", activeEffect->duration);
    logger::debug("      Active Magnitude: {:.2f}", activeEffect->magnitude);
    logger::debug("      Elapsed Time: {:.2f}s", activeEffect->elapsedSeconds);
}

// Function to handle saved spells
void HandleSavedSpell(RE::ActiveEffect* activeEffect, const SpellCastInfo& castInfo, const char* spellName) {
    RE::EffectSetting* mgef = activeEffect->GetBaseObject();
    const char* mgefName = mgef->GetName();
    if (!mgefName || mgefName[0] == '\0') {
        mgefName = "Unnamed Effect";
    }

    if (castInfo.alreadyOnPlayer) {
        logger::debug("Spell '{}' ({:#010x}) is already on player. Dispel it.", spellName,
                      castInfo.spellItem.GetFormID());
        activeEffect->Dispel(false);  // Remove the effect from the actor
        SpellDataPersistence::RemoveSpellFromSave(castInfo.spellItem.GetFormID());
        logger::debug("Spell '{}' ({:#010x}) is no longer saved.", spellName, castInfo.spellItem.GetFormID());
    } else {
        // The spell is already cached but has been dispelled already
        bool appliedConfig = ApplyConfigRulesToActiveEffect(activeEffect);
        if (!appliedConfig) {
            activeEffect->duration = Global::permanentSpellDuration;
        }
        logger::debug("Spell '{}' ({:#010x}) is not on player. Apply it.", spellName, castInfo.spellItem.GetFormID());
    }
}

// Helper function to check if an active effect is temporary
bool IsTemporaryEffect(RE::ActiveEffect* activeEffect) {
    return activeEffect->duration > 0.0f && activeEffect->duration < Global::permanentSpellDuration;
}
// Function to handle unsaved spells
void HandleUnsavedSpell(RE::ActiveEffect* activeEffect, const SpellCastInfo& castInfo) {
    bool appliedConfig = ApplyConfigRulesToActiveEffect(activeEffect);
    if (!appliedConfig && IsTemporaryEffect(activeEffect)) {
        activeEffect->duration = Global::permanentSpellDuration;
    }
    SpellDataPersistence::CacheSpellForSaving(&castInfo.spellItem);
}

// Main function to convert effects to permanent on the player
void ConvertToPermanentEffectOnPlayer(SpellCastInfo castInfo) {
    if (!castInfo.playerHandle) {
        logger::warn("CheckAppliedEffects: Player handle is null.");
        return;
    }

    RE::Actor* player = castInfo.playerHandle.get().get();
    if (!player) {
        logger::warn("CheckAppliedEffects: Player handle invalid on next frame.");
        return;
    }

    RE::MagicTarget* magicTarget = player->GetMagicTarget();
    if (!magicTarget) {
        return;
    }

    RE::BSSimpleList<RE::ActiveEffect*>* activeEffects = magicTarget->GetActiveEffectList();
    if (!activeEffects) {
        logger::warn("CheckAppliedEffects: Player has no ActiveEffects list.");
        return;
    }

    const char* spellName = castInfo.spellItem.GetName();
    if (!spellName || spellName[0] == '\0') {
        spellName = "Unnamed Spell";
    }

    logger::debug("Checking applied effects for spell '{}' ({:#010x}) cast last frame...", spellName,
                  castInfo.spellItem.GetFormID());

    bool isSpellSaved = SpellDataPersistence::IsSpellSaved(castInfo.spellItem.GetFormID());
    if (isSpellSaved) {
        logger::debug("Spell '{}' ({:#010x}) is saved.", spellName, castInfo.spellItem.GetFormID());
    }

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
    }
}