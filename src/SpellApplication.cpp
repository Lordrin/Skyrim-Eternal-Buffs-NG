#include "SpellApplication.h"

#include <SpellUtilities.h>

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

    SpellRule spellRule;
    if (GetSpellRuleForActiveEffect(activeEffect, spellRule)) {
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
        logger::trace("  - Active Effect: {:#010x} - {}", effectFormID, activeEffect->GetBaseObject()->GetName());

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
            activeEffect->duration = Config::GetSingleton().GetPermanentSpellDuration();  // Set to permanent duration
            activeEffect->elapsedSeconds = 0.0f;                                          // Reset elapsed time
            appliedSpellsIDs.insert(linkedSpellFormId);  // Add to the list of applied spells

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

void HandleSavedSpell(RE::ActiveEffect* activeEffect, const SpellCastInfo& castInfo, const char* spellName,
                      bool isSummonSpell) {
    // --- Initial Setup ---
    RE::EffectSetting* mgef = activeEffect->GetBaseObject();
    if (!mgef) {
        logger::error("HandleSavedSpell: ActiveEffect has no base MGEF!");
        return;
    }
    const char* mgefName = mgef->GetName();
    if (!mgefName || mgefName[0] == '\0') {
        mgefName = "Unnamed Effect";  // Use placeholder if name is missing
    }

    RE::SpellItem* spellItem = castInfo.spellItem;
    if (!spellItem) {
        logger::error("HandleSavedSpell: castInfo contains null spellItem!");
        return;
    }

    SpellRule spellRule = GetSpellRuleForActiveEffect(activeEffect);  // Get rules specific to this effect

    // --- Determine Required Action ---
    SpellHandlingAction finalAction;
    bool shouldConsiderDispel = castInfo.alreadyOnPlayer;  // Start with whether it was saved previously

    // Specific logic override for Summon Spells based on toggle key
    if (isSummonSpell) {
        bool keyHeld = Config::GetSingleton().GetToggleKeyHeld();
        if (keyHeld) {
            // If it's a summon and the key is held, we INTEND to dispel it, overriding 'alreadyOnPlayer'.
            // The original loop checking effect->baseEffect seems overly complex if we assume
            // activeEffect is indeed an instance of an effect from spellItem.
            logger::debug("Summon spell '{}' ({:#010x}) and toggle key Held. Marking for potential dispel.", spellName,
                          spellItem->GetFormID());
            shouldConsiderDispel = true;
        } else {
            // If it's a summon and the key is NOT held, we explicitly DO NOT want to dispel it.
            logger::debug("Summon spell '{}' ({:#010x}) and toggle key Not Held. Marking to keep.", spellName,
                          spellItem->GetFormID());
            shouldConsiderDispel = false;
        }
    }

    // Final decision: Can we actually dispel based on the rule?
    if (shouldConsiderDispel && spellRule.toggleable) {
        // We want to dispel (either originally on player or summon+key) AND the rule allows toggling.
        finalAction = SpellHandlingAction::kDispel;
        logger::debug("Determined Action for '{}': Dispel (Toggleable: {}, Initially Considered Dispel: {})", spellName,
                     spellRule.toggleable, shouldConsiderDispel);
    } else {
        // We either didn't want to dispel initially, OR we wanted to but the rule prevents toggling it off.
        finalAction = SpellHandlingAction::kApplyConfig;
        if (shouldConsiderDispel && !spellRule.toggleable) {
            logger::debug(
                "Determined Action for '{}': Apply Config (Toggleable: {}, Initially Considered Dispel: {}, Rule "
                "prevents toggle off)",
                spellName, spellRule.toggleable, shouldConsiderDispel);
        } else {
            logger::debug("Determined Action for '{}': Apply Config (Toggleable: {}, Initially Considered Dispel: {})",
                         spellName, spellRule.toggleable, shouldConsiderDispel);
        }
    }

    // --- Execute Action ---
    bool appliedConfig = false;
    switch (finalAction) {
        case SpellHandlingAction::kDispel:
            logger::debug("Executing Dispel for spell '{}' ({:#010x}).", spellName, spellItem->GetFormID());
            activeEffect->Dispel(false);  // false = No Hit Effects/Sound
            SpellDataPersistence::RemoveSpellFromSave(spellItem->GetFormID());
            logger::debug("Spell '{}' ({:#010x}) dispelled and removed from save.", spellName, spellItem->GetFormID());
            break;

        case SpellHandlingAction::kApplyConfig:
            logger::debug("Executing Apply Config / Keep for spell '{}' ({:#010x}).", spellName,
                          spellItem->GetFormID());
            // Try applying specific rules first
            appliedConfig = ApplyConfigRulesToActiveEffect(activeEffect);
            // If no specific rule applied, set the default permanent duration
            if (!appliedConfig) {
                activeEffect->duration = Config::GetSingleton().GetPermanentSpellDuration();
                logger::debug("No specific config rule applied, setting default duration {} for '{}'.",
                              activeEffect->duration, spellName);
            } else {
                logger::debug("Specific config rule applied to '{}'.", spellName);
            }
            break;

        default:
            logger::debug("Unhandled SpellHandlingAction for spell '{}'", spellName);
            break;
    }
}

bool IsTemporaryEffect(RE::ActiveEffect* activeEffect) {
    return activeEffect->duration > 0.0f && activeEffect->duration < Config::GetSingleton().GetPermanentSpellDuration();
}

void HandleUnsavedSpell(RE::ActiveEffect* activeEffect, const SpellCastInfo& castInfo) {
    bool appliedConfig = ApplyConfigRulesToActiveEffect(activeEffect);
    if (!appliedConfig && IsTemporaryEffect(activeEffect)) {
        activeEffect->duration = Config::GetSingleton().GetPermanentSpellDuration();
    }
    SpellDataPersistence::CacheSpellForSaving(castInfo.spellItem);
}

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

    RE::SpellItem* spellItem = castInfo.spellItem;

    if (!spellItem) {
        logger::warn("CheckAppliedEffects: SpellItem became null during the UI update.");
        return;
    }

    const char* spellName = spellItem->GetName();
    if (!spellName || spellName[0] == '\0') {
        spellName = "Unnamed Spell";
    }

    logger::debug("Checking applied effects for spell '{}' ({:#010x}) cast last frame...", spellName,
                  spellItem->GetFormID());

    bool isSummonSpell = IsSummonSpell(spellItem);

    if (isSummonSpell && !Config::GetSingleton().GetGeneralRule().spellsEnabled) {
        logger::info("Detected Summon Spell: {} ({:#010x}). Summons are disabled. Returning Early",
                     spellItem->GetName(), spellItem->GetFormID());
        return;
    }

    bool isSpellSaved = SpellDataPersistence::IsSpellSaved(spellItem->GetFormID());
    if (isSpellSaved) {
        logger::debug("Spell '{}' ({:#010x}) is saved.", spellName, spellItem->GetFormID());
    }

    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject() ||
            !activeEffect->target || !activeEffect->caster) {
            continue;
        }

        if (activeEffect->spell->GetFormID() == spellItem->GetFormID() &&
            activeEffect->caster == castInfo.playerHandle && activeEffect->target->MagicTargetIsActor() &&
            activeEffect->target == player->GetMagicTarget()) {
            LogActiveEffectDetails(activeEffect);

            if (isSpellSaved) {
                HandleSavedSpell(activeEffect, castInfo, spellName, isSummonSpell);
            } else {
                HandleUnsavedSpell(activeEffect, castInfo);
            }
        }
    }
}