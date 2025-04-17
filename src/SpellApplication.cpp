#include "SpellApplication.h"

constexpr RE::FormID MyPermanentAbilitySpellFormID = 0x000800;
const char* MyPluginName = "LoricaNG.esl";
const float permanentSpellDuration = 86313600.0f;

void ApplyConfigRulesToSpell(RE::SpellItem* spell) {
    if (!spell) {
        SKSE::log::warn("ApplyConfigRulesToSpell: Spell is null");
        return;
    }

    // Check if the spell is a permanent ability
    if (spell->GetFormID() == MyPermanentAbilitySpellFormID) {
        SKSE::log::info("ApplyConfigRulesToSpell: Permanent ability spell detected, setting duration to {}",
                        permanentSpellDuration);
        // spell->SetDuration(permanentSpellDuration);
    } else {
        SKSE::log::info("ApplyConfigRulesToSpell: Non-permanent ability spell detected, no changes made.");
    }
}

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

    // SpellDataPersistence::spellRules

    // Iterate over the active effects and process them
    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject()) {
            continue;
        }

        RE::FormID effectFormID = activeEffect->GetBaseObject()->GetFormID();
        SKSE::log::info("  - Active Effect: {:#010x} - {}", effectFormID, activeEffect->GetBaseObject()->GetName());

        // Check if the effect is in the saved spell data
        if (flattenedSpellData.find(effectFormID) == flattenedSpellData.end()) {
            continue;
        }

        RE::FormID linkedSpellFormId = activeEffect->spell->GetFormID();
        if (auto* spellItem = activeEffect->spell->As<RE::SpellItem>()) {
            ApplyConfigRulesToSpell(spellItem);  // Apply config rules to the spell
        } else {
            SKSE::log::warn("ApplyConfigRulesToSpell: MagicItem is not a SpellItem.");
        }
        auto spellRuleIt = SpellDataPersistence::spellRules.find(activeEffect->spell->GetFullName());

        // If the spell is in the SpellDataPersistence::spellRules - Apply rules
        if (spellRuleIt != SpellDataPersistence::spellRules.end()) {
            const auto& spellConfig = spellRuleIt->second;

            if (!spellConfig.isPermanentEnabled) {
                SKSE::log::info("Found spell '{}' in config. Dispelling effect.", activeEffect->spell->GetFullName());
                auto allSpells = SpellDataPersistence::GetAllSavedSpells();
                if (allSpells.find(linkedSpellFormId) != allSpells.end()) {
                    activeEffect->Dispel(false); 
                    SpellDataPersistence::RemoveSpellFromSave(linkedSpellFormId);
                }
                continue;
            }

            // TODO add all rules here
            // default for uint32_t = 3435973836
        }
        // If the effect is linked to a saved spell - Reset duration
        if (AllSavedSpells.find(linkedSpellFormId) != AllSavedSpells.end()) {
            SKSE::log::info("Found active effect with form ID: {:#010x}. Resetting duration.", effectFormID);
            // activeEffect->effect->effectItem.duration = 0;    // Set to 0 to hide timer --- This alters the base
            // effect. Alters ALL instances of the effect.
            activeEffect->duration = permanentSpellDuration;  // Set to permanent duration
            activeEffect->elapsedSeconds = 0.0f;              // Reset elapsed time

            // reset the effectItem duration
            // auto formID = activeEffect->effect->baseEffect->GetFormID();
            // auto form = RE::TESForm::LookupByID(formID);
            // if (form) {
            //     auto effectSetting = form->As<RE::EffectSetting>();
            //     if (effectSetting) {
            //         logger::info("the duration after reading the form: {}", effectSetting->data);
            //         // effectSetting->effectItem.duration = permanentSpellDuration;  // Set to permanent duration
            //     }
            // }
            // activeEffect->effect->effectItem.duration = permanentSpellDuration;  // Set to permanent duration

            // auto spellForm = RE::TESForm::LookupByID(linkedSpellFormId);
            // if (spellForm) {
            //     auto spellItem = spellForm->As<RE::SpellItem>();
            //     if (spellItem) {
            //         logger::info("the duration after reading the form: {}", spellItem->GetLongestDuration());
            //         for(auto& effect : spellItem->effects) {
            //             if (effect) {
            //                 logger::info("                 --- the duration after reading the form: {}",
            //                 effect->effectItem.duration);
            //                 // effect->effectItem.duration = permanentSpellDuration;  // Set to permanent duration
            //             }
            //         }
            //     }
            // }

            // RE::SpellItem* spellItem = activeEffect->spell->As<RE::SpellItem>();
            // if(spellItem) {
            //     DisableTimerOfEffectsOnSpell(*spellItem);  // Disable timer for the spell
            // }
        } else {
            SKSE::log::warn("  - Active effect not linked to a saved spell: {:#010x} - {}", linkedSpellFormId,
                            activeEffect->spell->GetName());
        }
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

// The function that will perform the check on the next frame
void ConvertToPermanentEffectOnPlayer(SpellCastInfo castInfo) {
    // --- Re-acquire Player and Spell ---
    if (!castInfo.playerHandle) {
        SKSE::log::warn("CheckAppliedEffects: Player handle is null.");
        return;
    }
    RE::Actor* player = castInfo.playerHandle.get().get();  // Get TESObjectREFR*, then Actor*
    if (!player) {
        SKSE::log::warn("CheckAppliedEffects: Player handle invalid on next frame.");
        return;
    }

    RE::MagicTarget* magicTarget = player->GetMagicTarget();
    if (!magicTarget) {
        return;
    }

    // --- Check Active Effects on the Player (Same logic as before) ---
    RE::BSSimpleList<RE::ActiveEffect*>* activeEffects = magicTarget->GetActiveEffectList();  // GetActiveEffectList();
    if (!activeEffects) {
        SKSE::log::warn("CheckAppliedEffects: Player has no ActiveEffects list.");
        return;
    }
    int activeEffectsCount = 0;
    for (auto* effect : *activeEffects) {
        if (effect) {
            ++activeEffectsCount;
        }
    }
    logger::info("active effects size: {}", activeEffectsCount);

    bool foundAppliedEffect = false;
    // Reduce threshold slightly, as we are checking *after* the event frame
    // const float timeThreshold = 0.5f;  // Allow a bit more time passage

    const char* spellName = castInfo.spellItem.GetName();
    if (!spellName || spellName[0] == '\0') {
        spellName = "Unnamed Spell";
    }
    // Log context message here, closer to the actual check
    SKSE::log::info("Checking applied effects for spell '{}' ({:#010x}) cast last frame...", spellName,
                    castInfo.spellItem.GetFormID());

    bool isSpellSaved = SpellDataPersistence::IsSpellSaved(castInfo.spellItem.GetFormID());

    if (isSpellSaved) {
        SKSE::log::info("Spell '{}' ({:#010x}) is saved.", spellName, castInfo.spellItem.GetFormID());
    }

    LogAllActiveEffectsOnActor(*player);  // Log active effects for debugging

    for (RE::ActiveEffect* activeEffect : *activeEffects) {
        if (!activeEffect || !activeEffect->spell || !activeEffect->effect || !activeEffect->GetBaseObject() ||
            !activeEffect->target || !activeEffect->caster) {
            continue;
        }

        // Match the Effect to the Spell Cast (using handles now for caster match)
        // logger::info("Checking if activeEffect->spell == castInfo.spellItem: {}", activeEffect->spell->GetFormID() ==
        // castInfo.spellItem->GetFormID()); logger::info("Active Effect spell name: {}, {:#010x}",
        // activeEffect->spell->GetName(), activeEffect->spell->GetFormID()); logger::info("CastInfo spell name: {},
        // {:#010x}", castInfo.spellItem->GetName(), castInfo.spellItem->GetFormID()); logger::info("ActtiveEffect spell
        // ") RE::SpellItem* spellItem = activeEffect->spell->As<RE::SpellItem>(); if (spellItem) {
        //     logger::info("    Active effect associated with spell: {:#010x} - {}", spellItem->GetFormID(),
        //     spellItem->GetName());
        // }
        if (activeEffect->spell->GetFormID() == castInfo.spellItem.GetFormID() &&
            activeEffect->caster == castInfo.playerHandle &&  // Compare handles
            activeEffect->target->MagicTargetIsActor() &&
            activeEffect->target == player->GetMagicTarget()  // caster == target
            // activeEffect->elapsedSeconds < timeThreshold
        ) {
            foundAppliedEffect = true;

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

            if (isSpellSaved) {
                logger::info("-----------------Dispelling effect from saved spell: {:#010x} ({})-----------------",
                             activeEffect->GetBaseObject()->GetFormID(), mgefName);
                activeEffect->Dispel(false);  // Remove the effect from the actor
            } else {
                if (activeEffect->duration > 0.0f &&
                    activeEffect->duration < permanentSpellDuration) {  // Only if it has a duration
                    activeEffect->duration = permanentSpellDuration;
                    SpellDataPersistence::CacheSpellForSaving(&castInfo.spellItem);
                }
            }
            // break;
        }
    }

    // DisableTimerOfEffectsOnSpell(castInfo.spellItem);  // Disable the timer of the spell effects

    // for(RE::Effect* effect : castInfo.spellItem->effects) {
    //     if (!effect) {
    //         continue;
    //     }
    //     effect->effectItem.duration = 0;  // Set duration to 0 so the timer is hidden
    // }

    // spellItem->effects[0]->effectItem.duration = 0;  // Set duration to 0 for the first effect

    if (isSpellSaved) {
        SpellDataPersistence::RemoveSpellFromSave(castInfo.spellItem.GetFormID());
        SKSE::log::info("Spell '{}' ({:#010x}) is no longer saved.", spellName, castInfo.spellItem.GetFormID());
        return;
    }
}